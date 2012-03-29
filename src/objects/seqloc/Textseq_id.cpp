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
 * Author:  Jim Ostell
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the ASN data definition file
 *   'seqloc.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/seqloc/Textseq_id.hpp>
#include <objects/seqloc/Seq_id.hpp>


// generated classes

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::


// destructor
CTextseq_id::~CTextseq_id(void)
{
    return;
}


CTextseq_id& CTextseq_id::Set
(const string&  acc_in,
 const string&  name_in,
 int            version,
 const string&  release_in,
 bool           allow_dot_version)
{
    // Perform general sanity checks up front.
    if (version < 0) {
        NCBI_THROW(CSeqIdException, eFormat,
                   "Unexpected negative version " + NStr::IntToString(version)
                   + " for accession " + acc_in);
    }

    string acc     = NStr::TruncateSpaces(acc_in,     NStr::eTrunc_Both);
    string name    = NStr::TruncateSpaces(name_in,    NStr::eTrunc_Both);
    string release = NStr::TruncateSpaces(release_in, NStr::eTrunc_Both);

    if (acc.empty()  &&  name.empty()) {
    }

    if (acc.empty()) {
        ResetAccession();
    } else {
        SIZE_TYPE idx = NPOS;

        if (allow_dot_version) {
            idx = acc.rfind('.');
        }

        if (idx == NPOS) {
            // no version within acc
            SetAccession(acc);

            // any standalone version is ok
            if (version > 0) {
                SetVersion(version); 
            } else {
                ResetVersion();
            }
        } else {
            // accession.version
            string accession = acc.substr(0, idx);
            string acc_ver   = acc.substr(idx + 1);
            int    ver       = NStr::StringToNonNegativeInt(acc_ver);
 
            if (ver <= 0) {
                NCBI_THROW(CSeqIdException, eFormat,
                           "Version embedded in accession " + acc
                           + " is not a positive integer");
            } else if (version > 0  &&  ver != version) {
                NCBI_THROW(CSeqIdException, eFormat,
                           "Incompatible version " + NStr::IntToString(version)
                           + " supplied for accession " + acc);
            }
 
            SetAccession(accession);
            SetVersion(ver);
        }
    }

    if (name.empty()) {
        ResetName();
    } else {
        SetName(name);
    }

    if (acc.empty()  &&  name.empty()) {
        NCBI_THROW(CSeqIdException, eFormat,
                   "Accession and name missing for Textseq-id (but got"
                   " version " + NStr::IntToString(version) + ", release "
                   + release + ')');
    } else if (release.empty()) {
        ResetRelease();
    } else {
        SetRelease(release);
    }
    return *this;
}


// comparison function
bool CTextseq_id::Match(const CTextseq_id& tsip2) const
{
    // Check Accessions first
    if (IsSetAccession()  &&  tsip2.IsSetAccession()) {
        if ( PNocase().Equals(GetAccession(), tsip2.GetAccession()) ) {
            if (IsSetVersion()  &&  tsip2.IsSetVersion()) {
                return GetVersion() == tsip2.GetVersion();
            } else {
                return true;
            }
        } else {
            return false;
        }
    }

    // then try name
    if (IsSetName()  &&  tsip2.IsSetName()) {
        if ( PNocase().Equals(GetName(), tsip2.GetName()) ) {
            if (IsSetVersion()  &&  tsip2.IsSetVersion()) {
                return GetVersion() == tsip2.GetVersion();
            }
            else {
                return true;
            }
        } else {
            return false;
        }
    }

    //nothing to compare
    return false;
}


// comparison function
int CTextseq_id::Compare(const CTextseq_id& tsip2) const
{
    // Check Accessions first
    // no-accession before accession
    if ( int diff = IsSetAccession() - tsip2.IsSetAccession() ) {
        return diff;
    }
    if ( IsSetAccession() ) {
        _ASSERT(tsip2.IsSetAccession());
        // sort by accession
        if ( int diff = PNocase().Compare(GetAccession(),
                                          tsip2.GetAccession()) ) {
            return diff;
        }
    }

    // Check version
    // no-version before version
    if ( int diff = IsSetVersion() - tsip2.IsSetVersion() ) {
        return diff;
    }
    if ( IsSetVersion() ) {
        _ASSERT(tsip2.IsSetVersion());
        // smaller version first
        if ( int diff = GetVersion() - tsip2.GetVersion() ) {
            return diff;
        }
    }
    if ( IsSetAccession() && IsSetVersion() ) {
        // acc.ver are the same -> equal Seq-ids
        return 0;
    }

    // Check name
    // no-name before name
    if ( int diff = IsSetName() - tsip2.IsSetName() ) {
        return diff;
    }
    if ( IsSetName() ) {
        _ASSERT(tsip2.IsSetName());
        if ( int diff = PNocase().Compare(GetName(), tsip2.GetName()) ) {
            return diff;
        }
    }
    
    // All checks failed to distinguish Seq-ids.
    return 0;
}


// format the contents FASTA string style
ostream& CTextseq_id::AsFastaString(ostream& s, bool allow_version) const
{
    if (IsSetAccession()) {
        s << GetAccession(); // no Upcase per Ostell - Karl 7/2001
        if (allow_version  &&  IsSetVersion()) {
            int version = GetVersion();
            if (version) {
                s << '.' << version;
            }
        }
    }

    s << '|';
    if ( IsSetName() ) {
        s << GetName();  // no Upcase per Ostell - Karl 7/2001
    }
    return s;
}


END_objects_SCOPE // namespace ncbi::objects::
END_NCBI_SCOPE
