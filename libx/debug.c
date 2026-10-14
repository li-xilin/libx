/*
 * Copyright (c) 2020-2025 Borislav Stanimirov
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to one person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "config.h"
#include "x/debug.h"
#include "x/memory.h"
#include "x/strbuf.h"
#include "x/printf.h"
#include "x/string.h"
#include "x/file.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(X_OS_WIN)

#include <x/file.h>
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>

#define STACKTRACE_ERROR_FLAG ((DWORD64)1 << 63)

static int SymInitialize_called = 0;

void x_trace_back(x_trace *trace)
{
	HANDLE process = GetCurrentProcess();
	if (!SymInitialize_called) {
		SymInitialize(process, NULL, TRUE);
		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEBUG);
		SymInitialize_called = 1;
	}
	trace->depth = RtlCaptureStackBackTrace(0, X_TRACE_MAX, trace->frame, NULL);
}

int x_trace_depth(x_trace *trace)
{
	return trace->depth;
}

void x_trace_dump(const x_trace *trace, x_strbuf *sb)
{
	HANDLE process = GetCurrentProcess();
#pragma pack(push, 1)
	struct {
		SYMBOL_INFO symbol;
		WCHAR name[1023];
	} symbol_buf;
#pragma pack(pop)
	symbol_buf.symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	symbol_buf.symbol.MaxNameLen = x_arrlen(symbol_buf.name) + 1;
	for (int i = 0; i < trace->depth; i++) {
		WCHAR wzModuleName[MAX_PATH] = {0};
		DWORD64 dwAddr = (DWORD64)(uintptr_t)trace->frame[i];
		DWORD64 displacement = 0;
		DWORD64 dwModuleBase = SymGetModuleBase64(process, dwAddr);
		DWORD dwAddrOffset = (unsigned)((DWORD)dwAddr - (DWORD)dwModuleBase);
		if (dwModuleBase && GetModuleFileNameW((HMODULE)(uintptr_t)dwModuleBase, wzModuleName, MAX_PATH))
			x_bprintf(sb, L"%S: ", wzModuleName);
		else
			x_bprintf(sb, L"??: ");
		if (!SymFromAddr(process, dwAddr, &displacement, &symbol_buf.symbol))
			x_bprintf(sb, L"at +0x%x [%p]\n", dwAddrOffset, (void*)(uintptr_t)dwAddr);
		else {
			IMAGEHLP_LINE64 line = {sizeof(IMAGEHLP_LINE64)};
			DWORD dwLineNumber = 0;
			if(SymGetLineFromAddr64(process, dwAddr, &dwLineNumber, &line))
				x_bprintf(sb, L"%s at %s:%lu\n", symbol_buf.symbol.Name, line.FileName, line.LineNumber);
			else
				x_bprintf(sb, L"%s at +0x%x [%p]\n", symbol_buf.symbol.Name, dwAddrOffset, (void*)(uintptr_t)dwAddr);
		}
	}
}

#else

#include <unistd.h>
#include <dlfcn.h>

#if defined(HAVE_EXECINFO_H) && (!defined(__ANDROID_API__) \
		|| (defined(__ANDROID_API__) && __ANDROID_API__ >= 33))

#include <execinfo.h>
#define _backtrace backtrace
#define _backtrace_symbols backtrace_symbols

#elif defined(HAVE__UNWIND_BACKTRACE)

#include <unwind.h>
struct BacktraceState {
	void** current;
	void** end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg) {
	struct BacktraceState* state = (struct BacktraceState*)arg;
	uintptr_t pc = _Unwind_GetIP(context);
	if (pc) {
		if (state->current == state->end) {
			return _URC_END_OF_STACK;
		} else {
			*state->current++ = (void*)pc;
		}
	}
	return _URC_NO_REASON;
}

int _backtrace(void** buffer, int max) {
	struct BacktraceState state = {buffer, buffer + max};
	_Unwind_Backtrace(unwindCallback, &state);
	return state.current - buffer;
}

char** _backtrace_symbols(void* const* buffer, int count) {
	if (count <= 0 || buffer == NULL)
		return NULL;
	char** symbols = (char**)malloc(sizeof(char*) * count);
	if (!symbols)
		return NULL;
	for (int i = 0; i < count; ++i) {
		Dl_info info;
		char buf[4096];
		if (dladdr(buffer[i], &info)) {
			long func_shift = -1;
			if (info.dli_saddr)
				func_shift = (long)((uintptr_t)buffer[i] - (uintptr_t)info.dli_saddr);
			const char *symbol_name = "";
			if (info.dli_sname)
				symbol_name = info.dli_sname;
			const char *file_name = "?";
			if (info.dli_fname)
				file_name = info.dli_fname;

			if (func_shift > 0)
				snprintf(buf, sizeof(buf), "%p <%s+0x%lx> at %s", buffer[i], symbol_name, func_shift, file_name);
			else
				snprintf(buf, sizeof(buf), "%p <%s+?> at %s", buffer[i], symbol_name, file_name);
		}
		else
			snprintf(buf, sizeof(buf), "%p <unknown> at ?", buffer[i]);
		symbols[i] = strdup(buf);
		if (symbols[i] == NULL) {
			for (int j = 0; j < i; ++j)
				free(symbols[j]);
			free(symbols);
			return NULL;
		}
	}
	return symbols;
}

#else

int _backtrace(void** buffer, int max)
{
	return 0;
}

char** _backtrace_symbols(void* const* buffer, int count)
{
	return NULL;
}

#endif

void x_trace_back(x_trace *trace)
{
	trace->depth = _backtrace((void **)trace->frame, X_TRACE_MAX);
}

int x_trace_depth(x_trace *trace)
{
	return trace->depth;
}

void x_trace_dump(const x_trace *trace, x_strbuf *sb)
{
	char** messages = _backtrace_symbols(trace->frame, trace->depth);
	if (!messages)
		return;
	for (int i = 0; i < trace->depth; ++i) {
		Dl_info info;
		if (dladdr(trace->frame[i], &info)) {
			long module_shift = (long)((uintptr_t)trace->frame[i] - 1 - (uintptr_t)info.dli_fbase);
			x_bprintf(sb, "%s [+0x%lx]\n", messages[i], module_shift);
		} else
			x_bprintf(sb, "%s [+?]\n", messages[i]);
	}
	free(messages);
}

#endif

inline static void char2hex(char dst[3], uint8_t src)
{
	uint8_t major = (src >> 4) , minor = src & 0x0F;
	dst[0] = (major < 0xA) ? '0' + major : 'A' + (major - 0xA);
	dst[1] = (minor < 0xA) ? '0' + minor : 'A' + (minor - 0xA);
	dst[2] = ' ';
}

static char *hexlify(const void *p, size_t size, char *out)
{
	int i;
	for (i = 0; i < size; i++)
		char2hex(out + i * 3, ((uint8_t *)p)[i]);
	for (i = 0; i < size; i++) {
		if (((char *)p)[i] > 0x20 && ((char *)p)[i] <= 0x7F)
			out[size * 3 + i] = ((char *)p)[i];
		else
			out[size * 3 + i] = '.';
	}
	out[size * 4] = '\0';
	return out;
}

void __x_dump_memory(const char *name, const void *p, size_t size, x_strbuf *sb)
{
#define DUMP_GROUP_LEN 16
	const char *m = p;
	x_bprintf(sb, x_u("memory of %s at %p for(%zu bytes):\n"), name, m, size);
	for (int i = 0; size; i += DUMP_GROUP_LEN) {
		char buf[DUMP_GROUP_LEN * 4 + 1];
		size_t ngrp = size > DUMP_GROUP_LEN ? DUMP_GROUP_LEN : size;
		hexlify(m, ngrp, buf);
		x_bprintf(sb, x_u("%04x %s\n"), i, buf);
		size -= ngrp;
		m += ngrp;
	}
	x_bprintf(sb, x_u("\n"));
}

void __x_sanitize_stub(const x_location *loc)
{
	x_fprintf(stderr, x_u("sanitization exception at %s (%s:%d)\n"), loc->func, loc->file, loc->line);
}

