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
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 1.9  2002/03/18 21:46:11  grichenk
 * +ConstructExcludedSequence()
 *
 * Revision 1.8  2001/12/20 20:00:28  grichenk
 * CObjectManager::ConstructBioseq(CSeq_loc) -> CBioseq::CBioseq(CSeq_loc ...)
 *
 * Revision 1.7  2001/10/12 19:32:55  ucko
 * move BREAK to a central location; move CBioseq::GetTitle to object manager
 *
 * Revision 1.6  2001/10/04 19:11:54  ucko
 * Centralize (rudimentary) code to get a sequence's title.
 *
 * Revision 1.5  2001/07/25 19:11:07  grichenk
 * Equals() and Assign() re-declared as protected
 *
 * Revision 1.4  2001/07/16 16:22:42  grichenk
 * Added CSerialUserOp class to create Assign() and Equals() methods for
 * user-defind classes.
 * Added SerialAssign<>() and SerialEquals<>() functions.
 *
 * Revision 1.3  2001/06/25 18:51:59  grichenk
 * Prohibited copy constructor and assignment operator
 *
 * Revision 1.2  2001/06/21 19:47:34  grichenk
 * Copy constructor and operator=() moved to "private" section
 *
 * Revision 1.1  2001/06/13 15:00:06  grichenk
 * Initial revision
 *
 *
 * ===========================================================================
 */

#ifndef OBJECTS_SEQ_BIOSEQ_HPP
#define OBJECTS_SEQ_BIOSEQ_HPP


// generated includes
#include <objects/seq/Bioseq_.hpp>
#include <map>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CSeq_entry;
class CSeq_loc;
class CDelta_ext;

class CBioseq : public CBioseq_Base, public CSerialUserOp
{
    typedef CBioseq_Base Tparent;
public:
    // constructor
    CBioseq(void);
    // destructor
    ~CBioseq(void);
    // Manage Seq-entry tree structure
    CSeq_entry* GetParentEntry(void) const;

    // see CScope::GetTitle in object manager.
    // string GetTitle(void) const;

    // Construct bioseq from seq-loc. The constructed bioseq
    // has id = "local|"+str_id or "local|constructed###", where
    // ### is a generated number; inst::repr = const,
    // inst::mol = other (since it is impossible to check sequence
    // type by seq-loc). The location is splitted into simple
    // locations (intervals, points, whole-s etc.) and put into
    // ext::delta.
    CBioseq(const CSeq_loc& loc, string str_id = "");

    // Create a segmented bioseq including all ranges from a source
    // sequence except those included in "loc". The "loc" must contain
    // ranges from a single sequence only. "len" is the total length
    // of the source sequence.
    static CBioseq& ConstructExcludedSequence(const CSeq_loc& loc,
                                              int len,
                                              string str_id = "");
protected:
    // From CSerialUserOp
    virtual void Assign(const CSerialUserOp& source);
    virtual bool Equals(const CSerialUserOp& object) const;

private:
    // Prohibit copy constructor and assignment operator
    CBioseq(const CBioseq& value);
    CBioseq& operator= (const CBioseq& value);

    // Seq-entry containing the Bioseq
    void SetParentEntry(CSeq_entry* entry);
    CSeq_entry* m_ParentEntry;

    static void x_SeqLoc_To_DeltaExt(const CSeq_loc& loc, CDelta_ext& ext);

    // Map included interval start to the interval end
    enum EPointType {
        eStart,
        eStop
    };
    typedef map<int, EPointType> TRanges;
    static void x_ExcludeRange(TRanges& ranges, int start, int stop);
    // Ranges are filled with intervals not covered by loc,
    // id is set to the source bioseq id (the only id mentioned by loc).
    static void x_ExcludeSeqLoc(const CSeq_loc& loc, TRanges& ranges, CSeq_id& id);
    // Convert a range into seq-interval or seq-point
    static void x_RangeToSeqLoc(CSeq_loc& loc, CSeq_id& id, int start, int stop);

    static int sm_ConstructedId;

    friend class CSeq_entry;
};



/////////////////// CBioseq inline methods

// constructor
inline
CBioseq::CBioseq(void)
    : m_ParentEntry(0)
{
}

inline
void CBioseq::SetParentEntry(CSeq_entry* entry)
{
    m_ParentEntry = entry;
}

inline
CSeq_entry* CBioseq::GetParentEntry(void) const
{
    return m_ParentEntry;
}

/////////////////// end of CBioseq inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


#endif // OBJECTS_SEQ_BIOSEQ_HPP
/* Original file checksum: lines: 85, chars: 2191, CRC32: 21fd3921 */
