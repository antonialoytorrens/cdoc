#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.0"

static void
usage(void);

static void
version(void);

static void
do_file(void);

static FILE* fp = NULL;

int
main(int argc, char** argv)
{
    bool parse_options = true;
    for (int i = 1; i < argc; ++i)
    {
        char const* const arg = argv[i];
        if (parse_options && strcmp(arg, "--help") == 0) { usage(); }
        if (parse_options && strcmp(arg, "--version") == 0) { version(); }
        if (parse_options && strcmp(arg, "--") == 0)
        {
            parse_options = false;
            continue;
        }

        fp = (strcmp(arg, "-") == 0 && !parse_options) ? stdin
                                                       : fopen(arg, "rb");
        if (fp == NULL)
        {
            perror(arg);
            exit(EXIT_FAILURE);
        }
        do_file();
        fclose(fp);
    }

    return EXIT_SUCCESS;
}

static void
usage(void)
{
    // clang-format off
    puts(
        "Usage: cdoc [OPTION]... [--] [FILE]..."                "\n"
                                                                "\n"
        "With no FILE, or when FILE is -, read standard input." "\n"
                                                                "\n"
        "Options:"                                              "\n"
        "  --help      Display usage information and exit."     "\n"
        "  --version   Display version information and exit."   "\n"
    );
    // clang-format on
    exit(EXIT_SUCCESS);
}

static void
version(void)
{
    puts(VERSION);
    exit(EXIT_SUCCESS);
}

// TODO: Change this function to implement file parsing and printing logic
// rather than just emulate `cat`.
static void
do_file(void)
{
    int c;
    while ((c = fgetc(fp)) != EOF) { fputc(c, stdout); }
}
