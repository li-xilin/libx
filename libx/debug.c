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
    HANDLE thread = GetCurrentThread();
    CONTEXT context;
    STACKFRAME64 frame; /* in/out stackframe */
    DWORD imageType;
    int i = 0;

    if (!SymInitialize_called) {
        SymInitialize(process, NULL, TRUE);
        SymInitialize_called = 1;
    }
    RtlCaptureContext(&context);
    memset(&frame, 0, sizeof(frame));
#ifdef _M_IX86
    imageType = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    imageType = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rsp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    imageType = IMAGE_FILE_MACHINE_IA64;
    frame.AddrPC.Offset = context.StIIP;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.IntSp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrBStore.Offset = context.RsBSP;
    frame.AddrBStore.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.IntSp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    #error "Platform not supported!"
#endif

	for (i = 0; i < X_TRACE_MAX; i++) {
        if (!StackWalk64(imageType, process, thread, &frame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            trace->frame[i].pc_offset = frame.AddrPC.Offset;
            trace->frame[i].ret_offset = STACKTRACE_ERROR_FLAG | GetLastError(); /* mark error */
			i += 1;
            break;
        }
		trace->frame[i].pc_offset = frame.AddrPC.Offset;
		trace->frame[i].ret_offset = frame.AddrReturn.Offset;
    }
	trace->depth = i;
	x_fprintf(stderr, x_u("depth = %zd"), trace->depth);
	exit(1);
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
		IMAGEHLP_SYMBOL64 symbol;
		WCHAR name[1023];
	} symbol_buf;
#pragma pack(pop)
    symbol_buf.symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
    symbol_buf.symbol.MaxNameLength = x_arrlen(symbol_buf.name) + 1;
    for (int i = 0; i < trace->depth; i++) {
        IMAGEHLP_LINE64 lineData;
        DWORD lineOffset = 0;
        DWORD64 symOffset = 0;
        if (trace->frame[i].ret_offset & STACKTRACE_ERROR_FLAG) {
            DWORD error = trace->frame[i].ret_offset & 0xFFFFFFFF;
            x_bprintf(sb, L"StackWalk64 error: %d @ %\n", error, trace->frame[i].pc_offset);
            break;
        }
        if (trace->frame[i].pc_offset == trace->frame[i].ret_offset) {
            x_bprintf(sb, L"Stack overflow @ %p\n", trace->frame[i].pc_offset);
            break;
        }
        SymGetLineFromAddr64(process, trace->frame[i].pc_offset, &lineOffset, &lineData);
        x_bprintf(sb, L"%s(%d): ", lineData.FileName, lineData.LineNumber);
        if (SymGetSymFromAddr64(process, trace->frame[i].pc_offset, &symOffset, &symbol_buf.symbol)) {
            x_bprintf(sb, L"%s\n", symbol_buf.symbol.Name);
        }
        else {
            x_bprintf(sb, L" Unkown symbol @ %p\n", trace->frame[i].pc_offset);
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

