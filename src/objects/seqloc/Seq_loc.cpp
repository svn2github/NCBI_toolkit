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
 * Author:   Author:  Cliff Clausen, Eugene Vasilchenko
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the ASN data definition file
 *   'seqloc.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.14  2002/09/12 21:19:02  kans
 * added IsPartialLeft and IsPartialRight
 *
 * Revision 6.13  2002/06/06 20:35:28  clausen
 * Moved methods using object manager to objects/util
 *
 * Revision 6.12  2002/05/31 13:33:02  grichenk
 * GetLength() -- return 0 for e_Null locations
 *
 * Revision 6.11  2002/05/06 03:39:12  vakatov
 * OM/OM1 renaming
 *
 * Revision 6.10  2002/05/03 21:28:18  ucko
 * Introduce T(Signed)SeqPos.
 *
 * Revision 6.9  2002/04/22 20:08:31  grichenk
 * Redesigned GetTotalRange() using CSeq_loc_CI
 *
 * Revision 6.8  2002/04/17 15:39:08  grichenk
 * Moved CSeq_loc_CI to the seq-loc library
 *
 * Revision 6.7  2002/01/16 18:56:32  grichenk
 * Removed CRef<> argument from choice variant setter, updated sources to
 * use references instead of CRef<>s
 *
 * Revision 6.6  2002/01/10 18:21:26  clausen
 * Added IsOneBioseq, GetStart, and GetId
 *
 * Revision 6.5  2001/10/22 11:40:32  clausen
 * Added Compare() implementation
 *
 * Revision 6.4  2001/01/03 18:59:09  vasilche
 * Added missing include.
 *
 * Revision 6.3  2001/01/03 16:39:05  vasilche
 * Added CAbstractObjectManager - stub for object manager.
 * CRange extracted to separate file.
 *
 * Revision 6.2  2000/12/26 17:28:55  vasilche
 * Simplified and formatted code.
 *
 * Revision 6.1  2000/11/17 21:35:10  vasilche
 * Added GetLength() method to CSeq_loc class.
 *
 *
 * ===========================================================================
 */

#include <objects/seqloc/Seq_point.hpp>
#include <objects/seqloc/Seq_loc_equiv.hpp>
#include <objects/seqloc/Seq_loc.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::


// destructor
CSeq_loc::~CSeq_loc(void)
{
}


// returns enclosing location range
// the total range is meaningless if there are several seq-ids
// in the location
CSeq_loc::TRange CSeq_loc::GetTotalRange(void) const
{
    TRange total_range(TRange::GetEmptyFrom(), TRange::GetEmptyTo());
    CSeq_loc_CI loc_ci(*this);
    for ( ; loc_ci; loc_ci++) {
        total_range += loc_ci.GetRange();
    }
    return total_range;
}


// CSeq_loc_CI implementation

CSeq_loc_CI::CSeq_loc_CI(void)
    : m_Location(0)
{
    m_CurLoc = m_LocList.end();
}


CSeq_loc_CI::CSeq_loc_CI(const CSeq_loc& loc)
    : m_Location(&loc)
{
    x_ProcessLocation(loc);
    m_CurLoc = m_LocList.begin();
}


CSeq_loc_CI::~CSeq_loc_CI(void)
{
    return;
}


CSeq_loc_CI::CSeq_loc_CI(const CSeq_loc_CI& iter)
{
    *this = iter;
}


CSeq_loc_CI& CSeq_loc_CI::operator= (const CSeq_loc_CI& iter)
{
    if (this == &iter)
        return *this;
    m_LocList.clear();
    m_Location = iter.m_Location;
    iterate(TLocList, li, iter.m_LocList) {
        TLocList::iterator tmp = m_LocList.insert(m_LocList.end(), *li);
        if (iter.m_CurLoc == li)
            m_CurLoc = tmp;
    }
    return *this;
}


void CSeq_loc_CI::x_ProcessLocation(const CSeq_loc& loc)
{
    switch ( loc.Which() ) {
    case CSeq_loc::e_not_set:
    case CSeq_loc::e_Null:
    case CSeq_loc::e_Empty:
        {
            // Ignore empty locations
            return;
        }
    case CSeq_loc::e_Whole:
        {
            SLoc_Info info;
            info.m_Id = &loc.GetWhole();
            info.m_Range.SetFrom(TRange::GetWholeFrom());
            info.m_Range.SetTo(TRange::GetWholeTo());
            m_LocList.push_back(info);
            return;
        }
    case CSeq_loc::e_Int:
        {
            SLoc_Info info;
            info.m_Id = &loc.GetInt().GetId();
            info.m_Range.SetFrom(loc.GetInt().GetFrom());
            info.m_Range.SetTo(loc.GetInt().GetTo());
            if ( loc.GetInt().IsSetStrand() )
                info.m_Strand = loc.GetInt().GetStrand();
            m_LocList.push_back(info);
            return;
        }
    case CSeq_loc::e_Pnt:
        {
            SLoc_Info info;
            info.m_Id = &loc.GetPnt().GetId();
            info.m_Range.SetFrom(loc.GetPnt().GetPoint());
            info.m_Range.SetTo(loc.GetPnt().GetPoint());
            if ( loc.GetPnt().IsSetStrand() )
                info.m_Strand = loc.GetPnt().GetStrand();
            m_LocList.push_back(info);
            return;
        }
    case CSeq_loc::e_Packed_int:
        {
            iterate ( CPacked_seqint::Tdata, ii, loc.GetPacked_int().Get() ) {
                SLoc_Info info;
                info.m_Id = &(*ii)->GetId();
                info.m_Range.SetFrom((*ii)->GetFrom());
                info.m_Range.SetTo((*ii)->GetTo());
                if ( (*ii)->IsSetStrand() )
                    info.m_Strand = (*ii)->GetStrand();
                m_LocList.push_back(info);
            }
            return;
        }
    case CSeq_loc::e_Packed_pnt:
        {
            iterate ( CPacked_seqpnt::TPoints, pi, loc.GetPacked_pnt().GetPoints() ) {
                SLoc_Info info;
                info.m_Id = &loc.GetPacked_pnt().GetId();
                info.m_Range.SetFrom(*pi);
                info.m_Range.SetTo(*pi);
                if ( loc.GetPacked_pnt().IsSetStrand() )
                    info.m_Strand = loc.GetPacked_pnt().GetStrand();
                m_LocList.push_back(info);
            }
            return;
        }
    case CSeq_loc::e_Mix:
        {
            iterate(CSeq_loc_mix::Tdata, li, loc.GetMix().Get()) {
                x_ProcessLocation(**li);
            }
            return;
        }
    case CSeq_loc::e_Equiv:
        {
            iterate(CSeq_loc_equiv::Tdata, li, loc.GetEquiv().Get()) {
                x_ProcessLocation(**li);
            }
            return;
        }
    case CSeq_loc::e_Bond:
        {
            SLoc_Info infoA;
            infoA.m_Id = &loc.GetBond().GetA().GetId();
            infoA.m_Range.SetFrom(loc.GetBond().GetA().GetPoint());
            infoA.m_Range.SetTo(loc.GetBond().GetA().GetPoint());
            if ( loc.GetBond().GetA().IsSetStrand() )
                infoA.m_Strand = loc.GetBond().GetA().GetStrand();
            m_LocList.push_back(infoA);
            if ( loc.GetBond().IsSetB() ) {
                SLoc_Info infoB;
                infoB.m_Id = &loc.GetBond().GetB().GetId();
                infoB.m_Range.SetFrom(loc.GetBond().GetB().GetPoint());
                infoB.m_Range.SetTo(loc.GetBond().GetB().GetPoint());
                if ( loc.GetBond().GetB().IsSetStrand() )
                    infoB.m_Strand = loc.GetBond().GetB().GetStrand();
                m_LocList.push_back(infoB);
            }
            return;
        }
    case CSeq_loc::e_Feat:
    default:
        {
            throw runtime_error
                ("CSeq_loc_CI -- unsupported location type");
        }
    }
}

bool CSeq_loc::IsPartialLeft (void) const

{
    switch (Which ()) {

        case CSeq_loc::e_Null :
            return true;

        case CSeq_loc::e_Int :
            return GetInt ().IsPartialLeft ();

        case CSeq_loc::e_Pnt :
            return GetPnt ().IsPartialLeft ();

        case CSeq_loc::e_Mix :
            return GetMix ().IsPartialLeft ();

        default :
            break;
    }

    return false;
}

bool CSeq_loc::IsPartialRight (void) const

{
    switch (Which ()) {

        case CSeq_loc::e_Null :
            return true;

        case CSeq_loc::e_Int :
            return GetInt ().IsPartialRight ();

        case CSeq_loc::e_Pnt :
            return GetPnt ().IsPartialRight ();

        case CSeq_loc::e_Mix :
            return GetMix ().IsPartialRight ();

        default :
            break;
    }

    return false;
}


END_objects_SCOPE // namespace ncbi::objects::
END_NCBI_SCOPE

