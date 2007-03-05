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
 *   using the following specifications:
 *   'omssa.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/omssa/MSSearch.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

int 
CMSSearch::SetUpSearchSettings(CRef <CMSSearchSettings> & SearchSettings, 
                               bool IsIterative)
{
    // the ordinal number of the SearchSettings
    int Settingid(0);

    // set up search settings
    if(IsIterative) {
        Settingid = 1 + (*SetRequest().begin())->SetMoresettings().Set().size();
        (*SetRequest().begin())->SetMoresettings().Set().push_back(SearchSettings);
    }
    else {
        (*SetRequest().begin())->SetSettings(*SearchSettings);
    }
    SearchSettings->SetSettingid() = Settingid;

    return Settingid;
}


// destructor
CMSSearch::~CMSSearch(void)
{
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 65, chars: 1884, CRC32: 7c1f0b36 */
