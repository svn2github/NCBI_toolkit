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
#include <algorithm>
#include <objects/seqalign/seqalign_exception.hpp>

// generated includes
#include <objects/seqalign/Dense_seg.hpp>

#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CDense_seg::~CDense_seg(void)
{
}


void CDense_seg::Assign(const CSerialObject& obj, ESerialRecursionMode how)
{
    /// do the base copy
    CSerialObject::Assign(obj, how);

    /// copy our specific items
    if (GetTypeInfo() == obj.GetThisTypeInfo()) {
        const CDense_seg& other = static_cast<const CDense_seg&>(obj);
        m_set_State1[0] = other.m_set_State1[0];
        m_Widths = other.m_Widths;
    }
}


CDense_seg::TNumseg CDense_seg::CheckNumSegs() const
{
    const CDense_seg::TStarts&  starts  = GetStarts();
    const CDense_seg::TStrands& strands = GetStrands();
    const CDense_seg::TLens&    lens    = GetLens();
    const CDense_seg::TWidths&  widths  = GetWidths();

    const size_t& numrows = GetDim();
    const size_t& numsegs = GetNumseg();
    const size_t  num     = numrows * numsegs;

    if (starts.size() != num) {
        string errstr = string("CDense_seg::CheckNumSegs():")
            + " starts.size is inconsistent with dim * numseg";
        NCBI_THROW(CSeqalignException, eInvalidAlignment, errstr);
    }
    if (lens.size() != numsegs) {
        string errstr = string("CDense_seg::CheckNumSegs():")
            + " lens.size is inconsistent with numseg";
        NCBI_THROW(CSeqalignException, eInvalidAlignment, errstr);
    }
    if (strands.size()  &&  strands.size() != num) {
        string errstr = string("CDense_seg::CheckNumSegs():")
            + " strands.size is inconsistent with dim * numseg";
        NCBI_THROW(CSeqalignException, eInvalidAlignment, errstr);
    }
    if (widths.size()  &&  widths.size() != numrows) {
        string errstr = string("CDense_seg::CheckNumSegs():")
            + " widths.size is inconsistent with dim";
        NCBI_THROW(CSeqalignException, eInvalidAlignment, errstr);
    }
    return numsegs;
}


TSeqPos CDense_seg::GetSeqStart(TDim row) const
{
    const TDim&    dim    = GetDim();
    const TNumseg& numseg = CheckNumSegs();
    const TStarts& starts = GetStarts();

    if (row < 0  ||  row >= dim) {
        NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                   "CDense_seg::GetSeqStart():"
                   " Invalid row number");
    }

    TSignedSeqPos start;
    if (CanGetStrands()  &&  GetStrands().size()  &&  GetStrands()[row] == eNa_strand_minus) {
        TNumseg seg = numseg;
        int pos = (seg - 1) * dim + row;
        while (seg--) {
            if ((start = starts[pos]) >= 0) {
                return start;
            }
            pos -= dim;
        }
    } else {
        TNumseg seg = -1;
        int pos = row;
        while (++seg < numseg) {
            if ((start = starts[pos]) >= 0) {
                return start;
            }
            pos += dim;
        }
    }
    NCBI_THROW(CSeqalignException, eInvalidAlignment,
               "CDense_seg::GetSeqStart(): Row is empty");
}


TSeqPos CDense_seg::GetSeqStop(TDim row) const
{
    const TDim& dim       = GetDim();
    const TNumseg& numseg = CheckNumSegs();
    const TStarts& starts = GetStarts();

    if (row < 0  ||  row >= dim) {
        NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                   "CDense_seg::GetSeqStop():"
                   " Invalid row number");
    }

    TSignedSeqPos start;
    if (CanGetStrands()  &&  GetStrands().size()  &&  GetStrands()[row] == eNa_strand_minus) {
        TNumseg seg = -1;
        int pos = row;
        while (++seg < numseg) {
            if ((start = starts[pos]) >= 0) {
                return start + GetLens()[seg] - 1;
            }
            pos += dim;
        }
    } else {
        TNumseg seg = numseg;
        int pos = (seg - 1) * dim + row;
        while (seg--) {
            if ((start = starts[pos]) >= 0) {
                return start + GetLens()[seg] - 1;
            }
            pos -= dim;
        }        
    }
    NCBI_THROW(CSeqalignException, eInvalidAlignment,
               "CDense_seg::GetSeqStop(): Row is empty");
}


ENa_strand CDense_seg::GetSeqStrand(TDim row) const
{
    if (row < 0  ||  row >= GetDim()) {
        NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                   "CDense_seg::GetSeqStrand():"
                   " Invalid row number");
    }


    if (!CanGetStrands()  ||  (int)GetStrands().size() <= row) {
        NCBI_THROW(CSeqalignException, eInvalidInputData,
                   "CDense_seg::GetSeqStrand():"
                   " Strand doesn't exist for this row.");
    }

    return GetStrands()[row];
}


void CDense_seg::Validate(bool full_test) const
{
    const CDense_seg::TStarts&  starts  = GetStarts();
    const CDense_seg::TStrands& strands = GetStrands();
    const CDense_seg::TLens&    lens    = GetLens();
    const CDense_seg::TWidths&  widths  = GetWidths();

    const size_t& numrows = CheckNumRows();
    const size_t& numsegs = CheckNumSegs();

    if (full_test) {
        const size_t  max     = numrows * (numsegs -1);

        bool strands_exist = !strands.empty();

        size_t numseg = 0, numrow = 0, offset = 0;
        for (numrow = 0;  numrow < numrows;  numrow++) {
            TSignedSeqPos min_start = -1, start;
            bool plus = strands_exist ? 
                strands[numrow] != eNa_strand_minus:
                true;
            
            if (plus) {
                offset = 0;
            } else {
                offset = max;
            }
            
            for (numseg = 0;  numseg < numsegs;  numseg++) {
                start = starts[offset + numrow];
                if (start >= 0) {
                    if (start < min_start) {
                        string errstr = string("CDense_seg::Validate():")
                            + " Starts are not consistent!"
                            + " Row=" + NStr::IntToString(numrow) +
                            " Seg=" + NStr::IntToString(plus ? numseg :
                                                        numsegs - 1 - numseg) +
                            " MinStart=" + NStr::IntToString(min_start) +
                            " Start=" + NStr::IntToString(start);
                        
                        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                                   errstr);
                    }
                    min_start = start + 
                        lens[plus ? numseg : numsegs - 1 - numseg] *
                        (widths.size() == numrows ?
                         widths[numrow] : 1);
                }
                if (plus) {
                    offset += numrows;
                } else {
                    offset -= numrows;
                }
            }
        }
    }
}


void CDense_seg::TrimEndGaps()
{
    _ASSERT(GetNumseg() == GetLens().size());
    _ASSERT(GetNumseg() * GetDim() == GetStarts().size());
    _ASSERT(!IsSetStrands()  ||  GetNumseg() * GetDim() == GetStrands().size());
    _ASSERT(GetDim() == GetIds().size());

    list<TSignedSeqRange> delete_ranges;
    int i;
    int j;

    /// leading gap segments first
    for (i = 0;  i < GetNumseg();  ++i) {
        int count_gaps = 0;
        for (j = 0;  j < GetDim();  ++j) {
            TSignedSeqPos this_start = GetStarts()[i * GetDim() + j];
            if (this_start == -1) {
                ++count_gaps;
            }
        }

        if (GetDim() - count_gaps > 1) {
            /// no can do
            break;
        }
    }

    if (i == GetNumseg() + 1) {
        /// error case - all gapped, so don't bother
        return;
    }
    if (i != 0) {
        delete_ranges.push_back(TSignedSeqRange(0, i));
    }

    /// trailing gap segments next
    for (i = GetNumseg() - 1;  i >= 0;  --i) {
        int count_gaps = 0;
        for (j = 0;  j < GetDim();  ++j) {
            TSignedSeqPos this_start = GetStarts()[i * GetDim() + j];
            if (this_start == -1) {
                ++count_gaps;
            }
        }

        if (GetDim() - count_gaps > 1) {
            /// no can do
            break;
        }
    }

    if (i != GetNumseg() - 1) {
        delete_ranges.push_back(TSignedSeqRange(i + 1, GetNumseg()));
    }

    list<TSignedSeqRange>::reverse_iterator iter = delete_ranges.rbegin();
    list<TSignedSeqRange>::reverse_iterator end  = delete_ranges.rend();
    for ( ;  iter != end;  ++iter) {
        TSignedSeqRange r = *iter;
        if (r.GetLength() == 0) {
            continue;
        }

        /// we can trim the first i segments
        if (IsSetStrands()) {
            _ASSERT(GetStrands().size() > r.GetTo() * GetDim());
            SetStrands().erase(SetStrands().begin() + r.GetFrom() * GetDim(),
                               SetStrands().begin() + r.GetTo()   * GetDim());
        }
        if (IsSetStarts()) {
            _ASSERT(GetStarts().size() > r.GetTo() * GetDim());
            SetStarts().erase(SetStarts().begin() + r.GetFrom() * GetDim(),
                              SetStarts().begin() + r.GetTo()   * GetDim());
        }
        if (IsSetLens()) {
            _ASSERT(GetLens().size() > r.GetTo());
            SetLens().erase(SetLens().begin() + r.GetFrom(),
                            SetLens().begin() + r.GetTo());
        }
    }

    /// fix our number of segments
    SetNumseg(SetLens().size());

    _ASSERT(GetNumseg() == GetLens().size());
    _ASSERT(GetNumseg() * GetDim() == GetStarts().size());
    _ASSERT(!IsSetStrands()  ||  GetNumseg() * GetDim() == GetStrands().size());
    _ASSERT(GetDim() == GetIds().size());
}


void CDense_seg::Compact()
{
    _ASSERT(GetNumseg() == GetLens().size());
    _ASSERT(GetNumseg() * GetDim() == GetStarts().size());
    _ASSERT(!IsSetStrands()  ||  GetNumseg() * GetDim() == GetStrands().size());
    _ASSERT(GetDim() == GetIds().size());
    int i;
    int j;
    for (i = 0;  i < GetNumseg() - 1;  ) {
        bool can_merge = true;
        int gap_count = 0;
        for (j = 0;  j < GetDim();  ++j) {
            TSignedSeqPos this_start = GetStarts()[i * GetDim() + j];
            TSignedSeqPos next_start = GetStarts()[(i + 1) * GetDim() + j];
            if (this_start == -1) {
                ++gap_count;
            }

            /// check to make sure there is not a gap mismatch
            if ( (this_start == -1  &&  next_start != -1)  ||  (this_start != -1  &&  next_start == -1) ) {
                can_merge = false;
                break;
            }

            /// check to make sure there is no unaligned space
            /// between this segment and the next
            if (this_start != -1  &&  next_start != -1) {
                TSignedSeqPos seg_len = GetLens()[i];
                if (IsSetStrands()  &&
                    GetStrands()[i * GetDim() + j] == eNa_strand_minus) {
                    seg_len = GetLens()[i + 1];
                    seg_len = -seg_len;
                }

                if (this_start + seg_len != next_start) {
                    can_merge = false;
                    break;
                }
            }
        }

        if (can_merge) {
            if (IsSetStrands()) {
                for (j = 0;  j < GetDim();  ++j) {
                    if (GetStrands()[i * GetDim() + j] == eNa_strand_minus) {
                        SetStarts()[i * GetDim() + j] =
                            SetStarts()[(i + 1) * GetDim() + j];
                        SetStrands()[i * GetDim() + j] =
                            SetStrands()[(i + 1) * GetDim() + j];
                    }
                }
                SetStrands().erase(SetStrands().begin() + (i + 1) * GetDim(),
                                   SetStrands().begin() + (i + 2) * GetDim());
            }
            SetStarts().erase(SetStarts().begin() + (i + 1) * GetDim(),
                              SetStarts().begin() + (i + 2) * GetDim());
            if (gap_count != GetDim()) {
                SetLens()[i] += GetLens()[i + 1];
            }
            SetLens().erase(SetLens().begin() + i + 1);
        } else {
            ++i;
        }
    }

    SetNumseg(SetLens().size());

    _ASSERT(GetNumseg() == GetLens().size());
    _ASSERT(GetNumseg() * GetDim() == GetStarts().size());
    _ASSERT(!IsSetStrands()  ||  GetNumseg() * GetDim() == GetStrands().size());
    _ASSERT(GetDim() == GetIds().size());
}


//-----------------------------------------------------------------------------
// PRE : none
// POST: same alignment, opposite orientation
void CDense_seg::Reverse(void)
{
    //flip strands
    if (IsSetStrands()) {
        NON_CONST_ITERATE (CDense_seg::TStrands, i, SetStrands()) {
            switch (*i) {
            case eNa_strand_plus:  *i = eNa_strand_minus; break;
            case eNa_strand_minus: *i = eNa_strand_plus;  break;
            default:                    break;//do nothing if not + or -
            }
        }
    } else {
        // Interpret unset strands as plus strands.
        // Since we're reversing, set them all to minus.
        SetStrands().resize(GetStarts().size(), eNa_strand_minus);
    }

    //reverse list o' lengths
    {
        CDense_seg::TLens::iterator f = SetLens().begin();
        CDense_seg::TLens::iterator r = SetLens().end();
        while (f < r) {
            swap(*(f++), *(--r));
        }
    }

    //reverse list o' starts
    CDense_seg::TStarts &starts = SetStarts();
    int f = 0;
    int r = (GetNumseg() - 1) * GetDim();
    while (f < r) {
        for (int i = 0;  i < GetDim();  ++i) {
            swap(starts[f+i], starts[r+i]);
        }
        f += GetDim();
        r -= GetDim();
    }
}

//-----------------------------------------------------------------------------
// PRE : numbers of the rows to swap
// POST: alignment rearranged with row1 where row2 used to be & vice versa
void CDense_seg::SwapRows(TDim row1, TDim row2)
{
    if (row1 >= GetDim()  ||  row1 < 0  ||
        row2 >= GetDim()  ||  row2 < 0) {
        NCBI_THROW(CSeqalignException, eOutOfRange,
                   "Row numbers supplied to CDense_seg::SwapRows must be "
                   "in the range [0, dim)");
    }

    //swap ids
    swap(SetIds()[row1], SetIds()[row2]);

    int idxStop = GetNumseg()*GetDim();
    
    //swap starts
    for(int i = 0; i < idxStop; i += GetDim()) {
        swap(SetStarts()[i+row1], SetStarts()[i+row2]);
    }

    //swap strands
    if (IsSetStrands()) {
        for(int i = 0; i < idxStop; i += GetDim()) {
            swap(SetStrands()[i+row1], SetStrands()[i+row2]);
        }
    }
}


/*---------------------------------------------------------------------------*/
// PRE : this is a validated dense seg; row & sequence position on row in
// alignment
// POST: the number of the segment in which this sequence position falls
CDense_seg::TNumseg CDense_seg::
x_FindSegment(CDense_seg::TDim row, TSignedSeqPos pos) const
{
    bool found = false;
    CDense_seg::TNumseg seg;
    for (seg = 0;  seg < GetNumseg()  &&  !found;  ++seg) {
        TSignedSeqPos start = GetStarts()[seg * GetDim() + row];
        TSignedSeqPos len   = GetLens()[seg];
        if (start != -1) {
            if (pos >= start  &&  pos < start + len) {
                found = true;
            }
        }
    }
    if (!found) {
        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CDense_seg::x_FindSegment(): "
                   "Can't find a segment containing position " +
                   NStr::IntToString(pos));
    }

    return seg - 1;
}


//-----------------------------------------------------------------------------
// PRE : range on a row in the alignment
// POST: dst Dense_seg reset & 
CRef<CDense_seg> CDense_seg::
ExtractSlice(TDim row, TSeqPos from, TSeqPos to) const
{
    if (row < 0  ||  row >= GetDim()) {
        NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                   "CDense_seg::ExtractSlice():"
                   " Invalid row number");
    }

    if (from > to) {
        swap(from, to);
    }
    if (from < GetSeqStart(row)) {
        NCBI_THROW(CSeqalignException, eOutOfRange,
                   "CDense_seg::ExtractSlice(): "
                   "start position (" + NStr::IntToString(from) +
                   ") off end of alignment");
    }
    if (to > GetSeqStop(row)) {
        NCBI_THROW(CSeqalignException, eOutOfRange,
                   "CDense_seg::ExtractSlice(): "
                   "stop position (" + NStr::IntToString(to) +
                   ") off end of alignment");
    }

 
    CRef<CDense_seg> ds(new CDense_seg);    
    ds->SetDim(GetDim());
    ds->SetNumseg(0);
    ITERATE(CDense_seg::TIds, idI, GetIds()) {
        CSeq_id *si = new CSeq_id;
        si->Assign(**idI);
        ds->SetIds().push_back(CRef<CSeq_id>(si));
    }

    //find start/stop segments
    CDense_seg::TNumseg startSeg = x_FindSegment(row, from);
    CDense_seg::TNumseg stopSeg  = x_FindSegment(row, to);

    TSeqPos startOffset = from - GetStarts()[startSeg * GetDim() + row];
    TSeqPos stopOffset  = GetStarts()[stopSeg * GetDim() + row] +
        GetLens()[stopSeg] - 1 - to;
    if (IsSetStrands() && GetStrands()[row] == eNa_strand_minus) {
        swap(startOffset, stopOffset);
        swap(startSeg, stopSeg); // make sure startSeg is first
    }

    for (CDense_seg::TNumseg seg = startSeg;  seg <= stopSeg;  ++seg) {
        //starts
        for (CDense_seg::TDim dim = 0;  dim < GetDim();  ++dim) {
            TSignedSeqPos start = GetStarts()[seg * GetDim() + dim];
            if (start != -1) {
                if (seg == startSeg  && (!IsSetStrands() ||
                    GetStrands()[seg * GetDim() + dim] == eNa_strand_plus)) {
                    start += startOffset;
                }
                if (seg == stopSeg  && IsSetStrands() &&
                    GetStrands()[seg * GetDim() + dim] == eNa_strand_minus) {
                    start += stopOffset;
                }
            }
            ds->SetStarts().push_back(start);
        }

        //len
        TSeqPos len = GetLens()[seg];
        if (seg == startSeg) {
            len -= startOffset;
        }
        if (seg == stopSeg) {
            len -= stopOffset;
        }
        ds->SetLens().push_back(len);

        //strands
        if (IsSetStrands()) {
            for (CDense_seg::TDim dim = 0;  dim < GetDim();  ++dim) {
                ds->SetStrands().push_back(GetStrands()[seg * GetDim() + dim]);
            }
        }
        ++ds->SetNumseg();
    }

#ifdef _DEBUG
    ds->Validate(true);
#endif
    return ds;
}



void CDense_seg::OffsetRow(TDim row,
                           TSignedSeqPos offset)
{
    if (offset == 0) return;

    // Check for out-of-range negative offset
    if (offset < 0) {
        for (TNumseg seg = 0, pos = row;
             seg < GetNumseg();
             ++seg, pos += GetDim()) {

            if (GetStarts()[pos] >= 0) {
                if (GetStarts()[pos] < -offset) {
                    NCBI_THROW(CSeqalignException, eOutOfRange,
                               "Negative offset greater than seq position");
                }
            }
        }
    }

    // Modify positions
    for (TNumseg seg = 0, pos = row;
         seg < GetNumseg();
         ++seg, pos += GetDim()) {
        if (GetStarts()[pos] >= 0) {
            SetStarts()[pos] += offset;
        }
    }
}


/// @deprecated
void CDense_seg::RemapToLoc(TDim row, const CSeq_loc& loc,
                            bool ignore_strand)
{
    if (loc.IsWhole()) {
        return;
    }

    TSeqPos row_stop  = GetSeqStop(row);

    size_t  ttl_loc_len = 0;
    {{
        CSeq_loc_CI seq_loc_i(loc);
        do {
            ttl_loc_len += seq_loc_i.GetRange().GetLength();
        } while (++seq_loc_i);
    }}

    // check the validity of the seq-loc
    if (ttl_loc_len < row_stop + 1) {
        string errstr("CDense_seg::RemapToLoc():"
                      " Seq-loc is not long enough to"
                      " cover the alignment!"
                      " Maximum row seq pos is ");
        errstr += NStr::IntToString(row_stop);
        errstr += ". The total seq-loc len is only ";
        errstr += NStr::IntToString(ttl_loc_len);
        errstr += ", it should be at least ";
        errstr += NStr::IntToString(row_stop+1);
        errstr += " (= max seq pos + 1).";
        NCBI_THROW(CSeqalignException, eOutOfRange, errstr);
    }

    const CDense_seg::TStarts&  starts  = GetStarts();
    const CDense_seg::TStrands& strands = GetStrands();
    const CDense_seg::TLens&    lens    = GetLens();

    const size_t& numrows = CheckNumRows();
    const size_t& numsegs = CheckNumSegs();

    CSeq_loc_CI seq_loc_i(loc);

    TSeqPos start, loc_len, len, len_so_far;
    start = seq_loc_i.GetRange().GetFrom();
    len = loc_len = seq_loc_i.GetRange().GetLength();
    len_so_far = 0;
    
    bool row_plus = !strands.size() || strands[row] != eNa_strand_minus;
    bool loc_plus = seq_loc_i.GetStrand() != eNa_strand_minus;

    // iterate through segments
    size_t  idx = loc_plus ? row : (numsegs - 1) * numrows + row;
    TNumseg seg = loc_plus ? 0 : numsegs - 1;
    while (loc_plus ? seg < GetNumseg() : seg >= 0) {
        if (starts[idx] == -1) {
            // ignore gaps in our sequence
            if (loc_plus) {
                idx += numrows; seg++;
            } else {
                idx -= numrows; seg--;
            }
            continue;
        }

        // iterate the seq-loc if needed
        if ((loc_plus == row_plus ?
             starts[idx] : ttl_loc_len - starts[idx] - lens[seg])
            > len_so_far + loc_len) {

            if (++seq_loc_i) {
                len_so_far += len;
                len   = seq_loc_i.GetRange().GetLength();
                start = seq_loc_i.GetRange().GetFrom();
            } else {
                NCBI_THROW(CSeqalignException, eInvalidInputData,
                           "CDense_seg::RemapToLoc():"
                           " Internal error");
            }

            // assert the strand is the same
            if (loc_plus != (seq_loc_i.GetStrand() != eNa_strand_minus)) {
                NCBI_THROW(CSeqalignException, eInvalidInputData,
                           "CDense_seg::RemapToLoc():"
                           " The strand should be the same accross"
                           " the input seq-loc");
            }
        }

        // offset for the starting position
        if (loc_plus == row_plus) {
            SetStarts()[idx] += start - len_so_far;
        } else {
            SetStarts()[idx] = 
                start - len_so_far + ttl_loc_len - starts[idx] - lens[seg];
        }

        if (lens[seg] > len) {
            TSignedSeqPos len_diff = lens[seg] - len;
            while (1) {
                // move to the next loc part that extends beyond our length
                ++seq_loc_i;
                if (seq_loc_i) {
                    start = seq_loc_i.GetRange().GetFrom();
                } else {
                    NCBI_THROW(CSeqalignException, eOutOfRange,
                               "CDense_seg::RemapToLoc():"
                               " Internal error");
                }

                // split our segment
                SetLens().insert(SetLens().begin() + 
                                 (loc_plus ? seg : seg + 1),
                                 len);
                SetLens()[loc_plus ? seg + 1 : seg] = len_diff;

                // insert new data to account for our split segment
                TStarts temp_starts(numrows, -1);
                for (size_t row_i = 0, tmp_idx = seg * numrows;
                     row_i < numrows;  ++row_i, ++tmp_idx) {
                    TSignedSeqPos& this_start = SetStarts()[tmp_idx];
                    if (this_start != -1) {
                        temp_starts[row_i] = this_start;
                        if (loc_plus == (strands[row_i] != eNa_strand_minus)) {
                            if ((size_t) row == row_i) {
                                temp_starts[row_i] = start;
                            } else {
                                temp_starts[row_i] += len;
                            }
                        } else {
                            this_start += len_diff;
                        }
                    }
                }

                len_so_far += loc_len;
                len = loc_len = seq_loc_i.GetRange().GetLength();

                SetStarts().insert(SetStarts().begin() +
                                   (loc_plus ? seg + 1 : seg) * numrows,
                                   temp_starts.begin(), temp_starts.end());
                
                if (strands.size()) {
                    SetStrands().insert
                        (SetStrands().begin(),
                         strands.begin(), strands.begin() + numrows);
                }

                SetNumseg()++;
                
                if ((len_diff = lens[seg] - len) > 0) {
                    if (loc_plus) {
                        idx += numrows; seg++;
                    } else {
                        idx -= numrows; seg--;
                    }
                } else {
                    break;
                }
            }
        } else {
            len -= lens[seg];
        }

        if (loc_plus) {
            idx += numrows; seg++;
        } else {
            idx -= numrows; seg--;
        }
    } // while iterating through segments
    
    // finally, modify the strands if different
    if ( !ignore_strand ) {
        if (loc_plus != row_plus) {
            if (!strands.size()) {
                // strands do not exist, create them
                SetStrands().resize(GetNumseg() * GetDim(), eNa_strand_plus);
            }
            for (seg = 0, idx = row;
                 seg < GetNumseg(); seg++, idx += numrows) {
                SetStrands()[idx] = loc_plus ? eNa_strand_plus : eNa_strand_minus;
            }
        }
    }

}


CRef<CDense_seg> CDense_seg::FillUnaligned()
{
    // this dense-seg
    const CDense_seg::TStarts&  starts  = GetStarts();
    const CDense_seg::TStrands& strands = GetStrands();
    const CDense_seg::TLens&    lens    = GetLens();
    const CDense_seg::TIds&     ids     = GetIds();

    const size_t& numrows = CheckNumRows();
    const size_t& numsegs = CheckNumSegs();

    bool strands_exist = !strands.empty();

    size_t seg = 0, row = 0;
    

    // extra segments
    CDense_seg::TStarts extra_starts;
    CDense_seg::TLens   extra_lens;
    size_t extra_numsegs = 0;
    size_t extra_seg = 0;
    vector<size_t> extra_segs;


    // new dense-seg
    CRef<CDense_seg> new_ds(new CDense_seg);
    CDense_seg::TStarts&  new_starts  = new_ds->SetStarts();
    CDense_seg::TStrands& new_strands = new_ds->SetStrands();
    CDense_seg::TLens&    new_lens    = new_ds->SetLens();
    CDense_seg::TIds&     new_ids     = new_ds->SetIds();

    // dimentions
    new_ds->SetDim(numrows);
    TNumseg& new_numsegs = new_ds->SetNumseg();
    new_numsegs = numsegs; // initialize

    // ids
    new_ids.resize(numrows);
    for (row = 0; row < numrows; row++) {
        CRef<CSeq_id> id(new CSeq_id);
        SerialAssign(*id, *ids[row]);
        new_ids[row] = id;
    }

    size_t new_seg = 0;
    
    // temporary data
    vector<TSignedSeqPos> expected_positions;
    expected_positions.resize(numrows, -1);

    vector<bool> plus;
    plus.resize(numrows, true);
    if (strands_exist) {
        for (row = 0; row < numrows; row++) {
            if (strands[row] == eNa_strand_minus) {
                plus[row] = false;
            }
        }
    }
    
    TSignedSeqPos extra_len = 0;
    size_t idx = 0, new_idx = 0, extra_idx = 0;

    // main loop through segments
    for (seg = 0; seg < numsegs; seg++) {
        const TSeqPos& len = lens[seg];
        for (row = 0; row < numrows; row++) {
            
            const TSignedSeqPos& start = starts[idx++];
 
            TSignedSeqPos& expected_pos = expected_positions[row];

            if (start >= 0) {

                if (expected_pos >= 0) {
                    // check if there's an unaligned insert

                    if (plus[row]) {
                        extra_len = start - expected_pos;
                    } else {
                        extra_len = expected_pos - start - len;
                    }

                    if (extra_len < 0) {
                        string errstr("CDense_seg::AddUnalignedSegments():"
                                      " Illegal overlap at Row ");
                        errstr += NStr::IntToString(row);
                        errstr += " Segment ";
                        errstr += NStr::IntToString(seg);
                        errstr += ".";
                        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                                   errstr);
                    } else if (extra_len > 0) {
                        // insert new segment
                        extra_segs.push_back(seg);
                        extra_lens.push_back(extra_len);
                        extra_starts.resize(extra_idx + numrows, -1);
                        extra_starts[extra_idx + row] = 
                            plus[row] ? expected_pos : start + len;

                        extra_idx += numrows;
                        ++extra_numsegs;
                    }
                }

                // set the new expected_pos
                if (plus[row]) {
                    expected_pos = start + len;
                } else {
                    expected_pos = start;
                }
            }
        }
    }

    // lens & starts
    new_numsegs = numsegs + extra_numsegs;
    new_lens.resize(numsegs + extra_numsegs);
    new_starts.resize(numrows * new_numsegs);
    for (seg = 0, new_seg = 0, extra_seg = 0,
             idx = 0, new_idx = 0, extra_idx = 0;
         seg < numsegs;
         seg++, new_seg++) {

        // insert extra segments
        if (extra_numsegs) {
            while (extra_segs[extra_seg] == seg) {
                new_lens[new_seg++] = extra_lens[extra_seg++];
                for (row = 0; row < numrows; row++) {
                    new_starts[new_idx++] = extra_starts[extra_idx++];
                }
            }   
        }

        // add the existing segment
        new_lens[new_seg] = lens[seg];
        for (row = 0; row < numrows; row++) {
            new_starts[new_idx++] = starts[idx++];
        }
    }
            
 
    // strands
    new_strands.resize(numrows * new_numsegs, eNa_strand_plus);
    if (strands_exist) {
        new_idx = 0;
        for (new_seg = 0; new_seg < new_numsegs; new_seg++) {
            for (row = 0; row < numrows; row++, new_idx++) {
                if ( !plus[row] ) {
                    new_strands[new_idx] = eNa_strand_minus;
                }
            }
        }
    }

    return new_ds;
}


//-----------------------------------------------------------------------------
// PRE : alignment transcript (RLE or not) and start coordinates
// POST: Starts, lens and strands. Ids and scores not affected.

// initialize from pairwise alignment transcript
void CDense_seg::FromTranscript(TSeqPos query_start, ENa_strand query_strand,
                                TSeqPos subj_start, ENa_strand subj_strand,
                                const string& transcript )
{
    // check that strands are specific
    bool query_strand_specific = 
        query_strand == eNa_strand_plus || query_strand == eNa_strand_minus;
    bool subj_strand_specific = 
        subj_strand == eNa_strand_plus || subj_strand == eNa_strand_minus;

    if(!query_strand_specific || !subj_strand_specific) {
        NCBI_THROW(CSeqalignException, eInvalidInputData, "Unknown strand");
    }

    TStarts &starts  = SetStarts();
    starts.clear();
    TLens &lens    = SetLens();
    lens.clear();
    TStrands &strands = SetStrands();
    strands.clear();

    SetDim(2);

    // iterate through the transcript
    size_t seg_count = 0;

    size_t start1 = 0, pos1 = 0; // relative to exon start in mrna
    size_t start2 = 0, pos2 = 0; // and genomic
    size_t seg_len = 0;
	
    string::const_iterator ib = transcript.begin(),
        ie = transcript.end(), ii = ib;
    unsigned char seg_type;
    const static char badsymerr[] = "Unknown or unsupported transcript symbol";
    char c = *ii++;
    if(c == 'M' || c == 'R') {
        seg_type = 0;
        ++pos1;
        ++pos2;
    }
    else if (c == 'I') {
        seg_type = 1;
        ++pos2;
    }
    else if (c == 'D') {
        seg_type = 2;
        ++pos1;
    }
    else {

        NCBI_THROW(CSeqalignException, eInvalidInputData, badsymerr);
    }    
    
    while(ii < ie) {

        c = *ii;
        if(isalpha((unsigned char) c)) {

            if(seg_type == 0 && (c == 'M' || c == 'R')) {
                ++pos1;
                ++pos2;
            }
            else if(seg_type == 1 && c == 'I') {
                ++pos2;
            }
            else if(seg_type == 2 && c == 'D') {
                ++pos1;
            }
            else {
                
                // close current seg
                TSeqPos query_close = query_strand == eNa_strand_plus?
                    start1: 1 - pos1;
                starts.push_back(seg_type == 1? -1: query_start + query_close);
                strands.push_back(query_strand);
                
                TSeqPos subj_close = subj_strand == eNa_strand_plus?
                    start2: 1- pos2;
                starts.push_back(seg_type == 2? -1: subj_start + subj_close);
                strands.push_back(subj_strand);
                
                switch(seg_type) {
                case 0: seg_len = pos1 - start1; break;
                case 1: seg_len = pos2 - start2; break;
                case 2: seg_len = pos1 - start1; break;
                }
                lens.push_back(seg_len);
                ++seg_count;
                
                // start a new seg
                start1 = pos1;
                start2 = pos2;
                
                if(c == 'M' || c == 'R'){
                    seg_type = 0; // matches and mismatches
                    ++pos1;
                    ++pos2;
                }
                else if (c == 'I') {
                    seg_type = 1;  // inserts
                    ++pos2;
                }
                else  if (c == 'D') {
                    seg_type = 2;  // dels
                    ++pos1;
                }
                else {

                    NCBI_THROW(CSeqalignException, eInvalidInputData,
                               badsymerr);
                }
            }
            ++ii;
        }
        else {

            if(!isdigit((unsigned char) c)) {

                NCBI_THROW(CSeqalignException, eInvalidInputData,
                           "Alignment transcript corrupt");
            }

            size_t len = 0;
            while(ii < ie && isdigit((unsigned char)(*ii))) {
                len = 10*len + *ii - '0';
                ++ii;
            }
            --len;
            switch(seg_type) {
            case 0: pos1 += len; pos2 += len; break;
            case 1: pos2 += len; break;
            case 2: pos1 += len; break;
            }
        }
    }
    
    TSeqPos query_close = query_strand == eNa_strand_plus? start1: 1 - pos1;
    starts.push_back(seg_type == 1? -1: query_start + query_close);
    strands.push_back(query_strand);
    
    TSeqPos subj_close = subj_strand == eNa_strand_plus? start2: 1 - pos2;
    starts.push_back(seg_type == 2? -1: subj_start + subj_close);
    strands.push_back(subj_strand);
    
    switch(seg_type) {

    case 0: seg_len = pos1 - start1; break;
    case 1: seg_len = pos2 - start2; break;
    case 2: seg_len = pos1 - start1; break;
    }
    lens.push_back(seg_len);
    ++seg_count;
    
    SetNumseg(seg_count);
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 64, chars: 1885, CRC32: 4483973b */
