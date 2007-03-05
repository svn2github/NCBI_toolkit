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
 *   'biblio.asn'.
 */

#ifndef OBJECTS_BIBLIO_CIT_GEN_HPP
#define OBJECTS_BIBLIO_CIT_GEN_HPP


// generated includes
#include <objects/biblio/Cit_gen_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class NCBI_BIBLIO_EXPORT CCit_gen : public CCit_gen_Base
{
    typedef CCit_gen_Base Tparent;
public:
    // constructor
    CCit_gen(void);
    // destructor
    ~CCit_gen(void);

    // Appends a label onto "label" based on content
    void GetLabel(string* label, bool unique = false) const;

private:
    // Prohibit copy constructor and assignment operator
    CCit_gen(const CCit_gen& value);
    CCit_gen& operator=(const CCit_gen& value);

};



/////////////////// CCit_gen inline methods

// constructor
inline
CCit_gen::CCit_gen(void)
{
}


/////////////////// end of CCit_gen inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#endif // OBJECTS_BIBLIO_CIT_GEN_HPP
/* Original file checksum: lines: 93, chars: 2380, CRC32: 90c2285b */
