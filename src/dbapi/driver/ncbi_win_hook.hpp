#ifndef NCBI_WIN_HOOK__HPP
#define NCBI_WIN_HOOK__HPP

/* $Id$
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
 * Author:  Sergey Sikorskiy
 *
 * File Description:  Windows DLL function hooking
 *
 */


/** @addtogroup WinHook
 *
 * @{
 */

#if defined(NCBI_OS_MSWIN)

    #include <corelib/ncbi_safe_static.hpp>
    #include <ImageHlp.h>
    #include <process.h>
    #include <Tlhelp32.h>
    #include <vector>

BEGIN_NCBI_SCOPE

namespace NWinHook
{

    ///////////////////////////////////////////////////////////////////////////////
    class NCBI_DBAPIDRIVER_EXPORT CWinHookException : public CCoreException
    {
    public:
        enum EErrCode {
            eDbghelp
        };

        /// Translate from the error code value to its string representation.
        virtual const char* GetErrCodeString(void) const;

        // Standard exception boilerplate code.
        NCBI_EXCEPTION_DEFAULT(CWinHookException, CCoreException);
    };


    ///////////////////////////////////////////////////////////////////////////////
    /// class CHookedFunctions
    ///
    class CHookedFunction;

    class CHookedFunctions {
    public:
        CHookedFunctions(void);
        ~CHookedFunctions(void);

    public:
        /// Return the address of an CHookedFunction object
        CHookedFunction* GetHookedFunction(PCSTR pszCalleeModName,
                                           PCSTR pszFuncName
                                           );
        /// Return the address of an CHookedFunction object
        CHookedFunction* GetHookedFunction(HMODULE hmod,
                                           PCSTR   pszFuncName
                                           );
        /// Add a new object to the container
        BOOL AddHook(CHookedFunction* pHook);
        /// Remove exising object pointer from the container
        BOOL RemoveHook(CHookedFunction* pHook);

        void UnHookAllFuncs(void);

        bool HaveHookedFunctions(void) const 
        {
            return(m_FunctionList.size() > 0);
        }

    private:
        /// Return the name of the function from EAT by its ordinal value
        BOOL GetFunctionNameFromExportSection(HMODULE hmodOriginal,
                                              DWORD   dwFuncOrdinalNum,
                                              PSTR    pszFuncName
                                              );
        /// Return the name of the function by its ordinal value
        void GetFunctionNameByOrdinal(PCSTR   pszCalleeModName,
                                      DWORD   dwFuncOrdinalNum,
                                      PSTR    pszFuncName
                                      );

    private:
        struct SNocaseCmp {
            bool operator()(const string& x, const string& y) const {
                return( stricmp(x.c_str(), y.c_str()) < 0 );
            }
        };
        typedef map<string, CHookedFunction*, SNocaseCmp> TFunctionList;

        TFunctionList m_FunctionList;
        friend class CApiHookMgr;
    };


    ///////////////////////////////////////////////////////////////////////////////
    /// class CApiHookMgr
    ///
    class CApiHookMgr {
    private:
        CApiHookMgr(void);
        ~CApiHookMgr(void);
        void operator =(const CApiHookMgr&);

    public:
        static CApiHookMgr& GetInstance(void);

        /// Hook up an API
        BOOL HookImport(PCSTR pszCalleeModName,
                        PCSTR pszFuncName,
                        PROC  pfnHook
                        );

        /// Restore hooked up API function
        BOOL UnHookImport(PCSTR pszCalleeModName,
                          PCSTR pszFuncName
                          );

        /// Indicates whether there is hooked function
        bool HaveHookedFunctions(void) const;

    private:
        /// Hook all needed system functions in order to trap loading libraries
        BOOL HookSystemFuncs(void);

        /// Unhook all functions and restore original ones
        void UnHookAllFuncs(void);

        /// Return the address of an CHookedFunction object
        /// Protected version.
        CHookedFunction* GetHookedFunction(HMODULE hmod,
                                           PCSTR   pszFuncName
                                           );

        CFastMutex m_Mutex;
        /// Container keeps track of all hacked functions
        CHookedFunctions m_pHookedFunctions;
        /// Determines whether all system functions has been successfuly hacked
        BOOL m_bSystemFuncsHooked;

        /// Used when a DLL is newly loaded after hooking a function
        void WINAPI HackModuleOnLoad(HMODULE hmod,
                                     DWORD   dwFlags
                                     );

        /// Used to trap events when DLLs are loaded
        static HMODULE WINAPI MyLoadLibraryA(PCSTR  pszModuleName);
        /// Used to trap events when DLLs are loaded
        static HMODULE WINAPI MyLoadLibraryW(PCWSTR pszModuleName);
        /// Used to trap events when DLLs are loaded
        static HMODULE WINAPI MyLoadLibraryExA(PCSTR  pszModuleName,
                                               HANDLE hFile,
                                               DWORD  dwFlags
                                               );
        /// Used to trap events when DLLs are loaded
        static HMODULE WINAPI MyLoadLibraryExW(PCWSTR pszModuleName,
                                               HANDLE hFile,
                                               DWORD  dwFlags
                                               );
        /// Returns address of replacement function if hooked function is 
        /// requested
        static FARPROC WINAPI MyGetProcAddress(HMODULE hmod,
                                               PCSTR   pszProcName
                                               );

        /// Returns original address of the API function
        static FARPROC WINAPI GetProcAddressWindows(HMODULE hmod,
                                                    PCSTR   pszProcName
                                                    );

        /// Add a newly intercepted function to the container
        BOOL AddHook(PCSTR  pszCalleeModName,
                     PCSTR  pszFuncName,
                     PROC   pfnOrig,
                     PROC   pfnHook
                     );

        /// Remove intercepted function from the container
        BOOL RemoveHook(PCSTR pszCalleeModName,
                        PCSTR pszFuncName
                        );

        friend class CSafeStaticPtr<CApiHookMgr>;
        friend class CHookedFunction;
    };


    class NCBI_DBAPIDRIVER_EXPORT COnExitProcess
    {
    public:
        typedef void (*TFunct) (void);

        static COnExitProcess& Instance(void);
        
        void Add(TFunct funct);
        void Remove(TFunct funct);
        void ClearAll(void);
        
    private:
        COnExitProcess(void);
        ~COnExitProcess(void);
        // Hook function prototype
        static void WINAPI ExitProcess(UINT uExitCode);
        
        mutable CFastMutex  m_Mutex;
        vector<TFunct>      m_Registry;
        
        friend class CSafeStaticPtr<COnExitProcess>;
    };
}

END_NCBI_SCOPE

#endif // NCBI_OS_MSWIN

/* @} */


/*
 * ===========================================================================
 * $Log$
 * Revision 1.2  2006/04/10 22:26:25  ssikorsk
 * Moved some class declarations into cpp.
 *
 * Revision 1.1  2006/04/05 14:05:48  ssikorsk
 * Initial version
 *
 * ===========================================================================
 */

#endif  // NCBI_WIN_HOOK__HPP
