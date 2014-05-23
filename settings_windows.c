/* settings_windows.c */
#include <Windows.h>
#include <Shlobj.h>

/* internal data */
static HANDLE settingsFile = INVALID_HANDLE_VALUE;
static stringbuf entryBuffer;

void fatal_stop(const char* message)
{
	fprintf(stderr,"%s: fatal error: %s\n",PROGRAM_NAME,message);
	ExitProcess(1);
}

const char* check_settings_path()
{
	/* get the user's user-profile folder path */
	static CHAR profilePath[MAX_PATH];
	static const char* const settingsFolder = "\\compile";
	int length;
	DWORD dwAttr;
	if (SHGetSpecialFolderPath(NULL,profilePath,CSIDL_PROFILE,FALSE) == FALSE) {
		fprintf(stderr,"%s: error: cannot determine user-profile path\n",PROGRAM_NAME);
		fatal_stop("targets file is unreachable");
	}
	/* compile settings directory name */
	length = strlen(profilePath);
	strcpy(profilePath+length,settingsFolder);
	/* check to see if exists as directory */
	dwAttr = GetFileAttributes(profilePath);
	if (dwAttr==INVALID_FILE_ATTRIBUTES || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		if ( !CreateDirectory(profilePath,NULL) )
			fatal_stop("cannot create settings directory");
		/* hide the settings directory */
		SetFileAttributes(profilePath,FILE_ATTRIBUTE_HIDDEN);
	}
	return profilePath;
}

const char* find_targets_file(const char* settingsFolder)
{
	static CHAR targetsPath[MAX_PATH];
	static const char* const targetsFile = "\\targets";
	int i;
	DWORD dwAttrib;
	/* compile the file name */
	i = strlen(settingsFolder);
	strcpy(targetsPath,settingsFolder);
	strcpy(targetsPath+i,targetsFile);
	/* check to see if the file is a regular file (not a directory) */
	dwAttrib = GetFileAttributes(targetsPath);
	if (dwAttrib==INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
		HANDLE hFile;
		DWORD dw;
		hFile = CreateFile(targetsPath,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_HIDDEN,NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			fatal_stop("cannot create default targets file");
		WriteFile(hFile,DEFAULT_TARGET_ENTRIES,strlen(DEFAULT_TARGET_ENTRIES),&dw,NULL);
		CloseHandle(hFile);
	}
	return targetsPath;
}

void open_settings_file(const char* fname)
{
	assert(settingsFile == INVALID_HANDLE_VALUE);
	settingsFile = CreateFile(fname,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (settingsFile == INVALID_HANDLE_VALUE) {
		fprintf(stderr,"%s: error: cannot open file '%s'\n",PROGRAM_NAME,fname);
		fatal_stop("cannot open needed settings file");
	}
	/* allocate string buffer for entry input */
	init_stringbuf(&entryBuffer);
}

void close_settings_file()
{
	assert(settingsFile != INVALID_HANDLE_VALUE);
	CloseHandle(settingsFile);
	settingsFile = INVALID_HANDLE_VALUE;
	/* deallocate entry buffer */
	destroy_stringbuf(&entryBuffer);
}

const char* read_next_entry()
{
	static int n = 0;
    static char ibuf[120];
    static const char* pbuf = NULL;
    assert(settingsFile != INVALID_HANDLE_VALUE);
    reset_stringbuf(&entryBuffer);
    while (1) {
        int len;
        char last;
        if (n <= 0) { /* (n could be less than 0) */
			DWORD dwRead;
            if ( !ReadFile(settingsFile,ibuf,120,&dwRead,NULL) )
                fatal_stop("could not read from settings file");
            else if (dwRead == 0) {
                if (entryBuffer.used == 0)
                    return NULL;
                break;
            }
			n = (int)dwRead;
            pbuf = ibuf;
        }
        len = 0;
        while (len<n && pbuf[len]!='\n')
            ++len;
		concat_stringbuf_ex(&entryBuffer,pbuf,len);
        last = len<n ? pbuf[len] : 0;
        n -= ++len; /* increment len to count pbuf[len] */
        pbuf += len;
        if (last == '\n')
            break;
    }
    return entryBuffer.buffer;
}
