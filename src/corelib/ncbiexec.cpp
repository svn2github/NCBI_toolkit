/*  $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Authors:  Vladimir Ivanov
 *
 */

#include <ncbi_pch.hpp>
#include <stdio.h>
#include <stdarg.h>
#include <corelib/ncbiexec.hpp>
#include <corelib/ncbifile.hpp>

#if defined(NCBI_OS_MSWIN)
#  include <process.h>
#elif defined(NCBI_OS_UNIX)
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <errno.h>
#elif defined(NCBI_OS_MAC)
#  error "Class CExec defined only for MS Windows and UNIX platforms"
#endif


BEGIN_NCBI_SCOPE



TExitCode CExec::CResult::GetExitCode(void)
{
    if ( m_IsHandle ) {
        NCBI_THROW(CExecException, eResult,
                   "CExec:: CResult contains process handle, not exit code");
    }
    return m_Result.exitcode;
}

TProcessHandle CExec::CResult::GetProcessHandle(void)
{
    if ( !m_IsHandle ) {
        NCBI_THROW(CExecException, eResult,
                   "CExec:: CResult contains process exit code, not handle");
    }
    return m_Result.handle;
}


#if defined(NCBI_OS_MSWIN)

// Convert CExec class mode to the real mode
static int s_GetRealMode(CExec::EMode mode)
{
    static const int s_Mode[] =  { 
        P_OVERLAY, P_WAIT, P_NOWAIT, P_DETACH 
    };

    int x_mode = (int) mode;
    _ASSERT(0 <= x_mode  &&  x_mode < sizeof(s_Mode)/sizeof(s_Mode[0]));
    return s_Mode[x_mode];
}
#endif


#if defined(NCBI_OS_UNIX)

// Type function to call
enum ESpawnFunc {eV, eVE, eVP, eVPE};

static int 
s_SpawnUnix(ESpawnFunc func, CExec::EMode mode, 
            const char *cmdname, const char *const *argv, 
            const char *const *envp = (const char *const*)0)
{
    // Empty environment for Spawn*E
    const char* empty_env[] = { 0 };
    if ( !envp ) {
        envp = empty_env;
    }

    // Replace the current process image with a new process image.
    if (mode == CExec::eOverlay) {
        switch (func) {
        case eV:
            return execv(cmdname, const_cast<char**>(argv));
        case eVP:
            return execvp(cmdname, const_cast<char**>(argv));
        case eVE:
        case eVPE:
            return execve(cmdname, const_cast<char**>(argv), 
                          const_cast<char**>(envp));
        }
        return -1;
    }
    // Fork child process
    pid_t pid;
    switch (pid = fork()) {
    case -1:
        // fork failed
        return -1;
    case 0:
        // Here we're the child
        if (mode == CExec::eDetach) {
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "a", stdout);
            freopen("/dev/null", "a", stderr);
            setsid();
        }
        int status =-1;
        switch (func) {
        case eV:
            status = execv(cmdname, const_cast<char**>(argv));
            break;
        case eVP:
            status = execvp(cmdname, const_cast<char**>(argv));
            break;
        case eVE:
        case eVPE:
            status = execve(cmdname, const_cast<char**>(argv),
                            const_cast<char**>(envp));
            break;
        }
        _exit(status);
    }
    // The "pid" contains the childs pid
    if ( mode == CExec::eWait ) {
        return CExec::Wait(pid);
    }
    return pid;
}

#endif


// On 64-bit platforms, check argument, passed into function with variable
// number of arguments, on possible using 0 instead NULL as last argument.
// Of course, the argument 'arg' can be aligned on segment boundary,
// when 4 low-order bytes are 0, but chance of this is very low.
// The function prints out a warning only in Debug mode.
#if defined(_DEBUG)  &&  SIZEOF_VOIDP > SIZEOF_INT
static void s_CheckExecArg(const char* arg)
{
#  if defined(WORDS_BIGENDIAN)
    int lo = int(((Uint8)arg >> 32) & 0xffffffffU);
    int hi = int((Uint8)arg & 0xffffffffU);
#  else
    int hi = int(((Uint8)arg >> 32) & 0xffffffffU);
    int lo = int((Uint8)arg & 0xffffffffU);
#  endif
    if (lo == 0  &&  hi != 0) {
        ERR_POST(Warning <<
                 "It is possible that you used 0 instead of NULL "
                 "to terminate the argument list of a CExec::Spawn*() call.");
    }
}
#else
#  define s_CheckExecArg(x) 
#endif

// Macros to get exec arguments

#if defined(NCBI_OS_MSWIN)
#  define GET_ARGS_0 \
    AutoPtr<string> p_cmdname; \
    if ( strstr(cmdname, " ") ) { \
        string* tmp = new string(string("\"") + cmdname + "\""); \
        p_cmdname.reset(tmp); \
        args[0] = tmp->c_str(); \
    } else { \
        args[0] = cmdname; \
    }
#else
#  define GET_ARGS_0 \
    args[0] = cmdname
#endif

#define GET_EXEC_ARGS \
    int xcnt = 2; \
    va_list vargs; \
    va_start(vargs, argv); \
    while ( va_arg(vargs, const char*) ) xcnt++; \
    va_end(vargs); \
    const char **args = new const char*[xcnt+1]; \
    typedef ArrayDeleter<const char*> TArgsDeleter; \
    AutoPtr<const char*, TArgsDeleter> p_args(args); \
    if ( !args ) \
        NCBI_THROW(CCoreException, eNullPtr, kEmptyStr); \
    GET_ARGS_0; \
    args[1] = argv; \
    va_start(vargs, argv); \
    int xi = 1; \
    while ( xi < xcnt ) { \
        xi++; \
        args[xi] = va_arg(vargs, const char*); \
        s_CheckExecArg(args[xi]); \
    } \
    args[xi] = (const char*)0


// Return result from Spawn method
#define RETURN_RESULT(func) \
    if (status == -1) { \
        NCBI_THROW(CExecException, eSpawn, "CExec::" #func "() failed"); \
    } \
    CResult result; \
    if (mode == eWait) { \
        result.m_IsHandle = false; \
        result.m_Result.exitcode = (TExitCode)status; \
    } else { \
        result.m_IsHandle = true; \
        result.m_Result.handle = (TProcessHandle)status; \
    } \
    return result


TExitCode CExec::System(const char *cmdline)
{ 
    int status;
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = system(cmdline); 
#elif defined(NCBI_OS_UNIX)
    status = system(cmdline);
#endif
    if (status == -1) {
        NCBI_THROW(CExecException, eSystem,
                   "CExec::System: call to system failed");
    }
#if defined(NCBI_OS_UNIX)
    if (cmdline) {
        return WIFSIGNALED(status) ? WTERMSIG(status) + 0x80
            : WEXITSTATUS(status);
    } else {
        return status;
    }
#else
    return status;
#endif
}


CExec::CResult
CExec::SpawnL(EMode mode, const char *cmdname, const char *argv, ...)
{
    intptr_t status;
    GET_EXEC_ARGS;

#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnv(s_GetRealMode(mode), cmdname, args);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eV, mode, cmdname, args);
#endif
    RETURN_RESULT(SpawnL);
}


CExec::CResult
CExec::SpawnLE(EMode mode, const char *cmdname,  const char *argv, ...)
{
    intptr_t status;
    GET_EXEC_ARGS;
    char** envp = va_arg(vargs, char**);
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnve(s_GetRealMode(mode), cmdname, args, envp);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eVE, mode, cmdname, args, envp);
#endif
    RETURN_RESULT(SpawnLE);
}


CExec::CResult
CExec::SpawnLP(EMode mode, const char *cmdname, const char *argv, ...)
{
    intptr_t status;
    GET_EXEC_ARGS;
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnvp(s_GetRealMode(mode), cmdname, args);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eVP, mode, cmdname, args);
#endif
    RETURN_RESULT(SpawnLP);
}


CExec::CResult
CExec::SpawnLPE(EMode mode, const char *cmdname, const char *argv, ...)
{
    intptr_t status;
    GET_EXEC_ARGS;
    char** envp = va_arg(vargs, char**);
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnve(s_GetRealMode(mode), cmdname, args, envp);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eVPE, mode, cmdname, args, envp);
#endif
    RETURN_RESULT(SpawnLPE);
}


CExec::CResult
CExec::SpawnV(EMode mode, const char *cmdname, const char *const *argv)
{
    intptr_t status;
    char** argp = const_cast<char**>(argv);
    argp[0] = const_cast<char*>(cmdname);
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnv(s_GetRealMode(mode), cmdname, argv);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eV, mode, cmdname, argv);
#endif
    RETURN_RESULT(SpawnV);
}


CExec::CResult
CExec::SpawnVE(EMode mode, const char *cmdname, 
               const char *const *argv, const char * const *envp)
{
    intptr_t status;
    char** argp = const_cast<char**>(argv);
    argp[0] = const_cast<char*>(cmdname);
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnve(s_GetRealMode(mode), cmdname, argv, envp);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eVE, mode, cmdname, argv, envp);
#endif
    RETURN_RESULT(SpawnVE);
}


CExec::CResult
CExec::SpawnVP(EMode mode, const char *cmdname, const char *const *argv)
{
    intptr_t status;
    char** argp = const_cast<char**>(argv);
    argp[0] = const_cast<char*>(cmdname);
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnvp(s_GetRealMode(mode), cmdname, argv);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eVP, mode, cmdname, argv);
#endif
    RETURN_RESULT(SpawnVP);
}


CExec::CResult
CExec::SpawnVPE(EMode mode, const char *cmdname,
                const char *const *argv, const char * const *envp)
{
    intptr_t status;
    char** argp = const_cast<char**>(argv);
    argp[0] = const_cast<char*>(cmdname);
#if defined(NCBI_OS_MSWIN)
    _flushall();
    status = spawnvpe(s_GetRealMode(mode), cmdname, argv, envp);
#elif defined(NCBI_OS_UNIX)
    status = s_SpawnUnix(eVPE, mode, cmdname, argv, envp);
#endif
    RETURN_RESULT(SpawnVPE);
}


int CExec::Wait(TProcessHandle handle, unsigned long timeout)
{
    return CProcess(handle, CProcess::eHandle).Wait(timeout);
}


CExec::CResult CExec::RunSilent(EMode mode, const char *cmdname,
                                const char *argv, ... /*, NULL */)
{
    intptr_t status = -1;

#if defined(NCBI_OS_MSWIN)
    _flushall();

    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    const int           kMaxCmdLength = 4096;
	string              cmdline;

    // Set startup info
	memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb          = sizeof(STARTUPINFO);
	StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_HIDE;
    DWORD dwCreateFlags     = (mode == eDetach) ? 
                              DETACHED_PROCESS : CREATE_NEW_CONSOLE;

	// Compose command line
    cmdline.reserve(kMaxCmdLength);
	cmdline = cmdname;

    if (argv) {
	    cmdline += " "; 
	    cmdline += argv;
        va_list vargs;
        va_start(vargs, argv);
        const char* p = NULL;
        while ( (p = va_arg(vargs, const char*)) ) {
	        cmdline += " "; 
	        cmdline += p;
        }
        va_end(vargs);
    }

    // Just check mode parameter
    s_GetRealMode(mode);

    // Run program
	if (CreateProcess(NULL, (LPSTR)cmdline.c_str(), NULL, NULL, FALSE,
		              dwCreateFlags, NULL, NULL, &StartupInfo, &ProcessInfo))
    {
        if (mode == eOverlay) {
            // destroy ourselves
            _exit(0);
        }
        else if (mode == eWait) {
            // wait running process
            WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
            DWORD exitcode = -1;
            GetExitCodeProcess(ProcessInfo.hProcess, &exitcode);
            status = exitcode;
            CloseHandle(ProcessInfo.hProcess);
        }
        else if (mode == eDetach) {
            // detached asynchronous spawn,
            // just close process handle, return 0 for success
            CloseHandle(ProcessInfo.hProcess);
            status = 0;
        }
        else if (mode == eNoWait) {
            // asynchronous spawn -- return PID
            status = (intptr_t)ProcessInfo.hProcess;
        }
        CloseHandle(ProcessInfo.hThread);
    }

#elif defined(NCBI_OS_UNIX)
    GET_EXEC_ARGS;
    status = s_SpawnUnix(eV, mode, cmdname, args);

#endif
    RETURN_RESULT(RunSilent);
}


END_NCBI_SCOPE


/*
 * ===========================================================================
 * $Log$
 * Revision 1.37  2006/05/23 18:50:50  ucko
 * System: if the child died on a signal, return a non-zero value
 * (namely, the signal + 128, per typical shells).
 *
 * Revision 1.36  2006/05/19 16:37:47  dicuccio
 * Guard against NULL argv in RunSilent()
 *
 * Revision 1.35  2006/05/16 16:14:11  ivanov
 * MS Windows: GET_EXEC_ARGS: quote argv[0] in the list of parameters
 * for executed process if it contains spaces. The module name itself
 * should not be quoted.
 *
 * Revision 1.34  2006/05/16 15:30:41  dicuccio
 * FIxed macro cut-and-paste of parameters-to-strings
 *
 * Revision 1.33  2006/05/08 13:57:19  ivanov
 * Changed return type of all Spawn* function from int to CResult
 *
 * Revision 1.32  2006/04/27 22:53:38  vakatov
 * Rollback odd commits
 *
 * Revision 1.30  2006/03/31 17:51:40  dicuccio
 * CExec::RunSilent(): Fixed bugs in handling of varargs: make sure to trap the
 * returned pointer and use it in appending to a string
 *
 * Revision 1.29  2006/03/20 15:46:41  ivanov
 * + CExec::RunSilent()
 *
 * Revision 1.28  2006/03/02 16:48:16  vakatov
 * Minor formatting
 *
 * Revision 1.27  2006/02/28 18:58:47  gouriano
 * MSVC x64 tuneup
 *
 * Revision 1.26  2006/02/21 16:13:11  ivanov
 * Deleted debug code from CExec::SpawnL()
 *
 * Revision 1.25  2006/01/23 16:20:05  ivanov
 * Flushes all streams on MS Windows before spawn a process.
 * Removed double diagnostic messages.
 *
 * Revision 1.24  2005/11/07 17:03:43  dicuccio
 * CExec::SpawnL()/CExec::SpawnLP(): (Win32) bugfix - use GET_EXEC_ARGS to format
 * varargs.  Added better exception text for clarity.
 *
 * Revision 1.23  2005/06/10 14:21:06  ivanov
 * UNIX: use freopen() to redirect standard I/O streams to /dev/null
 * for detached process
 *
 * Revision 1.22  2004/08/19 17:08:58  ucko
 * More cleanups to s_CheckExecArg; in particular, perform the sizeof
 * test at compilation-time rather than runtime.
 *
 * Revision 1.21  2004/08/19 17:01:22  ucko
 * Fix typos in s_CheckExecArg.
 *
 * Revision 1.20  2004/08/19 12:20:11  ivanov
 * Lines wrapped at 79th column.
 *
 * Revision 1.19  2004/08/19 12:18:02  ivanov
 * Added s_CheckExecArg() to check argument, passed into function with
 * variable number of arguments, on possible using 0 instead NULL as last
 * argument on 64-bit platforms.
 *
 * Revision 1.18  2004/08/18 16:00:50  ivanov
 * Use NULL instead 0 where necessary to avoid problems with 64bit platforms
 *
 * Revision 1.17  2004/05/14 13:59:27  gorelenk
 * Added include of ncbi_pch.hpp
 *
 * Revision 1.16  2003/10/01 20:22:05  ivanov
 * Formal code rearrangement
 *
 * Revision 1.15  2003/09/25 17:02:20  ivanov
 * CExec::Wait():  replaced all code with CProcess::Wait() call
 *
 * Revision 1.14  2003/09/16 17:48:08  ucko
 * Remove redundant "const"s from arguments passed by value.
 *
 * Revision 1.13  2003/08/12 16:57:55  ivanov
 * s_SpawnUnix(): use execv() instead execvp() for the eV-mode
 *
 * Revision 1.12  2003/04/23 21:02:48  ivanov
 * Removed redundant NCBI_OS_MAC preprocessor directives
 *
 * Revision 1.11  2003/02/24 19:56:05  gouriano
 * use template-based exceptions instead of errno and parse exceptions
 *
 * Revision 1.10  2002/08/15 18:26:29  ivanov
 * Changed s_SpawnUnix() -- empty environment, setsid() for eDetach mode
 *
 * Revision 1.9  2002/07/17 15:12:34  ivanov
 * Changed method of obtaining parameters in the SpawnLE/LPE functions
 * under MS Windows
 *
 * Revision 1.8  2002/07/16 15:04:22  ivanov
 * Fixed warning about uninitialized variable in s_SpawnUnix()
 *
 * Revision 1.7  2002/07/15 16:38:50  ivanov
 * Fixed bug in macro GET_EXEC_ARGS -- write over array bound
 *
 * Revision 1.6  2002/07/11 14:18:26  gouriano
 * exceptions replaced by CNcbiException-type ones
 *
 * Revision 1.5  2002/06/30 03:22:14  vakatov
 * s_GetRealMode() -- formal code rearrangement to avoid a warning
 *
 * Revision 1.4  2002/06/11 19:28:31  ivanov
 * Use AutoPtr<char*> for exec arguments in GET_EXEC_ARGS
 *
 * Revision 1.3  2002/06/04 19:43:20  ivanov
 * Done s_ThrowException static
 *
 * Revision 1.2  2002/05/31 20:49:33  ivanov
 * Removed excrescent headers
 *
 * Revision 1.1  2002/05/30 16:29:13  ivanov
 * Initial revision
 *
 * ===========================================================================
 */
