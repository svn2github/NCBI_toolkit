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
 */

/// @file MSRequest.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'omssa.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: MSRequest_.hpp


#ifndef OBJECTS_OMSSA_MSREQUEST_HPP
#define OBJECTS_OMSSA_MSREQUEST_HPP


// generated includes
#include <objects/omssa/MSRequest_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class NCBI_OMSSA_EXPORT CMSRequest : public CMSRequest_Base
{
    typedef CMSRequest_Base Tparent;
public:
    // constructor
    CMSRequest(void);
    // destructor
    ~CMSRequest(void);

    /**
     * return a settings by settingid
     * 
     * @param Id setting id
     * @return setting or null if no settings found
     */
    CConstRef <TSettings> GetSettingsById(const int Id) const;

private:
    // Prohibit copy constructor and assignment operator
    CMSRequest(const CMSRequest& value);
    CMSRequest& operator=(const CMSRequest& value);

};

/////////////////// CMSRequest inline methods

// constructor
inline
CMSRequest::CMSRequest(void)
{
}


/////////////////// end of CMSRequest inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2005/11/07 19:57:20  lewisg
* iterative search
*
*
* ===========================================================================
*/

#endif // OBJECTS_OMSSA_MSREQUEST_HPP
/* Original file checksum: lines: 94, chars: 2571, CRC32: dd4a491 */
