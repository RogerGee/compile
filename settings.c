/* settings.c */
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_NAME "compile"
#define PACKAGE_STRING "compile (build unknown)"
#endif

#define MAX_COMPILERS 512

extern const char* PROGRAM_NAME;

/* data internal to this unit */
static compiler loaded_compilers[MAX_COMPILERS];
static int loaded_compilers_c = 0;
static const char* const DEFAULT_TARGET_ENTRIES = ".c gcc -o$project\n";

/* functions internal to this unit */
static void fatal_stop(const char* message); /* system-specific implementation */
static const char* seek_until_space(const char* iterator);
static void seek_whitespace(const char** iterator);
static const char* check_settings_path(); /* system-specific implementation */
static const char* find_targets_file(const char* settingsDir); /* system-specific implementation */
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
    init_stringbuf(&pcomp->redirect);
    pcomp->options_c = 0;
}

void destroy_compiler(compiler* pcomp)
{
    destroy_stringbuf(&pcomp->program);
    destroy_stringbuf(&pcomp->options);
    destroy_stringbuf(&pcomp->extension);
    destroy_stringbuf(&pcomp->redirect);
    pcomp->options_c = 0;
}

void load_compiler(compiler* pcomp,const char* entry)
{
    /* entry format:
        ext program option option ... */
    int len;
    int state;
    const char* ptr;
    seek_whitespace(&entry);
    ptr = seek_until_space(entry);
    len = ptr - entry;
    assert(len > 0);
    /* read extension; note: if extension doesn't begin with
       a dot then we add it here */
    if (*entry != '.')
        concat_stringbuf(&pcomp->extension,".");
    concat_stringbuf_ex(&pcomp->extension,entry,len);
    entry = ptr+1;
    seek_whitespace(&entry);
    ptr = seek_until_space(entry);
    len = ptr - entry;
    if (len <= 0) {
        fprintf(stderr,"%s: syntax error: expected program name after extension '%s'\n",PROGRAM_NAME,pcomp->extension.buffer);
        fatal_stop("syntax error in target file");
    }
    /* read program name */
    assign_stringbuf_ex(&pcomp->program,entry,len);
    /* read options: note that options are optional; special
       option tokens are prefixed by a $ sign followed by an identifier */
    pcomp->options_c = 0;
    state = 0;
    while (*ptr) {
        entry = ptr+1;
        seek_whitespace(&entry);
        ptr = seek_until_space(entry);
        len = ptr - entry;
        if (len == 0) {
            continue;
        }

        /* Handle output redirect tokens. These will configure 'compile' to
         * redirect the compiler process's output to a file.
         */
        if (entry[0] == '>') {
            if (len > 1) {
                /* The file name token is a part of the current token:
                 *  (e.g. '>output')
                 */
                assign_stringbuf_ex(&pcomp->redirect,entry+1,len-1);
            }
            else {
                /* The file name token will appear as the next token:
                 *  (e.g. '> output')
                 */
                state = 1;
            }
            continue;
        }
        if (state == 1) {
            assign_stringbuf_ex(&pcomp->redirect,entry,len);
            state = 0;
            continue;
        }

        concat_stringbuf_ex(&pcomp->options,entry,len);
        /* separate the options by a zero byte */
        append_terminator_stringbuf(&pcomp->options);
        ++pcomp->options_c;
    }

    /* Handle invalid output redirection. */
    if (state != 0) {
        fprintf(stderr,"%s: format error: output redirection operator '<' requires operand\n",
            PROGRAM_NAME);
        fatal_stop("formatting error in target file");
    }

    /* add a final null terminator to signify the end */
    len = pcomp->options.used++;
    while (pcomp->options.used > pcomp->options.size)
        grow_stringbuf(&pcomp->options);
    pcomp->options.buffer[len] = 0;
}

void load_settings_from_file()
{
    const char* dname, *fname, *pentry;
    assert(loaded_compilers_c == 0);
    dname = check_settings_path();
    fname = find_targets_file(dname);
    open_settings_file(fname);
    while (loaded_compilers_c < MAX_COMPILERS) {
        int i;
        compiler* comp = loaded_compilers+loaded_compilers_c;
        pentry = read_next_entry();
        if (pentry == NULL)
            break;
        init_compiler(comp);
        load_compiler(comp,pentry);
        for (i = 0;i < loaded_compilers_c;++i) {
            if (strcmp(loaded_compilers[i].extension.buffer,comp->extension.buffer) == 0) {
                fprintf(stderr,"%s: warning: extension '%s' appear in targets file multiple times\n",PROGRAM_NAME,comp->extension.buffer);
                fprintf(stderr,"%s: warning: using first occurrance of extension '%s' in targets file\n",PROGRAM_NAME,comp->extension.buffer);
                break;
            }
        }
        ++loaded_compilers_c;
    }
    close_settings_file();
}

void unload_settings()
{
    int i = 0;
    while (i < loaded_compilers_c) {
        destroy_compiler(loaded_compilers+i);
        ++i;
    }
    loaded_compilers_c = 0;
}

compiler* lookup_compiler(const char* ext)
{
    int i;
    i = 0;
    while (i < loaded_compilers_c) {
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
    while (i < loaded_compilers_c) {
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
