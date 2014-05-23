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
 * Author:  J. Chen
 *
 * File Description:
 *   suspect product name check against rule
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'macro.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/macro/Suspect_rule.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSuspect_rule::~CSuspect_rule(void)
{
}

/*
bool CSuspect_rule :: Match(const string& str, const CSeq_feat& feat, CConstRef <CScope> scope)
{
   if (StringMatchesSuspectProductRule(str)) {
    
    // list the coding region, rather than the protein feature, if possible
    const CSeq_feat* feat_pnt = &feat;
    CBioseq_Handle 
       bioseq_hl 
         = sequence::GetBioseqFromSeqLoc(feat.GetLocation(), (CScope&)(scope));
    if (feat.GetData().IsProt()) {
      feat_pnt = sequence::GetCDSForProduct(bioseq_hl);
      if (!feat_pnt) {
         feat_pnt = &feat;
      }
    }

    // CConstraint_choice_set
    if (!CanGetFeat_constraint()) {
         return true;
    }
    else {
        if (!feat_pnt) {
            return false;
        }
        else {
          return GetFeat_constraint().Match(*feat_pnt, scope);
        }
    }
  }

  return false;
};
*/

bool CSuspect_rule :: StringMatchesSuspectProductRule(const string& str) const
{
  // CSearch_func: only about string
  const CSearch_func& func = GetFind();
  if (!func.Empty() && !func.Match(str)) {
      return false;
  }
  else if (CanGetExcept()) {
     const CSearch_func& exc_func = GetExcept();
     if (!exc_func.Empty() && !exc_func.Match(str)) {
       return false;
     }
  }
  return true;
};


/*

string CSuspect_rule :: GetSrcQualName(ESource_qual src_qual)
{
   string strtmp = ENUM_METHOD_NAME(ESource_qual)()->FindName(src_qual, true);
   if (strtmp == "bio-material-INST") {
      strtmp = "bio-material-inst";
   }
   else if (strtmp == "bio-material-COLL") {
      strtmp = "bio-material-coll";
   }
   else if (strtmp == "bio-material-SpecID") {
      strtmp = "bio-material-specid";
   }
   else if (strtmp == "common-name") {
      strtmp = "common name";
   }
   else if (strtmp == "culture-collection-INST") {
      strtmp = "culture-collection-inst";
   }
   else if (strtmp == "culture-collection-COLL") {
      strtmp = "culture-collection-coll";
   }
   else if (strtmp == "culture-collection-SpecID") {
      strtmp = "culture-collection-specid";
   }
   else if (strtmp == "orgmod-note") {
      strtmp = "note-orgmod";
   }
   else if (strtmp == "nat-host") {
      strtmp = "host";
   }
   else if (strtmp == "subsource-note") {
      strtmp = "sub-species";
   }
   else if (strtmp == "all-notes") {
      strtmp = "All Notes";
   }
   else if (strtmp == "all-quals") {
      strtmp = "All";
   }
   else if (strtmp == "all-primers") {
      strtmp = "All Primers";
   }

   return strtmp;
};

bool CSuspect_rule :: IsSubsrcQual(ESource_qual src_qual)
{
   switch (src_qual) {
     case eSource_qual_cell_line:
     case eSource_qual_cell_type:
     case eSource_qual_chromosome:
     case eSource_qual_clone:
     case eSource_qual_clone_lib:
     case eSource_qual_collected_by:
     case eSource_qual_collection_date:
     case eSource_qual_country:
     case eSource_qual_dev_stage:
     case eSource_qual_endogenous_virus_name:
     case eSource_qual_environmental_sample:
     case eSource_qual_frequency:
     case eSource_qual_fwd_primer_name:
     case eSource_qual_fwd_primer_seq:
     case eSource_qual_genotype:
     case eSource_qual_germline:
     case eSource_qual_haplotype:
     case eSource_qual_identified_by:
     case eSource_qual_insertion_seq_name:
     case eSource_qual_isolation_source:
     case eSource_qual_lab_host:
     case eSource_qual_lat_lon:
     case eSource_qual_map:
     case eSource_qual_metagenomic:
     case eSource_qual_plasmid_name:
     case eSource_qual_plastid_name:
     case eSource_qual_pop_variant:
     case eSource_qual_rearranged:
     case eSource_qual_rev_primer_name:
     case eSource_qual_rev_primer_seq:
     case eSource_qual_segment:
     case eSource_qual_sex:
     case eSource_qual_subclone:
     case eSource_qual_subsource_note:
     case eSource_qual_tissue_lib :
     case eSource_qual_tissue_type:
     case eSource_qual_transgenic:
     case eSource_qual_transposon_name:
     case eSource_qual_mating_type:
     case eSource_qual_linkage_group:
     case eSource_qual_haplogroup:
     case eSource_qual_altitude:
        return true;
     default: return false;
   }
};

bool CSuspect_rule :: x_DoesObjectMatchFieldConstraint(const CSeq_feat& data, const CField_constraint& field_cons) const 
{
  const CString_constraint& str_cons = field_cons.GetString_constraint();
  if (x_IsStringConstraintEmpty (&str_cons)) {
     return true;
  }

  bool rval = false;
  string str(kEmptyStr);
  const CField_type& field_type = field_cons.GetField();
  switch (field_type.Which()) {
    case CField_type::e_Source_qual:
      {
        const CBioSource* biosrc = sequence::GetBioSource(m_bioseq_hl);
        if (biosrc) {
          str = x_GetSrcQualValue4FieldType(*biosrc, 
                                      field_type.GetSource_qual(), &str_cons);
        }
        if (!str.empty()) rval = true;
      }
      break;
    case CField_type:: e_Feature_field:
      {
          const CFeature_field& feat_field = field_type.GetFeature_field();
          rval = DoesObjectMatchFeatureFieldConstraint(data, feat_field, str_cons);
      }
      break;
    case CField_type:: e_Rna_field:
      rval = DoesObjectMatchRnaQualConstraint (data, field_type.GetRna_field(), str_cons);
    case CField_type:: e_Cds_gene_prot:
      {
         CRef <CFeature_field> 
             feat_field = FeatureFieldFromCDSGeneProtField (field_type.GetCds_gene_prot());
         rval = DoesObjectMatchFeatureFieldConstraint (
                                    data, const_cast<CFeature_field&>(*feat_field), str_cons);
      }
      break;
    case CField_type:: e_Molinfo_field:
      if (m_bioseq_hl) {
        str = GetSequenceQualFromBioseq (field_type.GetMolinfo_field());
        if ( (str.empty() && str_cons.GetNot_present()) 
                || (!str.empty() && DoesStringMatchConstraint (str, &str_cons)) )
            rval = true;
      }
      break;
    case CField_type:: e_Misc:
    case CField_type:: e_Dblink:
      if (m_bioseq_hl) {
        str = GetFieldValueForObjectEx (field_type, str_cons);
        if (!str.empty()) rval = true;
      }
      break;

* TODO LATER * 
    case CField_type:: e_Pub:
      break;
    default: break;
  }
  return rval; 
};
bool CSuspect_rule :: x_DoesObjectMatchConstraint(const CSeq_feat& data, const vector <string>& strs, const CConstraint_choice& cons) const
{
  switch (cons.Which()) {
    case CConstraint_choice::e_String :
       return x_DoesObjectMatchStringConstraint (data, strs, cons.GetString());
    case CConstraint_choice::e_Location :
       return x_DoesFeatureMatchLocationConstraint (data, cons.GetLocation());
    case CConstraint_choice::e_Field :
       return x_DoesObjectMatchFieldConstraint (data, cons.GetField());
    case CConstraint_choice::e_Source :
       if (data.GetData().IsBiosrc()) {
          return x_DoesBiosourceMatchConstraint ( data.GetData().GetBiosrc(), 
                                                cons.GetSource());
       }
       else {
          CBioseq_Handle 
             seq_hl = sequence::GetBioseqFromSeqLoc(data.GetLocation(), 
                                                    *thisInfo.scope);
          const CBioSource* src = GetBioSource(seq_hl);
          if (src) {
            return x_DoesBiosourceMatchConstraint(*src, cons.GetSource());
          }
          else return false;
       }
       break;
    case CConstraint_choice::e_Cdsgeneprot_qual :
       return x_DoesFeatureMatchCGPQualConstraint (data, 
                                                 cons.GetCdsgeneprot_qual());
*
      if (choice == 0) {
        rval = DoesCGPSetMatchQualConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_SEQDESC) {
        rval = DoesSeqDescMatchCGPQualConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_SEQFEAT) {
        rval = DoesFeatureMatchCGPQualConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_BIOSEQ) {
        rval = DoesSequenceMatchCGPQualConstraint (data, cons->data.ptrvalue);
      } else {
        rval = FALSE;
      }
*
    case CConstraint_choice::e_Cdsgeneprot_pseudo :
       return x_DoesFeatureMatchCGPPseudoConstraint (data, cons.GetCdsgeneprot_pseudo());
*
      if (choice == 0) {
        rval = DoesCGPSetMatchPseudoConstraint (data, cons->data.ptrvalue);
      } else if (choice == OBJ_SEQFEAT) {
        rval = DoesFeatureMatchCGPPseudoConstraint (data, cons->data.ptrvalue);
      }
*
    case CConstraint_choice::e_Sequence :
      {
        if (m_bioseq_hl) { 
            return x_DoesSequenceMatchSequenceConstraint(cons.GetSequence());
        }
        break;
      }
    case CConstraint_choice::e_Pub:
      if (data.GetData().IsPub()) {
         return x_DoesPubMatchPublicationConstraint(data.GetData().GetPub(), cons.GetPub());
      }
      break;
    case CConstraint_choice::e_Molinfo:
       return x_DoesObjectMatchMolinfoFieldConstraint (data, cons.GetMolinfo()); // use bioseq_hl
    case CConstraint_choice::e_Field_missing:
     if (x_GetConstraintFieldFromObject(data, cons.GetField_missing()).empty()){
           return true; 
     }
     else return false;
    case CConstraint_choice::e_Translation:
     * must be coding region or protein feature *
      if (data.GetData().IsProt()) {
         const CSeq_feat* cds = sequence::GetCDSForProduct(m_bioseq_hl);
         if (cds) {
            return x_DoesCodingRegionMatchTranslationConstraint (
                              *cds, cons.GetTranslation());
         }
      }
      else if (data.GetData().IsCdregion()) {
         return x_DoesCodingRegionMatchTranslationConstraint(
                                             data, cons.GetTranslation());
      }
    default: break;
  }
  return true;
}

*/

bool CSuspect_rule::ApplyToString(string& val) const
{
    if (!IsSetReplace() || !StringMatchesSuspectProductRule(val)) {
        return false;
    }

    CRef<CString_constraint> constraint(NULL);
    if (IsSetFind() && GetFind().IsString_constraint()) {
        constraint.Reset(new CString_constraint());
        constraint->Assign(GetFind().GetString_constraint());
    }
    return GetReplace().ApplyToString(val, constraint);
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1729, CRC32: 243f48c6 */
