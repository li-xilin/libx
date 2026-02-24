/*
 * Copyright (c) 2026 Li Xilin <lixilin@gmx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
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
#ifndef X_COMPILER_H
#define X_COMPILER_H

#include "detect.h"

#if defined(__cplusplus)
	#if X_CPP_VERSION >= 201103L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201103L)
		#define X_THREAD_LOCAL thread_local
	#else
		#if defined(X_CC_GNU) || defined(X_CC_CLANG) || defined(X_CC_ICC)
			#define X_THREAD_LOCAL __thread
		#elif defined(X_CC_MSVC)
			#define X_THREAD_LOCAL __declspec(thread)
		#else
			#error "C++ compiler does not support thread-local storage"
		#endif
	#endif
#else
	#if X_C_VERSION >= 201112L
		#define X_THREAD_LOCAL _Thread_local
	#else
		#if defined(X_CC_GNU) || defined(X_CC_CLANG) || defined(X_CC_ICC)
			#define X_THREAD_LOCAL __thread
		#elif defined(X_CC_MSVC)
			#define X_THREAD_LOCAL __declspec(thread)
		#elif defined(X_CC_SUN) && __SUNPRO_C >= 0x590
			#define X_THREAD_LOCAL __thread
		#elif defined(X_CC_IBM) && defined(__IBMCPP__) && __IBMCPP__ >= 1210
			#define X_THREAD_LOCAL __declspec(thread)
		#else
			#error "C compiler does not support thread-local storage"
		#endif
	#endif
#endif

#if defined(X_CC_CLANG)
	#if __has_builtin(__builtin_expect_with_probability)
		#define X_COND_PROB(x,p) __builtin_expect_with_probability(!!(x), 1, (p))
	#elif __has_builtin(__builtin_expect)
		#define X_COND_PROB(x,p) __builtin_expect(!!(x), (p) >= 0.5)
	#else
		#define X_COND_PROB(x,p) (!!(x))
	#endif
	#if __has_attribute(always_inline)
		#define X_ALWAYS_INLINE __attribute__((always_inline)) inline
	#else
		#define X_ALWAYS_INLINE inline
	#endif
	#if __has_builtin(__builtin_expect)
		#define X_UNLIKELY(x) __builtin_expect(!!(x), 0)
	#else
		#define X_UNLIKELY(x) (!!(x))
	#endif
	#if __has_attribute(hot)
		#define X_HOT_CALL __attribute__((hot))
	#else
		#define X_HOT_CALL
	#endif
	#if __has_attribute(pure)
		#define X_PURE __attribute__((pure))
	#else
		#define X_PURE
	#endif
#elif defined(X_CC_GNU)
	#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 900
		#define X_COND_PROB(x, p) __builtin_expect_with_probability(!!(x), 1, (p))
	#else
		#define X_COND_PROB(x, p) __builtin_expect(!!(x), (p) >= 0.5)
	#endif
	#define X_ALWAYS_INLINE __attribute__((always_inline)) inline
	#define X_UNLIKELY(x)  __builtin_expect(!!(x), 0)
	#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403
		#define X_HOT_CALL __attribute__((hot))
	#else
		#define X_HOT_CALL
	#endif
	#define X_PURE __attribute__((pure))
#else
	#define X_COND_PROB(x, p) (!!(x))
	#define X_ALWAYS_INLINE inline
	#define X_UNLIKELY(x)  (!!(x))
	#define X_HOT_CALL
	#define X_PURE
#endif

#endif

