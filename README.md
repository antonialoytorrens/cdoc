CDOC
====

A minimalist documentation generator for the C programming language.
The `cdoc` application transforms one or more C source files into plain HTML.
An example of `cdoc` documentation is as follows:

```c
//! @file readme-example.h
//!     A small file to show off cdoc

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint8_t */
#include <string.h> /* memset */

//! @macro BUFFER_MAX_LENGTH
#define BUFFER_MAX_LENGTH 255

//! @struct buffer
//!     A buffer for holding some data.
//!     This is just an example so don't think about it too hard.
struct buffer
{
    //! @member data
    //!     Contents of this buffer.
    char data[BUFFER_MAX_LENGTH];
    //! @member length
    //!     Length of the buffer in bytes.
    uint8_t length;
};

//! @function clear_buffer
//!     Zero out the contents of this buffer and set its length to zero.
//! @return
//!     The previous length of the buffer.
static inline uint8_t
clear_buffer(struct buffer* buf)
{
    uint8_t const prev = buf->length;
    memset(buf->data, 0x00, BUFFER_MAX_LENGTH);
    buf->length = 0;
    return prev;
}
```

## Building

The `cdoc` application can be built using a POSIX c99 compiler:

```sh
$ make cdoc
$ ./cdoc --help
Usage: cdoc [OPTION]... [--] [FILE]...

With no FILE, or when FILE is -, read standard input.

Options:
  --help      Display usage information and exit.
  --version   Display version information and exit.
```
