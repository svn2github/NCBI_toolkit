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
 *   'seq.asn'.
 */

#ifndef OBJECTS_SEQ_MOLINFO_HPP
#define OBJECTS_SEQ_MOLINFO_HPP


// generated includes
#include <objects/seq/MolInfo_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CMolInfo : public CMolInfo_Base
{
    typedef CMolInfo_Base Tparent;
public:
    // constructor
    CMolInfo(void);
    // destructor
    ~CMolInfo(void);

    // Append a label to label based on content
    void GetLabel(string* label) const;

private:
    // Prohibit copy constructor and assignment operator
    CMolInfo(const CMolInfo& value);
    CMolInfo& operator=(const CMolInfo& value);

};



/////////////////// CMolInfo inline methods

// constructor
inline
CMolInfo::CMolInfo(void)
{
}


/////////////////// end of CMolInfo inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.2  2002/10/03 19:05:23  clausen
* Removed extra whitespace
*
* Revision 1.1  2002/10/03 16:54:56  clausen
* Added GetLabel()
*
*
* ===========================================================================
*/

#endif // OBJECTS_SEQ_MOLINFO_HPP
/* Original file checksum: lines: 93, chars: 2350, CRC32: 19009c50 */
