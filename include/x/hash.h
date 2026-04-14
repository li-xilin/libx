/*
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

#ifndef X_HASH_H
#define X_HASH_H

#include <stdint.h>
#include <stddef.h>

uint64_t x_hash64_jenkins(uint64_t key);
uint64_t x_hash64_jenkins_inv(uint64_t key);

uint32_t x_hash_murmur(const void *key, size_t len, uint32_t seed);
uint64_t x_hash64_murmur(const void *key, size_t len, uint32_t seed);
uint32_t x_hash_lookup3(const void *data, size_t size, uint32_t seed);
uint64_t x_hash64_lookup3(const void *data, size_t size, uint64_t seed);
uint32_t x_hash_crc(const void *data, size_t size, uint32_t initval);
uint64_t x_hash64_crc(const void *data, size_t size, uint64_t initval);

#endif
