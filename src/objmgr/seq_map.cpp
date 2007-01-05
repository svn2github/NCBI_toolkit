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
* Authors: Aleksey Grichenko, Michael Kimelman, Eugene Vasilchenko,
*          Andrei Gourianov
*
* File Description:
*   Sequence map for the Object Manager. Describes sequence as a set of
*   segments of different types (data, reference, gap or end).
*
*/

#include <ncbi_pch.hpp>
#include <objmgr/seq_map.hpp>
#include <objmgr/seq_map_ci.hpp>
//#include <objmgr/seq_map_ext.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/bioseq_handle.hpp>
#include <objmgr/impl/bioseq_info.hpp>
#include <objmgr/impl/tse_chunk_info.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Seq_literal.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Seg_ext.hpp>
#include <objects/seq/Ref_ext.hpp>

#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_point.hpp>
#include <objects/seqloc/Seq_loc_mix.hpp>
#include <objects/seqloc/Seq_loc_equiv.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqloc/Packed_seqint.hpp>
#include <objects/seqloc/Packed_seqpnt.hpp>

#include <algorithm>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

////////////////////////////////////////////////////////////////////
//  CSeqMap::CSegment

inline
CSeqMap::CSegment::CSegment(ESegmentType seg_type,
                            TSeqPos      length,
                            bool         unknown_len)
    : m_Position(kInvalidSeqPos),
      m_Length(length),
      m_UnknownLength(unknown_len),
      m_SegType(seg_type),
      m_ObjType(seg_type),
      m_RefMinusStrand(false),
      m_RefPosition(0)
{
}

////////////////////////////////////////////////////////////////////
//  CSeqMap


CSeqMap::CSeqMap(void)
    : m_Bioseq(0),
      m_Resolved(0),
      m_Mol(CSeq_inst::eMol_not_set),
      m_HasSegments(0),
      m_Changed(false),
      m_SeqLength(kInvalidSeqPos)
{
}


CSeqMap::CSeqMap(CSeqMap* /*parent*/, size_t /*index*/)
    : m_Bioseq(0),
      m_Resolved(0),
      m_Mol(CSeq_inst::eMol_not_set),
      m_HasSegments(0),
      m_Changed(false),
      m_SeqLength(kInvalidSeqPos)
{
}


CSeqMap::CSeqMap(const CSeq_loc& ref)
    : m_Bioseq(0),
      m_Resolved(0),
      m_Mol(CSeq_inst::eMol_not_set),
      m_HasSegments(0),
      m_Changed(false),
      m_SeqLength(kInvalidSeqPos)
{
    x_AddEnd();
    x_Add(ref);
    x_AddEnd();
}

/*
CSeqMap::CSeqMap(const CSeq_data& data, TSeqPos length)
    : m_Resolved(0),
      m_Mol(CSeq_inst::eMol_not_set),
      m_HasSegments(0),
      m_Changed(false),
      m_SeqLength(kInvalidSeqPos)
{
    x_AddEnd();
    x_Add(data, length);
    x_AddEnd();
}
*/

CSeqMap::CSeqMap(TSeqPos length)
    : m_Bioseq(0),
      m_Resolved(0),
      m_Mol(CSeq_inst::eMol_not_set),
      m_HasSegments(0),
      m_Changed(false),
      m_SeqLength(length)
{
    x_AddEnd();
    x_AddGap(length, false);
    x_AddEnd();
}


CSeqMap::CSeqMap(const CSeqMap& sm)
    : m_Bioseq(0),
      m_Segments(sm.m_Segments),
      m_Resolved(sm.m_Resolved),
      m_Delta(sm.m_Delta),
      m_Mol(sm.m_Mol),
      m_HasSegments(sm.m_HasSegments),
      m_Changed(sm.m_Changed),
      m_SeqLength(sm.m_SeqLength)
{
    NON_CONST_ITERATE ( TSegments, it, m_Segments ) {
        if ( it->m_ObjType == eSeqChunk ) {
            it->m_SegType = eSeqGap;
            it->m_ObjType = eSeqGap;
            it->m_RefObject = null;
        }
    }
}


CSeqMap::CSeqMap(const CSeq_inst& inst)
    : m_Bioseq(0),
      m_Resolved(0),
      m_Mol(CSeq_inst::eMol_not_set),
      m_HasSegments(0),
      m_Changed(false),
      m_SeqLength(kInvalidSeqPos)
{
    x_AddEnd();

    if ( inst.IsSetMol() ) {
        m_Mol = inst.GetMol();
    }
    if ( inst.IsSetLength() ) {
        m_SeqLength = inst.GetLength();
    }

    if ( inst.IsSetSeq_data() ) {
        if ( !inst.GetSeq_data().IsGap() ) {
            x_Add(inst.GetSeq_data(), inst.GetLength());
        }
        else {
            // split Seq-data
            x_AddGap(inst.GetLength(), false);
        }
    }
    else if ( inst.IsSetExt() ) {
        const CSeq_ext& ext = inst.GetExt();
        switch (ext.Which()) {
        case CSeq_ext::e_Seg:
            ITERATE ( CSeq_ext::TSeg::Tdata, iter, ext.GetSeg().Get() ) {
                x_Add(**iter);
            }
            break;
        case CSeq_ext::e_Ref:
            x_Add(ext.GetRef());
            break;
        case CSeq_ext::e_Delta:
            ITERATE ( CSeq_ext::TDelta::Tdata, iter, ext.GetDelta().Get() ) {
                x_Add(**iter);
            }
            break;
        case CSeq_ext::e_Map:
            //### Not implemented
            NCBI_THROW(CSeqMapException, eUnimplemented,
                       "CSeq_ext::e_Map -- not implemented");
        default:
            //### Not implemented
            NCBI_THROW(CSeqMapException, eUnimplemented,
                       "CSeq_ext::??? -- not implemented");
        }
    }
    else if ( inst.GetRepr() == CSeq_inst::eRepr_virtual ) {
        // Virtual sequence -- no data, no segments
        // The total sequence is gap
        x_AddGap(inst.GetLength(), false);
    }
    else if ( inst.GetRepr() != CSeq_inst::eRepr_not_set && 
              inst.IsSetLength() && inst.GetLength() != 0 ) {
        // split seq-data
        x_AddGap(inst.GetLength(), false);
    }
    else {
        if ( inst.GetRepr() != CSeq_inst::eRepr_not_set ) {
            NCBI_THROW(CSeqMapException, eDataError,
                       "CSeq_inst.repr of sequence without data "
                       "should be not_set");
        }
        if ( inst.IsSetLength() && inst.GetLength() != 0 ) {
            NCBI_THROW(CSeqMapException, eDataError,
                       "CSeq_inst.length of sequence without data "
                       "should be 0");
        }
        x_AddGap(0, false);
    }

    x_AddEnd();
}


CSeqMap::~CSeqMap(void)
{
    _ASSERT(!m_Bioseq);
    m_Resolved = 0;
    m_Segments.clear();
}


void CSeqMap::x_GetSegmentException(size_t /*index*/) const
{
    NCBI_THROW(CSeqMapException, eInvalidIndex,
               "Invalid segment index");
}


CSeqMap::CSegment& CSeqMap::x_SetSegment(size_t index)
{
    _ASSERT(index < m_Segments.size());
    return m_Segments[index];
}


CBioseq_Handle CSeqMap::x_GetBioseqHandle(const CSegment& seg,
                                          CScope* scope) const
{
    const CSeq_id& seq_id = x_GetRefSeqid(seg);
    if ( !scope ) {
        NCBI_THROW(CSeqMapException, eNullPointer,
                   "Cannot resolve "+
                   seq_id.AsFastaString()+": null scope pointer");
    }
    CBioseq_Handle bh = scope->GetBioseqHandle(seq_id);
    if ( !bh ) {
        NCBI_THROW(CSeqMapException, eFail,
                   "Cannot resolve "+
                   seq_id.AsFastaString()+": unknown");
    }
    return bh;
}


TSeqPos CSeqMap::x_ResolveSegmentLength(size_t index, CScope* scope) const
{
    const CSegment& seg = x_GetSegment(index);
    TSeqPos length = seg.m_Length;
    if ( length == kInvalidSeqPos ) {
        if ( seg.m_SegType == eSeqSubMap ) {
            length = x_GetSubSeqMap(seg, scope)->GetLength(scope);
        }
        else if ( seg.m_SegType == eSeqRef ) {
            length = x_GetBioseqHandle(seg, scope).GetBioseqLength();
        }
        if (length == kInvalidSeqPos) {
            NCBI_THROW(CSeqMapException, eDataError,
                    "Invalid sequence length");
        }
        seg.m_Length = length;
    }
    return length;
}


TSeqPos CSeqMap::x_ResolveSegmentPosition(size_t index, CScope* scope) const
{
    if ( index > x_GetSegmentsCount() ) {
        x_GetSegmentException(index);
    }
    size_t resolved = m_Resolved;
    if ( index <= resolved )
        return x_GetSegment(index).m_Position;
    TSeqPos resolved_pos = x_GetSegment(resolved).m_Position;
    do {
        TSeqPos seg_pos = resolved_pos;
        resolved_pos += x_GetSegmentLength(resolved, scope);
        if (resolved_pos < seg_pos  ||  resolved_pos == kInvalidSeqPos) {
            NCBI_THROW(CSeqMapException, eDataError,
                    "Sequence position overflow");
        }
        m_Segments[++resolved].m_Position = resolved_pos;
    } while ( resolved < index );
    {{
        CFastMutexGuard guard(m_SeqMap_Mtx);
        if ( m_Resolved < resolved )
            m_Resolved = resolved;
    }}
    return resolved_pos;
}


size_t CSeqMap::x_FindSegment(TSeqPos pos, CScope* scope) const
{
    size_t resolved = m_Resolved;
    TSeqPos resolved_pos = x_GetSegment(resolved).m_Position;
    if ( resolved_pos <= pos ) {
        do {
            if ( resolved >= x_GetSegmentsCount() ) {
                // end of segments
                m_Resolved = resolved;
                return size_t(-1);
            }
            TSeqPos seg_pos = resolved_pos;
            resolved_pos += x_GetSegmentLength(resolved, scope);
            if (resolved_pos < seg_pos  ||  resolved_pos == kInvalidSeqPos) {
                NCBI_THROW(CSeqMapException, eDataError,
                        "Sequence position overflow");
            }
            m_Segments[++resolved].m_Position = resolved_pos;
        } while ( resolved_pos <= pos );
        {{
            CFastMutexGuard guard(m_SeqMap_Mtx);
            if ( m_Resolved < resolved )
                m_Resolved = resolved;
        }}
        return resolved - 1;
    }
    else {
        TSegments::const_iterator end = m_Segments.begin()+resolved;
        TSegments::const_iterator it = 
            upper_bound(m_Segments.begin(), end,
                        pos, SPosLessSegment());
        if ( it == end ) {
            return size_t(-1);
        }
        return it - m_Segments.begin();
    }
}


void CSeqMap::x_LoadObject(const CSegment& seg) const
{
    _ASSERT(seg.m_Position != kInvalidSeqPos);
    if ( !seg.m_RefObject || seg.m_SegType != seg.m_ObjType ) {
        const CObject* obj = seg.m_RefObject.GetPointer();
        if ( obj && seg.m_ObjType == eSeqChunk ) {
            const CTSE_Chunk_Info* chunk =
                dynamic_cast<const CTSE_Chunk_Info*>(obj);
            if ( chunk ) {
                chunk->Load();
            }
        }
    }
}


const CObject* CSeqMap::x_GetObject(const CSegment& seg) const
{
    if ( !seg.m_RefObject || seg.m_SegType != seg.m_ObjType ) {
        x_LoadObject(seg);
    }
    if ( !seg.m_RefObject || seg.m_SegType != seg.m_ObjType ) {
        NCBI_THROW(CSeqMapException, eNullPointer, "null object pointer");
    }
    return seg.m_RefObject.GetPointer();
}


void CSeqMap::x_SetObject(const CSegment& seg, const CObject& obj)
{
    // lock for object modification
    CFastMutexGuard guard(m_SeqMap_Mtx);
    // check for object
    if ( seg.m_RefObject && seg.m_SegType == seg.m_ObjType ) {
        NCBI_THROW(CSeqMapException, eDataError, "object already set");
    }
    // set object
    const_cast<CSegment&>(seg).m_ObjType = seg.m_SegType;
    const_cast<CSegment&>(seg).m_RefObject.Reset(&obj);
}


void CSeqMap::x_SetChunk(const CSegment& seg, CTSE_Chunk_Info& chunk)
{
    // lock for object modification
    //CFastMutexGuard guard(m_SeqMap_Mtx);
    // check for object
    if ( seg.m_ObjType == eSeqChunk ||
         seg.m_RefObject && seg.m_SegType == seg.m_ObjType ) {
        NCBI_THROW(CSeqMapException, eDataError, "object already set");
    }
    // set object
    const_cast<CSegment&>(seg).m_RefObject.Reset(&chunk);
    const_cast<CSegment&>(seg).m_ObjType = eSeqChunk;
}


CConstRef<CSeqMap> CSeqMap::x_GetSubSeqMap(const CSegment& seg, CScope* scope,
                                           bool resolveExternal) const
{
    CConstRef<CSeqMap> ret;
    if ( seg.m_SegType == eSeqSubMap ) {
        ret.Reset(static_cast<const CSeqMap*>(x_GetObject(seg)));
    }
    else if ( resolveExternal && seg.m_SegType == eSeqRef ) {
        ret.Reset(&x_GetBioseqHandle(seg, scope).GetSeqMap());
    }
    return ret;
}


void CSeqMap::x_SetSubSeqMap(size_t /*index*/, CSeqMap_Delta_seqs* /*subMap*/)
{
    // not valid in generic seq map -> incompatible objects
    NCBI_THROW(CSeqMapException, eDataError, "Invalid parent map");
}


const CSeq_data& CSeqMap::x_GetSeq_data(const CSegment& seg) const
{
    if ( seg.m_SegType == eSeqData ) {
        return *static_cast<const CSeq_data*>(x_GetObject(seg));
    }
    NCBI_THROW(CSeqMapException, eSegmentTypeError,
               "Invalid segment type");
}


void CSeqMap::x_SetSeq_data(size_t index, CSeq_data& data)
{
    // check segment type
    CSegment& seg = x_SetSegment(index);
    if ( seg.m_SegType != eSeqData ) {
        NCBI_THROW(CSeqMapException, eSegmentTypeError,
                   "Invalid segment type");
    }
    x_SetObject(seg, data);
}


void CSeqMap::x_SetChanged(size_t index)
{
    while ( m_Resolved > index ) {
        x_SetSegment(m_Resolved--).m_Position = kInvalidSeqPos;
    }
    m_SeqLength = kInvalidSeqPos;
    m_HasSegments = 0;
    if ( !m_Changed ) {
        m_Changed = true;
        if ( m_Bioseq ) {
            m_Bioseq->x_SetChangedSeqMap();
        }
    }
}


void CSeqMap::x_SetSegmentGap(size_t index,
                              TSeqPos length)
{
    CFastMutexGuard guard(m_SeqMap_Mtx);
    CSegment& seg = x_SetSegment(index);
    if ( seg.m_SegType != eSeqGap ) {
        NCBI_THROW(CSeqMapException, eSegmentTypeError,
                   "Invalid segment type");
    }
    
    seg.m_SegType = seg.m_ObjType = eSeqGap;

    seg.m_Length = length;
    x_SetChanged(index);
}


void CSeqMap::x_SetSegmentData(size_t index,
                               TSeqPos length,
                               CSeq_data& data)
{
    CFastMutexGuard guard(m_SeqMap_Mtx);
    CSegment& seg = x_SetSegment(index);
    if ( seg.m_SegType != eSeqData ) {
        NCBI_THROW(CSeqMapException, eSegmentTypeError,
                   "Invalid segment type");
    }
    
    seg.m_SegType = seg.m_ObjType = eSeqData;
    seg.m_RefObject = &data;

    seg.m_Length = length;
    x_SetChanged(index);
}


void CSeqMap::x_SetSegmentRef(size_t index,
                              TSeqPos length,
                              const CSeq_id& ref_id,
                              TSeqPos ref_pos,
                              bool ref_minus_strand)
{
    CFastMutexGuard guard(m_SeqMap_Mtx);
    CSegment& seg = x_SetSegment(index);
    if ( seg.m_SegType != eSeqRef ) {
        NCBI_THROW(CSeqMapException, eSegmentTypeError,
                   "Invalid segment type");
    }
    
    seg.m_SegType = seg.m_ObjType = eSeqRef;
    CRef<CSeq_id> id(new CSeq_id);
    id->Assign(ref_id);
    seg.m_RefObject = id.GetPointer();
    seg.m_RefPosition = ref_pos;
    seg.m_RefMinusStrand = ref_minus_strand;

    seg.m_Length = length;
    x_SetChanged(index);
}


void CSeqMap::SetSegmentRef(const CSeqMap_CI& seg,
                            TSeqPos length,
                            const CSeq_id_Handle& ref_id,
                            TSeqPos ref_pos,
                            bool ref_minus_strand)
{
    x_SetSegmentRef(x_FindSegment(seg.GetPosition(), 0),
                    length, *ref_id.GetSeqId(), ref_pos, ref_minus_strand);
}


void CSeqMap::LoadSeq_data(TSeqPos pos, TSeqPos len,
                           const CSeq_data& data)
{
    size_t index = x_FindSegment(pos, 0);
    const CSegment& seg = x_GetSegment(index);
    if ( seg.m_Position != pos || seg.m_Length != len ) {
        NCBI_THROW(CSeqMapException, eDataError,
                   "Invalid segment size");
    }
    x_SetSeq_data(index, const_cast<CSeq_data&>(data));
}


const CSeq_id& CSeqMap::x_GetRefSeqid(const CSegment& seg) const
{
    if ( seg.m_SegType == eSeqRef ) {
        return static_cast<const CSeq_id&>(*x_GetObject(seg));
    }
    NCBI_THROW(CSeqMapException, eSegmentTypeError,
               "Invalid segment type");
}


TSeqPos CSeqMap::x_GetRefPosition(const CSegment& seg) const
{
    return seg.m_RefPosition;
}


bool CSeqMap::x_GetRefMinusStrand(const CSegment& seg) const
{
    return seg.m_RefMinusStrand;
}


CSeqMap_CI CSeqMap::Begin(CScope* scope) const
{
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, SSeqMapSelector());
}


CSeqMap_CI CSeqMap::End(CScope* scope) const
{
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, SSeqMapSelector(),
                      kMax_UInt);
}


CSeqMap_CI CSeqMap::FindSegment(TSeqPos pos, CScope* scope) const
{
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, SSeqMapSelector(), pos);
}


CSeqMap::const_iterator CSeqMap::begin(CScope* scope) const
{
    return Begin(scope);
}


CSeqMap::const_iterator CSeqMap::end(CScope* scope) const
{
    return End(scope);
}


CSeqMap_CI CSeqMap::BeginResolved(CScope*                scope,
                                  const SSeqMapSelector& sel) const
{
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, sel);
}


CSeqMap_CI CSeqMap::BeginResolved(CScope* scope) const
{
    SSeqMapSelector sel;
    sel.SetResolveCount(kMax_UInt);
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, sel);
}


CSeqMap_CI CSeqMap::EndResolved(CScope* scope) const
{
    SSeqMapSelector sel;
    sel.SetResolveCount(kMax_UInt);
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, sel, kMax_UInt);
}


CSeqMap_CI CSeqMap::EndResolved(CScope*                scope,
                                const SSeqMapSelector& sel) const
{
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, sel, kMax_UInt);
}


CSeqMap_CI CSeqMap::FindResolved(CScope*                scope,
                                 TSeqPos                pos,
                                 const SSeqMapSelector& selector) const
{
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, selector, pos);
}


CSeqMap_CI CSeqMap::ResolvedRangeIterator(CScope* scope,
                                          TSeqPos from,
                                          TSeqPos length,
                                          ENa_strand strand,
                                          size_t maxResolveCount,
                                          TFlags flags) const
{
    SSeqMapSelector sel;
    sel.SetFlags(flags).SetResolveCount(maxResolveCount);
    sel.SetRange(from, length).SetStrand(strand);
    return CSeqMap_CI(CConstRef<CSeqMap>(this), scope, sel);
}


bool CSeqMap::HasSegmentOfType(ESegmentType type) const
{
    if ( m_HasSegments == 0 ) {
        THasSegments flags = 0;
        ITERATE ( TSegments, it, m_Segments ) {
            flags |= 1<<it->m_SegType;
        }
        m_HasSegments = flags;
    }
    return bool((m_HasSegments >> type) & 1);
}


size_t CSeqMap::CountSegmentsOfType(ESegmentType type) const
{
    size_t count = 0;
    ITERATE ( TSegments, it, m_Segments ) {
        if ( it->m_SegType == type ) {
            ++count;
        }
    }
    return count;
}


bool CSeqMap::CanResolveRange(CScope* scope,
                              TSeqPos from,
                              TSeqPos length,
                              ENa_strand strand,
                              size_t depth,
                              TFlags flags) const
{
    SSeqMapSelector sel;
    sel.SetFlags(flags).SetResolveCount(depth);
    sel.SetRange(from, length).SetStrand(strand);
    return CanResolveRange(scope, sel);
}


bool CSeqMap::CanResolveRange(CScope* scope, const SSeqMapSelector& sel) const
{
    try {
        for(CSeqMap_CI seg(CConstRef<CSeqMap>(this), scope, sel); seg; ++seg) {
            // do nothing, just scan
        }
    }
    catch (exception) {
        return false;
    }
    return true;
}


CRef<CSeqMap> CSeqMap::CreateSeqMapForBioseq(const CBioseq& seq)
{
    return Ref(new CSeqMap(seq.GetInst()));
}


CRef<CSeqMap> CSeqMap::CloneFor(const CBioseq& seq) const
{
    return CreateSeqMapForBioseq(seq);
    /*
    CFastMutexGuard guard(m_SeqMap_Mtx);
    CRef<CSeqMap> ret;
    const CSeq_inst& inst = seq.GetInst();
    if ( inst.IsSetSeq_data() ) {
        ret.Reset(new CSeqMap_Seq_data(inst));
    }
    else if ( inst.IsSetExt() ) {
        const CSeq_ext& ext = inst.GetExt();
        switch (ext.Which()) {
        case CSeq_ext::e_Seg:
            ret.Reset(new CSeqMap_Seq_locs(ext.GetSeg(),
                                           ext.GetSeg().Get()));
            break;
        case CSeq_ext::e_Ref:
            ret.Reset(new CSeqMap(ext.GetRef()));
            break;
        case CSeq_ext::e_Delta:
            ret.Reset(new CSeqMap_Delta_seqs(ext.GetDelta()));
            break;
        case CSeq_ext::e_Map:
            //### Not implemented
            NCBI_THROW(CSeqMapException, eUnimplemented,
                       "CSeq_ext::e_Map -- not implemented");
        default:
            //### Not implemented
            NCBI_THROW(CSeqMapException, eUnimplemented,
                       "CSeq_ext::??? -- not implemented");
        }
    }
    else if ( inst.GetRepr() == CSeq_inst::eRepr_virtual ) {
        // Virtual sequence -- no data, no segments
        // The total sequence is gap
        ret.Reset(new CSeqMap(inst.GetLength()));
    }
    else if ( inst.GetRepr() != CSeq_inst::eRepr_not_set && 
              inst.IsSetLength() && inst.GetLength() != 0 ) {
        // split seq-data
        ret.Reset(new CSeqMap_Seq_data(inst));
    }
    else {
        if ( inst.GetRepr() != CSeq_inst::eRepr_not_set ) {
            NCBI_THROW(CSeqMapException, eDataError,
                       "CSeq_inst.repr of sequence without data "
                       "should be not_set");
        }
        if ( inst.IsSetLength() && inst.GetLength() != 0 ) {
            NCBI_THROW(CSeqMapException, eDataError,
                       "CSeq_inst.length of sequence without data "
                       "should be 0");
        }
        ret.Reset(new CSeqMap(TSeqPos(0)));
    }
    ret->m_Mol = inst.GetMol();
    if ( inst.IsSetLength() ) {
        ret->m_SeqLength = inst.GetLength();
    }
    return ret;
    */
}


CRef<CSeqMap> CSeqMap::CreateSeqMapForSeq_loc(const CSeq_loc& loc,
                                              CScope* scope)
{
    CRef<CSeqMap> ret(new CSeqMap(loc));
    if ( scope ) {
        CSeqMap_CI i(ret, scope, SSeqMapSelector(fFindData, kMax_UInt));
        for ( ; i; ++i ) {
            _ASSERT(i.GetType() == eSeqData);
            switch ( i.GetRefData().Which() ) {
            case CSeq_data::e_Ncbi2na:
            case CSeq_data::e_Ncbi4na:
            case CSeq_data::e_Ncbi8na:
            case CSeq_data::e_Ncbipna:
            case CSeq_data::e_Iupacna:
                ret->m_Mol = CSeq_inst::eMol_na;
                break;
            case CSeq_data::e_Ncbi8aa:
            case CSeq_data::e_Ncbieaa:
            case CSeq_data::e_Ncbipaa:
            case CSeq_data::e_Ncbistdaa:
            case CSeq_data::e_Iupacaa:
                ret->m_Mol = CSeq_inst::eMol_aa;
                break;
            default:
                ret->m_Mol = CSeq_inst::eMol_not_set;
            }
        }
    }
    return ret;
}


inline
void CSeqMap::x_AddSegment(ESegmentType type,
                           TSeqPos len,
                           bool unknown_len)
{
    m_Segments.push_back(CSegment(type, len, unknown_len));
}


void CSeqMap::x_AddSegment(ESegmentType type, TSeqPos len,
                           const CObject* object)
{
    x_AddSegment(type, len);
    CSegment& ret = m_Segments.back();
    ret.m_RefObject.Reset(object);
}


void CSeqMap::x_AddSegment(ESegmentType type,
                           const CObject* object,
                           TSeqPos refPos,
                           TSeqPos len,
                           ENa_strand strand)
{
    x_AddSegment(type, len, object);
    CSegment& ret = m_Segments.back();
    ret.m_RefPosition = refPos;
    ret.m_RefMinusStrand = IsReverse(strand);
}


void CSeqMap::x_AddEnd(void)
{
    TSeqPos pos = m_Segments.empty()? 0: kInvalidSeqPos;
    x_AddSegment(eSeqEnd, 0);
    CSegment& ret = m_Segments.back();
    ret.m_Position = pos;
}


void CSeqMap::x_AddGap(TSeqPos len, bool unknown_len)
{
    x_AddSegment(eSeqGap, len, unknown_len);
}


void CSeqMap::x_AddUnloadedSeq_data(TSeqPos len)
{
    x_AddSegment(eSeqData, len);
}


void CSeqMap::x_Add(const CSeq_data& data, TSeqPos len)
{
    x_AddSegment(eSeqData, len, &data);
}


void CSeqMap::x_Add(const CSeq_point& ref)
{
    x_AddSegment(eSeqRef, &ref.GetId(),
                 ref.GetPoint(), 1,
                 ref.IsSetStrand()? ref.GetStrand(): eNa_strand_unknown);
}


void CSeqMap::x_Add(const CSeq_interval& ref)
{
    x_AddSegment(eSeqRef, &ref.GetId(),
                 ref.GetFrom(), ref.GetLength(),
                 ref.IsSetStrand()? ref.GetStrand(): eNa_strand_unknown);
}


void CSeqMap::x_Add(const CSeq_id& ref)
{
    x_AddSegment(eSeqRef, &ref, 0, kInvalidSeqPos);
}

/*
CSeqMap::CSegment& CSeqMap::x_Add(CSeqMap* submap)
{
    return x_AddSegment(eSeqSubMap, kInvalidSeqPos, submap);
}
*/

void CSeqMap::x_Add(const CPacked_seqint& seq)
{
    ITERATE ( CPacked_seqint::Tdata, it, seq.Get() ) {
        x_Add(**it);
    }
    //return x_Add(new CSeqMap_Seq_intervals(seq));
}


void CSeqMap::x_Add(const CPacked_seqpnt& seq)
{
    const CSeq_id& id = seq.GetId();
    ENa_strand strand = seq.IsSetStrand()? seq.GetStrand(): eNa_strand_unknown;
    ITERATE ( CPacked_seqpnt::TPoints, it, seq.GetPoints() ) {
        x_AddSegment(eSeqRef, &id, *it, 1, strand);
    }
    //return x_Add(new CSeqMap_SeqPoss(seq));
}


void CSeqMap::x_Add(const CSeq_loc_mix& seq)
{
    ITERATE ( CSeq_loc_mix::Tdata, it, seq.Get() ) {
        x_Add(**it);
    }
    //return x_Add(new CSeqMap_Seq_locs(seq, seq.Get()));
}


void CSeqMap::x_Add(const CSeq_loc_equiv& seq)
{
    ITERATE ( CSeq_loc_equiv::Tdata, it, seq.Get() ) {
        x_Add(**it);
    }
    //return x_Add(new CSeqMap_Seq_locs(seq, seq.Get()));
}


void CSeqMap::x_Add(const CSeq_literal& seq)
{
    if ( seq.IsSetSeq_data() && !seq.GetSeq_data().IsGap() ) {
        x_Add(seq.GetSeq_data(), seq.GetLength());
    }
    else {
        // No data exist - treat it like a gap
        x_AddGap(seq.GetLength(), seq.CanGetFuzz()); //???
    }
}


void CSeqMap::x_Add(const CSeq_loc& loc)
{
    switch ( loc.Which() ) {
    case CSeq_loc::e_not_set:
    case CSeq_loc::e_Null:
    case CSeq_loc::e_Empty:
        x_AddGap(0, false); // Add gap ???
        break;
    case CSeq_loc::e_Whole:
        x_Add(loc.GetWhole());
        break;
    case CSeq_loc::e_Int:
        x_Add(loc.GetInt());
        break;
    case CSeq_loc::e_Pnt:
        x_Add(loc.GetPnt());
        break;
    case CSeq_loc::e_Packed_int:
        x_Add(loc.GetPacked_int());
        break;
    case CSeq_loc::e_Packed_pnt:
        x_Add(loc.GetPacked_pnt());
        break;
    case CSeq_loc::e_Mix:
        x_Add(loc.GetMix());
        break;
    case CSeq_loc::e_Equiv:
        x_Add(loc.GetEquiv());
        break;
    case CSeq_loc::e_Bond:
        NCBI_THROW(CSeqMapException, eDataError,
                   "e_Bond is not allowed as a reference type");
    case CSeq_loc::e_Feat:
        NCBI_THROW(CSeqMapException, eDataError,
                   "e_Feat is not allowed as a reference type");
    default:
        NCBI_THROW(CSeqMapException, eDataError,
                   "invalid reference type");
    }
}


void CSeqMap::x_Add(const CDelta_seq& seq)
{
    switch ( seq.Which() ) {
    case CDelta_seq::e_Loc:
        x_Add(seq.GetLoc());
        break;
    case CDelta_seq::e_Literal:
        x_Add(seq.GetLiteral());
        break;
    default:
        NCBI_THROW(CSeqMapException, eDataError,
                   "Can not add empty Delta-seq");
    }
}


void CSeqMap::SetRegionInChunk(CTSE_Chunk_Info& chunk,
                               TSeqPos pos, TSeqPos length)
{
    if ( length == kInvalidSeqPos ) {
        _ASSERT(pos == 0);
        _ASSERT(m_SeqLength != kInvalidSeqPos);
        length = m_SeqLength;
    }
    size_t index = x_FindSegment(pos, 0);
    CFastMutexGuard guard(m_SeqMap_Mtx);
    while ( length ) {
        // get segment
        if ( index > x_GetSegmentsCount() ) {
            x_GetSegmentException(index);
        }
        const CSegment& seg = x_GetSegment(index);

        // update segment position if not set yet
        if ( index > m_Resolved ) {
            _ASSERT(index == m_Resolved + 1);
            _ASSERT(seg.m_Position == kInvalidSeqPos || seg.m_Position == pos);
            seg.m_Position = pos;
            m_Resolved = index;
        }
        // check segment
        if ( seg.m_Position != pos || seg.m_Length > length ) {
            NCBI_THROW(CSeqMapException, eDataError,
                       "SeqMap segment crosses split chunk boundary");
        }
        if ( seg.m_SegType != eSeqGap ) {
            NCBI_THROW(CSeqMapException, eDataError,
                       "split chunk covers bad SeqMap segment");
        }
        _ASSERT(!seg.m_RefObject);

        // update segment
        const_cast<CSegment&>(seg).m_SegType = eSeqData;
        x_SetChunk(seg, chunk);

        // next
        pos += seg.m_Length;
        length -= seg.m_Length;
        ++index;
    }
}


bool CSeqMap::x_DoUpdateSeq_inst(CSeq_inst& inst)
{
    inst.SetLength(GetLength(0));
    if ( HasSegmentOfType(eSeqData) ) {
        if ( x_GetSegmentsCount() == 1 ) {
            // seq-data
            CSegment& seg = x_SetSegment(x_GetFirstEndSegmentIndex() + 1);
            _ASSERT(seg.m_SegType == eSeqData);
            inst.SetSeq_data(const_cast<CSeq_data&>(x_GetSeq_data(seg)));
            inst.ResetExt();
            return true;
        }
    }
    else if ( HasSegmentOfType(eSeqGap) ) {
        if ( x_GetSegmentsCount() == 1 ) {
            inst.SetRepr(CSeq_inst::eRepr_virtual);
            inst.ResetSeq_data();
            inst.ResetExt();
            return true;
        }
    }
    else {
        // ref only -> CSeg_ext
        CSeg_ext::Tdata& data = inst.SetExt().SetSeg().Set();
        CSeg_ext::Tdata::iterator iter = data.begin();
        for ( size_t index = x_GetFirstEndSegmentIndex() + 1;
              index < x_GetLastEndSegmentIndex(); ++index ) {
            CSegment& seg = x_SetSegment(index);
            _ASSERT(seg.m_SegType == eSeqRef);
            if ( iter == data.end() ) {
                iter = data.insert(iter, CSeg_ext::Tdata::value_type());
            }
            if ( !*iter ) {
                iter->Reset(new CSeq_loc);
            }
            CSeq_loc& loc = **iter;
            ++iter;
            CSeq_interval& interval = loc.SetInt();
            interval.SetId(const_cast<CSeq_id&>(x_GetRefSeqid(seg)));
            TSeqPos pos = seg.m_RefPosition;
            interval.SetFrom(pos);
            interval.SetTo(pos+x_GetSegmentLength(index, 0)-1);
            if ( seg.m_RefMinusStrand ) {
                interval.SetStrand(eNa_strand_minus);
            }
            else {
                interval.ResetStrand();
            }
            interval.ResetFuzz_from();
            interval.ResetFuzz_to();
        }
        data.erase(iter, data.end());
        return true;
    }

    // delta
    CDelta_ext::Tdata& delta = inst.SetExt().SetDelta().Set();
    CDelta_ext::Tdata::iterator iter = delta.begin();
    for ( size_t index = x_GetFirstEndSegmentIndex() + 1;
          index < x_GetLastEndSegmentIndex(); ++index ) {
        CSegment& seg = x_SetSegment(index);
        if ( iter == delta.end() ) {
            iter = delta.insert(iter, CDelta_ext::Tdata::value_type());
        }
        if ( !*iter ) {
            iter->Reset(new CDelta_seq);
        }
        CDelta_seq& dseq = **iter;
        ++iter;
        if ( seg.m_SegType == eSeqData ) {
            CSeq_literal& lit = dseq.SetLiteral();
            lit.SetLength(x_GetSegmentLength(index, 0));
            lit.SetSeq_data(const_cast<CSeq_data&>(x_GetSeq_data(seg)));
            lit.ResetFuzz();
        }
        else if ( seg.m_SegType == eSeqGap ) {
            CSeq_literal& lit = dseq.SetLiteral();
            lit.SetLength(x_GetSegmentLength(index, 0));
            lit.ResetSeq_data();
            lit.ResetFuzz();
        }
        else {
            _ASSERT(seg.m_SegType == eSeqRef);
            CSeq_loc& loc = dseq.SetLoc();
            CSeq_interval& interval = loc.SetInt();
            interval.SetId(const_cast<CSeq_id&>(x_GetRefSeqid(seg)));
            TSeqPos pos = seg.m_RefPosition;
            interval.SetFrom(pos);
            interval.SetTo(pos+x_GetSegmentLength(index, 0)-1);
            if ( seg.m_RefMinusStrand ) {
                interval.SetStrand(eNa_strand_minus);
            }
            else {
                interval.ResetStrand();
            }
            interval.ResetFuzz_from();
            interval.ResetFuzz_to();
        }
    }
    delta.erase(iter, delta.end());
    return true;
}


END_SCOPE(objects)
END_NCBI_SCOPE
