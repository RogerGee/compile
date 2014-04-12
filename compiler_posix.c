/* compiler_posix.c */
#include <sys/types.h>
#include <sys/stat.h>
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
    int top;
    int srclen;
    DIR* pdir;
    top = 0;
    pdir = opendir(".");
    *ext = NULL;
    srclen = strlen(source);
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
                if (top<MAX_EXTENSIONS && *pext=='.' && len==srclen && strncmp(source,ent->d_name,srclen)==0)
                {
                    /* the extension must belong to one of the handled extensions */
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

int check_file(const char* fileName)
{
    struct stat st;
    if (stat(fileName,&st) == 0)
    {
        /* check file type */
        if ((st.st_mode & S_IFMT) != S_IFREG)
            return FILE_CHECK_NOT_REGULAR_FILE;
        /* check permissions */
        if (access(fileName,R_OK) == -1)
        {
            if (errno == EACCES)
                return FILE_CHECK_ACCESS_DENIED;
            return -1;
        }
        return FILE_CHECK_SUCCESS;
    }
    if (errno == EACCES)
        return FILE_CHECK_ACCESS_DENIED;
    if (errno == ENOENT)
        return FILE_CHECK_DOES_NOT_EXIST;
    return -1;
}
