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
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the data definition file
 *   'seqalign.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>
#include <objects/seqalign/seqalign_exception.hpp>
#include <objects/seqalign/Dense_seg.hpp>
#include <objects/seqalign/Dense_diag.hpp>
#include <objects/seqalign/Std_seg.hpp>
#include <objects/seqalign/Seq_align_set.hpp>
#include <objects/seqalign/Score.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <serial/iterator.hpp>

// generated includes
#include <objects/seqalign/Seq_align.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSeq_align::~CSeq_align(void)
{
}


CSeq_align::TDim CSeq_align::CheckNumRows(void) const
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        return GetSegs().GetDenseg().CheckNumRows();
    case C_Segs::e_Std:
        {
            TDim numrows = 0;
            ITERATE (C_Segs::TStd, std_i, GetSegs().GetStd()) {
                const TDim& seg_numrows = (*std_i)->CheckNumRows();
                if (numrows) {
                    if (seg_numrows != numrows) {
                        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                                   "CSeq_align::CheckNumRows(): Number of rows "
                                   "is not the same for each std seg.");
                    }
                } else {
                    numrows = seg_numrows;
                }
            }
            return numrows;
        }
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::CheckNumRows() currently does not handle "
                   "this type of alignment");
    }
}


CRange<TSeqPos> CSeq_align::GetSeqRange(TDim row) const
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        return GetSegs().GetDenseg().GetSeqRange(row);
    case C_Segs::e_Dendiag:
        {
            CRange<TSeqPos> rng;
            ITERATE (C_Segs::TDendiag, dendiag_i, GetSegs().GetDendiag()) {
                rng.CombineWith((*dendiag_i)->GetSeqRange(row));
            }
            return rng;
        }
    case C_Segs::e_Std:
        {
            CRange<TSeqPos> rng;
            CSeq_id seq_id;
            size_t seg_i = 0;
            ITERATE (C_Segs::TStd, std_i, GetSegs().GetStd()) {
                TDim row_i = 0;
                ITERATE (CStd_seg::TLoc, i, (*std_i)->GetLoc()) {
                    const CSeq_loc& loc = **i;
                    if (row_i++ == row) {
                        if (loc.IsInt()) {
                            if ( !seg_i ) {
                                seq_id.Assign(loc.GetInt().GetId());
                            } else if (seq_id.Compare(loc.GetInt().GetId())
                                       != CSeq_id::e_YES) {
                                NCBI_THROW(CSeqalignException,
                                           eInvalidRowNumber,
                                           "CSeq_align::GetSeqRange():"
                                           " Row seqids not consistent."
                                           " Cannot determine range.");
                            }
                            rng.CombineWith
                                (CRange<TSeqPos>
                                 (loc.GetInt().GetFrom(),
                                  loc.GetInt().GetTo()));
                        }
                    }
                }
                if (row < 0  ||  row >= row_i) {
                    NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                               "CSeq_align::GetSeqRange():"
                               " Invalid row number");
                }
                if (CanGetDim()  &&  row_i != GetDim()) {
                    NCBI_THROW(CSeqalignException, eInvalidAlignment,
                               "CSeq_align::GetSeqRange():"
                               " loc.size is inconsistent with dim");
                }
                seg_i++;
            }
            if (rng.Empty()) {
                NCBI_THROW(CSeqalignException, eInvalidAlignment,
                           "CSeq_align::GetSeqRange(): Row is empty");
            }
            return rng;
        }
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::GetSeqRange() currently does not handle "
                   "this type of alignment.");
    }
}


TSeqPos CSeq_align::GetSeqStart(TDim row) const
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        return GetSegs().GetDenseg().GetSeqStart(row);
    case C_Segs::e_Dendiag:
    case C_Segs::e_Std:
        return GetSeqRange(row).GetFrom();
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::GetSeqStart() currently does not handle "
                   "this type of alignment.");
    }
}


TSeqPos CSeq_align::GetSeqStop (TDim row) const
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        return GetSegs().GetDenseg().GetSeqStop(row);
    case C_Segs::e_Dendiag:
    case C_Segs::e_Std:
        return GetSeqRange(row).GetTo();
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::GetSeqStop() currently does not handle "
                   "this type of alignment.");
    }
}


ENa_strand CSeq_align::GetSeqStrand(TDim row) const
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        return GetSegs().GetDenseg().GetSeqStrand(row);
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::GetSeqStrand() currently does not handle "
                   "this type of alignment.");
    }
}


const CSeq_id& CSeq_align::GetSeq_id(TDim row) const
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        {
            if ( GetSegs().GetDenseg().IsSetIds()  &&
                row < GetSegs().GetDenseg().GetIds().size()) {
                return *GetSegs().GetDenseg().GetIds()[row];
            }
            break;
        }
    case C_Segs::e_Dendiag:
        {
            // If segments have different number of rows, this will try
            // to find the segment with enough rows to get the id.
            ITERATE(CSeq_align::C_Segs::TDendiag, seg, GetSegs().GetDendiag()) {
                if( (*seg)->IsSetIds()  &&
                    row < (*seg)->GetIds().size() ) {
                    return *(*seg)->GetIds()[row];
                }
            }
            break;
        }
    case C_Segs::e_Std:
        {
            // If segments have different number of rows, this will try
            // to find the segment with enough rows to get the id.
            ITERATE(CSeq_align::C_Segs::TStd, seg, GetSegs().GetStd()) {
                if ( (*seg)->IsSetLoc()  &&
                    row < (*seg)->GetLoc().size() ) {
                    CSeq_loc_CI loc_iter(*(*seg)->GetLoc()[row]);
                    return loc_iter.GetSeq_id();
                }
            }
            break;
        }
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::GetSeq_id() currently does not handle "
                   "this type of alignment.");
   }
    NCBI_THROW(CSeqalignException, eInvalidRowNumber,
               "CSeq_align::GetSeq_id(): "
               "can not get seq-id for the row requested.");
    // return CSeq_id();
}


typedef pair<CSeq_align::EScoreType, string> TScoreNamePair;
static TScoreNamePair sc_ScoreNames[] = {
    TScoreNamePair(CSeq_align::eScore_Score,           "score"),
    TScoreNamePair(CSeq_align::eScore_BitScore,        "bit_score"),
    TScoreNamePair(CSeq_align::eScore_EValue,          "e_value"),
    TScoreNamePair(CSeq_align::eScore_IdentityCount,   "num_ident"),
    TScoreNamePair(CSeq_align::eScore_PercentIdentity, "pct_identity")
};


/// retrieve a named score object
CConstRef<CScore> CSeq_align::x_GetNamedScore(const string& name) const
{
    CConstRef<CScore> score;
    if (IsSetScore()) {
        ITERATE (TScore, iter, GetScore()) {
            if ((*iter)->IsSetId()  &&  (*iter)->GetId().IsStr()  &&
                (*iter)->GetId().GetStr() == name) {
                score = *iter;
                break;
            }
        }
    }

    return score;
}


CRef<CScore> CSeq_align::x_SetNamedScore(const string& name)
{
    CRef<CScore> score;
    if (IsSetScore()) {
        NON_CONST_ITERATE (TScore, iter, SetScore()) {
            if ((*iter)->IsSetId()  &&  (*iter)->GetId().IsStr()  &&
                (*iter)->GetId().GetStr() == name) {
                score = *iter;
                break;
            }
        }
    }

    if ( !score ) {
        score.Reset(new CScore);
        score->SetId().SetStr(name);
        SetScore().push_back(score);
    }

    return score;
}


///---------------------------------------------------------------------------
/// PRE : name of score to return
/// POST: whether or not we found that score; score converted to int
bool CSeq_align::GetNamedScore(const string &id, int &score) const
{
    CConstRef<CScore> ref = x_GetNamedScore(id);
    if (ref) {
        if (ref->GetValue().IsInt()) {
            score = ref->GetValue().GetInt();
        } else {
            score = (int)ref->GetValue().GetReal();
        }
        return true;
    }
    return false;
}

///---------------------------------------------------------------------------
/// PRE : name of score to return
/// POST: whether or not we found that score; score converted to double
bool CSeq_align::GetNamedScore(const string &id, double &score) const
{
    CConstRef<CScore> ref = x_GetNamedScore(id);
    if (ref) {
        if (ref->GetValue().IsInt()) {
            score = (double)ref->GetValue().GetInt();
        } else {
            score = ref->GetValue().GetReal();
        }
        return true;
    }
    return false;
}


bool CSeq_align::GetNamedScore(EScoreType type, int& score) const
{
    return GetNamedScore(sc_ScoreNames[type].second, score);
}

bool CSeq_align::GetNamedScore(EScoreType type, double& score) const
{
    return GetNamedScore(sc_ScoreNames[type].second, score);
}


void CSeq_align::SetNamedScore(EScoreType type, int score)
{
    CRef<CScore> ref = x_SetNamedScore(sc_ScoreNames[type].second);
    ref->SetValue().SetInt(score);
}

void CSeq_align::SetNamedScore(EScoreType type, double score)
{
    CRef<CScore> ref = x_SetNamedScore(sc_ScoreNames[type].second);
    ref->SetValue().SetReal(score);
}

void CSeq_align::SetNamedScore(const string& id, int score)
{
    CRef<CScore> ref = x_SetNamedScore(id);
    ref->SetValue().SetInt(score);
}

void CSeq_align::SetNamedScore(const string& id, double score)
{
    CRef<CScore> ref = x_SetNamedScore(id);
    ref->SetValue().SetReal(score);
}


void CSeq_align::Validate(bool full_test) const
{
    switch (GetSegs().Which()) {
    case TSegs::e_Dendiag:
        ITERATE(TSegs::TDendiag, dendiag_it, GetSegs().GetDendiag()) {
            (*dendiag_it)->Validate();
        }
        break;
    case C_Segs::e_Denseg:
        GetSegs().GetDenseg().Validate(full_test);
        break;
    case C_Segs::e_Disc:
        ITERATE(CSeq_align_set::Tdata, seq_align_it, GetSegs().GetDisc().Get()) {
            (*seq_align_it)->Validate(full_test);
        }
        break;
    case C_Segs::e_Std:
        CheckNumRows();
        break;
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::Validate() currently does not handle "
                   "this type of alignment");
    }
}


///---------------------------------------------------------------------------
/// PRE : currently only implemented for dense-seg segments
/// POST: same alignment, opposite orientation
void CSeq_align::Reverse(void)
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        SetSegs().SetDenseg().Reverse();
        break;
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::Reverse() currently only handles dense-seg "
                   "alignments");
    }
}

///---------------------------------------------------------------------------
/// PRE : currently only implemented for dense-seg segments; two row numbers
/// POST: same alignment, position of the two rows has been swapped
void CSeq_align::SwapRows(TDim row1, TDim row2)
{
    switch (GetSegs().Which()) {
    case C_Segs::e_Denseg:
        SetSegs().SetDenseg().SwapRows(row1, row2);
        break;
    case C_Segs::e_Disc:
        SetSegs().SetDisc().SwapRows(row1, row2);
        break;
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::SwapRows currently only handles dense-seg "
                   "alignments");
    }
}

///----------------------------------------------------------------------------
/// PRE : the Seq-align has StdSeg segs
/// POST: Seq_align of type Dense-seg is created with m_Widths if necessary
CRef<CSeq_align> 
CSeq_align::CreateDensegFromStdseg(SSeqIdChooser* SeqIdChooser) const
{
    if ( !GetSegs().IsStd() ) {
        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                   "CSeq_align::CreateDensegFromStdseg(): "
                   "Input Seq-align should have segs of type StdSeg!");
    }

    CRef<CSeq_align> sa(new CSeq_align);
    sa->SetType(eType_not_set);
    if (IsSetScore()) {
        sa->SetScore() = GetScore();
    }
    CDense_seg& ds = sa->SetSegs().SetDenseg();

    typedef CDense_seg::TDim    TNumrow;
    typedef CDense_seg::TNumseg TNumseg;

    vector<TSeqPos>       row_lens;
    CDense_seg::TLens&    seg_lens = ds.SetLens();
    CDense_seg::TStarts&  starts   = ds.SetStarts();
    CDense_seg::TStrands& strands  = ds.SetStrands();
    CDense_seg::TWidths&  widths   = ds.SetWidths();
    vector<bool>          widths_determined;

    TSeqPos row_len;
    TSeqPos from, to;
    ENa_strand strand;


    TNumseg seg = 0;
    TNumrow dim = 0, row = 0;
    ITERATE (C_Segs::TStd, std_i, GetSegs().GetStd()) {

        const CStd_seg& ss = **std_i;

        seg_lens.push_back(0);
        TSeqPos& seg_len = seg_lens.back();
        row_len = 0;
        row_lens.clear();
        widths_determined.push_back(false);

        row = 0;
        ITERATE (CStd_seg::TLoc, i, ss.GetLoc()) {

            const CSeq_id* seq_id = 0;
            // push back initialization values
            if (seg == 0) {
                widths.push_back(0);
                strands.push_back(eNa_strand_unknown);
            }

            if ((*i)->IsInt()) {
                const CSeq_interval& interval = (*i)->GetInt();
                
                // determine start and len
                from = interval.GetFrom();
                to = interval.GetTo();
                starts.push_back(from);
                row_len = to - from + 1;
                row_lens.push_back(row_len);
                
                int width = 0;
                // try to determine/check the seg_len and width
                if (!seg_len) {
                    width = 0;
                    seg_len = row_len;
                } else {
                    if (row_len * 3 == seg_len) {
                        seg_len /= 3;
                        width = 1;
                    } else if (row_len / 3 == seg_len) {
                        width = 3;
                    } else if (row_len != seg_len) {
                        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                                   "CreateDensegFromStdseg(): "
                                   "Std-seg segment lengths not accurate!");
                    }
                }
                if (width > 0) {
                    widths_determined[seg] = true;
                    if (widths[row] > 0  &&  widths[row] != width) {
                        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                                   "CreateDensegFromStdseg(): "
                                   "Std-seg segment lengths not accurate!");
                    } else {
                        widths[row] = width;
                    }
                }

                // get the id
                seq_id = &(*i)->GetInt().GetId();

                // determine/check the strand
                if (interval.CanGetStrand()) {
                    strand = interval.GetStrand();
                    if (seg == 0  ||  strands[row] == eNa_strand_unknown) {
                        strands[row] = strand;
                    } else {
                        if (strands[row] != strand) {
                            NCBI_THROW(CSeqalignException,
                                       eInvalidInputAlignment,
                                       "CreateDensegFromStdseg(): "
                                       "Inconsistent strands!");
                        }
                    }
                } else {
                    strand = eNa_strand_unknown;
                }

                    
            } else if ((*i)->IsEmpty()) {
                starts.push_back(-1);
                if (seg == 0) {
                    strands[row] = eNa_strand_unknown;
                }
                seq_id = &(*i)->GetEmpty();
                row_lens.push_back(0);

            }

            // determine/check the id
            if (seg == 0) {
                CRef<CSeq_id> id(new CSeq_id);
                SerialAssign(*id, *seq_id);
                ds.SetIds().push_back(id);
            } else {
                CSeq_id& id = *ds.SetIds()[row];
                if (!SerialEquals(id, *seq_id)) {
                    if (SeqIdChooser) {
                        SeqIdChooser->ChooseSeqId(id, *seq_id);
                    } else {
                        string errstr =
                            string("CreateDensegFromStdseg(): Seq-ids: ") +
                            id.AsFastaString() + " and " +
                            seq_id->AsFastaString() + " are not identical!" +
                            " Without the OM it cannot be determined if they belong to"
                            " the same sequence."
                            " Define and pass ChooseSeqId to resolve seq-ids.";
                        NCBI_THROW(CSeqalignException, eInvalidInputAlignment, errstr);
                    }
                }
            }

            // next row
            row++;
            if (seg == 0) {
                dim++;
            }
        }
        if (dim != ss.GetDim()  ||  row != dim) {
            NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                       "CreateDensegFromStdseg(): "
                       "Inconsistent dimentions!");
        }

        if (widths_determined[seg]) {
            // go back and determine/check widths
            for (row = 0; row < dim; row++) {
                if ((row_len = row_lens[row]) > 0) {
                    int width = 0;
                    if (row_len == seg_len * 3) {
                        width = 3;
                    } else if (row_len == seg_len) {
                        width = 1;
                    }
                    if (widths[row] > 0  &&  widths[row] != width) {
                        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                                   "CreateDensegFromStdseg(): "
                                   "Std-seg segment lengths not accurate!");
                    } else {
                        widths[row] = width;
                    }
                }
            }
        }

        // next seg
        seg++;
    }

    ds.SetDim(dim);
    ds.SetNumseg(seg);

    // go back and finish lens determination
    bool widths_failure = false;
    bool widths_success = false;
    for (seg = 0; seg < ds.GetNumseg(); seg++) {
        if (!widths_determined[seg]) {
            for(row = 0; row < dim; row++) {
                if (starts[seg * dim + row] >= 0) {
                    int width = widths[row];
                    if (width == 3) {
                        seg_lens[seg] /= 3;
                    } else if (width == 0) {
                        widths_failure = true;
                    }
                    break;
                }
            }
        } else {
            widths_success = true;
        }
    }

    if (widths_failure) {
        if (widths_success) {
            NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                       "CreateDensegFromStdseg(): "
                       "Some widths cannot be determined!");
        } else {
            ds.ResetWidths();
        }
    }
    
    // finish the strands
    for (seg = 1; seg < ds.GetNumseg(); seg++) {
        for (row = 0; row < dim; row++) {
            strands.push_back(strands[row]);
        }
    }

    return sa;
}


///----------------------------------------------------------------------------
/// PRE : the Seq-align is a Dense-seg of aligned nucleotide sequences
/// POST: Seq_align of type Dense-seg is created with each of the m_Widths 
///       equal to 3 and m_Lengths devided by 3.
CRef<CSeq_align> 
CSeq_align::CreateTranslatedDensegFromNADenseg() const
{
    if ( !GetSegs().IsDenseg() ) {
        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                   "CSeq_align::CreateTranslatedDensegFromNADenseg(): "
                   "Input Seq-align should have segs of type Dense-seg!");
    }
    
    CRef<CSeq_align> sa(new CSeq_align);
    sa->SetType(eType_not_set);

    if (GetSegs().GetDenseg().IsSetWidths()) {
        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                   "CSeq_align::CreateTranslatedDensegFromNADenseg(): "
                   "Widths already exist for the original alignment");
    }

    // copy from the original
    sa->Assign(*this);

    CDense_seg& ds = sa->SetSegs().SetDenseg();

    // fix the lengths
    const CDense_seg::TLens& orig_lens = GetSegs().GetDenseg().GetLens();
    CDense_seg::TLens&       lens      = ds.SetLens();

    for (CDense_seg::TNumseg numseg = 0; numseg < ds.GetNumseg(); numseg++) {
        if (orig_lens[numseg] % 3) {
            string errstr =
                string("CSeq_align::CreateTranslatedDensegFromNADenseg(): ") +
                "Length of segment " + NStr::IntToString(numseg) +
                " is not divisible by 3.";
            NCBI_THROW(CSeqalignException, eInvalidInputAlignment, errstr);
        } else {
            lens[numseg] = orig_lens[numseg] / 3;
        }
    }

    // add the widths
    ds.SetWidths().resize(ds.GetDim(), 3);

#if _DEBUG
    ds.Validate(true);
#endif

    return sa;
}


/// Strict weak ordering for pairs (by first)
/// Used by CreateDensegFromDisc
template <typename T, typename Pred = less<TSeqPos> >
struct ds_cmp : public binary_function<T, T, bool> {
    bool operator()(const T& x, const T& y) { 
        return m_Pred(x.first, y.first); 
    }
private:
    Pred m_Pred;
};


CRef<CSeq_align> 
CSeq_align::CreateDensegFromDisc(SSeqIdChooser* SeqIdChooser) const
{
    if ( !GetSegs().IsDisc() ) {
        NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                   "CSeq_align::CreateDensegFromDisc(): "
                   "Input Seq-align should have segs of type StdSeg!");
    }

    CRef<CSeq_align> new_sa(new CSeq_align);
    new_sa->SetType(eType_not_set);
    if (IsSetScore()) {
        new_sa->SetScore() = GetScore();
    }

    CDense_seg& new_ds = new_sa->SetSegs().SetDenseg();

    new_ds.SetDim(0);
    new_ds.SetNumseg(0);


    /// Order the discontinuous densegs
    typedef pair<TSeqPos, const CDense_seg *> TPosDsPair;
    typedef vector<TPosDsPair> TDsVec;
    TDsVec ds_vec;
    ds_vec.reserve(GetSegs().GetDisc().Get().size());
    int strand = -1;
    ITERATE (CSeq_align_set::Tdata, sa_i, GetSegs().GetDisc().Get()) {
        const CDense_seg& ds = (*sa_i)->GetSegs().GetDenseg();
        ds_vec.push_back(make_pair<TSeqPos, const CDense_seg *>(ds.GetSeqStart(0), &ds));
        if (strand < 0) {
            strand = ds.GetStrands()[0];
        } else {
            if (strand != ds.GetStrands()[0]) {
                NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                           "CreateDensegFromDisc(): "
                           "Inconsistent strands!");
            }
        }
    }
    if (strand != eNa_strand_minus) {
        sort(ds_vec.begin(), ds_vec.end(),
             ds_cmp<TPosDsPair>());
    } else {
        sort(ds_vec.begin(), ds_vec.end(),
             ds_cmp<TPosDsPair, greater<TSeqPos> >());
    }


    /// First pass: determine dim & numseg
    ITERATE(TDsVec, ds_i, ds_vec) {
        const CDense_seg& ds = *ds_i->second;

        /// Numseg
        new_ds.SetNumseg() += ds.GetNumseg();

        /// Dim
        if ( !new_ds.GetDim() ) {
            new_ds.SetDim(ds.GetDim());
        } else if (new_ds.GetDim() != ds.GetDim() ) {
            NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                       "CreateDensegFromDisc(): "
                       "All disc dense-segs need to have the same dimension!");
        }

        /// Strands?
        if ( !ds.GetStrands().empty() ) {
            if (new_ds.GetStrands().empty()) {
                new_ds.SetStrands().assign(ds.GetStrands().begin(),
                                           ds.GetStrands().begin() + ds.GetDim());
            } else {
                if ( !equal(new_ds.GetStrands().begin(),
                            new_ds.GetStrands().end(),
                            ds.GetStrands().begin()) ) {
                    NCBI_THROW(CSeqalignException, eInvalidInputAlignment,
                               "CreateDensegFromDisc(): "
                               "All disc dense-segs need to have the same strands!");
                }
            }
        }
    }
    
    new_ds.SetStarts().resize(new_ds.GetDim() * new_ds.GetNumseg());
    new_ds.SetLens().resize(new_ds.GetNumseg());
    if ( !new_ds.GetStrands().empty() ) {
        /// Multiply the strands by the number of segments.
        new_ds.SetStrands().reserve(new_ds.GetDim() * new_ds.GetNumseg());
        for (CDense_seg::TNumseg seg = 0; 
             seg < new_ds.GetNumseg() - 1;
             ++seg) {
            new_ds.SetStrands().insert(new_ds.SetStrands().end(),
                                       new_ds.GetStrands().begin(), 
                                       new_ds.GetStrands().begin() + new_ds.GetDim());
        }
    }
    

    /// Second pass: validate and set ids and starts

    CDense_seg::TNumseg new_seg = 0;
    int                 new_starts_i = 0;

    ITERATE(TDsVec, ds_i, ds_vec) {
        const CDense_seg& ds = *ds_i->second;
        
        _ASSERT(ds.GetStarts().size() == ds.GetNumseg() * ds.GetDim());
        _ASSERT(new_ds.GetDim() == ds.GetDim());

        /// Ids
        if (new_ds.GetIds().empty()) {
            new_ds.SetIds().resize(new_ds.GetDim());
            for (CDense_seg::TDim row = 0;  row < ds.GetDim();  ++row) {
                CRef<CSeq_id> id(new CSeq_id);
                SerialAssign(*id, *ds.GetIds()[row]);
                new_ds.SetIds()[row] = id;
            }
        } else {
            if (SeqIdChooser) {
                for (CDense_seg::TDim row = 0;  row < ds.GetDim();  ++row) {
                    SeqIdChooser->ChooseSeqId(*new_ds.SetIds()[row], *ds.GetIds()[row]);
                }
            } else {
#if _DEBUG                    
                for (CDense_seg::TDim row = 0;  row < ds.GetDim();  ++row) {
                    const CSeq_id& new_id = *new_ds.GetIds()[row];
                    const CSeq_id& id     = *ds.GetIds()[row];
                    if ( !SerialEquals(new_id, id) ) {
                        string errstr =
                            string("CreateDensegFromDisc(): Seq-ids: ") +
                            new_id.AsFastaString() + " and " +
                            id.AsFastaString() + " are not identical!" +
                            " Without the OM it cannot be determined if they belong to"
                            " the same sequence."
                            " Define and pass ChooseSeqId to resolve seq-ids.";
                        NCBI_THROW(CSeqalignException, eInvalidInputAlignment, errstr);
                    }
                }
#endif
            }
        }

        /// Starts
        int starts_i = 0;
        for (CDense_seg::TNumseg seg = 0;
             seg < ds.GetNumseg();
             ++new_seg, ++seg) {

            new_ds.SetLens()[new_seg] = ds.GetLens()[seg];

            for (CDense_seg::TDim row = 0;
                 row < ds.GetDim(); 
                 ++starts_i, ++new_starts_i, ++row) {
                new_ds.SetStarts()[new_starts_i] = ds.GetStarts()[starts_i];
            }
        }
    }

    
    /// Perform full test (including segment starts consistency check)
    new_sa->Validate(true);


    return new_sa;
}


void CSeq_align::OffsetRow(TDim row,
                          TSeqPos offset)
{
    if (offset == 0) return;

    _ASSERT(offset > 0);

    switch (SetSegs().Which()) {
    case TSegs::e_Dendiag:
        NON_CONST_ITERATE(TSegs::TDendiag, dendiag_it, SetSegs().SetDendiag()) {
            (*dendiag_it)->OffsetRow(row, offset);
        }
        break;
    case TSegs::e_Denseg:
        SetSegs().SetDenseg().OffsetRow(row, offset);
        break;
    case TSegs::e_Std:
        NON_CONST_ITERATE(TSegs::TStd, std_it, SetSegs().SetStd()) {
            (*std_it)->OffsetRow(row, offset);
        }
        break;
    case TSegs::e_Disc:
        NON_CONST_ITERATE(CSeq_align_set::Tdata, seq_align_it, SetSegs().SetDisc().Set()) {
            (*seq_align_it)->OffsetRow(row, offset);
        }
        break;
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::OffsetRow() currently does not handle "
                   "this type of alignment");
    }
}


/// @deprecated
void CSeq_align::RemapToLoc(TDim row,
                            const CSeq_loc& dst_loc,
                            bool ignore_strand)
{
    // Limit to certain types of locs:
    switch (dst_loc.Which()) {
    case CSeq_loc::e_Whole:
        return;
    case CSeq_loc::e_Int:
        break;
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::RemapToLoc only supports int target seq-locs");
    }

    switch (SetSegs().Which()) {
    case TSegs::e_Denseg:
        SetSegs().SetDenseg().RemapToLoc(row, dst_loc, ignore_strand);
        break;
    case TSegs::e_Std:
        NON_CONST_ITERATE(TSegs::TStd, std_it, SetSegs().SetStd()) {
            (*std_it)->RemapToLoc(row, dst_loc, ignore_strand);
        }
        break;
    case TSegs::e_Disc:
        NON_CONST_ITERATE(CSeq_align_set::Tdata, seq_align_it, SetSegs().SetDisc().Set()) {
            (*seq_align_it)->RemapToLoc(row, dst_loc, ignore_strand);
        }
        break;
    default:
        NCBI_THROW(CSeqalignException, eUnsupported,
                   "CSeq_align::RemapToLoc only supports Dense-seg and Std-seg alignments.");
    }
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.29  2006/07/24 13:11:05  dicuccio
* Doxygenated.  Added support for creating named scores
*
* Revision 1.28  2006/07/17 15:49:26  todorov
* Simplified and optimized building the strands in CreateDensegFromDisc.
*
* Revision 1.27  2006/07/13 20:34:55  todorov
* CreateDensegFromDisc() now copies the scores too.
*
* Revision 1.26  2006/07/12 23:01:44  todorov
* Added support for Dendiag and Disc alignments in Validate()
*
* Revision 1.25  2006/07/12 21:24:07  todorov
* Added support for Dense-diag in OffsetRow.
*
* Revision 1.24  2006/07/07 14:25:03  todorov
* 1) Support for unordered disc densegs in CreateDensegFromDisc.
* 2) Better strands support in CreateDensegFromDisc.
*
* Revision 1.23  2006/06/06 22:42:54  todorov
* Added OffsetRow method.
* Marked RemapToLoc for deprecation.
*
* Revision 1.22  2006/05/23 18:41:29  todorov
* Added CreateDensegFromDisc.
*
* Revision 1.21  2006/05/09 15:44:37  todorov
* Added support of disc alignments to RemapToLoc.
*
* Revision 1.20  2006/05/08 21:42:09  todorov
* Added a RemapToLoc method.
*
* Revision 1.19  2005/08/15 18:22:42  todorov
* CreateDensegFromStdseg now copies the Seq-align's scores too.
*
* Revision 1.18  2005/07/06 19:07:21  todorov
* Added support for Dense-diag in the Get{Start,Stop,Range} methods.
*
* Revision 1.17  2004/06/14 22:09:03  johnson
* Added GetSeqStrand method (analogous to GetSeq_id)
*
* Revision 1.16  2004/05/19 17:25:43  gorelenk
* Added include of PCH - ncbi_pch.hpp
*
* Revision 1.15  2004/05/05 19:16:25  johnson
* Added SwapRows method for 'disc' seq-align / seq-align-set
*
* Revision 1.14  2004/04/27 19:17:13  johnson
* Added GetNamedScore helper function
*
* Revision 1.13  2004/04/19 17:27:22  grichenk
* Added GetSeq_id(TDim row)
*
* Revision 1.12  2004/04/12 16:51:02  vasilche
* Fixed uninitialized variables.
*
* Revision 1.11  2004/03/09 17:27:42  kuznets
* Bug fix(compilation): CSeq_align::CreateDensegFromStdseg MSVC
*
* Revision 1.10  2004/03/09 17:14:33  todorov
* changed the C-style callback to a functor
*
* Revision 1.9  2004/02/23 15:31:24  todorov
* +TChooseSeqIdCallback to abstract resolving seq-ids in CreateDensegFromStdseg()
*
* Revision 1.8  2004/02/13 17:10:24  todorov
* - inconsistent ids exception in CreateDensegFromStdseg
*
* Revision 1.7  2004/01/09 16:32:06  todorov
* +SetType(eType_not_set)
*
* Revision 1.6  2003/12/16 22:54:31  todorov
* +CreateTranslatedDensegFromNADenseg
*
* Revision 1.5  2003/12/11 22:34:05  todorov
* Implementation of GetSeq{Start,Stop}
*
* Revision 1.4  2003/09/16 15:31:14  todorov
* Added validation methods. Added seq range methods
*
* Revision 1.3  2003/08/26 20:28:38  johnson
* added 'SwapRows' method
*
* Revision 1.2  2003/08/19 21:11:13  todorov
* +CreateDensegFromStdseg
*
* Revision 1.1  2003/08/13 18:12:03  johnson
* added 'Reverse' method
*
*
* ===========================================================================
*/
/* Original file checksum: lines: 64, chars: 1885, CRC32: 4e5d1825 */
