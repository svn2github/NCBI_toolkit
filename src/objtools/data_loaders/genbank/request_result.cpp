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
*  Author: Eugene Vasilchenko
*
*  File Description: GenBank Data loader
*
*/

#include <ncbi_pch.hpp>
#include <objtools/data_loaders/genbank/request_result.hpp>
#include <objmgr/objmgr_exception.hpp>
#include <objmgr/impl/tse_info.hpp>
#include <corelib/ncbithr.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


static inline TThreadSystemID GetThreadId(void)
{
    TThreadSystemID thread_id = 0;
    CThread::GetSystemID(&thread_id);
    return thread_id;
}


/////////////////////////////////////////////////////////////////////////////
// CResolveInfo
/////////////////////////////////////////////////////////////////////////////

CLoadInfo::CLoadInfo(void)
{
}


CLoadInfo::~CLoadInfo(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// CLoadInfoSeq_ids
/////////////////////////////////////////////////////////////////////////////

CLoadInfoSeq_ids::CLoadInfoSeq_ids(void)
{
}


CLoadInfoSeq_ids::~CLoadInfoSeq_ids(void)
{
}


/////////////////////////////////////////////////////////////////////////////
// CLoadInfoBlob_ids
/////////////////////////////////////////////////////////////////////////////

CLoadInfoBlob_ids::CLoadInfoBlob_ids(const TSeq_id& id)
    : m_Seq_id(id)
{
}


CLoadInfoBlob_ids::~CLoadInfoBlob_ids(void)
{
}


CLoadInfoBlob_ids::TBlob_Info&
CLoadInfoBlob_ids::AddBlob_id(const TBlob_id& id, const TBlob_Info& info)
{
    _ASSERT(!IsLoaded());
    return m_Blob_ids.insert(TBlob_ids::value_type(id, info))
        .first->second;
}


/////////////////////////////////////////////////////////////////////////////
// CLoadInfoBlob
/////////////////////////////////////////////////////////////////////////////
#if 0
CLoadInfoBlob::CLoadInfoBlob(const TBlob_id& id)
    : m_Blob_id(id),
      m_Blob_State(eState_normal)
{
}


CLoadInfoBlob::~CLoadInfoBlob(void)
{
}


CRef<CTSE_Info> CLoadInfoBlob::GetTSE_Info(void) const
{
    return m_TSE_Info;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadInfoLock
/////////////////////////////////////////////////////////////////////////////

CLoadInfoLock::CLoadInfoLock(CReaderRequestResult& owner,
                             const CRef<CLoadInfo>& info)
    : m_Owner(owner),
      m_Info(info),
      m_Guard(m_Info->m_LoadLock, owner)
{
}


CLoadInfoLock::~CLoadInfoLock(void)
{
    ReleaseLock();
    m_Owner.ReleaseLoadLock(m_Info);
}


void CLoadInfoLock::ReleaseLock(void)
{
    m_Guard.Release();
}


void CLoadInfoLock::SetLoaded(CObject* obj)
{
    _ASSERT(!m_Info->m_LoadLock);
    if ( !obj ) {
        obj = new CObject;
    }
    m_Info->m_LoadLock.Reset(obj);
    ReleaseLock();
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLock_Base
/////////////////////////////////////////////////////////////////////////////

void CLoadLock_Base::Lock(TInfo& info, TMutexSource& src)
{
    m_Info.Reset(&info);
    if ( !m_Info->IsLoaded() ) {
        m_Lock = src.GetLoadLock(m_Info);
    }
}


void CLoadLock_Base::SetLoaded(CObject* obj)
{
    m_Lock->SetLoaded(obj);
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLockSeq_ids
/////////////////////////////////////////////////////////////////////////////


CLoadLockSeq_ids::CLoadLockSeq_ids(TMutexSource& src, const string& seq_id)
{
    CRef<TInfo> info = src.GetInfoSeq_ids(seq_id);
    Lock(*info, src);
}


CLoadLockSeq_ids::CLoadLockSeq_ids(TMutexSource& src,
                                   const CSeq_id_Handle& seq_id)
{
    CRef<TInfo> info = src.GetInfoSeq_ids(seq_id);
    Lock(*info, src);
}


CLoadLockSeq_ids::CLoadLockSeq_ids(TMutexSource& src,
                                   const CSeq_id& seq_id)
{
    CRef<TInfo> info = src.GetInfoSeq_ids(CSeq_id_Handle::GetHandle(seq_id));
    Lock(*info, src);
}


void CLoadLockSeq_ids::AddSeq_id(const CSeq_id_Handle& seq_id)
{
    Get().m_Seq_ids.push_back(seq_id);
}


void CLoadLockSeq_ids::AddSeq_id(const CSeq_id& seq_id)
{
    AddSeq_id(CSeq_id_Handle::GetHandle(seq_id));
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLockBlob_ids
/////////////////////////////////////////////////////////////////////////////


CLoadLockBlob_ids::CLoadLockBlob_ids(TMutexSource& src,
                                     const CSeq_id_Handle& seq_id)
{
    CRef<TInfo> info = src.GetInfoBlob_ids(seq_id);
    Lock(*info, src);
}


CLoadLockBlob_ids::CLoadLockBlob_ids(TMutexSource& src,
                                     const CSeq_id& seq_id)
{
    CRef<TInfo> info = src.GetInfoBlob_ids(CSeq_id_Handle::GetHandle(seq_id));
    Lock(*info, src);
}


CBlob_Info& CLoadLockBlob_ids::AddBlob_id(const CBlob_id& blob_id,
                                          const CBlob_Info& blob_info)
{
    return Get().AddBlob_id(blob_id, blob_info);
}


/////////////////////////////////////////////////////////////////////////////
// CLoadLockBlob
/////////////////////////////////////////////////////////////////////////////
#if 0
CLoadLockBlob::CLoadLockBlob(TMutexSource& src, const CBlob_id& blob_id)
{
    for ( ;; ) {
        CRef<TInfo> info = src.GetInfoBlob(blob_id);
        Lock(*info, src);
        if ( src.AddTSE_Lock(*this) ) {
            // locked
            break;
        }
        else {
            // failed to lock
            if ( !IsLoaded() ) {
                // not loaded yet -> OK
                break;
            }
            else {
                if ( info->IsNotLoadable() ) {
                    // private or withdrawn
                    break;
                }
                // already loaded and dropped while trying to lock
                // we need to repeat locking procedure
            }
        }
    }
}
#endif

CLoadLockBlob::CLoadLockBlob(CReaderRequestResult& src,
                             const CBlob_id& blob_id)
    : CTSE_LoadLock(src.GetBlobLoadLock(blob_id))
{
    if ( IsLoaded() ) {
        src.AddTSE_Lock(*this);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CReaderRequestResult
/////////////////////////////////////////////////////////////////////////////


CReaderRequestResult::CReaderRequestResult(void)
{
}


CReaderRequestResult::~CReaderRequestResult(void)
{
    UpdateLoadedSet();
    _ASSERT(m_LockMap.empty());
}


CTSE_LoadLock CReaderRequestResult::GetBlobLoadLock(const CBlob_id& blob_id)
{
    CTSE_LoadLock& lock = m_BlobLoadLocks[blob_id];
    if ( !lock ) {
        lock = GetTSE_LoadLock(blob_id);
    }
    return lock;
}


#if 0
void CReaderRequestResult::SetTSE_Info(CLoadLockBlob& blob,
                                       const CRef<CTSE_Info>& tse)
{
    blob->m_TSE_Info = tse;
    AddTSE_Lock(AddTSE(tse, blob->GetBlob_id()));
    SetLoaded(blob);
}


CRef<CTSE_Info> CReaderRequestResult::GetTSE_Info(const CLoadLockBlob& blob)
{
    return blob->GetTSE_Info();
}


void CReaderRequestResult::SetTSE_Info(const CBlob_id& blob_id,
                                       const CRef<CTSE_Info>& tse)
{
    CLoadLockBlob blob(*this, blob_id);
    SetTSE_Info(blob, tse);
}


CRef<CTSE_Info> CReaderRequestResult::GetTSE_Info(const CBlob_id& blob_id)
{
    return GetTSE_Info(CLoadLockBlob(*this, blob_id));
}
#endif

CRef<CLoadInfoLock>
CReaderRequestResult::GetLoadLock(const CRef<CLoadInfo>& info)
{
    CLoadInfoLock*& lock_ptr = m_LockMap[info];
    if ( !lock_ptr ) {
        lock_ptr = new CLoadInfoLock(*this, info);
    }
    else {
        _ASSERT(lock_ptr->Referenced());
    }
    return Ref(lock_ptr);
}


void CReaderRequestResult::ReleaseLoadLock(const CRef<CLoadInfo>& info)
{
    _ASSERT(m_LockMap[info] && !m_LockMap[info]->Referenced());
    m_LockMap.erase(info);
}


void CReaderRequestResult::AddTSE_Lock(const TTSE_Lock& tse_lock)
{
    _ASSERT(tse_lock);
    m_TSE_LockSet.insert(tse_lock);
}

#if 0
bool CReaderRequestResult::AddTSE_Lock(const TKeyBlob& blob_id)
{
    return AddTSE_Lock(CLoadLockBlob(*this, blob_id));
}


bool CReaderRequestResult::AddTSE_Lock(const CLoadLockBlob& blob)
{
    CRef<CTSE_Info> tse = blob->GetTSE_Info();
    if ( !tse ) {
        return false;
    }
    TTSE_Lock tse_lock = LockTSE(tse);
    if ( !tse_lock ) {
        return false;
    }
    AddTSE_Lock(tse_lock);
    return true;
}


TTSE_Lock CReaderRequestResult::LockTSE(CRef<CTSE_Info> /*tse*/)
{
    return TTSE_Lock();
}


TTSE_Lock CReaderRequestResult::AddTSE(CRef<CTSE_Info> /*tse*/,
                                       const TKeyBlob& blob_id)
{
    return TTSE_Lock();
}
#endif

void CReaderRequestResult::SetLoaded(CLoadLock_Base& lock, CObject* object)
{
    CRef<CLoadInfoLock> ref = lock.GetLock();
    _ASSERT(ref);
    m_LoadedSet[ref] = object;
}


void CReaderRequestResult::SaveLocksTo(TTSE_LockSet& locks)
{
    UpdateLoadedSet();
    ITERATE ( TTSE_LockSet, it, GetTSE_LockSet() ) {
        locks.insert(*it);
    }
}


void CReaderRequestResult::ReleaseLocks(void)
{
    NON_CONST_ITERATE( TLockMap, it, m_LockMap ) {
        it->second->ReleaseLock();
    }
    m_BlobLoadLocks.clear();
    m_LoadedSet.clear();
    m_TSE_LockSet.clear();
}


void CReaderRequestResult::ResetLoadedSet(void)
{
    m_LoadedSet.clear();
}


void CReaderRequestResult::UpdateLoadedSet(const TLoadedSet& ls)
{
    ITERATE ( TLoadedSet, it, ls ) {
        CRef<CLoadInfoLock> lock = it->first;
        CRef<CObject> obj = it->second;
        lock->SetLoaded(obj);
    }
}


void CReaderRequestResult::UpdateLoadedSet(void)
{
    UpdateLoadedSet(m_LoadedSet);
    ResetLoadedSet();
}


/////////////////////////////////////////////////////////////////////////////
// CStandaloneRequestResult
/////////////////////////////////////////////////////////////////////////////


CStandaloneRequestResult::CStandaloneRequestResult(void)
{
}


CStandaloneRequestResult::~CStandaloneRequestResult(void)
{
    ReleaseLocks();
}


CRef<CLoadInfoSeq_ids>
CStandaloneRequestResult::GetInfoSeq_ids(const string& key)
{
    CRef<CLoadInfoSeq_ids>& ret = m_InfoSeq_ids[key];
    if ( !ret ) {
        ret = new CLoadInfoSeq_ids();
    }
    return ret;
}


CRef<CLoadInfoSeq_ids>
CStandaloneRequestResult::GetInfoSeq_ids(const CSeq_id_Handle& key)
{
    CRef<CLoadInfoSeq_ids>& ret = m_InfoSeq_ids2[key];
    if ( !ret ) {
        ret = new CLoadInfoSeq_ids();
    }
    return ret;
}


CRef<CLoadInfoBlob_ids>
CStandaloneRequestResult::GetInfoBlob_ids(const CSeq_id_Handle& key)
{
    CRef<CLoadInfoBlob_ids>& ret = m_InfoBlob_ids[key];
    if ( !ret ) {
        ret = new CLoadInfoBlob_ids(key);
    }
    return ret;
}


CTSE_LoadLock
CStandaloneRequestResult::GetTSE_LoadLock(const CBlob_id& blob_id)
{
    if ( !m_DataSource ) {
        m_DataSource = new CDataSource;
    }
    CConstRef<CObject> key(new CBlob_id(blob_id));
    return m_DataSource->GetTSE_LoadLock(key);
}


CStandaloneRequestResult::operator CInitMutexPool&(void)
{
    return m_MutexPool;
}


CStandaloneRequestResult::TConn CStandaloneRequestResult::GetConn(void)
{
    return 0;
}


void CStandaloneRequestResult::ReleaseConn(void)
{
}


END_SCOPE(objects)
END_NCBI_SCOPE
