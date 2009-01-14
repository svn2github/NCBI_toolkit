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
 *   'seqloc.asn'.
 */

#ifndef OBJECTS_SEQLOC_SEQ_ID_HPP
#define OBJECTS_SEQLOC_SEQ_ID_HPP


// generated includes
#include <objects/seqloc/Seq_id_.hpp>
#include <corelib/ncbi_limits.hpp>
#include <serial/serializable.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seqloc/Textseq_id.hpp>

// generated classes

BEGIN_NCBI_SCOPE

class ILineReader;

BEGIN_objects_SCOPE // namespace ncbi::objects::

/** @addtogroup OBJECTS_Seqid
 *
 * @{
 */


class CBioseq;


class NCBI_SEQLOC_EXPORT CSeq_id : public CSeq_id_Base,
                                   public CSerializable
{
    typedef CSeq_id_Base Tparent;

public:

    ///
    /// See also CSeq_id related functions in "util/sequence.hpp":
    ///
    ///   TSeqPos GetLength(const CSeq_id&, CScope*);
    ///   bool IsSameBioseq(const CSeq_id&, const CSeq_id&, CScope*);
    ///

    /// Default constructor
    CSeq_id(void);

    /// Takes either a FastA-style string delimited by vertical bars or
    /// a raw accession (with optional version) or GI.
    explicit CSeq_id(const string& the_id);

    /// Construct a seq-id from a dbtag.
    /// @param tag
    ///   Input dbtag.
    /// @param set_as_general
    ///   Whether to store tags from unrecognized databases as is in
    ///   Seq-ids of type general rather than rejecting them altogether.
    explicit CSeq_id(const CDbtag& tag, bool set_as_general = true);

    /// Construct a numeric Seq-id.
    /// @param the_type
    ///   Type of Seq-id (normally e_Gi)
    /// @param int_seq_id
    ///   Numeric value.
    CSeq_id(E_Choice the_type,
            int      int_seq_id);

    /// Construct a Seq-id from a flat representation.
    /// @param the_type
    ///   Type of Seq_id to construct
    /// @param acc_in
    ///   Primary string value -- normally accession, overridden as
    ///   country for patents, database for "general" IDs, molecule ID
    ///   for PDB IDs.
    /// @param name_in
    ///   Additional string value -- normally name/locus, overridden as
    ///   (application) number for patents, tag for "general" IDs,
    ///   chain ID for PDB.
    /// @param version
    ///   Numeric value -- normally version number, overriden as sequence
    ///   number for patents.
    /// @param release_in
    ///   Historically used to convey release identifiers; for patents,
    ///   may be set to "pgp" (case-insensitive) to indicate a
    ///   Pre-Grant Patent [application].
    CSeq_id(E_Choice      the_type,
            const string& acc_in,
            const string& name_in    = kEmptyStr,
            int           version    = 0,
            const string& release_in = kEmptyStr);

    /// Reassign based on flat specifications; arguments interpreted
    /// as with constructors.  (Returns a reference to self.)

    CSeq_id& Set(const string& the_id);

    CSeq_id& Set(const CDbtag& tag, bool set_as_general = true);

    CSeq_id& Set(E_Choice the_type,
                 int      int_seq_id);

    CSeq_id& Set(CSeq_id_Base::E_Choice the_type,
                 const string&          acc_in,
                 const string&          name_in    = kEmptyStr,
                 int                    version    = 0,
                 const string&          release_in = kEmptyStr);

    /// Destructor
    virtual ~CSeq_id(void);

    /// Converts a string to a choice, no need to require a member.
    static E_Choice WhichInverseSeqId(const char* SeqIdCode);

    /// For IdentifyAccession (below)
    enum EAccessionInfo {
        // Mask for Seq_id type; allow 8 bits to be safe
        eAcc_type_mask = 0xff,

        // Useful general flags (not inherent in choice of division,
        // albeit not necessarily applicable to all divisions).
        fAcc_nuc       = 0x80000000,
        fAcc_prot      = 0x40000000,
        fAcc_predicted = 0x20000000, // only for refseq
        fAcc_specials  = 0x10000000, // has special cases; only used internally
        eAcc_flag_mask = 0xf0000000,

        // Divisions and categories (multiples of 1 << 8; always
        // globally unique nowadays, no matter how specialized)
        eAcc_other         =   0 << 8, // no further classification
        eAcc_est           =   1 << 8, // expressed sequence tag (mRNA)
        eAcc_dirsub        =   2 << 8, // direct submission of anything
        eAcc_div_patent    =   3 << 8, // patented sequence
        eAcc_mrna          =   4 << 8, // non-EST mRNA or cDNA
        eAcc_ncrna         =   5 << 8, // non-coding RNA
        eAcc_gsdb_ds       =   6 << 8, // Genome Sequence DB direct submission
        eAcc_gsdb          =   7 << 8, // other GSDB record
        eAcc_backbone      =   8 << 8, // from journal scanning
        eAcc_tsa           =   9 << 8, // transcriptome shotgun assembly
        eAcc_ambig         = 112 << 8, // multiply assigned (!)
        eAcc_ambig_g       = 113 << 8, // assigned in GenBank
        eAcc_ambig_e       = 114 << 8, // assigned in EMBL
        eAcc_ambig_ge      = 115 << 8, // assigned in both GenBank and EMBL
        eAcc_ambig_d       = 116 << 8, // assigned in DDBJ
        eAcc_ambig_gd      = 117 << 8, // GenBank and DDBJ
        eAcc_ambig_ed      = 118 << 8, // EMBL and DDBJ
        eAcc_ambig_ged     = 119 << 8, // all three
        eAcc_unreserved    = 127 << 8, // not yet formally assigned
        fAcc_genomic       = 128 << 8, // genomic (flag; any of the below)
        eAcc_genome        = 128 << 8, // complete genome
        eAcc_htgs          = 129 << 8, // high-throughput genome sequence
        eAcc_con           = 130 << 8, // intermediate genomic assembly; contig
        eAcc_segset        = eAcc_con, // (was once wrongly split out)
        eAcc_wgs           = 131 << 8, // whole-genome shotgun collection
        eAcc_gss           = 132 << 8, // genome survey sequence
        eAcc_chromosome    = 133 << 8, // whole chromosome
        eAcc_genomic_rgn   = 134 << 8, // incomplete genomic region
        eAcc_wgs_intermed  = 135 << 8, // WGS intermediate assembly
        eAcc_sts           = 136 << 8, // sequence tagged site
        eAcc_division_mask = 0xff00,

        // Actual return values with EXAMPLE prefixes (to be followed
        // by digits) or IDs, grouped by Seq-id type.  In most cases,
        // there are other prefixes with the same classification, and
        // if not there could be in principle.
        eAcc_unknown         = e_not_set | eAcc_other,
        // Most N accessions are GenBank ESTs, but some low-numbered
        // ones (now only used as secondary accessions) were assigned
        // haphazardly, and some are therefore ambiguous.
        eAcc_ambiguous_nuc   = e_not_set | eAcc_ambig      | fAcc_nuc, // N0-N1
        eAcc_maybe_gb        = e_not_set | eAcc_ambig_g    | fAcc_nuc,
        eAcc_maybe_embl      = e_not_set | eAcc_ambig_e    | fAcc_nuc,
        eAcc_maybe_ddbj      = e_not_set | eAcc_ambig_d    | fAcc_nuc,
        eAcc_gb_embl         = e_not_set | eAcc_ambig_ge   | fAcc_nuc, //N00001
        eAcc_gb_ddbj         = e_not_set | eAcc_ambig_gd   | fAcc_nuc, //N00006
        eAcc_embl_ddbj       = e_not_set | eAcc_ambig_ed   | fAcc_nuc, //N00070
        eAcc_gb_embl_ddbj    = e_not_set | eAcc_ambig_ged  | fAcc_nuc, //N00005
        eAcc_unreserved_nuc  = e_not_set | eAcc_unreserved | fAcc_nuc, // XY
        eAcc_unreserved_prot = e_not_set | eAcc_unreserved | fAcc_prot,// XYZ

        eAcc_local  = e_Local  | eAcc_other,
        eAcc_gibbsq = e_Gibbsq | eAcc_other,
        eAcc_gibbmt = e_Gibbmt | eAcc_other,
        eAcc_giim   = e_Giim   | eAcc_other,

        eAcc_gb_prot        = e_Genbank | eAcc_other      | fAcc_prot, // AAA
        eAcc_gb_other_nuc   = e_Genbank | eAcc_other      | fAcc_nuc,  // AS
        eAcc_gb_est         = e_Genbank | eAcc_est        | fAcc_nuc,  // H
        eAcc_gb_dirsub      = e_Genbank | eAcc_dirsub     | fAcc_nuc,  // U
        eAcc_gb_patent      = e_Genbank | eAcc_div_patent /* | fAcc_nuc */, //I
        eAcc_gb_patent_prot = e_Genbank | eAcc_div_patent | fAcc_prot, // AAE
        eAcc_gb_cdna        = e_Genbank | eAcc_mrna       | fAcc_nuc,  // BC
        eAcc_gsdb_dirsub    = e_Genbank | eAcc_gsdb_ds    | fAcc_nuc,  // J
        eAcc_gb_gsdb        = e_Genbank | eAcc_gsdb       | fAcc_nuc,  // AD
        eAcc_gb_backbone    = e_Genbank | eAcc_backbone   | fAcc_nuc,  // S
        eAcc_gb_tsa_nuc     = e_Genbank | eAcc_tsa        | fAcc_nuc,  // EZ
        eAcc_gb_tsa_prot    = e_Genbank | eAcc_tsa        | fAcc_prot,
        eAcc_gb_genome      = e_Genbank | eAcc_genome     | fAcc_nuc,  // AE
        eAcc_gb_htgs        = e_Genbank | eAcc_htgs       | fAcc_nuc,  // AC
        eAcc_gb_con         = e_Genbank | eAcc_con        | fAcc_nuc,  // CH
        eAcc_gb_segset      = eAcc_gb_con, // for compatibility
        eAcc_gb_wgs_nuc     = e_Genbank | eAcc_wgs        | fAcc_nuc,  // AAAA
        eAcc_gb_wgs_prot    = e_Genbank | eAcc_wgs        | fAcc_prot, // EAA
        eAcc_gb_gss         = e_Genbank | eAcc_gss        | fAcc_nuc,  // B
        eAcc_gb_sts         = e_Genbank | eAcc_sts        | fAcc_nuc,  // G

        eAcc_embl_prot      = e_Embl | eAcc_other      | fAcc_prot, // CAA
        eAcc_embl_other_nuc = e_Embl | eAcc_other      | fAcc_nuc,  // N00060
        eAcc_embl_est       = e_Embl | eAcc_est        | fAcc_nuc,  // F
        eAcc_embl_dirsub    = e_Embl | eAcc_dirsub     | fAcc_nuc,  // V
        eAcc_embl_patent    = e_Embl | eAcc_div_patent | fAcc_nuc,  // A
        eAcc_embl_tsa_nuc   = e_Embl | eAcc_tsa        | fAcc_nuc,
        eAcc_embl_tsa_prot  = e_Embl | eAcc_tsa        | fAcc_prot,
        eAcc_embl_genome    = e_Embl | eAcc_genome     | fAcc_nuc,  // AL
        eAcc_embl_htgs      = e_Embl | eAcc_htgs       | fAcc_nuc,  // unused
        eAcc_embl_con       = e_Embl | eAcc_con        | fAcc_nuc,  // AN
        eAcc_embl_wgs_nuc   = e_Embl | eAcc_wgs        | fAcc_nuc,  // CAAA
        eAcc_embl_wgs_prot  = e_Embl | eAcc_wgs        | fAcc_prot, // unused

        eAcc_pir       = e_Pir       | eAcc_other | fAcc_prot,
        eAcc_swissprot = e_Swissprot | eAcc_other | fAcc_prot, // P
        eAcc_patent    = e_Patent    | eAcc_div_patent,

        eAcc_refseq_prot        = e_Other | eAcc_other        | fAcc_prot,//NP_
        eAcc_refseq_mrna        = e_Other | eAcc_mrna         | fAcc_nuc, //NM_
        eAcc_refseq_ncrna       = e_Other | eAcc_ncrna        | fAcc_nuc, //NR_
        eAcc_refseq_unreserved  = e_Other | eAcc_unreserved,              //AA_
        eAcc_refseq_genome      = e_Other | eAcc_genome       | fAcc_nuc, //NS_
        eAcc_refseq_contig      = e_Other | eAcc_con          | fAcc_nuc, //NT_
        eAcc_refseq_wgs_nuc     = e_Other | eAcc_wgs          | fAcc_nuc, //NZ_
        eAcc_refseq_wgs_prot    = e_Other | eAcc_wgs          | fAcc_prot,//ZP_
        eAcc_refseq_chromosome  = e_Other | eAcc_chromosome   | fAcc_nuc, //NC_
        eAcc_refseq_genomic     = e_Other | eAcc_genomic_rgn  | fAcc_nuc, //NG_
        eAcc_refseq_wgs_intermed= e_Other | eAcc_wgs_intermed | fAcc_nuc, //NW_
        eAcc_refseq_prot_predicted  = eAcc_refseq_prot  | fAcc_predicted, //XP_
        eAcc_refseq_mrna_predicted  = eAcc_refseq_mrna  | fAcc_predicted, //XM_
        eAcc_refseq_ncrna_predicted = eAcc_refseq_ncrna | fAcc_predicted, //XR_

        eAcc_general = e_General | eAcc_other,
        eAcc_gi      = e_Gi      | eAcc_other,

        eAcc_ddbj_prot      = e_Ddbj | eAcc_other      | fAcc_prot, // BAA
        eAcc_ddbj_other_nuc = e_Ddbj | eAcc_other      | fAcc_nuc,  // N00028
        eAcc_ddbj_est       = e_Ddbj | eAcc_est        | fAcc_nuc,  // C
        eAcc_ddbj_dirsub    = e_Ddbj | eAcc_dirsub     | fAcc_nuc,  // D
        eAcc_ddbj_patent    = e_Ddbj | eAcc_div_patent | fAcc_nuc,  // E
        eAcc_ddbj_tsa_nuc   = e_Ddbj | eAcc_tsa        | fAcc_nuc,
        eAcc_ddbj_tsa_prot  = e_Ddbj | eAcc_tsa        | fAcc_prot,
        eAcc_ddbj_genome    = e_Ddbj | eAcc_genome     | fAcc_nuc,  // AP
        eAcc_ddbj_htgs      = e_Ddbj | eAcc_htgs       | fAcc_nuc,  // AK
        eAcc_ddbj_con       = e_Ddbj | eAcc_con        | fAcc_nuc,  // BA
        eAcc_ddbj_wgs_nuc   = e_Ddbj | eAcc_wgs        | fAcc_nuc,  // BAAA
        eAcc_ddbj_wgs_prot  = e_Ddbj | eAcc_wgs        | fAcc_prot, // GAA
        eAcc_ddbj_gss       = e_Ddbj | eAcc_gss        | fAcc_nuc,  // DE

        eAcc_prf = e_Prf | eAcc_other | fAcc_prot,
        eAcc_pdb = e_Pdb | eAcc_other, // not necessarily protein!

        eAcc_gb_tpa_nuc        = e_Tpg | eAcc_other      | fAcc_nuc,  // BK
        eAcc_gb_tpa_prot       = e_Tpg | eAcc_other      | fAcc_prot, // DAA
        eAcc_gb_tpa_con        = e_Tpg | eAcc_con        | fAcc_nuc,  // GJ
        eAcc_gb_tpa_wgs_nuc    = e_Tpg | eAcc_wgs        | fAcc_nuc,  // DAAA
        eAcc_gb_tpa_wgs_prot   = e_Tpg | eAcc_wgs        | fAcc_prot, // HAA
        eAcc_gb_tpa_chromosome = e_Tpg | eAcc_chromosome | fAcc_nuc,  // GK

        eAcc_embl_tpa_nuc  = e_Tpe | eAcc_other | fAcc_nuc,  // BN
        eAcc_embl_tpa_prot = e_Tpe | eAcc_other | fAcc_prot, // CAD29848

        eAcc_ddbj_tpa_nuc  = e_Tpd | eAcc_other | fAcc_nuc,  // BR
        eAcc_ddbj_tpa_prot = e_Tpd | eAcc_other | fAcc_prot, // FAA

        // genome pipeline, modeled after RefSeq
        eAcc_gpipe_other_nuc  = e_Gpipe | eAcc_other      | fAcc_nuc,  // GPN_
        eAcc_gpipe_prot       = e_Gpipe | eAcc_other      | fAcc_prot, // GPP_
        eAcc_gpipe_scaffold   = e_Gpipe | eAcc_con        | fAcc_nuc,  // GPS_
        eAcc_gpipe_mrna       = e_Gpipe | eAcc_mrna       | fAcc_nuc,  // GPM_
        eAcc_gpipe_chromosome = e_Gpipe | eAcc_chromosome | fAcc_nuc,  // GPC_
        eAcc_gpipe_genomic    = e_Gpipe | eAcc_genomic_rgn| fAcc_nuc,  // GPG_
        eAcc_gpipe_ncrna      = e_Gpipe | eAcc_ncrna      | fAcc_nuc,  // GPR_
        eAcc_gpipe_unreserved = e_Gpipe | eAcc_unreserved,             // GPX_

        // named annotation track; mixed nucleotides and proteins
        eAcc_named_annot_track = e_Named_annot_track | eAcc_other // AT_
    };

    static E_Choice GetAccType(EAccessionInfo info)
        { return static_cast<E_Choice>(info & eAcc_type_mask); }

    /// Deduces information from a bare accession a la WHICH_db_accession;
    /// may report false negatives on properties.
    static EAccessionInfo IdentifyAccession(const string& accession);
    EAccessionInfo IdentifyAccession(void) const;

    static void LoadAccessionGuide(const string& filename);
    static void LoadAccessionGuide(ILineReader& in);

    /// Match() - TRUE if SeqIds are equivalent
    bool Match(const CSeq_id& sid2) const;

    /// Compare return values
    enum E_SIC {
        e_error = 0,  /// some problem
        e_DIFF,       /// different SeqId types-can't compare
        e_NO,         /// SeqIds compared, but are different
        e_YES         /// SeqIds compared, are equivalent
    };

    /// Compare() - more general
    E_SIC Compare(const CSeq_id& sid2) const;
    int CompareOrdered(const CSeq_id& sid2) const;
    bool operator<(const CSeq_id& sid2) const
        {
            return CompareOrdered(sid2) < 0;
        }

    /// Return embedded CTextseq_id, if any
    const CTextseq_id* GetTextseq_Id(void) const;

    /// Implement serializable interface
    virtual void WriteAsFasta(ostream& out) const;
    CProxy DumpAsFasta(void) const { return Dump(eAsFasta); }
    const string AsFastaString(void) const;

    /// return the label for a given string
    enum ELabelType {
        eType,
        eContent,
        eBoth,
        eFasta,

        /// default is to show type + content
        eDefault = eBoth
    };

    enum ELabelFlags {
        fLabel_Version            = 0x10,
        fLabel_GeneralDbIsContent = 0x20,

        /// default options - always show the version
        fLabel_Default = fLabel_Version
    };
    typedef int TLabelFlags;
    void GetLabel(string*     label,
                  ELabelType  type  = eDefault,
                  TLabelFlags flags = fLabel_Default) const;

    ///Return seqid string with optional version for text seqid type
    string GetSeqIdString(bool with_version = false) const;

    /// Get a string representation of the sequence IDs of a given bioseq.  This
    /// function produces strings in a number of possible formats.
    enum EStringFormat {
        eFormat_FastA,              // FastA format
        eFormat_ForceGI,            // GI only, in FastA format
        eFormat_BestWithoutVersion, // 'Best' accession, without the version
        eFormat_BestWithVersion     // 'Best' accession, with version
    };
    static string GetStringDescr(const CBioseq& bioseq, EStringFormat fmt);

    /// Write a bioseq's IDs in FASTA format
    /// @param ostr
    ///    Stream to write to
    /// @param bioseq
    ///    Bioseq to get IDs from
    /// @return
    ///    The stream that was passed in, after all writes occurred
    static CNcbiOstream& WriteAsFasta(CNcbiOstream& ostr,
                                      const CBioseq& bioseq);

    /// Parse an entire set of |-delimited FASTA-style IDs, appending
    /// the results to IDS.
    /// @param ids
    ///   Destination ID set.  Existing contents will be preserved and
    ///   appended to.
    /// @param s
    ///   Input string to parse.
    /// @param allow_partial_failure
    ///   If s contains invalid IDs, warn about them and try to
    ///   process the remainder of the string, rather than throwing
    ///   any exceptions.
    /// @return
    ///   The number of IDs successfully parsed.
    static SIZE_TYPE ParseFastaIds(CBioseq::TId& ids, const string& s,
                                   bool allow_partial_failure = false);

    /// Numerical quality ranking; lower is better.  (Text)Score, aka
    /// WorstRank, corresponds to the C Toolkit's SeqIdFindWorst,
    /// which favors textual accessions, whereas BestRank corresponds
    /// to the C Toolkit's SeqIdFindBest and favors GIs.  In addition,
    /// there is a pair of methods corresponding to the C Toolkit's
    /// GetOrderBySeqId, used when generating FASTA deflines.
    ///
    /// All rankings give a slight bonus to accessions that carry
    /// versions.

    int AdjustScore       (int base_score) const;
    int BaseTextScore     (void)           const;
    int BaseBestRankScore (void)           const;
    int BaseWorstRankScore(void)           const { return BaseTextScore(); }
    int BaseFastaAAScore  (void)           const;
    int BaseFastaNAScore  (void)           const;

    int TextScore     (void) const { return AdjustScore(BaseTextScore()); }
    int BestRankScore (void) const { return AdjustScore(BaseBestRankScore()); }
    int WorstRankScore(void) const { return TextScore(); }
    int FastaAAScore  (void) const { return AdjustScore(BaseFastaAAScore()); }
    int FastaNAScore  (void) const { return AdjustScore(BaseFastaNAScore()); }

    /// Wrappers for use with FindBestChoice from <corelib/ncbiutil.hpp>
    static int Score(const CRef<CSeq_id>& id)
        { return id ? id->TextScore() : kMax_Int; }
    static int BestRank(const CRef<CSeq_id>& id)
        { return id ? id->BestRankScore() : kMax_Int; }
    static int WorstRank(const CRef<CSeq_id>& id)
        { return Score(id); }
    static int FastaAARank(const CRef<CSeq_id>& id)
        { return id ? id->FastaAAScore() : kMax_Int; }
    static int FastaNARank(const CRef<CSeq_id>& id)
        { return id ? id->FastaNAScore() : kMax_Int; }

    /// Optimized implementation of CSerialObject::Assign, which is
    /// not so efficient.
    virtual void Assign(const CSerialObject& source,
                        ESerialRecursionMode how = eRecursive);

private:
    void x_Init(list<string>& fasta_pieces);

    // Prohibit copy constructor & assignment operator
    CSeq_id(const CSeq_id&);
    CSeq_id& operator= (const CSeq_id&);

    //CRef<CAbstractObjectManager> m_ObjectManager;

};


/////////////////////////////////////////////////////////////////////////////
///
/// CSeqIdException --
///
/// Define exceptions generated by CSeq_id.

class CSeqIdException : public CException
{
public:
    /// Error types that CSeq_id can generate.
    enum EErrCode {
        eUnknownType,   ///< Unrecognized Seq-id type
        eFormat         ///< Contents not parsable as expected
    };

    /// Translate from the error code value to its string representation.
    virtual const char* GetErrCodeString(void) const
    {
        switch (GetErrCode()) {
        case eUnknownType:  return "eUnknownType";
        case eFormat:       return "eFormat";
        default:            return CException::GetErrCodeString();
        }
    }

    // Standard exception boilerplate code.
    NCBI_EXCEPTION_DEFAULT(CSeqIdException, CException);
};


/// Dummy convertor for container search functions
template<class TId>
CConstRef<CSeq_id> Get_ConstRef_Seq_id(TId& id)
{
    return CConstRef<CSeq_id>(id);
}


/// Search the container of CRef<CSeq_id> for the id of given type.
/// Return the id of requested type, or null CRef.
template<class container>
CConstRef<CSeq_id> GetSeq_idByType(const container& ids,
                                   CSeq_id::E_Choice choice)
{
    ITERATE (typename container, iter, ids) {
        if ( *iter  &&  Get_ConstRef_Seq_id(*iter)->Which() == choice ) {
            return Get_ConstRef_Seq_id(*iter);
        }
    }
    return CConstRef<CSeq_id>(0);
}

/// Return gi from id list if exists, return 0 otherwise
template<class container>
int FindGi(const container& ids)
{
    CConstRef<CSeq_id> id = GetSeq_idByType(ids, CSeq_id::e_Gi);
    return id ? id->GetGi() : 0;
}


/// Return text seq-id from id list if exists, return 0 otherwise
template<class container>
CConstRef<CSeq_id> FindTextseq_id(const container& ids)
{
    ITERATE (typename container, iter, ids) {
        if ( *iter  &&  Get_ConstRef_Seq_id(*iter)->GetTextseq_Id() ) {
            return Get_ConstRef_Seq_id(*iter);
        }
    }
    return CConstRef<CSeq_id>(0);
}


/////////////////// CSeq_id inline methods

// Match - just uses Compare
inline
bool CSeq_id::Match (const CSeq_id& sid2) const
{
    return Compare(sid2) == e_YES;
}


inline
int CSeq_id::AdjustScore(int base_score) const
{
    int score = base_score * 10;
    const CTextseq_id* text_id = GetTextseq_Id();
    if (text_id) {
        if ( !text_id->IsSetVersion() ) {
            score += 4;
        }
        if ( !text_id->IsSetAccession() ) {
            score += 3;
        }
        if ( !text_id->IsSetName() ) {
            score += 2;
        }
    }
    return score;
}


inline
int CSeq_id::BaseTextScore(void) const
{
    switch (Which()) {
    case e_not_set:                                return 83;
    case e_Giim:    case e_Gi:                     return 20;
    case e_General: case e_Gibbsq: case e_Gibbmt:  return 15;
    case e_Local:   case e_Patent:                 return 10;
    case e_Gpipe:   case e_Named_annot_track:      return 9;
    case e_Other:                                  return 8;
    default:                                       return 5;
    }
}


inline
int CSeq_id::BaseBestRankScore(void) const
{
    switch (Which()) {
    case e_not_set:                               return 83;
    case e_General: case e_Local:                 return 80;
    case e_Gibbsq: case e_Gibbmt: case e_Giim:    return 70;
    case e_Named_annot_track:                     return 69;
    case e_Gpipe:                                 return 68;
    case e_Patent:                                return 67;
    case e_Other:                                 return 65;
    case e_Gi:                                    return 51;
    default:                                      return 60;
    }
}


inline
int CSeq_id::BaseFastaNAScore(void) const
{
    switch (Which()) {
        // these few are bogus, at least for nucleotide sequences
    case e_not_set: case e_Giim:
    case e_Pir: case e_Swissprot: case e_Prf:  return 255;
    case e_Local:                              return 230;
    case e_Gi:                                 return 120;
    case e_General:                            return 50;
    case e_Patent:                             return 40;
    case e_Gibbsq: case e_Gibbmt: case e_Pdb:  return 30;
    case e_Other:                              return 15;
    default: /* [third party] GB/EMBL/DDBJ */  return 20;
    }
}


inline
int CSeq_id::BaseFastaAAScore(void) const
{
    switch (Which()) {
    case e_not_set: case e_Giim:   return 255;
    case e_Local:                  return 230;
    case e_Gi:                     return 120;
    case e_General:                return 90;
    case e_Patent:                 return 80;
    case e_Prf:                    return 70;
    case e_Pdb:                    return 50;
    case e_Gibbsq: case e_Gibbmt:  return 40;
    case e_Pir:                    return 30;
    case e_Swissprot:              return 20;
    case e_Other:                  return 15;
    default:                       return 60; // [third party] GB/EMBL/DDBJ
    }
}

/////////////////// end of CSeq_id inline methods

/* @} */


END_objects_SCOPE // namespace ncbi::objects::
END_NCBI_SCOPE

#endif // OBJECTS_SEQLOC_SEQ_ID_HPP
