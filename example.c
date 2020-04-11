//! @file example.c
//!     This is a C source file used to test cdoc.
//! @license 0BSD

#include <stddef.h> // size_t
#include <stdint.h> // [u]intN_t

//! @function swap
//!     Exchange two objects of equal size.
//! @param p1
//!     Pointer to the first object.
//! @param p2
//!     Pointer to the second object.
//! @param size
//!     The sizeof both objects.
//! @note
//!     Objects p1 and p2 may point to overlapping memory.
//!     E.g swap(&foo, &foo, sizeof(foo)) is valid.
void swap(void* p1, void* p2, size_t size);

//! @struct string
//!     POD type representing a byte-array with a known size.
struct string
{
    //! @member data
    //!     Underlying byte buffer for this string.
    //! @note
    //!     Although the type of data is char* there are no guarantees about the
    //!     encoding of the buffer. Do <strong>NOT</strong> assume that data
    //!     points to an ASCII byte string.
    char* data;

    //! @member size
    //!     Length of data in bytes.
    size_t size;
};

//! @union foobar
//!     A data packet sent over the wire from FooBar Inc.
union foobar
{
    uint32_t foo;
    float bar;
};

//! @enum color
//! @todo
//!     Add more colors.
typedef enum
{
    RED = 0,
    BLUE = 1,
    GREEN = 2
} color;

//! @typedef colour
//!     Convenient typedef for non-American software engineers.
typedef color colour;

//! @function get_color
//! Cdoc should be lenient with whitespace.
//!     Any amount of indenting should be allowed.
//!	Even with tabs.
//! @return
//!     My favorite color.
color get_color(void)
{
    return RED;
}

//! @macro M_PER_KM
#define M_PER_KM 1000

//! @macro KM
//!     Convert meters into kilometers.
#define KM(meters) \
    (meters * M_PER_KM)

//! @macro NUM_FOOBAR
//!     256 is a computer-ish number, right?
#define NUM_FOOBAR 256
//! @variable foobars
union foobar foobars[NUM_FOOBAR];
