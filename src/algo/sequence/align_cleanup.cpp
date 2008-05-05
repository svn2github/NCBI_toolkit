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
 * Authors:  Mike DiCuccio
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>

#include <algo/sequence/align_cleanup.hpp>

#include <objtools/alnmgr/aln_container.hpp>
#include <objtools/alnmgr/aln_tests.hpp>
#include <objtools/alnmgr/aln_stats.hpp>
#include <objtools/alnmgr/pairwise_aln.hpp>
#include <objtools/alnmgr/aln_converters.hpp>
#include <objtools/alnmgr/aln_builders.hpp>
#include <objtools/alnmgr/aln_user_options.hpp>
#include <objtools/alnmgr/aln_generators.hpp>
#include <objtools/alnmgr/seqids_extractor.hpp>

#include <objtools/alnmgr/alnmix.hpp>

#include <serial/serial.hpp>
#include <serial/iterator.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);

/////////////////////////////////////////////////////////////////////////////

void CAlignCleanup::CreatePairwiseFromMultiple(const CSeq_align& multiple,
                                               TAligns&          pairwise)
{
    _ASSERT(multiple.GetSegs().IsDenseg());

    const CDense_seg& seg = multiple.GetSegs().GetDenseg();
    for (CDense_seg::TDim row = 1;  row < seg.GetDim();  ++row) {
        CRef<CDense_seg> new_seg(new CDense_seg);

        /// we are creating pairwise alignments
        new_seg->SetDim(2);

        /// get IDs right
        CRef<CSeq_id> id;
        id.Reset(new CSeq_id);
        id->Assign(*seg.GetIds()[0]);
        new_seg->SetIds().push_back(id);

        id.Reset(new CSeq_id);
        id->Assign(*seg.GetIds()[row]);
        new_seg->SetIds().push_back(id);

        /// copy the starts
        CDense_seg::TNumseg segs = 0;
        for (CDense_seg::TNumseg j = 0;  j < seg.GetNumseg();  ++j) {
            TSignedSeqPos start_0 = seg.GetStarts()[j * seg.GetDim() + 0];
            TSignedSeqPos start_1 = seg.GetStarts()[j * seg.GetDim() + row];

            if (start_0 < 0  &&  start_1 < 0) {
                /// segment is entirely gapped
                continue;
            }
            new_seg->SetLens().push_back(seg.GetLens()[j]);
            new_seg->SetStarts().push_back(start_0);
            new_seg->SetStarts().push_back(start_1);
            ++segs;
        }

        /// set the number of segments
        /// we will clean this up later
        new_seg->SetNumseg(segs);

        if (segs) {
            CRef<CSeq_align> align(new CSeq_align);

            /// make sure we set type correctly
            align->SetType(multiple.GetType());

            align->SetSegs().SetDenseg(*new_seg);
            pairwise.push_back(align);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////

CAlignCleanup::CAlignCleanup(CScope& scope)
    : m_Scope(&scope)
    , m_SortByScore(true)
    , m_AllowTransloc(true)
{
}


void CAlignCleanup::Cleanup(const TConstAligns& aligns_in,
                            TAligns&            aligns_out,
                            EMode               mode)
{
    switch (mode) {
    case eAlignVec:
        x_Cleanup_AlignVec(aligns_in, aligns_out);
        break;

    case eAnchoredAlign:
        x_Cleanup_AnchoredAln(aligns_in, aligns_out);
        break;
    }
}


void CAlignCleanup::x_Cleanup_AnchoredAln(const TConstAligns& aligns_in,
                                          TAligns&            aligns_out)
{
    CAlnContainer aln_container;

    ///
    /// step 1: add to alignment container
    ///
    size_t count = 0;
    size_t count_invalid = 0;
    ITERATE (TConstAligns, iter, aligns_in) {

        try {
            ++count;
            CConstRef<CSeq_align> aln = *iter;
            aln_container.insert(*aln);

            ///
            /// validation is optional!
            aln->Validate(true);
        }
        catch (CException& e) {
            LOG_POST(Error
                << "CAlgoPlugin_AlignCleanup::x_Run_AlignMgr(): "
                << "failed to validate: " << e.GetMsg());
            ++count_invalid;
        }
    }

    if (count_invalid) {
        string msg;
        msg += NStr::IntToString(count_invalid);
        msg += "/";
        msg += NStr::IntToString(count);
        msg += " alignments failed validation.";
        if (count_invalid == count) {
            NCBI_THROW(CException, eUnknown, msg);
        } else {
            LOG_POST(Warning << msg);
        }
    }

    /// Types we use here:
    typedef CSeq_align::TDim TDim;
    typedef CAlnSeqIdsExtract<CAlnSeqId> TIdExtract;
    typedef CAlnIdMap<vector< CConstRef<CSeq_align> >, TIdExtract> TAlnIdMap;
    typedef CAlnStats<TAlnIdMap> TAlnStats;


    /// Create a vector of seq-ids per seq-align
    TIdExtract id_extract;
    TAlnIdMap aln_id_map(id_extract, aln_container.size());
    size_t count_accepted = 0;
    ITERATE(CAlnContainer, aln_it, aln_container) {
        try {
            aln_id_map.push_back(**aln_it);
            ++count_accepted;
        }
        catch (CAlnException& e) {
            LOG_POST(Error
                << "CAlgoPlugin_AlignCleanup::x_Run_AlignMgr(): "
                << "failed to extract IDs: " << e.GetMsg());
        }
    }

    if (count_accepted != aln_container.size()) {
        if (count_accepted == 0) {
            NCBI_THROW(CException, eUnknown, 
                       "No valid alignments found");
            return;
        }

        LOG_POST(Warning
            << count_accepted << "/" << aln_container.size()
            << " alignments had no IDs to extract.");
    }

    ///
    /// gather statistics about our alignment
    ///
    TAlnStats aln_stats(aln_id_map);

    CAlnUserOptions opts;
    opts.m_MergeAlgo = CAlnUserOptions::eMergeAllSeqs;
    opts.m_Direction = CAlnUserOptions::eBothDirections;
    opts.SetMergeFlags(CAlnUserOptions::fTruncateOverlaps, true);

    ///
    /// create a set of anchored alignments
    ///
    typedef vector<CRef<CAnchoredAln> > TAnchoredAlnVec;
    TAnchoredAlnVec anchored_aln_vec;
    CreateAnchoredAlnVec(aln_stats, anchored_aln_vec, opts);

    ///
    /// now, build
    ///
    CAnchoredAln out_anchored_aln;
    BuildAln(anchored_aln_vec, out_anchored_aln, opts);

    ///
    /// create dense-segs and return
    ///
    CRef<CSeq_align> ds_align =
        CreateSeqAlignFromAnchoredAln(out_anchored_aln,
                                      CSeq_align::TSegs::e_Denseg);

    aligns_out.push_back(ds_align);
}


void CAlignCleanup::x_Cleanup_AlignVec(const TConstAligns& aligns_in,
                                       TAligns&            aligns_out)
{
    /// first, sort the alignments by the set of IDs they contain
    typedef set<CSeq_id_Handle> TIdSet;
    typedef map<TIdSet, list< CConstRef<CSeq_align> > > TAlignments;
    TAlignments align_map;
    ITERATE (TConstAligns, iter, aligns_in) {
        CConstRef<CSeq_align> al = *iter;

        TIdSet id_set;
        CTypeConstIterator<CSeq_id> id_iter(*al);
        for ( ;  id_iter;  ++id_iter) {
            CSeq_id_Handle idh = CSeq_id_Handle::GetHandle(*id_iter);
            id_set.insert(idh);
        }

        align_map[id_set].push_back(al);
    }

    CAlnMix::TMergeFlags merge_flags = CAlnMix::fTruncateOverlaps |
                                       CAlnMix::fRemoveLeadTrailGaps |
                                       CAlnMix::fGapJoin |
                                       CAlnMix::fMinGap;
    if (m_SortByScore) {
        merge_flags |= CAlnMix::fSortInputByScore;
    }
    if (m_AllowTransloc) {
        merge_flags |= CAlnMix::fAllowTranslocation;
    }

    /// next, merge each sublist independently, if needed
    string msg;
    ITERATE (TAlignments, iter, align_map) {
        typedef list< CConstRef<CSeq_align> > TAlignList;
        list<TAlignList> alignments;

        alignments.push_back(TAlignList());
        TAlignList& pos_strand = alignments.back();

        alignments.push_back(TAlignList());
        TAlignList& neg_strand = alignments.back();

        ITERATE (list< CConstRef<CSeq_align> >, it, iter->second) {
            const CSeq_align& align = **it;
            if (align.GetSegs().IsDenseg()  &&  align.GetSegs().GetDenseg().GetDim() == 2) {
                const CDense_seg& ds = align.GetSegs().GetDenseg();
                /// common case: dense-seg, particularly pairwise
                if (ds.IsSetStrands()  &&  ds.GetStrands()[0] != ds.GetStrands()[1]) {
                    neg_strand.push_back(*it);
                } else {
                    pos_strand.push_back(*it);
                }
            } else {
                /// mixed, so we bain - this is not a common case
                pos_strand.insert(pos_strand.end(),
                                  neg_strand.begin(), neg_strand.end());
                for ( ; it != iter->second.end();  ++it) {
                    pos_strand.push_back(*it);
                }
            }
        }

        /// now, we mix two sets of alignments,
        /// one negative strand, one positive strand
        size_t count = 0;
        ITERATE (list<TAlignList>, it, alignments) {
            ++count;
            if (it->empty()) {
                continue;
            }
            try {
                CAlnMix mix(*m_Scope);
                CAlnMix::TAddFlags flags = 0;
                if (iter->first.size() == 1) {
                    flags |= CAlnMix::fPreserveRows;
                }
                ITERATE (TAlignList, i, *it) {
                    mix.Add(**i, flags);
                }

                mix.Merge(merge_flags);

                if (mix.GetDenseg().GetStarts().size() == 0) {
                    NCBI_THROW(CException, eUnknown,
                               "Mix produced empty alignment");
                }

                if (mix.GetDenseg().GetLens().size() == 0) {
                    NCBI_THROW(CException, eUnknown,
                               "Mix produced empty alignment");
                }

                list< CRef<CSeq_align> > aligns;
                CRef<CSeq_align> new_align(new CSeq_align);
                new_align->Assign(mix.GetSeqAlign());

                if (new_align->GetSegs().IsDenseg()  &&
                    new_align->GetSegs().GetDenseg().GetDim() > 2) {
                    CreatePairwiseFromMultiple(*new_align, aligns);
                } else {
                    aligns.push_back(new_align);
                }

                NON_CONST_ITERATE (list< CRef<CSeq_align> >, align, aligns) {
                    if ((*align)->GetSegs().IsDenseg()) {
                        (*align)->SetSegs().SetDenseg().Compact();
                    }
                    aligns_out.push_back(*align);
                }
            }
            catch (CException& e) {
                LOG_POST(Error << "CAlgoPlugin_AlignCleanup::Run(): "
                    "error merging alignments: "
                    << e.GetMsg());
                if ( !msg.empty() ) {
                    msg += "\n";
                }
                msg += e.GetMsg();
            }
        }
    }
}


END_NCBI_SCOPE

