/*  $Id$
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
 * File Description:
 *   Sample library
 *
 */

#include <ncbi_pch.hpp>
#include <objects/seqloc/Seq_point.hpp>
#include <objects/general/Object_id.hpp>

#include <objects/seqalign/Seq_align.hpp>
#include <objects/seqalign/Spliced_seg.hpp>
#include <objects/seqalign/Spliced_exon.hpp>
#include <objects/seqalign/Product_pos.hpp>
#include <objects/seqalign/Prot_pos.hpp>
#include <objmgr/seq_loc_mapper.hpp>


#include <objects/seq/seqport_util.hpp>

#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/variation/Variation.hpp>
#include <objects/variation/VariantPlacement.hpp>
#include <objects/variation/VariationMethod.hpp>

#include <objects/seqfeat/Variation_inst.hpp>
#include <objects/seqfeat/Delta_item.hpp>
#include <objects/seqfeat/Ext_loc.hpp>


#include <objects/seqfeat/SeqFeatXref.hpp>

#include <objects/seq/Seq_literal.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/Numbering.hpp>
#include <objects/seq/Num_ref.hpp>
#include <objects/seq/Annot_descr.hpp>
#include <objects/seq/Annotdesc.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/general/User_object.hpp>
#include <objects/seqloc/Seq_loc_equiv.hpp>

#include <serial/iterator.hpp>
#include <objmgr/util/sequence.hpp>
#include <objmgr/seq_vector.hpp>
#include <objtools/hgvs/hgvs_parser2.hpp>
#include <objtools/hgvs/variation_util2.hpp>


BEGIN_NCBI_SCOPE

namespace variation {

#define HGVS_THROW(err_code, message) NCBI_THROW(CHgvsParser::CHgvsParserException, err_code, message)

#define HGVS_ASSERT_RULE(i, rule_id) \
    if((i->value.id()) != (SGrammar::rule_id))             \
    {HGVS_THROW(eGrammatic, "Unexpected rule " + CHgvsParser::SGrammar::s_GetRuleName(i->value.id()) ); }


CHgvsParser::SGrammar::TRuleNames CHgvsParser::SGrammar::s_rule_names;
CHgvsParser::SGrammar CHgvsParser::s_grammar;

CVariantPlacement& SetFirstPlacement(CVariation& v)
{
    if(v.SetPlacements().size() == 0) {
        CRef<CVariantPlacement> p(new CVariantPlacement);
        v.SetPlacements().push_back(p);
    }
    return *v.SetPlacements().front();
}

TSeqPos GetLength(const CVariantPlacement& p)
{
    return ncbi::sequence::GetLength(p.GetLoc(), NULL)
        + (p.IsSetStop_offset() ? p.GetStop_offset() : 0)
        - (p.IsSetStart_offset() ? p.GetStart_offset() : 0);
}

void SetComputational(CVariation& variation)
{
    CVariationMethod& m = variation.SetMethod();
    m.SetMethod();

    if(find(m.GetMethod().begin(),
            m.GetMethod().end(),
            CVariationMethod::eMethod_E_computational) == m.GetMethod().end())
    {
        m.SetMethod().push_back(CVariationMethod::eMethod_E_computational);
    }
}


bool SeqsMatch(const string& query, const char* text)
{
    static const string iupac_bases = ".TGKCYSBAWRDMHVN"; //position of the iupac literal = 4-bit mask for A|C|G|T
    for(size_t i = 0; i < query.size(); i++) {
        size_t a = iupac_bases.find(query[i]);
        size_t b = iupac_bases.find(text[i]);
        if(!(a & b)) {
            return false;
        }
    }
    return true;
}


CRef<CSeq_loc> FindSSRLoc(const CSeq_loc& loc, const string& seq, CScope& scope)
{
    //Extend the loc 10kb up and down; Find all occurences of seq in the resulting
    //interval, create locs for individual repeat units; then merge them, and keep the interval that
    //overlaps the original.

    const TSeqPos ext_interval = 10000;

    CRef<CSeq_loc> loc1 = sequence::Seq_loc_Merge(loc, CSeq_loc::fMerge_SingleRange, NULL);
    CBioseq_Handle bsh = scope.GetBioseqHandle(loc);
    TSeqPos seq_len = bsh.GetInst_Length();
    loc1->SetInt().SetFrom() -= min(ext_interval, loc1->GetInt().GetFrom());
    loc1->SetInt().SetTo() += min(ext_interval, seq_len - 1 - loc1->GetInt().GetTo());

    CSeqVector v(*loc1, scope, CBioseq_Handle::eCoding_Iupac);
    string str1;
    v.GetSeqData(v.begin(), v.end(), str1);

    CRef<CSeq_loc> container(new CSeq_loc(CSeq_loc::e_Mix));

    for(size_t i = 0; i < str1.size() - seq.size(); i++) {
        if(SeqsMatch(seq, &str1[i])) {
            CRef<CSeq_loc> repeat_unit_loc(new CSeq_loc);
            repeat_unit_loc->Assign(*loc1);

            if(sequence::GetStrand(loc, NULL) == eNa_strand_minus) {
                repeat_unit_loc->SetInt().SetTo() -= i;
                repeat_unit_loc->SetInt().SetFrom(repeat_unit_loc->GetInt().GetTo() - (seq.size() - 1));
            } else {
                repeat_unit_loc->SetInt().SetFrom() += i;
                repeat_unit_loc->SetInt().SetTo(repeat_unit_loc->GetInt().GetFrom() + (seq.size() - 1));
            }
            container->SetMix().Set().push_back(repeat_unit_loc);
        }
    }

    CRef<CSeq_loc> merged_repeats = sequence::Seq_loc_Merge(*container, CSeq_loc::fSortAndMerge_All, NULL);
    merged_repeats->ChangeToMix();
    CRef<CSeq_loc> result(new CSeq_loc(CSeq_loc::e_Null));
    result->Assign(loc);

    for(CSeq_loc_CI ci(*merged_repeats); ci; ++ci) {
        const CSeq_loc& loc2 = ci.GetEmbeddingSeq_loc();
        if(sequence::Compare(loc, loc2, NULL) != sequence::eNoOverlap) {
            result->Add(loc2);
        }
    }

    return sequence::Seq_loc_Merge(*result, CSeq_loc::fSortAndMerge_All, NULL);
}




void CHgvsParser::s_SetStartOffset(CVariantPlacement& p, const CHgvsParser::SFuzzyInt& fint)
{
    p.ResetStart_offset();
    p.ResetStart_offset_fuzz();
    if(fint.value || fint.fuzz) {
        p.SetStart_offset(fint.value);
    }
    if(fint.fuzz) {
        p.SetStart_offset_fuzz().Assign(*fint.fuzz);
    }
}

void CHgvsParser::s_SetStopOffset(CVariantPlacement& p, const CHgvsParser::SFuzzyInt& fint)
{
    p.ResetStop_offset();
    p.ResetStop_offset_fuzz();
    if(fint.value || fint.fuzz) {
        p.SetStop_offset(fint.value);
    }
    if(fint.fuzz) {
        p.SetStop_offset_fuzz().Assign(*fint.fuzz);
    }
}



//if a variation has an asserted sequence, stored in placement.seq, repackage it as a set having
//the original variation and a synthetic one representing the asserted sequence. The placement.seq
//is cleared, as it is a placeholder for the actual reference sequence.
void RepackageAssertedSequence(CVariation& vr)
{
    if(vr.IsSetPlacements() && SetFirstPlacement(vr).IsSetSeq()) {
        CRef<CVariation> container(new CVariation);
        container->SetPlacements() = vr.SetPlacements();

        CRef<CVariation> orig(new CVariation);
        orig->Assign(vr);
        orig->ResetPlacements(); //location will be set on the package, as it is the same for both members

        container->SetData().SetSet().SetType(CVariation::TData::TSet::eData_set_type_package);
        container->SetData().SetSet().SetVariations().push_back(orig);

        CRef<CVariation> asserted_vr(new CVariation);
        asserted_vr->SetData().SetInstance().SetObservation(CVariation_inst::eObservation_asserted);
        asserted_vr->SetData().SetInstance().SetType(CVariation_inst::eType_identity);

        CRef<CDelta_item> delta(new CDelta_item);
        delta->SetSeq().SetLiteral().Assign(SetFirstPlacement(vr).GetSeq());
        asserted_vr->SetData().SetInstance().SetDelta().push_back(delta);

        SetFirstPlacement(*container).ResetSeq();
        container->SetData().SetSet().SetVariations().push_back(asserted_vr);

        vr.Assign(*container);

    } else if(vr.GetData().IsSet()) {
        NON_CONST_ITERATE(CVariation::TData::TSet::TVariations, it, vr.SetData().SetSet().SetVariations()) {
            RepackageAssertedSequence(**it);
        }
    }
}



const CSeq_feat& CHgvsParser::CContext::GetCDS() const
{
    if(m_cds.IsNull()) {
        HGVS_THROW(eContext, "No CDS feature in context");
    }
    return *m_cds;
}

const CSeq_id& CHgvsParser::CContext::GetId() const
{
    return sequence::GetId(GetPlacement().GetLoc(), NULL);
}


void CHgvsParser::CContext::SetId(const CSeq_id& id, CVariantPlacement::TMol mol)
{
    Clear();

    SetPlacement().SetMol(mol);
    SetPlacement().SetLoc().SetWhole().Assign(id);

    m_bsh = m_scope->GetBioseqHandle(id);

    if(!m_bsh) {
        HGVS_THROW(eContext, "Cannnot get bioseq for seq-id " + id.AsFastaString());
    }

    if(mol == CVariantPlacement::eMol_cdna) {
        for(CFeat_CI ci(m_bsh); ci; ++ci) {
            const CMappedFeat& mf = *ci;
            if(mf.GetData().IsCdregion()) {
                if(m_cds.IsNull()) {
                    m_cds.Reset(new CSeq_feat());
                    m_cds->Assign(mf.GetMappedFeature());
                } else {
                    HGVS_THROW(eContext, "Multiple CDS features on the sequence");
                }
            }
        }
        if(m_cds.IsNull()) {
            HGVS_THROW(eContext, "Could not find CDS feat");
        }
    }
}


const string& CHgvsParser::SGrammar::s_GetRuleName(boost::spirit::classic::parser_id id)
{
    TRuleNames::const_iterator it = s_GetRuleNames().find(id);
    if(it == s_GetRuleNames().end()) {
        HGVS_THROW(eLogic, "Rule name not hardcoded");
    } else {
        return it->second;
    }
}


CHgvsParser::SFuzzyInt CHgvsParser::x_int_fuzz(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_int_fuzz);
    TIterator it = i->children.begin();

    CRef<CInt_fuzz> fuzz(new CInt_fuzz);
    int value = 0;

    if(i->children.size() == 1) { //e.g. '5' or '?'
        string s(it->value.begin(), it->value.end());
        if(s == "?") {
            value = 0;
            fuzz->SetLim(CInt_fuzz::eLim_unk);
        } else {
            value = NStr::StringToInt(s);
            fuzz.Reset();
        }
    } else if(i->children.size() == 3) { //e.g. '(5)' or '(?)'
        ++it;
        string s(it->value.begin(), it->value.end());
        if(s == "?") {
            value = 0;
            fuzz->SetLim(CInt_fuzz::eLim_unk);
        } else {
            value = NStr::StringToInt(s);
            fuzz->SetLim(CInt_fuzz::eLim_unk);
        }
    } else if(i->children.size() == 5) { //e.g. '(5_7)' or '(?_10)'
        ++it;
        string s1(it->value.begin(), it->value.end());
        ++it;
        ++it;
        string s2(it->value.begin(), it->value.end());

        if(s1 == "?" && s2 == "?") {
            value = 0;
            fuzz->SetLim(CInt_fuzz::eLim_unk);
        } else if(s1 != "?" && s2 != "?") {
            value = NStr::StringToInt(s1);
            fuzz->SetRange().SetMin(NStr::StringToInt(s1));
            fuzz->SetRange().SetMax(NStr::StringToInt(s2));
        } else if(s2 == "?") {
            value = NStr::StringToInt(s1);
            fuzz->SetLim(CInt_fuzz::eLim_gt);
        } else if(s1 == "?") {
            value = NStr::StringToInt(s2);
            fuzz->SetLim(CInt_fuzz::eLim_lt);
        } else {
            HGVS_THROW(eLogic, "Unreachable code");
        }
    }

    CHgvsParser::SFuzzyInt fuzzy_int;
    fuzzy_int.value = value;
    fuzzy_int.fuzz = fuzz;

#if 0
    NcbiCerr << "Fuzzy int: " << value << " ";
    if(fuzz) {
        NcbiCerr << MSerial_AsnText << *fuzz;
    } else {
        NcbiCerr << "\n";
    }
#endif

    return fuzzy_int;
}

CRef<CSeq_point> CHgvsParser::x_abs_pos(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_abs_pos);
    TIterator it = i->children.begin();

    CRef<CSeq_point> pnt(new CSeq_point);
    pnt->SetId().Assign(context.GetId());
    pnt->SetStrand(eNa_strand_plus);
    TSignedSeqPos offset(0);

    bool is_relative_to_stop_codon = false;
    if(i->children.size() == 2) {
        is_relative_to_stop_codon = true;
        string s(it->value.begin(), it->value.end());
        if(s != "*") {
            HGVS_THROW(eGrammatic, "Expected literal '*'");
        }
        if(context.GetPlacement().GetMol() != CVariantPlacement::eMol_cdna) {
            HGVS_THROW(eContext, "Expected 'c.' context for stop-codon-relative coordinate");
        }

        offset = context.GetCDS().GetLocation().GetStop(eExtreme_Biological);
        ++it;
    } else {
        if (context.GetPlacement().GetMol() == CVariantPlacement::eMol_cdna) {
            //Note: in RNA coordinates (r.) the coordinates are absolute, like in genomic sequences,
            //  "The RNA sequence type uses only GenBank mRNA records. The value 1 is assigned to the first
            //  base in the record and from there all bases are counted normally."
            //so the cds-start offset applies only to "c." coordinates
            offset = context.GetCDS().GetLocation().GetStart(eExtreme_Biological);
        }
    }

    SFuzzyInt int_fuzz = x_int_fuzz(it, context);
    if(int_fuzz.value > 0 && !is_relative_to_stop_codon) {
        /* In HGVS:
         * the nucleotide 3' of the translation stop codon is *1, the next *2, etc.
         * # there is no nucleotide 0
         * # nucleotide 1 is the A of the ATG-translation initiation codon
         * # the nucleotide 5' of the ATG-translation initiation codon is -1, the previous -2, etc.
         * I.e. need to adjust if dealing with positive coordinates, except for *-relative ones.
         */
        offset--;
    }

    if(int_fuzz.fuzz.IsNull()) {
        pnt->SetPoint(offset + int_fuzz.value);
    } else {
        pnt->SetPoint(offset + int_fuzz.value);
        pnt->SetFuzz(*int_fuzz.fuzz);
        if(pnt->GetFuzz().IsRange()) {
            pnt->SetFuzz().SetRange().SetMin() += offset;
            pnt->SetFuzz().SetRange().SetMax() += offset;
        }
    }

    return pnt;
}


/*
 * general_pos is either simple abs-pos that is passed down to x_abs_pos,
 * or an intronic location that is specified by a mapping point in the
 * local coordinates and the -upstream / +downstream offset after remapping.
 *
 * The mapping point can either be an abs-pos in local coordinates, or
 * specified as offset in intron-specific coordinate system where IVS# specifies
 * the intron number
 */
CHgvsParser::SOffsetPoint CHgvsParser::x_general_pos(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_general_pos);

    SOffsetPoint ofpnt;

    if(i->children.size() == 1) {
        //local coordinates
        ofpnt.pnt = x_abs_pos(i->children.begin(), context);
    } else {
        //(str_p("IVS") >> int_p | abs_pos) >> sign_p >> int_fuzz

        TIterator it = i->children.end() - 1;
        ofpnt.offset = x_int_fuzz(it, context);
        --it;
        string s_sign(it->value.begin(), it->value.end());
        int sign1 = s_sign == "-" ? -1 : 1;
        ofpnt.offset.value *= sign1;
        if(ofpnt.offset.fuzz &&
           ofpnt.offset.fuzz->IsLim() &&
           ofpnt.offset.fuzz->GetLim() == CInt_fuzz::eLim_unk)
        {
            ofpnt.offset.fuzz->SetLim(sign1 < 0 ? CInt_fuzz::eLim_lt : CInt_fuzz::eLim_gt);
        }

        --it;
        if(it->value.id() == SGrammar::eID_abs_pos) {
            //base-loc is an abs-pos
            ofpnt.pnt = x_abs_pos(i->children.begin(), context);
        } else {
            //base-loc is IVS-relative.
            ofpnt.pnt.Reset(new CSeq_point);
            ofpnt.pnt->SetId().Assign(context.GetId());
            ofpnt.pnt->SetStrand(eNa_strand_plus);

            TIterator it = i->children.begin();
            string s_ivs(it->value.begin(), it->value.end());
            ++it;
            string s_ivs_num(it->value.begin(), it->value.end());
            int ivs_num = NStr::StringToInt(s_ivs_num);

            //If IVS3+50, the mapping point is the last base of third exon
            //if IVS3-50, the mapping point is the first base of the fourth exon
            size_t target_exon_num = sign1 < 0 ? ivs_num + 1 : ivs_num;

            SAnnotSelector sel;
            sel.IncludeFeatSubtype(CSeqFeatData::eSubtype_exon);
            CBioseq_Handle bsh = context.GetScope().GetBioseqHandle(context.GetId());
            size_t exon_num = 1;
            //Note: IVS is cDNA-centric, so we'll have to use ordinals of the exons instead of /number qual
            for(CFeat_CI ci(bsh, sel); ci; ++ci) {
                const CMappedFeat& mf = *ci;
                if(exon_num == target_exon_num) {
                    ofpnt.pnt->SetPoint(sign1 > 0 ? mf.GetLocation().GetStop(eExtreme_Biological)
                                                  : mf.GetLocation().GetStart(eExtreme_Biological));
                    break;
                }
                exon_num++;
            }
        }
    }

    return ofpnt;
}


CHgvsParser::SOffsetPoint CHgvsParser::x_fuzzy_pos(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_fuzzy_pos);

    SOffsetPoint pnt;
    SOffsetPoint pnt1 = x_general_pos(i->children.begin(), context);
    SOffsetPoint pnt2 = x_general_pos(i->children.begin() + 1, context);

    //Verify that on the same seq-id.
    if(!pnt1.pnt->GetId().Equals(pnt2.pnt->GetId())) {
        HGVS_THROW(eSemantic, "Points in a fuzzy pos are on different sequences");
    }
    if(pnt1.pnt->GetStrand() != pnt2.pnt->GetStrand()) {
        HGVS_THROW(eSemantic, "Range-loc start/stop are on different strands.");
    }

    //If One is empty, copy from the other and set TL for loc1 and TR for loc2
    if(pnt1.pnt->GetPoint() == kInvalidSeqPos && pnt2.pnt->GetPoint() != kInvalidSeqPos) {
        pnt1.pnt->Assign(*pnt2.pnt);
        pnt1.pnt->SetFuzz().SetLim(CInt_fuzz::eLim_tl);
    } else if(pnt1.pnt->GetPoint() != kInvalidSeqPos && pnt2.pnt->GetPoint() == kInvalidSeqPos) {
        pnt2.pnt->Assign(*pnt1.pnt);
        pnt2.pnt->SetFuzz().SetLim(CInt_fuzz::eLim_tr);
    }

    if((pnt1.offset.value != 0 || pnt2.offset.value != 0) && !pnt1.pnt->Equals(*pnt2.pnt)) {
        HGVS_THROW(eSemantic, "Base-points in an intronic fuzzy position must be equal");
    }

    pnt.pnt = pnt1.pnt;
    pnt.offset = pnt1.offset;

    if(pnt1.offset.value != pnt2.offset.value) {
        pnt.offset.fuzz.Reset(new CInt_fuzz);
        pnt.offset.fuzz->SetRange().SetMin(pnt1.offset.value);
        pnt.offset.fuzz->SetRange().SetMax(pnt2.offset.value);
    }

    return pnt;

#if 0
    todo: reconcile
    //If Both are Empty - the result is empty, otherwise reconciliate
    if(pnt1.pnt->GetPoint() == kInvalidSeqPos && pnt2.pnt->GetPoint() == kInvalidSeqPos) {
        pnt.pnt = pnt1.pnt;
        pnt.offset = pnt1.offset;
    } else {
        pnt.pnt.Reset(new CSeq_point);
        pnt.pnt.Assign(*pnt1.pnt);

        TSeqPos min_pos = min(pnt1.pnt->GetPoint(), pnt2.pnt->GetPoint());
        TSeqPos max_pos = max(pnt1.pnt->GetPoint(), pnt2.pnt->GetPoint());

        if(!pnt1->IsSetFuzz() && !pnt2->IsSetFuzz()) {
            //Both are non-fuzzy - create the min-max fuzz.
            //(10+50_10+60)
            pnt->SetFuzz().SetRange().SetMin(min_pos);
            pnt->SetFuzz().SetRange().SetMax(max_pos);

        } else if(pnt1->IsSetFuzz() && pnt2->IsSetFuzz()) {
            //Both are fuzzy - reconcile the fuzz.

            if(pnt1->GetFuzz().GetLim() == CInt_fuzz::eLim_tr
            && pnt2->GetFuzz().GetLim() == CInt_fuzz::eLim_tl)
            {
                //fuzz points inwards - create min-max fuzz
                //(10+?_11-?)
                pnt->SetFuzz().SetRange().SetMin(min_pos);
                pnt->SetFuzz().SetRange().SetMax(max_pos);

            } else if (pnt1->GetFuzz().GetLim() == CInt_fuzz::eLim_tl
                    && pnt2->GetFuzz().GetLim() == CInt_fuzz::eLim_tr)
            {
                //fuzz points outwards - set fuzz to unk
                //(10-?_10+?)
                //(?_10+?)
                //(10-?_?)
                pnt->SetFuzz().SetLim(CInt_fuzz::eLim_unk);

            }  else if (pnt1->GetFuzz().GetLim() == CInt_fuzz::eLim_tl
                     && pnt2->GetFuzz().GetLim() == CInt_fuzz::eLim_tl)
            {
                //fuzz is to the left - use 5'-most
                //(?_10-?)
                //(10-?_11-?)
                pnt->SetPoint(pnt->GetStrand() == eNa_strand_minus ? max_pos : min_pos);

            }  else if (pnt1->GetFuzz().GetLim() == CInt_fuzz::eLim_tr
                     && pnt2->GetFuzz().GetLim() == CInt_fuzz::eLim_tr)
            {
                //fuzz is to the right - use 3'-most
                //(10+?_?)
                //(10+?_11+?)
                pnt->SetPoint(pnt->GetStrand() == eNa_strand_minus ? min_pos : max_pos);

            } else {
                pnt->SetFuzz().SetLim(CInt_fuzz::eLim_unk);
            }
        } else {
            // One of the two is non-fuzzy:
            // use it to specify position, and the fuzz of the other to specify the fuzz
            // e.g.  (10+5_10+?)  -> loc1=100005; loc2=100000tr  -> 100005tr

            pnt->Assign(pnt1->IsSetFuzz() ? *pnt2 : *pnt1);
            pnt->SetFuzz().Assign(pnt1->IsSetFuzz() ? pnt1->GetFuzz()
                                                    : pnt2->GetFuzz());

        }
    }
#endif



}


CHgvsParser::CContext CHgvsParser::x_header(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_header);

    CContext ctx(context);

    TIterator it = i->children.rbegin()->children.begin();
    string mol(it->value.begin(), it->value.end());
    CVariantPlacement::TMol mol_type =
                       mol == "c" ? CVariantPlacement::eMol_cdna
                     : mol == "g" ? CVariantPlacement::eMol_genomic
                     : mol == "r" ? CVariantPlacement::eMol_rna
                     : mol == "p" ? CVariantPlacement::eMol_protein
                     : mol == "m" ? CVariantPlacement::eMol_mitochondrion
                     : mol == "mt" ? CVariantPlacement::eMol_mitochondrion
                     : CVariantPlacement::eMol_unknown;

    it  = (i->children.rbegin() + 1)->children.begin();
    string id_str(it->value.begin(), it->value.end());

    CRef<CSeq_id> id(new CSeq_id(id_str));
    ctx.SetId(*id, mol_type);

    if(i->children.size() == 3) {
        it  = (i->children.rbegin() + 2)->children.begin();
        string tag_str(it->value.begin(), it->value.end());
        //record tag in context, if it is necessary in the future
    }

    return ctx;
}


CHgvsParser::SOffsetPoint CHgvsParser::x_pos_spec(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_pos_spec);

    SOffsetPoint pnt;
    TIterator it = i->children.begin();
    if(it->value.id() == SGrammar::eID_general_pos) {
        pnt = x_general_pos(it, context);
    } else if(it->value.id() == SGrammar::eID_fuzzy_pos) {
        pnt = x_fuzzy_pos(it, context);
    } else {
        bool flip_strand = false;
        if(i->children.size() == 3) {
            //first child is 'o' - opposite
            flip_strand = true;
            ++it;
        }

        CContext local_ctx = x_header(it, context);
        ++it;
        pnt = x_pos_spec(it, local_ctx);

        if(flip_strand) {
            pnt.pnt->FlipStrand();
        }
    }

    return pnt;
}


CHgvsParser::SOffsetPoint CHgvsParser::x_prot_pos(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_prot_pos);
    TIterator it = i->children.begin();

    CRef<CSeq_literal> prot_literal = x_raw_seq(it, context);

    if(context.GetPlacement().GetMol() != CVariantPlacement::eMol_protein) {
        HGVS_THROW(eSemantic, "Expected protein context");
    }

    if(prot_literal->GetLength() != 1) {
        HGVS_THROW(eSemantic, "Expected single aa literal in prot-pos");
    }

    ++it;
    SOffsetPoint pnt = x_pos_spec(it, context);

    pnt.asserted_sequence = prot_literal->GetSeq_data().GetNcbieaa();

    return pnt;
}


CRef<CVariantPlacement> CHgvsParser::x_range(TIterator const& i, const CContext& context)
{
    SOffsetPoint pnt1, pnt2;

    CRef<CVariantPlacement> p(new CVariantPlacement);
    p->Assign(context.GetPlacement());

    if(i->value.id() == SGrammar::eID_prot_range) {
        pnt1 = x_prot_pos(i->children.begin(), context);
        pnt2 = x_prot_pos(i->children.begin() + 1, context);
    } else if(i->value.id() == SGrammar::eID_nuc_range) {
        pnt1 = x_pos_spec(i->children.begin(), context);
        pnt2 = x_pos_spec(i->children.begin() + 1, context);
    } else {
        HGVS_ASSERT_RULE(i, eID_NONE);
    }

    if(!pnt1.pnt->GetId().Equals(pnt2.pnt->GetId())) {
        HGVS_THROW(eSemantic, "Range-loc start/stop are on different seq-ids.");
    }
    if(pnt1.pnt->GetStrand() != pnt2.pnt->GetStrand()) {
        HGVS_THROW(eSemantic, "Range-loc start/stop are on different strands.");
    }

    p->SetLoc().SetInt().SetId(pnt1.pnt->SetId());
    p->SetLoc().SetInt().SetFrom(pnt1.pnt->GetPoint());
    p->SetLoc().SetInt().SetTo(pnt2.pnt->GetPoint());
    p->SetLoc().SetInt().SetStrand(pnt1.pnt->GetStrand());
    if(pnt1.pnt->IsSetFuzz()) {
        p->SetLoc().SetInt().SetFuzz_from(pnt1.pnt->SetFuzz());
    }

    if(pnt2.pnt->IsSetFuzz()) {
        p->SetLoc().SetInt().SetFuzz_to(pnt2.pnt->SetFuzz());
    }

    s_SetStartOffset(*p, pnt1.offset);
    s_SetStopOffset(*p, pnt2.offset);

    if(pnt1.asserted_sequence != "" || pnt2.asserted_sequence != "") {
        //for proteins, the asserted sequence is specified as part of location, rather than variation
        p->SetSeq().SetLength(GetLength(*p));
        string& seq_str = (context.GetPlacement().GetMol() == CVariantPlacement::eMol_protein)
                ? p->SetSeq().SetSeq_data().SetNcbieaa().Set()
                : p->SetSeq().SetSeq_data().SetIupacna().Set();
        seq_str = pnt1.asserted_sequence + ".." + pnt2.asserted_sequence;
    }

    return p;
}

CRef<CVariantPlacement> CHgvsParser::x_location(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_location);

    CRef<CVariantPlacement> placement(new CVariantPlacement);
    placement->Assign(context.GetPlacement());

    TIterator it = i->children.begin();
    CRef<CSeq_loc> loc(new CSeq_loc);
    if(it->value.id() == SGrammar::eID_prot_pos || it->value.id() == SGrammar::eID_pos_spec) {
        SOffsetPoint pnt = it->value.id() == SGrammar::eID_prot_pos
                ? x_prot_pos(it, context)
                : x_pos_spec(it, context);
        placement->SetLoc().SetPnt(*pnt.pnt);
        s_SetStartOffset(*placement, pnt.offset);
        if(pnt.asserted_sequence != "") {
            placement->SetSeq().SetLength(GetLength(*placement));
            string& seq_str = (context.GetPlacement().GetMol() == CVariantPlacement::eMol_protein)
                    ? placement->SetSeq().SetSeq_data().SetNcbieaa().Set()
                    : placement->SetSeq().SetSeq_data().SetIupacna().Set();
            seq_str = pnt.asserted_sequence;
        }
    } else if(it->value.id() == SGrammar::eID_nuc_range || it->value.id() == SGrammar::eID_prot_range) {
        placement = x_range(it, context);
    } else {
        HGVS_ASSERT_RULE(it, SGrammar::eID_NONE);
    }

    if(placement->GetLoc().IsPnt() && placement->GetLoc().GetPnt().GetPoint() == kInvalidSeqPos) {
        placement->SetLoc().SetEmpty().Assign(context.GetId());
    }

    return placement;
}


CRef<CSeq_loc> CHgvsParser::x_seq_loc(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_seq_loc);
    TIterator it = i->children.begin();

    bool flip_strand = false;
    if(i->children.size() == 3) {
        //first child is 'o' - opposite
        flip_strand = true;
        ++it;
    }

    CContext local_context = x_header(it, context);
    ++it;
    CRef<CVariantPlacement> p = x_location(it, local_context);

    if(flip_strand) {
        p->SetLoc().FlipStrand();
    }

    if(p->IsSetStop_offset() || p->IsSetStart_offset()) {
        HGVS_THROW(eSemantic, "Intronic seq-locs are not supported in this context");
    }

    CRef<CSeq_loc> loc(new CSeq_loc);
    loc->Assign(p->GetLoc());
    return loc;
}

CRef<CSeq_literal> CHgvsParser::x_raw_seq_or_len(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_raw_seq_or_len);

    CRef<CSeq_literal> literal;
    TIterator it = i->children.begin();

    if(it->value.id() == SGrammar::eID_raw_seq) {
        literal = x_raw_seq(it, context);
    } else if(it->value.id() == SGrammar::eID_int_fuzz) {
        SFuzzyInt int_fuzz = x_int_fuzz(it, context);
        literal.Reset(new CSeq_literal);
        literal->SetLength(int_fuzz.value);
        if(int_fuzz.fuzz.IsNull()) {
            ;//no-fuzz;
        } else if(int_fuzz.fuzz->IsLim() && int_fuzz.fuzz->GetLim() == CInt_fuzz::eLim_unk) {
            //unknown length (no value) - will represent as length=0 with gt fuzz
            literal->SetFuzz().SetLim(CInt_fuzz::eLim_gt);
        } else {
            literal->SetFuzz(*int_fuzz.fuzz);
        }
    } else {
        HGVS_ASSERT_RULE(it, eID_NONE);
    }
    return literal;
}

CHgvsParser::TDelta CHgvsParser::x_seq_ref(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_seq_ref);
    CHgvsParser::TDelta delta(new TDelta::TObjectType);
    TIterator it = i->children.begin();

    if(it->value.id() == SGrammar::eID_seq_loc) {
        CRef<CSeq_loc> loc = x_seq_loc(it, context);
        delta->SetSeq().SetLoc(*loc);
    } else if(it->value.id() == SGrammar::eID_nuc_range || it->value.id() == SGrammar::eID_prot_range) {
        CRef<CVariantPlacement> p = x_range(it, context);
        if(p->IsSetStart_offset() || p->IsSetStop_offset()) {
            HGVS_THROW(eSemantic, "Intronic loc is not supported in this context");
        }
        delta->SetSeq().SetLoc().Assign(p->GetLoc());
    } else if(it->value.id() == SGrammar::eID_raw_seq_or_len) {
        CRef<CSeq_literal> literal = x_raw_seq_or_len(it, context);
        delta->SetSeq().SetLiteral(*literal);
    } else {
        HGVS_ASSERT_RULE(it, eID_NONE);
    }

    return delta;
}

string CHgvsParser::s_hgvsaa2ncbieaa(const string& hgvsaa)
{
    string ncbieaa = hgvsaa;
    NStr::ReplaceInPlace(ncbieaa, "Gly", "G");
    NStr::ReplaceInPlace(ncbieaa, "Pro", "P");
    NStr::ReplaceInPlace(ncbieaa, "Ala", "A");
    NStr::ReplaceInPlace(ncbieaa, "Val", "V");
    NStr::ReplaceInPlace(ncbieaa, "Leu", "L");
    NStr::ReplaceInPlace(ncbieaa, "Ile", "I");
    NStr::ReplaceInPlace(ncbieaa, "Met", "M");
    NStr::ReplaceInPlace(ncbieaa, "Cys", "C");
    NStr::ReplaceInPlace(ncbieaa, "Phe", "F");
    NStr::ReplaceInPlace(ncbieaa, "Tyr", "Y");
    NStr::ReplaceInPlace(ncbieaa, "Trp", "W");
    NStr::ReplaceInPlace(ncbieaa, "His", "H");
    NStr::ReplaceInPlace(ncbieaa, "Lys", "K");
    NStr::ReplaceInPlace(ncbieaa, "Arg", "R");
    NStr::ReplaceInPlace(ncbieaa, "Gln", "Q");
    NStr::ReplaceInPlace(ncbieaa, "Asn", "N");
    NStr::ReplaceInPlace(ncbieaa, "Glu", "E");
    NStr::ReplaceInPlace(ncbieaa, "Asp", "D");
    NStr::ReplaceInPlace(ncbieaa, "Ser", "S");
    NStr::ReplaceInPlace(ncbieaa, "Thr", "T");
    NStr::ReplaceInPlace(ncbieaa, "X", "*");
    NStr::ReplaceInPlace(ncbieaa, "?", "-");
    return ncbieaa;
}


string CHgvsParser::s_hgvsUCaa2hgvsUL(const string& hgvsaa)
{
    string s = hgvsaa;
    NStr::ReplaceInPlace(s, "GLY", "Gly");
    NStr::ReplaceInPlace(s, "PRO", "Pro");
    NStr::ReplaceInPlace(s, "ALA", "Ala");
    NStr::ReplaceInPlace(s, "VAL", "Val");
    NStr::ReplaceInPlace(s, "LEU", "Leu");
    NStr::ReplaceInPlace(s, "ILE", "Ile");
    NStr::ReplaceInPlace(s, "MET", "Met");
    NStr::ReplaceInPlace(s, "CYS", "Cys");
    NStr::ReplaceInPlace(s, "PHE", "Phe");
    NStr::ReplaceInPlace(s, "TYR", "Tyr");
    NStr::ReplaceInPlace(s, "TRP", "Trp");
    NStr::ReplaceInPlace(s, "HIS", "His");
    NStr::ReplaceInPlace(s, "LYS", "Lys");
    NStr::ReplaceInPlace(s, "ARG", "Arg");
    NStr::ReplaceInPlace(s, "GLN", "Gln");
    NStr::ReplaceInPlace(s, "ASN", "Asn");
    NStr::ReplaceInPlace(s, "GLU", "Glu");
    NStr::ReplaceInPlace(s, "ASP", "Asp");
    NStr::ReplaceInPlace(s, "SER", "Ser");
    NStr::ReplaceInPlace(s, "THR", "Thr");
    return s;
}


CRef<CSeq_literal> CHgvsParser::x_raw_seq(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_raw_seq);
    TIterator it = i->children.begin();
    string seq_str(it->value.begin(), it->value.end());

    CRef<CSeq_literal>literal(new CSeq_literal);
    if(context.GetPlacement().GetMol() == CVariantPlacement::eMol_protein) {
        seq_str = s_hgvsaa2ncbieaa(seq_str);
        literal->SetSeq_data().SetNcbieaa().Set(seq_str);
    } else {
        if(context.GetPlacement().GetMol() == CVariantPlacement::eMol_rna) {
            seq_str = NStr::ToUpper(seq_str);
            NStr::ReplaceInPlace(seq_str, "U", "T");
        }
        literal->SetSeq_data().SetIupacna().Set(seq_str);
    }

    literal->SetLength(seq_str.size());

    vector<TSeqPos> bad;
    CSeqportUtil::Validate(literal->GetSeq_data(), &bad);

    if(bad.size() > 0) {
        HGVS_THROW(eSemantic, "Invalid sequence at pos " +  NStr::IntToString(bad[0]) + " in " + seq_str);
    }

    return literal;
}


CRef<CVariation> CHgvsParser::x_delins(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_delins);
    TIterator it = i->children.begin();
    CRef<CVariation> del_vr = x_deletion(it, context);
    ++it;
    CRef<CVariation> ins_vr = x_insertion(it, context);

    //The resulting delins variation has deletion's placement (with asserted seq, if any),
    //and insertion's inst, except action type is "replace" (default) rather than "ins-before",
    //so we reset action

    del_vr->SetData().SetInstance().SetType(CVariation_inst::eType_delins);
    del_vr->SetData().SetInstance().SetDelta() = ins_vr->SetData().SetInstance().SetDelta();
    del_vr->SetData().SetInstance().SetDelta().front()->ResetAction();

    return del_vr;
}

CRef<CVariation> CHgvsParser::x_deletion(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i,  SGrammar::eID_deletion);
    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();

    var_inst.SetType(CVariation_inst::eType_del);
    SetFirstPlacement(*vr).Assign(context.GetPlacement());

    CRef<CDelta_item> di(new CDelta_item);
    di->SetAction(CDelta_item::eAction_del_at);
    di->SetSeq().SetThis();
    var_inst.SetDelta().push_back(di);

    ++it;

    if(it->value.id() == SGrammar::eID_raw_seq_or_len) {
        CRef<CSeq_literal> literal = x_raw_seq_or_len(it, context);
        ++it;
        SetFirstPlacement(*vr).SetSeq(*literal);
    }

    var_inst.SetDelta();
    return vr;
}


CRef<CVariation> CHgvsParser::x_insertion(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_insertion);
    TIterator it = i->children.begin();
    ++it; //skip ins
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();

    var_inst.SetType(CVariation_inst::eType_ins);

    SetFirstPlacement(*vr).Assign(context.GetPlacement());

    //The delta consists of the self-location followed by the insertion sequence
    TDelta delta_ins = x_seq_ref(it, context);

    //todo:
    //alternative representation: if delta is literal, might use action=morph and prefix/suffix the insertion with the flanking nucleotides.
    delta_ins->SetAction(CDelta_item::eAction_ins_before);

    var_inst.SetDelta().push_back(delta_ins);

    return vr;
}


CRef<CVariation> CHgvsParser::x_duplication(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_duplication);
    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_ins); //replace seq @ location with this*2

    SetFirstPlacement(*vr).Assign(context.GetPlacement());

    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetThis(); //delta->SetSeq().SetLoc(vr->SetLocation());
    delta->SetMultiplier(2);
    var_inst.SetDelta().push_back(delta);

    ++it; //skip dup

    //the next node is either expected length or expected sequence
    if(it != i->children.end() && it->value.id() == SGrammar::eID_seq_ref) {
        TDelta dup_seq = x_seq_ref(it, context);
        if(dup_seq->GetSeq().IsLiteral()) {
            SetFirstPlacement(*vr).SetSeq(dup_seq->SetSeq().SetLiteral());
        }
    }

    return vr;
}


CRef<CVariation> CHgvsParser::x_nuc_subst(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_nuc_subst);
    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();

    SetFirstPlacement(*vr).Assign(context.GetPlacement());
    var_inst.SetType(CVariation_inst::eType_snv);

    CRef<CSeq_literal> seq_from = x_raw_seq(it, context);
    if(seq_from->GetLength() != 1) {
        HGVS_THROW(eSemantic, "Expected literal of length 1 left of '>'");
    }

    //context.Validate(*seq_from);
    SetFirstPlacement(*vr).SetSeq(*seq_from);

    ++it;//skip to ">"
    ++it;//skip to next
    CRef<CSeq_literal> seq_to = x_raw_seq(it, context);
    if(seq_to->GetLength() != 1) {
        HGVS_THROW(eSemantic, "Expected literal of length 1 right of '>'");
    }

    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetLiteral(*seq_to);
    var_inst.SetDelta().push_back(delta);

    return vr;
}


CRef<CVariation> CHgvsParser::x_nuc_inv(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_nuc_inv);

    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_inv);

    SetFirstPlacement(*vr).Assign(context.GetPlacement());

#if 0
    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetLoc().Assign(*loc);
    delta->SetSeq().SetLoc().FlipStrand();
    var_inst.SetDelta().push_back(delta);
#endif

    ++it;
    if(it != i->children.end()) {
        string len_str(it->value.begin(), it->value.end());
        TSeqPos len = NStr::StringToUInt(len_str);
        if(len != GetLength(SetFirstPlacement(*vr))) {
            HGVS_THROW(eSemantic, "Inversion length not equal to location length");
        }
    }

    return vr;
}


CRef<CVariation> CHgvsParser::x_ssr(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_ssr);
    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);
    vr->SetData().SetInstance().SetType(CVariation_inst::eType_microsatellite);


    CRef<CSeq_literal> literal;
    if(it->value.id() == SGrammar::eID_raw_seq) {
        literal = x_raw_seq(it, context);
        ++it;
    }



    SetFirstPlacement(*vr).Assign(context.GetPlacement());

#if 1
    if(!literal.IsNull() && literal->IsSetSeq_data() && literal->GetSeq_data().IsIupacna()) {
        CRef<CSeq_loc> ssr_loc = FindSSRLoc(SetFirstPlacement(*vr).GetLoc(), literal->GetSeq_data().GetIupacna(), context.GetScope());
        SetFirstPlacement(*vr).SetLoc().Assign(*ssr_loc);
    }
#else
    if(SetFirstPlacement(*vr).GetLoc().IsPnt() && !literal.IsNull()) {
        //The location may either specify a repeat unit, or point to the first base of a repeat unit.
        //We normalize it so it is alwas the repeat unit.
        ExtendDownstream(SetFirstPlacement(*vr), literal->GetLength() - 1);
    }
#endif


    if(it->value.id() == SGrammar::eID_ssr) { // list('['>>int_p>>']', '+') with '[',']','+' nodes discarded;
        //Note: see ssr grammar in the header for reasons why we have to match all alleles here
        //rather than match them separately as mut_insts

        vr->SetData().SetSet().SetType(CVariation::TData::TSet::eData_set_type_genotype);
        for(; it != i->children.end(); ++it) {
            string s1(it->value.begin(), it->value.end());
            CRef<CVariation> vr2(new CVariation);
            vr2->SetData().SetInstance().SetType(CVariation_inst::eType_microsatellite);

            TDelta delta(new TDelta::TObjectType);
            if(!literal.IsNull()) {
                delta->SetSeq().SetLiteral().Assign(*literal);
            } else {
                delta->SetSeq().SetThis();
            }
            delta->SetMultiplier(NStr::StringToInt(s1));

            vr2->SetData().SetInstance().SetDelta().push_back(delta);
            vr->SetData().SetSet().SetVariations().push_back(vr2);
        }
        vr = x_unwrap_iff_singleton(*vr);
    } else {
        TDelta delta(new TDelta::TObjectType);
        if(!literal.IsNull()) {
            delta->SetSeq().SetLiteral().Assign(*literal);
        } else {
            delta->SetSeq().SetThis();
        }

        SFuzzyInt int_fuzz = x_int_fuzz(it, context);
        delta->SetMultiplier(int_fuzz.value);
        if(int_fuzz.fuzz.IsNull()) {
            ;
        } else {
            delta->SetMultiplier_fuzz(*int_fuzz.fuzz);
        }
        vr->SetData().SetInstance().SetDelta().push_back(delta);
    }

    return vr;
}


CRef<CVariation> CHgvsParser::x_translocation(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_translocation);
    TIterator it = i->children.end() - 1; //note: seq-loc follows iscn expression, i.e. last child
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_translocation);

    CRef<CSeq_loc> loc = x_seq_loc(it, context);
    SetFirstPlacement(*vr).SetLoc().Assign(*loc);

    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetLoc().SetNull();
    var_inst.SetDelta().push_back(delta);

    return vr;
}


CRef<CVariation> CHgvsParser::x_conversion(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_conversion);
    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_transposon);

    SetFirstPlacement(*vr).Assign(context.GetPlacement());

    ++it;
    CRef<CSeq_loc> loc_other = x_seq_loc(it, context);

    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetLoc().Assign(*loc_other);
    var_inst.SetDelta().push_back(delta);

    return vr;
}


CRef<CVariation> CHgvsParser::x_prot_fs(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_prot_fs);
    TIterator it = i->children.begin();
    CRef<CVariation> vr(new CVariation);

    if(context.GetPlacement().GetMol() != CVariantPlacement::eMol_protein) {
        HGVS_THROW(eContext, "Frameshift can only be specified in protein context");
    }

    vr->SetData().SetNote("Frameshift");
    vr->SetFrameshift();

    SetFirstPlacement(*vr).Assign(context.GetPlacement());

    ++it; //skip 'fs'
    if(it != i->children.end()) {
        //fsX# description: the remaining tokens are 'X' and integer
        ++it; //skip 'X'
        string s(it->value.begin(), it->value.end());
        int x_length = NStr::StringToInt(s);
        vr->SetFrameshift().SetX_length(x_length);
    }

    return vr;
}


CRef<CVariation> CHgvsParser::x_prot_ext(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_prot_ext);
    TIterator it = i->children.begin();

    if(context.GetPlacement().GetMol() != CVariantPlacement::eMol_protein) {
        HGVS_THROW(eContext, "Expected protein context");
    }

    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_prot_other);
    string ext_type_str(it->value.begin(), it->value.end());
    ++it;
    string ext_len_str(it->value.begin(), it->value.end());
    int ext_len = NStr::StringToInt(ext_len_str);

    SetFirstPlacement(*vr).Assign(context.GetPlacement());
    SetFirstPlacement(*vr).SetLoc().SetPnt().SetId().Assign(context.GetId());
    SetFirstPlacement(*vr).SetLoc().SetPnt().SetStrand(eNa_strand_plus);

    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetLiteral().SetLength(abs(ext_len) + 1);
        //extension of Met or X by N bases = replacing first or last AA with (N+1) AAs

    if(ext_type_str == "extMet") {
        if(ext_len > 0) {
            HGVS_THROW(eSemantic, "extMet must be followed by a negative integer");
        }
        SetFirstPlacement(*vr).SetLoc().SetPnt().SetPoint(0);
        //extension precedes first AA
        var_inst.SetDelta().push_back(delta);
    } else if(ext_type_str == "extX") {
        if(ext_len < 0) {
            HGVS_THROW(eSemantic, "exX must be followed by a non-negative integer");
        }

        SetFirstPlacement(*vr).SetLoc().SetPnt().SetPoint(context.GetBioseqHandle().GetInst_Length() - 1);
        //extension follows last AA
        var_inst.SetDelta().push_back(delta);
    } else {
        HGVS_THROW(eGrammatic, "Unexpected ext_type: " + ext_type_str);
    }

    return vr;
}


CRef<CVariation> CHgvsParser::x_prot_missense(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_prot_missense);
    TIterator it = i->children.begin();

    HGVS_ASSERT_RULE(it, eID_aminoacid);
    TIterator it2 = it->children.begin();

    string seq_str(it2->value.begin(), it2->value.end());
    seq_str = s_hgvsaa2ncbieaa(seq_str);

    if(context.GetPlacement().GetMol() != CVariantPlacement::eMol_protein) {
        HGVS_THROW(eContext, "Expected protein context");
    }

    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_prot_missense);

    SetFirstPlacement(*vr).Assign(context.GetPlacement());

    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetLiteral().SetSeq_data().SetNcbieaa().Set(seq_str);
    delta->SetSeq().SetLiteral().SetLength(1);
    var_inst.SetDelta().push_back(delta);

    return vr;
}


CRef<CVariation> CHgvsParser::x_identity(const CContext& context)
{
    CRef<CVariation> vr(new CVariation);
    CVariation_inst& var_inst = vr->SetData().SetInstance();
    var_inst.SetType(CVariation_inst::eType_identity);


    SetFirstPlacement(*vr).Assign(context.GetPlacement());


    TDelta delta(new TDelta::TObjectType);
    delta->SetSeq().SetThis();
    var_inst.SetDelta().push_back(delta);

    return vr;
}


CRef<CVariation> CHgvsParser::x_mut_inst(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_mut_inst);

    TIterator it = i->children.begin();

    CRef<CVariation> vr(new CVariation);
    if(it->value.id() == SGrammar::eID_mut_inst) {
        string s(it->value.begin(), it->value.end());
        if(s == "?") {
            vr->SetData().SetUnknown();
            SetFirstPlacement(*vr).Assign(context.GetPlacement());
        } else if(s == "=") {
            vr = x_identity(context);
        } else {
            HGVS_THROW(eGrammatic, "Unexpected inst terminal: " + s);
        }
    } else {
        vr =
            it->value.id() == SGrammar::eID_delins        ? x_delins(it, context)
          : it->value.id() == SGrammar::eID_deletion      ? x_deletion(it, context)
          : it->value.id() == SGrammar::eID_insertion     ? x_insertion(it, context)
          : it->value.id() == SGrammar::eID_duplication   ? x_duplication(it, context)
          : it->value.id() == SGrammar::eID_nuc_subst     ? x_nuc_subst(it, context)
          : it->value.id() == SGrammar::eID_nuc_inv       ? x_nuc_inv(it, context)
          : it->value.id() == SGrammar::eID_ssr           ? x_ssr(it, context)
          : it->value.id() == SGrammar::eID_conversion    ? x_conversion(it, context)
          : it->value.id() == SGrammar::eID_prot_ext      ? x_prot_ext(it, context)
          : it->value.id() == SGrammar::eID_prot_fs       ? x_prot_fs(it, context)
          : it->value.id() == SGrammar::eID_prot_missense ? x_prot_missense(it, context)
          : it->value.id() == SGrammar::eID_translocation ? x_translocation(it, context)
          : CRef<CVariation>(NULL);

        if(vr.IsNull()) {
            HGVS_ASSERT_RULE(it, eID_NONE);
        }
    }

    return vr;
}

CRef<CVariation> CHgvsParser::x_expr1(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_expr1);
    TIterator it = i->children.begin();
    CRef<CVariation> vr;

    string s(it->value.begin(), it->value.end());
    if(it->value.id() == i->value.id() && s == "(") {
        ++it;
        vr = x_expr1(it, context);
        SetComputational(*vr);
    } else if(it->value.id() == SGrammar::eID_list1a) {
        vr = x_list(it, context);
    } else if(it->value.id() == SGrammar::eID_header) {
        CContext local_ctx = x_header(it, context);
        ++it;
        vr = x_expr2(it, local_ctx);
    } else if(it->value.id() == SGrammar::eID_translocation) {
        vr = x_translocation(it, context);
    } else {
        HGVS_ASSERT_RULE(it, eID_NONE);
    }

    return vr;
}

CRef<CVariation> CHgvsParser::x_expr2(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_expr2);
    TIterator it = i->children.begin();
    CRef<CVariation> vr;

    string s(it->value.begin(), it->value.end());
    if(it->value.id() == i->value.id() && s == "(") {
        ++it;
        vr = x_expr2(it, context);
        SetComputational(*vr);
    } else if(it->value.id() == SGrammar::eID_list2a) {
        vr = x_list(it, context);
    } else if(it->value.id() == SGrammar::eID_location) {
        CContext local_context(context);
        CRef<CVariantPlacement> placement = x_location(it, local_context);
        local_context.SetPlacement().Assign(*placement);
        ++it;
        vr = x_expr3(it, local_context);
    } else if(it->value.id() == SGrammar::eID_prot_ext) {
        vr = x_prot_ext(it, context);
    } else if(it->value.id() == i->value.id()) {
        vr.Reset(new CVariation);
        if(s == "?") {
            vr->SetData().SetUnknown();
            SetFirstPlacement(*vr).SetLoc().SetEmpty().Assign(context.GetId());
        } else if(s == "0?" || s == "0") {
            vr->SetData().SetUnknown();
            typedef CVariation::TConsequence::value_type::TObjectType TConsequence;
            CRef<TConsequence> cons(new TConsequence);
            cons->SetNote("loss of product");
            vr->SetConsequence().push_back(cons);
            SetFirstPlacement(*vr).SetLoc().SetEmpty().Assign(context.GetId());
            if(s == "0?") {
                SetComputational(*vr);
            }
        } else if(s == "=") {
            vr = x_identity(context);
        } else {
            HGVS_THROW(eGrammatic, "Unexpected expr terminal: " + s);
        }
    } else {
        HGVS_ASSERT_RULE(it, eID_NONE);
    }

    return vr;
}


CRef<CVariation> CHgvsParser::x_expr3(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_expr3);
    TIterator it = i->children.begin();
    CRef<CVariation> vr;

    string s(it->value.begin(), it->value.end());
    if(it->value.id() == i->value.id() && s == "(") {
        ++it;
        vr = x_expr3(it, context);
        SetComputational(*vr);
    } else if(it->value.id() == SGrammar::eID_list3a) {
        vr = x_list(it, context);
    } else if(it->value.id() == SGrammar::eID_mut_inst) {
        vr.Reset(new CVariation);
        vr->SetData().SetSet().SetType(CVariation::TData::TSet::eData_set_type_compound);
        for(; it != i->children.end(); ++it) {
            CRef<CVariation> inst_ref = x_mut_inst(it, context);

            if(inst_ref->GetData().IsNote()
              && inst_ref->GetData().GetNote() == "Frameshift"
              && vr->SetData().SetSet().SetVariations().size() > 1)
            {
                //if inst_ref is a frameshift subexpression, it is attached as attribute of the
                //last variation, as frameshift is not a subtype of Variation.data, and thus
                //not represented as a separate subvariation.

                vr->SetData().SetSet().SetVariations().back()->SetFrameshift().Assign(inst_ref->GetFrameshift());
            } else {
                vr->SetData().SetSet().SetVariations().push_back(inst_ref);
            }
        }
        vr = x_unwrap_iff_singleton(*vr);
    } else {
        HGVS_ASSERT_RULE(it, eID_NONE);
    }

    return vr;
}

CRef<CVariation> CHgvsParser::x_list(TIterator const& i, const CContext& context)
{
    if(!SGrammar::s_is_list(i->value.id())) {
        HGVS_ASSERT_RULE(i, eID_NONE);
    }

    CRef<CVariation> vr(new CVariation);
    TVariationSet& varset = vr->SetData().SetSet();
    varset.SetType(CVariation::TData::TSet::eData_set_type_unknown);
    string delimiter = "";

    for(TIterator it = i->children.begin(); it != i->children.end(); ++it) {
        //will process two elements from the children list: delimiter and following expression; the first one does not have the delimiter.
        if(it != i->children.begin()) {
            string delim(it->value.begin(), it->value.end());
            if(it->value.id() != i->value.id()) {
                HGVS_THROW(eGrammatic, "Expected terminal");
            } else if(delimiter == "") {
                //first time
                delimiter = delim;
            } else if(delimiter != delim) {
                HGVS_THROW(eSemantic, "Non-unique delimiters within a list");
            }
            ++it;
        } 

        CRef<CVariation> vr;
        if(it->value.id() == SGrammar::eID_expr1) {
            vr = x_expr1(it, context);
        } else if(it->value.id() == SGrammar::eID_expr2) {
            vr = x_expr2(it, context);
        } else if(it->value.id() == SGrammar::eID_expr3) {
            vr = x_expr3(it, context);
        } else if(SGrammar::s_is_list(it->value.id())) {
            vr = x_list(it, context);
        } else {
            HGVS_ASSERT_RULE(it, eID_NONE);
        }

        varset.SetVariations().push_back(vr);
    }

    if(delimiter == ";") {
        varset.SetType(CVariation::TData::TSet::eData_set_type_haplotype);
    } else if(delimiter == "+") { 
        varset.SetType(CVariation::TData::TSet::eData_set_type_genotype);
    } else if(delimiter == "(+)") {
        varset.SetType(CVariation::TData::TSet::eData_set_type_individual);
    } else if(delimiter == ",") { 
        //if the context is rna (r.) then this describes multiple products from the same precursor;
        //otherwise this describes mosaic cases
        if(context.GetPlacement().GetMol() == CVariantPlacement::eMol_rna) {
            //Note: GetMolType(check=false) because MolType may not eMol_not_set, as
            //there may not be a sequence in context, e.g.
            //[NM_004004.2:c.35delG,NM_006783.1:c.689_690insT]" - individual
            //elements have their own sequence context, but none at the set level.
            varset.SetType(CVariation::TData::TSet::eData_set_type_products);
        } else {
            varset.SetType(CVariation::TData::TSet::eData_set_type_mosaic);
        }
    } else if(delimiter == "") {
        ;//single-element list
    } else {
        HGVS_THROW(eGrammatic, "Unexpected delimiter " + delimiter);
    }

    vr = x_unwrap_iff_singleton(*vr);
    return vr;
}


CRef<CVariation> CHgvsParser::x_root(TIterator const& i, const CContext& context)
{
    HGVS_ASSERT_RULE(i, eID_root);

    CRef<CVariation> vr = x_list(i, context);

    CVariationUtil::s_FactorOutPlacements(*vr);
    RepackageAssertedSequence(*vr);

    vr->Index();
    return vr;
}

CRef<CVariation>  CHgvsParser::x_unwrap_iff_singleton(CVariation& v)
{
    if(v.GetData().IsSet() && v.GetData().GetSet().GetVariations().size() == 1) {
        CRef<CVariation> first = v.SetData().SetSet().SetVariations().front();
        if(!first->IsSetPlacements() && v.IsSetPlacements()) {
            first->SetPlacements() = v.SetPlacements();
        }
        return first;
    } else {
        return CRef<CVariation>(&v);
    }
}


CRef<CVariation> CHgvsParser::AsVariation(const string& hgvs_expression, TOpFlags flags)
{
    tree_parse_info<> info = pt_parse(hgvs_expression.c_str(), s_grammar, +space_p);
    CRef<CVariation> vr;

    try {
        if(!info.full) {
#if 0
            CNcbiOstrstream ostr;
            tree_to_xml(ostr, info.trees, hgvs_expression.c_str() , CHgvsParser::SGrammar::s_GetRuleNames());
            string tree_str = CNcbiOstrstreamToString(ostr);
#endif
            HGVS_THROW(eGrammatic, "Syntax error at pos " + NStr::IntToString(info.length + 1));
        } else {
            CContext context(m_scope);
            vr = x_root(info.trees.begin(), context);

            vr->SetName(hgvs_expression);
        }
    } catch (CException& e) {
        if(flags && fOpFlags_RelaxedAA && NStr::Find(hgvs_expression, "p.")) {
            //expression was protein, try non-hgvs-compliant representation of prots
            string hgvs_expr2 = s_hgvsUCaa2hgvsUL(hgvs_expression);
            TOpFlags flags2 = flags & ~fOpFlags_RelaxedAA; //unset the bit so we don't infinite-recurse
            vr = AsVariation(hgvs_expr2, flags2);
        } else {
            NCBI_RETHROW_SAME(e, "");
        }
    }

    return vr;
}


};

END_NCBI_SCOPE

