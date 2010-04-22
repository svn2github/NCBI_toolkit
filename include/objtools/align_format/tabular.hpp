/* $Id$ * ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's offical duties as a United States Government employee and
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
* Author:  Ilya Dondoshansky
*
* ===========================================================================
*/

/// @file: tabular.hpp
/// Formatting of pairwise sequence alignments in tabular form. 

#ifndef OBJTOOLS_ALIGN_FORMAT___TABULAR_HPP
#define OBJTOOLS_ALIGN_FORMAT___TABULAR_HPP

#include <corelib/ncbistre.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objmgr/scope.hpp>
#include <objtools/align_format/align_format_util.hpp>

#include <algorithm>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(align_format)

/// Class containing information needed for tabular formatting of BLAST 
/// results.
class NCBI_ALIGN_FORMAT_EXPORT CBlastTabularInfo : public CObject 
{
public:
    /// In what form should the sequence identifiers be shown?
    enum ESeqIdType {
        eFullId = 0, ///< Show full seq-id, with multiple ids concatenated.
        eAccession,  ///< Show only best accession
        eAccVersion, ///< Show only best accession.version
        eGi          ///< Show only gi
    };

    /// What delimiter to use between fields in each row of the tabular output.
    enum EFieldDelimiter {
        eTab = 0, ///< Tab
        eSpace,   ///< Space
        eComma    ///< Comma
    };

    /// Constructor
    /// @param ostr Stream to write output to [in]
    /// @param format Output format - what fields to include in the output [in]
    /// @param delim Delimiter to use between tabular fields [in]
    /// @note fields that are not recognized will be ignored, if no fields are
    /// specified (or left after purging those that are not recognized), the
    /// default format is assumed
    CBlastTabularInfo(CNcbiOstream& ostr, 
                      const string& format = kDfltArgTabularOutputFmt,
                      EFieldDelimiter delim = eTab,
                      bool parse_local_ids = false);

    /// Destructor
    ~CBlastTabularInfo();
    /// Set query id from a objects::CSeq_id
    /// @param id List of Seq-ids to use [in]
    void SetQueryId(list<CRef<objects::CSeq_id> >& id);
    /// Set query id from a Bioseq handle
    /// @param bh Bioseq handle to get Seq-ids from
    void SetQueryId(const objects::CBioseq_Handle& bh);
    /// Set subject id from a objects::CSeq_id
    /// @param id List of Seq-ids to use [in]
    void SetSubjectId(list<CRef<objects::CSeq_id> >& id);
    /// Set subject id from a Bioseq handle
    /// @param bh Bioseq handle to get Seq-ids from
    void SetSubjectId(const objects::CBioseq_Handle& bh);
    /// Set the HSP scores
    /// @param score Raw score [in]
    /// @param bit_score Bit score [in]
    /// @param evalue Expect value [in]
    void SetScores(int score, double bit_score, double evalue);
    /// Set the HSP endpoints. Note that if alignment is on opposite strands,
    /// the subject offsets must be reversed.
    /// @param q_start Starting offset in query [in]
    /// @param q_end Ending offset in query [in]
    /// @param s_start Starting offset in subject [in]
    /// @param s_end Ending offset in subject [in]
    void SetEndpoints(int q_start, int q_end, int s_start, int s_end);
    /// Set various counts/lengths
    /// @param num_ident Number of identities [in]
    /// @param length Alignment length [in]
    /// @param gaps Total number of gaps [in]
    /// @param gap_opens Number of gap openings [in]
    /// @param positives Number of positives [in]
    void SetCounts(int num_ident, int length, int gaps, int gap_opens, 
                   int positives =0, int query_frame = 1, 
                   int subject_frame = 1);
    /// Sets the Blast-traceback-operations string.
    /// @param btop_string string for blast traceback operations [in]
    void SetBTOP(string btop_string);
    /// Set all member fields, given a Seq-align
    /// @param sal Seq-align to get data from [in]
    /// @param scope Scope for Bioseq retrieval [in]
    /// @param matrix Matrix to calculate positives; NULL if not applicable. [in]
    /// @return 0 on success, 1 if query or subject Bioseq is not found.
    virtual int SetFields(const objects::CSeq_align& sal, 
                          objects::CScope& scope, 
                          CNcbiMatrix<int>* matrix=0);

    /// Print one line of tabular output
    virtual void Print(void);
    /// Print the tabular output header
    /// @param program Program name to show in the header [in]
    /// @param bioseq Query Bioseq [in]
    /// @param dbname Search database name [in]
    /// @param rid the search RID (if not applicable, it should be empty
    /// the string) [in]
    /// @param iteration Iteration number (for PSI-BLAST), use default
    /// parameter value when not applicable [in]
    /// @param align_set All alignments for this query [in]
    virtual void PrintHeader(const string& program, 
                             const objects::CBioseq& bioseq, 
                             const string& dbname, 
                             const string& rid = kEmptyStr,
                             unsigned int iteration = 
                                numeric_limits<unsigned int>::max(),
                             const objects::CSeq_align_set* align_set=0,
                             CConstRef<objects::CBioseq> subj_bioseq
                                = CConstRef<objects::CBioseq>()); 

     /// Prints number of queries processed.
     /// @param num_queries number of queries processed [in]
     void PrintNumProcessed(int num_queries);

    /// Return all field names supported in the format string.
    list<string> GetAllFieldNames(void);

    /// Should local IDs be parsed or not?
    /// @param val value to set [in]
    /// Returns true if the field was requested in the format specification
    /// @param field Which field to test [in]
    void SetParseLocalIds(bool val) { m_ParseLocalIds = val; }

protected:
    bool x_IsFieldRequested(ETabularField field);
    /// Add a field to the list of fields to show, if it is not yet present in
    /// the list of fields.
    /// @param field Which field to add? [in]
    void x_AddFieldToShow(ETabularField field);
    /// Delete a field from the list of fields to show
    /// @param field Which field to delete? [in]
    void x_DeleteFieldToShow(ETabularField field);
    /// Add a default set of fields to show.
    void x_AddDefaultFieldsToShow(void);
    /// Set fields to show, given an output format string
    /// @param format Output format [in]
    void x_SetFieldsToShow(const string& format);
    /// Reset values of all fields.
    void x_ResetFields(void);
    /// Set the tabular fields delimiter.
    /// @param delim Which delimiter to use
    void x_SetFieldDelimiter(EFieldDelimiter delim);
    /// Print the names of all supported fields
    void x_PrintFieldNames(void);
    /// Print the value of a given field
    /// @param field Which field to show? [in]
    void x_PrintField(ETabularField field);
    /// Print query Seq-id
    void x_PrintQuerySeqId(void) const;
    /// Print query gi
    void x_PrintQueryGi(void);
    /// Print query accession
    void x_PrintQueryAccession(void);
    /// Print query accession.version
    void x_PrintQueryAccessionVersion(void);
    /// Print subject Seq-id
    void x_PrintSubjectSeqId(void);
    /// Print all Seq-ids associated with this subject, separated by ';'
    void x_PrintSubjectAllSeqIds(void);
    /// Print subject gi
    void x_PrintSubjectGi(void);
    /// Print all gis associated with this subject, separated by ';'
    void x_PrintSubjectAllGis(void);
    /// Print subject accession
    void x_PrintSubjectAccession(void);
    /// Print subject accession.version
    void x_PrintSubjectAccessionVersion(void);
    /// Print all accessions associated with this subject, separated by ';'
    void x_PrintSubjectAllAccessions(void);
    /// Print aligned part of query sequence
    void x_PrintQuerySeq(void);
    /// Print aligned part of subject sequence
    void x_PrintSubjectSeq(void);
    /// Print query start
    void x_PrintQueryStart(void);
    /// Print query end
    void x_PrintQueryEnd(void);
    /// Print subject start
    void x_PrintSubjectStart(void);
    /// Print subject end
    void x_PrintSubjectEnd(void);
    /// Print e-value
    void x_PrintEvalue(void);
    /// Print bit score
    void x_PrintBitScore(void);
    /// Print raw score
    void x_PrintScore(void);
    /// Print alignment length
    void x_PrintAlignmentLength(void);
    /// Print percent of identical matches
    void x_PrintPercentIdentical(void);
    /// Print number of identical matches
    void x_PrintNumIdentical(void);
    /// Print number of mismatches
    void x_PrintMismatches(void);
    /// Print number of positive matches
    void x_PrintNumPositives(void);
    /// Print number of gap openings
    void x_PrintGapOpenings(void);
    /// Print total number of gaps
    void x_PrintGaps(void);
    /// Print percent positives
    void x_PrintPercentPositives();
    /// Print frames
    void x_PrintFrames();
    void x_PrintQueryFrame();
    void x_PrintSubjectFrame();
    void x_PrintBTOP();

    CNcbiOstream& m_Ostream; ///< Stream to write output to
    char m_FieldDelimiter;   ///< Delimiter character for tabular fields.
    string m_QuerySeq;       ///< Aligned part of the query sequence
    string m_SubjectSeq;     ///< Aligned part of the subject sequence
    int m_QueryStart;        ///< Starting offset in query
    int m_QueryEnd;          ///< Ending offset in query
    int m_QueryFrame;        ///< query frame

private:

    list<CRef<objects::CSeq_id> > m_QueryId;  ///< List of query ids for this HSP
    /// All subject sequence ids for this HSP
    vector<list<CRef<objects::CSeq_id> > > m_SubjectIds;
    int m_Score;             ///< Raw score of this HSP
    string m_BitScore;       ///< Bit score of this HSP, in appropriate format
    string m_Evalue;         ///< E-value of this HSP, in appropriate format
    int m_AlignLength;       ///< Alignment length of this HSP
    int m_NumGaps;           ///< Total number of gaps in this HSP
    int m_NumGapOpens;       ///< Number of gap openings in this HSP
    int m_NumIdent;          ///< Number of identities in this HSP
    int m_NumPositives;      ///< Number of positives in this HSP
    int m_SubjectStart;      ///< Starting offset in subject
    int m_SubjectEnd;        ///< Ending offset in subject 
    int m_SubjectFrame;      ///< subject frame
    /// Map of field enum values to field names.
    map<string, ETabularField> m_FieldMap; 
    list<ETabularField> m_FieldsToShow; ///< Which fields to show?
    /// Should the query deflines be parsed for local IDs?
    bool m_ParseLocalIds;
    string m_BTOP;            /// Blast-traceback-operations.
};

inline void CBlastTabularInfo::x_PrintQuerySeq(void)
{
    m_Ostream << m_QuerySeq;
}

inline void CBlastTabularInfo::x_PrintSubjectSeq(void)
{
    m_Ostream << m_SubjectSeq;
}

inline void CBlastTabularInfo::x_PrintQueryStart(void)
{
    m_Ostream << m_QueryStart;
}

inline void CBlastTabularInfo::x_PrintQueryEnd(void)
{
    m_Ostream << m_QueryEnd;
}

inline void CBlastTabularInfo::x_PrintSubjectStart(void)
{
    m_Ostream << m_SubjectStart;
}

inline void CBlastTabularInfo::x_PrintSubjectEnd(void)
{
    m_Ostream << m_SubjectEnd;
}

inline void CBlastTabularInfo::x_PrintEvalue(void)
{
    m_Ostream << m_Evalue;
}

inline void CBlastTabularInfo::x_PrintBitScore(void)
{
    m_Ostream << m_BitScore;
}

inline void CBlastTabularInfo::x_PrintScore(void)
{
    m_Ostream << m_Score;
}

inline void CBlastTabularInfo::x_PrintAlignmentLength(void)
{
    m_Ostream << m_AlignLength;
}

inline void CBlastTabularInfo::x_PrintPercentIdentical(void)
{
    double perc_ident = 
        (m_AlignLength > 0 ? ((double)m_NumIdent)/m_AlignLength * 100 : 0);
    m_Ostream << NStr::DoubleToString(perc_ident, 2);
}

inline void CBlastTabularInfo::x_PrintPercentPositives(void)
{
    double perc_positives = 
        (m_AlignLength > 0 ? ((double)m_NumPositives)/m_AlignLength * 100 : 0);
    m_Ostream << NStr::DoubleToString(perc_positives, 2);
}

inline void CBlastTabularInfo::x_PrintFrames(void)
{
    m_Ostream << m_QueryFrame << "/" << m_SubjectFrame;
}

inline void CBlastTabularInfo::x_PrintQueryFrame(void)
{
    m_Ostream << m_QueryFrame;
}

inline void CBlastTabularInfo::x_PrintSubjectFrame(void)
{
    m_Ostream << m_SubjectFrame;
}

inline void CBlastTabularInfo::x_PrintBTOP(void)
{
    m_Ostream << m_BTOP;
}

inline void CBlastTabularInfo::x_PrintNumIdentical(void)
{
    m_Ostream << m_NumIdent;
}

inline void CBlastTabularInfo::x_PrintMismatches(void)
{
    int num_mismatches = m_AlignLength - m_NumIdent - m_NumGaps;
    m_Ostream << num_mismatches;
}

inline void CBlastTabularInfo::x_PrintNumPositives(void)
{
    m_Ostream << m_NumPositives;
}

// FIXME; do this via a bit field
inline bool CBlastTabularInfo::x_IsFieldRequested(ETabularField field)
{
    return find(m_FieldsToShow.begin(), 
                m_FieldsToShow.end(), 
                field) != m_FieldsToShow.end();
}

inline void CBlastTabularInfo::x_PrintGapOpenings(void)
{
    m_Ostream << m_NumGapOpens;
}

inline void CBlastTabularInfo::x_PrintGaps(void)
{
    m_Ostream << m_NumGaps;
}

/// Class containing information needed for tabular formatting of BLAST 
/// results.
class NCBI_ALIGN_FORMAT_EXPORT CIgBlastTabularInfo : public CBlastTabularInfo
{
public:

    /// struct containing annotated domain information
    struct SIgDomain {
        SIgDomain(const string& n, int s, int e):
            name(n), start(s), end(e), length(0),
            num_match(0), num_mismatch(0), num_gap(0) {};
        const string name;
        int start;
        int end;
        int length;
        int num_match;
        int num_mismatch;
        int num_gap;
    };

    /// struct containing annotated gene information
    struct SIgGene {
        void Set(int s, int e) {
            start = s;
            end = e;
        }
        void Reset() {
            start = -1;
            end = -1;
        };
        int start;
        int end;
    };

    /// What delimiter to use between fields in each row of the tabular output.
    /// Constructor
    /// @param ostr Stream to write output to [in]
    /// @param format Output format - what fields to include in the output [in]
    CIgBlastTabularInfo(CNcbiOstream& ostr,
                        const string& format = kDfltArgTabularOutputFmt)
        : CBlastTabularInfo(ostr, format) { };

    /// Destructor
    ~CIgBlastTabularInfo() {
        x_ResetIgFields();
    };

    /// Set fields for master alignment
    int SetMasterFields(const objects::CSeq_align& align, 
                        objects::CScope&           scope, 
                        const string&              chain_type,
                        CNcbiMatrix<int>*          matrix=0);

    /// Set fields for all other alignments
    int SetFields(const objects::CSeq_align& align,
                  objects::CScope&           scope,
                  const string&              chain_type,
                  CNcbiMatrix<int>*          matrix=0);

    /// Override the print method
    virtual void Print(void);

    /// Print domain information
    void PrintMasterAlign() const;

    /// Set out-of-frame information                                        
    void SetFrame(const string &frame = "N/A") { 
        m_FrameInfo = frame;                               
    };

    /// Set strand information                                        
    void SetMinusStrand(bool minus = true) {
        m_IsMinusStrand = minus;
    };

    /// Set domain info
    void AddIgDomain(const string &name, int start, int end) {
        if (start <0 || end <= start) return;
        SIgDomain * domain = new SIgDomain(name, start, end);
        x_ComputeIgDomain(*domain);
        m_IgDomains.push_back(domain);
    };

    /// Set gene info
    void SetVGene(int s, int e) {
        m_VGene.Set(s,e);
    }

    /// Set gene info
    void SetDGene(int s, int e) {
        m_DGene.Set(s,e);
    }

    /// Set gene info
    void SetJGene(int s, int e) {
        m_JGene.Set(s,e);
    }

protected:
    void x_ResetIgFields();
    void x_PrintIgGenes() const;
    void x_ComputeIgDomain(SIgDomain &domain);
    void x_PrintIgDomain(const SIgDomain &domain) const;
    void x_PrintPartialQuery(int start, int end) const;

private:                                                                    
    string m_Query;
    bool m_IsMinusStrand;
    string m_FrameInfo;                                                           
    string m_ChainType;
    SIgGene m_VGene;
    SIgGene m_DGene;
    SIgGene m_JGene;
    vector<SIgDomain *> m_IgDomains;                                        
};

END_SCOPE(align_format)
END_NCBI_SCOPE

#endif /* OBJTOOLS_ALIGN_FORMAT___TABULAR_HPP */
