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
* Author: Aleksey Grichenko, Eugene Vasilchenko
*
* File Description:
*   Seq-feat handle
*
*/


#include <ncbi_pch.hpp>
#include <objmgr/seq_feat_handle.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/impl/seq_annot_info.hpp>
#include <objmgr/impl/snp_annot_info.hpp>
#include <objmgr/impl/scope_impl.hpp>
#include <objmgr/impl/annot_collector.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


CSeq_feat_Handle::CSeq_feat_Handle(const CSeq_annot_Handle& annot,
                                   const CAnnotObject_Info& feat_info,
                                   CCreatedFeat_Ref& created_ref)
    : m_Annot(annot),
      m_AnnotInfoType(eType_Seq_annot_Info),
      m_AnnotPtr(&feat_info),
      m_CreatedFeat(&created_ref)
{
    _ASSERT(feat_info.IsFeat());
    _ASSERT(&feat_info.GetSeq_annot_Info() == &annot.x_GetInfo());
}


CSeq_feat_Handle::CSeq_feat_Handle(const CSeq_annot_Handle& annot,
                                   const SSNP_Info& snp_info,
                                   CCreatedFeat_Ref& created_ref)
    : m_Annot(annot),
      m_AnnotInfoType(eType_Seq_annot_SNP_Info),
      m_AnnotPtr(&snp_info),
      m_CreatedFeat(&created_ref)
{
    _ASSERT(annot.x_GetInfo().x_HasSNP_annot_Info());
}


void CSeq_feat_Handle::Reset(void)
{
    m_CreatedFeat.Reset();
    m_AnnotPtr = 0;
    m_AnnotInfoType = eType_null;
    m_Annot.Reset();
}


const SSNP_Info& CSeq_feat_Handle::x_GetSNP_Info(void) const
{
    if ( m_AnnotInfoType != eType_Seq_annot_SNP_Info ) {
        NCBI_THROW(CObjMgrException, eInvalidHandle,
                   "CSeq_feat_Handle::GetSNP_Info: not SNP info");
    }
    const SSNP_Info* snp_info = static_cast<const SSNP_Info*>(m_AnnotPtr);
    _ASSERT(snp_info);
    if ( snp_info->IsRemoved() ) {
        NCBI_THROW(CObjMgrException, eInvalidHandle,
                   "CSeq_feat_Handle::GetSNP_Info: SNP was removed");
    }
    return *snp_info;
}


const CSeq_annot_SNP_Info& CSeq_feat_Handle::x_GetSNP_annot_Info(void) const
{
    if ( m_AnnotInfoType != eType_Seq_annot_SNP_Info ) {
        NCBI_THROW(CObjMgrException, eInvalidHandle,
                   "CSeq_feat_Handle::GetSNP_annot_Info: not SNP info");
    }
    return GetAnnot().x_GetInfo().x_GetSNP_annot_Info();
}


const CSeq_feat& CSeq_feat_Handle::x_GetSeq_feat(void) const
{
    if ( m_AnnotInfoType != eType_Seq_annot_Info ) {
        NCBI_THROW(CObjMgrException, eInvalidHandle,
                   "CSeq_feat_Handle::GetSeq_feat: not Seq-feat info");
    }
    const CAnnotObject_Info* feat_info =
        static_cast<const CAnnotObject_Info*>(m_AnnotPtr);
    _ASSERT(feat_info);
    if ( feat_info->IsRemoved() ) {
        NCBI_THROW(CObjMgrException, eInvalidHandle,
                   "CSeq_feat_Handle::GetSeq_feat: Seq-feat was removed");
    }
    return feat_info->GetFeat();
}


CConstRef<CSeq_feat> CSeq_feat_Handle::GetSeq_feat(void) const
{
    switch (m_AnnotInfoType) {
    case eType_Seq_annot_Info:
        {
            return ConstRef(&x_GetSeq_feat());
        }
    case eType_Seq_annot_SNP_Info:
        {
            return m_CreatedFeat->MakeOriginalFeature(*this);
        }
    default:
        {
            return CConstRef<CSeq_feat>(0);
        }
    }
}


CSeq_feat_Handle::TRange CSeq_feat_Handle::GetRange(void) const
{
    switch (m_AnnotInfoType) {
    case eType_Seq_annot_Info:
        {
            return x_GetSeq_feat().GetLocation().GetTotalRange();
        }
    case eType_Seq_annot_SNP_Info:
        {
            const SSNP_Info& snp_info = x_GetSNP_Info();
            return TRange(snp_info.GetFrom(), snp_info.GetTo());
        }
    default:
        {
            return TRange::GetEmpty();
        }
    }
}


CSeq_id::TGi CSeq_feat_Handle::GetSNPGi(void) const
{
    return x_GetSNP_annot_Info().GetGi();
}


size_t CSeq_feat_Handle::GetSNPAllelesCount(void) const
{
    const SSNP_Info& snp = x_GetSNP_Info();
    size_t count = 0;
    for (; count < SSNP_Info::kMax_AllelesCount; ++count) {
        if (snp.m_AllelesIndices[count] == SSNP_Info::kNo_AlleleIndex) {
            break;
        }
    }
    return count;
}


const string& CSeq_feat_Handle::GetSNPAllele(size_t index) const
{
    _ASSERT(index < SSNP_Info::kMax_AllelesCount);
    const SSNP_Info& snp = x_GetSNP_Info();
    _ASSERT(snp.m_AllelesIndices[index] != SSNP_Info::kNo_AlleleIndex);
    return x_GetSNP_annot_Info().x_GetAllele(snp.m_AllelesIndices[index]);
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.11  2005/08/23 17:03:01  vasilche
 * Use CAnnotObject_Info pointer instead of annotation index in annot handles.
 *
 * Revision 1.10  2005/04/07 16:30:42  vasilche
 * Inlined handles' constructors and destructors.
 * Optimized handles' assignment operators.
 *
 * Revision 1.9  2005/03/15 19:10:29  vasilche
 * SSNP_Info structure is defined in separate header.
 *
 * Revision 1.8  2005/03/07 17:29:04  vasilche
 * Added "SNP" to names of SNP access methods
 *
 * Revision 1.7  2005/02/24 19:13:34  grichenk
 * Redesigned CMappedFeat not to hold the whole annot collector.
 *
 * Revision 1.6  2004/12/28 18:40:30  vasilche
 * Added GetScope() method.
 *
 * Revision 1.5  2004/12/22 15:56:12  vasilche
 * Used CSeq_annot_Handle in annotations' handles.
 *
 * Revision 1.4  2004/11/04 19:21:18  grichenk
 * Marked non-handle versions of SetLimitXXXX as deprecated
 *
 * Revision 1.3  2004/08/04 14:53:26  vasilche
 * Revamped object manager:
 * 1. Changed TSE locking scheme
 * 2. TSE cache is maintained by CDataSource.
 * 3. CObjectManager::GetInstance() doesn't hold CRef<> on the object manager.
 * 4. Fixed processing of split data.
 *
 * Revision 1.2  2004/05/21 21:42:13  gorelenk
 * Added PCH ncbi_pch.hpp
 *
 * Revision 1.1  2004/05/04 18:06:06  grichenk
 * Initial revision
 *
 *
 * ===========================================================================
 */
