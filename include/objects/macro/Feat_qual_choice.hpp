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

/// @file Feat_qual_choice.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'macro.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: Feat_qual_choice_.hpp


#ifndef OBJECTS_MACRO_FEAT_QUAL_CHOICE_HPP
#define OBJECTS_MACRO_FEAT_QUAL_CHOICE_HPP


// generated includes
#include <objects/macro/Feat_qual_choice_.hpp>
#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/Genetic_code.hpp>
#include <objmgr/util/sequence.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seq_vector.hpp>
#include <objtools/format/flat_file_config.hpp>
#include <objtools/format/context.hpp>
#include <objtools/format/items/qualifiers.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

/////////////////////////////////////////////////////////////////////////////
class CFeat_qual_choice : public CFeat_qual_choice_Base
{
    typedef CFeat_qual_choice_Base Tparent;
public:
    // constructor
    CFeat_qual_choice(void);
    // destructor
    ~CFeat_qual_choice(void);
 
    string GetQualFromFeatureAnyType(const CSeq_feat& feat, 
                                        const CString_constraint& str_cons,
                                        CConstRef <CScope> scope) const;
    static string GetFirstStringMatch(const list <string>& strs, 
                                     const CString_constraint& str_cons);
    static string GetFirstStringMatch(const vector <string>& strs, 
                                     const CString_constraint& str_cons);
private:
    // Prohibit copy constructor and assignment operator
    CFeat_qual_choice(const CFeat_qual_choice& value);
    CFeat_qual_choice& operator=(const CFeat_qual_choice& value);

    string x_GetLegalQualName(vector <EFeat_qual_legal>& v_qual, 
                                 EFeat_qual_legal qual) const;
    void x_GetTwoFieldSubfield(string& str, int subfield) const;
    string x_GetFirstGBQualMatch(const vector <CRef <CGb_qual> >& quals, 
                                  const string& qual_name, 
                                  int subfield, 
                                  const CString_constraint& str_cons) const;
    string x_GetQualViaFeatSeqTableColumn(const string& name, 
                                           const CSeq_feat& feat, 
                                           const CString_constraint& str_cons,
                                           CConstRef <CScope> scope) const;
    string x_GetFirstGBQualMatchConstraintName(const CSeq_feat& feat,
                                  const CString_constraint& str_cons) const;
    string x_GetCodeBreakString(const CSeq_feat& feat,
                                    CConstRef <CScope> scope) const;
};

/////////////////// CFeat_qual_choice inline methods

// constructor
inline
CFeat_qual_choice::CFeat_qual_choice(void)
{
}


/////////////////// end of CFeat_qual_choice inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_MACRO_FEAT_QUAL_CHOICE_HPP
/* Original file checksum: lines: 86, chars: 2519, CRC32: a8c85988 */
