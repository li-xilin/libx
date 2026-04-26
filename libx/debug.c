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

#elif defined(X_OS_DARWIN)

#include <execinfo.h>
#include <unistd.h>
#include <dlfcn.h>

void x_trace_back(x_trace *trace)
{
	ret->depth = backtrace((void **)ret->frame, X_TRACE_MAX);
}

int x_trace_depth(x_trace *trace)
{
	return trace->depth;
}

void x_trace_dump(const x_trace *trace, x_strbuf *sb)
{
	char** messages = backtrace_symbols(&trace->frame, trace->depth);
	if (!messages)
		return;
	for (int i = 0; i < trace->depth; ++i)
		x_bprintf(sb, "%s\n", messages[i]);
	free(messages);
}

#elif defined(X_OS_LINUX)

#include <execinfo.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>

void x_trace_back(x_trace *trace)
{
	trace->depth = backtrace((void **)trace->frame, X_TRACE_MAX);
}

int x_trace_depth(x_trace *trace)
{
	return trace->depth;
}

static void dump_frame(const char *frame_info, size_t name_len, uintptr_t address, x_strbuf *sb)
{
	char buf[PATH_MAX + 64];
	FILE* fp = NULL;
	snprintf(buf, 1024, "LANG=C addr2line -e '%.*s' -f -C -p %p 2>/dev/null", (int)name_len, frame_info, (void *)address);
	fp = popen(buf, "r");
	if (!fp)
		goto origin_frame;

	while (fgets(buf, sizeof(buf), fp)) {
		x_bprintf(sb, "%.*s: ", (int)name_len, frame_info);
		if (strstr(buf, "?? "))
			x_bprintf(sb, "at +0x%x\n", (unsigned int)address);
		else
			x_bprintf(sb, "%s", buf);
	}
	pclose(fp);
	return;
origin_frame:
	x_bprintf(sb, "%s at +0x%x\n", frame_info, (unsigned int)address);
}

void x_trace_dump(const x_trace *trace, x_strbuf *sb)
{
	char** addr_tab = backtrace_symbols(trace->frame, trace->depth);
	for (int i = 0; i < trace->depth; i++) {
		/* calculate load offset */
		Dl_info info;
		dladdr(trace->frame[i], &info);
		if (info.dli_fbase == (void*)0x400000)
			info.dli_fbase = NULL; /* address from executable, so don't offset */

		int name_length;
		for (name_length = 0; addr_tab[i][name_length] && addr_tab[i][name_length] != '('; name_length++);
		dump_frame(addr_tab[i], name_length, (uintptr_t)trace->frame[i] - (uintptr_t)info.dli_fbase, sb);
	}
	free(addr_tab);
}

#else

void x_trace_back(x_trace *trace)
{
}

int x_trace_depth(x_trace *trace)
{
	return 0;
}

void x_trace_dump(const x_trace *trace, x_strbuf *sb)
{
	x_bprintf(sb, "stacktrace not supported\n");
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
	x_bprintf(sb, "memory of %s at %p for(%zu bytes):\n", name, p, size);
	for (int i = 0; size; i += DUMP_GROUP_LEN) {
		char buf[DUMP_GROUP_LEN * 4 + 1];
		size_t ngrp = size > DUMP_GROUP_LEN ? DUMP_GROUP_LEN : size;
		hexlify(p, ngrp, buf);
		x_bprintf(sb, "%04x %s\n", i, buf);
		size -= ngrp;
		p += ngrp;
	}
	x_bprintf(sb, "\n");
}

