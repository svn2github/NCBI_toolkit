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
 * Author:  Anton Lavrentiev
 *
 * File Description:
 *   C++->C conversion tools for basic CORE connect stuff:
 *     Registry
 *     Logging
 *     Locking
 *
 */

#include <ncbi_pch.hpp>
#include "ncbi_ansi_ext.h"
#include "ncbi_priv.h"
#include <connect/ncbi_monkey.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/request_ctx.hpp>
#include <connect/error_codes.hpp>
#include <connect/ncbi_core_cxx.hpp>
#include <stdlib.h>

#define NCBI_USE_ERRCODE_X   Connect_Core


BEGIN_NCBI_SCOPE


/***********************************************************************
 *                              Registry                               *
 ***********************************************************************/

extern "C" {
static void s_REG_Get(void* user_data,
                      const char* section, const char* name,
                      char* value, size_t value_size) THROWS_NONE
{
    try {
        string result(static_cast<IRegistry*> (user_data)->Get(section, name));

        if (!result.empty()) {
            /*FIXME:  This is *bad* because of possible truncation*/
            strncpy0(value, result.c_str(), value_size - 1);
        }
    }
    NCBI_CATCH_ALL_X(1, "s_REG_Get() failed");
}
}


extern "C" {
static int s_REG_Set(void* user_data,
                     const char* section, const char* name,
                     const char* value, EREG_Storage storage) THROWS_NONE
{
    int result = 0;
    try {
        IRWRegistry* reg = static_cast<IRWRegistry*> (user_data);
        result = value
            ? reg->Set  (section, name, value, 
                         (storage == eREG_Persistent
                          ? CNcbiRegistry::fPersistent
                          | CNcbiRegistry::fTruncate
                          : CNcbiRegistry::fTruncate))
            : reg->Unset(section, name,
                         (storage == eREG_Persistent
                          ? CNcbiRegistry::fPersistent
                          : CNcbiRegistry::fTransient));
    }
    NCBI_CATCH_ALL_X(2, "s_REG_Set() failed");
    return result;
}
}


extern "C" {
static void s_REG_Cleanup(void* user_data) THROWS_NONE
{
    try {
        static_cast<IRegistry*> (user_data)->RemoveReference();
    }
    NCBI_CATCH_ALL_X(3, "s_REG_Cleanup() failed");
}
}


extern REG REG_cxx2c(IRWRegistry* reg, bool pass_ownership)
{
    if (pass_ownership  &&  reg) {
        reg->AddReference();
    }
    return reg
        ? REG_Create(static_cast<void*> (reg), s_REG_Get, s_REG_Set,
                     pass_ownership ? s_REG_Cleanup : 0, 0)
        : 0;
}


/***********************************************************************
 *                                Logger                               *
 ***********************************************************************/

extern "C" {
static void s_LOG_Handler(void*       /*user_data*/,
                          SLOG_Handler* call_data) THROWS_NONE
{
    try {
        EDiagSev level;
        switch (call_data->level) {
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
            /*FALLTHRU*/
        default:
            level = eDiag_Fatal;
            break;
        }
        if (!IsVisibleDiagPostLevel(level))
            return;

        CDiagCompileInfo info(call_data->file,
                              call_data->line,
                              call_data->func,
                              call_data->module);
        CNcbiDiag diag(info, level);
        diag.SetErrorCode(call_data->err_code, call_data->err_subcode);
        diag << call_data->message;
        if (call_data->raw_size) {
            diag <<
                "\n#################### [BEGIN] Raw Data (" <<
                call_data->raw_size <<
                " byte" << (call_data->raw_size != 1 ? "s" : "") << ")\n" <<
                NStr::PrintableString
                (CTempString(static_cast<const char*>(call_data->raw_data),
                             call_data->raw_size),
                 NStr::fNewLine_Passthru | NStr::fNonAscii_Quote) <<
                "\n#################### [END] Raw Data";
        }
    }
    NCBI_CATCH_ALL_X(4, "s_LOG_Handler() failed");
}
}


extern LOG LOG_cxx2c(void)
{
    return LOG_Create(0, s_LOG_Handler, 0, 0);
}


/***********************************************************************
 *                               MT-Lock                               *
 ***********************************************************************/

extern "C" {
static int/*bool*/ s_LOCK_Handler(void* user_data, EMT_Lock how)
    THROWS_NONE
{
    try {
        CRWLock* lock = static_cast<CRWLock*> (user_data);
        switch (how) {
        case eMT_Lock:
            lock->WriteLock();
            break;
        case eMT_LockRead:
            lock->ReadLock();
            break;
        case eMT_Unlock:
            lock->Unlock();
            break;
        case eMT_TryLock:
            if (!lock->TryWriteLock()) {
                return 0/*false*/;
            }
            break;
        case eMT_TryLockRead:
            if (!lock->TryReadLock()) {
                return 0/*false*/;
            }
            break;
        default:
            NCBI_THROW(CCoreException, eCore, "Lock used with unknown op #" +
                       NStr::UIntToString((unsigned int) how));
        }
        return 1/*true*/;
    }
    NCBI_CATCH_ALL_X(5, "s_LOCK_Handler() failed");
    return 0/*false*/;
}
}


extern "C" {
static void s_LOCK_Cleanup(void* user_data) THROWS_NONE
{
    try {
        delete static_cast<CRWLock*> (user_data);
    }
    NCBI_CATCH_ALL_X(6, "s_LOCK_Cleanup() failed");
}
}


extern MT_LOCK MT_LOCK_cxx2c(CRWLock* lock, bool pass_ownership)
{
    return MT_LOCK_Create(static_cast<void*> (lock ? lock : new CRWLock),
                          s_LOCK_Handler,
                          !lock  ||  pass_ownership ? s_LOCK_Cleanup : 0);
}


/***********************************************************************
 *                                 Fini                                *
 ***********************************************************************/

extern "C" {
static void s_Fini(void) THROWS_NONE
{
    CORE_SetREG(0);
    CORE_SetLOG(0);
    CORE_SetLOCK(0);
}
}


/***********************************************************************
 *                               App Name                              *
 ***********************************************************************/
extern "C" {
static const char* s_GetAppName(void)
{
    CMutexGuard guard(CNcbiApplication::GetInstanceMutex());
    CNcbiApplication* app = CNcbiApplication::Instance();
    return app ? app->GetProgramDisplayName().c_str() : 0;
}
}


/***********************************************************************
 *                           NCBI Request ID                           *
 ***********************************************************************/
extern "C" {
static char* s_GetRequestID(ENcbiRequestID reqid)
{
    string id;
    switch (reqid) {
    case eNcbiRequestID_SID:
        if (!CDiagContext::GetRequestContext().IsSetSessionID()) {
            CDiagContext::GetRequestContext().SetSessionID();
        }
        CDiagContext::GetRequestContext().GetSessionID().swap(id);
        break;
    case eNcbiRequestID_HitID:
        id = CDiagContext::GetRequestContext().GetNextSubHitID();
        break;
    default:
        return 0;
    }
    return id.empty() ? 0 : strdup(id.c_str());
}
}


/***********************************************************************
 *                          NCBI Request DTab                          *
 ***********************************************************************/
extern "C" {
static const char* s_GetRequestDTab(void)
{
    if (!CDiagContext::GetRequestContext().IsSetDtab()) {
        CDiagContext::GetRequestContext().SetDtab("");
    }
    return CDiagContext::GetRequestContext().GetDtab().c_str();
}
}

/***********************************************************************
 *                         CRAZY MONKEY CALLS                          *
 ***********************************************************************/
#ifdef NCBI_MONKEY
#   ifndef NCBI_OS_MSWIN
#       define __stdcall /*empty*/
#   endif /* NCBI_OS_MSWIN */
extern "C" {
    static int __stdcall s_MonkeySend(SOCKET sock, 
                            const char*  data, 
                            int    size, 
                            int    flags)
    {
        return CMonkey::Instance()->Send(sock, data, size, flags);
    }
    
    static int __stdcall s_MonkeyRecv(SOCKET sock,
                            char*  buf,
                            int    size,
                            int    flags)
    {
        return CMonkey::Instance()->Recv(sock, buf, size, flags);
    }

    
    static int __stdcall s_MonkeyConnect(SOCKET                 sock,
                               const struct sockaddr* name,
                               int                    namelen)
    {
        return CMonkey::Instance()->Connect(sock, name, namelen);
    }

    
    static int /*bool*/ s_MonkeyPoll(size_t*                  n,
                                     void* /* SSOCK_Poll** */ polls,
                                     EIO_Status*              return_status)
    {
        return CMonkey::Instance()->
            Poll(n, (SSOCK_Poll**)polls, return_status) ? 1 : 0;
    }


    static void s_MonkeyClose(SOCKET  sock)
    {
        CMonkey::Instance()->Close(sock);
    }
}
#endif /* NCBI_MONKEY */

/***********************************************************************
 *                                 Init                                *
 ***********************************************************************/

DEFINE_STATIC_FAST_MUTEX(s_ConnectInitMutex);

static enum EConnectInit {
    eConnectInit_Weak = -1,
    eConnectInit_Intact = 0,
    eConnectInit_Explicit = 1
} s_ConnectInit = eConnectInit_Intact;


/* NB: gets called under a lock */
static void s_Init(IRWRegistry*      reg  = 0,
                   CRWLock*          lock = 0,
                   TConnectInitFlags flag = 0,
                   EConnectInit      how  = eConnectInit_Weak)
{
    _ASSERT(how != eConnectInit_Intact);
    if (g_NCBI_ConnectRandomSeed == 0) {
        g_NCBI_ConnectRandomSeed  = (int) time(0) ^ NCBI_CONNECT_SRAND_ADDEND;
        srand(g_NCBI_ConnectRandomSeed);
    }
    CORE_SetLOCK(MT_LOCK_cxx2c(lock,
                               flag & eConnectInit_OwnLock ? true : false));
    CORE_SetLOG(LOG_cxx2c());
    CORE_SetREG(REG_cxx2c(reg, flag & eConnectInit_OwnRegistry));
    if (s_ConnectInit == eConnectInit_Intact) {
        atexit(s_Fini);
    }

    /* setup request ID retrieval callback */
    g_CORE_GetRequestID = s_GetRequestID;

    /* setup app name retrieval callback */
    g_CORE_GetAppName = s_GetAppName;

    /* setup DTab-Local retrieval */
    g_CORE_GetRequestDtab = s_GetRequestDTab;

#ifdef NCBI_MONKEY
    g_MONKEY_Write   = s_MonkeySend;
    g_MONKEY_Read    = s_MonkeyRecv;
    g_MONKEY_Connect = s_MonkeyConnect;
    g_MONKEY_Poll    = s_MonkeyPoll;
#endif /* NCBI_MONKEY */
    /* done! */
    s_ConnectInit = how;
}


static void s_InitInternal(void)
{
    CFastMutexGuard guard(s_ConnectInitMutex);
    if (!g_CORE_Registry  &&  !g_CORE_Log
        &&  g_CORE_MT_Lock == &g_CORE_MT_Lock_default) {
        try {
            if (s_ConnectInit == eConnectInit_Intact) {
                CMutexGuard guard(CNcbiApplication::GetInstanceMutex());
                CNcbiApplication* app = CNcbiApplication::Instance();
                s_Init(app ? &app->GetConfig() : 0);
            }
        }
        NCBI_CATCH_ALL_X(7, "CONNECT_InitInternal() failed");
    } else {
        s_ConnectInit = eConnectInit_Explicit;
    }
}


/* PUBLIC */
extern void CONNECT_Init(IRWRegistry*      reg,
                         CRWLock*          lock,
                         TConnectInitFlags flag)
{
    CFastMutexGuard guard(s_ConnectInitMutex);
    try {
        s_Init(reg, lock, flag, eConnectInit_Explicit);
    }
    NCBI_CATCH_ALL_X(8, "CONNECT_Init() failed");
}


CConnIniter::CConnIniter(void)
{
    if (s_ConnectInit == eConnectInit_Intact) {
        s_InitInternal();
    }
}


END_NCBI_SCOPE
