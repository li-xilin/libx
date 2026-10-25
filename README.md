# LIBX - A Lightweight C Extension Library

[[中文](./README_zh.md)]

## Introduction

Compared with other modern programming languages, the C standard library provides relatively limited functionality, which can make development slow and inefficient. Developers often compensate by introducing heavyweight libraries such as Glib and Apache APR, but these libraries tend to have cumbersome types and large footprints, making them unsuitable for many scenarios.

This project supplements the C standard library. It does not depend on any third-party program interfaces, and provides most of the basic tools developers expect in daily development, while deliberately avoiding advanced features such as SSL, HTTP, and ZIP. These advanced features are better implemented by dedicated libraries, which also helps keep this library lightweight.

## Features

- **Comprehensive utilities**: Covers common basic functionality; containers and algorithms meet the needs of most development scenarios.
- **Strong compatibility**: Developed based on C99, compatible with Windows, GNU/Linux, FreeBSD, OpenBSD, Android, OSX, and mainstream hardware platforms.
- **High reliability**: Comprehensive test coverage ensures the library is stable and reliable.
- **Interface design**: Uses intrusive containers to facilitate extension while minimizing internal memory allocation in containers.

## Building

The project uses Autotools for build management and provides reliable multi-platform compatibility.

```sh
$ cd libx
$ autoreconf -fiv
$ ./configure
$ make
```

Compilation options allow fine-grained feature trimming to suit resource-constrained environments such as embedded systems.

## Functionality

### Basic Capabilities

- System and architecture detection
- Compiler feature detection
- Byte sequence detection and conversion

### Containers and Algorithms

- Singly and doubly linked list
- Dynamic array
- Hash table
- Skip list
- Heap (Priority queue)
- Splay tree
- Multiple key indexer
- Deque
- Hierarchical FSM
- ID allocator
- Ring buffer
- Bitmap operations
- MT19937 pseudorandom number generation

### Hash Functions

- CRC checksum, with 32-bit and 64-bit support
- Thomas Wang integer hash and its inverse, with 32-bit and 64-bit support
- Murmur2, with 32-bit and 64-bit support
- Lookup3, with 32-bit and 64-bit support

### String Operations

- Cross-platform strings
- Rewritten printf family functions
- Full Unicode support
- Encoding conversion (GBK, EUC-KR, Big5, Shift-JIS, etc.)
- Rope container
- String buffer

### Memory Allocators

- Memory set
- Slab fixed-size memory allocator
- Arena (Linear allocator)

### Multithreading

- Threads
- Mutexes
- Condition variables
- Read-write locks
- Thread-local storage
- One-time initialization
- Thread pool
- Future/Promise model

### Platform Capabilities

- Process execution (with standard I/O read/write)
- Dynamic library loading
- Path operations
- Time calculation

### Interaction

- Signal handling
- Terminal color control
- Line editor
- Command-line argument parsing

### Networking

- Reactor event model (supports poll and epoll)
- Local IPC communication
- Asynchronous sockets
- Asynchronous DNS

### Basic Cryptography

- AES algorithm
- SHA-256 digest algorithm
- MD5 digest algorithm
- Secure random number generation

### Data Representation

- JSON operations
- JSON path search
- Base64 encoding/decoding
- INI configuration files
- UUID generation and parsing (supports generating v1, v4, and v7)

### Debugging and Error Handling

- Unit testing
- Error handling
- Static and dynamic assertions
- Stack traces
- Structured dumps
- Logging

For detailed descriptions of each header file, see [HEADERS.md](./HEADERS.md).

## About the Project

- **License**: MIT License
- **Project homepage**: [https://github.com/li-xilin/libx](https://github.com/li-xilin/libx)
- **Gitee**: [https://gitee.com/lihsilin/libx](https://gitee.com/lihsilin/libx)

Suggestions on design or implementation are welcome, as are contributions of new features.
