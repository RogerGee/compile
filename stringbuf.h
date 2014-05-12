/* stringbuf.h */
#ifndef STRINGBUF_H
#define STRINGBUF_H

/* string_buffer - a simple type
   that represents a null-terminated
   string */
typedef struct {
    char* buffer;
    int used; /* how many characters are used not including the null character */
    int size; /* how many characters are available in 'buffer' including the null character (allocation size) */
} stringbuf;

void init_stringbuf(stringbuf*);
void destroy_stringbuf(stringbuf*);
void grow_stringbuf(stringbuf*); /* grow string buffer - copy existing buffer into new buffer */
void assign_stringbuf(stringbuf*,const char* str);
void assign_stringbuf_ex(stringbuf*,const char* string,int n); /* assign up to null terminator or the first n bytes */
void concat_stringbuf(stringbuf*,const char* str);
void concat_stringbuf_ex(stringbuf*,const char* str,int n); /* concatenate up to null terminator or the first n bytes */
void truncate_stringbuf(stringbuf*,int length); /* truncate string up to specified length */
void append_terminator_stringbuf(stringbuf*); /* appends a zero byte to the end of the string */
void reset_stringbuf(stringbuf*);

#endif
