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
 *   'biblio.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 1.2  2002/01/16 18:56:22  grichenk
 * Removed CRef<> argument from choice variant setter, updated sources to
 * use references instead of CRef<>s
 *
 * Revision 1.1  2002/01/10 20:09:03  clausen
 * Added GetLabel
 *
 *
 * ===========================================================================
 */

#ifndef OBJECTS_BIBLIO_CIT_BOOK_HPP
#define OBJECTS_BIBLIO_CIT_BOOK_HPP


// generated includes
#include <objects/biblio/Cit_book_.hpp>
#include <objects/pub/Pub.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CCit_book : public CCit_book_Base
{
    typedef CCit_book_Base Tparent;
public:
    // constructor
    CCit_book(void);
    // destructor
    ~CCit_book(void);
    
    // Appends a label to "label" based on content
    void GetLabel(string* label) const;

private:
    // Prohibit copy constructor and assignment operator
    CCit_book(const CCit_book& value);
    CCit_book& operator=(const CCit_book& value);

};



/////////////////// CCit_book inline methods

// constructor
inline
CCit_book::CCit_book(void)
{
}

inline
void CCit_book::GetLabel(string* label) const
{
    // Wrap CCit_book in CPub and call CPub::GetLabel()
    CPub pub;
    pub.SetBook(const_cast<CCit_book&>(*this));
    pub.GetLabel(label);
}
/////////////////// end of CCit_book inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_BIBLIO_CIT_BOOK_HPP
/* Original file checksum: lines: 90, chars: 2383, CRC32: 6b7b4d6 */
