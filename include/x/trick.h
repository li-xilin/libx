/*
 * Copyright (c) 2022-2024 Li Xilin <lixilin@gmx.com>
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

#include "narg.h"

#ifndef X_TRICK_H
#define X_TRICK_H

#define __X_INC_0 1
#define __X_INC_1 2
#define __X_INC_2 3
#define __X_INC_3 4
#define __X_INC_4 5
#define __X_INC_5 6
#define __X_INC_6 7
#define __X_INC_7 8
#define __X_INC_8 9
#define __X_INC_9 10
#define __X_INC_10 11
#define __X_INC_11 12
#define __X_INC_12 13
#define __X_INC_13 14
#define __X_INC_14 15
#define __X_INC_15 16
#define __X_DEC_1 0
#define __X_DEC_2 1
#define __X_DEC_3 2
#define __X_DEC_4 3
#define __X_DEC_5 4
#define __X_DEC_6 5
#define __X_DEC_7 6
#define __X_DEC_8 7
#define __X_DEC_9 8
#define __X_DEC_10 9
#define __X_DEC_11 10
#define __X_DEC_12 11
#define __X_DEC_13 12
#define __X_DEC_14 13
#define __X_DEC_15 14
#define __X_DEC_16 15
#define X_INC(n) __X_CATENATE_2(__X_INC_, n)
#define X_DEC(n) __X_CATENATE_2(__X_DEC_, n)

#define __X_OVERLOAD_N(sym, n, ...) sym##n(__VA_ARGS__)
#define X_OVERLOAD_N(sym, n, ...) __X_OVERLOAD_N(sym, n, __VA_ARGS__)
#define X_OVERLOAD(sym, ...) X_OVERLOAD_N(sym, X_NARG(__VA_ARGS__), __VA_ARGS__)

#define __X_EXPAND_1(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_2(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_3(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_4(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_5(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_6(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_7(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_8(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_9(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_10(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_11(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_12(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_13(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_14(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_15(macro, ...) macro(__VA_ARGS__)
#define __X_EXPAND_16(macro, ...) macro(__VA_ARGS__)

#define __X_EXPAND_PREFIX_0(_pre, x) x
#define __X_EXPAND_PREFIX_1(_pre, x) __X_EXPAND_1(__X_EXPAND_PREFIX_0, _pre, _pre##x)
#define __X_EXPAND_PREFIX_2(_pre, x) __X_EXPAND_2(__X_EXPAND_PREFIX_1, _pre, _pre##x)
#define __X_EXPAND_PREFIX_3(_pre, x) __X_EXPAND_3(__X_EXPAND_PREFIX_2, _pre, _pre##x)
#define __X_EXPAND_PREFIX_4(_pre, x) __X_EXPAND_4(__X_EXPAND_PREFIX_3, _pre, _pre##x)
#define __X_EXPAND_PREFIX_5(_pre, x) __X_EXPAND_5(__X_EXPAND_PREFIX_4, _pre, _pre##x)
#define __X_EXPAND_PREFIX_6(_pre, x) __X_EXPAND_6(__X_EXPAND_PREFIX_5, _pre, _pre##x)
#define __X_EXPAND_PREFIX_7(_pre, x) __X_EXPAND_7(__X_EXPAND_PREFIX_6, _pre, _pre##x)
#define __X_EXPAND_PREFIX_8(_pre, x) __X_EXPAND_8(__X_EXPAND_PREFIX_7, _pre, _pre##x)
#define __X_EXPAND_PREFIX_9(_pre, x) __X_EXPAND_9(__X_EXPAND_PREFIX_8, _pre, _pre##x)
#define __X_EXPAND_PREFIX_10(_pre, x) __X_EXPAND_10(__X_EXPAND_PREFIX_9, _pre, _pre##x)
#define __X_EXPAND_PREFIX_11(_pre, x) __X_EXPAND_11(__X_EXPAND_PREFIX_10, _pre, _pre##x)
#define __X_EXPAND_PREFIX_12(_pre, x) __X_EXPAND_12(__X_EXPAND_PREFIX_11, _pre, _pre##x)
#define __X_EXPAND_PREFIX_13(_pre, x) __X_EXPAND_13(__X_EXPAND_PREFIX_12, _pre, _pre##x)
#define __X_EXPAND_PREFIX_14(_pre, x) __X_EXPAND_14(__X_EXPAND_PREFIX_13, _pre, _pre##x)
#define __X_EXPAND_PREFIX_15(_pre, x) __X_EXPAND_15(__X_EXPAND_PREFIX_14, _pre, _pre##x)
#define __X_EXPAND_PREFIX_16(_pre, x) __X_EXPAND_16(__X_EXPAND_PREFIX_15, _pre, _pre##x)
#define X_EXPAND_PREFIX(n, _pre, x) __X_EXPAND_PREFIX_##n(_pre, x)

#define __X_PAVE_TO_0(n, macro, ...)
#define __X_PAVE_TO_1(n, macro, ...) macro(n, __VA_ARGS__)
#define __X_PAVE_TO_2(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_1(1, macro, __VA_ARGS__)
#define __X_PAVE_TO_3(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_2(2, macro, __VA_ARGS__)
#define __X_PAVE_TO_4(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_3(3, macro, __VA_ARGS__)
#define __X_PAVE_TO_5(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_4(4, macro, __VA_ARGS__)
#define __X_PAVE_TO_6(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_5(5, macro, __VA_ARGS__)
#define __X_PAVE_TO_7(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_6(6, macro, __VA_ARGS__)
#define __X_PAVE_TO_8(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_7(7, macro, __VA_ARGS__)
#define __X_PAVE_TO_9(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_8(8, macro, __VA_ARGS__)
#define __X_PAVE_TO_10(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_9(9, macro, __VA_ARGS__)
#define __X_PAVE_TO_11(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_10(10, macro, __VA_ARGS__)
#define __X_PAVE_TO_12(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_11(11, macro, __VA_ARGS__)
#define __X_PAVE_TO_13(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_12(12, macro, __VA_ARGS__)
#define __X_PAVE_TO_14(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_13(13, macro, __VA_ARGS__)
#define __X_PAVE_TO_15(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_14(14, macro, __VA_ARGS__)
#define __X_PAVE_TO_16(n, macro, ...) macro(n, __VA_ARGS__) __X_PAVE_TO_15(15, macro, __VA_ARGS__)
#define X_PAVE_TO(n, macro, ...) __X_PAVE_TO_##n(n, macro, __VA_ARGS__)

#define __X_CATENATE_1(seq) seq
#define __X_CATENATE_2(seq, ...) __X_CATENATE_1(seq##__VA_ARGS__)
#define __X_CATENATE_3(seq, ...) __X_CATENATE_2(seq##__VA_ARGS__)
#define __X_CATENATE_4(seq, ...) __X_CATENATE_3(seq##__VA_ARGS__)
#define __X_CATENATE_5(seq, ...) __X_CATENATE_4(seq##__VA_ARGS__)
#define __X_CATENATE_6(seq, ...) __X_CATENATE_5(seq##__VA_ARGS__)
#define __X_CATENATE_7(seq, ...) __X_CATENATE_6(seq##__VA_ARGS__)
#define __X_CATENATE_8(seq, ...) __X_CATENATE_7(seq##__VA_ARGS__)
#define __X_CATENATE_9(seq, ...) __X_CATENATE_8(seq##__VA_ARGS__)
#define __X_CATENATE_10(seq, ...) __X_CATENATE_9(seq##__VA_ARGS__)
#define __X_CATENATE_11(seq, ...) __X_CATENATE_10(seq##__VA_ARGS__)
#define __X_CATENATE_12(seq, ...) __X_CATENATE_11(seq##__VA_ARGS__)
#define __X_CATENATE_13(seq, ...) __X_CATENATE_12(seq##__VA_ARGS__)
#define __X_CATENATE_14(seq, ...) __X_CATENATE_13(seq##__VA_ARGS__)
#define __X_CATENATE_15(seq, ...) __X_CATENATE_14(seq##__VA_ARGS__)
#define __X_CATENATE_16(seq, ...) __X_CATENATE_15(seq##__VA_ARGS__)
#define X_CATENATE(...) X_OVERLOAD(__X_CATENATE_, __VA_ARGS__)

#define __X_TRANSFORM_3( macro, i, x)      macro(i, x)
#define __X_TRANSFORM_4( macro, i, x, ...) macro(i, x) __X_TRANSFORM_3 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_5( macro, i, x, ...) macro(i, x) __X_TRANSFORM_4 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_6( macro, i, x, ...) macro(i, x) __X_TRANSFORM_5 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_7( macro, i, x, ...) macro(i, x) __X_TRANSFORM_6 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_8( macro, i, x, ...) macro(i, x) __X_TRANSFORM_7 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_9( macro, i, x, ...) macro(i, x) __X_TRANSFORM_8 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_10(macro, i, x, ...) macro(i, x) __X_TRANSFORM_9 (macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_11(macro, i, x, ...) macro(i, x) __X_TRANSFORM_10(macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_12(macro, i, x, ...) macro(i, x) __X_TRANSFORM_11(macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_13(macro, i, x, ...) macro(i, x) __X_TRANSFORM_12(macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_14(macro, i, x, ...) macro(i, x) __X_TRANSFORM_13(macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_15(macro, i, x, ...) macro(i, x) __X_TRANSFORM_14(macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define __X_TRANSFORM_16(macro, i, x, ...) macro(i, x) __X_TRANSFORM_15(macro, __X_CATENATE_2(__X_INC_, i), __VA_ARGS__)
#define X_TRANSFORM(macro, ...) X_OVERLOAD(__X_TRANSFORM_, macro, 0, __VA_ARGS__)

#endif
