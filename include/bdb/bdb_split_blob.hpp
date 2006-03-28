#ifndef BDB___SPLIT_BLOB_HPP__
#define BDB___SPLIT_BLOB_HPP__

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
 * Author:  Anatoliy Kuznetsov
 *   
 * File Description: BDB library split BLOB store.
 *
 */


/// @file bdb_split_blob.hpp
/// BDB library split BLOB store.

#include <corelib/ncbistd.hpp>
#include <corelib/ncbimtx.hpp>

#include <bdb/bdb_blob.hpp>
#include <bdb/bdb_bv_store.hpp>
#include <bdb/bdb_cursor.hpp>

#include <util/id_mux.hpp>


BEGIN_NCBI_SCOPE

/** @addtogroup BDB_BLOB
 *
 * @{
 */


/// Persistent storage for demux information
///
template<class TBV>
class CBDB_BlobStoreDict : public CBDB_BvStore<TBV>
{
public:
    CBDB_FieldUint4        dim;     ///< dimention
    CBDB_FieldUint4        dim_idx; ///< projection index

    typedef CBDB_BvStore<TBV>         TParent;

public:
    CBDB_BlobStoreDict()
    {
        this->BindKey("dim",       &dim);
        this->BindKey("dim_idx",   &dim_idx);
    }
};

/// Volume split BLOB demultiplexer
///
/// This class is doing some simple accounting, counting size and number
/// of incoming LOBs, splitting them into [volume, page size]
///
class CBDB_BlobDeMux : public IObjDeMux<unsigned>
{
public:
    typedef vector<double>    TVolumeSize;
    typedef vector<unsigned>  TVolumeRecs;

public:
    CBDB_BlobDeMux(double    vol_max = 1.5 * (1024*1024*1024), 
                  unsigned  rec_max = 3 * 1000000) 
        : m_VolMax(vol_max), m_RecMax(rec_max)
    {
        NewVolume(); // have at least one volume
    }

    /// coordinates:
    ///
    ///  0 - active volume number
    ///  1 - page split number
    ///
    void GetCoordinates(unsigned blob_size, unsigned* coord)
    {
        _ASSERT(m_VolS.size());
        _ASSERT(m_RecS.size());

        coord[0] = m_RecS.size() - 1;      // current volume number
        coord[1] = SelectSplit(blob_size);  // size based split

        TVolumeSize::iterator it = m_VolS.end(); --it;
        double new_size = *it + blob_size;
        *it = new_size;
        ++(m_RecS[coord[0]]);  // inc. number of LOBs in the volume 
        
        if (new_size > m_VolMax) {
            NewVolume();
        }
    }
protected:
    void NewVolume()
    {
        m_VolS.push_back(0);
        m_RecS.push_back(0);
    }

    /// LOBs are getting split into slices based on LOB size,
    /// similar BLOBs go to the compartment with more optimal storage 
    /// paramaters
    ///
    static
    unsigned SelectSplit(unsigned blob_size)
    {
        static unsigned size_split[] = {256, 512, 2048, 4096, 8192};
        for(unsigned i = 0; i < sizeof(size_split)/sizeof(*size_split); ++i) {
            if (blob_size < size_split[i])  
                return i;
        }
        return 5;
    }

protected:
    TVolumeSize  m_VolS;  ///< Volumes BLOB sizes
    TVolumeRecs  m_RecS;  ///< Volumes record counts
    
    double       m_VolMax; ///< Volume max size
    unsigned     m_RecMax; ///< Maximum number of records
};


/// BLOB storage based on single unsigned integer key
/// Supports BLOB volumes and different base page size files in the volume
/// to guarantee the best fit.
///
/// Object maintains a matrix of BDB databases.
/// Every row maintains certain database volume or(and) number of records.
/// Every column groups BLOBs of certain size together, so class can choose
/// the best page size to store BLOBs without long chains of overflow pages.
/// <pre>
///                      Page size split:
///  Volume 
///  split:        4K     8K     16K    32K
///              +------+------+------+------+
///  row = 0     | DB   | ...................|  = SUM = N Gbytes
///  row = 1     | DB   | .....              |  = SUM = N GBytes
///
///                .........................
///
///              +------+------+------+------+
///
/// </pre>
///
template<class TBV, class TObjDeMux=CBDB_BlobDeMux, class TL=CFastMutex>
class CBDB_BlobSpitStore
{
public:
    typedef CIdDeMux<TBV>                TIdDeMux;
    typedef TBV                          TBitVector;
    typedef CBDB_BlobStoreDict<TBV>      TDeMuxStore;
    typedef TL                           TLock;
    typedef typename TL::TWriteLockGuard TLockGuard;
    
    /// BDB Database together with the locker
    struct SLockedDb 
    {
        AutoPtr<CBDB_IdBlobFile> db;    ///< database file
        AutoPtr<TLock>           lock;  ///< db lock
    };

    /// Volume split on optimal page size
    struct SVolume 
    {
        vector<SLockedDb>  db_vect;        
    };

    typedef vector<SVolume*>  TVolumeVect;

public:
    /// Construction
    /// The main parameter here is object demultiplexer for splitting
    /// incoming LOBs into volumes and slices
    ///
    CBDB_BlobSpitStore(TObjDeMux* de_mux);
    ~CBDB_BlobSpitStore();

    /// Open storage (reads storage dictionary into memory)
    void Open(const string&             storage_name, 
              CBDB_RawFile::EOpenMode   open_mode);

    /// Save storage dictionary (demux disposition).
    /// If you modified storage (like added new BLOBs to the storage)
    /// you MUST call save; otherwise some disposition information is lost.
    ///
    void Save();


    void SetVolumeCacheSize(unsigned int cache_size) 
        { m_VolumeCacheSize = cache_size; }
    /// Associate with the environment. Should be called before opening.
    void SetEnv(CBDB_Env& env) { m_Env = &env; }


    // ---------------------------------------------------------------
    // Data manipulation interface
    // ---------------------------------------------------------------

    /// Insert BLOB into the storage.
    ///
    /// This method does NOT check if this object is already storead
    /// somewhere. Method can create duplicates.
    ///
    /// @param id      insertion key
    /// @param data    buffer pointer
    /// @param size    LOB data size in bytes
    ///
    EBDB_ErrCode Insert(unsigned  id, 
                        const void* data, size_t size);

    /// Update or insert BLOB
    EBDB_ErrCode UpdateInsert(unsigned id, 
                              const void* data, size_t size);

    /// Read BLOB into vector. 
    /// If BLOB does not fit, method resizes the vector to accomodate.
    /// 
    EBDB_ErrCode ReadRealloc(unsigned id, 
	                         vector<char>& buffer, size_t* buf_size);


    /// Fetch LOB record directly into the provided '*buf'.
    /// If size of the LOB is greater than 'buf_size', then
    /// if reallocation is allowed -- '*buf' will be reallocated
    /// to fit the LOB size; otherwise, a exception will be thrown.
    ///
    EBDB_ErrCode Fetch(unsigned     id, 
                       void**       buf, 
                       size_t       buf_size, 
                       CBDB_RawFile::EReallocMode allow_realloc);

    /// Get size of the BLOB
    ///
    /// @note Price of this operation is almost the same as getting
    /// the actual BLOB. It is often better just to fetch BLOB speculatively, 
    /// hoping it fits in the buffer and resizing the buffer on exception.
    ///
    EBDB_ErrCode BlobSize(unsigned   id, 
                          size_t*    blob_size);

    /// Delete BLOB
    EBDB_ErrCode Delete(unsigned id, 
	                    CBDB_RawFile::EIgnoreError on_error = 
						                        CBDB_RawFile::eThrowOnError);


protected:
    /// Close volumes without saving or doing anything with id demux
    void CloseVolumes();

    void LoadIdDeMux(TIdDeMux& de_mux, TDeMuxStore& dict_file);

    /// Store id demux (projection vectors) into the database file
    void SaveIdDeMux(const TIdDeMux& de_mux, TDeMuxStore& dict_file);

    /// Select preferred page size for the specified slice
    unsigned GetPageSize(unsigned splice) const;

    /// Open split storage dictionary
    void OpenDict();

    /// Make BDB file name based on volume and page size split
    string MakeDbFileName(unsigned vol, 
                          unsigned slice);

    /// Get database pair (method opens and mounts database if necessary)
    SLockedDb& GetDb(unsigned vol, unsigned slice);

    /// Init database mutex lock (mathod is protected against double init)
    void InitDbMutex(SLockedDb* ldb); 

protected:
    vector<unsigned>        m_PageSizes;
    unsigned                m_VolumeCacheSize;
    CBDB_Env*               m_Env;
    auto_ptr<TDeMuxStore>   m_DictFile;  ///< Split dictionary(id demux file)

    auto_ptr<TIdDeMux>      m_IdDeMux;   ///< Id to coordinates mapper
    mutable CRWLock         m_IdDeMuxLock;

    auto_ptr<TObjDeMux>     m_ObjDeMux;  ///< Obj to coordinates mapper
    TLock                   m_ObjDeMuxLock;

    TVolumeVect             m_Volumes;     ///< Volumes
    mutable TLock           m_VolumesLock; ///< Volumes locker

    string                  m_StorageName;
    CBDB_RawFile::EOpenMode m_OpenMode;

};

/* @} */


/////////////////////////////////////////////////////////////////////////////
//  IMPLEMENTATION of INLINE functions
/////////////////////////////////////////////////////////////////////////////


template<class TBV, class TObjDeMux, class TL>
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::CBDB_BlobSpitStore(TObjDeMux* de_mux)
 : m_PageSizes(7),
   m_VolumeCacheSize(0),
   m_Env(0),
   m_IdDeMux(new TIdDeMux(2)),
   m_ObjDeMux(de_mux)
{
    m_PageSizes[0] = 0;
    m_PageSizes[1] = 0;
    m_PageSizes[2] = 8 * 1024;
    m_PageSizes[3] = 16* 1024;
    m_PageSizes[4] = 16* 1024;
    m_PageSizes[6] = 32* 1024;
}

template<class TBV, class TObjDeMux, class TL>
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::~CBDB_BlobSpitStore()
{
    CloseVolumes();
}

template<class TBV, class TObjDeMux, class TL>
void CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::CloseVolumes()
{

	for (size_t i = 0; i < m_Volumes.size(); ++i) {
		SVolume* v = m_Volumes[i];
		delete v;
	}
}

template<class TBV, class TObjDeMux, class TL>
EBDB_ErrCode 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::Insert(unsigned int id, 
                                               const void*  data, 
                                               size_t       size)
{
    unsigned coord[2];
    {{
        TLockGuard lg(m_ObjDeMuxLock);
        m_ObjDeMux->GetCoordinates(size, coord);
    }}

    {{
        CWriteLockGuard lg(m_IdDeMuxLock);
        m_IdDeMux->SetCoordinatesFast(id, coord);
    }}

    SLockedDb& dbp = this->GetDb(coord[0], coord[1]);
    {{
        TLockGuard lg(*(dbp.lock));
        dbp.db->id = id;
        return dbp.db->Insert(data, size);
    }}
}

template<class TBV, class TObjDeMux, class TL>
EBDB_ErrCode 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::UpdateInsert(unsigned int id, 
                                                     const void*  data, 
                                                     size_t       size)
{
    unsigned coord[2];
    bool found;
    {{
        CReadLockGuard lg(m_IdDeMuxLock);
        found = m_IdDeMux->GetCoordinatesFast(id, coord);
    }}
    if (!found) {
        return Insert(id, data, size);
    }
    SLockedDb& dbp = this->GetDb(coord[0], coord[1]);
    {{
        TLockGuard lg(*(dbp.lock));
        dbp.db->id = id;
        return dbp.db->UpdateInsert(data, size);
    }}
}

template<class TBV, class TObjDeMux, class TL>
EBDB_ErrCode 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::Delete(unsigned     id, 
                                               CBDB_RawFile::EIgnoreError on_error)
{
    unsigned coord[2];
    bool found;
    {{
        CReadLockGuard lg(m_IdDeMuxLock);
        found = m_IdDeMux->GetCoordinatesFast(id, coord);
    }}
    if (!found) {
        return eBDB_NotFound;
    }
    SLockedDb& dbp = this->GetDb(coord[0], coord[1]);
    {{
        TLockGuard lg(*(dbp.lock));
        dbp.db->id = id;
        return dbp.db->Delete(on_error);
    }}
}


template<class TBV, class TObjDeMux, class TL>
EBDB_ErrCode 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::Fetch(unsigned     id, 
                                              void**       buf, 
                                              size_t       buf_size, 
                               CBDB_RawFile::EReallocMode allow_realloc)
{
    unsigned coord[2];
    bool found;
    {{
        CReadLockGuard lg(m_IdDeMuxLock);
        found = m_IdDeMux->GetCoordinatesFast(id, coord);
    }}
    if (!found) {
        return eBDB_NotFound;
    }
    SLockedDb& dbp = this->GetDb(coord[0], coord[1]);
    {{
        TLockGuard lg(*(dbp.lock));
        dbp.db->id = id;
        return dbp.db->Fetch(buf, buf_size, allow_realloc);
    }}
}

template<class TBV, class TObjDeMux, class TL>
EBDB_ErrCode 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::ReadRealloc(unsigned      id,
                                                    vector<char>& buffer, 
                                                    size_t*       buf_size)
{
    unsigned coord[2];
    bool found;
    {{
        CReadLockGuard lg(m_IdDeMuxLock);
        found = m_IdDeMux->GetCoordinatesFast(id, coord);
    }}
    if (!found) {
        return eBDB_NotFound;
    }
    SLockedDb& dbp = this->GetDb(coord[0], coord[1]);
    {{
        TLockGuard lg(*(dbp.lock));
        dbp.db->id = id;
        EBDB_ErrCode e = dbp.db->ReadRealloc(id, buffer, buf_size);
        return e;
    }}

}

template<class TBV, class TObjDeMux, class TL>
EBDB_ErrCode 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::BlobSize(unsigned   id, 
                                                 size_t*    blob_size)
{
    unsigned coord[2];
    bool found;
    {{
        CReadLockGuard lg(m_IdDeMuxLock);
        found = m_IdDeMux->GetCoordinatesFast(id, coord);
    }}
    if (!found) {
        return eBDB_NotFound;
    }
    SLockedDb& dbp = this->GetDb(coord[0], coord[1]);
    {{
        TLockGuard lg(*(dbp.lock));
        dbp.db->id = id;
        EBDB_ErrCode e = dbp.db->Fetch();
        if (e != eBDB_Ok) {
            return e;
        }
        *blob_size = dbp.db->LobSize();
        return e;
    }}
}


template<class TBV, class TObjDeMux, class TL>
void 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::Open(const string&  storage_name, 
                                          CBDB_RawFile::EOpenMode   open_mode)
{
    CloseVolumes();
    m_StorageName = storage_name;
    m_OpenMode = open_mode;

    OpenDict();
    LoadIdDeMux(*m_IdDeMux, *m_DictFile);
}

template<class TBV, class TObjDeMux, class TL>
void 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::OpenDict()
{
    m_DictFile.reset(new TDeMuxStore);
    if (m_Env) {
        m_DictFile->SetEnv(*m_Env);
    }
    string dict_fname(m_StorageName);
    dict_fname.append(".splitd");

    m_DictFile->Open(dict_fname.c_str(), m_OpenMode);

    m_IdDeMux.reset(new TIdDeMux(2));
}

template<class TBV, class TObjDeMux, class TL>
void 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::LoadIdDeMux(TIdDeMux&      de_mux, 
                                                   TDeMuxStore&   dict_file)
{
    CBDB_FileCursor cur(dict_file);
    cur.SetCondition(CBDB_FileCursor::eGE);
    cur.From << 0;

    typename TDeMuxStore::TBuffer& buf = dict_file.GetBuffer();
    EBDB_ErrCode err;
    while (true) {
        err = dict_file.FetchToBuffer(cur);
        if (err != eBDB_Ok) {
            break;
        }
        unsigned dim = dict_file.dim;
        unsigned dim_idx = dict_file.dim_idx;

        auto_ptr<TBitVector>  bv(new TBitVector(bm::BM_GAP));
        dict_file.Deserialize(bv.get(), &buf[0]);

        de_mux.SetProjection(dim, dim_idx, bv.release());
        
    } // while
}

template<class TBV, class TObjDeMux, class TL>
void 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::Save()
{
    this->SaveIdDeMux(*m_IdDeMux, *m_DictFile);
}


template<class TBV, class TObjDeMux, class TL>
void 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::SaveIdDeMux(const TIdDeMux& de_mux, 
                                                   TDeMuxStore&    dict_file)
{
    size_t N = de_mux.GetN();
    for (size_t i = 0; i < N; ++i) {
        const typename TIdDeMux::TDimVector& dv = de_mux.GetDimVector(i);

        for (size_t j = 0; j < dv.size(); ++j) {
            dict_file.dim = i; 
            dict_file.dim_idx = j;

            const TBitVector* bv = dv[j].get();
            if (!bv) {
                dict_file.Delete(CBDB_RawFile::eIgnoreError);
            } else {
                dict_file.WriteVector(*bv, TDeMuxStore::eCompact);
            }

        } // for j
    } // for i
}

template<class TBV, class TObjDeMux, class TL>
unsigned 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::GetPageSize(unsigned splice) const
{
    if (splice < m_PageSizes.size()) 
        return m_PageSizes[splice];
    return 32* 1024;
}

template<class TBV, class TObjDeMux, class TL>
string 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::MakeDbFileName(unsigned vol, 
                                                      unsigned slice)
{
    return m_StorageName + "_" + 
           NStr::UIntToString(vol) + "_" + NStr::UIntToString(slice) + ".db";
}

template<class TBV, class TObjDeMux, class TL>
void CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::InitDbMutex(SLockedDb* ldb)
{
    if (ldb->lock.get() == 0) {
        TLockGuard lg(m_VolumesLock);
        if (ldb->lock.get() == 0) {
            ldb->lock.reset(new TLock);
        }        
    }
}


template<class TBV, class TObjDeMux, class TL>
typename CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::SLockedDb& 
CBDB_BlobSpitStore<TBV, TObjDeMux, TL>::GetDb(unsigned vol, unsigned slice)
{
    // speculative un-locked check if everything is open already
    // (we don't close or shrink the store in parallel, so it is safe)
    SLockedDb* lp = 0;

    if ((m_Volumes.size() > vol) && 
        ((m_Volumes[vol])->db_vect.size() > slice)
        ) {
        SVolume& volume = *(m_Volumes[vol]);
        lp = &(volume.db_vect[slice]);
        InitDbMutex(lp);
        return *lp;
    }

    // lock protected open

    {{
    TLockGuard lg(m_VolumesLock);
    while (m_Volumes.size() < (vol+1)) {
        m_Volumes.push_back(new SVolume);
    }

    SVolume& volume = *(m_Volumes[vol]);
    if (volume.db_vect.size() <= slice) {
        volume.db_vect.resize(slice+1);
    }
    lp = &(volume.db_vect[slice]);

    if (lp->lock.get() == 0) {
        lp->lock.reset(new TLock);
    }
    }}

    {{
    InitDbMutex(lp);
    TLockGuard lg(*(lp->lock));
    if (lp->db.get() == 0) {
        string fname = this->MakeDbFileName(vol, slice);
        lp->db.reset(new CBDB_IdBlobFile);
        if (m_Env) {
            lp->db->SetEnv(*m_Env);
        } else {
            if (m_VolumeCacheSize) {
                lp->db->SetCacheSize(m_VolumeCacheSize);
            }
        }
        unsigned page_size = GetPageSize(slice);
        if (page_size) {
            lp->db->SetPageSize(page_size);
        }
        lp->db->Open(fname.c_str(), m_OpenMode);
    }
    }}

    return *lp;
}





END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.2  2006/03/28 17:17:03  kuznets
 * Fixed GCC compilation issues
 *
 * Revision 1.1  2006/03/28 16:41:59  kuznets
 * Initial revision
 *
 *
 * ===========================================================================
 */

#endif 

