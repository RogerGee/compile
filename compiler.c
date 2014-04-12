/* compiler.c */
#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define FILE_CHECK_SUCCESS 0
#define FILE_CHECK_DOES_NOT_EXIST 1
#define FILE_CHECK_ACCESS_DENIED 2
#define FILE_CHECK_NOT_REGULAR_FILE 3
#define MAX_EXTENSIONS 5 /* maximum number of extensions to potentially examine */

extern const char* PROGRAM_NAME;

/* functions used in this unit */
static void fatal_stop(const char* message); /* system-specific implementation */
static void process_target(const char* source,stringbuf* dest,compiler** pinfo);
static int lookup_ext(const char** ext,const char* source); /* system-specific implementation */
static int check_file(const char* fileName); /* system-specific implementation - returns FILE_CHECK code */

/* platform-dependent code */

#if defined(BUILD_COMPILE_POSIX)
#include "compiler_posix.c"
#elif defined(BUILD_COMPILE_WINDOWS)
#include "compiler_windows.c"
#endif

/* platform-independent code */

void init_session(session* psession,int size)
{
    int i;
    psession->compiler_info = NULL; /* no compiler info by default */
    psession->targets = malloc(size*sizeof(stringbuf));
    for (i = 0;i<size;i++)
        init_stringbuf(psession->targets+i);
    psession->targets_c = 0;
    psession->options = malloc(size*sizeof(stringbuf));
    for (i = 0;i<size;i++)
        init_stringbuf(psession->options+i);
    psession->options_c = 0;
    psession->alloc_size = size;
}

void destroy_session(session* psession)
{
    int i;
    psession->compiler_info = NULL;
    for (i = 0;i<psession->alloc_size;i++)
        destroy_stringbuf(psession->targets+i);
    psession->targets_c = 0;
    free(psession->targets);
    for (i = 0;i<psession->alloc_size;i++)
        destroy_stringbuf(psession->options+i);
    psession->options_c = 0;
    free(psession->options);
    psession->alloc_size = 0;
}

void load_session(session* psession,int argc,const char** argv)
{
    int i, ui, ti;
    stringbuf ext;
    for (i = 0,ui = 0,ti = 0;i<argc;i++)
    {
        if (argv[i][0] == '-')
        {
            assert(ui < psession->alloc_size);
            assign_stringbuf(psession->options+ui++,argv[i]);
        }
        else
        {
            assert(ti < psession->alloc_size);
            process_target(argv[i],psession->targets+ti,&psession->compiler_info);
        }
    }
}

void compile_session(session* psession)
{
    /* test */
    printf("Target: %s\nOption: %s\nCompiler: %s\n",psession->targets[0].buffer,
        psession->options[0].buffer,psession->compiler_info->program.buffer);
}

/* definitions of internal functions in this unit */

void process_target(const char* source,stringbuf* dest,compiler** pinfo)
{
    /* assume the source is a target file; attempt to determine compiler */
    int i;
    int len;
    short found;
    int check_flag;
    const char* extensions[MAX_EXTENSIONS];
    const char** pext = extensions; /* point to first extension string */
    /* if the source has a final .ext, get a pointer to it;
       else set the pointer to NULL */
    len = strlen(source);
    *pext = source+len;
    i = len-1;
    found = 0;
    while (i>0 && **pext!='.')
    {
        --i;
        --*pext;
    }
    if (**pext != '.')
        *pext = NULL;
    else
        found = 1;
    if (*pinfo == NULL)
    {
        /* compiler info has not yet been determined; use
           the file extension of the source to lookup compiler
           info; if no extension has been specified, look up
           files to determine the extension */
        if (*pext == NULL)
        {
            /* attempt to determine file extension(s) from
               files in current directory */
            int ex_c = lookup_ext(pext,source);
            if (ex_c<=0 || *pext==NULL)
            {
                fprintf(stderr,"%s: error: target '%s' did not match any existing targetable file\n",PROGRAM_NAME,source);
                fatal_stop("cannot resolve target");
            }
            else if (ex_c > 1)
            {
                fprintf(stderr,"%s: error: target '%s' ambiguously matches multiple targetable files\n",PROGRAM_NAME,source);
                fprintf(stderr,"%s: note: suggest explicit file extension: one of:",PROGRAM_NAME);
                for (i = 0;i<ex_c;i++)
                    fprintf(stderr," %s",pext[i]);
                fprintf(stderr,"\n");
                fatal_stop("cannot resolve ambiguous targets");
            }
        }
        *pinfo = lookup_compiler(*pext);
        if (*pinfo == NULL)
        {
            fprintf(stderr,"%s: error: target '%s' does not match any targetable file type\n",PROGRAM_NAME,source);
            fatal_stop("cannot perform action with specified targets");
        }
    }
    /* copy source name to destination buffer; include extension if need be */
    if (!found) /* extension wasn't found initially; append looked-up extension to source file name */
    {
        /* assume all input files use the same extension */
        assign_stringbuf(dest,source);
        concat_stringbuf(dest,(*pinfo)->extension.buffer);
    }
    else
        /* simply assign filename to destination */
        assign_stringbuf(dest,source);
    /* check to make sure target exists */
    check_flag = check_file(dest->buffer);
    if (check_flag != FILE_CHECK_SUCCESS)
    {
        if (check_flag == FILE_CHECK_DOES_NOT_EXIST)
            fprintf(stderr,"%s: error: target '%s' does not exist\n",PROGRAM_NAME,source);
        else if (check_flag == FILE_CHECK_ACCESS_DENIED)
            fprintf(stderr,"%s: error: permission denied: cannot access target '%s'\n",PROGRAM_NAME,source);
        fatal_stop("bad target");
    }
}
