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
 *   'biblio.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.6  2005/05/20 13:32:48  shomrat
 * Added BasicCleanup()
 *
 * Revision 6.5  2004/11/15 19:13:17  shomrat
 * Fixed label generation
 *
 * Revision 6.4  2004/09/29 13:57:10  shomrat
 * Fixed call to GetLabelContent
 *
 * Revision 6.3  2004/05/19 17:18:17  gorelenk
 * Added include of PCH - ncbi_pch.hpp
 *
 * Revision 6.2  2004/02/24 15:53:45  grichenk
 * Redesigned GetLabel(), moved most functionality from pub to biblio
 *
 * Revision 6.1  2002/01/10 20:06:13  clausen
 * Added GetLabel
 *
 *
 * ===========================================================================
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/biblio/Cit_art.hpp>
#include <objects/biblio/Imprint.hpp>
#include <objects/biblio/Title.hpp>
#include <objects/biblio/Cit_jour.hpp>
#include <objects/biblio/Cit_book.hpp>
#include <objects/biblio/Cit_proc.hpp>
#include <objects/biblio/label_util.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CCit_art::~CCit_art(void)
{
}


void CCit_art::GetLabel(string* label, bool unique) const
{
    const CCit_jour*  journal = 0;
    const CCit_book*  book = 0;
    const CImprint*   imprint = 0;
    const CAuth_list* authors = 0;
    const CTitle*     title = 0;
    const string*     titleunique = 0;
    if ( IsSetAuthors() ) {
        authors = &GetAuthors();
    }
    if ( IsSetTitle() ) {
        const CRef<CTitle::C_E>& ce = GetTitle().Get().front();
        switch ( ce->Which() ) {
        case CTitle::C_E::e_Name:
            titleunique = &ce->GetName();
            break;
        case CTitle::C_E::e_Tsub:
            titleunique = &ce->GetTsub();
            break;
        case CTitle::C_E::e_Trans:
            titleunique = &ce->GetTrans();
            break;
        case CTitle::C_E::e_Jta:
            titleunique = &ce->GetJta();
            break;
        case CTitle::C_E::e_Iso_jta:
            titleunique = &ce->GetIso_jta();
            break;
        case CTitle::C_E::e_Ml_jta:
            titleunique = &ce->GetMl_jta();
            break;
        case CTitle::C_E::e_Coden:
            titleunique = &ce->GetCoden();
            break;
        case CTitle::C_E::e_Issn:
            titleunique = &ce->GetIssn();
            break;
        case CTitle::C_E::e_Abr:
            titleunique = &ce->GetAbr();
            break;
        case CTitle::C_E::e_Isbn:
            titleunique = &ce->GetIsbn();
            break;
        default:
            break;
        }
    }
    switch ( GetFrom().Which() ) {
    case CCit_art::C_From::e_Journal:
        journal = &GetFrom().GetJournal();
        imprint = &journal->GetImp();
        title = &journal->GetTitle();
        break;
    case CCit_art::C_From::e_Book:
        book = &GetFrom().GetBook();
        imprint = &book->GetImp();
        if (!authors) {
            authors = &book->GetAuthors();
        }
        break;
    case CCit_art::C_From::e_Proc:
        book = &GetFrom().GetProc().GetBook();
        imprint = &book->GetImp();
        if (!authors) {
            authors = &book->GetAuthors();
        }
    default:
        break;
    }
    GetLabelContent(label, unique, authors, imprint, title, book, journal,
        0, 0, titleunique);
}   


void CCit_art::BasicCleanup(bool fix_initials)
{
    if (IsSetAuthors()) {
        SetAuthors().BasicCleanup(fix_initials);
    }
    if (IsSetFrom()) {
        TFrom& from = SetFrom();
        if (from.IsBook()) {
            from.SetBook().BasicCleanup(fix_initials);
        } else if (from.IsProc()) {
            from.SetProc().BasicCleanup(fix_initials);
        }
    }
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 61, chars: 1880, CRC32: 30f0bfed */
