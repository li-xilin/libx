/*
 * Copyright (c) 2023,2025 Li Xilin <lixilin@gmx.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "x/proc.h"
// #include "x/sys.h"
#include "x/detect.h"
#include "x/errno.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifdef X_OS_WIN

#include <x/string.h>
#include <io.h>
#include <sys/fcntl.h>
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

struct x_proc_st
{
	x_pid pid;
	HANDLE hProcess;
	FILE *in, *out, *err;
};

#define PIPE_BUFSIZ 4096

static FILE* fhopen(HANDLE hFile, const char *zMode)
{
    int fd = _open_osfhandle((intptr_t)hFile, _O_BINARY);
    if( fd != -1)
        return _fdopen(fd, zMode);
    else
        return NULL;
}

static HANDLE CreateChildProcess(LPCWSTR pFile, LPWSTR pCmdLine, HANDLE hIn, HANDLE hOut, HANDLE hErr, LPDWORD pProcessID)
{
	HANDLE hProcess = INVALID_HANDLE_VALUE;
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOW si = { 
		.cb = sizeof si,
		.dwFlags = STARTF_USESTDHANDLES,
		.hStdInput  = hIn,
		.hStdOutput = hOut,
		.hStdError  = hErr,
	};
	if (!CreateProcessW(pFile, pCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		x_eval_errno();
		goto out;
	}
	CloseHandle(pi.hThread);
	*pProcessID = pi.dwProcessId;
	hProcess = pi.hProcess;
out:
	return hProcess;
}

static BOOL ExecuteWithPipe(LPCWSTR pwzFile, LPWSTR pwzCmdLine, struct x_proc_st *proc)
{
	BOOL bSuccess = FALSE;
	HANDLE hInput[2] = { 0 }, hOutput[2] = { 0 }, hError[2] = { 0 };
	HANDLE hProcess = INVALID_HANDLE_VALUE;
	SECURITY_ATTRIBUTES saAttr;

	proc->in = proc->out = proc->err = NULL;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL; 
	if (!CreatePipe(&hInput[0], &hInput[1], &saAttr, PIPE_BUFSIZ)) {
		x_eval_errno();
		goto out;
	}
	if (!SetHandleInformation(hInput[0], HANDLE_FLAG_INHERIT, TRUE)
			|| !SetHandleInformation(hInput[1], HANDLE_FLAG_INHERIT, FALSE)) {
		x_eval_errno();
		goto out;
	}
	if (!CreatePipe(&hOutput[0], &hOutput[1], &saAttr, PIPE_BUFSIZ)) {
		x_eval_errno();
		goto out;
	}
	if (!SetHandleInformation(hOutput[1], HANDLE_FLAG_INHERIT, TRUE)
			|| !SetHandleInformation(hOutput[0], HANDLE_FLAG_INHERIT, FALSE)) {
		x_eval_errno();
		goto out;
	}
	if (!CreatePipe(&hError[0], &hError[1], &saAttr, PIPE_BUFSIZ)) {
		x_eval_errno();
		goto out;
	}
	if (!SetHandleInformation(hError[1], HANDLE_FLAG_INHERIT, TRUE)
			|| !SetHandleInformation(hError[0], HANDLE_FLAG_INHERIT, FALSE)) {
		x_eval_errno();
		goto out;
	}
	DWORD dwProcessId;
	hProcess = CreateChildProcess(pwzFile, pwzCmdLine, hInput[0],
			hOutput[1], hError[1], &dwProcessId);
	if (hProcess == INVALID_HANDLE_VALUE)
		goto out;
	proc->in = fhopen(hInput[1], "wb");
	proc->out = fhopen(hOutput[0], "rb");
	proc->err = fhopen(hError[0], "rb");
	if (!proc->in || !proc->out || !proc->err)
		goto out;
	proc->hProcess = hProcess;
	proc->pid = dwProcessId;
	bSuccess = TRUE;
out:
	if (!bSuccess) {
		if (proc->in)
			fclose(proc->in);
		else if (hInput[1])
			CloseHandle(hInput[1]);
		if (proc->out)
			fclose(proc->out);
		else if (hOutput[0])
			CloseHandle(hOutput[0]);
		if (proc->err)
			fclose(proc->err);
		else if (hError[0])
			CloseHandle(hError[0]);
	}
	if (hInput[0])
		CloseHandle(hInput[0]); 
	if (hOutput[1])
		CloseHandle(hOutput[1]);
	if (hError[1])
		CloseHandle(hError[1]);
	return bSuccess;
}

x_proc *x_proc_open(const x_uchar* file, x_uchar *const argv[])
{
	x_proc *pProc = NULL;
	LPWSTR pCmdBuf = NULL;
	CONST DWORD dwBufLen = 32 * 1024;
	if (!(pCmdBuf = HeapAlloc(GetProcessHeap(), 0, dwBufLen * sizeof(x_uchar)))) {
		x_eval_errno();
		goto fail;
	}
	DWORD dwBufOffset = 0;

	for (int i = 0; argv[i]; i++) {
		/* Construct sequence with <space,",arg[i],"> */
		BOOL bQuote = FALSE, bEscape = FALSE;
		int j;
		for (j = 0; argv[i][j]; j++) {
			if (argv[i][j] == L' ' || argv[i][j] == L'\t') {
				bQuote = TRUE;
			}
			else if (argv[i][j] == L'"' || argv[i][j] == L'\\') {
				bEscape = TRUE;
			}
		}
		if (j == 0)
			bQuote = TRUE; /* argv[i] is empty, quote it */
		LPWSTR pwzEscaped = argv[i];
		if (bEscape) {
			/* Escape special token " and \ to \" and \\*/
			LPWSTR pwzEscapeSlash = NULL;
			if (!(pwzEscapeSlash = x_wcsrepl(argv[i], L"\\", L"\\\\")))
				goto fail;

			if (!(pwzEscaped = x_wcsrepl(pwzEscapeSlash, L"\"", L"\\\""))) {
				free(pwzEscapeSlash);
				goto fail;
			}
			free(pwzEscapeSlash);
		}
		/* Do not append space char at first time, notice !!i */
		DWORD dwAppendLen = lstrlenW(pwzEscaped) + !!bQuote * 2 + !!i;
		if (dwAppendLen + dwBufOffset >= dwBufLen) {
			if (bEscape)
				free(pwzEscaped);
			errno = X_ENOBUFS;
			goto fail;
		}
		lstrcpyW(pCmdBuf + dwBufOffset, ((LPCWSTR)L" " + !i));
		if (bQuote)
			lstrcatW(pCmdBuf + dwBufOffset, L"\"");
		lstrcatW(pCmdBuf + dwBufOffset, pwzEscaped);
		if (bQuote)
			lstrcatW(pCmdBuf + dwBufOffset, L"\"");
		dwBufOffset += dwAppendLen;

		if (bEscape)
			free(pwzEscaped);
	}
	if (!(pProc = malloc(sizeof(*pProc))))
		goto fail;
	if (!ExecuteWithPipe(file, pCmdBuf, pProc))
		goto fail;
	HeapFree(GetProcessHeap(), 0, pCmdBuf);
	return pProc;
fail:
	HeapFree(GetProcessHeap(), 0, pCmdBuf);
	free(pProc);
	return NULL;
}

int x_proc_kill(const x_proc *proc)
{
	if (!proc) {
		errno = EINVAL;
		return -1;
	}
	if (!TerminateProcess(proc->hProcess, 1)) {
		x_eval_errno();
		goto fail;
	}
	return 0;
fail:
	return -1;
}

int x_proc_close(x_proc *proc)
{
	int exit_code = -1;
	DWORD dwExitCode = 0;
	if (!proc) {
		errno = X_EINVAL;
		goto out;
	}
	if (proc->in)
		fclose(proc->in);
	if (proc->out)
		fclose(proc->out);
	if (proc->err)
		fclose(proc->err);
	if (WaitForSingleObject(proc->hProcess, INFINITE)) {
		x_eval_errno();
		goto out;
	}
	if (!GetExitCodeProcess(proc->hProcess, &dwExitCode)) {
		x_eval_errno();
		goto out;
	}
	exit_code = dwExitCode;
out:
	CloseHandle(proc->hProcess);
	free(proc);
	return exit_code;
}

#else
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#define CLOSE_PIPE(pipe) do { close((pipe)[0]); close((pipe)[1]); } while(0)

/* To close all fd inherited after forked */
static x_proc *sg_proc_chain;
static pthread_mutex_t sg_lock = PTHREAD_MUTEX_INITIALIZER;

struct x_proc_st
{
	struct x_proc_st *next;
	pid_t pid;
	FILE *in, *out, *err;
};

static int _do_popen3(x_proc *proc, const x_uchar *file, x_uchar *const argv[])
{
	int retval = -1;
	int child_in[2] = { -1 };
	int child_out[2] = { -1 };
	int child_err[2] = { -1 };
	if (pipe(child_in) || pipe(child_out) || pipe(child_err)) {
		x_eval_errno();
		goto out;
	}
	pid_t cpid = fork();
	if (cpid < 0) {
		x_eval_errno();
		goto out;
	}
	if (cpid == 0) {
		if (dup2(child_in[0], 0) == -1)
			_Exit(127);
		if (dup2(child_out[1], 1) == -1)
			_Exit(127);
		if (dup2(child_err[1], 2) == -1)
			_Exit(127);
		CLOSE_PIPE(child_in);
		CLOSE_PIPE(child_out);
		CLOSE_PIPE(child_err);
		for (x_proc *p = sg_proc_chain; p; p = p->next) {
			close(fileno(p->in));
			close(fileno(p->out));
			close(fileno(p->err));
		}
		execv(file, argv);
		_Exit(127);
	}
	close(child_in[0]);
	close(child_out[1]);
	close(child_err[1]);
	proc->in = fdopen(child_in[1], "w");
	proc->out = fdopen(child_out[0], "r");
	proc->err = fdopen(child_err[0], "r");
	proc->pid = cpid;
	retval = 0;
out:
	if (retval) {
		if (child_in[0] != -1)
			CLOSE_PIPE(child_in);
		if (child_out[0] != -1)
			CLOSE_PIPE(child_out);
		if (child_err[0] != -1)
			CLOSE_PIPE(child_err);
	}
	return retval;
}

x_proc *x_proc_open(const x_uchar* file, x_uchar *const argv[])
{
	x_proc *proc = malloc(sizeof(*proc));
	if (!proc)
		return NULL;
	if (0 > _do_popen3(proc, file, argv)) {
		free(proc);
		return NULL;
	}
	pthread_mutex_lock(&sg_lock);
	proc->next = sg_proc_chain;
	sg_proc_chain = proc;
	pthread_mutex_unlock(&sg_lock);
	return proc;
}

int x_proc_kill(const x_proc *proc)
{
	int retval;
	if ((retval = kill(proc->pid, SIGKILL)))
		x_eval_errno();
	return retval;
}

int x_proc_close(x_proc *proc)
{
	pid_t wait_pid;
	pid_t pid = proc->pid;
	x_proc **p = &sg_proc_chain;
	while (*p) {
		if (*p == proc)
			goto found;
		p = &(*p)->next;
	}
	errno = EINVAL;
	return -1;

found:
	*p = (*p)->next;

	if (proc->in)
		fclose(proc->in);
	if (proc->out)
		fclose(proc->out);
	if (proc->err)
		fclose(proc->err);
	free(proc);
	int status = -1;
	while (1) {
		wait_pid = waitpid(pid, &status, 0);
		if (wait_pid == -1) {
			if (errno == EINTR)
				continue;
			x_eval_errno();
			return -1;
		}

		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		else if (WIFSIGNALED(status))
			return 1;
	}
}
#endif

FILE *x_proc_stdio(const x_proc *proc, int fd)
{
	switch (fd) {
		case 0: return proc->in;
		case 1: return proc->out;
		case 2: return proc->err;
		default:
			errno = X_EINVAL;
			return NULL;
	}
}

int x_proc_fclose(x_proc *proc, int fd)
{
	int retval = -1;
	if (!proc) {
		errno = EINVAL;
		return -1;
	}
	switch (fd) {
		case 0:
			retval = fclose(proc->in);
			proc->in = NULL;
			break;
		case 1:
			retval = fclose(proc->out);
			proc->out = NULL;
			break;
		case 2:
			retval = fclose(proc->err);
			proc->err = NULL;
			break;
		default:
			errno = X_EINVAL;
			break;
	}
	return retval;
}

x_pid x_proc_pid(const x_proc *proc)
{
	return proc->pid;
}

