/* compiler_posix.c */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h> /* requires _GNU_SOURCE to be defined */
#include <errno.h>

void fatal_stop(const char* message)
{
    fprintf(stderr,"%s: fatal error: %s\n",PROGRAM_NAME,message);
    _exit(1);
}

int lookup_ext(const char** ext,const char* source)
{
    /* look through files in the current working directory */
    int i;
    int top;
    int srclen;
    DIR* pdir;
    const char* useSource;
    top = 0;
    *ext = NULL;
    /* locate file name part of source string */
    srclen = strlen(source);
    i = srclen - 1;
    while (i>=0 && source[i]!='/')
        --i;
    ++i;
    useSource = source+i;
    /* open needed directory */
    if (i > 0) {
        /* file location is specified in part of source string */
        stringbuf part;
        init_stringbuf(&part);
        assign_stringbuf_ex(&part,source,i);
        pdir = opendir(part.buffer);
        destroy_stringbuf(&part);
    }
    else
        /* file location is understood to be current directory */
        pdir = opendir(".");
    /* get length of source string that is used for matching */
    srclen = strlen(useSource);
    if (pdir != NULL) {
        struct dirent* ent;
        while (1) {
            ent = readdir(pdir);
            if (ent == NULL)
                break;
            if (ent->d_type == DT_REG) {
                int len;
                char* pext;
                /* obtain file extension of entry */
                len = strlen(ent->d_name);
                pext = ent->d_name+len;
                while (len>0 && *pext!='.') {
                    --len;
                    --pext;
                }
                /* check to see if prefix.ext matches prefix */
                if (top<MAX_EXTENSIONS && *pext=='.' && len==srclen && strncmp(useSource,ent->d_name,srclen)==0) {
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
    if (stat(fileName,&st) == 0) {
        /* check file type */
        if ((st.st_mode & S_IFMT) != S_IFREG)
            return FILE_CHECK_NOT_REGULAR_FILE;
        /* check permissions */
        if (access(fileName,R_OK) == -1) {
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

int invoke_compiler(const char* compilerName,const char* arguments)
{
    int i;
    int status;
    pid_t pid;
    char* argv[MAX_ARGUMENT+1]; /* include the terminating NULL ptr */
    pid = fork();
    if (pid == -1)
        return -1;
    if (pid != 0) {
        waitpid(pid,&status,0);
        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        return -1;
    }
    /* child process */
    /* TODO: hook into source parser if available */
    i = 0;
    status = 0;
    while (arguments[i] && status<MAX_ARGUMENT) {
        argv[status++] = (char*) (arguments+i);
        while ( arguments[i] )
            ++i;
        ++i;
    }
    argv[status] = NULL;
    if (execvp(compilerName,argv) == -1)
        _exit(1);
}
