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

/// @OrgMod.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'seqfeat.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: OrgMod_.hpp


#ifndef OBJECTS_SEQFEAT_ORGMOD_HPP
#define OBJECTS_SEQFEAT_ORGMOD_HPP


// generated includes
#include <objects/seqfeat/OrgMod_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class NCBI_SEQFEAT_EXPORT COrgMod : public COrgMod_Base
{
    typedef COrgMod_Base Tparent;
public:
    // constructor
    COrgMod(void);
    COrgMod(TSubtype subtype, const TSubname& subname);
    COrgMod(const string& subtype, const TSubname& subname);
    // destructor
    ~COrgMod(void);

    // Find the enumerated subtype value.
    // does case-insesitive search and '_' are converted to '-'.
    // Throws an exception on failure.
    static TSubtype GetSubtypeValue(const string& str);
    static string GetSubtypeName(TSubtype stype);

	static bool ParseStructuredVoucher(const string& str, string& inst, string& coll, string& id);
  static bool IsInstitutionCodeValid(const string& inst_coll, string &voucher_type, bool& is_miscapitalized, string& correct_cap, bool& needs_country);

	private:
    // Prohibit copy constructor and assignment operator
    COrgMod(const COrgMod& value);
    COrgMod& operator=(const COrgMod& value);

};

/////////////////// COrgMod inline methods

// constructor
inline
COrgMod::COrgMod(void)
{
}

inline
COrgMod::COrgMod(TSubtype subtype, const TSubname& subname)
{
    SetSubtype(subtype);
    SetSubname(subname);
}


inline
COrgMod::COrgMod(const string& subtype, const TSubname& subname)
{
    SetSubtype(GetSubtypeValue(subtype));
    SetSubname(subname);
}

/////////////////// end of COrgMod inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

#endif // OBJECTS_SEQFEAT_ORGMOD_HPP
/* Original file checksum: lines: 94, chars: 2521, CRC32: 79cf88de */
