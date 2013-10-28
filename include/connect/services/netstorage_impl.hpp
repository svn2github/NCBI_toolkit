#ifndef CONNECT_SERVICES___NETSTORAGE_IMPL__HPP
#define CONNECT_SERVICES___NETSTORAGE_IMPL__HPP

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
 * Author:  Dmitry Kazimirov
 *
 * File Description:
 *   NetStorage implementation declarations.
 *
 */

#include "netstorage.hpp"
#include "compound_id.hpp"

BEGIN_NCBI_SCOPE

/// @internal
struct NCBI_XCONNECT_EXPORT SNetFileImpl : public CObject
{
    virtual string GetID() = 0;
    virtual size_t Read(void* buffer, size_t buf_size) = 0;
    virtual void Read(string* data);
    virtual bool Eof() = 0;
    virtual void Write(const void* buffer, size_t buf_size) = 0;
    virtual Uint8 GetSize() = 0;
    virtual CNetFileInfo GetInfo() = 0;
    virtual void Close() = 0;
};

inline string CNetFile::GetID()
{
    return m_Impl->GetID();
}

inline size_t CNetFile::Read(void* buffer, size_t buf_size)
{
    return m_Impl->Read(buffer, buf_size);
}

inline void CNetFile::Read(string* data)
{
    m_Impl->Read(data);
}

inline bool CNetFile::Eof()
{
    return m_Impl->Eof();
}

inline void CNetFile::Write(const void* buffer, size_t buf_size)
{
    m_Impl->Write(buffer, buf_size);
}

inline void CNetFile::Write(const string& data)
{
    m_Impl->Write(data.data(), data.length());
}

inline Uint8 CNetFile::GetSize()
{
    return m_Impl->GetSize();
}

inline CNetFileInfo CNetFile::GetInfo()
{
    return m_Impl->GetInfo();
}

inline void CNetFile::Close()
{
    m_Impl->Close();
}

/// @internal
struct NCBI_XCONNECT_EXPORT SNetStorageImpl : public CObject
{
    virtual CNetFile Create(TNetStorageFlags flags = 0) = 0;
    virtual CNetFile Open(const string& file_id,
            TNetStorageFlags flags = 0) = 0;
    virtual string Relocate(const string& file_id, TNetStorageFlags flags) = 0;
    virtual bool Exists(const string& file_id) = 0;
    virtual void Remove(const string& file_id) = 0;
};

inline CNetFile CNetStorage::Create(TNetStorageFlags flags)
{
    return m_Impl->Create(flags);
}

inline CNetFile CNetStorage::Open(const string& file_id, TNetStorageFlags flags)
{
    return m_Impl->Open(file_id, flags);
}

inline string CNetStorage::Relocate(const string& file_id,
        TNetStorageFlags flags)
{
    return m_Impl->Relocate(file_id, flags);
}

inline bool CNetStorage::Exists(const string& file_id)
{
    return m_Impl->Exists(file_id);
}

inline void CNetStorage::Remove(const string& file_id)
{
    m_Impl->Remove(file_id);
}

/// @internal
struct NCBI_XCONNECT_EXPORT SNetStorageByKeyImpl : public CObject
{
    virtual CNetFile Open(const string& unique_key,
            TNetStorageFlags flags = 0) = 0;
    virtual string Relocate(const string& unique_key,
            TNetStorageFlags flags, TNetStorageFlags old_flags = 0) = 0;
    virtual bool Exists(const string& key, TNetStorageFlags flags = 0) = 0;
    virtual void Remove(const string& key, TNetStorageFlags flags = 0) = 0;
};

inline CNetFile CNetStorageByKey::Open(const string& unique_key,
        TNetStorageFlags flags)
{
    return m_Impl->Open(unique_key, flags);
}

inline string CNetStorageByKey::Relocate(const string& unique_key,
        TNetStorageFlags flags, TNetStorageFlags old_flags)
{
    return m_Impl->Relocate(unique_key, flags, old_flags);
}

inline bool CNetStorageByKey::Exists(const string& key, TNetStorageFlags flags)
{
    return m_Impl->Exists(key, flags);
}

inline void CNetStorageByKey::Remove(const string& key, TNetStorageFlags flags)
{
    m_Impl->Remove(key, flags);
}

/// @internal
enum ENetFileCompoundIDCues {
    eNFCIDC_KeyAndNamespace,
    eNFCIDC_NetICache,
    eNFCIDC_AllowXSiteConn,
    eNFCIDC_TTL
};

/// @internal
enum ENetFileIDFields {
    fNFID_KeyAndNamespace   = (1 << eNFCIDC_KeyAndNamespace),
    fNFID_NetICache         = (1 << eNFCIDC_NetICache),
#ifdef NCBI_GRID_XSITE_CONN_SUPPORT
    fNFID_AllowXSiteConn    = (1 << eNFCIDC_AllowXSiteConn),
#endif
    fNFID_TTL               = (1 << eNFCIDC_TTL),
};
///< @internal Bitwise OR of ENetFileIDFields
typedef unsigned char TNetFileIDFields;

/// @internal
class NCBI_XCONNECT_EXPORT CNetFileID
{
public:
    CNetFileID(CCompoundIDPool::TInstance cid_pool,
            TNetStorageFlags flags,
            Uint8 random_number);
    CNetFileID(CCompoundIDPool::TInstance cid_pool,
            TNetStorageFlags flags,
            const string& app_domain,
            const string& unique_key);
    CNetFileID(CCompoundIDPool::TInstance cid_pool,
            const string& packed_id);

    void SetStorageFlags(TNetStorageFlags storage_flags)
    {
        m_StorageFlags = storage_flags;
        m_Dirty = true;
    }

    TNetStorageFlags GetStorageFlags() const {return m_StorageFlags;}
    TNetFileIDFields GetFields() const {return m_Fields;}

    Int8 GetTimestamp() const {return m_Timestamp;}
    Uint8 GetRandom() const {return m_Random;}

    string GetAppDomain() const {return m_AppDomain;}
    string GetUserKey() const {return m_UserKey;}

    // A combination of either timestamp+random or app_domain+user_key.
    string GetUniqueKey() const {return m_UniqueKey;}

    void ClearNetICacheParams();
    void SetNetICacheParams(const string& service_name,
        const string& cache_name, Uint4 server_ip, unsigned short server_port
#ifdef NCBI_GRID_XSITE_CONN_SUPPORT
        , bool allow_xsite_conn
#endif
        );

    string GetNCServiceName() const {return m_NCServiceName;}
    string GetCacheName() const {return m_CacheName;}
    Uint4 GetNetCacheIP() const {return m_NetCacheIP;}
    Uint2 GetNetCachePort() const {return m_NetCachePort;}

    void SetCacheChunkSize(size_t cache_chunk_size)
    {
        m_CacheChunkSize = cache_chunk_size;
        m_Dirty = true;
    }

    Uint8 GetCacheChunkSize() const {return m_CacheChunkSize;}

    void SetTTL(Uint8 ttl)
    {
        if (ttl == 0)
            ClearFieldFlags(fNFID_TTL);
        else {
            m_TTL = ttl;
            SetFieldFlags(fNFID_TTL);
        }
        m_Dirty = true;
    }

    Uint8 GetTTL() const {return m_TTL;}

    string GetID()
    {
        if (m_Dirty)
            x_Pack();
        return m_PackedID;
    }

    // Serialize to a JSON object.
    CJsonNode ToJSON() const;

private:
    void x_SetUniqueKeyFromRandom();
    void x_SetUniqueKeyFromUserDefinedKey();

    void x_Pack();
    void SetFieldFlags(TNetFileIDFields flags) {m_Fields |= flags;}
    void ClearFieldFlags(TNetFileIDFields flags) {m_Fields &= ~flags;}

    CCompoundIDPool m_CompoundIDPool;

    TNetStorageFlags m_StorageFlags;
    TNetFileIDFields m_Fields;

    Int8 m_Timestamp;
    Uint8 m_Random;

    string m_AppDomain;
    string m_UserKey;

    string m_UniqueKey;

    string m_NCServiceName;
    string m_CacheName;
    Uint4 m_NetCacheIP;
    Uint2 m_NetCachePort;
    Uint8 m_CacheChunkSize;
    Uint8 m_TTL;

    bool m_Dirty;

    string m_PackedID;
};

END_NCBI_SCOPE

#endif  /* CONNECT_SERVICES___NETSTORAGE_IMPL__HPP */
