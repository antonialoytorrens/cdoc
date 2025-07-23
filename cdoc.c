/*!
 * @file cdoc.c
 * @license 0BSD
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.3a"

/*!
 * @function usage
 * Print usage information and exit.
 */
static void
usage(void);
/*!
 * @function version
 * Print version information and exit.
 */
static void
version(void);

/*!
 * @macro LINENO
 * Calculates a 1-indexed line number from the pointer offset of two
 * pointers to lines in the file.
 * @param start
 * Starting line of the file.
 * @param cur
 * Some line in the file.
 */
#define LINENO(/* char** */ start, /* char** */ cur) ((int)(cur - start + 1))

/*!
 * @function errorf
 * Write a formatted error message and exit with EXIT_FAILURE status.
 * @param fmt
 * Printf-style format string.
 * @param ...
 * Format arguments.
 * @note
 * A newline character is automatically written after the formatted error
 * message.
 */
static void
errorf(char const* fmt, ...);
/*!
 * @function xalloc
 * General purpose allocation & reallocation function that will never
 * return an invalid pointer.
 * Behavior is identical to realloc with the following exceptions:
 * (1) Allocation failure will print an error message and terminate the
 * program with EXIT_FAILURE status.
 * (2) The call xalloc(p, 0) is guaranteed to free the memory backing p.
 * A pointer p returned by xalloc may be freed with xalloc(p, 0) or
 * free(p).
 */
static void*
xalloc(void* ptr, size_t size);
/*!
 * @function read_text_file
 * Returns the contents of stream as a NUL-terminated heap-allocated
 * cstring.
 * Encountering a NUL-byte or read failure will print an error message and
 * terminate the program with EXIT_FAILURE status.
 */
static char*
read_text_file(FILE* stream);
/*!
 * @function text_to_lines
 * Transform provided NUL-terminated text string into a heap-allocated
 * NULL-terminated list of NUL-terminated cstrings by replacing newline
 * characters with NUL bytes.
 * @note
 * This function modifies text in place.
 */
static char**
text_to_lines(char* text);
/*!
 * @function is_hspace
 * Returns true if c is a horizontal whitespace character.
 * The characters '\t', '\r', and ' ' are considered horizontal whitespace.
 */
static bool
is_hspace(int c);
/*!
 * @function is_doc_comment
 * Returns true if the first non-whitespace characters of a line
 * are "/\*!", the start of a doc-comment.
 * @param line
 * NUL-terminated character string.
 */
static bool
is_doc_comment(char const* line);
/*!
 * @function clean_doc_line
 * Returns a pointer to the start of the actual content within a doc
 * comment line, skipping leading whitespace and the optional '*'.
 */
static char*
clean_doc_line(char* line);

/*!
 * @function do_file
 * Generate documentation for the provided file.
 * @param fp
 * File pointer returned from fopen.
 * The position fp's file offset should be at the beginning of the file.
 */
static void
do_file(FILE* fp);

/*!
 * @struct doc
 * Representation of a document, aka a list of doc-sections documenting the
 * a single portion of a C source file.
 *
 * The cdoc documentation:
 * <pre><code>
 * /\*!
 * * @struct foo
 * * The struct used for fooing.
 * * @note
 * * Be careful not to bar this foo.
 * *\/
 * struct foo
 * {
 * int some_member;
 * };
 * </code></pre>
 * is comprised of two sections (beginning with @struct and @note) and
 * four lines of source.
 */
struct doc
{
    /*! @member sections
     * Dynamically allocated list of sections in this doc.
     */
    struct section* sections;
    /*! @member section_count
     * Number of sections in this doc.
     */
    size_t section_count;
    /*! @member source
     * Dynamically allocated NULL-terminated list of source code associated
     * with this doc.
     * A value of NULL implies that this doc has no associated source code.
     */
    char** source;
};
/*!
 * @struct section
 * Representation of a portion of a document, specifically one facet of the
 * subject of the doc.
 * Each section has a mandatory tag, an optional name, and optional body
 * of description text.
 * The doc-lines:
 * <pre><code>
 * /\*!
 * * @struct foo
 * * The struct used for fooing.
 * *\/
 * </code></pre>
 * form one section with the tag "struct", the name "foo" and a text body
 * containing the single line of text "The struct used for fooing.".
 */
struct section
{
    /*! @member tag_start
     * Location of the first character in the tag slice.
     */
    char const* tag_start;
    /*! @member tag_len
     * Length of the tag slice.
     */
    int tag_len;

    /*! @member name_start
     * Location of the first character in the name slice.
     */
    char const* name_start;
    /*! @member name_len
     * Length of the name slice.
     * A length of zero implies that this tag has no name.
     */
    int name_len;

    /*! @member text_start
     * Location of the first line in the text body.
     */
    char** text_start;
    /*! @member text_len
     * Length of the text body.
     * A length of zero implies that this tag has no body.
     */
    int text_len;
};

/*!
 * @function parse_struct_source
 * Parse the lines of a struct forward declaration or definition using the
 * provided parse state.
 * @note
 * The method used for parsing in this function happens to work for
 * struct, union, enum, typedef, and variable declarations.
 * @todo
 * Currently this function does not handle comments (C or C++)
 * correctly which may lead to erroneous parsing.
 * This function should be modified to correctly handle comments.
 */
static char**
parse_struct_source(char*** linepp);
/*!
 * @function parse_function_source
 * Parse the lines of a function prototype or definition using the provided
 * parse state.
 */
static char**
parse_function_source(char*** linepp);
/*!
 * @function parse_macro_source
 * Parse the lines of a preprocessor macro using the provided parse state.
 */
static char**
parse_macro_source(char*** linepp);
/*!
 * @function parse_doc
 * Construct and return a doc from the provided parse state parameters.
 */
static struct doc
parse_doc(char* const* lines_begin, char*** linepp);
/*!
 * @function print_doc
 * Print a formatted representation of this doc to stdout.
 */
static void
print_doc(struct doc const d);
/*!
 * @function parse_section
 * Construct and return a section from the provided parse state parameters.
 */
static struct section
parse_section(char* const* lines_begin, char* line, char*** text_body_p);
/*!
 * @function print_section
 * Print a formatted representation of this section stdout.
 */
static void
print_section(struct section const s);

int
main(int argc, char** argv)
{
    bool parse_options = true;
    for (int i = 1; i < argc; ++i) {
        char const* const arg = argv[i];
        if (parse_options && strcmp(arg, "--help") == 0) {
            usage();
        }
        if (parse_options && strcmp(arg, "--version") == 0) {
            version();
        }
        if (parse_options && strcmp(arg, "--") == 0) {
            parse_options = false;
            continue;
        }

        bool const use_stdin = strcmp(arg, "-") == 0 && !parse_options;
        FILE* const fp = use_stdin ? stdin : fopen(arg, "rb");
        if (fp == NULL) {
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
    if (fmt == NULL) {
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
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    if ((ptr = realloc(ptr, size)) == NULL) {
        errorf("[%s] Out of memory", __func__);
    }
    return ptr;
}

static char*
read_text_file(FILE* stream)
{
    char* buf = NULL;
    size_t len = 0;

    int c;
    while ((c = fgetc(stream)) != EOF) {
        if (c == '\0') {
            errorf("Encountered illegal NUL byte");
        }
        buf = xalloc(buf, len + 1);
        buf[len++] = (char)c;
    }
    if (!feof(stream)) {
        errorf("Failed to read entire text file");
    }

    buf = xalloc(buf, len + 1);
    buf[len++] = '\0';
    return buf;
}

static char**
text_to_lines(char* text)
{
    if (text == NULL || *text == '\0') {
        char** lines = xalloc(NULL, sizeof(char*));
        lines[0] = NULL;
        return lines;
    }
    char** lines = xalloc(NULL, (strlen(text) + 1) * sizeof(char*));
    size_t count = 0;
    lines[count++] = text;

    for (; *text; ++text) {
        if (*text != '\n') {
            continue;
        }
        *text = '\0';
        lines[count++] = text + 1;
    }
    lines[count] = NULL;

    return lines;
}

static bool
is_hspace(int c)
{
    return c == '\t' || c == '\r' || c == ' ';
}

static bool
is_doc_comment(char const* line)
{
    if (line == NULL) {
        return false;
    }
    while (is_hspace(*line)) {
        line += 1;
    }
    return strncmp(line, "/*!", 3) == 0;
}

static char*
clean_doc_line(char* line)
{
    char* p = line;
    while (is_hspace(*p)) {
        p++;
    }
    if (*p == '*') {
        p++;
        if (is_hspace(*p)) {
            p++;
        }
    }
    return p;
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
    while (*linep != NULL) {
        if (!is_doc_comment(*linep)) {
            linep += 1;
            continue;
        }
        docs = xalloc(docs, (doc_count + 1) * sizeof(*docs));
        docs[doc_count++] = parse_doc(lines, &linep);
    }

    // PRINT
    for (size_t i = 0; i < doc_count; ++i) {
        print_doc(docs[i]);
    }

    // CLEANUP
    for (size_t i = 0; i < doc_count; ++i) {
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
    for (; **linepp && !parsed; *linepp += 1) {
        lines = xalloc(lines, (count + 1) * sizeof(char*));
        lines[count++] = **linepp;

        for (char* cp = **linepp; *cp != '\0'; ++cp) {
            brackets += *cp == '{';
            brackets -= *cp == '}';
            if (brackets == 0 && *cp == ';') {
                parsed = true;
            }
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
    for (; **linepp && !parsed; *linepp += 1) {
        lines = xalloc(lines, (count + 1) * sizeof(char*));
        lines[count++] = **linepp;

        for (char* cp = **linepp; *cp != '\0'; ++cp) {
            if (*cp == ';') {
                parsed = true;
                break;
            }
            else if (*cp == '{') {
                parsed = true;
                lines = xalloc(lines, (count + 1) * sizeof(char*));
                lines[count++] = "/* function definition... */";
            }
            if (*cp == '{' || *cp == ';') {
                parsed = true;
            }
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
    for (; **linepp && !parsed; *linepp += 1) {
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

    // Collect all lines within the doc comment block.
    char** block_lines = NULL;
    size_t block_line_count = 0;
    bool in_comment = true;

    char* first_line = **linepp;
    char* start = strstr(first_line, "/*!");
    if (!start) errorf("Internal error: parse_doc called on non-doc line.");
    
    char* content = start + 3; // Move past "/*!".
    char* end = strstr(content, "*/");
    if (end) {
        *end = '\0';
        in_comment = false;
    }
    
    block_lines = xalloc(block_lines, (block_line_count + 1) * sizeof(*block_lines));
    block_lines[block_line_count++] = content;

    if (in_comment) {
        *linepp += 1;
        for (; **linepp; *linepp += 1) {
            char* current_line = **linepp;
            end = strstr(current_line, "*/");
            if (end) {
                *end = '\0';
                in_comment = false;
            }
            block_lines = xalloc(block_lines, (block_line_count + 1) * sizeof(*block_lines));
            block_lines[block_line_count++] = current_line;
            if (!end) continue;
            break;
        }
    }
    
    *linepp += 1; // Consume the line with the end of the comment.

    // NULL-terminate the block lines array.
    block_lines = xalloc(block_lines, (block_line_count + 1) * sizeof(*block_lines));
    block_lines[block_line_count] = NULL;

    // Parse the collected lines into sections.
    char** current_line_p = block_lines;
    while(current_line_p && *current_line_p) { // Check added for safety
        char* cleaned_line = clean_doc_line(*current_line_p);
        if (*cleaned_line == '@') {
             d.sections = xalloc(d.sections, (d.section_count + 1) * sizeof(*d.sections));
             d.sections[d.section_count++] = parse_section(lines_begin, cleaned_line, &current_line_p);
        } else {
             current_line_p++; // Skip empty lines between sections
        }
    }
    // Duplicate an array of text‐line pointers for a section body so that
    // they remain valid after freeing the temporary block_lines buffer.
    for (size_t i = 0; i < d.section_count; ++i) {
        struct section* s = &d.sections[i];
        if (s->text_len > 0) {
            char** new_text = xalloc(NULL, (s->text_len + 1) * sizeof(char*));
            for (int j = 0; j < s->text_len; ++j) {
                new_text[j] = s->text_start[j];
            }
            new_text[s->text_len] = NULL;
            s->text_start = new_text;
        }
    }
    
    free(block_lines);

    if (d.section_count == 0) return d;

    // Parse associated source code.
    char const* const tag_start = d.sections[0].tag_start;
    size_t const len = (size_t)d.sections[0].tag_len;
#define DOC_IS(tag) (len == strlen(tag) && strncmp(tag_start, tag, len) == 0)
    if (DOC_IS("struct") || DOC_IS("union") || DOC_IS("enum")
        || DOC_IS("typedef") || DOC_IS("variable")) {
        d.source = parse_struct_source(linepp);
    }
    else if (DOC_IS("function")) {
        d.source = parse_function_source(linepp);
    }
    else if (DOC_IS("macro")) {
        d.source = parse_macro_source(linepp);
    }
#undef DOC_IS

    return d;
}

static void
print_doc(struct doc const d)
{
    for (size_t i = 0; i < d.section_count; ++i) {
        print_section(d.sections[i]);
    }
    if (d.source != NULL) {
        puts("<pre><code>");
        for (char** ln = d.source; *ln != NULL; ++ln) {
            if (!is_doc_comment(*ln)) {
                puts(*ln);
            }
        }
        puts("</code></pre>");
    }
    puts("<hr>");
}

static struct section
parse_section(char* const* lines_begin, char* line, char*** text_body_p)
{
    struct section s = {0};
    char* cp = line;

    if (*cp++ != '@') {
        errorf(
            "[line %d] Doc-section must begin with @<TAG>",
            LINENO(lines_begin, *text_body_p));
    }
    if (is_hspace(*cp) || *cp == '\0') {
        errorf("[line %d] Empty doc-comment tag", LINENO(lines_begin, *text_body_p));
    }

    // TAG
    s.tag_start = cp;
    while (!is_hspace(*cp) && (*cp != '\0')) {
        cp += 1;
    }
    s.tag_len = (int)(cp - s.tag_start);

    while (is_hspace(*cp)) {
        cp += 1;
    }

    // NAME
    s.name_start = cp;
    while (!is_hspace(*cp) && (*cp != '\0')) {
        cp += 1;
    }
    s.name_len = (int)(cp - s.name_start);

    while (is_hspace(*cp)) {
        cp += 1;
    }
    if (*cp != '\0') {
        errorf(
            "[line %d] Extra character(s) after tag line <NAME>",
            LINENO(lines_begin, *text_body_p));
    }

    // TEXT
    *text_body_p += 1;
    s.text_start = *text_body_p;
    while(**text_body_p) {
        char* peek = clean_doc_line(**text_body_p);
        if (*peek == '@') break;
        *text_body_p += 1;
    }
    s.text_len = (int)(*text_body_p - s.text_start);

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
    for (int i = 0; i < s.text_len; ++i) {
        puts(clean_doc_line(s.text_start[i]));
    }
}
