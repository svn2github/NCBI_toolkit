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
 *   'general.asn'.
 *
 * ===========================================================================
 */

#ifndef OBJECTS_GENERAL_INT_FUZZ_HPP
#define OBJECTS_GENERAL_INT_FUZZ_HPP


// generated includes
#include <objects/general/Int_fuzz_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// Forward declarations
class CSeq_point;
class CInt_fuzz : public CInt_fuzz_Base
{
    typedef CInt_fuzz_Base Tparent;
public:
    // constructor
    CInt_fuzz(void);
    // destructor
    ~CInt_fuzz(void);

    void GetLabel(string* label, TSeqPos pos, bool right = true) const;

private:
    // Prohibit copy constructor and assignment operator
    CInt_fuzz(const CInt_fuzz& value);
    CInt_fuzz& operator=(const CInt_fuzz& value);

};



/////////////////// CInt_fuzz inline methods

// constructor
inline
CInt_fuzz::CInt_fuzz(void)
{
}


/////////////////// end of CInt_fuzz inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.2  2002/10/03 19:02:27  clausen
 * Removed extra whitespace
 *
 * Revision 1.1  2002/10/03 16:42:36  clausen
 * First version
 *
 *
 * ===========================================================================
*/


#endif // OBJECTS_GENERAL_INT_FUZZ_HPP
/* Original file checksum: lines: 90, chars: 2388, CRC32: e4127209 */
