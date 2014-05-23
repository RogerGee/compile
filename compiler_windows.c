/* compiler_windows.c */
#include <Windows.h>

void fatal_stop(const char* message)
{
	fprintf(stderr,"%s: fatal error: %s\n",PROGRAM_NAME,message);
	ExitProcess(1);
}

int lookup_ext(const char** ext,const char* source)
{
	int i;
	int top;
	int length;
	const char* useSource;
	HANDLE fFindInfo;
	WIN32_FIND_DATA findData;
	/* set up out parameter */
	top = 0;
	*ext = NULL;
	/* find file name part of source string */
	length = strlen(source);
	i = length - 1;
	while (i>=0 && (source[i]!='\\' || source[i]!='/'))
		--i;
	++i;
	useSource = source+i;
	/* open needed directory */
	if (i > 0) {
		/* needed directory is specified in source string */
		stringbuf part;
        init_stringbuf(&part);
        assign_stringbuf_ex(&part,source,i); /* assign filePath\\ */
		concat_stringbuf(&part,"*"); /* concat to filePath\\* */
		fFindInfo = FindFirstFile(part.buffer,&findData);
		destroy_stringbuf(&part);
	}
	else
		/* needed directory is the current working directory */
		fFindInfo = FindFirstFile(".\\*",&findData);
	length = strlen(useSource);
	/* cycle through the directory's listing if it was successfully opened */
	if (fFindInfo != INVALID_HANDLE_VALUE) {
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				int len;
				const char* pext;
				/* find extension */
				len = strlen(findData.cFileName);
				pext = findData.cFileName+len;
				while (len>0 && *pext!='.') {
					--len;
					--pext;
				}
				/* check to see if prefix.ext matches prefix */
				if (top<MAX_EXTENSIONS && *pext=='.' && len==length && strncmp(useSource,findData.cFileName,length)==0) {
					/* see if the extension denotes a defined target */
					ext[top] = check_extension(pext);
					if (ext[top] != NULL)
						++top;
				}
			}
		} while (FindNextFile(fFindInfo,&findData) != 0);
		FindClose(fFindInfo);
	}
	else
		fatal_stop("could not open needed directory");
	return top;
}

int check_file(const char* fileName)
{
	DWORD dwAttribs;
	dwAttribs = GetFileAttributes(fileName);
	if (dwAttribs == INVALID_FILE_ATTRIBUTES) {
		if (GetLastError() == ERROR_ACCESS_DENIED)
			return FILE_CHECK_ACCESS_DENIED;
		return FILE_CHECK_DOES_NOT_EXIST;
	}
	if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
		return FILE_CHECK_NOT_REGULAR_FILE;
	return FILE_CHECK_SUCCESS;
}

int invoke_compiler(const char* compilerName,const char* arguments)
{
	int i;
	DWORD exitCode;
	stringbuf cmdLine;
	STARTUPINFO startInfo;
	PROCESS_INFORMATION processInfo;
	/* compile the command line (arguments are separated by zero bytes and contains program name) */
	init_stringbuf(&cmdLine);
	assign_stringbuf(&cmdLine,compilerName);
	i = strlen(arguments)+1; /* move past first argument which is program name */
	while ( arguments[i] ) {
		concat_stringbuf(&cmdLine," ");
		concat_stringbuf(&cmdLine,arguments+i);
		while ( arguments[i] )
			++i;
		++i;
	}
	/* prepare process start info */
	ZeroMemory(&processInfo,sizeof(PROCESS_INFORMATION));
	ZeroMemory(&startInfo,sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);
	/* run the compiler process; don't specify an application name so that
	   the program name is run through the shell which will locate the compiler */
	if (CreateProcess(NULL,cmdLine.buffer,NULL,NULL,FALSE,0,NULL,NULL,&startInfo,&processInfo) == 0)
		return -1;
	WaitForSingleObject(processInfo.hProcess,INFINITE);
	exitCode = -1;
	GetExitCodeProcess(processInfo.hProcess,&exitCode);
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);
	destroy_stringbuf(&cmdLine);
	return (int)exitCode;
}