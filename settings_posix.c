/* settings_posix.c */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>

/* internal data definitions */
static int settings_fd = -1;
static stringbuf entry_buffer;

/* internal function definitions */
void fatal_stop(const char* message)
{
    fprintf(stderr,"%s: fatal error: %s\n",PROGRAM_NAME,message);
    _exit(1);
}

const char* find_settings_file()
{
    static char fnbuf[FILENAME_MAX];
    uid_t uid;
    struct passwd* pwd;
    /* lookup user info to find home directory */
    uid = getuid();
    pwd = getpwuid(uid);
    if (pwd == NULL)
        fatal_stop("could not obtain user information for accessing settings");
    else
    {
        const char* init_dir = "/.compile"; /* path relative to home directory */
        const char* targ_fname = "/targets"; /* path relative to init_dir */
        int i;
        struct stat istat;
        short flag;
        flag = 0;
        /* compile settings directory name */
        i = strlen(pwd->pw_dir);
        strcpy(fnbuf,pwd->pw_dir);
        strcpy(fnbuf+i,init_dir);
        /* check to see if settings directory exists as directory */
        if (stat(fnbuf,&istat) == -1)
        {
            if (errno == ENOENT)
            {
                /* attempt to create settings directory */
                if (mkdir(fnbuf,S_IRWXU) == -1)
                {
                    if (errno == EACCES)
                        fprintf(stderr,"%s: error: cannot create settings directory: permission denied\n",PROGRAM_NAME);
                    else
                        fprintf(stderr,"%s: error: cannot create settings directory",PROGRAM_NAME);
                    flag = 1;
                }
            }
            else
            {
                if (errno == EACCES)
                    fprintf(stderr,"%s: error: cannot access settings directory: permission denied\n",PROGRAM_NAME);
                flag = 1;
            }
        }
        else if ( !S_ISDIR(istat.st_mode) )
        {
            fprintf(stderr,"%s: error: settings directory name exists as something other than a directory!\n",PROGRAM_NAME);
            flag = 1;
        }
        if (flag == 1)
            fatal_stop("settings directory is unreachable");
        /* compile targets file name */
        i = strlen(fnbuf);
        strcpy(fnbuf+i,targ_fname);
        /* check to see if targets file exists as regular file */
        flag = 0;
        if (stat(fnbuf,&istat) == -1)
        {
            if (errno == ENOENT)
            {
                /* attempt to create a default targets file */
                int fd;
                if ((fd = open(fnbuf,O_CREAT|O_WRONLY,S_IWUSR|S_IRUSR)) == -1)
                {
                    fprintf(stderr,"%s: error: cannot create default targets file\n",PROGRAM_NAME);
                    flag = 1;
                }
                else
                {
                    write(fd,DEFAULT_TARGET_ENTRIES,strlen(DEFAULT_TARGET_ENTRIES));
                    close(fd);
                }
            }
            else
            {
                if (errno == EACCES)
                    fprintf(stderr,"%s: error: cannot access targets file: permission denied\n",PROGRAM_NAME);
                flag = 1;
            }
        }
        else if ( !S_ISREG(istat.st_mode) )
        {
            fprintf(stderr,"%s: error: targets file name exists as something other than a regular file!\n",PROGRAM_NAME);
            flag = 1;
        }
        if (flag == 1)
            fatal_stop("targets file is unreachable");
    }
    return fnbuf;
}

void open_settings_file(const char* fname)
{
    assert(settings_fd == -1);
    settings_fd = open(fname,O_RDONLY);
    if (settings_fd == -1)
        fatal_stop("could not open targets file");
    /* allocate string buffer for entry input */
    init_stringbuf(&entry_buffer);
}

void close_settings_file()
{
    assert(settings_fd != -1);
    close(settings_fd);
    settings_fd = -1;
    /* deallocate entry buffer */
    destroy_stringbuf(&entry_buffer);
}

const char* read_next_entry()
{
    static int n = 0;
    static char ibuf[120];
    static const char* pbuf = NULL;
    assert(settings_fd != -1);
    reset_stringbuf(&entry_buffer);
    while (1)
    {
        int len;
        if (n == 0)
        {
            n = read(settings_fd,ibuf,120);
            if (n < 0)
                fatal_stop("could not read from targets file");
            else if (n == 0)
            {
                if (entry_buffer.used == 0)
                    return NULL;
                break;
            }
            pbuf = ibuf;
        }
        len = 0;
        while (len<n && ibuf[len]!='\n')
            ++len;
        concat_stringbuf_ex(&entry_buffer,pbuf,len);
        if (len<n && ibuf[len]=='\n')
            break;
        n -= len;
        pbuf += len;
    }
    return entry_buffer.buffer;
}
