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
 *   'seqfeat.asn'.
 */

#ifndef OBJECTS_SEQFEAT_SEQ_FEAT_HPP
#define OBJECTS_SEQFEAT_SEQ_FEAT_HPP


// generated includes
#include <objects/seqfeat/Seq_feat_.hpp>
#include <objects/seqfeat/ISeq_feat.hpp>
#include <objects/seqfeat/Gene_ref.hpp>
#include <objects/seqfeat/Prot_ref.hpp>
#include <objects/seqfeat/SeqFeatData.hpp>
#include <objects/seqloc/Seq_loc.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE /// namespace ncbi::objects::

class NCBI_SEQFEAT_EXPORT CSeq_feat : public CSeq_feat_Base,
                                      public ISeq_feat
{
    typedef CSeq_feat_Base Tparent;
public:
    /// constructor
    CSeq_feat(void);
    /// destructor
    ~CSeq_feat(void);
    
    //
    /// See related function in util/feature.hpp
    //
    ///   void GetLabel (const CSeq_feat&, string*, ELabelType, CScope*)
    //

    /// get gene (if present) from Seq-feat.xref list
    const CGene_ref* GetGeneXref(void) const;
    void SetGeneXref(CGene_ref& value);
    CGene_ref& SetGeneXref(void);

    /// get protein (if present) from Seq-feat.xref list
    const CProt_ref* GetProtXref(void) const;
    void SetProtXref(CProt_ref& value);
    CProt_ref& SetProtXref(void);

    /// Add a qualifier to this feature
    void AddQualifier(const string& qual_name, const string& qual_val);

    /// Add a qualifier to this feature, or replace the value for the first
    /// one if it already exists.
    void AddOrReplaceQualifier(const string& qual_name, const string& qual_val);

    /// Remove all qualifiers with the given name; do nothing if
    /// no such qualifier exists.  Complexity is linear in the number of quals,
    /// which is the best that can be done since quals may not be sorted.
    void RemoveQualifier(const string& qual_name);

    /// add a DB xref to this feature
    void AddDbxref(const string& db_name, const string& db_key);
    void AddDbxref(const string& db_name, int db_key);

    /// Add the given exception_text and set the except flag to true.
    /// If the exception_text is already there, it just sets the except flag.
    void AddExceptText(const string & exception_text);

    /// Remove all instances of the given exception text in this feature,
    /// and reset the except flag if there are no exception texts left.
    void RemoveExceptText(const string & exception_text);

    /// Return a specified DB xref.  This will find the *first* item in the
    /// given referenced database.  If no item is found, an empty CConstRef<>
    /// is returned.
    CConstRef<CDbtag> GetNamedDbxref(const string& db) const;

    /// Return a named qualifier.  This will return the first item matching the
    /// qualifier name.  If no such qualifier is found, an empty string is
    /// returned.
    const string& GetNamedQual(const string& qual_name) const;

    /// Warning: This is invalidated if the underlying except_text is 
    /// changed in any way.
    typedef set<CTempStringEx, PNocase> TExceptionTextSet;

    /// Returns a case-insensitive set of exception texts.  
    /// Warning: The returned contents are invalidated 
    /// if the underlying exception_text is changed in any way.
    AutoPtr<TExceptionTextSet> GetTempExceptionTextSet(void) const;

    /// Returns whether or not the given exception_text is set for this 
    /// feature.  Note that it always returns false if IsExcept is set,
    /// even if there is exception text.
    ///
    /// @param exception_text
    ///   The exception text to search for.
    bool HasExceptionText(const string & exception_text ) const;

    /// Compare relative order of this feature and feature f2,
    /// ordering first by features' coordinates, by importance of their type,
    /// by complexity of location, and by some other fields depending on
    /// their types.
    /// Return a value < 0 if this feature should come before f2.
    /// Return a value > 0 if this feature should come after f2.
    /// Return zero if the features are unordered.
    /// Note, that zero value doesn't mean that the features are identical.
    int Compare(const CSeq_feat& f2) const;
    int Compare(const CSeq_feat& f2,
                const CSeq_loc& mapped1, const CSeq_loc& mapped2) const;

    /// Compare relative order of this feature and feature f2 similarily
    /// to the Compare() method, assuming their locations are already
    /// compared as unordered.
    /// The features' locations are still needed because the order depends
    /// also on complexity of location (mix or not, etc).
    /// The features are assumed to have location as given in arguments:
    /// loc1 - location of this feature, loc2 - location of feature f2.
    /// This method is useful if features are mapped to a master sequence,
    /// so their location are changed after mapping.
    /// Return a value < 0 if this feature should come before f2.
    /// Return a value > 0 if this feature should come after f2.
    /// Return zero if the features are unordered.
    /// Note, that zero value doesn't mean that the features are identical.
    int CompareNonLocation(const CSeq_feat& f2,
                           const CSeq_loc& loc1, const CSeq_loc& loc2) const;

    /// Return relative importance order of features by their type.
    /// These methods are used by Compare() and CompareNonLocation() methods.
    int GetTypeSortingOrder(void) const;
    static int GetTypeSortingOrder(CSeqFeatData::E_Choice type);

    /// Find extension by type in exts container.
    /// @param ext_type
    ///   String id of the extension to find.
    /// @result
    ///   User-object of the requested type or NULL.
    CConstRef<CUser_object> FindExt(const string& ext_type) const;
    /// Non-const version of FindExt().
    CRef<CUser_object> FindExt(const string& ext_type);

private:

    /// Prohibit copy constructor and assignment operator
    CSeq_feat(const CSeq_feat& value);
    CSeq_feat& operator=(const CSeq_feat& value);        
};


/////////////////// CSeq_feat inline methods

// constructor
inline
CSeq_feat::CSeq_feat(void)
{
}


// Corresponds to SortFeatItemListByPos from the C toolkit
inline
int CSeq_feat::Compare(const CSeq_feat& f2,
                       const CSeq_loc& loc1, const CSeq_loc& loc2) const
{
    int diff = loc1.Compare(loc2);
    return diff != 0? diff: CompareNonLocation(f2, loc1, loc2);
}


// Corresponds to SortFeatItemListByPos from the C toolkit
inline
int CSeq_feat::Compare(const CSeq_feat& f2) const
{
    return Compare(f2, GetLocation(), f2.GetLocation());
}


inline
int CSeq_feat::GetTypeSortingOrder(void) const
{
    return GetTypeSortingOrder(GetData().Which());
}


NCBI_SEQFEAT_EXPORT
inline
bool operator< (const CSeq_feat& f1, const CSeq_feat& f2)
{
    return f1.Compare(f2) < 0;
}

/////////////////// end of CSeq_feat inline methods


END_objects_SCOPE /// namespace ncbi::objects::

END_NCBI_SCOPE


#endif /// OBJECTS_SEQFEAT_SEQ_FEAT_HPP
/* Original file checksum: lines: 90, chars: 2388, CRC32: c285198b */
