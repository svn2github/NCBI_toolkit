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
* Author: Aleksey Grichenko
*
* File Description:
*   Object manager iterators
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.5  2002/03/05 16:08:13  grichenk
* Moved TSE-restriction to new constructors
*
* Revision 1.4  2002/03/04 15:07:47  grichenk
* Added "bioseq" argument to CAnnotTypes_CI constructor to iterate
* annotations from a single TSE.
*
* Revision 1.3  2002/02/21 19:27:04  grichenk
* Rearranged includes. Added scope history. Added searching for the
* best seq-id match in data sources and scopes. Updated tests.
*
* Revision 1.2  2002/01/16 16:25:56  gouriano
* restructured objmgr
*
* Revision 1.1  2002/01/11 19:06:15  gouriano
* restructured objmgr
*
*
* ===========================================================================
*/

#include <objects/objmgr1/align_ci.hpp>

#include "annot_object.hpp"

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)



CAlign_CI::CAlign_CI(void)
{
    return;
}


CAlign_CI::CAlign_CI(CScope& scope,
                     const CSeq_loc& loc)
    : CAnnotTypes_CI(scope, loc,
          SAnnotSelector(CSeq_annot::C_Data::e_Align))
{
    return;
}


CAlign_CI::CAlign_CI(CBioseq_Handle& bioseq, int start, int stop)
    : CAnnotTypes_CI(bioseq, start, stop,
          SAnnotSelector(CSeq_annot::C_Data::e_Align))
{
    return;
}


CAlign_CI::CAlign_CI(const CAlign_CI& iter)
    : CAnnotTypes_CI(iter)
{
    return;
}


CAlign_CI::~CAlign_CI(void)
{
    return;
}


CAlign_CI& CAlign_CI::operator= (const CAlign_CI& iter)
{
    CAnnotTypes_CI::operator=(iter);
    return *this;
}


CAlign_CI& CAlign_CI::operator++ (void)
{
    Walk();
    return *this;
}


CAlign_CI& CAlign_CI::operator++ (int)
{
    Walk();
    return *this;
}


CAlign_CI::operator bool (void) const
{
    return IsValid();
}


const CSeq_align& CAlign_CI::operator* (void) const
{
    CAnnotObject* annot = Get();
    _ASSERT(annot  &&  annot->IsAlign());
    return annot->GetAlign();
}


const CSeq_align* CAlign_CI::operator-> (void) const
{
    CAnnotObject* annot = Get();
    _ASSERT(annot  &&  annot->IsAlign());
    return &annot->GetAlign();
}

END_SCOPE(objects)
END_NCBI_SCOPE
