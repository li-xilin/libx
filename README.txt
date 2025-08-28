A lightweight cross-platform C extension library implemented in C99 based on POSIX and WIN32

Basic Headers:
    x/macros.h       Macro functions
    x/types.h        Type definitions
    x/errno.h        Error handling
    x/assert.h       Static and dynamic assertions
    x/detect.h       Compilation environment detection macros
    x/dump.h         Data structure visualization and dumping
    x/flowctl.h      Flow control statements
    x/memory.h       Memory allocation management
    x/narg.h         Argument count calculation
    x/trick.h        Macro template functions
    x/time.h         High-precision time calculation
    x/test.h         Unit testing

File Operations:
    x/path.h         File path operations
    x/file.h         File reading and writing
    x/log.h          Logging
    x/sys.h          File system operations
    x/stat.h         File metadata retrieval
    x/dir.h          Directory reading

String Operations:
    x/unicode.h      UNICODE operations
    x/uchar.h        Cross-platform string handling
    x/printf.h       String formatting and printing
    x/string.h       String manipulation functions
    x/cliarg.h       CLI argument parsing

Data Representation:
    x/json.h         JSON string parsing
    x/jpath.h        JsonPath querying
    x/base64.h       Base64 encoding and decoding
    x/ini.h          INI configuration files

Data Structures:
    x/btnode.h       Binary tree
    x/bitmap.h       Bitmap
    x/heap.h         Heap (priority queue)
    x/hmap.h         Hash map
    x/list.h         Linked list
    x/splay.h        Splay tree
    x/pipe.h         FIFO queue
    x/rope.h         Rope
    x/strbuf.h       Dynamic contiguous string

Threading and Processes:
    x/cond.h         Condition variables
    x/mutex.h        Mutexes
    x/once.h         Atomic initialization
    x/thread.h       Thread operations
    x/tss.h          Thread-local storage
    x/tpool.h        Thread pool
    x/future.h       Future/Promise model
    x/proc.h         Process execution
    x/lib.h          Dynamic library loading

Terminal Operations:
    x/tsignal.h      Process/terminal signals
    x/tcolor.h       Terminal color support
    x/edit.h         Terminal line editor

Network Operations:
    x/event.h        Event operations
    x/reactor.h      Reactor pattern
    x/socket.h       Socket operations
    x/sockmux.h      Socket multiplexing
