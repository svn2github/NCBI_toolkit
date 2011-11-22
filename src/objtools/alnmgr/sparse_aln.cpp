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
 * Authors:  Andrey Yazhuk
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>

#include <objtools/alnmgr/sparse_aln.hpp>
#include <objtools/alnmgr/sparse_ci.hpp>
#include <objtools/alnmgr/alnexception.hpp>
#include <objtools/error_codes.hpp>

#include <objects/seqalign/Sparse_align.hpp>
#include <objects/seqfeat/Genetic_code_table.hpp>


#define NCBI_USE_ERRCODE_X   Objtools_Aln_Sparse


BEGIN_NCBI_SCOPE
USING_SCOPE(ncbi::objects);


CSparseAln::CSparseAln(const CAnchoredAln& anchored_aln,
                       objects::CScope& scope)
    : m_Scope(&scope),
      m_GapChar('-')
{
    x_Build(anchored_aln);
}

    
CSparseAln::~CSparseAln()
{
}


CSparseAln::TDim CSparseAln::GetDim() const {
    return m_Aln->GetDim();
}


struct SGapRange
{
    TSignedSeqPos from; // original position of the gap
    TSignedSeqPos len;  // length of the gap
    int           row;  // Row, containing the gap.
    size_t        idx;  // Index of the gap in the original vector.
    // This gap's 'from' must be shifted by 'shift'.
    // Segments at or after 'from' position must be offset by 'shift + len'.
    TSignedSeqPos shift;

    bool operator<(const SGapRange& rg) const
    {
        if (from != rg.from) return from < rg.from; // sort by pos
        return row < rg.row; // lower rows go first.
        // Don't check the length. Stable sort will preserve the order
        // of consecutive gaps on the same row.
    }
};


typedef vector<SGapRange> TGapRanges;


void CSparseAln::x_Build(const CAnchoredAln& src_align)
{
    const TDim& dim = src_align.GetDim();

    m_BioseqHandles.clear();
    m_BioseqHandles.resize(dim);

    m_SeqVectors.clear();
    m_SeqVectors.resize(dim);

    // Collect all gaps on all rows. They need to be inserted into the
    // new anchor as normal segments.
    TGapRanges gaps;
    for (TDim row = 0; row < dim; ++row) {
        const CPairwiseAln& pw = *src_align.GetPairwiseAlns()[row];
        const CPairwiseAln::TAlignRangeVector& ins_vec = pw.GetInsertions();
        gaps.reserve(gaps.size() + ins_vec.size());
        for (size_t i = 0; i < ins_vec.size(); i++) {
            SGapRange gap;
            gap.from = ins_vec[i].GetFirstFrom();
            gap.len = ins_vec[i].GetLength();
            gap.row = row;
            gap.shift = 0;
            gap.idx = i;
            gaps.push_back(gap);
        }
    }
    // We need to preserve the order of consecutive gaps at the same
    // position on the same row. Use stable_sort.
    stable_sort(gaps.begin(), gaps.end());
    // Set shift for all gaps.
    TSignedSeqPos shift = 0;
    NON_CONST_ITERATE(TGapRanges, gap_it, gaps) {
        gap_it->shift = shift;
        shift += gap_it->len;
    }

    m_Aln.Reset(new CAnchoredAln());
    m_Aln->SetDim(dim);
    m_Aln->SetAnchorRow(src_align.GetAnchorRow());
    m_Aln->SetScore(src_align.GetScore());

    // Now for each row convert insertions to normal segments and change
    // the old anchor coordinates to alignment coordinates.
    for (TDim row = 0; row < dim; ++row) {
        const CPairwiseAln& pw = *src_align.GetPairwiseAlns()[row];
        const CPairwiseAln::TAlignRangeVector& ins_vec = pw.GetInsertions();
        CPairwiseAln::const_iterator seg_it = pw.begin();
        CPairwiseAln::TAlignRangeVector::const_iterator ins_it = ins_vec.begin();
        TGapRanges::const_iterator gap = gaps.begin();
        TGapRanges::const_iterator next_gap = gap;
        if (next_gap != gaps.end()) {
            ++next_gap;
        }
        TSignedSeqPos shift = 0;
        CRef<CPairwiseAln> dst(new CPairwiseAln(
            pw.GetFirstId(), pw.GetSecondId(), pw.GetFlags()));
        while (seg_it != pw.end()  ||  ins_it != ins_vec.end()) {
            // Have more insertions in the source?
            // Is the next insertion at or before the next segment
            // (or there are no more segments)?
            CPairwiseAln::TAlignRange rg;
            if (ins_it != ins_vec.end()  &&
                (seg_it == pw.end()  ||
                ins_it->GetFirstFrom() <= seg_it->GetFirstFrom())) {
                rg = *ins_it;
                size_t idx = ins_it - ins_vec.begin();
                ++ins_it;
                // Skip all gaps with lower positions and/or on lower rows.
                while (next_gap != gaps.end()  &&
                    (gap->row != row  ||  gap->idx != idx)) {
                    ++next_gap;
                    ++gap;
                }
                // The insertion must be in gaps.
                _ASSERT(gap->from == rg.GetFirstFrom());
                _ASSERT(gap->len == rg.GetLength());
                _ASSERT(gap->row == row);
                _ASSERT(gap->idx == idx);
                rg.SetFirstFrom(rg.GetFirstFrom() + gap->shift);
                shift = gap->shift + gap->len;
                // This will merge new range with the previous one if necessary.
                dst->push_back(rg);
                continue;
            }
            else if (seg_it != pw.end()) {
                rg = *seg_it;
                ++seg_it;
            }
            // Check if there are new gaps at or before the new segment's position.
            while (next_gap != gaps.end()  &&
                next_gap->from <= rg.GetFirstFrom()) {
                ++next_gap;
                ++gap;
                shift = gap->shift + gap->len;
            }
            rg.SetFirstFrom(rg.GetFirstFrom() + shift);
            dst->push_back(rg);
        }
        m_Aln->SetPairwiseAlns()[row] = dst;
    }
    // Now the new anchored alignment should contain all segments and gaps
    // aligned to the new anchor coordinates.

    m_SecondRanges.resize(dim);
    for (TDim row = 0;  row < dim;  ++row) {

        /// Check collections flags
        _ASSERT( !m_Aln->GetPairwiseAlns()[row]->IsSet(TAlnRngColl::fInvalid) );
        _ASSERT( !m_Aln->GetPairwiseAlns()[row]->IsSet(TAlnRngColl::fUnsorted) );
        _ASSERT( !m_Aln->GetPairwiseAlns()[row]->IsSet(TAlnRngColl::fOverlap) );
        _ASSERT( !m_Aln->GetPairwiseAlns()[row]->IsSet(TAlnRngColl::fMixedDir) );

        /// Determine m_FirstRange
        if (row == 0) {
            m_FirstRange = m_Aln->GetPairwiseAlns()[row]->GetFirstRange();
        } else {
            m_FirstRange.CombineWith(m_Aln->GetPairwiseAlns()[row]->GetFirstRange());
        }

        /// Determine m_SecondRanges
        CAlignRangeCollExtender<TAlnRngColl> ext(*m_Aln->GetPairwiseAlns()[row]);
        ext.UpdateIndex();
        m_SecondRanges[row] = ext.GetSecondRange();
    }
}


CSparseAln::TRng CSparseAln::GetAlnRange() const
{
    return m_FirstRange;
}


void CSparseAln::SetGapChar(TResidue gap_char)
{
    m_GapChar = gap_char;
}


CRef<CScope> CSparseAln::GetScope() const
{
    return m_Scope;
}


const CSparseAln::TAlnRngColl&
CSparseAln::GetAlignCollection(TNumrow row)
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return *m_Aln->GetPairwiseAlns()[row];
}


const CSeq_id& CSparseAln::GetSeqId(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return m_Aln->GetPairwiseAlns()[row]->GetSecondId()->GetSeqId();
}


TSignedSeqPos   CSparseAln::GetSeqAlnStart(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return m_Aln->GetPairwiseAlns()[row]->GetFirstFrom();
}


TSignedSeqPos CSparseAln::GetSeqAlnStop(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return m_Aln->GetPairwiseAlns()[row]->GetFirstTo();
}


CSparseAln::TSignedRange CSparseAln::GetSeqAlnRange(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return TSignedRange(GetSeqAlnStart(row), GetSeqAlnStop(row));
}


TSeqPos CSparseAln::GetSeqStart(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return m_SecondRanges[row].GetFrom();
}


TSeqPos CSparseAln::GetSeqStop(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return m_SecondRanges[row].GetTo();
}


CSparseAln::TRange CSparseAln::GetSeqRange(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    return TRange(GetSeqStart(row), GetSeqStop(row));
}


bool CSparseAln::IsPositiveStrand(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    _ASSERT( !m_Aln->GetPairwiseAlns()[row]->IsSet(CPairwiseAln::fMixedDir) );
    return m_Aln->GetPairwiseAlns()[row]->IsSet(CPairwiseAln::fDirect);
}


bool CSparseAln::IsNegativeStrand(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    _ASSERT( !m_Aln->GetPairwiseAlns()[row]->IsSet(CPairwiseAln::fMixedDir) );
    return m_Aln->GetPairwiseAlns()[row]->IsSet(CPairwiseAln::fReversed);
}


bool CSparseAln::IsTranslated() const {
    /// TODO: Does BaseWidth of 1 always mean nucleotide?  Should we
    /// have an enum (with an invalid (unasigned) value?
    const int k_unasigned_base_width = 0;
    int base_width = k_unasigned_base_width;
    for (TDim row = 0;  row < GetDim();  ++row) {
        if (base_width == k_unasigned_base_width) {
            base_width = m_Aln->GetPairwiseAlns()[row]->GetFirstBaseWidth();
        }
        if (base_width != m_Aln->GetPairwiseAlns()[row]->GetFirstBaseWidth()  ||
            base_width != m_Aln->GetPairwiseAlns()[row]->GetSecondBaseWidth()) {
            return true; //< there *at least one* base diff base width
        }
        /// TODO: or should this check be stronger:
        if (base_width != 1) {
            return true;
        }
    }
    return false;
}


inline CSparseAln::TAlnRngColl::ESearchDirection
GetCollectionSearchDirection(CSparseAln::ESearchDirection dir)
{
    typedef CSparseAln::TAlnRngColl   T;
    switch(dir) {
    case CSparseAln::eNone:
        return T::eNone;
    case CSparseAln::eLeft:
        return T::eLeft;
    case CSparseAln::eRight:
        return T::eRight;
    case CSparseAln::eForward:
        return T::eForward;
    case CSparseAln::eBackwards:
        return T::eBackwards;
    }
    _ASSERT(false); // invalid
    return T::eNone;
}


TSignedSeqPos 
CSparseAln::GetAlnPosFromSeqPos(TNumrow row, 
                                TSeqPos seq_pos,
                                ESearchDirection dir,
                                bool try_reverse_dir) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    TAlnRngColl::ESearchDirection c_dir = GetCollectionSearchDirection(dir);
    return m_Aln->GetPairwiseAlns()[row]->GetFirstPosBySecondPos(seq_pos, c_dir);
}


TSignedSeqPos CSparseAln::GetSeqPosFromAlnPos(TNumrow row, TSeqPos aln_pos,
                                              ESearchDirection dir,
                                              bool try_reverse_dir) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    TAlnRngColl::ESearchDirection c_dir = GetCollectionSearchDirection(dir);
    return m_Aln->GetPairwiseAlns()[row]->GetSecondPosByFirstPos(aln_pos, c_dir);
}


const CBioseq_Handle&  CSparseAln::GetBioseqHandle(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    if ( !m_BioseqHandles[row] ) {
        if ( !(m_BioseqHandles[row] = m_Scope->GetBioseqHandle(GetSeqId(row))) ) {
            string errstr = "Invalid bioseq handle.  Seq id \"" +
                GetSeqId(row).AsFastaString() + "\" not in scope?";
            NCBI_THROW(CAlnException, eInvalidRequest, errstr);
        }
    }
    return m_BioseqHandles[row];
}


CSeqVector& CSparseAln::x_GetSeqVector(TNumrow row) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    if ( !m_SeqVectors[row] ) {
        CSeqVector vec = GetBioseqHandle(row).GetSeqVector
            (CBioseq_Handle::eCoding_Iupac,
             IsPositiveStrand(row) ? 
             CBioseq_Handle::eStrand_Plus :
             CBioseq_Handle::eStrand_Minus);
        m_SeqVectors[row].Reset(new CSeqVector(vec));
    }
    return *m_SeqVectors[row];
}


void CSparseAln::TranslateNAToAA(const string& na,
                                 string& aa,
                                 int gencode)
{
    const CTrans_table& tbl = CGen_code_table::GetTransTable(gencode);

    size_t na_remainder = na.size() % 3;
    size_t na_size = na.size() - na_remainder;

    if (&aa != &na) {
        aa.resize(na_size / 3 + (na_remainder ? 1 : 0));
    }

    size_t aa_i = 0;
    int state = 0;
    for (size_t na_i = 0;  na_i < na_size; ) {
        for (size_t i = 0;  i < 3;  ++i, ++na_i) {
            state = tbl.NextCodonState(state, na[na_i]);
        }
        aa[aa_i++] = tbl.GetCodonResidue(state);
    }
    if (na_remainder) {
        aa[aa_i++] = '\\';
    }

    if (&aa == &na) {
        aa[aa_i] = 0;
        aa.resize(aa_i);
    }
}


string& CSparseAln::GetSeqString(TNumrow row,
                                 string &buffer,
                                 TSeqPos seq_from, TSeqPos seq_to,
                                 bool force_translation) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    buffer.erase();
    int width = m_Aln->GetPairwiseAlns()[row]->GetSecondBaseWidth();
    if (width > 1) {
        seq_from /= 3;
        seq_to /= 3;
    }
    if (seq_to >= seq_from) {
        CSeqVector& seq_vector = x_GetSeqVector(row);

        size_t size = seq_to - seq_from + 1;
        buffer.resize(size, m_GapChar);

        if (IsPositiveStrand(row)) {
            seq_vector.GetSeqData(seq_from, seq_to + 1, buffer);
        } else {
            TSeqPos vec_size = seq_vector.size();
            seq_vector.GetSeqData(vec_size - seq_to - 1, vec_size - seq_from, buffer);
        }
    }
    return buffer;
}


string& CSparseAln::GetSeqString(TNumrow row,
                                 string &buffer,
                                 const TRange &seq_range,
                                 bool force_translation) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    return GetSeqString(row,
                        buffer,
                        seq_range.GetFrom(), seq_range.GetTo(),
                        force_translation);
}


string& CSparseAln::GetAlnSeqString(TNumrow row,
                                    string &buffer,
                                    const TSignedRange &aln_range,
                                    bool force_translation) const
{
    _ASSERT(row >= 0  &&  row < GetDim());

    bool translate = force_translation  ||  IsTranslated();

    buffer.erase();

    if(aln_range.GetLength() > 0)   {
        const CPairwiseAln& pairwise_aln = *m_Aln->GetPairwiseAlns()[row];
        _ASSERT( !pairwise_aln.empty() );
        if (pairwise_aln.empty()) {
            string errstr = "Invalid (empty) row (" + NStr::IntToString(row) + ").  Seq id \"" +
                GetSeqId(row).AsFastaString() + "\".";
            NCBI_THROW(CAlnException, eInvalidRequest, errstr);
        }

        CSeqVector& seq_vector = x_GetSeqVector(row);
        TSeqPos vec_size = seq_vector.size();

        const int base_width = pairwise_aln.GetSecondBaseWidth();

        // buffer holds sequence for "aln_range", 0 index corresonds to aln_range.GetFrom()
        size_t size = aln_range.GetLength();
        if (translate) {
            size /= 3;
        }
        buffer.resize(size, ' ');

        // check whether we have a gap at start position
        size_t prev_to_open = (pairwise_aln.GetFirstFrom() > aln_range.GetFrom()) ? string::npos : 0;

        string s;
        CSparse_CI it(pairwise_aln, IAlnSegmentIterator::eSkipGaps, aln_range);

        //LOG_POST_X(1, "GetAlnSeqString(" << row << ") ==========================================" );
        while (it)   {
            const IAlnSegment::TSignedRange& aln_r = it->GetAlnRange(); // in alignment
            const IAlnSegment::TSignedRange& r = it->GetRange(); // on sequence

            size_t off;
            //LOG_POST_X(2, "Aln [" << aln_r.GetFrom() << ", " << aln_r.GetTo() << "], Seq  "
            //                      << r.GetFrom() << ", " << r.GetTo());
            if (base_width == 1) {
                // TODO performance issue - waiting for better API
                if (IsPositiveStrand(row)) {
                    seq_vector.GetSeqData(r.GetFrom(), r.GetToOpen(), s);
                } else {
                    seq_vector.GetSeqData(vec_size - r.GetToOpen(),
                                          vec_size - r.GetFrom(), s);
                }
                if (translate) {
                    TranslateNAToAA(s, s);
                }
                off = aln_r.GetFrom() - aln_range.GetFrom();
                if (translate) {
                    off /= 3;
                }
            } else {
                _ASSERT(base_width == 3);
                IAlnSegment::TSignedRange prot_r = r;
                prot_r.SetFrom(r.GetFrom() / 3);
                prot_r.SetLength(r.GetLength() < 3 ? 1 : r.GetLength() / 3);
                if (IsPositiveStrand(row)) {
                    seq_vector.GetSeqData(prot_r.GetFrom(),
                                          prot_r.GetToOpen(), s);
                } else {
                    seq_vector.GetSeqData(vec_size - prot_r.GetToOpen(),
                                          vec_size - prot_r.GetFrom(), s);
                }
                off = (aln_r.GetFrom() - aln_range.GetFrom()) / 3;
            }
            /*if(it->IsReversed())    {
                std::reverse(s.begin(), s.end());
            }*/

            if(prev_to_open == string::npos) {
                // we have a gap at the start position
                buffer.replace(0, off, off, m_GapChar);
            } else {   // this is not the first segement
                off = max(prev_to_open, off);
                int gap_size = off - prev_to_open;
                buffer.replace(prev_to_open, gap_size, gap_size, m_GapChar);
            }

            size_t len = min(buffer.size() - off, s.size());

            _ASSERT(off + len <= buffer.size());

            buffer.replace(off, len, s, 0, len);
            prev_to_open = off + len;
            ++it;
        }
        int fill_len = size - prev_to_open;
        if(prev_to_open != string::npos  &&  fill_len > 0  &&  pairwise_aln.GetFirstTo() > aln_range.GetTo()) {
            // there is gap on the right
            buffer.replace(prev_to_open, fill_len, fill_len, m_GapChar);
        }
        //LOG_POST_X(3, buffer);
    }
    return buffer;
}


IAlnSegmentIterator*
CSparseAln::CreateSegmentIterator(TNumrow row,
                                  const TSignedRange& range,
                                  IAlnSegmentIterator::EFlags flag) const
{
    _ASSERT(row >= 0  &&  row < GetDim());
    _ASSERT( !m_Aln->GetPairwiseAlns()[row]->empty() );
    if (m_Aln->GetPairwiseAlns()[row]->empty()) {
        string errstr = "Invalid (empty) row (" + NStr::IntToString(row) + ").  Seq id \"" +
            GetSeqId(row).AsFastaString() + "\".";
        NCBI_THROW(CAlnException, eInvalidRequest, errstr);
    }
    return new CSparse_CI(*m_Aln->GetPairwiseAlns()[row], flag, range);
}


END_NCBI_SCOPE
