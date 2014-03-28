/* compiler_posix.c */
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h> /* needs _BSD_SOURCE or _GNU_SOURCE */
#include <errno.h>

void fatal_stop(const char* message)
{
    fprintf(stderr,"%s: fatal error: %s\n",PROGRAM_NAME,message);
    _exit(1);
}

int lookup_ext(const char** ext,const char* source)
{
    /* look through files in the current working directory */
    int top = 0;
    DIR* pdir;
    pdir = opendir(".");
    *ext = NULL;
    if (pdir != NULL)
    {
        struct dirent* ent;
        while (1)
        {
            ent = readdir(pdir);
            if (ent == NULL)
                break;
            if (ent->d_type == DT_REG)
            {
                int i;
                int len;
                char* pext;
                /* obtain file extension of entry */
                len = strlen(ent->d_name);
                pext = ent->d_name+len;
                while (len>0 && *pext!='.')
                {
                    --len;
                    --pext;
                }
                /* check to see if prefix.ext matches prefix */
                if (top<MAX_EXTENSIONS && *pext=='.' && strncmp(source,ent->d_name,len)==0)
                {
                    ext[top] = check_extension(pext);
                    if (ext[top] != NULL)
                        ++top;
                }
            }
        }
        closedir(pdir);
    }
    else if (errno == EACCES)
        fatal_stop("cannot open current directory: permission denied");
    else
        fatal_stop("cannot open current directory");
    return top;
}
