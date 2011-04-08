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
 *   'pub.asn'.
 */

#ifndef OBJECTS_PUB_PUB_HPP
#define OBJECTS_PUB_PUB_HPP


// generated includes
#include <objects/pub/Pub_.hpp>

#include <objects/biblio/citation_base.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CAuth_list;

class NCBI_PUB_EXPORT CPub : public CPub_Base, public IAbstractCitation
{
    typedef CPub_Base Tparent;
public:
    // constructor
    CPub(void);
    // destructor
    ~CPub(void);
    
    enum ELabelType {
        eType,
        eContent,
        eBoth};
     
    /// Concatenate a label for this pub to label. If flags contains
    /// fCitLabel_Unique, add a unique tag to the end of the
    /// concatenated label.
    bool GetLabel(string*          label, 
                  ELabelType       type    = eContent,
                  TLabelFlags      flags   = 0,
                  ELabelVersion    version = eLabel_DefaultVersion) const;

    bool GetLabel(string*          label,
                  ELabelType       type,
                  bool             unique = false) const;

    bool GetLabel(string* label, TLabelFlags flags = 0,
                  ELabelVersion version = eLabel_DefaultVersion) const;

    // convenience functions to get author list from underlying pub
    bool IsSetAuthors(void) const;
    const CAuth_list& GetAuthors(void) const;


protected:
  
	private:
    // Prohibit copy constructor and assignment operator
    CPub(const CPub& value);
    CPub& operator=(const CPub& value);

};



/////////////////// CPub inline methods

// constructor
inline
CPub::CPub(void)
{
}

inline
bool CPub::GetLabel(string* label, ELabelType type, bool unique) const
{
    return GetLabel(label, type, unique ? fLabel_Unique : 0);
}

inline
bool CPub::GetLabel(string* label, TLabelFlags flags, ELabelVersion version)
    const
{
    return GetLabel(label, eContent, flags, version);
}


/////////////////// end of CPub inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_PUB_PUB_HPP
/* Original file checksum: lines: 90, chars: 2283, CRC32: 573f38c8 */
