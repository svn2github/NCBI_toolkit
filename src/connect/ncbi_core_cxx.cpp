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
 * Author:  Anton Lavrentiev
 *
 * File description:
 *   C++->C conversion functions for basic corelib stuff:
 *     Registry
 *     Logging
 *     Locking
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.8  2002/01/25 23:33:04  vakatov
 * s_LOCK_Handler() -- to match handler proto, return boolean value (not VOID!)
 *
 * Revision 6.7  2002/01/15 21:28:49  lavr
 * +MT_LOCK_cxx2c()
 *
 * Revision 6.6  2001/03/02 20:08:17  lavr
 * Typos fixed
 *
 * Revision 6.5  2001/01/25 17:03:46  lavr
 * s_LOG_Handler: user_data commented out as unused
 *
 * Revision 6.4  2001/01/23 23:08:06  lavr
 * LOG_cxx2c introduced
 *
 * Revision 6.3  2001/01/12 05:48:50  vakatov
 * Use reinterpret_cast<> rather than static_cast<> to cast functions.
 * Added more paranoia to catch ALL possible exceptions in the s_*** functions.
 *
 * Revision 6.2  2001/01/11 23:51:47  lavr
 * static_cast instead of linkage specification 'extern "C" {}'.
 * Reason: MSVC++ doesn't allow C-linkage of the funs compiled in C++ file.
 *
 * Revision 6.1  2001/01/11 23:08:16  lavr
 * Initial revision
 *
 * ===========================================================================
 */

#include <connect/ncbi_core_cxx.hpp>
#include <corelib/ncbistr.hpp>
#include <string.h>


BEGIN_NCBI_SCOPE


static void s_REG_Get(void* user_data,
                      const char* section, const char* name,
                      char* value, size_t value_size)
{
    try {
        string result = static_cast<CNcbiRegistry*> (user_data)->
            Get(section, name);

        if ( !result.empty() ) {
            strncpy(value, result.c_str(), value_size - 1);
            value[value_size - 1] = '\0';
        }
    }
    STD_CATCH_ALL("s_REG_Get() failed");
}


static void s_REG_Set(void* user_data,
                      const char* section, const char* name,
                      const char* value, EREG_Storage storage)
{
    try {
        static_cast<CNcbiRegistry*> (user_data)->
            Set(section, name, value,
                (storage == eREG_Persistent ? CNcbiRegistry::ePersistent : 0) |
                CNcbiRegistry::eOverride | CNcbiRegistry::eTruncate);
    }
    STD_CATCH_ALL("s_REG_Set() failed");
}


static void s_REG_Cleanup(void* user_data)
{
    try {
        delete static_cast<CNcbiRegistry*> (user_data);
    }
    STD_CATCH_ALL("s_REG_Cleanup() failed");
}


extern REG REG_cxx2c(CNcbiRegistry* reg, bool pass_ownership)
{
    return REG_Create
        (static_cast<void*> (reg),
         reinterpret_cast<FREG_Get> (s_REG_Get),
         reinterpret_cast<FREG_Set> (s_REG_Set),
         pass_ownership ? reinterpret_cast<FREG_Cleanup> (s_REG_Cleanup) : 0,
         0);
}


static void s_LOG_Handler(void* /*user_data*/, SLOG_Handler* call_data)
{
    try {
        EDiagSev level;
        switch ( call_data->level ) {
        case eLOG_Trace:
            level = eDiag_Trace;
            break;
        case eLOG_Note:
            level = eDiag_Info;
            break;
        case eLOG_Warning:
            level = eDiag_Warning;
            break;
        case eLOG_Error:
            level = eDiag_Error;
            break;
        case eLOG_Critical:
            level = eDiag_Critical;
            break;
        case eLOG_Fatal:
            /*fallthru*/
        default:
            level = eDiag_Fatal;
            break;
        }

        CNcbiDiag diag(level, eDPF_Default);
        if (call_data->file) {
            diag.SetFile(call_data->file);
        }
        if (call_data->line) {
            diag.SetLine(call_data->line);
        }
        diag << call_data->message;
        if (call_data->raw_data  &&  call_data->raw_size) {
            diag <<
                "\n#################### [BEGIN] Raw Data (" <<
                call_data->raw_size <<
                " byte" << (call_data->raw_size != 1 ? "s" : "") << ")\n" <<
                NStr::PrintableString
                (string(static_cast<const char*>(call_data->raw_data),
                        call_data->raw_size)) <<
                "\n#################### [END] Raw Data";
        }
    }
    STD_CATCH_ALL("s_LOG_Handler() failed");
}


extern LOG LOG_cxx2c(void)
{
    return LOG_Create(0,
                      reinterpret_cast<FLOG_Handler> (s_LOG_Handler),
                      0,
                      0);
}


static int/*bool*/ s_LOCK_Handler(void* user_data, EMT_Lock how)
{
    try {
        CRWLock* lock = static_cast<CRWLock*> (user_data);
        switch ( how ) {
        case eMT_Lock:
            lock->WriteLock();
            break;
        case eMT_LockRead:
            lock->ReadLock();
            break;
        case eMT_Unlock:
            lock->Unlock();
            break;
        default:
            THROW1_TRACE(runtime_error,
                         "s_LOCK_Handler() used with op " + (unsigned int)how);
        }
        return 1/*true*/;

    }
    STD_CATCH_ALL("s_LOCK_Handler() failed");
    return 0/*false*/;
}


static void s_LOCK_Cleanup(void* user_data)
{
    try {
        delete static_cast<CRWLock*> (user_data);
    }
    STD_CATCH_ALL("s_LOCK_Cleanup() failed");
}


extern MT_LOCK MT_LOCK_cxx2c(CRWLock* lock, bool pass_ownership)
{
    return MT_LOCK_Create(static_cast<void*> (lock ? lock : new CRWLock),
                          reinterpret_cast<FMT_LOCK_Handler> (s_LOCK_Handler),
                          !lock  ||  pass_ownership ?
                          reinterpret_cast<FMT_LOCK_Cleanup> (s_LOCK_Cleanup) :
                          0);
}


END_NCBI_SCOPE
