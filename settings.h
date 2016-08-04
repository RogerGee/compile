/* settings.h */
#ifndef SETTINGS_H
#define SETTINGS_H
#include "stringbuf.h"

typedef struct {
    stringbuf program; /* program name to invoke */
    /* 'options' are separated by null characters and terminated by a final null character
        e.g.: option1\0option2\0option3\0final-option\0\0 */
    stringbuf options; /* string of default options to be supplied to compiler */
    stringbuf extension; /* the file extension that maps to the compiler */
    int options_c;
    stringbuf redirect;
} compiler;

void init_compiler(compiler*);
void destroy_compiler(compiler*);
void load_compiler(compiler*,const char* entry); /* load compiler settings from entry in settings file */

/* settings file management */
void load_settings_from_file(); /* read settings file(s) to initialize settings information */
void unload_settings();
compiler* lookup_compiler(const char* ext);
const char* check_extension(const char* ext); /* returns pointer to compiler info extension string buffer on success else NULL */

#endif
