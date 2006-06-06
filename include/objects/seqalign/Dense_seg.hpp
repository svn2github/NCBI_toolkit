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

#ifndef OBJECTS_SEQALIGN_DENSE_SEG_HPP
#define OBJECTS_SEQALIGN_DENSE_SEG_HPP


// generated includes
#include <objects/seqalign/Dense_seg_.hpp>

#include <objects/seqalign/seqalign_exception.hpp>
#include <util/range.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// forward declarations
class CSeq_loc;

class NCBI_SEQALIGN_EXPORT CDense_seg : public CDense_seg_Base
{
    typedef CDense_seg_Base Tparent;
public:
    // constructor
    CDense_seg(void);
    // destructor
    ~CDense_seg(void);

    /// overloaded Assign()
    void Assign(const CSerialObject& obj,
                ESerialRecursionMode how = eRecursive);

    typedef vector<int> TWidths;

    // optional
    // typedef vector<int> TWidths
    bool IsSetWidths(void) const;
    bool CanGetWidths(void) const;
    void ResetWidths(void);
    const TWidths& GetWidths(void) const;
    TWidths& SetWidths(void);

    // Validators
    TDim    CheckNumRows(void)                   const;
    TNumseg CheckNumSegs(void)                   const;
    void    Validate    (bool full_test = false) const;

    // GetSeqRange
    CRange<TSeqPos> GetSeqRange(TDim row) const;
    TSeqPos         GetSeqStart(TDim row) const;
    TSeqPos         GetSeqStop(TDim row) const;

    // Get strand (the first one if segments have different strands).
    ENa_strand      GetSeqStrand(TDim row) const;

    /// Reverse the segments' orientation
    void Reverse(void);

    /// Swap two rows (changing *order*, not content)
    void SwapRows(TDim row1, TDim row2);

    /// Create a new dense-seg with added all unaligned pieces
    /// (implicit inserts), if any, between segments.
    CRef<CDense_seg> FillUnaligned();

    /// Extract a slice of the alignment that includes the specified range
    CRef<CDense_seg> ExtractSlice(TDim row, TSeqPos from, TSeqPos to) const;

    /// Join adjacent mergeable segments to create a more compact
    /// alignment
    void Compact();

    /// Trim leading/training gaps if possible
    void TrimEndGaps();

    /// Offset row's coords
    void OffsetRow(TDim row, TSeqPos offset);

    /// @deprecated
    NCBI_DEPRECATED void RemapToLoc(TDim row, const CSeq_loc& loc,
                                    bool ignore_strand = false);

    // initialize from pairwise alignment transcript
    void FromTranscript(TSeqPos query_start, ENa_strand query_strand,
                        TSeqPos subj_start, ENa_strand subj_strand,
                        const string& transcript );

protected:
    TNumseg x_FindSegment(TDim row, TSignedSeqPos pos) const;


private:
    // Prohibit copy constructor and assignment operator
    CDense_seg(const CDense_seg& value);
    CDense_seg& operator=(const CDense_seg& value);

    // data
    Uint4 m_set_State1[1];
    TWidths m_Widths;
};



/////////////////// CDense_seg inline methods

// constructor
inline
CDense_seg::CDense_seg(void)
{
    memset(m_set_State1,0,sizeof(m_set_State1));
}


inline
bool CDense_seg::IsSetWidths(void) const
{
    return ((m_set_State1[0] & 0x3) != 0);
}


inline
bool CDense_seg::CanGetWidths(void) const
{
    return true;
}


inline
const vector<int>& CDense_seg::GetWidths(void) const
{
    return m_Widths;
}


inline
vector<int>& CDense_seg::SetWidths(void)
{
    m_set_State1[0] |= 0x1;
    return m_Widths;
}


inline
void CDense_seg::ResetWidths(void)
{
    m_Widths.clear();
    m_set_State1[0] &= ~0x3;

}


inline
CRange<TSeqPos> CDense_seg::GetSeqRange(TDim row) const
{
    return CRange<TSeqPos>(GetSeqStart(row), GetSeqStop(row));
}


inline
CDense_seg::TDim CDense_seg::CheckNumRows() const
{
    const size_t& dim = GetDim();
    if (dim != GetIds().size()) {
        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CDense_seg::CheckNumRows()"
                   " ids.size is inconsistent with dim");
    }
    return dim;
}


/////////////////// end of CDense_seg inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.17  2006/06/06 22:44:47  todorov
* Added OffsetRow method.
* Marked RemapToLoc for deprecation.
*
* Revision 1.16  2006/03/15 15:49:03  dicuccio
* Added TrimEndGaps()
*
* Revision 1.15  2005/11/22 18:47:37  dicuccio
* Restore default parameter for Assign()
*
* Revision 1.14  2005/11/22 18:15:05  dicuccio
* Added Assign()
*
* Revision 1.13  2005/03/02 14:58:58  dicuccio
* Added CDense_seg::Compact(): merges abutting segments where appropriate
*
* Revision 1.12  2004/10/21 01:37:58  kapustin
* +FromTranscript
*
* Revision 1.11  2004/07/01 20:35:49  todorov
* + FillUnaligned()
*
* Revision 1.10  2004/06/29 18:42:59  johnson
* +ExtractSlice
*
* Revision 1.9  2004/06/14 22:09:02  johnson
* Added GetSeqStrand method (analogous to GetSeq_id)
*
* Revision 1.8  2004/05/06 18:23:35  todorov
* + optional ignore_strand param to RemapToLoc
*
* Revision 1.7  2004/04/09 14:31:35  dicuccio
* Back out previous commit - don't automatically set bit 1 and 2 in Set()
*
* Revision 1.6  2004/04/09 14:26:46  dicuccio
* FIxed bug in SetWidths(): use the correct bit to indicate set state
*
* Revision 1.5  2003/11/04 14:46:16  todorov
* +RemapToLoc
*
* Revision 1.4  2003/09/16 15:31:59  todorov
* Added validation methods. Added seq range methods
*
* Revision 1.3  2003/08/26 20:28:38  johnson
* added 'SwapRows' method
*
* Revision 1.2  2003/08/14 19:52:53  todorov
* Added m_Widths to support nuc2prot aligns
*
* Revision 1.1  2003/08/13 18:11:35  johnson
* added 'Reverse' method
*
*
* ===========================================================================
*/

#endif // OBJECTS_SEQALIGN_DENSE_SEG_HPP
/* Original file checksum: lines: 93, chars: 2426, CRC32: 48766ca1 */
