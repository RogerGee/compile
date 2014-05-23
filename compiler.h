/* compiler.h */
#ifndef COMPILER_H
#define COMPILER_H
#define MAX_BUFFER_SIZE 100
#include "stringbuf.h"
#include "settings.h"

/* session - information required for a
   complete invocation of a compiler process */
typedef struct {
    compiler* compiler_info; /* compiler information for session */
    stringbuf project; /* project name; based on first target minus extension */
    stringbuf* targets; /* list of target files to pass to compiler */
    stringbuf* options; /* list of options supplied by user on command line */
    int targets_c; /* number of targets */
    int options_c; /* number of user supplied options used in options_user */
    int alloc_size; /* allocated number of elements per list */
} session;

void init_session(session*,int size); /* allocate string buffers for at most 'size' options per type */
void destroy_session(session*);
void load_session(session*,int argc,const char** argv); /* returns 0 on success */
int compile_session(session*); /* returns 0 on success */

#endif
