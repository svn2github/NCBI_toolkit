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
* Author: Aleksey Grichenko
*
* File Description:
*   Alignment mapper
*
*/

#include <ncbi_pch.hpp>
#include <objmgr/impl/seq_align_mapper.hpp>
#include <objmgr/seq_loc_mapper.hpp>
#include <objmgr/objmgr_exception.hpp>
#include <objects/seqalign/seqalign__.hpp>
#include <algorithm>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


CSeq_align_Mapper::CSeq_align_Mapper(const CSeq_align& align,
                                     bool map_widths,
                                     CScope* scope)
    : m_Scope(scope)
{
    m_MapWidths = map_widths  &&  scope;
    x_Init(align);
}


CSeq_align_Mapper::~CSeq_align_Mapper(void)
{
}


// Mapping through CSeq_loc_Conversion_Set

struct CConversionRef_Less
{
    bool operator()(const CRef<CSeq_loc_Conversion>& x,
                    const CRef<CSeq_loc_Conversion>& y) const;
};


bool CConversionRef_Less::operator()(const CRef<CSeq_loc_Conversion>& x,
                                     const CRef<CSeq_loc_Conversion>& y) const
{
    if (x->m_Src_id_Handle != y->m_Src_id_Handle) {
        return x->m_Src_id_Handle < y->m_Src_id_Handle;
    }
    // Leftmost first
    if (x->m_Src_from != y->m_Src_from) {
        return x->m_Src_from < y->m_Src_from;
    }
    // Longest first
    return x->m_Src_to > y->m_Src_to;
}


void CSeq_align_Mapper::Convert(CSeq_loc_Conversion_Set& cvts,
                                unsigned int loc_index_shift)
{
    m_DstAlign.Reset();

    if (m_SubAligns.size() > 0) {
        NON_CONST_ITERATE(TSubAligns, it, m_SubAligns) {
            dynamic_cast<CSeq_align_Mapper*>(it->GetPointer())->
                Convert(cvts, loc_index_shift);
            loc_index_shift += (*it)->GetDim();
        }
        return;
    }
    x_ConvertAlignCvt(cvts, loc_index_shift);
}


void CSeq_align_Mapper::x_ConvertAlignCvt(CSeq_loc_Conversion_Set& cvts,
                                          unsigned int loc_index_shift)
{
    if (cvts.m_CvtByIndex.size() == 0) {
        // Single mapping
        _ASSERT(cvts.m_SingleConv);
        x_ConvertRowCvt(*cvts.m_SingleConv,
            cvts.m_SingleIndex - loc_index_shift);
        return;
    }
    CSeq_loc_Conversion_Set::TConvByIndex::iterator idx_it =
        cvts.m_CvtByIndex.lower_bound(loc_index_shift);
    for ( ; idx_it != cvts.m_CvtByIndex.end()
        &&  idx_it->first < loc_index_shift + GetDim(); ++idx_it) {
        x_ConvertRowCvt(idx_it->second, idx_it->first - loc_index_shift);
    }
}


void CSeq_align_Mapper::x_ConvertRowCvt(CSeq_loc_Conversion& cvt,
                                        size_t row)
{
    CSeq_id_Handle dst_id;
    TSegments::iterator seg_it = m_Segs.begin();
    for ( ; seg_it != m_Segs.end(); ) {
        if (seg_it->m_Rows.size() <= row) {
            // No such row in the current segment
            ++seg_it;
            m_AlignFlags = eAlign_MultiDim;
            continue;
        }
        CSeq_id_Handle seg_id = x_ConvertSegmentCvt(seg_it, cvt, row);
        if (dst_id) {
            if (dst_id != seg_id  &&  m_AlignFlags == eAlign_Normal) {
                m_AlignFlags = eAlign_MultiId;
            }
            dst_id = seg_id;
        }
    }
}


void CSeq_align_Mapper::x_ConvertRowCvt(TIdMap& cvts,
                                        size_t row)
{
    CSeq_id_Handle dst_id;
    TSegments::iterator seg_it = m_Segs.begin();
    for ( ; seg_it != m_Segs.end(); ) {
        if (seg_it->m_Rows.size() <= row) {
            // No such row in the current segment
            ++seg_it;
            m_AlignFlags = eAlign_MultiDim;
            continue;
        }
        CSeq_id_Handle seg_id = x_ConvertSegmentCvt(seg_it, cvts, row);
        if (dst_id) {
            if (dst_id != seg_id  &&  m_AlignFlags == eAlign_Normal) {
                m_AlignFlags = eAlign_MultiId;
            }
            dst_id = seg_id;
        }
    }
}


CSeq_id_Handle
CSeq_align_Mapper::x_ConvertSegmentCvt(TSegments::iterator& seg_it,
                                       CSeq_loc_Conversion& cvt,
                                       size_t row)
{
    TSegments::iterator old_it = seg_it;
    ++seg_it;
    SAlignment_Segment::SAlignment_Row& aln_row = old_it->m_Rows[row];
    if (aln_row.m_Id != cvt.m_Src_id_Handle) {
        return aln_row.m_Id;
    }
    if (aln_row.m_Start == kInvalidSeqPos) {
        // ??? Skipped row - change ID
        aln_row.m_Id = cvt.m_Dst_id_Handle;
        aln_row.SetMapped();
        return aln_row.m_Id;
    }
    TRange rg(aln_row.m_Start, aln_row.m_Start + old_it->m_Len - 1);
    if (!cvt.ConvertInterval(rg.GetFrom(), rg.GetTo(), aln_row.m_Strand) ) {
        // Do not erase the segment, just change the row ID and reset start
        aln_row.m_Start = kInvalidSeqPos;
        aln_row.m_Id = cvt.m_Dst_id_Handle;
        aln_row.SetMapped();
        return aln_row.m_Id;
    }

    // At least part of the interval was converted.
    TSeqPos dl = cvt.m_Src_from <= rg.GetFrom() ?
        0 : cvt.m_Src_from - rg.GetFrom();
    TSeqPos dr = cvt.m_Src_to >= rg.GetTo() ?
        0 : rg.GetTo() - cvt.m_Src_to;
    if (dl > 0) {
        // Add segment for the skipped range
        SAlignment_Segment& lseg = x_InsertSeg(seg_it, dl,
            old_it->m_Rows.size());
        for (size_t r = 0; r < old_it->m_Rows.size(); ++r) {
            SAlignment_Segment::SAlignment_Row& lrow =
                lseg.CopyRow(r, old_it->m_Rows[r]);
            if (r == row) {
                lrow.m_Start = kInvalidSeqPos;
                lrow.m_Id = cvt.m_Dst_id_Handle;
            }
            else if (lrow.m_Start != kInvalidSeqPos &&
                !lrow.SameStrand(aln_row)) {
                // Adjust start for minus strand
                lrow.m_Start += old_it->m_Len - lseg.m_Len;
            }
        }
    }
    rg.SetFrom(rg.GetFrom() + dl);
    SAlignment_Segment& mseg = x_InsertSeg(seg_it,
        rg.GetLength() - dr,
        old_it->m_Rows.size());
    if (!dl  &&  !dr) {
        // copy scores if there's no truncation
        mseg.SetScores(old_it->m_Scores);
    }
    for (size_t r = 0; r < old_it->m_Rows.size(); ++r) {
        SAlignment_Segment::SAlignment_Row& mrow =
            mseg.CopyRow(r, old_it->m_Rows[r]);
        if (r == row) {
            // translate id and coords
            mrow.m_Id = cvt.m_Dst_id_Handle;
            mrow.m_Start = cvt.m_LastRange.GetFrom();
            mrow.m_IsSetStrand |= (cvt.m_LastStrand != eNa_strand_unknown);
            mrow.m_Strand = cvt.m_LastStrand;
            mrow.SetMapped();
            mseg.m_HaveStrands |= mrow.m_IsSetStrand;
        }
        else {
            if (mrow.m_Start != kInvalidSeqPos) {
                if (mrow.SameStrand(aln_row)) {
                    mrow.m_Start += dl;
                }
                else {
                    mrow.m_Start += old_it->m_Len - dl - mseg.m_Len;
                }
            }
        }
    }
    cvt.m_LastType = cvt.eMappedObjType_not_set;
    dl += rg.GetLength() - dr;
    rg.SetFrom(rg.GetTo() - dr);
    if (dr > 0) {
        // Add the remaining unmapped range
        SAlignment_Segment& rseg = x_InsertSeg(seg_it,
            dr,
            old_it->m_Rows.size());
        for (size_t r = 0; r < old_it->m_Rows.size(); ++r) {
            SAlignment_Segment::SAlignment_Row& rrow =
                rseg.CopyRow(r, old_it->m_Rows[r]);
            if (r == row) {
                rrow.m_Start = kInvalidSeqPos;
                rrow.m_Id = cvt.m_Dst_id_Handle;
            }
            else {
                if (rrow.SameStrand(aln_row)) {
                    rrow.m_Start += dl;
                }
            }
        }
    }
    m_Segs.erase(old_it);
    return cvt.m_Dst_id_Handle;
}


CSeq_id_Handle
CSeq_align_Mapper::x_ConvertSegmentCvt(TSegments::iterator& seg_it,
                                       TIdMap& id_map,
                                       size_t row)
{
    TSegments::iterator old_it = seg_it;
    SAlignment_Segment& seg = *old_it;
    ++seg_it;
    SAlignment_Segment::SAlignment_Row& aln_row = seg.m_Rows[row];
    if (aln_row.m_Start == kInvalidSeqPos) {
        // skipped row
        return aln_row.m_Id;
    }
    TRange rg(aln_row.m_Start, aln_row.m_Start + seg.m_Len - 1);
    TIdMap::iterator id_it = id_map.find(aln_row.m_Id);
    if (id_it == id_map.end()) {
        // ID not found in the segment, leave the row unchanged
        return aln_row.m_Id;
    }
    TRangeMap& rmap = id_it->second;
    if ( rmap.empty() ) {
        // No mappings for this segment
        return aln_row.m_Id;
    }
    // Sorted mappings
    typedef vector< CRef<CSeq_loc_Conversion> > TSortedConversions;
    TSortedConversions cvts;
    for (TRangeMap::iterator rg_it = rmap.begin(rg); rg_it; ++rg_it) {
        cvts.push_back(rg_it->second);
    }
    sort(cvts.begin(), cvts.end(), CConversionRef_Less());

    bool mapped = false;
    CSeq_id_Handle dst_id;
    EAlignFlags align_flags = eAlign_Normal;
    TSeqPos left_shift = 0;
    for (size_t cvt_idx = 0; cvt_idx < cvts.size(); ++cvt_idx) {
        CSeq_loc_Conversion& cvt = *cvts[cvt_idx];
        if (!cvt.ConvertInterval(rg.GetFrom(), rg.GetTo(), aln_row.m_Strand) ) {
            continue;
        }
        // Check destination id
        if ( dst_id ) {
            if (cvt.m_Dst_id_Handle != dst_id) {
                align_flags = eAlign_MultiId;
            }
        }
        dst_id = cvt.m_Dst_id_Handle;

        // At least part of the interval was converted.
        TSeqPos dl = cvt.m_Src_from <= rg.GetFrom() ?
            0 : cvt.m_Src_from - rg.GetFrom();
        TSeqPos dr = cvt.m_Src_to >= rg.GetTo() ?
            0 : rg.GetTo() - cvt.m_Src_to;
        if (dl > 0) {
            // Add segment for the skipped range
            SAlignment_Segment& lseg = x_InsertSeg(seg_it,
                dl, seg.m_Rows.size());
            for (size_t r = 0; r < seg.m_Rows.size(); ++r) {
                SAlignment_Segment::SAlignment_Row& lrow =
                    lseg.CopyRow(r, seg.m_Rows[r]);
                if (r == row) {
                    lrow.m_Start = kInvalidSeqPos;
                    lrow.m_Id = dst_id;
                }
                else if (lrow.m_Start != kInvalidSeqPos) {
                    if (lrow.SameStrand(aln_row)) {
                        lrow.m_Start += left_shift;
                    }
                    else {
                        lrow.m_Start += seg.m_Len - lseg.m_Len - left_shift;
                    }
                }
            }
        }
        left_shift += dl;
        SAlignment_Segment& mseg = x_InsertSeg(seg_it,
            rg.GetLength() - dl - dr, seg.m_Rows.size());
        if (!dl  &&  !dr) {
            // copy scores if there's no truncation
            mseg.SetScores(seg.m_Scores);
        }
        for (size_t r = 0; r < seg.m_Rows.size(); ++r) {
            SAlignment_Segment::SAlignment_Row& mrow =
                mseg.CopyRow(r, seg.m_Rows[r]);
            if (r == row) {
                // translate id and coords
                mrow.m_Id = cvt.m_Dst_id_Handle;
                mrow.m_Start = cvt.m_LastRange.GetFrom();
                mrow.m_IsSetStrand |= (cvt.m_LastStrand != eNa_strand_unknown);
                mrow.m_Strand = cvt.m_LastStrand;
                mrow.SetMapped();
                mseg.m_HaveStrands |= mrow.m_IsSetStrand;
            }
            else {
                if (mrow.m_Start != kInvalidSeqPos) {
                    if (mrow.SameStrand(aln_row)) {
                        mrow.m_Start += left_shift;
                    }
                    else {
                        mrow.m_Start += seg.m_Len - left_shift - mseg.m_Len;
                    }
                }
            }
        }
        cvt.m_LastType = cvt.eMappedObjType_not_set;
        mapped = true;
        left_shift += mseg.m_Len;
        rg.SetFrom(aln_row.m_Start + left_shift);
    }
    if (align_flags == eAlign_MultiId  &&  m_AlignFlags == eAlign_Normal) {
        m_AlignFlags = align_flags;
    }
    if ( !mapped ) {
        // Do not erase the segment, just change the row ID and reset start
        seg.m_Rows[row].m_Start = kInvalidSeqPos;
        seg.m_Rows[row].m_Id = rmap.begin()->second->m_Dst_id_Handle;
        seg.m_Rows[row].SetMapped();
        return seg.m_Rows[row].m_Id;
    }
    if (rg.GetFrom() <= rg.GetTo()) {
        // Add the remaining unmapped range
        SAlignment_Segment& rseg = x_InsertSeg(seg_it,
            rg.GetLength(), seg.m_Rows.size());
        for (size_t r = 0; r < seg.m_Rows.size(); ++r) {
            SAlignment_Segment::SAlignment_Row& rrow =
                rseg.CopyRow(r, seg.m_Rows[r]);
            if (r == row) {
                rrow.m_Start = kInvalidSeqPos;
                rrow.m_Id = dst_id;
            }
            else if (rrow.m_Start != kInvalidSeqPos) {
                if (rrow.SameStrand(aln_row)) {
                    rrow.m_Start += left_shift;
                }
            }
        }
    }
    m_Segs.erase(old_it);
    return align_flags == eAlign_MultiId ? CSeq_id_Handle() : dst_id;
}


int CSeq_align_Mapper::GetSeqWidth(const CSeq_id& id) const
{
    if ( !m_Scope ) {
        return 0;
    }
    CBioseq_Handle bh = m_Scope->GetBioseqHandle(id);
    switch ( bh.GetBioseqMolType() ) {
    case CSeq_inst::eMol_aa:
        return 3;
    case CSeq_inst::eMol_na:
    case CSeq_inst::eMol_dna:
    case CSeq_inst::eMol_rna:
        return 1;
    default:
        return 0;
    }
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.23  2006/12/11 17:14:12  grichenk
* Added CSeq_loc_Mapper_Base and CSeq_align_Mapper_Base.
*
* Revision 1.22  2006/11/29 20:44:04  grichenk
* Set correct strand for gaps.
*
* Revision 1.21  2006/09/27 21:27:59  vasilche
* Added accessors.
*
* Revision 1.20  2006/08/21 15:46:42  grichenk
* Added CMappingRanges for storing mappings.
* Added CSeq_loc_Mapper(CMappingRanges&) constructor.
*
* Revision 1.19  2006/05/11 19:03:12  grichenk
* Fixed mapping of gaps in alignments
*
* Revision 1.18  2006/05/04 21:07:24  grichenk
* Fixed mapping of std-segs.
*
* Revision 1.17  2006/04/20 19:11:10  grichenk
* Fixed strand check for alignments
*
* Revision 1.16  2005/11/07 15:40:19  vasilche
* Fixed warning in switch (enum).
*
* Revision 1.15  2005/09/26 21:38:42  grichenk
* Adjust segment length when mapping between nucleotide and protein
* (den-diag, packed-seg).
*
* Revision 1.14  2005/09/26 16:33:24  ucko
* Pull CSeq_align_Mapper's destructor out of line, as its (implicit)
* destruction of m_Scope may require a full declaration thereof.
*
* Revision 1.13  2005/09/22 20:49:33  grichenk
* Adjust segment length when mapping alignment between nuc and prot.
*
* Revision 1.12  2004/12/22 15:56:32  vasilche
* Removed redundant include.
*
* Revision 1.11  2004/07/12 15:05:32  grichenk
* Moved seq-id mapper from xobjmgr to seq library
*
* Revision 1.10  2004/05/27 17:52:11  grichenk
* Fixed warnings
*
* Revision 1.9  2004/05/26 14:57:47  ucko
* Change some types from unsigned int to size_t for consistency with STL
* containers' size() methods.
*
* Revision 1.8  2004/05/26 14:29:20  grichenk
* Redesigned CSeq_align_Mapper: preserve non-mapping intervals,
* fixed strands handling, improved performance.
*
* Revision 1.7  2004/05/21 21:42:12  gorelenk
* Added PCH ncbi_pch.hpp
*
* Revision 1.6  2004/05/13 19:10:57  grichenk
* Preserve scores in mapped alignments
*
* Revision 1.5  2004/05/10 20:22:24  grichenk
* Fixed more warnings (removed inlines)
*
* Revision 1.4  2004/04/12 14:35:59  grichenk
* Fixed mapping of alignments between nucleotides and proteins
*
* Revision 1.3  2004/04/07 18:36:12  grichenk
* Fixed std-seg mapping
*
* Revision 1.2  2004/04/01 20:11:17  rsmith
* add missing break to switch on seq-id types.
*
* Revision 1.1  2004/03/30 15:40:35  grichenk
* Initial revision
*
*
* ===========================================================================
*/
