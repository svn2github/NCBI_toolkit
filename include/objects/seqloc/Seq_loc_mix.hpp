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
 *   using specifications from the ASN data definition file
 *   'seqloc.asn'.
 */

#ifndef OBJECTS_SEQLOC_SEQ_LOC_MIX_HPP
#define OBJECTS_SEQLOC_SEQ_LOC_MIX_HPP

// generated includes
#include <objects/seqloc/Seq_loc_mix_.hpp>

#include <objects/seqloc/Na_strand.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CSeq_id;


class NCBI_SEQLOC_EXPORT CSeq_loc_mix : public CSeq_loc_mix_Base
{
    typedef CSeq_loc_mix_Base Tparent;
public:
    // constructor
    CSeq_loc_mix(void);
    // destructor
    ~CSeq_loc_mix(void);
    
    //
    // See related function in util/sequence.hpp:
    //
    //   TSeqPos GetLength(const CSeq_loc_mix&, CScope*)
    //

    // check left (5') or right (3') end of location for e_Lim fuzz
    bool IsPartialLeft  (void) const;
    bool IsPartialRight (void) const;

    // set / remove e_Lim fuzz on left (5') or right (3') end
    void SetPartialLeft (bool val);
    void SetPartialRight(bool val);

    // Add a Seq-loc to the mix.
    // NB: This is just a structural change, no guarantees as to the biological
    // validity of the data are made.
    // See sequence::MergeLocations(..) for context aware function.
    void AddSeqLoc(const CSeq_loc& other);
    void AddSeqLoc(CSeq_loc& other);
    // convenience function; automatically optimizes down points
    void AddInterval(const CSeq_id& id, TSeqPos from, TSeqPos to,
                     ENa_strand strand = eNa_strand_unknown);
        
    bool IsReverseStrand(void) const;
    TSeqPos GetStart(TSeqPos circular_length = kInvalidSeqPos) const;
    TSeqPos GetEnd(TSeqPos circular_length = kInvalidSeqPos) const;

private:
    // Prohibit copy constructor & assignment operator
    CSeq_loc_mix(const CSeq_loc_mix&);
    CSeq_loc_mix& operator= (const CSeq_loc_mix&);

};



/////////////////// CSeq_loc_mix inline methods

/////////////////// end of CSeq_loc_mix inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/*
 * ===========================================================================
 *
 * $Log$
 * Revision 1.17  2004/09/01 15:33:44  grichenk
 * Check strand in GetStart and GetEnd. Circular length argument
 * made optional.
 *
 * Revision 1.16  2004/08/19 13:05:36  dicuccio
 * Added missing predeclaration for CSeq_id
 *
 * Revision 1.15  2004/05/06 16:54:41  shomrat
 * Added methods to set partial left and right
 *
 * Revision 1.14  2004/01/28 17:15:37  shomrat
 * Added methods to ease the construction of objects
 *
 * Revision 1.13  2003/05/23 20:23:57  ucko
 * +AddInterval (also handles points automatically); CVS log -> end
 *
 * Revision 1.12  2002/12/26 12:43:42  dicuccio
 * Added Win32 export specifiers
 *
 * Revision 1.11  2002/09/12 21:15:11  kans
 * added IsPartialLeft and IsPartialRight
 *
 * Revision 1.10  2002/06/07 11:47:38  clausen
 * Added relate function comment
 *
 * Revision 1.9  2002/06/06 20:52:24  clausen
 * Moved GetLength to objects/util/sequence.cpp
 *
 * Revision 1.8  2002/05/03 21:28:04  ucko
 * Introduce T(Signed)SeqPos.
 *
 * Revision 1.7  2002/04/22 20:09:06  grichenk
 * -GetTotalRange(), GetRangeMap(), ResetRangeMap()
 *
 * Revision 1.6  2001/08/24 13:45:01  grichenk
 * included <memory>
 *
 * Revision 1.5  2001/08/23 22:33:49  vakatov
 * + <memory>
 *
 * Revision 1.4  2001/06/25 18:52:02  grichenk
 * Prohibited copy constructor and assignment operator
 *
 * Revision 1.3  2001/01/05 20:11:42  vasilche
 * CRange, CRangeMap were moved to util.
 *
 * Revision 1.2  2001/01/03 16:38:59  vasilche
 * Added CAbstractObjectManager - stub for object manager.
 * CRange extracted to separate file.
 *
 * Revision 1.1  2000/11/17 21:35:03  vasilche
 * Added GetLength() method to CSeq_loc class.
 *
 * ===========================================================================
 */

#endif // OBJECTS_SEQLOC_SEQ_LOC_MIX_HPP
