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
 *   'seq.asn'.
 *
 */

// standard includes
#include <vector>
#include <serial/enumvalues.hpp>
#include <serial/typeinfo.hpp>
#include <corelib/ncbiutil.hpp>

// generated includes
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Textseq_id.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/seq/Seq_inst.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqloc/Seq_point.hpp>
#include <objects/seq/Bioseq.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CBioseq::~CBioseq(void)
{
}

void CBioseq::UserOp_Assign(const CSerialUserOp& source)
{
    const CBioseq& src = dynamic_cast<const CBioseq&>(source);
    m_ParentEntry = src.m_ParentEntry;
}

bool CBioseq::UserOp_Equals(const CSerialUserOp& object) const
{
    const CBioseq& obj = dynamic_cast<const CBioseq&>(object);
    return m_ParentEntry == obj.m_ParentEntry;
}


int CBioseq::sm_ConstructedId = 0;

void CBioseq::x_SeqLoc_To_DeltaExt(const CSeq_loc& loc, CDelta_ext& ext)
{
    switch ( loc.Which() ) {
    case CSeq_loc::e_Packed_int:
        {
            // extract each range, create and add simple location
            iterate ( CPacked_seqint::Tdata, ii, loc.GetPacked_int().Get() ) {
                CSeq_loc* int_loc = new CSeq_loc;
                SerialAssign<CSeq_id>
                    (int_loc->SetInt().SetId(), (*ii)->GetId());
                int_loc->SetInt().SetFrom((*ii)->GetFrom());
                int_loc->SetInt().SetTo((*ii)->GetTo());
                if ( (*ii)->IsSetStrand() )
                    int_loc->SetInt().SetStrand((*ii)->GetStrand());
                CDelta_seq* dseq = new CDelta_seq;
                dseq->SetLoc(*int_loc);
                ext.Set().push_back(dseq);
            }
            break;
        }
    case CSeq_loc::e_Packed_pnt:
        {
            // extract each point
            iterate ( CPacked_seqpnt::TPoints, pi,
                      loc.GetPacked_pnt().GetPoints() ) {
                CSeq_loc* pnt_loc = new CSeq_loc;
                SerialAssign<CSeq_id>
                    (pnt_loc->SetPnt().SetId(), loc.GetPacked_pnt().GetId());
                pnt_loc->SetPnt().SetPoint(*pi);
                if ( loc.GetPacked_pnt().IsSetStrand() ) {
                    pnt_loc->SetPnt().SetStrand(
                        loc.GetPacked_pnt().GetStrand());
                }
                CDelta_seq* dseq = new CDelta_seq;
                dseq->SetLoc(*pnt_loc);
                ext.Set().push_back(dseq);
            }
        }
    case CSeq_loc::e_Mix:
        {
            // extract sub-locations
            iterate ( CSeq_loc_mix::Tdata, li, loc.GetMix().Get() ) {
                x_SeqLoc_To_DeltaExt(**li, ext);
            }
            return;
        }
    default:
        {
            // Just add the location
            CDelta_seq* dseq = new CDelta_seq;
            CSeq_loc* cp_loc = new CSeq_loc;
            SerialAssign<CSeq_loc>(*cp_loc, loc);
            dseq->SetLoc(*cp_loc);
            ext.Set().push_back(dseq);
        }
    }
}


CBioseq::CBioseq(const CSeq_loc& loc, string str_id)
    : m_ParentEntry(0)
{
    CBioseq::TId& id_list = SetId();

    // Id
    CSeq_id* id = new CSeq_id;
    if ( str_id.empty() ) {
        id->SetLocal().SetStr("constructed" + NStr::IntToString(sm_ConstructedId++));
    }
    else {
        id->SetLocal().SetStr(str_id);
    }
    id_list.push_back(id);

    // Inst
    CSeq_inst& inst = SetInst();
    inst.SetRepr(CSeq_inst::eRepr_const);
    inst.SetMol(CSeq_inst::eMol_other);

    CDelta_ext& ext = inst.SetExt().SetDelta();
    x_SeqLoc_To_DeltaExt(loc, ext);
}


void CBioseq::GetLabel(string* label, ELabelType type, bool worst) const
{
    if (!label) {
        return;
    }
    
    if (type != eType  &&  !GetId().empty()) {
        const CSeq_id* id;
        if (!worst) {
            id = GetId().begin()->GetPointer();
        } else {
            const CSeq_id* wid =
                FindBestChoice(GetId(), CSeq_id::WorstRank).GetPointer();
            if (wid) {
                CNcbiOstrstream wos;
                wid->WriteAsFasta(wos);
                string sid = CNcbiOstrstreamToString(wos);
                CSeq_id worst_id(sid);
                CTextseq_id* tid =
                    const_cast<CTextseq_id*>(worst_id.GetTextseq_Id());
                if (tid) {
                    tid->ResetAccession();
                }
                id = &worst_id;
            }
        }
        CNcbiOstrstream os;
        if (id) {
            id->WriteAsFasta(os);
            string s = CNcbiOstrstreamToString(os);
            (*label) += s;
        }
    }

    if (type == eContent) {
        return;
    }

    if (!label->empty()) {
        (*label) += ": ";
    }
    
    const CEnumeratedTypeValues* tv;
    tv = CSeq_inst::GetTypeInfo_enum_ERepr();
    (*label) += tv->FindName(GetInst().GetRepr(), true) + ",";
    tv = CSeq_inst::GetTypeInfo_enum_EMol();
    (*label) += tv->FindName(GetInst().GetMol(), true);
    if (GetInst().IsSetLength()) {
        (*label) += string(" len=") + NStr::IntToString(GetInst().GetLength());
    }
}


const CSeq_id* CBioseq::GetFirstId() const
{
    // If no ids for Bioseq, return 0 -- should not happen
    if (GetId().empty()) {
        return 0;
    }

    return *GetId().begin();
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
 * ===========================================================================
 * $Log$
 * Revision 6.15  2002/10/08 19:09:39  clausen
 * Fixed formatting but in GetLabel()
 *
 * Revision 6.14  2002/10/03 21:29:59  ivanov
 * Fixed error in GetLabel()
 *
 * Revision 6.13  2002/10/03 19:07:31  clausen
 * Removed extra whitespace
 *
 * Revision 6.12  2002/10/03 16:57:50  clausen
 * Added GetLabel() and GetFirstId()
 *
 * Revision 6.11  2002/05/22 14:03:38  grichenk
 * CSerialUserOp -- added prefix UserOp_ to Assign() and Equals()
 *
 * Revision 6.10  2002/04/22 20:09:57  grichenk
 * -ConstructExcludedSequence() -- use
 * CBioseq_Handle::GetSequenceView() instead
 *
 * Revision 6.9  2002/03/28 21:21:49  grichenk
 * Fixed range exclusion
 *
 * Revision 6.8  2002/03/18 21:46:13  grichenk
 * +ConstructExcludedSequence()
 *
 * Revision 6.7  2002/01/16 18:56:31  grichenk
 * Removed CRef<> argument from choice variant setter, updated sources to
 * use references instead of CRef<>s
 *
 * Revision 6.6  2001/12/20 20:00:31  grichenk
 * CObjectManager::ConstructBioseq(CSeq_loc) -> CBioseq::CBioseq(CSeq_loc ...)
 *
 * Revision 6.5  2001/10/12 19:32:57  ucko
 * move BREAK to a central location; move CBioseq::GetTitle to object manager
 *
 * Revision 6.4  2001/10/04 19:11:54  ucko
 * Centralize (rudimentary) code to get a sequence's title.
 *
 * Revision 6.3  2001/07/16 16:20:19  grichenk
 * Initial revision
 *
 *
 * ===========================================================================
 */
/* Original file checksum: lines: 61, chars: 1871, CRC32: 1d5d7d05 */
