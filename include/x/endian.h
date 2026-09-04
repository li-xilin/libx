#ifndef X_ENDIAN_H
#define X_ENDIAN_H

#include "detect.h"

/* Detect machine byte-order */
#if defined(X_OS_WIN)
#  include <sys/param.h>
#elif defined(X_OS_MACOS)
#    include <libkern/OSByteOrder.h>
#elif defined(X_OS_LINUX) ||  defined(X_OS_CYGWIN)
#  include <endian.h>
#else
#  include <sys/endian.h>
#endif
#if defined(__BYTE_ORDER__)
#  define X_BYTE_ORDER __BYTE_ORDER__
#elif defined(__BYTE_ORDER)
#  define X_BYTE_ORDER __BYTE_ORDER
#elif defined(BYTE_ORDER)
#  define X_BYTE_ORDER BYTE_ORDER
#else
#  error "Unknown byte-order"
#endif
#if defined(__ORDER_BIG_ENDIAN__)
#  define X_BIG_ENDIAN __ORDER_BIG_ENDIAN__
#elif defined(__BIG_ENDIAN)
#  define X_BIG_ENDIAN __BIG_ENDIAN
#else
#  define X_BIG_ENDIAN 4321
#endif
#if defined(__ORDER_LITTLE_ENDIAN__)
#  define X_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#elif defined(__LITTLE_ENDIAN)
#  define X_LITTLE_ENDIAN __LITTLE_ENDIAN
#else
#  define X_LITTLE_ENDIAN 1234
#endif
#if defined(__ORDER_PDP_ENDIAN__)
#  define X_PDP_ENDIAN __ORDER_PDP_ENDIAN__
#elif defined(__ORDER_PDP_ENDIAN)
#  define X_PDP_ENDIAN __ORDER_PDP_ENDIAN
#else
#  define X_PDP_ENDIAN 3412
#endif

#endif
