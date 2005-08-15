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
 * Author:  Aaron Ucko, NCBI
 *
 * File Description:
 *   Simple utilities for deducing directions
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the data definition file
 *   'seqloc.asn'.
 */

#ifndef OBJECTS_SEQLOC_NA_STRAND_HPP
#define OBJECTS_SEQLOC_NA_STRAND_HPP


// generated includes
#include <objects/seqloc/Na_strand_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/// Used to determine the meaning of a location's Start/Stop positions.
/// On the minus strand the numerical values are different than the 
/// biological ones.
/// @sa
///   CSeq_loc::GetStart(), CSeq_loc::GetStop()
enum ESeqLocExtremes {
    eExtreme_Biological,   ///< 5' and 3'
    eExtreme_Positional    ///< numerical value
};


inline
bool IsForward(ENa_strand s)
{
    return (s == eNa_strand_plus  ||  s == eNa_strand_both);
}


inline
bool IsReverse(ENa_strand s)
{
    // treat unknown as forward
    return (s == eNa_strand_minus  ||  s == eNa_strand_both_rev);
}


inline
bool SameOrientation(ENa_strand a, ENa_strand b)
{
    return IsReverse(a) == IsReverse(b);
}


inline
ENa_strand Reverse(ENa_strand s)
{
    switch ( s ) {
    case eNa_strand_unknown:  // defaults to plus
    case eNa_strand_plus:
        return eNa_strand_minus;
    case eNa_strand_minus:
        return eNa_strand_plus;
    case eNa_strand_both:
        return eNa_strand_both_rev;
    case eNa_strand_both_rev:
        return eNa_strand_both;
    default:
        return s;
    }
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.8  2005/08/15 16:24:18  vakatov
* DOXY fix
*
* Revision 1.7  2005/08/15 16:02:48  shomrat
* Added explanation for ESeqLocExtremes
*
* Revision 1.6  2005/02/18 14:58:34  shomrat
* + ESeqLocExtremes
*
* Revision 1.5  2004/08/16 17:59:26  grichenk
* Added IsForward()
*
* Revision 1.4  2004/02/19 18:00:50  shomrat
* changed logic to match C toolkit
*
* Revision 1.3  2003/12/18 02:55:52  ucko
* Rework Reverse to avoid warnings about unhandled cases.
*
* Revision 1.2  2003/08/27 14:20:27  vasilche
* Added Reverse(ENa_strand).
*
* Revision 1.1  2002/11/12 19:53:25  ucko
* Add simple utilities to distinguish forward- and reverse-orientation strands.
*
*
* ===========================================================================
*/

#endif // OBJECTS_SEQLOC_NA_STRAND_HPP
/* Original file checksum: lines: 63, chars: 1928, CRC32: 1071d9d3 */
