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
* Author:  Eugene Vasilchenko
*
* File Description:
*   Application for splitting blobs withing ID1 cache
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <objmgr/split/blob_splitter_impl.hpp>

#include <serial/objostr.hpp>
#include <serial/serial.hpp>
#include <serial/iterator.hpp>

#include <objects/general/Object_id.hpp>

#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>

#include <objects/seqset/Seq_entry.hpp>
#include <objects/seqset/Bioseq_set.hpp>

#include <objects/seq/Seq_descr.hpp>
#include <objects/seq/Seqdesc.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Seq_literal.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Annot_id.hpp>
#include <objects/seq/Annot_descr.hpp>
#include <objects/seq/Annotdesc.hpp>

#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqres/Seq_graph.hpp>

#include <objects/seqsplit/ID2S_Chunk_Id.hpp>
#include <objects/seqsplit/ID2S_Chunk_Id.hpp>
#include <objects/seqsplit/ID2S_Chunk.hpp>

#include <objmgr/split/blob_splitter.hpp>
#include <objmgr/split/object_splitinfo.hpp>
#include <objmgr/split/annot_piece.hpp>
#include <objmgr/split/asn_sizer.hpp>
#include <objmgr/split/chunk_info.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

template<class C>
inline
C& NonConst(const C& c)
{
    return const_cast<C&>(c);
}


/////////////////////////////////////////////////////////////////////////////
// CBlobSplitterImpl
/////////////////////////////////////////////////////////////////////////////


static CAsnSizer s_Sizer;

static CSize small_annot;

static size_t landmark_annot_count;
static size_t complex_annot_count;
static size_t simple_annot_count;

void CBlobSplitterImpl::CopySkeleton(CSeq_entry& dst, const CSeq_entry& src)
{
    small_annot.clear();
    landmark_annot_count = complex_annot_count = simple_annot_count = 0;

    if ( src.IsSeq() ) {
        CopySkeleton(dst.SetSeq(), src.GetSeq());
    }
    else {
        CopySkeleton(dst.SetSet(), src.GetSet());
    }

    if ( m_Params.m_Verbose ) {
        // annot statistics
        if ( small_annot ) {
            NcbiCout << "Small Seq-annots: " << small_annot << NcbiEndl;
        }
    }
#if 0
    NcbiCout << "Total: after: " <<
        (landmark_annot_count + complex_annot_count + simple_annot_count) <<
        NcbiEndl;
#endif

    if ( m_Params.m_Verbose && m_Skeleton == &dst ) {
        // skeleton statistics
        s_Sizer.Set(*m_Skeleton, m_Params);
        CSize size(s_Sizer);
        NcbiCout <<
            "\nSkeleton: " << size << NcbiEndl;
    }
}


void CBlobSplitterImpl::CopySkeleton(CBioseq& dst, const CBioseq& src)
{
    dst.Reset();
    int gi = 0;
    ITERATE ( CBioseq::TId, it, src.GetId() ) {
        const CSeq_id& id = **it;
        if ( id.IsGi() ) {
            gi = id.GetGi();
        }
        dst.SetId().push_back(Ref(&NonConst(id)));
    }

    bool need_split_descr;
    if ( m_Params.m_DisableSplitDescriptions ) {
        need_split_descr = false;
    }
    else {
        need_split_descr = src.IsSetDescr() && !src.GetDescr().Get().empty();
    }

    bool need_split_inst;
    if ( m_Params.m_DisableSplitSequence ) {
        need_split_inst = false;
    }
    else {
        need_split_inst = false;
        const CSeq_inst& inst = src.GetInst();
        if ( inst.IsSetSeq_data() && inst.IsSetExt() ) {
            // both data and ext
            need_split_inst = false;
        }
        else {
            if ( inst.IsSetSeq_data() ) {
                need_split_inst = true;
            }
            if ( inst.IsSetExt() ) {
                const CSeq_ext& ext = inst.GetExt();
                if ( ext.Which() == CSeq_ext::e_Delta ) {
                    need_split_inst = true;
                }
            }
        }
    }

    bool need_split_annot;
    if ( m_Params.m_DisableSplitAnnotations ) {
        need_split_annot = false;
    }
    else {
        need_split_annot = !src.GetAnnot().empty();
    }

    bool need_split = need_split_descr || need_split_inst || need_split_annot;
    CBioseq_SplitInfo* info = 0;

    if ( need_split ) {
        if ( gi <= 0 ) {
            ERR_POST("Bioseq doesn't have gi");
        }
        else {
            info = &m_Bioseqs[gi];
            
            if ( info->m_Id != 0 ) {
                ERR_POST("Several Bioseqs with the same gi: " << gi);
                info = 0;
            }
            else {
                _ASSERT(info->m_Id == 0);
                _ASSERT(!info->m_Bioseq);
                _ASSERT(!info->m_Bioseq_set);
                info->m_Id = gi;
                info->m_Bioseq.Reset(&dst);
            }
        }
        
        if ( !info ) {
            need_split_descr = need_split_inst = need_split_annot = false;
        }
    }
    
    if ( need_split_descr ) {
        ITERATE ( CBioseq::TDescr::Tdata, it, src.GetDescr().Get() ) {
            if ( !CopyDesc(*info, **it) ) {
                dst.SetDescr().Set().push_back(*it);
            }
        }
    }
    else {
        if ( src.IsSetDescr() ) {
            dst.SetDescr().Set() = src.GetDescr().Get();
        }
    }

    if ( need_split_inst ) {
        CopySequence(*info, gi, dst.SetInst(), src.GetInst());
    }
    else {
        dst.SetInst(NonConst(src.GetInst()));
    }

    if ( need_split_annot ) {
        ITERATE ( CBioseq::TAnnot, it, src.GetAnnot() ) {
            if ( !CopyAnnot(*info, **it) ) {
                dst.SetAnnot().push_back(*it);
            }
        }
    }
}


void CBlobSplitterImpl::CopySkeleton(CBioseq_set& dst, const CBioseq_set& src)
{
    dst.Reset();
    int id = 0;
    if ( src.IsSetId() ) {
        dst.SetId(NonConst(dst.GetId()));
        if ( src.GetId().IsId() ) {
            id = src.GetId().GetId();
        }
    }
    else {
        id = m_NextBioseq_set_Id++;
        dst.SetId().SetId(id);
    }
    if ( src.IsSetColl() ) {
        dst.SetColl(NonConst(src.GetColl()));
    }
    if ( src.IsSetLevel() ) {
        dst.SetLevel(src.GetLevel());
    }
    if ( src.IsSetClass() ) {
        dst.SetClass(src.GetClass());
    }
    if ( src.IsSetRelease() ) {
        dst.SetRelease(src.GetRelease());
    }
    if ( src.IsSetDate() ) {
        dst.SetDate(NonConst(src.GetDate()));
    }

    bool need_split_descr;
    if ( m_Params.m_DisableSplitDescriptions ) {
        need_split_descr = false;
    }
    else {
        need_split_descr = src.IsSetDescr() && !src.GetDescr().Get().empty();
    }

    bool need_split_annot;
    if ( m_Params.m_DisableSplitAnnotations ) {
        need_split_annot = false;
    }
    else {
        need_split_annot = !src.GetAnnot().empty();
    }

    bool need_split = need_split_descr || need_split_annot;
    CBioseq_SplitInfo* info = 0;

    if ( need_split ) {
        if ( id <= 0 ) {
            ERR_POST("Bioseq_set doesn't have integer id");
        }
        else {
            info = &m_Bioseqs[-id];
            if ( info->m_Id != 0 ) {
                ERR_POST("Several Bioseqs with the same gi: " << id);
                info = 0;
            }
            else {
                _ASSERT(info->m_Id == 0);
                _ASSERT(!info->m_Bioseq);
                _ASSERT(!info->m_Bioseq_set);
                info->m_Id = -id;
                info->m_Bioseq_set.Reset(&dst);
            }
        }

        if ( !info ) {
            need_split_descr = need_split_annot = 0;
        }
    }


    if ( need_split_descr ) {
        ITERATE ( CBioseq_set::TDescr::Tdata, it, src.GetDescr().Get() ) {
            if ( !CopyDesc(*info, **it) ) {
                dst.SetDescr().Set().push_back(*it);
            }
        }
    }
    else {
        if ( src.IsSetDescr() ) {
            dst.SetDescr().Set() = src.GetDescr().Get();
        }
    }

    if ( need_split_annot ) {
        ITERATE ( CBioseq_set::TAnnot, it, src.GetAnnot() ) {
            if ( !CopyAnnot(*info, **it) ) {
                dst.SetAnnot().push_back(*it);
            }
        }
    }

    ITERATE ( CBioseq_set::TSeq_set, it, src.GetSeq_set() ) {
        dst.SetSeq_set().push_back(Ref(new CSeq_entry));
        CopySkeleton(*dst.SetSeq_set().back(), **it);
    }
}


bool CBlobSplitterImpl::CopyDesc(CBioseq_SplitInfo& /*bioseq_info*/,
                                 const CSeqdesc& /*desc*/)
{
    return false;
}


TSeqPos GetLength(const CSeq_loc& loc)
{
    _ASSERT(loc.Which() == CSeq_loc::e_Int);
    return loc.GetInt().GetLength();
}


bool CBlobSplitterImpl::CopySequence(CBioseq_SplitInfo& bioseq_info,
                                     int gi,
                                     CSeq_inst& dst,
                                     const CSeq_inst& src)
{
    _ASSERT(!bioseq_info.m_Seq_inst);
    bioseq_info.m_Seq_inst.Reset(new CSeq_inst_SplitInfo);
    CSeq_inst_SplitInfo& info = *bioseq_info.m_Seq_inst;
    info.m_Seq_inst.Reset(&src);

    dst.SetRepr(src.GetRepr());
    dst.SetMol(src.GetMol());
    if ( src.IsSetLength() )
        dst.SetLength(src.GetLength());
    if ( src.IsSetFuzz() )
        dst.SetFuzz(const_cast<CInt_fuzz&>(src.GetFuzz()));
    if ( src.IsSetTopology() )
        dst.SetTopology(src.GetTopology());
    if ( src.IsSetStrand() )
        dst.SetStrand(src.GetStrand());
    if ( src.IsSetHist() )
        dst.SetHist(const_cast<CSeq_hist&>(src.GetHist()));

    if ( src.IsSetSeq_data() ) {
        CSeq_data_SplitInfo data;
        CRange<TSeqPos> range;
        range.SetFrom(0).SetLength(src.GetLength());
        data.SetSeq_data(gi, range, src.GetSeq_data(), m_Params);
        info.Add(data);
    }
    else {
        _ASSERT(src.IsSetExt());
        const CSeq_ext& src_ext = src.GetExt();
        _ASSERT(src_ext.Which() == CSeq_ext::e_Delta);
        const CDelta_ext& src_delta = src_ext.GetDelta();
        CDelta_ext& dst_delta = dst.SetExt().SetDelta();
        TSeqPos pos = 0;
        ITERATE ( CDelta_ext::Tdata, it, src_delta.Get() ) {
            const CDelta_seq& src_seq = **it;
            TSeqPos length;
            CRef<CDelta_seq> new_seq;
            switch ( src_seq.Which() ) {
            case CDelta_seq::e_Loc:
                new_seq = *it;
                length = GetLength(src_seq.GetLoc());
                break;
            case CDelta_seq::e_Literal:
            {{
                const CSeq_literal& src_lit = src_seq.GetLiteral();
                new_seq.Reset(new CDelta_seq);
                CSeq_literal& dst_lit = new_seq->SetLiteral();
                length = src_lit.GetLength();
                dst_lit.SetLength(length);
                if ( src_lit.IsSetFuzz() )
                    dst_lit.SetFuzz(const_cast<CInt_fuzz&>(src_lit.GetFuzz()));
                if ( src_lit.IsSetSeq_data() ) {
                    CSeq_data_SplitInfo data;
                    CRange<TSeqPos> range;
                    range.SetFrom(pos).SetLength(length);
                    data.SetSeq_data(gi, range,
                                     src_lit.GetSeq_data(), m_Params);
                    info.Add(data);
                }
                break;
            }}
            default:
                new_seq.Reset(new CDelta_seq);
                length = 0;
                break;
            }
            dst_delta.Set().push_back(new_seq);
            pos += length;
        }
    }
    return false;
}


bool CBlobSplitterImpl::CopyAnnot(CBioseq_SplitInfo& bioseq_info,
                                  const CSeq_annot& annot)
{
    if ( m_Params.m_DisableSplitAnnotations ) {
        return false;
    }

    switch ( annot.GetData().Which() ) {
    case CSeq_annot::TData::e_Ftable:
    case CSeq_annot::TData::e_Align:
    case CSeq_annot::TData::e_Graph:
        break;
    default:
        // we don't split other types of Seq-annot
        return false;
    }

    CSeq_annot_SplitInfo& info = bioseq_info.m_Seq_annots[ConstRef(&annot)];
    info.SetSeq_annot(bioseq_info.m_Id, annot, m_Params);

    landmark_annot_count += info.m_LandmarkObjects.size();
    complex_annot_count += info.m_ComplexLocObjects.size();
    ITERATE ( TSimpleLocObjects, it, info.m_SimpleLocObjects ) {
        simple_annot_count += it->second.size();
    }

    if ( info.m_Size.GetAsnSize() > 1024 ) {
        if ( m_Params.m_Verbose ) {
            NcbiCout << info;
        }
    }
    else {
        small_annot += info.m_Size;
    }

    return true;
}


size_t CBlobSplitterImpl::CountAnnotObjects(const CSeq_entry& entry)
{
    size_t count = 0;
    for ( CTypeConstIterator<CSeq_annot> it(ConstBegin(entry)); it; ++it ) {
        count += CSeq_annot_SplitInfo::CountAnnotObjects(*it);
    }
    return count;
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.9  2004/06/15 14:05:50  vasilche
* Added splitting of sequence.
*
* Revision 1.8  2004/05/21 21:42:14  gorelenk
* Added PCH ncbi_pch.hpp
*
* Revision 1.7  2004/03/24 18:29:19  vasilche
* Fixed performance problem in verbose output.
*
* Revision 1.6  2004/03/05 17:40:34  vasilche
* Added 'verbose' option to splitter parameters.
*
* Revision 1.5  2004/01/22 20:10:42  vasilche
* 1. Splitted ID2 specs to two parts.
* ID2 now specifies only protocol.
* Specification of ID2 split data is moved to seqsplit ASN module.
* For now they are still reside in one resulting library as before - libid2.
* As the result split specific headers are now in objects/seqsplit.
* 2. Moved ID2 and ID1 specific code out of object manager.
* Protocol is processed by corresponding readers.
* ID2 split parsing is processed by ncbi_xreader library - used by all readers.
* 3. Updated OBJMGR_LIBS correspondingly.
*
* Revision 1.4  2004/01/07 17:36:25  vasilche
* Moved id2_split headers to include/objmgr/split.
* Fixed include path to genbank.
*
* Revision 1.3  2003/12/17 15:19:37  vasilche
* Added missing return if Seq-annot cannot be split.
*
* Revision 1.2  2003/11/26 23:04:58  vasilche
* Removed extra semicolons after BEGIN_SCOPE and END_SCOPE.
*
* Revision 1.1  2003/11/12 16:18:29  vasilche
* First implementation of ID2 blob splitter withing cache.
*
* ===========================================================================
*/
