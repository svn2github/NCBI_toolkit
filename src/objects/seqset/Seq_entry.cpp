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
 *   'seqset.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objects/seqset/Bioseq_set.hpp>

#include <objects/seq/Bioseq.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSeq_entry::~CSeq_entry(void)
{
}

void CSeq_entry::ResetParentEntry(void)
{
    m_ParentEntry = 0;
}

void CSeq_entry::SetParentEntry(CSeq_entry* entry)
{
    m_ParentEntry = entry;
}

void CSeq_entry::Parentize(void)
{
    switch ( Which() ) {
    case e_Seq:
        SetSeq().SetParentEntry(this);
        break;
    case e_Set:
        NON_CONST_ITERATE ( CBioseq_set::TSeq_set, si, SetSet().SetSeq_set() ) {
            (*si)->SetParentEntry(this);
            (*si)->Parentize();
        }
        break;
    case e_not_set:
        break;
    }
}

void CSeq_entry::ParentizeOneLevel(void)
{
    switch ( Which() ) {
    case e_Seq:
        SetSeq().SetParentEntry(this);
        break;
    case e_Set:
        NON_CONST_ITERATE ( CBioseq_set::TSeq_set, si, SetSet().SetSeq_set() ) {
            (*si)->SetParentEntry(this);
        }
        break;
    case e_not_set:
        break;
    }
}

void CSeq_entry::UserOp_Assign(const CSerialUserOp& /* source */)
{
    m_ParentEntry = 0;
    ParentizeOneLevel();
}

bool CSeq_entry::UserOp_Equals(const CSerialUserOp& /*object*/) const
{
    return true;
}


static CBioseq::ELabelType s_GetBioseqLabelType(CSeq_entry::ELabelType lt)
{
    switch (lt) {
        case CSeq_entry::eType:    return CBioseq::eType;
        case CSeq_entry::eContent: return CBioseq::eContent;
        default:
            _ASSERT(lt==CSeq_entry::eBoth);
            return CBioseq::eBoth;
    }
}


static CBioseq_set::ELabelType s_GetBioseqSetLabelType(CSeq_entry::ELabelType lt)
{
    switch (lt) {
        case CSeq_entry::eType:    return CBioseq_set::eType;
        case CSeq_entry::eContent: return CBioseq_set::eContent;
        default:
            _ASSERT(lt==CSeq_entry::eBoth);
            return CBioseq_set::eBoth;
    }
}


void CSeq_entry::GetLabel(string* label, ELabelType type) const
{
    switch ( Which() ) {
    case e_Seq:
        GetSeq().GetLabel(label, s_GetBioseqLabelType(type));
        break;
    case e_Set:
        GetSet().GetLabel(label, s_GetBioseqSetLabelType(type));
        break;
    case e_not_set:
    default:
        *label += "???";
    }
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 61, chars: 1886, CRC32: 18c50f7 */
