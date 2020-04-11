#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.1"

static void
usage(void);

static void
version(void);

static void
do_file(FILE* fp);

#define LINENO(/* char** */ start, /* char** */ cur) ((int)(cur - start + 1))

struct doc
{
    struct section* sections; // dynamically allocated
    size_t section_count;

    // NULL-terminated list of associated source code.
    char** source; // dynamically allocated
};
static struct doc
parse_doc(char* const* lines_begin, char*** linepp);
static void
print_doc(struct doc const d);

struct section
{
    char const* tag_start;
    int tag_len;

    char const* name_start;
    int name_len; // name_len == 0 implies tag line has no name

    char** text_start;
    int text_len;
};
static struct section
parse_section(char* const* lines_begin, char*** linepp);
static void
print_section(struct section const s);

int
main(int argc, char** argv)
{
    bool parse_options = true;
    for (int i = 1; i < argc; ++i)
    {
        char const* const arg = argv[i];
        if (parse_options && strcmp(arg, "--help") == 0) usage();
        if (parse_options && strcmp(arg, "--version") == 0) version();
        if (parse_options && strcmp(arg, "--") == 0)
        {
            parse_options = false;
            continue;
        }

        bool const use_stdin = strcmp(arg, "-") == 0 && !parse_options;
        FILE* const fp = use_stdin ? stdin : fopen(arg, "rb");
        if (fp == NULL)
        {
            perror(arg);
            exit(EXIT_FAILURE);
        }
        do_file(fp);
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

static void
errorf(char const* fmt, ...)
{
    if (fmt == NULL)
    {
        fputs("error: errorf called with NULL format string\n", stderr);
        goto terminate;
    }

    va_list args;
    va_start(args, fmt);
    fputs("error: ", stderr);
    vfprintf(stderr, fmt, args);
    fputs("\n", stderr);
    va_end(args);

terminate:
    // Any error should immediately terminate the program.
    exit(EXIT_FAILURE);
}

static void*
xalloc(void* ptr, size_t size)
{
    if (size == 0) return free(ptr), NULL;
    if ((ptr = realloc(ptr, size)) == NULL) errorf("Out of memory");
    return ptr;
}

// Returns the contents of stream as a NUL-terminated heap-allocated cstring.
static char*
read_text_file(FILE* stream)
{
    char* buf = NULL;
    size_t len = 0;

    int c;
    while ((c = fgetc(stream)) != EOF)
    {
        if (c == '\0') errorf("Encountered illegal NUL byte");
        buf = xalloc(buf, len + 1);
        buf[len++] = (char)c;
    }
    if (!feof(stream)) errorf("Failed to read entire text file");

    buf = xalloc(buf, len + 1);
    buf[len++] = '\0';
    return buf;
}

// Transform the NUL-terminated text into a heap-allocated NULL-ended list of
// NUL-terminated cstrings by replacing newline characters with NUL bytes.
// Modifies text in place.
static char**
text_to_lines(char* text)
{
    char** lines = xalloc(NULL, (strlen(text) + 1) * sizeof(char*));
    size_t count = 0;
    lines[count++] = text;

    for (; *text; ++text)
    {
        if (*text != '\n') continue;
        *text = '\0';
        lines[count++] = text + 1;
    }
    lines[count - 1] = NULL;

    return lines;
}

static bool
is_hspace(int c)
{
    return c == '\t' || c == '\r' || c == ' ';
}

static bool
is_doc_line(char const* line)
{
    if (line == NULL) return false;
    while (is_hspace(*line)) line += 1;
    if (*line++ != '/') return false;
    if (*line++ != '/') return false;
    if (*line++ != '!') return false;
    return true;
}

static char const*
doc_content_start(char const* cp)
{
    while (is_hspace(*cp)) cp += 1; // Whitespace before '//!'
    if (cp[0] != '/' || cp[1] != '/' || cp[2] != '!')
        errorf("[%s] Received non-doc-line", __func__);
    cp += 3; // Length of "//!"
    while (is_hspace(*cp)) cp += 1; // Whitespace after '//!'
    return cp;
}

static void
do_file(FILE* fp)
{
    char* const text = read_text_file(fp);
    char** const lines = text_to_lines(text);
    char** linep = lines; // current line

    // PARSE
    struct doc* docs = NULL;
    size_t doc_count = 0;
    while (*linep != NULL)
    {
        if (!is_doc_line(*linep))
        {
            linep += 1;
            continue;
        }
        docs = xalloc(docs, (doc_count + 1) * sizeof(*docs));
        docs[doc_count++] = parse_doc(lines, &linep);
    }

    // PRINT
    for (size_t i = 0; i < doc_count; ++i)
    {
        print_doc(docs[i]);
    }

    // CLEANUP
    for (size_t i = 0; i < doc_count; ++i)
    {
        free(docs[i].sections);
        free(docs[i].source);
    }
    free(docs);
    free(text);
    free(lines);
}

static char**
parse_struct_source(char*** linepp)
{
    char** lines = NULL;
    size_t count = 0;

    bool parsed = false; // Are we finished parsing the source?
    int brackets = 0; // Number of '{' minus number of '}'.
    for (; !parsed; *linepp += 1)
    {
        if (*linepp == NULL) errorf("Unexpected end-of-file");
        lines = xalloc(lines, (count + 1) * sizeof(char*));
        lines[count++] = **linepp;

        for (char* cp = **linepp; *cp != '\0'; ++cp)
        {
            brackets += *cp == '{';
            brackets -= *cp == '}';
            if (brackets == 0 && *cp == ';') parsed = true;
        }
    }

    lines = xalloc(lines, (count + 1) * sizeof(char*));
    lines[count] = NULL;
    return lines;
}

static char**
parse_function_source(char*** linepp)
{
    char** lines = NULL;
    size_t count = 0;

    bool parsed = false;
    for (; !parsed; *linepp += 1)
    {
        if (*linepp == NULL) errorf("Unexpected end-of-file");
        lines = xalloc(lines, (count + 1) * sizeof(char*));
        lines[count++] = **linepp;

        for (char* cp = **linepp; *cp != '\0'; ++cp)
        {
            if (*cp == ';')
            {
                parsed = true;
                break;
            }
            else if (*cp == '{')
            {
                parsed = true;
                lines = xalloc(lines, (count + 1) * sizeof(char*));
                lines[count++] = "/* function definition... */";
            }
            if (*cp == '{' || *cp == ';') parsed = true;
        }
    }

    lines = xalloc(lines, (count + 1) * sizeof(char*));
    lines[count] = NULL;
    return lines;
}

static char**
parse_macro_source(char*** linepp)
{
    char** lines = NULL;
    size_t count = 0;

    bool parsed = false;
    for (; !parsed; *linepp += 1)
    {
        if (*linepp == NULL) errorf("Unexpected end-of-file");
        lines = xalloc(lines, (count + 1) * sizeof(char*));
        lines[count++] = **linepp;
        parsed = (**linepp)[strlen(**linepp) - 1] != '\\';
    }

    lines = xalloc(lines, (count + 1) * sizeof(char*));
    lines[count] = NULL;
    return lines;
}

static struct doc
parse_doc(char* const* lines_begin, char*** linepp)
{
    struct doc d = {0};
    while (is_doc_line(**linepp))
    {
        d.sections =
            xalloc(d.sections, (d.section_count + 1) * sizeof(*d.sections));
        d.sections[d.section_count++] = parse_section(lines_begin, linepp);
    }

    char const* const start = d.sections[0].tag_start;
    size_t const len = (size_t)d.sections[0].tag_len;
#define DOC_IS(tag) (len == strlen(tag) && strncmp(start, tag, len) == 0)
    if (DOC_IS("struct") || DOC_IS("union") || DOC_IS("enum")
        || DOC_IS("typedef") || DOC_IS("variable"))
        d.source = parse_struct_source(linepp);
    else if (DOC_IS("function"))
        d.source = parse_function_source(linepp);
    else if (DOC_IS("macro"))
        d.source = parse_macro_source(linepp);
#undef DOC_IS

    return d;
}

static void
print_doc(struct doc const d)
{
    for (size_t i = 0; i < d.section_count; ++i) print_section(d.sections[i]);
    if (d.source != NULL)
    {
        puts("<pre><code>");
        char** ln = d.source;
        while (*ln != NULL) puts(*ln++);
        puts("</code></pre>");
    }
    puts("<hr>");
}

static struct section
parse_section(char* const* lines_begin, char*** linepp)
{
    struct section s = {0};
    char const* cp = doc_content_start(**linepp);
    if (*cp++ != '@')
        errorf(
            "[line %d] Doc-section must begin with @<TAG>",
            LINENO(lines_begin, *linepp));
    if (is_hspace(*cp) || *cp == '\0')
        errorf("[line %d] Empty doc-comment tag", LINENO(lines_begin, *linepp));

    // TAG
    s.tag_start = cp;
    while (!is_hspace(*cp) && (*cp != '\0')) cp += 1;
    s.tag_len = (int)(cp - s.tag_start);

    while (is_hspace(*cp)) cp += 1;

    // NAME
    s.name_start = cp;
    while (!is_hspace(*cp) && (*cp != '\0')) cp += 1;
    s.name_len = (int)(cp - s.name_start);

    while (is_hspace(*cp)) cp += 1;
    if (*cp != '\0')
        errorf(
            "[line %d] Extra character(s) after tag line <NAME>",
            LINENO(lines_begin, *linepp));

    // TEXT
    *linepp += 1;
    s.text_start = *linepp;
    while (is_doc_line(**linepp) && *doc_content_start(**linepp) != '@')
        *linepp += 1;
    s.text_len = (int)(*linepp - s.text_start);

    return s;
}

static void
print_section(struct section const s)
{
    printf(
        "<h3>%.*s: %.*s</h3>\n",
        s.tag_len,
        s.tag_start,
        s.name_len,
        s.name_start);
    for (int i = 0; i < s.text_len; ++i)
    {
        char const* const start = doc_content_start(s.text_start[i]);
        puts(start);
    }
}
