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
 *   'genome_collection.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/general/User_field.hpp>
#include <objects/general/User_object.hpp>
#include <objects/genomecoll/GC_Replicon.hpp>
#include <objects/genomecoll/GC_AssemblyUnit.hpp>
#include <objects/genomecoll/GC_Assembly.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// constructor
CGC_Replicon::CGC_Replicon(void)
    : m_Assembly(NULL)
    , m_AssemblyUnit(NULL)
{
}

// destructor
CGC_Replicon::~CGC_Replicon(void)
{
}


/// Access the assembly unit the sequence belongs to
CConstRef<CGC_AssemblyUnit> CGC_Replicon::GetAssemblyUnit() const
{
    return CConstRef<CGC_AssemblyUnit>(m_AssemblyUnit);
}


/// Access the assembly the sequence belongs to
CConstRef<CGC_Assembly> CGC_Replicon::GetFullAssembly() const
{
    return CConstRef<CGC_Assembly>(m_Assembly);
}

static 
CConstRef<CUser_object> x_GetMolLocTypeUserObj(const CGC_Replicon& rep)
{
    const CGC_Sequence& seq = rep.GetSequence().GetSingle();
    if (seq.IsSetDescr()) {
        ITERATE(CSeq_descr::Tdata, dit, seq.GetDescr().Get()) {
            if ((*dit)->IsUser()) {
                const CUser_object& uo = (*dit)->GetUser();
                if ( uo.GetType().IsStr() && uo.GetType().GetStr() == "molecule-location-type") {
                    return CConstRef<CUser_object>(&uo);
                }
            }
        }
    }
    return CConstRef<CUser_object>();
}

string CGC_Replicon::GetMoleculeLocation() const
{
    CConstRef<CUser_object> uo = x_GetMolLocTypeUserObj(*this);
    if (uo) {
        return uo->GetField("location").GetData().GetStr();
    }
    return kEmptyStr;
}

string CGC_Replicon::GetMoleculeType() const
{
    CConstRef<CUser_object> uo = x_GetMolLocTypeUserObj(*this);
    if (uo) {
        return uo->GetField("type").GetData().GetStr();
    }
    return kEmptyStr;
}

string CGC_Replicon::GetMoleculeLabel() const
{
    CConstRef<CUser_object> uo = x_GetMolLocTypeUserObj(*this);
    if (uo) {
        return uo->GetField("label").GetData().GetStr();
    }
    return kEmptyStr;
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1758, CRC32: 4b99539e */
