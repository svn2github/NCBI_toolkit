#ifndef OBJMGR_UTIL_AUTODEF_SOURCE_GROUP__HPP
#define OBJMGR_UTIL_AUTODEF_SOURCE_GROUP__HPP

/*  $Id$
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
* Author:  Colleen Bollin
*
* File Description:
*   Creates unique definition lines for sequences in a set using organism
*   descriptions and feature clauses.
*/

#include <corelib/ncbistd.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/seqfeat/OrgMod.hpp>
#include <objects/seqfeat/SubSource.hpp>
#include <objects/seqfeat/OrgName.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seq/MolInfo.hpp>

#include <objmgr/bioseq_handle.hpp>
#include <objmgr/seq_entry_handle.hpp>

#include <objmgr/util/autodef_available_modifier.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class NCBI_XOBJUTIL_EXPORT CAutoDefSourceGroup
{
public:
    CAutoDefSourceGroup();
    CAutoDefSourceGroup(CAutoDefSourceGroup *value);
    ~CAutoDefSourceGroup();
    
    unsigned int GetNumDescriptions();
    CAutoDefSourceDescription *GetSourceDescription(unsigned int index);
    void AddSourceDescription(CAutoDefSourceDescription *tmp);

    bool SourceDescBelongsHere(CAutoDefSourceDescription *source_desc, IAutoDefCombo *mod_combo);
    
    CAutoDefSourceGroup *RemoveNonMatchingDescriptions (IAutoDefCombo *mod_combo);
    
    void GetAvailableModifiers
        (CAutoDefSourceDescription::TAvailableModifierVector &modifier_list);

    typedef vector<CAutoDefSourceDescription *> TSourceDescriptionVector;
    bool HasTrickyHIV();
    bool GetDefaultExcludeSp();

private:
    TSourceDescriptionVector m_SourceList;
    
    void x_SortDescriptions(IAutoDefCombo *mod_combo);

};
    
END_SCOPE(objects)
END_NCBI_SCOPE

#endif //OBJMGR_UTIL_AUTODEF_SOURCE_GROUP__HPP
