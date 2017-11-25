/* compile.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h" /* gets settings.h */

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define PACKAGE_NAME "compile"
#define PACKAGE_STRING "compile (build unknown)"
#endif

/* globals */
const char* PROGRAM_NAME;

static void option_help();
static void option_version();

int main(int argc,const char* argv[])
{
    int i;
    int ret = 0;
    int acnt; /* number of args passed to the compiler */
    int fproceed; /* if non-zero then proceed with invokation */
    char const** compilerArgs; /* arguments passed to the compiler */
    PROGRAM_NAME = argv[0];

    /* Read and process settings file at startup. Do this before proceeding so
     * that we can create the default targets file on startup.
     */
    load_settings_from_file();

    if (--argc == 0) {
        fprintf(stderr,"%s: no input targets\n",PROGRAM_NAME);
        return 1;
    }
    /* process arguments:
        -arg - send as option to compiler
        --arg - process as own option
        ---arg - change to --arg; send option to compiler
    */
    acnt = 0;
    fproceed = 1;
    compilerArgs = malloc(sizeof(char*)*argc);
    for (i = 1;i<=argc;i++) {
        if (argv[i][0] == '-') {
            int cnt = 1;
            while (argv[i][cnt] == '-')
                ++cnt;
            if (cnt == 1)
                /* this option goes straight to the compiler */
                compilerArgs[acnt++] = argv[i];
            else if (cnt >= 3)
                /* this option gets modified slightly to --arg */
                compilerArgs[acnt++] = argv[i]+cnt-2;
            else if (cnt == 2) {
                /* these args refer to options to this program */
                const char* option = argv[i]+2;
                fproceed = 0;
                if (strcmp(option,"help") == 0)
                    option_help();
                else if (strcmp(option,"version") == 0)
                    option_version();
                else {
                    fprintf(stderr,"%s: unknown option '%s'\n",argv[0],option);
                    ret = 1;
                }
            }
        }
        else
            compilerArgs[acnt++] = argv[i];
    }
    if (fproceed) {
        session ses;
        init_session(&ses,acnt);
        load_session(&ses,acnt,compilerArgs);
        ret = compile_session(&ses);
        destroy_session(&ses);
    }
    unload_settings();
    free((void*)compilerArgs);
    return ret;
}

void option_help()
{
    printf("usage: compile [target files [...]] [--help] [--version] [-compiler-option value ...]\
[---compiler-long-option ...]\n\
\n\
Written by Roger Gee <rpg11a@acu.edu\n");
}

void option_version()
{
    printf("%s\n",PACKAGE_STRING);
}
