#ifndef OBJMGR_UTIL___CREATE_DEFLINE__HPP
#define OBJMGR_UTIL___CREATE_DEFLINE__HPP

/*
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
* Author: Jonathan Kans, Aaron Ucko
*
* ===========================================================================
*/

/// @file create_defline.hpp
/// API (CDeflineGenerator) for computing sequences' titles ("definitions").

#include <util/strsearch.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objmgr/mapped_feat.hpp>
#include <objmgr/util/feature.hpp>

/** @addtogroup ObjUtilSequence
 *
 * @{
 */

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

// Forward declarations
class CScope;
class CBioseq_Handle;

BEGIN_SCOPE(sequence)

/// Class for computing sequences' titles ("definitions").
///
/// PREFERRED USAGE:
///
/// CDeflineGenerator gen(tseh);
///
/// const string& title = gen.GenerateDefline(bsh, flags);
///
/// Same CDeflineGenerator should be used for all titles within nuc-prot set
/// blob, since it tracks presence or absence of biosource features to speed
/// up protein title generation

class NCBI_XOBJUTIL_EXPORT CDeflineGenerator
{
public:
    /// Constructor
    CDeflineGenerator (void);

    /// Constructor
    CDeflineGenerator (const CSeq_entry_Handle& tseh);

    /// Destructor
    ~CDeflineGenerator (void);

    /// User-settable flags for tuning behavior
    enum EUserFlags {
        fIgnoreExisting    = 0x1, ///< Generate fresh titles unconditionally.
        fAllProteinNames   = 0x2, ///< List all relevant proteins, not just one.
        fLocalAnnotsOnly   = 0x4, ///< Never use related sequences' annotations.
        /// Refrain from anything that could add substantial overhead.
        fNoExpensiveOps    = fLocalAnnotsOnly,
        fGpipeMode         = 0x8, ///< Use GPipe defaults.
        fOmitTaxonomicName = 0x10, ///< Do not add organism suffix to proteins.
        fDevMode           = 0x20 ///< Development mode for testing new features.
    };
    typedef int TUserFlags; ///< Binary "OR" of EUserFlags

    /// Main method
    string GenerateDefline (
        const CBioseq_Handle& bsh,
        TUserFlags flags = 0
    );

    /// Main method
    string GenerateDefline (
        const CBioseq& bioseq,
        CScope& scope,
        TUserFlags flags = 0
    );

private:
    // Prohibit copy constructor & assignment operator
    CDeflineGenerator (const CDeflineGenerator&);
    CDeflineGenerator& operator= (const CDeflineGenerator&);

private:
    /// internal methods

    void x_Init (void);

    void x_SetFlags (
        const CBioseq_Handle& bsh,
        TUserFlags flags
    );
    void x_SetBioSrc (
        const CBioseq_Handle& bsh
    );

    bool x_CDShasLowQualityException (
        const CSeq_feat& sft
    );

    void x_DescribeClones (
        vector<CTempString>& desc,
        string& buf
    );
    CConstRef<CSeq_feat> x_GetLongestProtein (
        const CBioseq_Handle& bsh
    );
    CConstRef<CGene_ref> x_GetGeneRefViaCDS (
        const CMappedFeat& mapped_cds
    );

    void x_SetTitleFromBioSrc (void);
    void x_SetTitleFromNC (void);
    void x_SetTitleFromNM (
        const CBioseq_Handle& bsh
    );
    void x_SetTitleFromNR (
        const CBioseq_Handle& bsh
    );
    void x_SetTitleFromPatent (void);
    void x_SetTitleFromPDB (void);
    void x_SetTitleFromGPipe (void);
    void x_SetTitleFromProtein (
        const CBioseq_Handle& bsh
    );
    void x_SetTitleFromSegSeq (
        const CBioseq_Handle& bsh
    );
    void x_SetTitleFromWGS (void);
    void x_SetTitleFromMap (void);

    void x_SetPrefix (
        string& prefix
    );
    void x_SetSuffix (
        string& suffix,
        const CBioseq_Handle& bsh,
        bool appendComplete
    );

    void x_AdjustProteinTitleSuffix (
        const CBioseq_Handle& bsh
    );

private:
    /// internal feature tree for parent mapping
    CRef<feature::CFeatTree> m_Feat_Tree;
    CSeq_entry_Handle m_TopSEH;
    bool m_ConstructedFeatTree;
    bool m_InitializedFeatTree;

    /// ignore existing title is forced for certain types
    bool m_Reconstruct;
    bool m_AllProtNames;
    bool m_LocalAnnotsOnly;
    bool m_GpipeMode;
    bool m_OmitTaxonomicName;
    bool m_DevMode;

    /// seq-inst fields
    bool m_IsNA;
    bool m_IsAA;
    CSeq_inst::TTopology m_Topology;

    bool m_IsSeg;
    bool m_IsDelta;
    bool m_IsVirtual;
    bool m_IsMap;

    /// seq-id fields
    bool m_IsNC;
    bool m_IsNM;
    bool m_IsNR;
    bool m_IsPatent;
    bool m_IsPDB;
    bool m_IsWP;
    bool m_ThirdParty;
    bool m_WGSMaster;
    bool m_TSAMaster;
    bool m_TLSMaster;

    string m_MainTitle;
    string m_GeneralStr;
    string m_PatentCountry;
    string m_PatentNumber;

    int m_PatentSequence;

    int m_PDBChain;

    /// molinfo fields
    CMolInfo::TBiomol m_MIBiomol;
    CMolInfo::TTech m_MITech;
    CMolInfo::TCompleteness m_MICompleteness;

    bool m_HTGTech;
    bool m_HTGSUnfinished;
    bool m_IsTLS;
    bool m_IsTSA;
    bool m_IsWGS;
    bool m_IsEST_STS_GSS;

    bool m_UseBiosrc;

    /// genbank or embl block keyword fields
    bool m_HTGSCancelled;
    bool m_HTGSDraft;
    bool m_HTGSPooled;
    bool m_TPAExp;
    bool m_TPAInf;
    bool m_TPAReasm;
    bool m_Unordered;

    /// pdb block fields
    CTempString m_PDBCompound;

    /// biosource fields
    CConstRef<CBioSource> m_Source;
    CTempString m_Taxname;
    CTempString m_Genus;
    CTempString m_Species;
    bool m_Multispecies;
    CBioSource::TGenome m_Genome;
    bool m_IsPlasmid;
    bool m_IsChromosome;

    CTempString m_Organelle;

    string m_FirstSuperKingdom;
    string m_SecondSuperKingdom;
    bool m_IsCrossKingdom;

    /// subsource fields
    CTempString m_Chromosome;
    CTempString m_Clone;
    bool m_has_clone;
    CTempString m_Map;
    CTempString m_Plasmid;
    CTempString m_Segment;

    /// orgmod fields
    CTempString m_Breed;
    CTempString m_Cultivar;
    CTempString m_Isolate;
    CTempString m_Strain;

    /// user object fields
    bool m_IsUnverified;

    /// map fields
    string m_rEnzyme;

    /// exception fields

    /// (Careful: CTextFsm has no virtual destructor)
    class CLowQualityTextFsm : public CTextFsm<int> {
    public:
        CLowQualityTextFsm(void);
    };

    static CSafeStatic<CLowQualityTextFsm> ms_p_Low_Quality_Fsa;
};


END_SCOPE(sequence)
END_SCOPE(objects)
END_NCBI_SCOPE

/* @} */

#endif /* OBJMGR_UTIL___CREATE_DEFLINE__HPP */
