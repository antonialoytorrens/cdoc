#include <stddef.h> // size_t
#include <stdint.h> // [u]intN_t

enum colors
{
    RED = 0,
    BLUE = 1,
    GREEN = 2
};

struct string
{
    char* data;
    size_t len;
};

union vec3
{
    struct rgb
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } rgb;

    struct xyz
    {
        uint32_t x;
        uint32_t y;
        uint32_t z;
    } xyz;
};

int
foo(char const* bar, size_t n);
