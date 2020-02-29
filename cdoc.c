#include <assert.h>
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
char** lines; // NULL-terminated list of all lines in the file.
char** linep; // Pointer to the current line.
#define LINENO ((int)(linep - lines + 1))

struct doc
{
    struct section* sections; // dynamically allocated
    size_t section_count;
};
static struct doc
parse_doc(void);
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
parse_section(void);
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

// Returns the contents of stream as a NUL-terminated heap-allocated cstring.
static char*
read_text_file(FILE* stream)
{
    char* buf = NULL;
    size_t len = 0;

    int c;
    while ((c = fgetc(stream)) != EOF)
    {
        if (c == '\0')
        {
            fputs("error: Encountered illegal NUL byte!\n", stderr);
            exit(EXIT_FAILURE);
        }
        buf = realloc(buf, len + 1);
        assert(buf != NULL);
        buf[len++] = (char)c;
    }
    if (!feof(stream))
    {
        fputs("error: Failed to read entire text file!\n", stderr);
        exit(EXIT_FAILURE);
    }

    buf = realloc(buf, len + 1);
    assert(buf != NULL);
    buf[len++] = '\0';
    return buf;
}

// Transform the NUL-terminated text into a heap-allocated NULL-ended list of
// NUL-terminated cstrings by replacing newline characters with NUL bytes.
// Modifies text in place.
static char**
text_to_lines(char* text)
{
    char** lines = realloc(NULL, (strlen(text) + 1) * sizeof(char*));
    assert(lines != NULL);
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
    assert(cp[0] == '/');
    assert(cp[1] == '/');
    assert(cp[2] == '!');
    cp += 3; // The '//!'
    while (is_hspace(*cp)) cp += 1; // Whitespace after '//!'
    return cp;
}

static void
do_file(void)
{
    char* const text = read_text_file(fp);
    lines = text_to_lines(text);
    linep = lines;

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
        docs = realloc(docs, (doc_count + 1) * sizeof(*docs));
        assert(docs != NULL);
        docs[doc_count++] = parse_doc();
    }

    // PRINT
    for (size_t i = 0; i < doc_count; ++i)
    {
        print_doc(docs[i]);
    }

    // CLEANUP
    for (size_t i = 0; i < doc_count; ++i) free(docs[i].sections);
    free(docs);
    free(text);
    free(lines);
}

static struct doc
parse_doc(void)
{
    struct doc d = {0};
    while (is_doc_line(*linep))
    {
        d.sections =
            realloc(d.sections, (d.section_count + 1) * sizeof(*d.sections));
        assert(d.sections != NULL);
        d.sections[d.section_count++] = parse_section();
    }
    return d;
}

static void
print_doc(struct doc const d)
{
    for (size_t i = 0; i < d.section_count; ++i) print_section(d.sections[i]);
    puts("<hr>");
}

static struct section
parse_section(void)
{
    struct section s = {0};
    char const* cp = doc_content_start(*linep);
    if (*cp++ != '@')
    {
        fprintf(
            stderr,
            "error(line %d): Doc-section must begin with @<TAG>\n",
            LINENO);
        exit(EXIT_FAILURE);
    }
    if (is_hspace(*cp))
    {
        fprintf(stderr, "error(line %d): Empty doc-comment tag\n", LINENO);
        exit(EXIT_FAILURE);
    }

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
    {
        fprintf(
            stderr,
            "error(line %d): Extra character(s) after tag line <NAME>\n",
            LINENO);
        exit(EXIT_FAILURE);
    }

    // TEXT
    linep += 1;
    s.text_start = linep;
    while (is_doc_line(*linep) && *doc_content_start(*linep) != '@') linep += 1;
    s.text_len = (int)(linep - s.text_start);

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
