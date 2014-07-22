/* compile.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h" /* gets settings.h */

/* globals */
const char* PROGRAM_NAME;
const unsigned short PROGRAM_MAJOR_VERSION = 2;
const unsigned short PROGRAM_MINOR_VERSION = 10; /* every increment counts as a hundreth */

static void option_help();
static void option_version();

int main(int argc,const char* argv[])
{
    int i;
    int acnt; /* number of args passed to the compiler */
    int fproceed; /* if non-zero then proceed with invokation */
    char const** compilerArgs; /* arguments passed to the compiler */
    if (--argc == 0) {
        fprintf(stderr,"%s: no input targets\n",argv[0]);
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
    PROGRAM_NAME = argv[0];
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
                else
                    fprintf(stderr,"%s: unknown option '%s'\n",argv[0],option);
            }
        }
        else
            compilerArgs[acnt++] = argv[i];
    }
    if (fproceed) {
        session ses;
        /* read and process settings file */
        load_settings_from_file();
        init_session(&ses,acnt);
        load_session(&ses,acnt,compilerArgs);
        compile_session(&ses);
        destroy_session(&ses);
        unload_settings();
    }
    free((void*)compilerArgs);
}

void option_help()
{
    printf("usage: compile [target files [...]] [--help] [--version] [-compiler-option value] [---compiler-long-option]\n\
\n\
Written by Roger Gee <rpg11a@acu.edu\n");
}

void option_version()
{
    char versionString[20];
    sprintf(versionString,"%hu.%.2hu",PROGRAM_MAJOR_VERSION,PROGRAM_MINOR_VERSION);
    printf("%s %s\n",PROGRAM_NAME,versionString);
}
