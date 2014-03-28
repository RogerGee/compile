/* settings.c */
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define MAX_COMPILERS 512

extern const char* PROGRAM_NAME;

/* data internal to this unit */
static compiler loaded_compilers[MAX_COMPILERS];
static int loaded_compilers_c = 0;
static const char* const DEFAULT_TARGET_ENTRIES = ".c gcc";

/* functions internal to this unit */
static void fatal_stop(const char* message); /* system-specific implementation */
static const char* seek_until_space(const char* iterator);
static void seek_whitespace(const char** iterator);
static const char* find_settings_file(); /* system-specific implementation */
static void open_settings_file(const char* fname); /* system-specific implementation */
static void close_settings_file(); /* system-specific implementation */
static const char* read_next_entry(); /* system-specific implementation */

/* platform-dependent code */

#if defined(BUILD_COMPILE_POSIX)
#include "settings_posix.c"
#elif defined(BUILD_COMPILE_WINDOWS)
#include "settings_windows.c"
#endif

/* platform-independent code */

void init_compiler(compiler* pcomp)
{
    init_stringbuf(&pcomp->program);
    init_stringbuf(&pcomp->options);
    init_stringbuf(&pcomp->extension);
}

void destroy_compiler(compiler* pcomp)
{
    destroy_stringbuf(&pcomp->program);
    destroy_stringbuf(&pcomp->options);
    destroy_stringbuf(&pcomp->extension);
}

void load_compiler(compiler* pcomp,const char* entry)
{
    /* entry format:
        ext program option option ... */
    int len;
    const char* ptr;
    seek_whitespace(&entry);
    ptr = seek_until_space(entry);
    len = ptr - entry;
    assert(len > 0);
    /* read extension; note: if extension begins with
       a dot, the dot is excluded from the extension */
    assign_stringbuf_ex(&pcomp->extension,entry,len);
    entry = ptr+1;
    seek_whitespace(&entry);
    ptr = seek_until_space(entry);
    len = ptr - entry;
    if (len <= 0)
    {
        fprintf(stderr,"%s: syntax error: expected program name after extension '%s'\n",PROGRAM_NAME,pcomp->extension.buffer);
        fatal_stop("syntax error");
    }
    /* read program name */
    assign_stringbuf_ex(&pcomp->program,entry,len);
    /* read options: note that options are optional */
    while (*ptr)
    {
        entry = ptr+1;
        seek_whitespace(&entry);
        ptr = seek_until_space(entry);
        len = ptr - entry;
        concat_stringbuf_ex(&pcomp->options,entry,len);
        ++pcomp->options.used; /* include null terminator in used part of string buffer */
    }
    /* add a final null terminator to signify the end */
    len = pcomp->options.used++;
    while (pcomp->options.used > pcomp->options.size)
        grow_stringbuf(&pcomp->options);
    pcomp->options.buffer[len] = 0;
}

void load_settings_from_file()
{
    const char* fname, *pentry;
    assert(loaded_compilers_c == 0);
    fname = find_settings_file();
    open_settings_file(fname);
    while (loaded_compilers_c < MAX_COMPILERS)
    {
        compiler* comp = loaded_compilers+loaded_compilers_c;
        pentry = read_next_entry();
        if (pentry == NULL)
            break;
        init_compiler(comp);
        load_compiler(comp,pentry);
        ++loaded_compilers_c;
    }
    close_settings_file(fname);
}

void unload_settings()
{
    int i = 0;
    while (i < loaded_compilers_c)
    {
        destroy_compiler(loaded_compilers+i);
        ++i;
    }
    loaded_compilers_c = 0;
}

compiler* lookup_compiler(const char* ext)
{
    int i;
    i = 0;
    while (i < loaded_compilers_c)
    {
        if (strcmp(loaded_compilers[i].extension.buffer,ext) == 0)
            return loaded_compilers+i;
        ++i;
    }
    return NULL;
}

const char* check_extension(const char* ext)
{
    int i;
    i = 0;
    while (i < loaded_compilers_c)
    {
        if (strcmp(loaded_compilers[i].extension.buffer,ext) == 0)
            return loaded_compilers[i].extension.buffer;
        ++i;
    }
    return NULL;
}

/* definitions of internal functions */
const char* seek_until_space(const char* iterator)
{
    while (*iterator && !isspace(*iterator))
        ++iterator;
    return iterator;
}

void seek_whitespace(const char** iterator)
{
    while ( isspace(**iterator) )
        ++(*iterator);
}
