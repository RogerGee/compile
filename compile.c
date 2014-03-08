/* compile.c - invokes compiler and performs preprocessing on command-line arguements */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* macros */
#define BOOL int
#define TRUE 1
#define FALSE 0
#define MAX_ARGUMENTS 100
#define MAX_OPTION_LENGTH 50
#define VERSION "1.02"

/* constants */
const char* PROG_NAME;
const char* const COMPILER_FLAGS[] = {
    "-Wall",
    "-Werror",
    "-Wextra",
    "-Wshadow",
    "-Wfatal-errors",
    "-Wno-unused-variable",
    "-pedantic-errors",
    "--std=gnu++0x",
    NULL
};
const char HELP_TEXT[] = 
"compile by Roger Gee\n\
\
options:\n\
 --extension=[ext] - append .[ext] to input files that do not specify an extension\n\
 --compiler=[program] - use [program] as the compiler program to invoke; this is some program within the GNU Compiler Collection\n\
 --help - show this message\n\
 --version - print version information\n";

/* options */
char programOptions[][MAX_OPTION_LENGTH+1] = { /* add 1 to length to account for null terminator */
    "cpp", /* extension option: look for this extension when parsing source file names */
    "g++" /* compiler program: invoke this program and pass it the command line */
};
static const char* const VALUE_OPTIONS[] = {
    "extension=", /* specify which extension to look for */
    "compiler=", /* specify name of compiler program to invoke */
    NULL
};
static const char* const PLAIN_OPTIONS[] = {
    "help",
    "version",
    NULL
};

/* functions */
BOOL run_compiler( char* const* ); /* command-line arguments */
int process_own_options( int,const char**,const char*** ); /* original count,original source arguments, ptr. to array of strings in which to store resultant arguments */
void process_arguments( int,const char**,char** ); /* original count, in: pointer to original arguments out: pointer to altered arguments, out: pointer to memory dynamically allocated for arguments */
BOOL process_value_option( const char* ); /* source string - returns TRUE if a match was found */
void process_normal_option( int ); /* normal option index */
char* grow_string( char**,int,int* ); /* source string pointer pointer, current size, pointer to current capacity */

int main(int argc,const char* argv[])
{
    int i;
    int cnt;
    int rCode = 1; /* assume failure */
    const char** args = NULL;
    char* argmemory = NULL;
    /* cache pointer to program name */
    PROG_NAME = argv[0];
    /* process options for this program (--prefix) */
    cnt = process_own_options(argc-1,argv+1,&args);
    if (cnt > 0)
    {
        /* prepare argument string for compiler */
        process_arguments(cnt,args,&argmemory);
        /* invoke the compiler for the specified options */
        if ( run_compiler((char*const*)args) )
        {
            printf("%s: message: compilation success\n",PROG_NAME);
            rCode = 0;
        }
        else
            printf("%s: message: compilation failure\n",PROG_NAME);
    }
    else if (cnt == 0)
        printf("%s: error: no input files\n",PROG_NAME);
    else /* processed normal option */
        rCode = 0;
    /* free dynamic argument memory */
    if (args != NULL)
        free(args);
    if (argmemory != NULL)
        free(argmemory);
    /* return status code */
    return rCode;
}

BOOL run_compiler(char* const* args)
{
    pid_t pid;
    int rcode;
    pid = fork();
    if (pid == -1)
    {
        fprintf(stderr,"%s: error: fork\n",PROG_NAME);
        return FALSE;
    }
    if (pid==0 && execvp(programOptions[1],args)==-1)
    { /* child */
        fprintf(stderr,"%s: error: The compiler program '%s' could not be executed.\n",PROG_NAME,programOptions[1]);
        fprintf(stderr,"%s: message: %s\n",PROG_NAME,strerror(errno));
        exit(1);
    }
    /* parent
       allow 5 seconds for the compiler process until timeout */
    int i = 0;
    int status;
    while (i < 50) /* 50*100000ms => 5s */
    {
        status = waitpid(pid,&rcode,WNOHANG); /* query status */
        if (status == pid)
            break; /* the process's state changed */
        if (status == -1) /* the child most likely didn't run properly */
        {
            /* fprintf(stderr,"%s: error: wait\n",PROG_NAME); */
            return FALSE;
        }
        if (status == 0)
        {
            /* timeout */
            usleep(100000);
        }
        ++i;
    }
    if (status == 0) /* compiler process still running */
    {
        kill(pid,SIGKILL);
        fprintf(stderr,"%s: message: The operation timed out. The child process was killed.\n",PROG_NAME);
        return FALSE;
    }
    printf("%s: message: the child process returned exit status %d\n",PROG_NAME,WEXITSTATUS(rcode));
    return rcode==0;
}

int process_own_options(int originalc,const char** originals,const char*** results)
{
    int i, j,
        cnt; /* store count of args going to compiler */
    BOOL foundFiles = FALSE;
    cnt = 0;
    for (i = 0;i<originalc;i++)
    {
        j = 0;
        while (j < 2)
        {
            if ( !originals[i][j] || originals[i][j]!='-' )
                break;
            j++;
        }
        if (j == 2) /* matched '--' */
        {
            if ( originals[i][j] ) /* process option */
            {
                /* check plain options first */
                int k = 0;
                while ( PLAIN_OPTIONS[k] )
                {
                    if ( strcmp(originals[i]+j,PLAIN_OPTIONS[k])==0 )
                    {
                        process_normal_option(k);
                        return -1;
                    }
                    k++;
                }
                if ( !process_value_option(originals[i]+j) )
                    fprintf(stderr,"%s: warning: ignoring unrecognized option '%s' in command-line\n",PROG_NAME,originals[i]);
                /* mark argument as null */
                originals[i] = 0;
            }
            else
                fprintf(stderr,"%s: warning: ignoring empty option '--'\n",PROG_NAME);
        }
        else /* count arg as going to the compiler */
        {
            if (originals[i][0] != '-')
                foundFiles = TRUE;
            ++cnt;
        }
    }
    if (foundFiles)
    {
        BOOL useFlags = strcmp(programOptions[1],"g++")==0;
        cnt++; /* count compiler program name argument */
        cnt++; /* count output option */
        /* count compiler flags if using g++ */
        if (useFlags)
        {
            i = 0;
            while (COMPILER_FLAGS[i])
            {
                cnt++;
                i++;
            }
        }
        *results = malloc(sizeof(char*)*(cnt+1)); /* account for null argument at the end of argument list */
        /* add compiler program name as first argument */
        (*results)[0] = programOptions[1];
        /* this will be used for a default output option */
        (*results)[1] = NULL;
        i = 2;
        if (useFlags)
        {
            /* add compiler flags */
            j = 0;
            while (COMPILER_FLAGS[j])
                (*results)[i++] = COMPILER_FLAGS[j++];
        }
        for (j = 0;i<cnt && j<originalc;i++,j++)
        {
            while (originals[j] == NULL)
                ++j;
            (*results)[i] = originals[j];
        }
        /* add null argument at the end */
        (*results)[cnt] = NULL;
        return cnt;
    }
    return 0;
}

void process_arguments(int originalc,const char** originals,char** sbuffer)
{
    int i;
    int sz = 0, /* size of sbuffer */
        cap = 0; /* capacity of sbuffer */
    int extSz = 0;
    BOOL switchPart = FALSE;
    const char* firstFile = NULL;
    /* find first file name (there should be one) */
    i = 2;
    while (i < originalc)
    {
        if (originals[i][0] != '-')
        {
            firstFile = originals[i];
            break;
        }
        i++;
    }
    /* add output name of first named file */
    if (firstFile != NULL)
    {
        int j, offset;
        char* space;
        /* compute length of first file not counting its extension */
        while (firstFile[extSz])
            ++extSz;
        j = extSz;
        while (j>=0 && firstFile[j]!='.' && firstFile[j]!='/')
            --j;
        if (j>0 && firstFile[j]=='.')
            extSz = j;
        offset = extSz;
        while (offset>=0 && firstFile[offset]!='/')
            --offset;
        if (offset >= 0)
            ++offset;
        else
            offset = 0;
        extSz -= offset;
        extSz += 3; /* count "-o" and null terminator */
        while (cap < extSz)
            grow_string(sbuffer,sz,&cap);
        space = sbuffer[sz];
        sz = extSz;
        space[0] = '-';
        space[1] = 'o';
        j = sz-1;
        for (i = 2;i<j;i++,offset++)
            space[i] = firstFile[offset];
        space[j] = 0; /* null terminate string */
    }
    /* compute size of extension */
    extSz = 0;
    while (programOptions[0][extSz])
        ++extSz;
    ++extSz; /* account for dot */
    /* compute new arguments based off the specified */
    for (i = 2;i<originalc;i++)
    {
        const char* arg = originals[i];
        char* barg; /* point in string buffer to store argument */
        /* compute argument size */
        int argSz = 0;
        while (arg[argSz])
            ++argSz;
        /* ensure the string buffer is large enough */
        argSz += sz;
        while (cap < argSz)
            grow_string(sbuffer,sz,&cap);
        barg = *sbuffer+sz; /* make barg point to start of argument */
        sz = argSz;
        /* copy argument to string buffer */
        argSz = 0;
        while (arg[argSz])
        {
            barg[argSz] = arg[argSz];
            ++argSz;
        }
        /* check for file extensions */
        if (arg[0]!='-') /* is not option */
        {
            int j = argSz-1;
            while (j>0 && arg[j]!='.' & arg[j]!='/')
                --j;
            if (arg[j] != '.')
            {
                char* ext;
                /* ensure the string buffer is large enough */
                argSz = sz+extSz;
                while (cap < argSz)
                    grow_string(sbuffer,sz,&cap);
                ext = *sbuffer+sz;
                sz = argSz;
                /* copy extension to buffer */
                *ext++ = '.'; /* add default dot for extension */
                argSz = 0;
                while (programOptions[0][argSz])
                {
                    ext[argSz] = programOptions[0][argSz];
                    ++argSz;
                }
            }
        }
        /* add null terminator */
        argSz = sz+1;
        while (cap < argSz) /* ensure that the buffer is large enough */
            grow_string(sbuffer,sz,&cap);
        (*sbuffer)[sz] = 0;
        sz = argSz;
    }
    /* place a final null terminator at the end */
    i = sz+1;
    while (cap < i)
        grow_string(sbuffer,sz,&cap);
    (*sbuffer)[sz] = 0;
    /* change string pointers in original argument array */
    i = 1;
    sz = 0;
    while (i<originalc && (*sbuffer)[sz])
    {
        originals[i] = *sbuffer+sz;
        while ( (*sbuffer)[sz] ) /* move to the end of the argument */
            ++sz;
        ++i;
        ++sz;
    }
}

/* handle any options that have a user-supplied value */
BOOL process_value_option(const char* option)
{
    int i = 0;
    while ( VALUE_OPTIONS[i] )
    {
        const char* poption1 = option;
        const char* poption2 = VALUE_OPTIONS[i];
        /* compare option strings */
        while (*poption1 && *poption2 && *poption1!='=' && *poption2!='=' && *poption1==*poption2)
        {
            ++poption1;
            ++poption2;
        }
        if (*poption1=='=' && *poption2=='=')
        {
            /* option matched - process value */
            int j = 0;
            ++poption1;
            while (*poption1 && j<MAX_OPTION_LENGTH)
                programOptions[i][j++] = *poption1++;
            programOptions[i][j] = 0; /* null terminate string */
            return TRUE;
        }
        ++i;
    }
    /* option did not match valid option value */
    return FALSE;
}

void process_normal_option(int optionIndex)
{
    if (optionIndex == 0) /* help */
        printf(HELP_TEXT);
    else if (optionIndex == 1) /* version */
        printf("compile by Roger Gee\nVersion: %s\n",VERSION);
}

char* grow_string(char** pps,int sz,int* pcap)
{
    int i;
    char* newBuffer;
    /* calculate new buffer capacity */
    *pcap = *pcap==0 ? 20 : *pcap*2;
    /* allocate new buffer */
    newBuffer = malloc(*pcap);
    /* copy old data */
    for (i = 0;i<sz && i<*pcap;i++)
        newBuffer[i] = (*pps)[i];
    /* delete old buffer */
    if (*pps != NULL)
        free(*pps);
    /* assign new buffer */
    *pps = newBuffer;
    return newBuffer;
}
