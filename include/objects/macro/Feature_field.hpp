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

/// @file Feature_field.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'macro.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Feature_field_.hpp


#ifndef OBJECTS_MACRO_FEATURE_FIELD_HPP
#define OBJECTS_MACRO_FEATURE_FIELD_HPP


// generated includes
#include <objects/macro/Feature_field_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class CFeature_field : public CFeature_field_Base
{
    typedef CFeature_field_Base Tparent;
public:
    // constructor
    CFeature_field(void);
    // destructor
    ~CFeature_field(void);

/*
    bool Match(const CSeq_feat& feat, 
           const CString_constraint& str_cons, CConstRef <CScope> scope) const;
*/

private:
    // Prohibit copy constructor and assignment operator
    CFeature_field(const CFeature_field& value);
    CFeature_field& operator=(const CFeature_field& value);

/*
    string x_GetQualFromFeature(const CSeq_feat& feat, 
                                 const CString_constraint& str_cons, 
                                 CConstRef <CScope> scope) const;
*/
    CSeqFeatData::ESubtype x_GetFeatSubtype() const;
};

/////////////////// CFeature_field inline methods

// constructor
inline
CFeature_field::CFeature_field(void)
{
}


/////////////////// end of CFeature_field inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_MACRO_FEATURE_FIELD_HPP
/* Original file checksum: lines: 86, chars: 2462, CRC32: 3a309e4a */
