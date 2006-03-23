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
 * Author:  Robert G. Smith
 *
 * File Description:
 *   Implementation of BasicCleanup for Seq-feat and sub-objects.
 *
 */

#include <ncbi_pch.hpp>
#include "cleanup_utils.hpp"
#include <objects/general/Object_id.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/seqfeat/Cdregion.hpp>
#include <objects/seqfeat/Genetic_code.hpp>
#include <objects/seqfeat/Gb_qual.hpp>
#include <objects/seqfeat/Imp_feat.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/SeqFeatXref.hpp>
#include <util/static_map.hpp>

#include <objects/seqfeat/RNA_ref.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/general/User_object.hpp>
#include <objects/general/User_field.hpp>
#include <objects/seq/seqport_util.hpp>
#include <vector>

#include "cleanupp.hpp"


BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::



// ==========================================================================
//                             CSeq_feat Cleanup section
// ==========================================================================

void CCleanup_imp::x_CleanupExcept_text(string& except_text)
{
    if (NStr::Find(except_text, "ribosome slippage") == NPOS     &&
        NStr::Find(except_text, "trans splicing") == NPOS        &&
        NStr::Find(except_text, "alternate processing") == NPOS  &&
        NStr::Find(except_text, "non-consensus splice site") == NPOS) {
        return;
    }

    vector<string> exceptions;
    NStr::Tokenize(except_text, ",", exceptions);

    NON_CONST_ITERATE(vector<string>, it, exceptions) {
        string& text = *it;
        NStr::TruncateSpacesInPlace(text);
        if (!text.empty()) {
            if (text == "ribosome slippage") {
                text = "ribosomal slippage";
            } else if (text == "trans splicing") {
                text = "trans-splicing";
            } else if (text == "alternate processing") {
                text = "alternative processing";
            } else if (text == "non-consensus splice site") {
                text = "nonconsensus splice site";
            }
        }
    }
    except_text = NStr::Join(exceptions, ",");
}


// === Gene

static bool s_IsEmptyGeneRef(const CGene_ref& gref)
{
    return (!gref.IsSetLocus()  &&  !gref.IsSetAllele()  &&
        !gref.IsSetDesc()  &&  !gref.IsSetMaploc()  &&  !gref.IsSetDb()  &&
        !gref.IsSetSyn()  &&  !gref.IsSetLocus_tag());
}


void CCleanup_imp::x_MoveDbxrefToFeat(CSeq_feat& feat)
{
    CSeq_feat::TData::TGene& g = feat.SetData().SetGene();

    // move db_xref from Gene-ref to feature
    if (g.IsSetDb()) {
        copy(g.GetDb().begin(), g.GetDb().end(), back_inserter(feat.SetDbxref()));
        g.ResetDb();
    }

    // move db_xref from gene xrefs to feature
    if (feat.IsSetXref()) {
        CSeq_feat::TXref& xrefs = feat.SetXref();
        CSeq_feat::TXref::iterator it = xrefs.begin();
        while (it != xrefs.end()) {
            CSeqFeatXref& xref = **it;
            if (xref.IsSetData()  &&  xref.GetData().IsGene()) {
                CGene_ref& gref = xref.SetData().SetGene();
                if (gref.IsSetDb()) {
                    copy(gref.GetDb().begin(), gref.GetDb().end(), 
                        back_inserter(feat.SetDbxref()));
                    gref.ResetDb();
                }
                // remove gene xref if it has no values set
                if (s_IsEmptyGeneRef(gref)) {
                    it = xrefs.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}


void CCleanup_imp::x_CleanupGene(CSeq_feat& feat)
{
    _ASSERT(feat.IsSetData()  &&  feat.GetData().IsGene());

    CSeqFeatData::TGene& gene = feat.SetData().SetGene();

    // remove feat.comment if equal to gene.locus
    if (gene.IsSetLocus()  &&  feat.IsSetComment()) {
        if (feat.GetComment() == gene.GetLocus()) {
            feat.ResetComment();
        }
    }

    // move db_xrefs from the Gene-ref or gene Xrefs to the feature
    x_MoveDbxrefToFeat(feat);
}


// === Site

typedef pair<const string, CSeqFeatData::TSite>  TSiteElem;
static const TSiteElem sc_site_map[] = {
    TSiteElem("acetylation", CSeqFeatData::eSite_acetylation),
    TSiteElem("active", CSeqFeatData::eSite_active),
    TSiteElem("amidation", CSeqFeatData::eSite_amidation),
    TSiteElem("binding", CSeqFeatData::eSite_binding),
    TSiteElem("blocked", CSeqFeatData::eSite_blocked),
    TSiteElem("cleavage", CSeqFeatData::eSite_cleavage),
    TSiteElem("dna binding", CSeqFeatData::eSite_dna_binding),
    TSiteElem("dna-binding", CSeqFeatData::eSite_dna_binding),
    TSiteElem("gamma carboxyglutamic acid", CSeqFeatData::eSite_gamma_carboxyglutamic_acid),
    TSiteElem("gamma-carboxyglutamic-acid", CSeqFeatData::eSite_gamma_carboxyglutamic_acid),
    TSiteElem("glycosylation", CSeqFeatData::eSite_glycosylation),
    TSiteElem("hydroxylation", CSeqFeatData::eSite_hydroxylation),
    TSiteElem("inhibit", CSeqFeatData::eSite_inhibit),
    TSiteElem("lipid binding", CSeqFeatData::eSite_lipid_binding),
    TSiteElem("lipid-binding", CSeqFeatData::eSite_lipid_binding),
    TSiteElem("metal binding", CSeqFeatData::eSite_metal_binding),
    TSiteElem("metal-binding", CSeqFeatData::eSite_metal_binding),
    TSiteElem("methylation", CSeqFeatData::eSite_methylation),
    TSiteElem("modified", CSeqFeatData::eSite_modified),
    TSiteElem("mutagenized", CSeqFeatData::eSite_mutagenized),
    TSiteElem("myristoylation", CSeqFeatData::eSite_myristoylation),
    TSiteElem("np binding", CSeqFeatData::eSite_np_binding),
    TSiteElem("np-binding", CSeqFeatData::eSite_np_binding),
    TSiteElem("oxidative deamination", CSeqFeatData::eSite_oxidative_deamination),
    TSiteElem("oxidative-deamination", CSeqFeatData::eSite_oxidative_deamination),
    TSiteElem("phosphorylation", CSeqFeatData::eSite_phosphorylation),
    TSiteElem("pyrrolidone carboxylic acid", CSeqFeatData::eSite_pyrrolidone_carboxylic_acid),
    TSiteElem("pyrrolidone-carboxylic-acid", CSeqFeatData::eSite_pyrrolidone_carboxylic_acid),
    TSiteElem("signal peptide", CSeqFeatData::eSite_signal_peptide),
    TSiteElem("signal-peptide", CSeqFeatData::eSite_signal_peptide),
    TSiteElem("sulfatation", CSeqFeatData::eSite_sulfatation),
    TSiteElem("transit peptide", CSeqFeatData::eSite_transit_peptide),
    TSiteElem("transit-peptide", CSeqFeatData::eSite_transit_peptide),
    TSiteElem("transmembrane region", CSeqFeatData::eSite_transmembrane_region),
    TSiteElem("transmembrane-region", CSeqFeatData::eSite_transmembrane_region)
};
typedef CStaticArrayMap<const string, CSeqFeatData::TSite>   TSiteMap;
static const TSiteMap sc_SiteMap(sc_site_map, sizeof(sc_site_map));


void CCleanup_imp::x_CleanupSite(CSeq_feat& feat)
{
    _ASSERT(feat.IsSetData()  &&  feat.SetData().IsSite());

    CSeqFeatData::TSite site = feat.GetData().GetSite();
    if (feat.IsSetComment()  &&
        (site == CSeqFeatData::TSite(0)  ||  site == CSeqFeatData::eSite_other)) {
        const string& comment = feat.GetComment();
        ITERATE (TSiteMap, it, sc_SiteMap) {
            if (NStr::StartsWith(comment, it->first, NStr::eNocase)) {
                feat.SetData().SetSite(it->second);
                if (NStr::IsBlank(comment, it->first.length())  ||
                    NStr::EqualNocase(comment, it->first.length(), NPOS, " site")) {
                    feat.ResetComment();
                }
            }
        }
    }
}


// === Prot

static const string uninf_names[] = {
    "peptide", "putative", "signal", "signal peptide", "signal-peptide",
    "signal_peptide", "transit", "transit peptide", "transit-peptide",
    "transit_peptide", "unknown", "unnamed"
};
typedef CStaticArraySet<string, PNocase> TUninformative;
static const TUninformative sc_UninfNames(uninf_names, sizeof(uninf_names));

static bool s_IsInformativeName(const string& name)
{
    return sc_UninfNames.find(name) == sc_UninfNames.end();
}


void CCleanup_imp::x_CleanupProt(CSeq_feat& feat)
{
    _ASSERT(feat.IsSetData()  &&  feat.GetData().IsProt());

    CSeq_feat::TData::TProt& prot = feat.SetData().SetProt();

    if (prot.IsSetProcessed()  &&  prot.IsSetName()) {
        CProt_ref::TProcessed processed = prot.GetProcessed();
        CProt_ref::TName& name = prot.SetName();
        if (processed == CProt_ref::eProcessed_signal_peptide  ||
            processed == CProt_ref::eProcessed_transit_peptide) {
            CProt_ref::TName::iterator it = name.begin();
            while (it != name.end()) {
                if (!feat.IsSetComment()) {
                    if (NStr::Find(*it, "putative") != NPOS  ||
                        NStr::Find(*it, "put. ") != NPOS) {
                        feat.SetComment("putative");
                    }
                }
                // remove uninformative names
                if (!s_IsInformativeName(*it)) {
                    it = name.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    // move Prot-ref.db to Seq-feat.dbxref
    if (prot.IsSetDb()) {
        copy(prot.GetDb().begin(), prot.GetDb().end(),
            back_inserter(feat.SetDbxref()));
        prot.ResetDb();
    }
}


// === RNA

void CCleanup_imp::x_CleanupRna(CSeq_feat& feat)
{
    _ASSERT(feat.IsSetData()  &&  feat.GetData().IsRna());

    CSeq_feat::TData& data = feat.SetData();
    CSeq_feat::TData::TRna& rna = data.SetRna();
    
    /*if (rna.IsSetExt()  &&  rna.GetExt().IsName()) {
        string &name = rna.SetExt().SetName();
    }*/

    // !!! more?
}


// === Imp

void CCleanup_imp::x_AddReplaceQual(CSeq_feat& feat, const string& str)
{
    if (!NStr::EndsWith(str, ')')) {
        return;
    }

    SIZE_TYPE start = str.find_first_of('\"');
    if (start != NPOS) {
        SIZE_TYPE end = str.find_first_of('\"', start + 1);
        if (end != NPOS) {
            feat.AddQualifier("replace", str.substr(start + 1, end));
        }
    }
}

typedef pair<const string, CRNA_ref::TType> TRnaTypePair;
static const TRnaTypePair sc_rna_type_map[] = {
    TRnaTypePair("mRNA", CRNA_ref::eType_premsg),
    TRnaTypePair("misc_RNA", CRNA_ref::eType_other),
    TRnaTypePair("precursor_RNA", CRNA_ref::eType_premsg),
    TRnaTypePair("rRNA", CRNA_ref::eType_tRNA),
    TRnaTypePair("scRNA", CRNA_ref::eType_scRNA),
    TRnaTypePair("snRNA", CRNA_ref::eType_snRNA),
    TRnaTypePair("snoRNA", CRNA_ref::eType_snoRNA),
    TRnaTypePair("tRNA", CRNA_ref::eType_mRNA)
};
typedef CStaticArrayMap<const string, CRNA_ref::TType> TRnaTypeMap;
static const TRnaTypeMap sc_RnaTypeMap(sc_rna_type_map, sizeof(sc_rna_type_map));

void CCleanup_imp::x_CleanupImp(CSeq_feat& feat, bool is_embl_or_ddbj)
{
    _ASSERT(feat.IsSetData()  &&  feat.GetData().IsImp());

    CSeqFeatData& data = feat.SetData();
    CSeqFeatData::TImp& imp = data.SetImp();

    if (imp.IsSetLoc()  &&  (NStr::Find(imp.GetLoc(), "replace") != NPOS)) {
        x_AddReplaceQual(feat, imp.GetLoc());
        imp.ResetLoc();
    }

    if (imp.IsSetKey()) {
        const CImp_feat::TKey& key = imp.GetKey();

        if (key == "CDS") {
            if (!is_embl_or_ddbj) {
                data.SetCdregion();
                //s_CleanupCdregion(feat);
            }
        } else if (!imp.IsSetLoc()  ||  NStr::IsBlank(imp.GetLoc())) {
            TRnaTypeMap::const_iterator rna_type_it = sc_RnaTypeMap.find(key);
            if (rna_type_it != sc_RnaTypeMap.end()) {
                CSeqFeatData::TRna& rna = data.SetRna();
                rna.SetType(rna_type_it->second);
                x_CleanupRna(feat);
            } else {
                // !!! need to find protein bioseq without object manager
            }
        }
    }
}


// === Region

void CCleanup_imp::x_CleanupRegion(CSeq_feat& feat)
{
    _ASSERT(feat.IsSetData()  &&  feat.GetData().IsRegion());

    string &region = feat.SetData().SetRegion();
    CleanString(region);
    ConvertDoubleQuotes(region);
    if (region.empty()) {
        feat.SetData().SetComment();
    }
}


// === Seq-feat.data

void CCleanup_imp::BasicCleanup(CSeqFeatData& data) 
{    
    // basic localized cleanup of kinds of CSeqFeatData.
    switch (data.Which()) {
        case CSeqFeatData::e_Gene:
            BasicCleanup(data.SetGene());
            break;
        case CSeqFeatData::e_Org:
            break;
        case CSeqFeatData::e_Cdregion:
            break;
        case CSeqFeatData::e_Prot:
            BasicCleanup(data.SetProt());
            break;
        case CSeqFeatData::e_Rna:
            BasicCleanup(data.SetRna());
            break;
        case CSeqFeatData::e_Pub:
            break;
        case CSeqFeatData::e_Seq:
            break;
        case CSeqFeatData::e_Imp:
            BasicCleanup(data.SetImp());
            break;
        case CSeqFeatData::e_Region:
            break;
        case CSeqFeatData::e_Comment:
            break;
        case CSeqFeatData::e_Bond:
            break;
        case CSeqFeatData::e_Site:
            break;
        case CSeqFeatData::e_Rsite:
            break;
        case CSeqFeatData::e_User:
            BasicCleanup(data.SetUser());
            break;
        case CSeqFeatData::e_Txinit:
            break;
        case CSeqFeatData::e_Num:
            break;
        case CSeqFeatData::e_Psec_str:
            break;
        case CSeqFeatData::e_Non_std_residue:
            break;
        case CSeqFeatData::e_Het:
            break;
        case CSeqFeatData::e_Biosrc:
            BasicCleanup(data.SetBiosrc());
            break;
        default:
            break;
    }
}


void CCleanup_imp::x_CleanupData(CSeq_feat& feat, bool is_embl_or_ddbj) 
{
    // change things in the feat based on what is in data and vice versa.
    // does not call any other BasicCleanup routine.
    _ASSERT(feat.IsSetData());

    CSeq_feat::TData& data = feat.SetData();

    switch (data.Which()) {
    case CSeqFeatData::e_Gene:
        x_CleanupGene(feat);
        break;
    case CSeqFeatData::e_Org:
        break;
    case CSeqFeatData::e_Cdregion:
        break;
    case CSeqFeatData::e_Prot:
        x_CleanupProt(feat);
        break;
    case CSeqFeatData::e_Rna:
        // x_CleanupRna(feat); // doesn't do anything at this time.
        break;
    case CSeqFeatData::e_Pub:
        break;
    case CSeqFeatData::e_Seq:
        break;
    case CSeqFeatData::e_Imp:
        x_CleanupImp(feat, is_embl_or_ddbj);
        break;
    case CSeqFeatData::e_Region:
        x_CleanupRegion(feat);
        break;
    case CSeqFeatData::e_Comment:
        break;
    case CSeqFeatData::e_Bond:
        break;
    case CSeqFeatData::e_Site:
        x_CleanupSite(feat);
        break;
    case CSeqFeatData::e_Rsite:
        break;
    case CSeqFeatData::e_User:
        break;
    case CSeqFeatData::e_Txinit:
        break;
    case CSeqFeatData::e_Num:
        break;
    case CSeqFeatData::e_Psec_str:
        break;
    case CSeqFeatData::e_Non_std_residue:
        break;
    case CSeqFeatData::e_Het:
        break;
    case CSeqFeatData::e_Biosrc:
        break;
    default:
        break;
    }
}


// seq-feat.qual

void CCleanup_imp::x_TrimParenthesesAndCommas(string& str)
{
    string::iterator it;
    for (it = str.begin(); it != str.end(); ++it) {
        char ch = *it;
        if (ch == '\0'  ||  (ch >= ' ' &&  ch != '('  &&  ch != ',')) {
            break;
        }
    }

    if (it != str.end()) {
        str.erase(str.begin(), it);
    }

    for (it = str.end(); it != str.begin(); --it) {
        char ch = *it;
        if (ch == '\0'  ||  (ch >= ' ' &&  ch != '('  &&  ch != ',')) {
            break;
        }
    }

    if (it != str.begin()) {
        str.erase(it);
    }
}


void CCleanup_imp::x_CombineSplitQual(string& val, string& new_val)
{
    if (NStr::Find(val, new_val) != NPOS) {
        return;
    }

    x_TrimParenthesesAndCommas(val);
    x_TrimParenthesesAndCommas(new_val);

    val.insert(0, "(");
    ((val += ',') += new_val) += ')';
}


bool CCleanup_imp::x_HandleGbQualOnGene(CSeq_feat& feat, const string& qual, const string& val)
{
    _ASSERT(feat.GetData().IsGene());

    CGene_ref& gene = feat.SetData().SetGene();

    bool retval = true;
    if (NStr::EqualNocase(qual, "map")) {
        if (gene.IsSetMaploc()  ||  NStr::IsBlank(val)) {
            retval = false;
        } else {
            gene.SetMaploc(val);
        }
    } else if (NStr::EqualNocase(qual, "allele")) {
        if (gene.IsSetAllele()  ||  NStr::IsBlank(val)) {
            retval = false;
        } else {
            gene.SetAllele(val);
        }
    } else if (NStr::EqualNocase(qual, "locus_tag")) {
        if (gene.IsSetLocus_tag()  ||  NStr::IsBlank(val)) {
            retval = false;
        } else {
            gene.SetLocus_tag(val);
        }
    }

    return retval;
}


bool CCleanup_imp::x_HandleGbQualOnCDS(CSeq_feat& feat, const string& qual, const string& val)
{
    _ASSERT(feat.GetData().IsCdregion());

    CSeq_feat::TData::TCdregion& cds = feat.SetData().SetCdregion();

    // transl_except qual -> Cdregion.code_break
    if (NStr::EqualNocase(qual, "transl_except")) {
        return false ; // s_ParseCodeBreak(feat, val);
    }

    // codon_start qual -> Cdregion.frame
    if (NStr::EqualNocase(qual, "codon_start")) {
        CCdregion::TFrame frame = cds.GetFrame();
        CCdregion::TFrame new_frame = CCdregion::TFrame(NStr::StringToNumeric(val));
        if (new_frame == CCdregion::eFrame_one  ||
            new_frame == CCdregion::eFrame_two  ||
            new_frame == CCdregion::eFrame_three) {
            if (frame == CCdregion::eFrame_not_set  ||
                (feat.IsSetPseudo()  &&  feat.GetPseudo()  &&  !feat.IsSetProduct())) {
                cds.SetFrame(new_frame);
            }
            return true;
        }
    }

    // transl_table qual -> Cdregion.code
    if (NStr::EqualNocase(qual, "transl_table")) {
        if (cds.IsSetCode()) {
            const CCdregion::TCode& code = cds.GetCode();
            int transl_table = 1;
            ITERATE (CCdregion::TCode::Tdata, it, code.Get()) {
                if ((*it)->IsId()  &&  (*it)->GetId() != 0) {
                    transl_table = (*it)->GetId();
                    break;
                }
            }
            
            if (NStr::EqualNocase(NStr::UIntToString(transl_table), val)) {
                return true;
            }
        } else {
            int new_val = NStr::StringToNumeric(val);
            if (new_val > 0) {
                CRef<CGenetic_code::C_E> gc(new CGenetic_code::C_E);
                gc->SetId(new_val);
                cds.SetCode().Set().push_back(gc);
                return true;
            }
        }
    }

    /*if (NStr::EqualNocase(qual, "translation")) {
        return TRUE;
    }*/
    return false;
}


bool CCleanup_imp::x_HandleGbQualOnRna(CSeq_feat& feat, const string& qual, const string& val, bool is_embl_or_ddbj)
{
    _ASSERT(feat.GetData().IsRna());

    CSeq_feat::TData::TRna& rna = feat.SetData().SetRna();

    bool is_std_name = NStr::EqualNocase(qual, "standard_name");
    if (NStr::EqualNocase(qual, "product")  ||  (is_std_name  &&  !is_embl_or_ddbj)) {
        if (rna.IsSetType()) {
            if (rna.GetType() == CRNA_ref::eType_unknown) {
                rna.SetType(CRNA_ref::eType_other);
            }
        } else {
            rna.SetType(CRNA_ref::eType_other);
        }
        _ASSERT(rna.IsSetType());

        CRNA_ref::TType type = rna.GetType();
        
        if (type == CRNA_ref::eType_other  &&  is_std_name) {
            return false;
        }

        if (type == CRNA_ref::eType_tRNA  &&  rna.IsSetExt()  &&  rna.GetExt().IsName()) {
            //!!! const string& name = rna.GetExt().GetName();
        }
    }
    return false;
}


bool CCleanup_imp::x_HandleGbQualOnProt(CSeq_feat& feat, const string& qual, const string& val)
{
    _ASSERT(feat.GetData().IsProt());
    
    CSeq_feat::TData::TProt& prot = feat.SetData().SetProt();

    if (NStr::EqualNocase(qual, "product")  ||  NStr::EqualNocase(qual, "standard_name")) {
        if (!prot.IsSetName()  ||  NStr::IsBlank(prot.GetName().front())) {
            prot.SetName().push_back(val);
            if (prot.IsSetDesc()) {
                const CProt_ref::TDesc& desc = prot.GetDesc();
                ITERATE (CProt_ref::TName, it, prot.GetName()) {
                    if (NStr::EqualNocase(desc, *it)) {
                        prot.ResetDesc();
                        break;
                    }
                }
            }
            return true;
        }
    } else if (NStr::EqualNocase(qual, "function")) {
        prot.SetActivity().push_back(val);
        return true;
    } else if (NStr::EqualNocase(qual, "EC_number")) {
        prot.SetEc().push_back(val);
        return true;
    }

    return false;
}


static bool s_IsCompoundRptTypeValue( 
    const string& value )
//
//  Format of compound rpt_type values: (value[,value]*)
//
//  These are internal to sequin and are in theory cleaned up before the material
//  is released. However, some compound values have escaped into the wild and have 
//  not been retro-fixed yet (as of 2006-03-17).
//
{
    size_t length = value.length();
    return ( NStr::StartsWith( value, '(' ) && NStr::EndsWith( value, ')' ) );
};


static void s_CleanupQualRptType( 
    CSeq_feat::TQual& quals, 
    CSeq_feat::TQual::iterator& it )
//
//  Rules for "rpt_type" qualifiers (as of 2006-03-07):
//
//  There can be multiple occurrences of this qualifier, and we need to keep them 
//  all.
//  The value of this qualifier can also be a *list of values* which is *not* 
//  conforming to the ASN.1 and thus needs to be cleaned up. 
//
//  The cleanup entails turning the list of values into multiple occurrences of the 
//  rpt_type qualifier, each occurrence taking one of the values in the original 
//  list.
//
{
    CGb_qual& qual = **it;
    string& val = qual.SetVal();
    if ( ! s_IsCompoundRptTypeValue( val ) ) {
        //
        //  nothing to do ...
        //
        return;
    }

    //
    //  Generate list of cleaned up values. Fix original qualifier and generate 
    //  list of new qualifiers to be added to the original list:
    //    
    vector< string > newValues;
    string valueList = val.substr(1, val.length() - 2);
    NStr::Tokenize(valueList, ",", newValues);
    
    qual.SetVal( newValues[0] );
    
    vector< CRef< CGb_qual > > newQuals;
    for ( size_t i=1; i < newValues.size(); ++i ) {
        CRef< CGb_qual > newQual( new CGb_qual() );
        newQual->SetQual( "rpt_type" );
        newQual->SetVal( newValues[i] );
        newQuals.push_back( newQual ); 
    }
    
    //
    //  Integrate new rpt_type qualifiers into the original qualifier list and 
    //  adjust the cleanup iterator past the new (already clean) elements in the 
    //  list:
    //
    if ( newQuals.size() > 0 ) {
        size_t initialPos = it - quals.begin();
        quals.insert( it, newQuals.begin(), newQuals.end() );
        it = quals.begin() + initialPos + newQuals.size();
    }
};


void CCleanup_imp::x_CleanupQual(CSeq_feat& feat, bool is_embl_or_ddbj)
{
    _ASSERT(feat.IsSetQual()  &&  feat.IsSetData());

    CSeq_feat::TQual& qual = feat.SetQual();
    CSeq_feat::TData& data = feat.SetData();

    vector<CSeq_feat::TQual::iterator> remove;

    CRef<CGb_qual> rpt_type, rpt_unit, usedin, old_locus_tag, compare;

    NON_CONST_ITERATE (CSeq_feat::TQual, it, qual) {
        CGb_qual& gb_qual = **it;

        _ASSERT(gb_qual.IsSetQual()  &&  gb_qual.IsSetVal());

        string& qual = gb_qual.SetQual();
        string& val  = gb_qual.SetVal();

        // 'replace' qualifier
        if (NStr::EqualNocase(qual, "replace")) {
            if (data.IsImp()  &&  data.GetImp().IsSetKey()) {
                CSeq_feat::TData::TImp& imp = feat.SetData().SetImp();
                if (NStr::EqualNocase(imp.GetKey(), "variation")) {
                    NStr::ToLower(val);
                }
            }
        }

        if (NStr::EqualNocase(qual, "partial")) {
            feat.SetPartial();
            remove.push_back(it);  // mark qual for deletion
        } else if (NStr::EqualNocase(qual, "evidence")) {
            if (NStr::EqualNocase(val, "experimental")) {
                if (!feat.IsSetExp_ev()  ||  feat.GetExp_ev() != CSeq_feat::eExp_ev_not_experimental) {
                    feat.SetExp_ev(CSeq_feat::eExp_ev_experimental);
                }
            } else if (NStr::EqualNocase(val, "not_experimental")) {
                feat.SetExp_ev(CSeq_feat::eExp_ev_not_experimental);
            }
            remove.push_back(it);  // mark qual for deletion
        } else if (NStr::EqualNocase(qual, "exception")) {
            feat.SetExcept(true);
            if (!NStr::IsBlank(val)  &&  !NStr::EqualNocase(val, "true")) {
                if (!feat.IsSetExcept_text()) {
                    feat.SetExcept_text(val);
                }
            }
            remove.push_back(it);  // mark qual for deletion
        } else if (NStr::EqualNocase(qual, "note")) {
            if (!feat.IsSetComment()) {
                feat.SetComment(val);
            } else {
                (feat.SetComment() += "; ") += val;
            }
            remove.push_back(it);  // mark qual for deletion
        } else if (NStr::EqualNocase(qual, "db_xref")) {
            string tag, db;
            if (NStr::SplitInTwo(val, ":", db, tag)) {
                CRef<CDbtag> dbp(new CDbtag);
                dbp->SetDb(db);
                dbp->SetTag().SetStr(tag);
                feat.SetDbxref().push_back(dbp);
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (NStr::EqualNocase(qual, "gdb_xref")) {
            CRef<CDbtag> dbp(new CDbtag);
            dbp->SetDb("GDB");
            dbp->SetTag().SetStr(val);
            feat.SetDbxref().push_back(dbp);
            remove.push_back(it);  // mark qual for deletion
        } else if (NStr::EqualNocase(qual, "rpt_type")) {
        
            s_CleanupQualRptType( feat.SetQual(), it ); 
            
        } else if (NStr::EqualNocase(qual, "rpt_unit")) {
            if (rpt_unit.Empty()) {
                rpt_unit.Reset(&gb_qual);
            } else {
                x_CombineSplitQual(rpt_unit->SetVal(), val);
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (NStr::EqualNocase(qual, "usedin")) {
            if (usedin.Empty()) {
                usedin.Reset(&gb_qual);
            } else {
                x_CombineSplitQual(usedin->SetVal(), val);
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (NStr::EqualNocase(qual, "old_locus_tag")) {
            if (old_locus_tag.Empty()) {
                old_locus_tag.Reset(&gb_qual);
            } else {
                x_CombineSplitQual(old_locus_tag->SetVal(), val);
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (NStr::EqualNocase(qual, "compare")) {
            if (compare.Empty()) {
                compare.Reset(&gb_qual);
            } else {
                x_CombineSplitQual(compare->SetVal(), val);
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (NStr::EqualNocase(qual, "pseudo")) {
            feat.SetPseudo(true);
            remove.push_back(it);  // mark qual for deletion
        } else if (NStr::EqualNocase(qual, "gene")) {
            if (!NStr::IsBlank(val)) {
                CRef<CSeqFeatXref> xref(new CSeqFeatXref);
                xref->SetData().SetGene().SetLocus(val);
                feat.SetXref().push_back(xref);
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (NStr::EqualNocase(qual, "codon_start")) {
            if (!data.IsCdregion()) {
                // not legal on anything but CDS, so remove it
                remove.push_back(it);  // mark qual for deletion
            }
        } else if (data.IsGene()  &&  x_HandleGbQualOnGene(feat, qual, val)) {
            remove.push_back(it);  // mark qual for deletion
        } else if (data.IsCdregion()  &&  x_HandleGbQualOnCDS(feat, qual, val)) {
            remove.push_back(it);  // mark qual for deletion
        } else if (data.IsRna()  &&  x_HandleGbQualOnRna(feat, qual, val, is_embl_or_ddbj)) {
            remove.push_back(it);  // mark qual for deletion
        } else if (data.IsProt()  &&  x_HandleGbQualOnProt(feat, qual, val)) {
            remove.push_back(it);  // mark qual for deletion
        }
    }

    if (rpt_unit.NotEmpty()) {
        // re-trigger cleanup for newly constructed rpt_unit qualifier
        BasicCleanup(*rpt_unit);
    }

    // delete all marked quals
    ITERATE (vector<CSeq_feat::TQual::iterator>, it, remove) {
        feat.SetQual().erase(*it);
    }
}


// === Seq-feat.dbxref


struct SDbtagCompare
{
    // is dbt1 < dbt2
    bool operator()(const CRef<CDbtag>& dbt1, const CRef<CDbtag>& dbt2) {
        return dbt1->Compare(*dbt2) < 0;
    }
};


struct SDbtagEqual
{
    // is dbt1 < dbt2
    bool operator()(const CRef<CDbtag>& dbt1, const CRef<CDbtag>& dbt2) {
        return dbt1->Compare(*dbt2) == 0;
    }
};


void CCleanup_imp::x_CleanupDbxref(CSeq_feat& feat)
{
    _ASSERT(feat.IsSetDbxref());

    CSeq_feat::TDbxref& dbxref = feat.SetDbxref();

    // dbxrefs cleanup
    CSeq_feat::TDbxref::iterator it = dbxref.begin();
    while (it != dbxref.end()) {
        if (it->Empty()) {
            it = dbxref.erase(it);
            continue;
        }
        BasicCleanup(**it);

        ++it;
    }

    // sort/unique db_xrefs
    stable_sort(dbxref.begin(), dbxref.end(), SDbtagCompare());
    it = unique(dbxref.begin(), dbxref.end(), SDbtagEqual());
    dbxref.erase(it, dbxref.end());
}   


// BasicCleanup
void CCleanup_imp::BasicCleanup(CSeq_feat& f, ECleanupMode mode)
{
    if (!f.IsSetData()) {
        return;
    }

    CLEAN_STRING_MEMBER(f, Comment);
    if (f.IsSetComment()  &&  f.GetComment() == ".") {
        f.ResetComment();
    }
    CLEAN_STRING_MEMBER(f, Title);
    CLEAN_STRING_MEMBER(f, Except_text);
    if (f.IsSetExcept_text()) {
        x_CleanupExcept_text(f.SetExcept_text());
    }

    bool is_embl_or_ddbj = (mode == eCleanup_EMBL  ||  mode == eCleanup_DDBJ);

    BasicCleanup(f.SetData());
    x_CleanupData(f, is_embl_or_ddbj);

    if (f.IsSetDbxref()) {
       x_CleanupDbxref(f);
    }
    if (f.IsSetQual()) {
        NON_CONST_ITERATE (CSeq_feat::TQual, it, f.SetQual()) {
            CGb_qual& gb_qual = **it;
            BasicCleanup(gb_qual);
        }
        x_CleanupQual(f, is_embl_or_ddbj);
    }
}

// ==========================================================================
//                             end of Seq_feat cleanup section
// ==========================================================================




void CCleanup_imp::BasicCleanup(CGene_ref& gene_ref)
{
    CLEAN_STRING_MEMBER(gene_ref, Locus);
    CLEAN_STRING_MEMBER(gene_ref, Allele);
    CLEAN_STRING_MEMBER(gene_ref, Desc);
    CLEAN_STRING_MEMBER(gene_ref, Maploc);
    CLEAN_STRING_MEMBER(gene_ref, Locus_tag);
    CLEAN_STRING_LIST(gene_ref, Syn);
    
    // remove synonyms equal to locus
    if (gene_ref.IsSetLocus()  &&  gene_ref.IsSetSyn()) {
        const CGene_ref::TLocus& locus = gene_ref.GetLocus();
        CGene_ref::TSyn& syns = gene_ref.SetSyn();
        
        CGene_ref::TSyn::iterator it = syns.begin();
        while (it != syns.end()) {
            if (locus == *it) {
                it = syns.erase(it);
            } else {
                ++it;
            }
        }
    }
}


// perform basic cleanup functionality (trim spaces from strings etc.)
void CCleanup_imp::BasicCleanup(CProt_ref& prot_ref)
{
    CLEAN_STRING_MEMBER(prot_ref, Desc);
    CLEAN_STRING_LIST(prot_ref, Name);
    CLEAN_STRING_LIST(prot_ref, Ec);
    CLEAN_STRING_LIST(prot_ref, Activity);
    
    if (prot_ref.IsSetProcessed()  &&  !prot_ref.IsSetName()) {
        CProt_ref::TProcessed processed = prot_ref.GetProcessed();
        if (processed == CProt_ref::eProcessed_preprotein  ||  
            processed == CProt_ref::eProcessed_mature) {
            prot_ref.SetName().push_back("unnamed");
        }
    }
}


// RNA_ref basic cleanup 
void CCleanup_imp::BasicCleanup(CRNA_ref& rr)
{
    if (rr.IsSetExt()) {
        CRNA_ref::TExt& ext = rr.SetExt();
        switch (ext.Which()) {
            case CRNA_ref::TExt::e_Name:
                x_CleanupExtName(rr);
                break;
            case CRNA_ref::TExt::e_TRNA:
                x_CleanupExtTRNA(rr);
                break;
            default:
                break;
        }
    }
}


void CCleanup_imp::x_CleanupExtName(CRNA_ref& rr)
{
    static const string rRNA = " rRNA";
    static const string kRibosomalrRna = " ribosomal rRNA";
    
    _ASSERT(rr.IsSetExt()  &&  rr.GetExt().IsName());
    
    string& name = rr.SetExt().SetName();
    CleanString(name);
    
    if (name.empty()) {
        rr.ResetExt();
    } else if (rr.IsSetType()) {
        switch (rr.GetType()) {
            case CRNA_ref::eType_rRNA:
            {{
                size_t len = name.length();
                if (len >= rRNA.length()                       &&
                    NStr::EndsWith(name, rRNA, NStr::eNocase)  &&
                    !NStr::EndsWith(name, kRibosomalrRna, NStr::eNocase)) {
                    name.replace(len - rRNA.length(), name.size(), kRibosomalrRna);
                }
                break;
            }}
            case CRNA_ref::eType_tRNA:
            {{
                // !!!
                break;
            }}
            case CRNA_ref::eType_other:
            {{
                if (NStr::EqualNocase(name, "its1")) {
                    name = "internal transcribed spacer 1";
                } else if (NStr::EqualNocase(name, "its2")) {
                    name = "internal transcribed spacer 2";
                } else if (NStr::EqualNocase(name, "its3")) {
                    name = "internal transcribed spacer 3";
                }
                break;
            }}
            default:
                break;
        }
    }
}


void CCleanup_imp::x_CleanupExtTRNA(CRNA_ref& rr)
{
    _ASSERT(rr.IsSetExt()  &&  rr.GetExt().IsTRNA());
    // !!!
}


// Imp_feat cleanup
void CCleanup_imp::BasicCleanup(CImp_feat& imf)
{
    CLEAN_STRING_MEMBER(imf, Key);
    CLEAN_STRING_MEMBER(imf, Loc);
    CLEAN_STRING_MEMBER(imf, Descr);
    
    if (imf.IsSetKey()) {
        const CImp_feat::TKey& key = imf.GetKey();
        if (key == "allele"  ||  key == "mutation") {
            imf.SetKey("variation");
        }
    }
}


// Gb_qual cleanup
static bool s_IsJustQuotes(const string& str)
{
    ITERATE (string, it, str) {
        if ((*it > ' ')  &&  (*it != '"')  &&  (*it  != '\'')) {
            return false;
        }
    }
    return true;
}


void CCleanup_imp::x_CleanupConsSplice(CGb_qual& gbq)

{
    string& val = gbq.SetVal();
    
    if (!NStr::StartsWith(val, "(5'site:")) {
        return;
    }
    
    size_t pos = val.find(",3'site:");
    if (pos != NPOS) {
        val.insert(pos + 1, " ");
    }
}


void CCleanup_imp::x_CleanupRptUnit(CGb_qual& gbq)
{
    CGb_qual::TVal& val = gbq.SetVal();
    
    if (NStr::IsBlank(val)) {
        return;
    }
    string s;
    string::const_iterator it = val.begin();
    string::const_iterator end = val.end();
    while (it != end) {
        while (it != end  &&  (*it == '('  ||  *it == ')'  ||  *it == ',')) {
            s += *it++;
        }
        while (it != end  &&  isspace((unsigned char)(*it))) {
            ++it;
        }
        while (it != end  &&  isdigit((unsigned char)(*it))) {
            s += *it++;
        }
        if (it != end  &&  (*it == '.'  ||  *it == '-')) {
            while (it != end  &&  (*it == '.'  ||  *it == '-')) {
                ++it;
            }
            s += "..";
        }
        while (it != end  &&  isspace((unsigned char)(*it))) {
            ++it;
        }
        while (it != end  &&  isdigit((unsigned char)(*it))) {
            s += *it++;
        }
        while (it != end  &&  isspace((unsigned char)(*it))) {
            ++it;
        }
        if (it != end) {
            char c = *it;
            if (c != '('  &&  c != ')'  &&  c != ','  &&  c != '.'  &&
                !isspace((unsigned char) c)  &&  !isdigit((unsigned char) c)) {
                NStr::ToLower(val);
                return;
            }
        }
    }
    val = s;
}


void CCleanup_imp::BasicCleanup(CGb_qual& gbq)
{
    CLEAN_STRING_MEMBER(gbq, Qual);
    if (!gbq.IsSetQual()) {
        gbq.SetQual(kEmptyStr);
    }
    
    CLEAN_STRING_MEMBER(gbq, Val);
    if (gbq.IsSetVal()  &&  s_IsJustQuotes(gbq.GetVal())) {
        gbq.ResetVal();
    }
    if (!gbq.IsSetVal()) {
        gbq.SetVal(kEmptyStr);
    }
    _ASSERT(gbq.IsSetQual()  &&  gbq.IsSetVal());
    
    if (NStr::EqualNocase(gbq.GetQual(), "cons_splice")) {
        x_CleanupConsSplice(gbq);
    } else if (NStr::EqualNocase(gbq.GetQual(), "rpt_unit")) {
        x_CleanupRptUnit(gbq);
    }
}



END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/*
 * ===========================================================================
 *
 * $Log$
 * Revision 1.3  2006/03/23 18:33:32  rsmith
 * Separate straight BasicCleanup of SFData and GBQuals from more complicated
 * changes to various parts of the Seq_feat.
 *
 * Revision 1.2  2006/03/20 14:21:25  rsmith
 * move Biosource related cleanup to its own file.
 *
 * Revision 1.1  2006/03/14 20:21:50  rsmith
 * Move BasicCleanup functionality from objects to objtools/cleanup
 *
 *
 * ===========================================================================
 */
