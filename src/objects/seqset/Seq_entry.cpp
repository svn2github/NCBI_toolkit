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
    }
}

void CSeq_entry::UserOp_Assign(const CSerialUserOp& source)
{
    const CSeq_entry& src = dynamic_cast<const CSeq_entry&>(source);
    m_ParentEntry = 0;
    ParentizeOneLevel();
}

bool CSeq_entry::UserOp_Equals(const CSerialUserOp& /*object*/) const
{
    return true;
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/*
 * ===========================================================================
 *
 * $Log$
 * Revision 6.26  2003/06/04 17:25:05  ucko
 * Move FASTA reader to objtools/readers.
 *
 * Revision 6.25  2003/05/31 15:55:48  lavr
 * Explicit 'int'->'bool' casts to avoid compiler warnings
 *
 * Revision 6.24  2003/05/23 20:27:50  ucko
 * Enhance ReadFasta: recognize various types of comment, and optionally
 * report lowercase characters' location.
 *
 * Revision 6.23  2003/05/15 18:50:41  kuznets
 * Implemented ReadFastaFileMap function. Function reads multientry FASTA
 * file filling SFastaFileMap structure(seq_id, sequence offset, description)
 *
 * Revision 6.22  2003/05/14 15:06:30  ucko
 * ReadFasta: if unable to conclude anything from the IDs, try passing the
 * first line of data to CFormatGuess::SequenceType (unless ForceType is set)
 *
 * Revision 6.21  2003/05/09 21:46:27  ucko
 * ReadFasta: fix initial data assignment to avoid ten leading "A"s;
 * also, fix some typos (including one that identified everything as NA)
 *
 * Revision 6.20  2003/05/09 16:08:35  ucko
 * Rename fReadFasta_Redund to fReadFasta_AllSeqIds.
 *
 * Revision 6.19  2003/05/09 15:48:59  ucko
 * +fReadFasta_{Redund,NoSeqData} (suggested by Michel Dumontier)
 * Split off s_ParseFastaDefline.
 *
 * Revision 6.18  2003/04/24 16:14:12  vasilche
 * Fixed Parentize().
 *
 * Revision 6.17  2003/03/11 15:56:34  kuznets
 * iterate -> ITERATE
 *
 * Revision 6.16  2003/03/10 21:18:27  grichenk
 * Fixed Parentize()
 *
 * Revision 6.15  2003/03/10 21:08:19  grichenk
 * UserOp_Assign resets m_Parent
 *
 * Revision 6.14  2003/02/24 20:03:11  gouriano
 * use template-based exceptions instead of errno and parse exceptions
 *
 * Revision 6.13  2003/01/13 20:01:24  gouriano
 * corrected parsing fasta seq ids
 *
 * Revision 6.12  2003/01/10 19:34:50  gouriano
 * corrected s_EndOfFastaID in case of incomplete CSeq_id::e_Other
 *
 * Revision 6.11  2003/01/06 16:14:04  gouriano
 * corrected ReadFasta: set sequence's molecule class
 *
 * Revision 6.10  2003/01/03 13:16:08  dicuccio
 * Minor formatting change.  Added comment about intentional fall-through.  Fixed
 * big in parsing of FastA IDs: must skip trailing space in ID string
 *
 * Revision 6.9  2002/11/25 18:50:01  ucko
 * Skip initial > when parsing IDs (caught by Mike DiCuccio)
 *
 * Revision 6.8  2002/11/04 21:29:17  grichenk
 * Fixed usage of const CRef<> and CRef<> constructor
 *
 * Revision 6.7  2002/10/30 02:34:41  ucko
 * Change seq to seq.NotEmpty() in compound test to make MSVC happy.
 *
 * Revision 6.6  2002/10/29 22:09:36  ucko
 * +fReadFasta_OneSeq
 *
 * Revision 6.5  2002/10/23 19:23:15  ucko
 * Move the FASTA reader from objects/util/sequence.?pp to
 * objects/seqset/Seq_entry.?pp because it doesn't need the OM.
 * Move the CVS log to the end of the file per current practice.
 *
 * Revision 6.4  2002/07/25 15:01:55  grichenk
 * Replaced non-const GetXXX() with SetXXX()
 *
 * Revision 6.3  2002/05/22 14:03:41  grichenk
 * CSerialUserOp -- added prefix UserOp_ to Assign() and Equals()
 *
 * Revision 6.2  2001/07/16 16:22:50  grichenk
 * Added CSerialUserOp class to create Assign() and Equals() methods for
 * user-defind classes.
 * Added SerialAssign<>() and SerialEquals<>() functions.
 *
 * Revision 6.1  2001/06/13 14:59:33  grichenk
 * Initial revision
 *
 * ===========================================================================
 */
/* Original file checksum: lines: 61, chars: 1886, CRC32: 18c50f7 */
