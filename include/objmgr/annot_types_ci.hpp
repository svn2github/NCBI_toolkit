#ifndef ANNOT_TYPES_CI__HPP
#define ANNOT_TYPES_CI__HPP

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
* Author: Aleksey Grichenko, Michael Kimelman
*
* File Description:
*   Object manager iterators
*
*/

#include <objects/objmgr/bioseq_handle.hpp>
#include <objects/objmgr/annot_ci.hpp>
#include <objects/objmgr/impl/tse_info.hpp>
#include <objects/seqloc/Na_strand.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/objmgr/impl/annot_object.hpp>
#include <corelib/ncbiobj.hpp>
#include <set>
#include <map>
#include <memory>
#include <deque>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CScope;
class CTSE_Info;
class CSeq_loc;
class CSeqMap_CI;
class CAnnotObject_Info;
class CFeat_Less;
class CAnnotObject_Less;

class NCBI_XOBJMGR_EXPORT CAnnotObject_Ref
{
public:
    CAnnotObject_Ref(void);
    CAnnotObject_Ref(const CAnnotObject_Info& object);
    ~CAnnotObject_Ref(void);

    const CAnnotObject_Info& Get(void) const;

    bool IsPartial(void) const;
    void SetPartial(bool value);

    bool IsMappedLoc(void) const;
    const CSeq_loc& GetMappedLoc(void) const;
    CSeq_loc& SetMappedLoc(void);
    void SetMappedLoc(CSeq_loc& loc);
    const CSeq_loc& GetFeatLoc(void) const;

    bool IsMappedProd(void) const;
    const CSeq_loc& GetMappedProd(void) const;
    CSeq_loc& SetMappedProd(void);
    void SetMappedProd(CSeq_loc& loc);

private:
    friend class CFeat_Less;
    friend class CAnnotObject_Less;

    CConstRef<CAnnotObject_Info> m_Object;
    CRef<CSeq_loc>          m_MappedLoc;  // master sequence coordinates
    CRef<CSeq_loc>          m_MappedProd; // master sequence coordinates
    bool                    m_Partial;    // Partial flag (same as in features)
};


class NCBI_XOBJMGR_EXPORT CFeat_Less
{
public:
    // Compare CRef-s: if at least one is NULL, compare as pointers,
    // otherwise call non-inline x_CompareAnnot() method
    bool operator ()(const CAnnotObject_Ref& x,
                     const CAnnotObject_Ref& y) const;
};


class NCBI_XOBJMGR_EXPORT CAnnotObject_Less
{
public:
    // Compare CRef-s: if at least one is NULL, compare as pointers,
    // otherwise call non-inline x_CompareAnnot() method
    bool operator ()(const CAnnotObject_Ref& x,
                     const CAnnotObject_Ref& y) const;
};


// Base class for specific annotation iterators
class NCBI_XOBJMGR_EXPORT CAnnotTypes_CI
{
public:
    // Flag to indicate references resolution method
    enum EResolveMethod {
        eResolve_None, // Do not search annotations on segments
        eResolve_TSE,  // Search annotations only on sequences in the same TSE
        eResolve_All   // Search annotations for all referenced sequences
    };

    CAnnotTypes_CI(void);
    // Search all TSEs in all datasources, by default get annotations defined
    // on segments in the same TSE (eResolve_TSE method).
    CAnnotTypes_CI(CScope& scope,
                   const CSeq_loc& loc,
                   SAnnotSelector selector,
                   CAnnot_CI::EOverlapType overlap_type,
                   EResolveMethod resolve,
                   const CSeq_entry* entry = 0);
    // Search only in TSE, containing the bioseq, by default get annotations
    // defined on segments in the same TSE (eResolve_TSE method).
    // If "entry" is set, search only annotations from the seq-entry specified
    // (but no its sub-entries or parent entry).
    CAnnotTypes_CI(const CBioseq_Handle& bioseq,
                   TSeqPos start, TSeqPos stop,
                   SAnnotSelector selector,
                   CAnnot_CI::EOverlapType overlap_type,
                   EResolveMethod resolve,
                   const CSeq_entry* entry = 0);
    CAnnotTypes_CI(const CAnnotTypes_CI& it);
    virtual ~CAnnotTypes_CI(void);

    CAnnotTypes_CI& operator= (const CAnnotTypes_CI& it);

    typedef set<CTSE_Lock> TTSESet;

    const CSeq_annot& GetSeq_annot(void) const;

protected:
    // Check if a datasource and an annotation are selected.
    bool IsValid(void) const;
    // Move to the next valid position
    void Walk(void);
    // Return current annotation
    const CAnnotObject_Ref& Get(void) const;

private:
    typedef vector<CAnnotObject_Ref> TAnnotSet;

    void x_Initialize(const CHandleRangeMap& master_loc);
    void x_SearchMain(CHandleRangeMap loc);
    void x_SearchLocation(const CSeqMap_CI& seg,
                          CHandleRangeMap::const_iterator master_loc);
    
    // Release all locked resources TSE etc
    void x_ReleaseAll(void);
    // Convert seq-loc to the master location coordinates, return ePartial
    // if any location was adjusted (used as Partial flag for features),
    // eMapped if a location was recalculated but not truncated, eNone
    // if no convertions were necessary.
    enum EConverted {
        eConverted_None,
        eConverted_Mapped,
        eConverted_Partial
    };

    SAnnotSelector               m_Selector;
    // Set of all the annotations found
    TAnnotSet                    m_AnnotSet;
    // TSE set to keep all the TSEs locked
    TTSESet                      m_TSESet;
    // Current annotation
    TAnnotSet::const_iterator    m_CurAnnot;
    mutable CRef<CScope>         m_Scope;
    // If non-zero, search annotations in the "native" TSE only
    CTSE_Lock                    m_NativeTSE;
    // Search only within this seq-entry if set
    CConstRef<CSeq_entry>        m_SingleEntry;
    // Reference resolving method
    EResolveMethod               m_ResolveMethod;
    // Overlap type for CAnnot_CI
    CAnnot_CI::EOverlapType      m_OverlapType;
};


inline
const CAnnotObject_Info& CAnnotObject_Ref::Get(void) const
{
    return *m_Object;
}

inline
bool CAnnotObject_Ref::IsPartial(void) const
{
    return m_Partial;
}

inline
void CAnnotObject_Ref::SetPartial(bool value)
{
    m_Partial = value;
}

inline
bool CAnnotObject_Ref::IsMappedLoc(void) const
{
    return bool(m_MappedLoc);
}

inline
const CSeq_loc& CAnnotObject_Ref::GetMappedLoc(void) const
{
    _ASSERT(m_MappedLoc);
    return *m_MappedLoc;
}

inline
CSeq_loc& CAnnotObject_Ref::SetMappedLoc(void)
{
    if (!m_MappedLoc) {
        m_MappedLoc.Reset(new CSeq_loc);
    }
    return *m_MappedLoc;
}

inline
void CAnnotObject_Ref::SetMappedLoc(CSeq_loc& loc)
{
    m_MappedLoc.Reset(&loc);
}

inline
const CSeq_loc& CAnnotObject_Ref::GetFeatLoc(void) const
{
    _ASSERT(Get().IsFeat());
    return m_MappedLoc? *m_MappedLoc: Get().GetFeat().GetLocation();
}

inline
bool CAnnotObject_Ref::IsMappedProd(void) const
{
    return bool(m_MappedProd);
}

inline
const CSeq_loc& CAnnotObject_Ref::GetMappedProd(void) const
{
    _ASSERT(m_MappedProd);
    return *m_MappedProd;
}

inline
CSeq_loc& CAnnotObject_Ref::SetMappedProd(void)
{
    if (!m_MappedProd) {
        m_MappedProd.Reset(new CSeq_loc);
    }
    return *m_MappedProd;
}

inline
void CAnnotObject_Ref::SetMappedProd(CSeq_loc& loc)
{
    m_MappedProd.Reset(&loc);
}

inline
bool CAnnotTypes_CI::IsValid(void) const
{
    return m_CurAnnot != m_AnnotSet.end();
}


inline
void CAnnotTypes_CI::Walk(void)
{
    ++m_CurAnnot;
}


inline
const CAnnotObject_Ref& CAnnotTypes_CI::Get(void) const
{
    _ASSERT( IsValid() );
    return *m_CurAnnot;
}


inline
const CSeq_annot& CAnnotTypes_CI::GetSeq_annot(void) const
{
    return Get().Get().GetSeq_annot();
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.29  2003/02/24 21:35:21  vasilche
* Reduce checks in CAnnotObject_Ref comparison.
* Fixed compilation errors on MS Windows.
* Removed obsolete file src/objects/objmgr/annot_object.hpp.
*
* Revision 1.28  2003/02/24 18:57:20  vasilche
* Make feature gathering in one linear pass using CSeqMap iterator.
* Do not use feture index by sub locations.
* Sort features at the end of gathering in one vector.
* Extracted some internal structures and classes in separate header.
* Delay creation of mapped features.
*
* Revision 1.27  2003/02/13 14:34:31  grichenk
* Renamed CAnnotObject -> CAnnotObject_Info
* + CSeq_annot_Info and CAnnotObject_Ref
* Moved some members of CAnnotObject to CSeq_annot_Info
* and CAnnotObject_Ref.
* Added feat/align/graph to CAnnotObject_Info map
* to CDataSource.
*
* Revision 1.26  2003/02/04 21:44:10  grichenk
* Convert seq-loc instead of seq-annot to the master coordinates
*
* Revision 1.25  2002/12/26 20:51:35  dicuccio
* Added Win32 export specifier
*
* Revision 1.24  2002/12/26 16:39:21  vasilche
* Object manager class CSeqMap rewritten.
*
* Revision 1.23  2002/12/24 15:42:44  grichenk
* CBioseqHandle argument to annotation iterators made const
*
* Revision 1.22  2002/12/06 15:35:57  grichenk
* Added overlap type for annot-iterators
*
* Revision 1.21  2002/11/04 21:28:58  grichenk
* Fixed usage of const CRef<> and CRef<> constructor
*
* Revision 1.20  2002/11/01 20:46:41  grichenk
* Added sorting to set< CRef<CAnnotObject> >
*
* Revision 1.19  2002/10/08 18:57:27  grichenk
* Added feature sorting to the iterator class.
*
* Revision 1.18  2002/07/08 20:50:56  grichenk
* Moved log to the end of file
* Replaced static mutex (in CScope, CDataSource) with the mutex
* pool. Redesigned CDataSource data locking.
*
* Revision 1.17  2002/05/31 17:52:58  grichenk
* Optimized for better performance (CTSE_Info uses atomic counter,
* delayed annotations indexing, no location convertions in
* CAnnot_Types_CI if no references resolution is required etc.)
*
* Revision 1.16  2002/05/24 14:58:53  grichenk
* Fixed Empty() for unsigned intervals
* SerialAssign<>() -> CSerialObject::Assign()
* Improved performance for eResolve_None case
*
* Revision 1.15  2002/05/06 03:30:35  vakatov
* OM/OM1 renaming
*
* Revision 1.14  2002/05/03 21:28:01  ucko
* Introduce T(Signed)SeqPos.
*
* Revision 1.13  2002/04/30 14:30:41  grichenk
* Added eResolve_TSE flag in CAnnot_Types_CI, made it default
*
* Revision 1.12  2002/04/22 20:06:15  grichenk
* Minor changes in private interface
*
* Revision 1.11  2002/04/17 21:11:58  grichenk
* Fixed annotations loading
* Set "partial" flag in features if necessary
* Implemented most seq-loc types in reference resolving methods
* Fixed searching for annotations within a signle TSE
*
* Revision 1.10  2002/04/11 12:07:28  grichenk
* Redesigned CAnnotTypes_CI to resolve segmented sequences correctly.
*
* Revision 1.9  2002/04/05 21:26:16  grichenk
* Enabled iteration over annotations defined on segments of a
* delta-sequence.
*
* Revision 1.8  2002/03/07 21:25:31  grichenk
* +GetSeq_annot() in annotation iterators
*
* Revision 1.7  2002/03/05 16:08:12  grichenk
* Moved TSE-restriction to new constructors
*
* Revision 1.6  2002/03/04 15:07:46  grichenk
* Added "bioseq" argument to CAnnotTypes_CI constructor to iterate
* annotations from a single TSE.
*
* Revision 1.5  2002/02/21 19:27:00  grichenk
* Rearranged includes. Added scope history. Added searching for the
* best seq-id match in data sources and scopes. Updated tests.
*
* Revision 1.4  2002/02/15 20:36:29  gouriano
* changed implementation of HandleRangeMap
*
* Revision 1.3  2002/02/07 21:27:33  grichenk
* Redesigned CDataSource indexing: seq-id handle -> TSE -> seq/annot
*
* Revision 1.2  2002/01/16 16:26:35  gouriano
* restructured objmgr
*
* Revision 1.1  2002/01/11 19:03:59  gouriano
* restructured objmgr
*
*
* ===========================================================================
*/

#endif  // ANNOT_TYPES_CI__HPP
