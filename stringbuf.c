/* stringbuf.c */
#include "stringbuf.h"
#include <stdlib.h>
#include <string.h>

void init_stringbuf(stringbuf* pbuf)
{
    pbuf->buffer = malloc(20);
    pbuf->buffer[0] = 0; /* make empty string */
    pbuf->used = 0;
    pbuf->size = 20;
}

void destroy_stringbuf(stringbuf* pbuf)
{
    free(pbuf->buffer);
    pbuf->buffer = NULL;
    pbuf->used = 0;
    pbuf->size = 0;
}

void grow_stringbuf(stringbuf* pbuf)
{
    int newsz;
    newsz = pbuf->size * 2;
    if (newsz > 1) {
        int i;
        char* pnew;
        pnew = malloc(newsz);
        for (i = 0;i<pbuf->used;i++)
            pnew[i] = pbuf->buffer[i];
        pnew[i] = 0;
        free(pbuf->buffer);
        pbuf->buffer = pnew;
        pbuf->size = newsz;
    }
}

void assign_stringbuf(stringbuf* pbuf,const char* str)
{
    /* include null character */
    int sz;
    sz = strlen(str);
    while (sz >= pbuf->size)
        grow_stringbuf(pbuf);
    strcpy(pbuf->buffer,str);
    pbuf->used = sz;
}

void assign_stringbuf_ex(stringbuf* pbuf,const char* str,int n)
{
    int sz = 0;
    while (sz<n && str[sz])
        ++sz;
    n = sz;
    while (sz >= pbuf->size)
        grow_stringbuf(pbuf);
    strncpy(pbuf->buffer,str,n); /* provide null terminator */
    pbuf->buffer[n] = 0;
    pbuf->used = n;
}

void concat_stringbuf(stringbuf* pbuf,const char* str)
{
    /* include null character */
    int sz;
    sz = strlen(str);
    sz += pbuf->used;
    while (sz >= pbuf->size)
        grow_stringbuf(pbuf);
    strcpy(pbuf->buffer+pbuf->used,str);
    pbuf->used = sz;
}

void concat_stringbuf_ex(stringbuf* pbuf,const char* str,int n)
{
    /* provide null terminator */
    int sz = 0;
    char* app;
    while (sz<n && str[sz])
        ++sz;
    n = sz;
    sz += pbuf->used;
    while (sz >= pbuf->size)
        grow_stringbuf(pbuf);
    app = pbuf->buffer+pbuf->used;
    strncpy(app,str,n);
    app[n] = 0; /* provide null terminator */
    pbuf->used = sz;
}

void truncate_stringbuf(stringbuf* pbuf,int length)
{
    if (length>=0 && length<pbuf->used) {
        pbuf->buffer[length] = 0;
        pbuf->used = length;
        pbuf->size = length+1;
    }
}

void append_terminator_stringbuf(stringbuf* pbuf)
{
    ++pbuf->used; /* include last terminator in string payload */
    if (pbuf->used >= pbuf->size)
        grow_stringbuf(pbuf);
    pbuf->buffer[pbuf->used] = 0;
}

void reset_stringbuf(stringbuf* pbuf)
{
    pbuf->used = 0;
    pbuf->buffer[0] = 0;
}
