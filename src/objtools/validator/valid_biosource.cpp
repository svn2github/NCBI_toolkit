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
 * Author:  Jonathan Kans, Clifford Clausen, Aaron Ucko, Mati Shomrat, ....
 *
 * File Description:
 *   Implementation of private parts of the validator
 *   .......
 *
 */
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbistr.hpp>
#include <corelib/ncbiapp.hpp>
#include <objmgr/object_manager.hpp>

#include "validatorp.hpp"
#include "utilities.hpp"

#include <serial/iterator.hpp>
#include <serial/enumvalues.hpp>

#include <objects/general/Dbtag.hpp>
#include <objects/general/Person_id.hpp>
#include <objects/general/Name_std.hpp>

#include <objects/seqalign/Seq_align.hpp>

#include <objects/seqset/Bioseq_set.hpp>
#include <objects/seqset/Seq_entry.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seq/Seqdesc.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objects/seq/Pubdesc.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/OrgMod.hpp>
#include <objects/seqfeat/OrgName.hpp>
#include <objects/seqfeat/Org_ref.hpp>
#include <objects/seqfeat/Seq_feat.hpp>
#include <objects/seqfeat/SubSource.hpp>

#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seqloc/Seq_interval.hpp>
#include <objects/seqloc/Seq_point.hpp>
#include <objects/seqloc/Textseq_id.hpp>

#include <objects/seqres/Seq_graph.hpp>

#include <objects/submit/Seq_submit.hpp>
#include <objects/submit/Submit_block.hpp>

#include <objmgr/bioseq_ci.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/graph_ci.hpp>
#include <objmgr/seq_annot_ci.hpp>
#include <objmgr/util/feature.hpp>
#include <objmgr/util/sequence.hpp>

#include <objmgr/feat_ci.hpp>
#include <objmgr/align_ci.hpp>
#include <objmgr/seq_vector.hpp>
#include <objmgr/scope.hpp>

#include <objects/pub/Pub.hpp>
#include <objects/pub/Pub_equiv.hpp>

#include <objects/biblio/Author.hpp>
#include <objects/biblio/Auth_list.hpp>
#include <objects/biblio/Cit_art.hpp>
#include <objects/biblio/Cit_book.hpp>
#include <objects/biblio/Cit_gen.hpp>
#include <objects/biblio/Cit_jour.hpp>
#include <objects/biblio/Cit_let.hpp>
#include <objects/biblio/Cit_proc.hpp>
#include <objects/biblio/Cit_sub.hpp>
#include <objects/biblio/PubMedId.hpp>
#include <objects/biblio/PubStatus.hpp>
#include <objects/biblio/Title.hpp>
#include <objects/biblio/Imprint.hpp>
#include <objects/biblio/Affil.hpp>
#include <objects/misc/sequence_macros.hpp>
#include <objects/taxon3/taxon3.hpp>
#include <objects/taxon3/Taxon3_reply.hpp>

#include <objtools/error_codes.hpp>
#include <util/sgml_entity.hpp>
#include <util/line_reader.hpp>

#include <algorithm>


#include <serial/iterator.hpp>

#define NCBI_USE_ERRCODE_X   Objtools_Validator

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)
BEGIN_SCOPE(validator)
using namespace sequence;


auto_ptr<CTextFsa> CValidError_imp::m_SourceQualTags;





static bool s_UnbalancedParentheses (string str)
{
	if (NStr::IsBlank(str)) {
		return false;
	}

    int par = 0, bkt = 0;
	string::iterator it = str.begin();
	while (it != str.end()) {
		if (*it == '(') {
            ++par;
		} else if (*it == ')') {
			--par;
			if (par < 0) {
				return true;
			}
		} else if (*it == '[') {
			++bkt;
		} else if (*it == ']') {
			--bkt;
			if (bkt < 0) {
				return true;
			}
		}
		++it;
	}
	if (par > 0 || bkt > 0) {
		return true;
	} else {
		return false;
	}
}


const string sm_ValidSexQualifierValues[] = {
  "female",
  "male",
  "hermaphrodite",
  "unisexual",
  "bisexual",
  "asexual",
  "monoecious",
  "monecious",
  "dioecious",
  "diecious",
};

static bool s_IsValidSexQualifierValue (string str)

{
	str = NStr::ToLower(str);

    const string *begin = sm_ValidSexQualifierValues;
    const string *end = &(sm_ValidSexQualifierValues[sizeof(sm_ValidSexQualifierValues) / sizeof(string)]);

    return find(begin, end, str) != end;
}


const string sm_ValidModifiedPrimerBases[] = {
  "ac4c",
  "chm5u",
  "cm",
  "cmnm5s2u",
  "cmnm5u",
  "d",
  "fm",
  "gal q",
  "gm",
  "i",
  "i6a",
  "m1a",
  "m1f",
  "m1g",
  "m1i",
  "m22g",
  "m2a",
  "m2g",
  "m3c",
  "m5c",
  "m6a",
  "m7g",
  "mam5u",
  "mam5s2u",
  "man q",
  "mcm5s2u",
  "mcm5u",
  "mo5u",
  "ms2i6a",
  "ms2t6a",
  "mt6a",
  "mv",
  "o5u",
  "osyw",
  "p",
  "q",
  "s2c",
  "s2t",
  "s2u",
  "s4u",
  "t",
  "t6a",
  "tm",
  "um",
  "yw",
  "x",
  "OTHER"
};

static bool s_IsValidPrimerSequence (string str, char& bad_ch)
{
	bad_ch = 0;
	if (NStr::IsBlank(str)) {
		return false;
	}

	if (NStr::Find(str, ",") == string::npos) {
		if (NStr::Find(str, "(") != string::npos
			|| NStr::Find(str, ")") != string::npos) {
			return false;
		}
	} else {
		if (!NStr::StartsWith(str, "(") || !NStr::EndsWith(str, ")")) {
			return false;
		}
	}

	if (NStr::Find(str, ";") != string::npos) {
		return false;
	}

    const string *list_begin = sm_ValidModifiedPrimerBases;
    const string *list_end = &(sm_ValidModifiedPrimerBases[sizeof(sm_ValidModifiedPrimerBases) / sizeof(string)]);

	size_t pos = 0;
	string::iterator sit = str.begin();
	while (sit != str.end()) {
		if (*sit == '<') {
			size_t pos2 = NStr::Find (str, ">", pos + 1);
			if (pos2 == string::npos) {
				bad_ch = '<';
				return false;
			}
			string match = str.substr(pos + 1, pos2 - pos - 1);
			if (find(list_begin, list_end, match) == list_end) {
				bad_ch = '<';
				return false;
			}
			sit += pos2 - pos + 1;
			pos = pos2 + 1;
		} else {
			if (*sit != '(' && *sit != ')' && *sit != ',' && *sit != ':') {
				if (!isalpha (*sit)) {
				    bad_ch = *sit;
					return false;
				}
				char ch = toupper(*sit);
				if (strchr ("ABCDGHKMNRSTVWY", ch) == NULL) {
					bad_ch = tolower (ch);
					return false;
				}
			}
			++sit;
			++pos;
		}
	}

	return true;
}


static void s_IsCorrectLatLonFormat (string lat_lon, bool& format_correct, bool& lat_in_range, bool& lon_in_range,
									 double& lat_value, double& lon_value)
{
	format_correct = false;
	lat_in_range = false;
	lon_in_range = false;
	double ns, ew;
	char lon, lat;
	int processed;

	lat_value = 0.0;
	lon_value = 0.0;
	
	if (NStr::IsBlank(lat_lon)) {
		return;
	} else if (sscanf (lat_lon.c_str(), "%lf %c %lf %c%n", &ns, &lat, &ew, &lon, &processed) != 4
		       || processed != lat_lon.length()) {
		return;
	} else if ((lat != 'N' && lat != 'S') || (lon != 'E' && lon != 'W')) {
		return;
	} else {
        // init values found
		if (lat == 'N') {
			lat_value = ns;
		} else {
			lat_value = 0.0 - ns;
		}
		if (lon == 'E') {
			lon_value = ew;
		} else {
			lon_value = 0.0 - ew;
		}

        // make sure format is correct
		vector<string> pieces;
		NStr::Tokenize(lat_lon, " ", pieces);
		if (pieces.size() > 3) {
			int precision_lat = 0;
			size_t pos = NStr::Find(pieces[0], ".");
			if (pos != string::npos) {
				precision_lat = pieces[0].length() - pos - 1;
			}
			int precision_lon = 0;
			pos = NStr::Find(pieces[2], ".");
			if (pos != string::npos) {
				precision_lon = pieces[2].length() - pos - 1;
			}
			
			char reformatted[1000];
			sprintf (reformatted, "%.*lf %c %.*lf %c", precision_lat, ns, lat,
				                                       precision_lon, ew, lon);

			size_t len = strlen (reformatted);
			if (NStr::StartsWith(lat_lon, reformatted)
				&& (len == lat_lon.length() 
				  || (len < lat_lon.length() 
				      && lat_lon.c_str()[len] == ';'))) {
                format_correct = true;
				if (ns <= 90 && ns >= 0) {
					lat_in_range = true;
				}
				if (ew <= 180 && ew >= 0) {
				    lon_in_range = true;
				}
			}
		}
	}
}


bool CValidError_imp::IsSyntheticConstruct (const CBioSource& src) 
{
    if (!src.IsSetOrg()) {
        return false;
    }
    if (src.GetOrg().IsSetTaxname()) {
        string taxname = src.GetOrg().GetTaxname();
        if (NStr::EqualNocase(taxname, "synthetic construct") || NStr::FindNoCase(taxname, "vector") != string::npos) {
            return true;
        }
    }

    if (src.GetOrg().IsSetLineage()) {
        if (NStr::FindNoCase(src.GetOrg().GetLineage(), "artificial sequences") != string::npos) {
            return true;
        }
    }

    if (src.GetOrg().IsSetOrgname() && src.GetOrg().GetOrgname().IsSetDiv()
        && NStr::EqualNocase(src.GetOrg().GetOrgname().GetDiv(), "syn")) {
        return true;
    }
    return false;
}


bool CValidError_imp::IsArtificial (const CBioSource& src) 
{
    if (src.IsSetOrigin() 
        && (src.GetOrigin() == CBioSource::eOrigin_artificial
            || src.GetOrigin() == CBioSource::eOrigin_synthetic)) {
        return true;
    }
    return false;
}


bool CValidError_imp::IsOrganelle (int genome)
{
    bool rval = false;
    switch (genome) {
        case CBioSource::eGenome_chloroplast:
        case CBioSource::eGenome_chromoplast:
        case CBioSource::eGenome_kinetoplast:
        case CBioSource::eGenome_mitochondrion:
        case CBioSource::eGenome_cyanelle:
        case CBioSource::eGenome_nucleomorph:
        case CBioSource::eGenome_apicoplast:
        case CBioSource::eGenome_leucoplast:
        case CBioSource::eGenome_proplastid:
        case CBioSource::eGenome_hydrogenosome:
            rval = true;
            break;
        default:
            rval = false;
            break;
    }
    return rval;
}


bool CValidError_imp::IsOrganelle (CBioseq_Handle seq)
{
    bool rval = false;
    CSeqdesc_CI sd(seq, CSeqdesc::e_Source);
    if (sd && sd->GetSource().IsSetGenome() && IsOrganelle(sd->GetSource().GetGenome())) {
        rval = true;
    }
    return rval;
}


void CValidError_imp::ValidateBioSource
(const CBioSource&    bsrc,
 const CSerialObject& obj,
 const CSeq_entry *ctx)
{
    bool is_synthetic_construct = IsSyntheticConstruct (bsrc);
    bool is_artificial = IsArtificial(bsrc);

    if (is_synthetic_construct) {
        if (!is_artificial) {
            PostObjErr (eDiag_Error, eErr_SEQ_DESCR_InvalidForType, 
                        "synthetic construct should have artificial origin", obj, ctx);
        }
    } else if (is_artificial) {
        PostObjErr (eDiag_Error, eErr_SEQ_DESCR_InvalidForType, 
                    "artificial origin should have other-genetic and synthetic construct", obj, ctx);
    }

    if ( !bsrc.CanGetOrg() ) {
        PostObjErr(eDiag_Error, eErr_SEQ_DESCR_NoOrgFound,
            "No organism has been applied to this Bioseq.  Other qualifiers may exist.", obj, ctx);
        return;
    }

	const COrg_ref& orgref = bsrc.GetOrg();
  
	// validate legal locations.
	if ( bsrc.GetGenome() == CBioSource::eGenome_transposon  ||
		 bsrc.GetGenome() == CBioSource::eGenome_insertion_seq ) {
		  PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_ObsoleteSourceLocation,
            "Transposon and insertion sequence are no longer legal locations",
            obj, ctx);
	}

	if (IsIndexerVersion()
		&& bsrc.IsSetGenome() 
		&& bsrc.GetGenome() == CBioSource::eGenome_chromosome) {
		PostObjErr(eDiag_Info, eErr_SEQ_DESCR_ChromosomeLocation, 
			       "INDEXER_ONLY - BioSource location is chromosome",
				   obj, ctx);
	}

    bool isViral = false, isAnimal = false, isPlant = false, isBacteria = false, isArchaea = false, isFungal = false;
	if (bsrc.IsSetLineage()) {
		string lineage = bsrc.GetLineage();
		if (NStr::StartsWith(lineage, "Viruses; ", NStr::eNocase)) {
            isViral = true;
	    } else if (NStr::StartsWith(lineage, "Eukaryota; Metazoa; ", NStr::eNocase)) {
			isAnimal = true;
		} else if (NStr::StartsWith(lineage, "Eukaryota; Viridiplantae; Streptophyta; Embryophyta; ", NStr::eNocase)
                   || NStr::StartsWith(lineage, "Eukaryota; Rhodophyta; ", NStr::eNocase)
                   || NStr::StartsWith(lineage, "Eukaryota; stramenopiles; Phaeophyceae; ", NStr::eNocase)) {
			isPlant = true;
		} else if (NStr::StartsWith(lineage, "Bacteria; ", NStr::eNocase)) {
			isBacteria = true;
		} else if (NStr::StartsWith(lineage, "Archaea; ", NStr::eNocase)) {
			isArchaea = true;
		} else if (NStr::StartsWith(lineage, "Eukaryota; Fungi; ", NStr::eNocase)) {
			isFungal = true;
		}
	}

	int chrom_count = 0;
	bool chrom_conflict = false;
	int country_count = 0, lat_lon_count = 0;
	int fwd_primer_seq_count = 0, rev_primer_seq_count = 0;
	int fwd_primer_name_count = 0, rev_primer_name_count = 0;
	CPCRSetList pcr_set_list;
    const CSubSource *chromosome = 0;
	string countryname;
	string lat_lon;
	double lat_value = 0.0, lon_value = 0.0;
    bool germline = false;
    bool rearranged = false;
	bool transgenic = false;
	bool env_sample = false;
	bool metagenomic = false;
	bool sex = false;
	bool mating_type = false;
	bool iso_source = false;

    FOR_EACH_SUBSOURCE_ON_BIOSOURCE (ssit, bsrc) {
        ValidateSubSource (**ssit, obj, ctx);
		if (!(*ssit)->IsSetSubtype()) {
			continue;
		}

		int subtype = (*ssit)->GetSubtype();
        switch ( subtype ) {
            
        case CSubSource::eSubtype_country:
			++country_count;
            if (!NStr::IsBlank (countryname)) {
                PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadCountryCode, 
                            "Multiple country names on BioSource", obj, ctx);
            }
            countryname = (**ssit).GetName();
            break;

		case CSubSource::eSubtype_lat_lon:
			if ((*ssit)->IsSetName()) {
				lat_lon = (*ssit)->GetName();
			    bool format_correct = false, lat_in_range = false, lon_in_range = false;
    			s_IsCorrectLatLonFormat (lat_lon, format_correct, 
	                     lat_in_range, lon_in_range,
						 lat_value, lon_value);
			}

			++lat_lon_count;
			if (lat_lon_count > 1) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_LatLonProblem, 
					       "Multiple lat_lon on BioSource", obj, ctx);
			}
			break;
            
        case CSubSource::eSubtype_chromosome:
            ++chrom_count;
            if ( chromosome != 0 ) {
                if ( NStr::CompareNocase((**ssit).GetName(), chromosome->GetName()) != 0) {
                    chrom_conflict = true;
                }          
            } else {
                chromosome = ssit->GetPointer();
            }
            break;

		case CSubSource::eSubtype_fwd_primer_name:
			if ((*ssit)->IsSetName()) {
				pcr_set_list.AddFwdName((*ssit)->GetName());
			}
			++fwd_primer_name_count;
			break;

		case CSubSource::eSubtype_rev_primer_name:
			if ((*ssit)->IsSetName()) {
				pcr_set_list.AddRevName((*ssit)->GetName());
			}
            ++rev_primer_name_count;
			break;
            
		case CSubSource::eSubtype_fwd_primer_seq:
			if ((*ssit)->IsSetName()) {
				pcr_set_list.AddFwdSeq((*ssit)->GetName());
			}
			++fwd_primer_seq_count;
			break;

		case CSubSource::eSubtype_rev_primer_seq:
			if ((*ssit)->IsSetName()) {
				pcr_set_list.AddRevSeq((*ssit)->GetName());
			}
			++rev_primer_seq_count;
			break;

		case CSubSource::eSubtype_transposon_name:
        case CSubSource::eSubtype_insertion_seq_name:
            break;
            
        case 0:
            break;
            
        case CSubSource::eSubtype_other:
            break;

        case CSubSource::eSubtype_germline:
            germline = true;
            break;

        case CSubSource::eSubtype_rearranged:
            rearranged = true;
            break;

        case CSubSource::eSubtype_transgenic:
            transgenic = true;
            break;

        case CSubSource::eSubtype_metagenomic:
            metagenomic = true;
            break;

		case CSubSource::eSubtype_environmental_sample:
            env_sample = true;
            break;

		case CSubSource::eSubtype_isolation_source:
            iso_source = true;
			break;

		case CSubSource::eSubtype_sex:
			sex = true;
			if (isAnimal || isPlant) {
				// always use /sex, do not check values at this time
			} else if (isViral || isBacteria || isArchaea || isFungal) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					"Unexpected use of /sex qualifier", obj, ctx);
			} else if (s_IsValidSexQualifierValue((*ssit)->GetName())) {
				// otherwise values are restricted to specific list
			} else {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					"Unexpected use of /sex qualifier", obj, ctx);
			}

			break;

		case CSubSource::eSubtype_mating_type:
			mating_type = true;
			if (isAnimal || isPlant || isViral) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					"Unexpected use of /mating_type qualifier", obj, ctx);
			} else if (s_IsValidSexQualifierValue((*ssit)->GetName())) {
				// complain if one of the values that should go in /sex
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					"Unexpected use of /mating_type qualifier", obj, ctx);
			}
			break;

		case CSubSource::eSubtype_plasmid_name:
			if (!bsrc.IsSetGenome() || bsrc.GetGenome() != CBioSource::eGenome_plasmid) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					"Plasmid subsource but not plasmid location", obj, ctx);
			}
			break;

		case CSubSource::eSubtype_plastid_name:
			{
				if ((*ssit)->IsSetName()) {
					CBioSource::TGenome genome = CBioSource::eGenome_unknown;
					if (bsrc.IsSetGenome()) {
						genome = bsrc.GetGenome();
					}

				    const string& subname = ((*ssit)->GetName());
					CBioSource::EGenome genome_from_name = CBioSource::GetGenomeByOrganelle (subname, NStr::eCase,  false);
                    if (genome_from_name == CBioSource::eGenome_chloroplast
						|| genome_from_name == CBioSource::eGenome_chromoplast
						|| genome_from_name == CBioSource::eGenome_kinetoplast
						|| genome_from_name == CBioSource::eGenome_plastid
						|| genome_from_name == CBioSource::eGenome_apicoplast
						|| genome_from_name == CBioSource::eGenome_leucoplast
						|| genome_from_name == CBioSource::eGenome_proplastid) {
					    if (genome_from_name != genome) {
							string val_name = CBioSource::GetOrganelleByGenome(genome_from_name);
							if (NStr::StartsWith(val_name, "plastid:")) {
								val_name = val_name.substr(8);
							}
							PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
								"Plastid name subsource " + val_name + " but not " + val_name + " location", obj, ctx);
						}
                    } else {
						PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
							"Plastid name subsource contains unrecognized value", obj, ctx);
                    }
				}
			}
			break;
		
		case CSubSource::eSubtype_cell_line:
			if (isViral) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					"Virus has unexpected cell_line qualifier", obj, ctx);
			}
			break;

		case CSubSource::eSubtype_cell_type:
			if (isViral) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					"Virus has unexpected cell_type qualifier", obj, ctx);
			}
			break;

		case CSubSource::eSubtype_tissue_type:
			if (isViral) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					"Virus has unexpected tissue_type qualifier", obj, ctx);
			}
			break;

		case CSubSource::eSubtype_frequency:
			break;

		case CSubSource::eSubtype_collection_date:
			break;

        default:
            break;
        }
    }
    if ( germline  &&  rearranged ) {
        PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
            "Germline and rearranged should not both be present", obj, ctx);
    }
	if (transgenic && env_sample) {
        PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
            "Transgenic and environmental sample should not both be present", obj, ctx);
    }
	if (metagenomic && (! env_sample)) {
		PostObjErr(eDiag_Critical, eErr_SEQ_DESCR_BioSourceInconsistency, 
			"Metagenomic should also have environmental sample annotated", obj, ctx);
	}
	if (sex && mating_type) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
			 "Sex and mating type should not both be present", obj, ctx);
	}

	if ( chrom_count > 1 ) {
		string msg = 
			chrom_conflict ? "Multiple conflicting chromosome qualifiers" :
							 "Multiple identical chromosome qualifiers";
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleChromosomes, msg, obj, ctx);
	}

    if (country_count > 1) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceQualifiers, 
			       "Multiple country qualifiers present", obj, ctx);
	}
    if (lat_lon_count > 1) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceQualifiers, 
			       "Multiple lat_lon qualifiers present", obj, ctx);
	}
    if (fwd_primer_seq_count > 1) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceQualifiers, 
			       "Multiple fwd_primer_seq qualifiers present", obj, ctx);
	}
    if (rev_primer_seq_count > 1) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceQualifiers, 
			       "Multiple rev_primer_seq qualifiers present", obj, ctx);
	}
    if (fwd_primer_name_count > 1) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceQualifiers, 
			       "Multiple fwd_primer_name qualifiers present", obj, ctx);
	}
    if (rev_primer_name_count > 1) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceQualifiers, 
			       "Multiple rev_primer_name qualifiers present", obj, ctx);
	}

	if ((fwd_primer_seq_count > 0 && rev_primer_seq_count == 0)
		|| (fwd_primer_seq_count == 0 && rev_primer_seq_count > 0)) {
        if (fwd_primer_name_count == 0 && rev_primer_name_count == 0) {
		    PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadPCRPrimerSequence, 
			           "PCR primer does not have both sequences", obj, ctx);
        }
	}

	if (!pcr_set_list.AreSetsUnique()) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_DuplicatePCRPrimerSequence,
					"PCR primer sequence has duplicates", obj, ctx);
	}

	// check that country and lat_lon are compatible
	if (!NStr::IsBlank(countryname) && !NStr::IsBlank(lat_lon)) {
		size_t pos = NStr::Find(countryname, ";");
		// first, get rid of comments after semicolon
		if (pos != string::npos) {
			countryname = countryname.substr(0, pos);
		}
		string test_country = countryname;
		pos = NStr::Find(test_country, ":");
		if (pos != string::npos) {
			test_country = test_country.substr(0, pos);
		}
		if (lat_lon_map.HaveLatLonForCountry(test_country)) {
			if (lat_lon_map.IsCountryInLatLon(test_country, lat_value, lon_value)) {
				// match, now try stricter match
				if (pos != string::npos) {
					test_country = countryname;
					size_t end_region = NStr::Find(test_country, ",", pos);
					if (end_region != string::npos) {
						test_country = test_country.substr(0, end_region);
					}
					if (lat_lon_map.HaveLatLonForCountry(test_country)
						&& !lat_lon_map.IsCountryInLatLon(test_country, lat_value, lon_value)
						&& !CCountryLatLonMap::DoesStringContainBodyOfWater(test_country)) {
						string guess = lat_lon_map.GuessCountryForLatLon(lat_value, lon_value);
						if (NStr::IsBlank(guess)) {
							PostObjErr (eDiag_Info, eErr_SEQ_DESCR_LatLonState,
									  "Lat_lon '" + lat_lon + "' does not map to subregion '" + test_country + "'",
									  obj, ctx);
						} else {
							PostObjErr (eDiag_Info, eErr_SEQ_DESCR_LatLonState,
									  "Lat_lon '" + lat_lon + "' does not map to subregion '" + test_country 
									  + "', but may be in '" + guess + "'",
									  obj, ctx);
						}
					}
				}
			} else {
				if (lat_lon_map.IsCountryInLatLon(test_country, -lat_value, lon_value)) {
					if (lat_value < 0.0) {
						PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonValue, 
							"Latitude should be set to N (northern hemisphere)",
							obj, ctx);
					} else {
						PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonValue, 
							"Latitude should be set to S (southern hemisphere)",
							obj, ctx);
					}
				} else if (lat_lon_map.IsCountryInLatLon(test_country, lat_value, -lon_value)) {
					if (lon_value < 0.0) {
						PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonValue, 
									"Longitude should be set to E (eastern hemisphere)",
									obj, ctx);
					} else {
						PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonValue, 
							"Longitude should be set to W (western hemisphere)",
									obj, ctx);
					}
				} else if (!CCountryLatLonMap::DoesStringContainBodyOfWater(test_country)) {
					string guess = lat_lon_map.GuessCountryForLatLon(lat_value, lon_value);
					if (NStr::IsBlank(guess)) {
						PostObjErr (eDiag_Info, eErr_SEQ_DESCR_LatLonCountry,
								  "Lat_lon '" + lat_lon + "' does not map to '" + test_country + "'",
								  obj, ctx);
					} else {
						PostObjErr (eDiag_Info, eErr_SEQ_DESCR_LatLonCountry,
								  "Lat_lon '" + lat_lon + "' does not map to '" + test_country 
								  + "', but may be in '" + guess + "'",
								  obj, ctx);
					}
				}
			}
		}
	}

	// validates orgref in the context of lineage
    if ( !orgref.IsSetOrgname()  ||
         !orgref.GetOrgname().IsSetLineage()  ||
         orgref.GetOrgname().GetLineage().empty() ) {
		PostObjErr(eDiag_Error, eErr_SEQ_DESCR_MissingLineage, 
			     "No lineage for this BioSource.", obj, ctx);
	} else {
		const COrgName& orgname = orgref.GetOrgname();
        const string& lineage = orgname.GetLineage();
		if ( bsrc.GetGenome() == CBioSource::eGenome_kinetoplast ) {
			if ( lineage.find("Kinetoplastida") == string::npos ) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadOrganelle, 
						 "Only Kinetoplastida have kinetoplasts", obj, ctx);
			}
		} 
		if ( bsrc.GetGenome() == CBioSource::eGenome_nucleomorph ) {
			if ( lineage.find("Chlorarachniophyceae") == string::npos  &&
				lineage.find("Cryptophyta") == string::npos ) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadOrganelle, 
                    "Only Chlorarachniophyceae and Cryptophyta have nucleomorphs", obj, ctx);
			}
		}

		if (orgname.IsSetDiv()) {
			const string& div = orgname.GetDiv();
			if (NStr::EqualCase(div, "BCT")
				&& bsrc.GetGenome() != CBioSource::eGenome_unknown
				&& bsrc.GetGenome() != CBioSource::eGenome_genomic
				&& bsrc.GetGenome() != CBioSource::eGenome_plasmid) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
						   "Bacterial source should not have organelle location",
						   obj, ctx);
			} else if (NStr::EqualCase(div, "ENV") && !env_sample) {
                PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					       "BioSource with ENV division is missing environmental sample subsource",
						   obj, ctx);
			}
        }

		if (!metagenomic && NStr::FindNoCase(lineage, "metagenomes") != string::npos) {
			PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
				       "If metagenomes appears in lineage, BioSource should have metagenomic qualifier",
					   obj, ctx);
		}

	}

    // look for conflicts in orgmods
    bool specific_host = false;
	FOR_EACH_ORGMOD_ON_ORGREF (it, orgref) {
        if (!(*it)->IsSetSubtype()) {
            continue;
        }
        COrgMod::TSubtype subtype = (*it)->GetSubtype();

		if (subtype == COrgMod::eSubtype_nat_host) {
			specific_host = true;
        } else if (subtype == COrgMod::eSubtype_strain) {
            if (env_sample) {
                PostObjErr(eDiag_Error, eErr_SEQ_DESCR_BioSourceInconsistency, "Strain should not be present in an environmental sample",
                           obj, ctx);
            }
        } else if (subtype == COrgMod::eSubtype_metagenome_source) {
            if (!metagenomic) {
                PostObjErr(eDiag_Error, eErr_SEQ_DESCR_BioSourceInconsistency, "Metagenome source should also have metagenomic qualifier",
                           obj, ctx);
            }
        }
    }
    if (env_sample && !iso_source && !specific_host) {
		PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
			       "Environmental sample should also have isolation source or specific host annotated",
				   obj, ctx);
	}

	ValidateOrgRef (orgref, obj, ctx);
}


void CValidError_imp::ValidateSubSource
(const CSubSource&    subsrc,
 const CSerialObject& obj,
 const CSeq_entry *ctx)
{
    if (!subsrc.IsSetSubtype()) {
        PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadSubSource,
            "Unknown subsource subtype 0", obj, ctx);
        return;
    }

	int subtype = subsrc.GetSubtype();
    switch ( subtype ) {
        
    case CSubSource::eSubtype_country:
        {
			string countryname = subsrc.GetName();
            bool is_miscapitalized = false;
            if ( CCountries::IsValid(countryname, is_miscapitalized) ) {
                if (is_miscapitalized) {
                    PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadCountryCapitalization, 
                                "Bad country capitalization [" + countryname + "]",
                                obj, ctx);
                }
            } else {
                if ( countryname.empty() ) {
                    countryname = "?";
                }
                if ( CCountries::WasValid(countryname) ) {
                    PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_ReplacedCountryCode,
                            "Replaced country name [" + countryname + "]", obj, ctx);
                } else {
                    PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadCountryCode,
                            "Bad country name [" + countryname + "]", obj, ctx);
                }
            }
        }
        break;

	case CSubSource::eSubtype_lat_lon:
        if (subsrc.IsSetName()) {
			bool format_correct = false, lat_in_range = false, lon_in_range = false;
	        double lat_value = 0.0, lon_value = 0.0;
			string lat_lon = subsrc.GetName();
			s_IsCorrectLatLonFormat (lat_lon, format_correct, 
				                     lat_in_range, lon_in_range,
									 lat_value, lon_value);
			if (!format_correct) {
				size_t pos = NStr::Find(lat_lon, ",");
				if (pos != string::npos) {
					s_IsCorrectLatLonFormat (lat_lon.substr(0, pos - 1), format_correct, lat_in_range, lon_in_range, lat_value, lon_value);
					if (format_correct) {
						PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_LatLonFormat, 
							       "lat_lon format has extra text after correct dd.dd N|S ddd.dd E|W format",
								   obj, ctx);
					}
				}
			}

			if (!format_correct) {
                PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonFormat, 
				            "lat_lon format is incorrect - should be dd.dd N|S ddd.dd E|W",
                            obj, ctx);
			} else {
				if (!lat_in_range) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonRange, 
						        "latitude value is out of range - should be between 90.00 N and 90.00 S",
								obj, ctx);
				}
				if (!lon_in_range) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_LatLonRange, 
						        "longitude value is out of range - should be between 180.00 E and 180.00 W",
								obj, ctx);
				}
			}
		}
		break;
        
    case CSubSource::eSubtype_chromosome:
        break;

	case CSubSource::eSubtype_fwd_primer_name:
		if (subsrc.IsSetName()) {
			string name = subsrc.GetName();
			char bad_ch;
			if (name.length() > 10
				&& s_IsValidPrimerSequence(name, bad_ch)) {
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadPCRPrimerName, 
							"PCR primer name appears to be a sequence",
							obj, ctx);
			}
		}
		break;

	case CSubSource::eSubtype_rev_primer_name:
		if (subsrc.IsSetName()) {
			string name = subsrc.GetName();
			char bad_ch;
			if (name.length() > 10
				&& s_IsValidPrimerSequence(name, bad_ch)) {
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadPCRPrimerName, 
							"PCR primer name appears to be a sequence",
							obj, ctx);
			}
		}
		break;
        
	case CSubSource::eSubtype_fwd_primer_seq:
		{
			char bad_ch;
			if (!subsrc.IsSetName() || !s_IsValidPrimerSequence(subsrc.GetName(), bad_ch)) {
				if (bad_ch < ' ' || bad_ch > '~') {
					bad_ch = '?';
				}

				string msg = "PCR forward primer sequence format is incorrect, first bad character is '";
				msg += bad_ch;
				msg += "'";
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadPCRPrimerSequence,
							msg, obj, ctx);
			}
		}
		break;

	case CSubSource::eSubtype_rev_primer_seq:
		{
			char bad_ch;
			if (!subsrc.IsSetName() || !s_IsValidPrimerSequence(subsrc.GetName(), bad_ch)) {
				if (bad_ch < ' ' || bad_ch > '~') {
					bad_ch = '?';
				}

				string msg = "PCR reverse primer sequence format is incorrect, first bad character is '";
				msg += bad_ch;
				msg += "'";
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadPCRPrimerSequence,
							msg, obj, ctx);
			}
		}
		break;

	case CSubSource::eSubtype_transposon_name:
    case CSubSource::eSubtype_insertion_seq_name:
        PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_ObsoleteSourceQual,
            "Transposon name and insertion sequence name are no "
            "longer legal qualifiers", obj, ctx);
        break;
        
    case 0:
        PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadSubSource,
            "Unknown subsource subtype 0", obj, ctx);
        break;
        
    case CSubSource::eSubtype_other:
        ValidateSourceQualTags(subsrc.GetName(), obj, ctx);
        break;

    case CSubSource::eSubtype_germline:
        break;

    case CSubSource::eSubtype_rearranged:
        break;

    case CSubSource::eSubtype_transgenic:
        break;

    case CSubSource::eSubtype_metagenomic:
        break;

	case CSubSource::eSubtype_environmental_sample:
        break;

	case CSubSource::eSubtype_isolation_source:
		break;

	case CSubSource::eSubtype_sex:
		break;

	case CSubSource::eSubtype_mating_type:
		break;

	case CSubSource::eSubtype_plasmid_name:
		break;

	case CSubSource::eSubtype_plastid_name:
		break;
	
	case CSubSource::eSubtype_cell_line:
		break;

	case CSubSource::eSubtype_cell_type:
		break;

	case CSubSource::eSubtype_tissue_type:
		break;

	case CSubSource::eSubtype_frequency:
		if (subsrc.IsSetName() && !NStr::IsBlank(subsrc.GetName())) {
			const string& frequency = subsrc.GetName();
			if (NStr::Equal(frequency, "0")) {
				//ignore
			} else if (NStr::Equal(frequency, "1")) {
				PostObjErr(eDiag_Info, eErr_SEQ_DESCR_BioSourceInconsistency, 
						   "bad frequency qualifier value " + frequency,
						   obj, ctx);
			} else {
				string::const_iterator sit = frequency.begin();
				bool bad_frequency = false;
				if (*sit == '0') {
					++sit;
				}
				if (sit != frequency.end() && *sit == '.') {
                    ++sit;
					if (sit == frequency.end()) {
						bad_frequency = true;
					}
					while (sit != frequency.end() && isdigit (*sit)) {
						++sit;
					}
					if (sit != frequency.end()) {
						bad_frequency = true;
					}
				} else {
					bad_frequency = true;
				}
				if (bad_frequency) {
					PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
						       "bad frequency qualifier value " + frequency,
							   obj, ctx);
				}
			}
		}
		break;
	case CSubSource::eSubtype_collection_date:
		if (!subsrc.IsSetName()) {
			PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadCollectionDate, 
				        "Collection_date format is not in DD-Mmm-YYYY format",
						obj, ctx);
		} else {
			try {
                CRef<CDate> coll_date = CSubSource::DateFromCollectionDate (subsrc.GetName());

				struct tm *tm;
				time_t t;

				time(&t);
				tm = localtime(&t);

				if (coll_date->GetStd().GetYear() > tm->tm_year + 1900) {
					PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadCollectionDate, 
						       "Collection_date is in the future",
							   obj, ctx);
				} else if (coll_date->GetStd().GetYear() == tm->tm_year + 1900
					       && coll_date->GetStd().IsSetMonth()) {
					if (coll_date->GetStd().GetMonth() > tm->tm_mon + 1) {
						PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadCollectionDate, 
								   "Collection_date is in the future",
								   obj, ctx);
					} else if (coll_date->GetStd().GetMonth() == tm->tm_mon + 1
						       && coll_date->GetStd().IsSetDay()) {
					    if (coll_date->GetStd().GetDay() > tm->tm_mday) {
							PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadCollectionDate, 
									   "Collection_date is in the future",
									   obj, ctx);
						}
					}
				}
			} catch (CException ) {
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadCollectionDate, 
							"Collection_date format is not in DD-Mmm-YYYY format",
							obj, ctx);
			}
		} 
		break;

    default:
        break;
    }

	if (subsrc.IsSetName()) {
		if (CSubSource::NeedsNoText(subtype)) {
			if (subsrc.IsSetName() && !NStr::IsBlank(subsrc.GetName())) {
				string subname = CSubSource::GetSubtypeName (subtype);
				if (subname.length() > 0) {
					subname[0] = toupper(subname[0]);
				}
                NStr::ReplaceInPlace(subname, "-", "_");
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					        subname + " qualifier should not have descriptive text",
							obj, ctx);
			}
		} else {
			const string& subname = subsrc.GetName();
			if (s_UnbalancedParentheses(subname)) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_UnbalancedParentheses,
						   "Unbalanced parentheses in subsource '" + subname + "'",
						   obj, ctx);
			}
			if (ContainsSgml(subname)) {
				PostObjErr(eDiag_Warning, eErr_GENERIC_SgmlPresentInText, 
						   "subsource " + subname + " has SGML", 
						   obj, ctx);
			}
		}
	}
}


static bool s_FindWholeName (string taxname, string value)
{
	if (NStr::IsBlank(taxname) || NStr::IsBlank(value)) {
		return false;
	}
	size_t pos = NStr::Find (taxname, value);
	size_t value_len = value.length();
	while (pos != string::npos 
		   && ((pos != 0 && isalpha (taxname.c_str()[pos - 1])
		       || isalpha (taxname.c_str()[pos + value_len])))) {
	    pos = NStr::Find(taxname, value, pos + value_len);
	}
	if (pos == string::npos) {
		return false;
	} else {
		return true;
	}
}


void CValidError_imp::ValidateOrgRef
(const COrg_ref&    orgref,
 const CSerialObject& obj,
 const CSeq_entry *ctx)
{
	// Organism must have a name.
	if ( (!orgref.IsSetTaxname()  ||  orgref.GetTaxname().empty())  &&
         (!orgref.IsSetCommon()   ||  orgref.GetCommon().empty()) ) {
		  PostObjErr(eDiag_Error, eErr_SEQ_DESCR_NoOrgFound,
            "No organism name has been applied to this Bioseq.  Other qualifiers may exist.", obj, ctx);
	}

    if (orgref.IsSetTaxname()) {
        string taxname = orgref.GetTaxname();
        if (s_UnbalancedParentheses (taxname)) {
            PostObjErr(eDiag_Error, eErr_SEQ_DESCR_UnbalancedParentheses,
                       "Unbalanced parentheses in taxname '" + orgref.GetTaxname() + "'", obj, ctx);
        }
        if (ContainsSgml(taxname)) {
			PostObjErr(eDiag_Warning, eErr_GENERIC_SgmlPresentInText, 
					   "taxname " + taxname + " has SGML", 
					   obj, ctx);
        }
    }

    if ( orgref.IsSetDb() ) {
        ValidateDbxref(orgref.GetDb(), obj, true, ctx);
    }

    if ( IsRequireTaxonID() ) {
        bool found = false;
        FOR_EACH_DBXREF_ON_ORGREF (dbt, orgref) {
            if ( NStr::CompareNocase((*dbt)->GetDb(), "taxon") == 0 ) {
                found = true;
                break;
            }
        }
        if ( !found ) {
            PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_NoTaxonID,
                "BioSource is missing taxon ID", obj, ctx);
        }
    }

    if ( !orgref.IsSetOrgname() ) {
        return;
    }
    const COrgName& orgname = orgref.GetOrgname();
    ValidateOrgName(orgname, obj, ctx);

	// Look for modifiers in taxname
	string taxname_search = "";
	if (orgref.IsSetTaxname()) {
		taxname_search = orgref.GetTaxname();
	}
	// skip first two words
	size_t pos = NStr::Find (taxname_search, " ");
	if (pos == string::npos) {
		taxname_search = "";
	} else {
		taxname_search = taxname_search.substr(pos + 1);
		NStr::TruncateSpacesInPlace(taxname_search);
		pos = NStr::Find (taxname_search, " ");
		if (pos == string::npos) {
			taxname_search = "";
		} else {
			taxname_search = taxname_search.substr(pos + 1);
			NStr::TruncateSpacesInPlace(taxname_search);
		}
	}

	// first, determine if variety is present and in taxname - if so,
	// can ignore missing subspecies
	int num_bad_subspecies = 0;
	bool have_variety_in_taxname = false;
    FOR_EACH_ORGMOD_ON_ORGNAME (it, orgname) {
		if (!(*it)->IsSetSubtype() || !(*it)->IsSetSubname()) {
			continue;
		}
		int subtype = (*it)->GetSubtype();
		const string& subname = (*it)->GetSubname();
		string orgmod_name = COrgMod::GetSubtypeName(subtype);
		if (orgmod_name.length() > 0) {
			orgmod_name[0] = toupper(orgmod_name[0]);
		}
		NStr::ReplaceInPlace(orgmod_name, "-", " ");
		if (subtype == COrgMod::eSubtype_sub_species) {
			if (!s_FindWholeName(taxname_search, subname)) {
			    ++num_bad_subspecies;
			}
		} else if (subtype == COrgMod::eSubtype_variety) {
			if (s_FindWholeName(taxname_search, subname)) {
                have_variety_in_taxname = true;
			} else {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					       orgmod_name + " value specified is not found in taxname",
                           obj, ctx);
			}
		} else if (subtype == COrgMod::eSubtype_forma
			       || subtype == COrgMod::eSubtype_forma_specialis) {
			if (!s_FindWholeName(taxname_search, subname)) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
					       orgmod_name + " value specified is not found in taxname",
                           obj, ctx);
			}
		}
	}
	if (!have_variety_in_taxname) {
		for (int i = 0; i < num_bad_subspecies; i++) {
			PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency,
				       "Subspecies value specified is not found in taxname",
                       obj, ctx);
		}
	}

}


void CValidError_imp::ValidateOrgName
(const COrgName&    orgname,
 const CSerialObject& obj,
 const CSeq_entry *ctx)
{
    if ( orgname.IsSetMod() ) {
        FOR_EACH_ORGMOD_ON_ORGNAME (omd_itr, orgname) {
            const COrgMod& omd = **omd_itr;
            int subtype = omd.GetSubtype();
            
			switch (subtype) {
				case 0:
				case 1:
                    PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadOrgMod, 
                               "Unknown orgmod subtype " + NStr::IntToString(subtype), obj, ctx);
					break;
				case COrgMod::eSubtype_variety:
					if ( (!orgname.IsSetDiv() || !NStr::EqualNocase( orgname.GetDiv(), "PLN" ))
						&& (!orgname.IsSetLineage() ||
						    (NStr::Find(orgname.GetLineage(), "Cyanobacteria") == string::npos
							&& NStr::Find(orgname.GetLineage(), "Myxogastria") == string::npos
							&& NStr::Find(orgname.GetLineage(), "Oomycetes") == string::npos))) {
						PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BadOrgMod, 
							"Orgmod variety should only be in plants, fungi, or cyanobacteria", 
							obj, ctx);
					}
					break;
				case COrgMod::eSubtype_other:
					ValidateSourceQualTags( omd.GetSubname(), obj, ctx);
					break;
				case COrgMod::eSubtype_synonym:
					if ((*omd_itr)->IsSetSubname() && !NStr::IsBlank((*omd_itr)->GetSubname())) {
						const string& val = (*omd_itr)->GetSubname();

						// look for synonym/gb_synonym duplication
						FOR_EACH_ORGMOD_ON_ORGNAME (it2, orgname) {
							if ((*it2)->IsSetSubtype() 
								&& (*it2)->GetSubtype() == COrgMod::eSubtype_gb_synonym
								&& (*it2)->IsSetSubname()
								&& NStr::EqualNocase(val, (*it2)->GetSubname())) {
                                PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
									       "OrgMod synonym is identical to OrgMod gb_synonym",
										   obj, ctx);
							}
						}
					}
                    break;
				case COrgMod::eSubtype_bio_material:
				case COrgMod::eSubtype_culture_collection:
				case COrgMod::eSubtype_specimen_voucher:
					ValidateOrgModVoucher (omd, obj, ctx);
					break;

				default:
					break;
            }
			if ( omd.IsSetSubname()) {
				const string& subname = omd.GetSubname();

				if (s_UnbalancedParentheses(subname)) {
					PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_UnbalancedParentheses,
							   "Unbalanced parentheses in orgmod '" + subname + "'",
							   obj, ctx);
				}
				if (ContainsSgml(subname)) {
					PostObjErr(eDiag_Warning, eErr_GENERIC_SgmlPresentInText, 
							   "orgmod " + subname + " has SGML", 
							   obj, ctx);
				}
			}
        }

		// look for multiple source vouchers
        FOR_EACH_ORGMOD_ON_ORGNAME (omd_itr, orgname) {
			if(!(*omd_itr)->IsSetSubtype() || !(*omd_itr)->IsSetSubname()) {
				continue;
			}
            const COrgMod& omd = **omd_itr;
            int subtype = omd.GetSubtype();

			if (subtype != COrgMod::eSubtype_specimen_voucher
				&& subtype != COrgMod::eSubtype_bio_material
				&& subtype != COrgMod::eSubtype_culture_collection) {
				continue;
			}

			string inst1 = "", coll1 = "", id1 = "";
			if (!COrgMod::ParseStructuredVoucher(omd.GetSubname(), inst1, coll1, id1)) {
			    continue;
		    }
            if (NStr::EqualNocase(inst1, "personal")
				|| NStr::EqualCase(coll1, "DNA")) {
				continue;
			}

			COrgName::TMod::const_iterator it_next = omd_itr;
			++it_next;
			while (it_next != orgname.GetMod().end()) {
				if (!(*it_next)->IsSetSubtype() || !(*it_next)->IsSetSubname() || (*it_next)->GetSubtype() != subtype) {
					++it_next;
					continue;
				}
				int subtype_next = (*it_next)->GetSubtype();
				if (subtype_next != COrgMod::eSubtype_specimen_voucher
					&& subtype_next != COrgMod::eSubtype_bio_material
					&& subtype_next != COrgMod::eSubtype_culture_collection) {
					++it_next;
					continue;
				}
			    string inst2 = "", coll2 = "", id2 = "";
				if (!COrgMod::ParseStructuredVoucher((*it_next)->GetSubname(), inst2, coll2, id2)
					|| NStr::IsBlank(inst2)
					|| NStr::EqualNocase(inst2, "personal")
					|| NStr::EqualCase(coll2, "DNA")
					|| !NStr::EqualNocase (inst1, inst2)) {
					++it_next;
					continue;
				}
				// at this point, we have identified two vouchers 
				// with the same institution codes
				// that are not personal and not DNA collections
				if (!NStr::IsBlank(coll1) && !NStr::IsBlank(coll2) 
					&& NStr::EqualNocase(coll1, coll2)) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceVouchers, 
						        "Multiple vouchers with same institution:collection",
								obj, ctx);
				} else {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_MultipleSourceVouchers, 
						        "Multiple vouchers with same institution",
								obj, ctx);
				}
                ++it_next;
			}

		}

    }
}


bool CValidError_imp::IsOtherDNA(const CBioseq_Handle& bsh) const
{
    if ( bsh ) {
        CSeqdesc_CI sd(bsh, CSeqdesc::e_Molinfo);
        if ( sd ) {
            const CSeqdesc::TMolinfo& molinfo = sd->GetMolinfo();
            if ( molinfo.CanGetBiomol()  &&
                 molinfo.GetBiomol() == CMolInfo::eBiomol_other ) {
                return true;
            }
        }
    }
    return false;
}


void CValidError_imp::ValidateBioSourceForSeq
(const CBioSource&    source,
 const CSerialObject& obj,
 const CSeq_entry    *ctx,
 const CBioseq_Handle& bsh)
{
    if ( source.IsSetIs_focus() ) {
        // skip proteins, segmented bioseqs, or segmented parts
        if ( !bsh.IsAa()  &&
            !(bsh.GetInst().GetRepr() == CSeq_inst::eRepr_seg)  &&
            !(GetAncestor(*(bsh.GetCompleteBioseq()), CBioseq_set::eClass_parts) != 0) ) {
            if ( !CFeat_CI(bsh, CSeqFeatData::e_Biosrc) ) {
                PostObjErr(eDiag_Error,
                    eErr_SEQ_DESCR_UnnecessaryBioSourceFocus,
                    "BioSource descriptor has focus, "
                    "but no BioSource feature", obj, ctx);
            }
        }
    }
    if ( source.CanGetOrigin()  &&  
         source.GetOrigin() == CBioSource::eOrigin_synthetic ) {
        if ( !IsOtherDNA(bsh) ) {
            PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_InvalidForType,
                "Molinfo-biomol other should be used if "
                "Biosource-location is synthetic", obj, ctx);
        }
    }

	// check locations for HIV biosource
	if (bsh.IsSetInst() && bsh.GetInst().IsSetMol()
		&& source.IsSetOrg() && source.GetOrg().IsSetTaxname() 
		&& (NStr::EqualNocase(source.GetOrg().GetTaxname(), "Human immunodeficiency virus")
			|| NStr::EqualNocase(source.GetOrg().GetTaxname(), "Human immunodeficiency virus 1")
			|| NStr::EqualNocase(source.GetOrg().GetTaxname(), "Human immunodeficiency virus 2"))) {

		if (bsh.GetInst().GetMol() == CSeq_inst::eMol_dna) {
			if (!source.IsSetGenome() || source.GetGenome() != CBioSource::eGenome_proviral) {
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					     "HIV with moltype DNA should be proviral", 
                         obj, ctx);
			}
		} else if (bsh.GetInst().GetMol() == CSeq_inst::eMol_rna) {
			bool location_ok = false;
			if (!source.IsSetGenome() || source.GetGenome() == CBioSource::eGenome_unknown) {
                location_ok = true;
			} else if (source.GetGenome() == CBioSource::eGenome_genomic) {
				CSeqdesc_CI mi(bsh, CSeqdesc::e_Molinfo);
				if (mi && mi->GetMolinfo().IsSetBiomol()
					&& mi->GetMolinfo().GetBiomol() == CMolInfo::eBiomol_genomic) {
					location_ok = true;
				}
			}
			if (!location_ok) {
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					     "HIV with moltype RNA should have source location unset or set to genomic (on genomic RNA sequence)", 
                         obj, ctx);
			}
		}
	}

    CSeqdesc_CI mi(bsh, CSeqdesc::e_Molinfo);

    // validate subsources in context

	FOR_EACH_SUBSOURCE_ON_BIOSOURCE (it, source) {
        if (!(*it)->IsSetSubtype()) {
            continue;
        }
        CSubSource::TSubtype subtype = (*it)->GetSubtype();

        switch (subtype) {
            case CSubSource::eSubtype_other:
                // look for conflicting cRNA notes on subsources
                if (mi && (*it)->IsSetName() && NStr::EqualNocase((*it)->GetName(), "cRNA")) {
                    const CMolInfo& molinfo = mi->GetMolinfo();
		            if (!molinfo.IsSetBiomol() 
			            || molinfo.GetBiomol() != CMolInfo::eBiomol_cRNA) {
			            PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					               "cRNA note conflicts with molecule type",
					               obj, ctx);
		            } else {
			            PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					               "cRNA note redundant with molecule type",
					               obj, ctx);
		            }
	            }
                break;
            default:
                break;
        }
	}


	// look at orgref in context
	if (source.IsSetOrg()) {
		const COrg_ref& orgref = source.GetOrg();

		// look at uncultured sequence length and required modifiers
		if (orgref.IsSetTaxname()) {
			if (NStr::EqualNocase(orgref.GetTaxname(), "uncultured bacterium")
				&& bsh.GetBioseqLength() >= 10000) {
				PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
					       "Uncultured bacterium sequence length is suspiciously high",
						   obj, ctx);
			}
			if (NStr::StartsWith(orgref.GetTaxname(), "uncultured ", NStr::eNocase)) {
				bool is_env_sample = false;
				FOR_EACH_SUBSOURCE_ON_BIOSOURCE (it, source) {
					if ((*it)->IsSetSubtype() && (*it)->GetSubtype() == CSubSource::eSubtype_environmental_sample) {
                        is_env_sample = true;
						break;
					}
				}
				if (!is_env_sample) {
					PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
						       "Uncultured should also have /environmental_sample",
							   obj, ctx);
				}
			}
		}

		if (mi) {						
            const CMolInfo& molinfo = mi->GetMolinfo();
			// look for conflicting cRNA notes on orgmod
			FOR_EACH_ORGMOD_ON_ORGREF (it, orgref) {
				if ((*it)->IsSetSubtype() 
					&& (*it)->GetSubtype() == COrgMod::eSubtype_other
					&& (*it)->IsSetSubname()
					&& NStr::EqualNocase((*it)->GetSubname(), "cRNA")) {
					if (!molinfo.IsSetBiomol() 
						|| molinfo.GetBiomol() != CMolInfo::eBiomol_cRNA) {
						PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
								   "cRNA note conflicts with molecule type",
								   obj, ctx);
					} else {
						PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
								   "cRNA note redundant with molecule type",
								   obj, ctx);
					}
				}
			}


			if (orgref.IsSetLineage()) {
				const string& lineage = orgref.GetOrgname().GetLineage();

				// look for incorrect DNA stage
				if (molinfo.IsSetBiomol()  && molinfo.GetBiomol () == CMolInfo::eBiomol_genomic
					&& bsh.IsSetInst() && bsh.GetInst().IsSetMol() && bsh.GetInst().GetMol() == CSeq_inst::eMol_dna
					&& NStr::StartsWith(lineage, "Viruses; ")
					&& NStr::FindNoCase(lineage, "no DNA stage") != string::npos) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
							 "Genomic DNA viral lineage indicates no DNA stage",
							 obj, ctx);
				}
			    if (NStr::FindNoCase (lineage, "negative-strand viruses") != string::npos) {
					bool is_ambisense = false;
					if (NStr::FindNoCase(lineage, "Arenavirus") != string::npos
						|| NStr::FindNoCase(lineage, "Phlebovirus") != string::npos
						|| NStr::FindNoCase(lineage, "Tospovirus") != string::npos
						|| NStr::FindNoCase(lineage, "Tenuivirus") != string::npos) {
						is_ambisense = true;
					}

					bool is_synthetic = false;
					if (orgref.IsSetDivision() && NStr::EqualNocase(orgref.GetDivision(), "SYN")) {
						is_synthetic = true;
					} else if (source.IsSetOrigin()
						       && (source.GetOrigin() == CBioSource::eOrigin_mut
							       || source.GetOrigin() == CBioSource::eOrigin_artificial
								   || source.GetOrigin() == CBioSource::eOrigin_synthetic)) {
					    is_synthetic = true;
					}

					bool has_cds = false;

					CFeat_CI cds_ci(bsh, SAnnotSelector(CSeqFeatData::e_Cdregion));
					while (cds_ci) {
						has_cds = true;
						if (cds_ci->GetLocation().GetStrand() == eNa_strand_minus) {
							if (!molinfo.IsSetBiomol() || molinfo.GetBiomol() != CMolInfo::eBiomol_genomic) {
								PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
									        "Negative-strand virus with minus strand CDS should be genomic",
									        obj, ctx);
							}
						} else {
                            if (!is_synthetic && !is_ambisense
								&& (!molinfo.IsSetBiomol() 
								    || (molinfo.GetBiomol() != CMolInfo::eBiomol_cRNA
									    && molinfo.GetBiomol() != CMolInfo::eBiomol_mRNA))) {
								PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
									        "Negative-strand virus with plus strand CDS should be mRNA or cRNA",
											obj, ctx);
							}
						}
						++cds_ci;
					}
					if (!has_cds) {
						CFeat_CI misc_ci(bsh, SAnnotSelector(CSeqFeatData::eSubtype_misc_feature));
						while (misc_ci) {
							if (misc_ci->IsSetComment()
								&& NStr::FindNoCase (misc_ci->GetComment(), "nonfunctional") != string::npos) {
							    if (misc_ci->GetLocation().GetStrand() == eNa_strand_minus) {
									if (!molinfo.IsSetBiomol() || molinfo.GetBiomol() != CMolInfo::eBiomol_genomic) {
										PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
											        "Negative-strand virus with nonfunctional minus strand misc_feature should be mRNA or cRNA",
													obj, ctx);
									}
								} else {
									if (!is_synthetic && !is_ambisense
										&& (!molinfo.IsSetBiomol() 
											|| (molinfo.GetBiomol() != CMolInfo::eBiomol_cRNA
												&& molinfo.GetBiomol() != CMolInfo::eBiomol_mRNA))) {
										PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BioSourceInconsistency, 
											        "Negative-strand virus with nonfunctional plus strand misc_feature should be mRNA or cRNA",
													obj, ctx);
									}
								}
							}
							++misc_ci;
						}
					}
				}
			}
	    }
	}

	

}


bool CValidError_imp::IsTransgenic(const CBioSource& bsrc)
{
    if (bsrc.CanGetSubtype()) {
        FOR_EACH_SUBSOURCE_ON_BIOSOURCE (sbs_itr, bsrc) {
            const CSubSource& sbs = **sbs_itr;
            if (sbs.GetSubtype() == CSubSource::eSubtype_transgenic) {
                return true;
            }
        }
    }
    return false;
}


const string CValidError_imp::sm_SourceQualPrefixes[] = {
    "acronym:",
    "anamorph:",
    "authority:",
    "biotype:",
    "biovar:",
    "bio_material:",
    "breed:",
    "cell_line:",
    "cell_type:",
    "chemovar:",
    "chromosome:",
    "clone:",
    "clone_lib:",
    "collected_by:",
    "collection_date:",
    "common:",
    "country:",
    "cultivar:",
    "culture_collection:",
    "dev_stage:",
    "dosage:",
    "ecotype:",
    "endogenous_virus_name:",
    "environmental_sample:",
    "forma:",
    "forma_specialis:",
    "frequency:",
    "fwd_pcr_primer_name",
    "fwd_pcr_primer_seq",
    "fwd_primer_name",
    "fwd_primer_seq",
    "genotype:",
    "germline:",
    "group:",
    "haplotype:",
    "identified_by:",
    "insertion_seq_name:",
    "isolate:",
    "isolation_source:",
    "lab_host:",
    "lat_lon:"
    "left_primer:",
    "map:",
    "metagenome_source:",
    "metagenomic:",
    "nat_host:",
    "pathovar:",
    "plasmid_name:",
    "plastid_name:",
    "pop_variant:",
    "rearranged:",
    "rev_pcr_primer_name",
    "rev_pcr_primer_seq",
    "rev_primer_name",
    "rev_primer_seq",
    "right_primer:",
    "segment:",
    "serogroup:",
    "serotype:",
    "serovar:",
    "sex:",
    "specimen_voucher:",
    "strain:",
    "subclone:",
    "subgroup:",
    "substrain:",
    "subtype:",
    "sub_species:",
    "synonym:",
    "taxon:",
    "teleomorph:",
    "tissue_lib:",
    "tissue_type:",
    "transgenic:",
    "transposon_name:",
    "type:",
    "variety:",
};


void CValidError_imp::InitializeSourceQualTags() 
{
    m_SourceQualTags.reset(new CTextFsa);
    size_t size = sizeof(sm_SourceQualPrefixes) / sizeof(string);

    for (size_t i = 0; i < size; ++i ) {
        m_SourceQualTags->AddWord(sm_SourceQualPrefixes[i]);
    }

    m_SourceQualTags->Prime();
}


void CValidError_imp::ValidateSourceQualTags
(const string& str,
 const CSerialObject& obj,
 const CSeq_entry *ctx)
{
    if ( NStr::IsBlank(str) ) return;

    size_t str_len = str.length();

    int state = m_SourceQualTags->GetInitialState();
    
    for ( size_t i = 0; i < str_len; ++i ) {
        state = m_SourceQualTags->GetNextState(state, str[i]);
        if ( m_SourceQualTags->IsMatchFound(state) ) {
            string match = m_SourceQualTags->GetMatches(state)[0];
            if ( match.empty() ) {
                match = "?";
            }
            size_t match_len = match.length();

            bool okay = true;
            if ( (int)(i - match_len) >= 0 ) {
                char ch = str[i - match_len];
                if ( !isspace((unsigned char) ch) || ch != ';' ) {
                    okay = false;
                    // look to see if there's a longer match in the list
                    for (size_t k = 0; 
                         k < sizeof (CValidError_imp::sm_SourceQualPrefixes) / sizeof (string); 
                         k++) {
                         size_t pos = NStr::FindNoCase (str, CValidError_imp::sm_SourceQualPrefixes[k]);
                         if (pos != string::npos) {
                             if (pos == 0 || isspace ((unsigned char) str[pos]) || str[pos] == ';') {
                                 okay = true;
                                 match = CValidError_imp::sm_SourceQualPrefixes[k];
                                 break;
                             }
                         }
                    }
                }
            }
            if ( okay ) {
                PostObjErr(eDiag_Warning, eErr_SEQ_DESCR_StructuredSourceNote,
                    "Source note has structured tag '" + match + "'", obj, ctx);
            }
        }
    }
}


void CValidError_imp::GatherSources
(const CSeq_entry& se, 
 vector<CConstRef<CSeqdesc> >& src_descs,
 vector<CConstRef<CSeq_entry> >& desc_ctxs,
 vector<CConstRef<CSeq_feat> >& src_feats)
{
	// get source descriptors
	FOR_EACH_DESCRIPTOR_ON_SEQENTRY (it, se) {
		if ((*it)->IsSource() && (*it)->GetSource().IsSetOrg()) {
			CConstRef<CSeqdesc> desc;
			desc.Reset(*it);
			src_descs.push_back(desc);
            CConstRef<CSeq_entry> r_se;
			r_se.Reset(&se);
			desc_ctxs.push_back(r_se);
		}
	}
	// also get features
	FOR_EACH_ANNOT_ON_SEQENTRY (annot_it, se) {
		FOR_EACH_SEQFEAT_ON_SEQANNOT (feat_it, **annot_it) {
			if ((*feat_it)->IsSetData() && (*feat_it)->GetData().IsBiosrc()
				&& (*feat_it)->GetData().GetBiosrc().IsSetOrg()) {
				CConstRef<CSeq_feat> feat;
				feat.Reset(*feat_it);
				src_feats.push_back(feat);
			}
		}
	}

	// if set, recurse
	if (se.IsSet()) {
		FOR_EACH_SEQENTRY_ON_SEQSET (it, se.GetSet()) {
			GatherSources (**it, src_descs, desc_ctxs, src_feats);
		}
	}
}



static bool s_HasMisSpellFlag (const CT3Data& data)
{
    bool has_misspell_flag = false;

	if (data.IsSetStatus()) {
		ITERATE (CT3Reply::TData::TStatus, status_it, data.GetStatus()) {
			if ((*status_it)->IsSetProperty()) {
				string prop = (*status_it)->GetProperty();
				if (NStr::EqualNocase(prop, "misspelled_name")) {
					has_misspell_flag = true;
					break;
				}
			}
		}
	}
    return has_misspell_flag;
}


static string s_FindMatchInOrgRef (string str, const COrg_ref& org)
{
	string match = "";

	if (NStr::IsBlank(str)) {
		// do nothing;
	} else if (org.IsSetTaxname() && NStr::EqualNocase(str, org.GetTaxname())) {
		match = org.GetTaxname();
	} else if (org.IsSetCommon() && NStr::EqualNocase(str, org.GetCommon())) {
		match = org.GetCommon();
	} else {
		FOR_EACH_SYN_ON_ORGREF (syn_it, org) {
			if (NStr::EqualNocase(str, *syn_it)) {
				match = *syn_it;
				break;
			}
		}
		if (NStr::IsBlank(match)) {
			FOR_EACH_ORGMOD_ON_ORGREF (mod_it, org) {
				if ((*mod_it)->IsSetSubtype()
					&& ((*mod_it)->GetSubtype() == COrgMod::eSubtype_gb_synonym
					    || (*mod_it)->GetSubtype() == COrgMod::eSubtype_old_name)
					&& (*mod_it)->IsSetSubname()
					&& NStr::EqualNocase(str, (*mod_it)->GetSubname())) {
					match = (*mod_it)->GetSubname();
					break;
				}
			}
		}
	}
	return match;
}


void CValidError_imp::ValidateSpecificHost 
(const vector<CConstRef<CSeqdesc> > & src_descs,
 const vector<CConstRef<CSeq_entry> > & desc_ctxs,
 const vector<CConstRef<CSeq_feat> > & src_feats)
{
    vector<CConstRef<CSeqdesc> > local_src_descs;
    vector<CConstRef<CSeq_entry> > local_desc_ctxs;
    vector<CConstRef<CSeq_feat> > local_src_feats;

    vector< CRef<COrg_ref> > org_rq_list;

    // first do descriptors
    vector<CConstRef<CSeqdesc> >::const_iterator desc_it = src_descs.begin();
    vector<CConstRef<CSeq_entry> >::const_iterator ctx_it = desc_ctxs.begin();
    while (desc_it != src_descs.end() && ctx_it != desc_ctxs.end()) {
        FOR_EACH_ORGMOD_ON_BIOSOURCE (mod_it, (*desc_it)->GetSource()) {
            if ((*mod_it)->IsSetSubtype()
                && (*mod_it)->GetSubtype() == COrgMod::eSubtype_nat_host
                && (*mod_it)->IsSetSubname()
                && isupper ((*mod_it)->GetSubname().c_str()[0])) {
               	string host = (*mod_it)->GetSubname();
                size_t pos = NStr::Find(host, " ");
                if (pos != string::npos) {
                    if (! NStr::StartsWith(host.substr(pos + 1), "sp.")
                        && ! NStr::StartsWith(host.substr(pos + 1), "(")) {
                        pos = NStr::Find(host, " ", pos + 1);
                        if (pos != string::npos) {
                            host = host.substr(0, pos);
                        }
                    } else {
                        host = host.substr(0, pos);
                    }
                }

                CRef<COrg_ref> rq(new COrg_ref);
            	rq->SetTaxname(host);
                org_rq_list.push_back(rq);
            	local_src_descs.push_back(*desc_it);
            	local_desc_ctxs.push_back(*ctx_it);
            }
        }

        ++desc_it;
        ++ctx_it;
    }

    // collect features with specific hosts
    vector<CConstRef<CSeq_feat> >::const_iterator feat_it = src_feats.begin();
    while (feat_it != src_feats.end()) {
        FOR_EACH_ORGMOD_ON_BIOSOURCE (mod_it, (*feat_it)->GetData().GetBiosrc()) {
            if ((*mod_it)->IsSetSubtype()
				&& (*mod_it)->GetSubtype() == COrgMod::eSubtype_nat_host
				&& (*mod_it)->IsSetSubname()
				&& isupper ((*mod_it)->GetSubname().c_str()[0])) {
				string host = (*mod_it)->GetSubname();
				size_t pos = NStr::Find(host, " ");
				if (pos != string::npos) {
					if (! NStr::StartsWith(host.substr(pos + 1), "sp.")
					    && ! NStr::StartsWith(host.substr(pos + 1), "(")) {
					    pos = NStr::Find(host, " ", pos + 1);
					    if (pos != string::npos) {
							host = host.substr(0, pos);
						}
					} else {
						host = host.substr(0, pos);
					}
				}

				CRef<COrg_ref> rq(new COrg_ref);
				rq->SetTaxname(host);
				org_rq_list.push_back(rq);
				local_src_feats.push_back(*feat_it);
			}
		}

		++feat_it;
	}


	CTaxon3 taxon3;
	taxon3.Init();
	CRef<CTaxon3_reply> reply = taxon3.SendOrgRefList(org_rq_list);
	if (reply) {
		CTaxon3_reply::TReply::const_iterator reply_it = reply->GetReply().begin();
		vector< CRef<COrg_ref> >::iterator rq_it = org_rq_list.begin();
		// process descriptor responses
		desc_it = local_src_descs.begin();
		ctx_it = local_desc_ctxs.begin();

		while (reply_it != reply->GetReply().end()
			   && rq_it != org_rq_list.end()
			   && desc_it != local_src_descs.end()
			   && ctx_it != local_desc_ctxs.end()) {
		    
		    string host = (*rq_it)->GetTaxname();
		    if ((*reply_it)->IsError()) {
				string err_str = "?";
				if ((*reply_it)->GetError().IsSetMessage()) {
					err_str = (*reply_it)->GetError().GetMessage();
				}
				if(NStr::Find(err_str, "ambiguous") != string::npos) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost,
								"Specific host value is ambiguous: " + host,
								**desc_it, *ctx_it);
				} else {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Invalid value for specific host: " + host,
								**desc_it, *ctx_it);
				}
			} else if ((*reply_it)->IsData()) {
				if (s_HasMisSpellFlag((*reply_it)->GetData())) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Specific host value is misspelled: " + host,
								**desc_it, *ctx_it);
				} else if ((*reply_it)->GetData().IsSetOrg()) {
                    string match = s_FindMatchInOrgRef (host, (*reply_it)->GetData().GetOrg());
					if (!NStr::EqualCase(match, host)) {
						PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
									"Specific host value is incorrectly capitalized:  " + host,
									**desc_it, *ctx_it);
					}
				} else {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Invalid value for specific host: " + host,
								**desc_it, *ctx_it);
				}
			}
			++reply_it;
			++rq_it;
			++desc_it;
			++ctx_it;
		}

		// TO DO - process feature responses
        feat_it = local_src_feats.begin(); 
		while (reply_it != reply->GetReply().end()
			   && rq_it != org_rq_list.end()
			   && feat_it != local_src_feats.end()) {
		    string host = (*rq_it)->GetTaxname();
		    if ((*reply_it)->IsError()) {
				string err_str = "?";
				if ((*reply_it)->GetError().IsSetMessage()) {
					err_str = (*reply_it)->GetError().GetMessage();
				}
				if(NStr::Find(err_str, "ambiguous") != string::npos) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost,
								"Specific host value is ambiguous: " + host,
								**feat_it);
				} else {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Invalid value for specific host: " + host,
								**feat_it);
				}
			} else if ((*reply_it)->IsData()) {
				if (s_HasMisSpellFlag((*reply_it)->GetData())) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Specific host value is misspelled: " + host,
								**feat_it);
				} else if ((*reply_it)->GetData().IsSetOrg()) {
                    string match = s_FindMatchInOrgRef (host, (*reply_it)->GetData().GetOrg());
					if (!NStr::EqualCase(match, host)) {
						PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
									"Specific host value is incorrectly capitalized:  " + host,
									**feat_it);
					}
				} else {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Invalid value for specific host: " + host,
								**feat_it);
                }
            }
            ++reply_it;
            ++rq_it;
            ++feat_it;
        }
    }
}


void CValidError_imp::ValidateTaxonomy(const CSeq_entry& se)
{
    vector<CConstRef<CSeqdesc> > src_descs;
	vector<CConstRef<CSeq_entry> > desc_ctxs;
	vector<CConstRef<CSeq_feat> > src_feats;

	GatherSources (se, src_descs, desc_ctxs, src_feats);

	// request list for taxon3
	vector< CRef<COrg_ref> > org_rq_list;

	// first do descriptors
	vector<CConstRef<CSeqdesc> >::iterator desc_it = src_descs.begin();
	vector<CConstRef<CSeq_entry> >::iterator ctx_it = desc_ctxs.begin();
	while (desc_it != src_descs.end() && ctx_it != desc_ctxs.end()) {
		CRef<COrg_ref> rq(new COrg_ref);
		const COrg_ref& org = (*desc_it)->GetSource().GetOrg();
		rq->Assign(org);
		org_rq_list.push_back(rq);

		++desc_it;
		++ctx_it;
	}

	// now do features
	vector<CConstRef<CSeq_feat> >::iterator feat_it = src_feats.begin();
	while (feat_it != src_feats.end()) {
		CRef<COrg_ref> rq(new COrg_ref);
		const COrg_ref& org = (*feat_it)->GetData().GetBiosrc().GetOrg();
		rq->Assign(org);
		org_rq_list.push_back(rq);

		++feat_it;
	}

	CTaxon3 taxon3;
	taxon3.Init();
	CRef<CTaxon3_reply> reply = taxon3.SendOrgRefList(org_rq_list);
	if (reply) {
		CTaxon3_reply::TReply::const_iterator reply_it = reply->GetReply().begin();

		// process descriptor responses
		desc_it = src_descs.begin();
		ctx_it = desc_ctxs.begin();

		while (reply_it != reply->GetReply().end()
			   && desc_it != src_descs.end()
			   && ctx_it != desc_ctxs.end()) {
		    if ((*reply_it)->IsError()) {
				string err_str = "?";
				if ((*reply_it)->GetError().IsSetMessage()) {
					err_str = (*reply_it)->GetError().GetMessage();
				}
				PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup failed with message '" + err_str + "'",
							**desc_it, *ctx_it);
			} else if ((*reply_it)->IsData()) {
				bool is_species_level = true;
				bool force_consult = false;
				bool has_nucleomorphs = false;
				(*reply_it)->GetData().GetTaxFlags(is_species_level, force_consult, has_nucleomorphs);
				if (!is_species_level) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup reports is_species_level FALSE",
							**desc_it, *ctx_it);
				}
				if (force_consult) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup reports taxonomy consultation needed",
							**desc_it, *ctx_it);
				}
                if ((*desc_it)->GetSource().IsSetGenome()
					&& (*desc_it)->GetSource().GetGenome() == CBioSource::eGenome_nucleomorph
					&& !has_nucleomorphs) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup does not have expected nucleomorph flag",
							**desc_it, *ctx_it);
				}
			}
			++reply_it;
			++desc_it;
			++ctx_it;
		}
		// process feat responses
        feat_it = src_feats.begin(); 
		while (reply_it != reply->GetReply().end()
			   && feat_it != src_feats.end()) {
		    if ((*reply_it)->IsError()) {
				string err_str = "?";
				if ((*reply_it)->GetError().IsSetMessage()) {
					err_str = (*reply_it)->GetError().GetMessage();
				}
				PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
						"Taxonomy lookup failed with message '" + err_str + "'",
						**feat_it);
			} else if ((*reply_it)->IsData()) {
				bool is_species_level = true;
				bool force_consult = false;
				bool has_nucleomorphs = false;
				(*reply_it)->GetData().GetTaxFlags(is_species_level, force_consult, has_nucleomorphs);
				if (!is_species_level) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup reports is_species_level FALSE",
							**feat_it);
				}
				if (force_consult) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup reports taxonomy consultation needed",
							**feat_it);
				}
                if ((*feat_it)->GetData().GetBiosrc().IsSetGenome()
					&& (*feat_it)->GetData().GetBiosrc().GetGenome() == CBioSource::eGenome_nucleomorph
					&& !has_nucleomorphs) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup does not have expected nucleomorph flag",
							**feat_it);
				}
			}
			++reply_it;
			++feat_it;
		}            
	}

	// Now look at specific-host values
    ValidateSpecificHost (src_descs, desc_ctxs, src_feats);

}


void CValidError_imp::ValidateTaxonomy(const COrg_ref& org, int genome)
{
    // request list for taxon3
    vector< CRef<COrg_ref> > org_rq_list;
	CRef<COrg_ref> rq(new COrg_ref);
	rq->Assign(org);
	org_rq_list.push_back(rq);

	CTaxon3 taxon3;
	taxon3.Init();
	CRef<CTaxon3_reply> reply = taxon3.SendOrgRefList(org_rq_list);
	if (reply) {
		CTaxon3_reply::TReply::const_iterator reply_it = reply->GetReply().begin();

		while (reply_it != reply->GetReply().end()) {
		    if ((*reply_it)->IsError()) {
				string err_str = "?";
				if ((*reply_it)->GetError().IsSetMessage()) {
					err_str = (*reply_it)->GetError().GetMessage();
				}
				PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup failed with message '" + err_str + "'", org);
			} else if ((*reply_it)->IsData()) {
				bool is_species_level = true;
				bool force_consult = false;
				bool has_nucleomorphs = false;
				(*reply_it)->GetData().GetTaxFlags(is_species_level, force_consult, has_nucleomorphs);
				if (!is_species_level) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup reports is_species_level FALSE", org);
				}
				if (force_consult) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup reports taxonomy consultation needed", org);
				}
                if (genome == CBioSource::eGenome_nucleomorph
					&& !has_nucleomorphs) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_TaxonomyLookupProblem, 
							"Taxonomy lookup does not have expected nucleomorph flag", org);
				}
			}
			++reply_it;
		}
    }

	// Now look at specific-host values
    org_rq_list.clear();

    FOR_EACH_ORGMOD_ON_ORGREF  (mod_it, org) {
        if ((*mod_it)->IsSetSubtype()
            && (*mod_it)->GetSubtype() == COrgMod::eSubtype_nat_host
            && (*mod_it)->IsSetSubname()
            && isupper ((*mod_it)->GetSubname().c_str()[0])) {
       	    string host = (*mod_it)->GetSubname();
            size_t pos = NStr::Find(host, " ");
            if (pos != string::npos) {
                if (! NStr::StartsWith(host.substr(pos + 1), "sp.")
                    && ! NStr::StartsWith(host.substr(pos + 1), "(")) {
                    pos = NStr::Find(host, " ", pos + 1);
                    if (pos != string::npos) {
                        host = host.substr(0, pos);
                    }
                } else {
                    host = host.substr(0, pos);
                }
            }

            CRef<COrg_ref> rq(new COrg_ref);
    	    rq->SetTaxname(host);
            org_rq_list.push_back(rq);
        }
    }

    reply = taxon3.SendOrgRefList(org_rq_list);
	if (reply) {
		CTaxon3_reply::TReply::const_iterator reply_it = reply->GetReply().begin();
		vector< CRef<COrg_ref> >::iterator rq_it = org_rq_list.begin();

		while (reply_it != reply->GetReply().end()
			   && rq_it != org_rq_list.end()) {
		    
		    string host = (*rq_it)->GetTaxname();
		    if ((*reply_it)->IsError()) {
				string err_str = "?";
				if ((*reply_it)->GetError().IsSetMessage()) {
					err_str = (*reply_it)->GetError().GetMessage();
				}
				if(NStr::Find(err_str, "ambiguous") != string::npos) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost,
								"Specific host value is ambiguous: " + host, org);
				} else {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Invalid value for specific host: " + host, org);
				}
			} else if ((*reply_it)->IsData()) {
				if (s_HasMisSpellFlag((*reply_it)->GetData())) {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Specific host value is misspelled: " + host, org);
				} else if ((*reply_it)->GetData().IsSetOrg()) {
                    string match = s_FindMatchInOrgRef (host, (*reply_it)->GetData().GetOrg());
					if (!NStr::EqualCase(match, host)) {
						PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
									"Specific host value is incorrectly capitalized:  " + host, org);
					}
				} else {
					PostErr (eDiag_Warning, eErr_SEQ_DESCR_BadSpecificHost, 
								"Invalid value for specific host: " + host, org);
				}
			}
			++reply_it;
			++rq_it;
		}
    }

}


CPCRSet::CPCRSet(size_t pos) : m_OrigPos (pos)
{
}


CPCRSet::~CPCRSet(void)
{
}


CPCRSetList::CPCRSetList(void)
{
	m_SetList.clear();
}


CPCRSetList::~CPCRSetList(void)
{
	for (int i = 0; i < m_SetList.size(); i++) {
		delete m_SetList[i];
	}
	m_SetList.clear();
}


void CPCRSetList::AddFwdName (string name)
{
	int pcr_num = 0;
	if (NStr::StartsWith(name, "(") && NStr::EndsWith(name, ")") && NStr::Find(name, ",") != string::npos) {
		name = name.substr(1, name.length() - 2);
		vector<string> mult_names;
		NStr::Tokenize(name, ",", mult_names);
		int name_num = 0;
		while (name_num < mult_names.size()) {
			while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetFwdName())) {
				pcr_num++;
			}
			if (pcr_num == m_SetList.size()) {
				m_SetList.push_back(new CPCRSet(pcr_num));
			}
			m_SetList[pcr_num]->SetFwdName(mult_names[name_num]);
			name_num++;
			pcr_num++;
		}
	} else {
		while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetFwdName())) {
			pcr_num++;
		}
		if (pcr_num == m_SetList.size()) {
			m_SetList.push_back(new CPCRSet(pcr_num));
		}
		m_SetList[pcr_num]->SetFwdName(name);
	}
}


void CPCRSetList::AddRevName (string name)
{
	int pcr_num = 0;
	if (NStr::StartsWith(name, "(") && NStr::EndsWith(name, ")") && NStr::Find(name, ",") != string::npos) {
		name = name.substr(1, name.length() - 2);
		vector<string> mult_names;
		NStr::Tokenize(name, ",", mult_names);
		int name_num = 0;
		while (name_num < mult_names.size()) {
			while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetRevName())) {
				pcr_num++;
			}
			if (pcr_num == m_SetList.size()) {
				m_SetList.push_back(new CPCRSet(pcr_num));
			}
			m_SetList[pcr_num]->SetRevName(mult_names[name_num]);
			name_num++;
			pcr_num++;
		}
	} else {
		while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetRevName())) {
			pcr_num++;
		}
		if (pcr_num == m_SetList.size()) {
			m_SetList.push_back(new CPCRSet(pcr_num));
		}
		m_SetList[pcr_num]->SetRevName(name);
	}
}


void CPCRSetList::AddFwdSeq (string name)
{
	int pcr_num = 0;
	if (NStr::StartsWith(name, "(") && NStr::EndsWith(name, ")") && NStr::Find(name, ",") != string::npos) {
		name = name.substr(1, name.length() - 2);
		vector<string> mult_names;
		NStr::Tokenize(name, ",", mult_names);
		int name_num = 0;
		while (name_num < mult_names.size()) {
			while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetFwdSeq())) {
				pcr_num++;
			}
			if (pcr_num == m_SetList.size()) {
				m_SetList.push_back(new CPCRSet(pcr_num));
			}
			m_SetList[pcr_num]->SetFwdSeq(mult_names[name_num]);
			name_num++;
			pcr_num++;
		}
	} else {
		while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetFwdSeq())) {
			pcr_num++;
		}
		if (pcr_num == m_SetList.size()) {
			m_SetList.push_back(new CPCRSet(pcr_num));
		}
		m_SetList[pcr_num]->SetFwdSeq(name);
	}
}


void CPCRSetList::AddRevSeq (string name)
{
	int pcr_num = 0;
	if (NStr::StartsWith(name, "(") && NStr::EndsWith(name, ")") && NStr::Find(name, ",") != string::npos) {
		name = name.substr(1, name.length() - 2);
		vector<string> mult_names;
		NStr::Tokenize(name, ",", mult_names);
		int name_num = 0;
		while (name_num < mult_names.size()) {
			while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetRevSeq())) {
				pcr_num++;
			}
			if (pcr_num == m_SetList.size()) {
				m_SetList.push_back(new CPCRSet(pcr_num));
			}
			m_SetList[pcr_num]->SetRevSeq(mult_names[name_num]);
			name_num++;
			pcr_num++;
		}
	} else {
		while (pcr_num < m_SetList.size() && NStr::IsBlank(m_SetList[pcr_num]->GetRevSeq())) {
			pcr_num++;
		}
		if (pcr_num == m_SetList.size()) {
			m_SetList.push_back(new CPCRSet(pcr_num));
		}
		m_SetList[pcr_num]->SetRevSeq(name);
	}
}


static bool s_PCRSetCompare (
    const CPCRSet* p1,
    const CPCRSet* p2
)

{
	int compare = NStr::CompareNocase(p1->GetFwdSeq(), p2->GetFwdSeq());
	if (compare < 0) {
		return true;
	} else if (compare > 0) {
		return false;
	} else if ((compare = NStr::CompareNocase(p1->GetRevSeq(), p2->GetRevSeq())) < 0) {
		return true;
	} else if (compare > 0) {
		return false;
	} else if ((compare = NStr::CompareNocase(p1->GetFwdName(), p2->GetFwdName())) < 0) {
		return true;
	} else if (compare > 0) {
		return false;
	} else if ((compare = NStr::CompareNocase(p1->GetRevName(), p2->GetRevName())) < 0) {
		return true;
	} else if (p1->GetOrigPos() < p2->GetOrigPos()) {
		return true;
	} else {
		return false;
	}
}


static bool s_PCRSetEqual (
    const CPCRSet* p1,
    const CPCRSet* p2
)

{
	if (NStr::EqualNocase(p1->GetFwdSeq(), p2->GetFwdSeq())
		&& NStr::EqualNocase(p1->GetRevSeq(), p2->GetRevSeq())
		&& NStr::EqualNocase(p1->GetFwdName(), p2->GetFwdName())
		&& NStr::EqualNocase(p1->GetRevName(), p2->GetRevName())) {
		return true;
	} else {
		return false;
	}
}


bool CPCRSetList::AreSetsUnique(void)
{
	stable_sort (m_SetList.begin(), 
				 m_SetList.end(),
				 s_PCRSetCompare);

	return seq_mac_is_unique (m_SetList.begin(),
                   m_SetList.end(),
                   s_PCRSetEqual);

}


// CCountryBlock
CCountryBlock::CCountryBlock 
(string country_name, double min_x, double min_y, double max_x, double max_y)
: m_CountryName(country_name) ,
  m_MinX(min_x) ,
  m_MinY(min_y) ,
  m_MaxX(max_x) ,
  m_MaxY(max_y)
{
}


CCountryBlock::~CCountryBlock (void)
{
}

bool CCountryBlock::IsLatLonInCountryBlock (double x, double y)
{
	if (m_MinX <= x && m_MaxX >= x && m_MinY <= y && m_MaxY >= y) {
		return true;
	} else {
		return false;
	}
}


CCountryLatLonMap::CCountryLatLonMap (void) 
{
	// initialize list of country blocks
    m_CountryBlockList.clear();

	// note - may want to do this initialization later, when needed
    string dir;
    if (CNcbiApplication* app = CNcbiApplication::Instance()) {
        dir = app->GetConfig().Get("NCBI", "Data");
        if ( !dir.empty()  
            && CFile(CDirEntry::MakePath(dir, "country_lat_lon.txt")).Exists()) {
            dir = CDirEntry::AddTrailingPathSeparator(dir);
        } else {
            dir.erase();
        }
    }
    if (dir.empty()) {
        ERR_POST_X(2, Info << "CCountryLatLonMap: "
                   "data not found.");
    }

    CRef<ILineReader> lr;
    if ( !dir.empty() ) {
        lr.Reset(ILineReader::New
                 (CDirEntry::MakePath(dir, "country_lat_lon.txt")));
    }
    if (!lr.Empty()) {
        for (++*lr; !lr->AtEOF(); ++*lr) {
			const string& line = **lr;
			vector<string> tokens;
			NStr::Tokenize(line, "\t", tokens);
			if (tokens.size() < 6 || (tokens.size() - 2) % 4 > 0) {
				ERR_POST_X(1, Warning << "Malformed country_lat_lon.txt line " << line
						   << "; disregarding");
			} else {
				vector <CCountryBlock *> blocks_from_line;
				bool line_ok = true;
				try {
					size_t offset = 2;
					while (offset < tokens.size()) {
						CCountryBlock *block = new CCountryBlock(tokens[0],
							NStr::StringToDouble(tokens[offset + 1]),
							NStr::StringToDouble(tokens[offset]),
							NStr::StringToDouble(tokens[offset + 3]),
							NStr::StringToDouble(tokens[offset + 2]));
                        blocks_from_line.push_back(block);
					    offset += 4;
					}
				} catch (CException ) {
					line_ok = false;
				}
				if (line_ok) {
					for (int i = 0; i < blocks_from_line.size(); i++) {
						m_CountryBlockList.push_back(blocks_from_line[i]);
					}
				} else {
					ERR_POST_X(1, Warning << "Malformed country_lat_lon.txt line " << line
							   << "; disregarding");
					for (int i = 0; i < blocks_from_line.size(); i++) {
						delete blocks_from_line[i];
					}
				}
			}
		}
	}
}


CCountryLatLonMap::~CCountryLatLonMap (void)
{
	size_t i;

	for (i = 0; i < m_CountryBlockList.size(); i++) {
        delete (m_CountryBlockList[i]);
	}
	m_CountryBlockList.clear();
}


bool CCountryLatLonMap::IsCountryInLatLon (string country, double x, double y)
{
	// note - if we need more speed later, create country index and search
	size_t i;

	for (i = 0; i < m_CountryBlockList.size(); i++) {
		if (NStr::Equal (country, m_CountryBlockList[i]->GetCountry())) {
			if (m_CountryBlockList[i]->IsLatLonInCountryBlock(x, y)) {
				return true;
			}
		}
	}
	return false;
}


string CCountryLatLonMap::GuessCountryForLatLon(double x, double y)
{
	// note - if we need more speed later, create x then y index
    size_t i;

	for (i = 0; i < m_CountryBlockList.size(); i++) {
		if (m_CountryBlockList[i]->IsLatLonInCountryBlock(x, y)) {
			return m_CountryBlockList[i]->GetCountry();
		}
	}
	return "";
}


bool CCountryLatLonMap::HaveLatLonForCountry(string country)
{
	// note - if we need more speed later, create country index and search
	size_t i;

	for (i = 0; i < m_CountryBlockList.size(); i++) {
		if (NStr::Equal (country, m_CountryBlockList[i]->GetCountry())) {
            return true;
		}
	}
	return false;
}


const string CCountryLatLonMap::sm_BodiesOfWater [] = {
  "Bay",
  "Canal",
  "Channel",
  "Coastal",
  "Cove",
  "Estuary",
  "Fjord",
  "Freshwater",
  "Gulf",
  "Harbor",
  "Inlet",
  "Lagoon",
  "Lake",
  "Narrows",
  "Ocean",
  "Passage",
  "River",
  "Sea",
  "Seawater",
  "Sound",
  "Strait",
  "Water",
  "Waters"
};


bool CCountryLatLonMap::DoesStringContainBodyOfWater(const string& country)
{
	const string *begin = sm_BodiesOfWater;
    const string *end = &(sm_BodiesOfWater[sizeof(sm_BodiesOfWater) / sizeof(string)]);
    bool found = false;
	const string *it = begin;

	while (!found && it < end) {
		if (NStr::Find(country, *it) != string::npos) {
			return found = true;
		}
		++it;
	}
    return found;
}


// ===== for validating instituation and collection codes in specimen-voucher, ================
// ===== biomaterial, and culture-collection BioSource subsource modifiers     ================

typedef map<string, string, PNocase> TInstitutionCodeMap;
static TInstitutionCodeMap s_BiomaterialInstitutionCodeMap;
static TInstitutionCodeMap s_SpecimenVoucherInstitutionCodeMap;
static TInstitutionCodeMap s_CultureCollectionInstitutionCodeMap;
static TInstitutionCodeMap s_InstitutionCodeTypeMap;
static bool                    s_InstitutionCollectionCodeMapInitialized = false;
DEFINE_STATIC_FAST_MUTEX(s_InstitutionCollectionCodeMutex);

static const char* const kInstitutionCollectionCodeList[] = {
"A	s	Arnold Arboretum, Harvard University",
"AA	s	Ministry of Science, Academy of Sciences",
"AAH	s	Arnold Arboretum, Harvard University",
"AAPI	s	Plant Industry Laboratory",
"AAR	s	Reliquae Aaronsohnianae",
"AAS	s	British Antarctic Survey",
"AAU	s	University of Aarhus, Institute of Biological Sciences",
"AAUB	s	Anhui Agricultural University, Department of Basic Courses",
"AAUF	s	Anhui Agricultural University, Forest Utilization Faculty",
"ABB	c	Asian Bacterial Bank",
"ABD	s	University of Aberdeen, Plant and Soil Science Department",
"ABDAM	s	Aberdeen Art Gallery and Museum",
"ABDC	s	Aba Institute for Drug Control",
"ABDF	s	University of Aberdeen, Forestry Department",
"ABDH	s	United Arab Emirates University, Department of Biology",
"ABDM	s	Marischal College, University of Aberdeen",
"ABFM	s	The Barnes Foundation Arboretum",
"ABH	s	Universidad de Alicante, Centro Iberoamericano de la Biodiversidad (CIBIO)",
"ABI	s	Centre ORSTOM d'Adiopodoume",
"ABKMI	c	Department of Applied Biology, Faculty of science",
"ABL	s	Adviesbureau voor Bryologie en Lichenologie",
"ABN	s	Radley College",
"ABO	s	Aboyne Castle",
"ABRC	b	Arabidopsis Biological Resource Center",
"ABRIICC	c	ABRIICC Agricultural Biotechnology Research Institute of Iran Culture collection",
"ABRN	s	Centre for Ecology and Hydrology",
"ABS<UK>	s	University of Wales, Botany Department",
"ABS<USA>	s	Archbold Biological Station",
"ABSH	s	Southern Illinois Universitiy, Department of Plant Biology",
"ABSL	s	University of Minnesota, American Bryological and Lichenological Society",
"ABSM	s	Duke University, Botany Department",
"ABT	s	Laboratoire de Biologie Vegetale et d'Ecologie Forestiere",
"ABTC	c	Australian Biological Tissue Collection, South Australian Museum",
"AC	s	Amherst College",
"ACA	s	Agricultural University of Athens",
"ACA-DC	c	Greek Coordinated Collections of Microorganisms",
"ACAD	s	Acadia University, K. C. Irving Environmental Science Centre & Harriet Irving Botanical Gardens",
"ACAM	c	The Australian Collection of Antarctic Microorganisms, Cooperative Research Center for the Antarctic and Southern Ocean Environment",
"ACAP	s	Aquaculture Center of Aomori Prefecture",
"ACBC	s	Agriculture Canada Research Station",
"ACBR	c	Austrian Center of Biological Resources and Applied Mycology",
"ACBV	s	Agriculture Canada Research Station, The Aphids of British Columbia",
"ACC	s	Oak Hill Park Museum",
"ACCC	c	Agricultural Culture Collection of China",
"ACD	s	Alemaya University of Agriculture",
"ACE<CHN>	s	Anhui College of Education, Biology Department",
"ACE<EGY>	s	Arachnid Collection of Egypt",
"ACH	c	Mycology Culture Collection, Women's and Children's Hospital",
"ACHE	s	Institute of Terrestrial Ecology",
"ACK	s	Agriculture and Agri-Food Canada",
"ACM<AUST-QLD>	c	Australian Collection of Microorganisms",
"ACM<CHN>	s	Anhui College of Traditional Chinese Medicine, Chinese Materia Medica Department",
"ACNB	s	Agriculture Canada Research Station",
"ACNS	s	Agriculture Canada Nova Scotia",
"ACOI	c	Coimbra Collection of Algae",
"ACOR	s	Universidad Nacional de Cordoba",
"ACTC	s	Austin College",
"ACUNHC	s	Abilene Christian University, Natural History Collection",
"AD	s	Plant Biodiversity Centre",
"ADA	s	Department of Agriculture",
"ADMONT	s	Benediktinerstift Admont",
"ADO	s	Kirikkale University, Biology Department",
"ADR	s	Adrian College, Biology Department",
"ADRZ	s	Rudjer Boskovic Institute, CIM-Botany",
"ADSH	s	Arachnology Division",
"ADT	s	Antarctic Division",
"ADU	s	University of Adelaide, Botany Department",
"ADUG	s	Geology Department, University of Adelaide",
"ADUZ	s	Zoology Department, University of Adelaide",
"ADW	s	University of Adelaide",
"AEF	s	University of Ankara, Department of Pharmaceutical Botany",
"AEI	s	American Entomological Institute",
"AEIC	s	American Entomological Institute",
"AES	s	University of Alaska",
"AESB	s	Agriculture Experiment Station",
"AFAQ	s	Amateur Fisheries Association of Queensland",
"AFES	s	Maritimes Forest Research Centre",
"AFGMC	s	Alaska Department of Fish and Game",
"AFS	s	University of Michigan",
"AFSDU	s	Suleyman Demirel University, Agricultural Faculty",
"AGRITEC	b	AGRITEC, Ltd.",
"AGRL	s	Lethbridge Research Station",
"AGUAT	s	Universidad de San Carlos",
"AH	s	Universidad de Alcala, Departamento de Biologia Vegetal",
"AHBC	s	Dixie College",
"AHF	s	Allan Hancock Foundation, University of Southern California",
"AHFH	s	University of Southern California",
"AHLDA	c	Animal Health Division Culture Collection",
"AHMA	s	Agharkar Research Institute, Maharashtra Association for the Cultivation of Science, Botany Group",
"AHNU	s	Anhui Normal University Conservation Genetics Lab",
"AHS	s	Austin High School",
"AHU	c	AHU Culture Collection",
"AHUC	s	University of California, Agronomy and Range Science Department",
"AIB	s	Anhui Institute of Biology",
"AIBU	s	Abant Izzet Baysal Ueniversitesi, Biyoloji Boeluemue",
"AICH	s	Aichi Kyoiku University, Biology Department",
"AIM	s	Auckland Institute and Museum",
"AIMS	s	Australian Institute of Marine Science",
"AIS	s	Academie imperial des Sciences",
"AISIY	s	Armenian Institute for the Scientific Investigation of Cattle Breeding and Veterinary, Department of Meadows and Pastures",
"AIX	s	Museum d'Histoire Naturelle d'Aix-en-Provence",
"AJ	c	Central Research Laboratories",
"AJBC	s	Atkins Jardin Botanico de Cienfuegos",
"AJOU	s	Ajou University, Biological Sciences Department",
"AK	s	Auckland War Memorial Museum",
"AKU<JPN>	c	Faculty of Agriculture",
"AKU<NZ>	s	University of Auckland, School of Biological Sciences",
"AL	s	Universite d'Alger",
"ALA<USA-AK>	s	University of Alaska Museum",
"ALA<USA-AL>	s	University of Alabama Museum of Natural History",
"ALAJ	s	University of Alaska",
"ALAM	s	Adams State College, Biology Department",
"ALB	s	Civico Museo Archeologico e di Scienze Naturali Federico Eusebio",
"ALBA	s	Universidad de Castilla, La Mancha, Departamento de Ciencia y Tecnologia Agroforestal",
"ALBC	s	Albion College, Biology Department",
"ALBU	s	Rocky Mountain Forest and Range Experiment Station",
"ALCB	s	Universidade Federal da Bahia, Campus Universitario de Ondina",
"ALCP	c	Algotheque du Laboratoire de Cryptogamie",
"ALD	s	Alderney Society and Museum",
"ALEX	s	University of Alexandria, Department of Botany",
"ALF	s	Campus International de Baillarguet, Departement d'Elevage et de Medecine Veterinaire",
"ALGOBANK	c	ALGOBANK",
"ALIRU	c	Australian Legume Inoculants Research Unit",
"ALK	s	Alnwick Scientific and Mechanical Institution",
"ALM<FRA>	s	Museum National Historie Naturelle",
"ALM<GBR>	s	Art Gallery and Museum, Central Library",
"ALMA	s	Alma College, Biology Department",
"ALME	s	Estacion Experimental de Zonas Aridas",
"ALN	s	Alnwick Botanical Society",
"ALT	s	Curtis Museum",
"ALTA	s	University of Alberta, Biological Sciences Department",
"ALTB	s	University of Barnaul, Altai State University",
"ALU	s	Alabama Museum of Natural History",
"AM	s	Australian Museum",
"AM:Arachnology	s	Australian Museum, Invertebrate Collections: Arachnology",
"AM:EBU	s	Australian Museum, Evolutionary Biology Unit Tissue Collection",
"AM:Entomology	s	Australian Museum, Invertebrate Collections: Entomology",
"AM:Herpetology	s	Australian Museum, Vertebrate Collections: Herpetology",
"AM:Ichthyology	s	Australian Museum, Vertebrate Collections: Ichthyology",
"AM:Malacology	s	Australian Museum, Invertebrate Collections: Malacology",
"AM:Mammalogy	s	Australian Museum, Vertebrate Collections: Mammalogy",
"AM:Marine_Invertebrates	s	Australian Museum, Invertebrate Collections: Marine and other invertebrates",
"AM:Mineralogy	s	Australian Museum, Earth Sciences: Mineralogy and Petrology Collection",
"AM:Ornithology	s	Australian Museum, Vertebrate Collections: Ornithology",
"AM:Palaeontology	s	Australian Museum, Palaeontology Collection",
"AM<BLG>	s	Asenovgrad Museum",
"AMAZ	s	Universidad Nacional de la Amazonia Peruana",
"AMC	c	Department of Biologics Research",
"AMCC	s	Ambrose Monell Cryo Collection, American Museum of Natural History",
"AMCL	s	Macapa, Museu Territorial de Historia Natural \"Angelo Moreira da Costa Lima\"",
"AMD	s	Hugo de Vries-Laboratory, University of Amsterdam",
"AMDE	s	Brathay Field Centre for Exploration and Field Studies",
"AMES	s	Harvard University",
"AMG	s	Albany Museum",
"AMGS	s	Albany Museum",
"AMH	s	Agharkar Research Institute, Mycology and Plant Pathology Department",
"AMMRL	c	Australian National Reference Laboratory in Medical Mycology",
"AMMS	s	Academy of Military Medical Sciences",
"AMNH	s	American Museum of Natural History",
"AMNH:Herp	s	American Museum of Natural History, Herpetology collection",
"AMNH<ISL>	s	Icelandic Institute of Natural History, Akureyri Division",
"AMNZ	s	Auckland Institute and Museum",
"AMO	s	Herbario AMO",
"AMP<AUS>	c	Australian Mycological Panel",
"AMP<GBR>	s	Ampleforth College",
"AMS	s	Australian Museum",
"AMSA	s	Albany Museum",
"AMUZ	s	Aligarh Muslim University",
"ANA	s	Orange County Department of Agriculture",
"ANC	s	Universita di Ancona, Dipartimento di Biotecnologie Agrarie ed Ambientali",
"ANCB	s	Museo Nacional de Historia Natural",
"AND	s	Slezske zemske muzeum Opava, Arboretum Nopvy Dvur, Dendrology Department",
"ANES	s	Anadolu University, Biology Department",
"ANFM	s	Associazione Naturalisti Forlivesi Pro Museo",
"ANG	s	Arboretum de la Maulevrie",
"ANGU	s	Instituto Nacional de Tecnologia Agropecuaria",
"ANGUC	s	Universite Catholiques de l'Ouest",
"ANH	s	Andong National University, School of Bioresource Science",
"ANIC	s	Australian National Insect Collection",
"ANK	s	Ankara Ueniversitesi, Biyoloji Boeluemue",
"ANKO	s	Forest Research Institute",
"ANLW	s	Amt Der Niederosterreichischen Landsregierung",
"ANMR	c	Asian Network on Microbial Researches",
"ANSM	s	Universidad Autonoma Agraria Antonio Narro, Departamento de Botanica",
"ANSP	s	Academy of Natural Sciences of Philadelphia",
"ANTU	s	Changbai Mountain National Nature Reserve Administration Bureau",
"ANU<AUS>	s	Australian National University",
"ANU<CHN>	s	Anhui University, Biology Department",
"ANUB	s	Anhui Normal University, Biology Department",
"ANUC	s	Australian National University, Chemistry Department",
"ANUG	s	Anhui Normal University, Geography Department",
"ANWC	s	Australian National Wildlife Collection",
"AO	s	Museo Regionale di Scienze Naturali della Valle d'Aosta",
"APCR	s	Arkansas Tech University, Biological Sciences Department",
"APEI	s	Agriculture Canada Research Station",
"APH	s	Society of Apothecaries",
"APHI	s	U.S. Department of Agriculture, Animal and Plant Health Inspection Service",
"APIY	s	Abovian Pedagogical Institute, Botany Department",
"APM	s	Algonquin Provincial Park, Algonquin Visitor Centre",
"APMJ	s	Aomori Prefectural Museum",
"APP	s	Parco Nazionale del Gran Sasso e Monti della Laga - Universita di Camerino, Centro Richerche Floristiche dell'Appennino",
"APSC	s	Austin Peay State University, Biology Department",
"AQC	s	Aquinas College, Biology Department",
"AQUI	s	Universita degli Studi di L'Aquila, Dipartimento di Scienze Ambientali",
"ARAGO	s	Universite Pierre et Marie Curie",
"ARAN	s	Alto de Zorroaga s.n., Departamento de Botanica",
"ARB	s	Salahiddin University, Biology Department",
"ARBH	s	Arbroath Scientific and Natural History Society",
"ARC<ARG>	s	Universidad Nacional del Comahue, Facultad de Ciencias Agrarias",
"ARC<CAN>	s	Atlantic Reference Centre",
"ARCH	s	Archbold Biological Station",
"ARCM	s	Atlantic Reference Centre",
"ARER	s	Associazione Romana di Entomologia",
"ARG	s	Argotti Botanic Garden",
"ARIZ	s	University of Arizona, Department of Plant Sciences",
"ARK	s	University of Arkansas",
"ARM	s	County Museum",
"ARMFN	s	Armagh Field Naturalists' Society",
"ARSEF	c	ARS Collection of Entomopathogenic Fungi",
"AS<DEU>	s	Paleontological Collection",
"AS<PRY>	s	Jardin Botanico",
"ASAY	s	Academy of Science of Armenia",
"ASC	s	Northern Arizona University, Biological Sciences Department",
"ASCC	s	Adams State College Collection",
"ASCU	s	Agricultural Scientific Collections Unit",
"ASDM	s	Arizona-Sonora Museum",
"ASE	s	Universidade Federal de Sergipe, Departamento de Biologia",
"ASH	s	National Institute of Deserts Flora and Fauna",
"ASIB	c	Algensammlung am Institut fur Botanik",
"ASIO	s	Academia Sinica Institute of Oceanology",
"ASIZB	s	Academia Sinica Institute of Zoology",
"ASIZT	s	Academia Sinica Institute of Zoology",
"ASM	s	Arts and Science University",
"ASNHC	s	Angelo State Natural History Collection",
"ASSAM	s	Botanical Survey of India, Eastern Circle",
"ASSL	s	Academy of Sciences",
"AST	s	University of Aston",
"ASTC	s	Stephen F. Austin State University, Biology Department",
"ASTN	s	Ashton-under-Lyne Linnean Botanical Society",
"ASU	s	School of Life Sciences, Arizona State University",
"ASUA	s	Ain Shams University",
"ASUF	s	Rocky Mountain Research Station, USDA Forest Service",
"ASUMC	s	Arizona State University, Mammal Collection",
"ASUMZ	s	Arkansas State University, Collection of Recent Mammals",
"ASUT	s	Frank M. Hasbrouck Insect Collection",
"ASW	s	South Valley University, Botany Department",
"ASW<AUT>	c	Culture Collection of Algae at the University of Vienna",
"ATA	s	Atatuerk Ueniversitesi",
"ATCC	c	American Type Culture Collection",
"ATH	s	Goulandris Natural History Museum",
"ATHU	s	National and Kapodistrian University of Athens, Biology Department",
"ATHUM	c	ATHUM Culture Collection of Fungi",
"ATU	c	Dept.of Biotechnology University of Tokyo",
"AU	s	Xiamen University, Biology Department",
"AUA	s	Auburn University, Biological Sciences Department",
"AUB	s	Andrews University",
"AUBL	s	Museum of Natural History",
"AUBSN	s	All-Union Botanical Society",
"AUCE	s	El Azhar University",
"AUEM	s	Auburn University Entomological Museum",
"AUG	s	Augustana College, Biology Department",
"AUGD	s	Department of Geology and Petroleum Geology",
"AUM	s	Auburn University Museum",
"AUT	s	Museum d'Histoire Naturelle",
"AUW	s	Acadia University, Wildlife Museum",
"AV	s	Museum Requien",
"AVE	s	Universidade de Aveiro, Departamento de Biologia",
"AVU	s	Vrije Universiteit, Department of Systematic Botany",
"AWH	s	Dr. Henri Van Heurck Museum",
"AWL	s	Abitibi Paper Company",
"AWQC	c	Australian Water Quality Centre",
"AWRI	c	The Australian Wine Research Institute",
"AYBY	s	Buckinghamshire County Museum Technical Centre",
"AYDN	s	Adnan Menderes University, Department of Biology",
"AYR	s	South Ayrshire Council",
"AZ	s	Museu Carlos Machado, Natural History Department",
"AZAN	s	Akademia Nauk Azerbaijana-Bulgarian Academy of Science of Azerbaijan",
"AZU	s	Universidade dos Acores, Departamento de Ciencias Agrarias",
"AZUS	s	Citrus College, Biological Sciences Department",
"B	s	Berlin Botanic Garden and Botanical Museum",
"BA	s	Museo Argentino de Ciencias Naturales Bernardino Rivadavia",
"BAA	s	Universidad de Buenos Aires, Facultad de Agronomia",
"BAAC	s	Musee de Beni Abbes",
"BAB	s	Instituto Nacional de Tecnologia Agropecuaria, Instituto de Recursos Biologicos",
"BAC<CHN>	s	Beijing Agricultural College",
"BAC<UK>	s	Bacup Natural History Society",
"BACC	c	Brucella AFSSA Culture Collection",
"BACP	s	CEFYBO, Unidad Botanica",
"BAE	s	Willis Museum and Art Gallery",
"BAF	s	Universidad de Buenos Aires, Facultad de Farmacia y Bioquimica",
"BAFC	s	Universidad de Buenos Aires, Departamento de Ciencias Biologicas",
"BAG	s	Ministry of Agriculture",
"BAH<BRZ>	s	Empresa Baiana de Desenvolvimento Agricola",
"BAH<GRM>	s	Biologische Anstalt Helgoland Marine Station",
"BAI	s	Instituto Forestal Nacional (IFONA), Centro Forestal Castelar",
"BAIL	s	Conservatoire Botanique National de Bailleul",
"BAJ	s	Instituto Municipal de Botanica, Parque Pte. Dr. Nicolas Avellaneda",
"BAK	s	Academy of Sciences of Azerbaijan",
"BAL	s	INTA, EEA Balcarce, Catedra de Botanica Agricola",
"BALT	s	Towson University, Department of Biological Sciences",
"BAN	s	Banaras Hindu University, Botany Department",
"BAP	s	Oxford Botanic Garden",
"BAR	s	University of the West Indies, Department of Biological and Chemical Sciences",
"BARC	s	Systematic Botany and Mycology Laboratory, USDA/ARS",
"BARO	s	Maharaja Sayajirao University of Baroda, Botany Department",
"BAS<BGR>	s	Bulgarian Academy of Science",
"BAS<CHE>	s	Universitaet Basel",
"BASBG	s	Universitaet Basel, Basler Botanische Gesellschaft",
"BASSA	s	Museo Civico, Bassano del Grappa",
"BAT	s	Bagshaw Museum",
"BATA	s	Instituto Nacional de Desarollo Forestal",
"BATH	s	Bath Natural History Society",
"BATHG	s	Geology Museum",
"BATU	s	Batumi Botanical Garden, Botany Department",
"BAU	s	Beijing Agricultural University",
"BAV	s	Slovenskej akademie vied",
"BAY	s	Museum d'Histoire Naturelle de Bayonne",
"BAYLU	s	Baylor University, Biology Department",
"BB<ARG>	s	Universidad Nacional del Sur, Departamento de Agronomia",
"BB<USA-WY>	s	Buffalo Bill Museum",
"BBB	s	Universidad Nacional del Sur, Departamento de Biologia, Bioquimica y Farmacia",
"BBF	s	Conservatoire Botanique National de Midi-Pyrenees, Conservatoire botanique pyreneen",
"BBG	s	Birmingham Botanical Gardens",
"BBH	s	National Science and Technology Development Agency",
"BBLF	c	Institut fur Pflanzenschutz im Forst",
"BBNP	s	Big Bend National Park",
"BBPP	c	Bacteriology Branch, Plant Pathology and Microbiology Division, Department of Agricultural Science",
"BBS	s	University of Suriname",
"BBSUK	s	National Museum and Gallery, Department of Biodiversity and Systematic Biology",
"BC	s	Institut Botanic de Barcelona",
"BCB	s	Universitat Autonoma de Barcelona, Unitat de Botanica",
"BCC<ESP>	s	Universitat de Barcelona, Departament de Biologia Vegetal (Unitat de Botanica)",
"BCC<THA>	c	BIOTEC Culture Collection",
"BCCM/IHEM	c	BCCM/IHEM",
"BCCM/LMBP	c	Belgian Coordinated Collections of Microorganisms / LMBP Plasmid Collection",
"BCCM/MUCL	c	The Belgian Co-ordinated Collections of Micro-organisms /BCCM/MUCL (Agro)Industrial Fungi & Yeasts Collection",
"BCCUSP	c	Brazilian Cyanobacteria Collection - University of Sao Paulo",
"BCF	s	Universitat de Barcelona, Laboratori de Botanica",
"BCFH	s	Bureau of Commercial Fisheries",
"BCKN	s	Blackburn Museum and Art Gallery",
"BCL	s	Bates College, Biology Department",
"BCM<SPN>	s	Campus Universitario de Tafira, Departamento de Biologia",
"BCM<USA-NY>	s	Brooklyn Children's Museum",
"BCMEX	s	Universidad Autonoma de Baja California, Reg. MX-HR-007-BC",
"BCMM	s	Beijing College of Traditional Chinese Medicine",
"BCN	s	Universitat de Barcelona",
"BCNP	s	Bryce Canyon National Park",
"BCPM	s	British Columbia Provincial Museum",
"BCRC	c	Bioresource Collection and Research Center",
"BCRJ	c	Rio de Janeiro Cell Bank (Banco de Celulas do Rio de Janeiro)",
"BCRU	s	Universidad Nacional del Comahue, Departamento de Botanica",
"BCTC	s	Birmingham Central Technical College",
"BCU	s	Chulalongkorn University, Botany Department",
"BCUE	s	Department of Biology, Ch'ongju University of Education",
"BCUZ	s	Basque Country University, Laboratory of Zoology",
"BCW	s	Bedford College, University of London",
"BCWL	s	Biological Control of Weeds Laboratory-Europe",
"BDD	s	University of Bradford, Biology Department",
"BDI	s	Putnam Museum of History and Natural Science, Natural History Department",
"BDK	s	North Hertfordshire Museums Service, Natural History Department",
"BDLU	s	Laurentian University",
"BDMU	s	McMaster University",
"BDPA	s	Arboretum, Bolestraszyce - Zamek, Department of Physiography",
"BDUC	s	University of Calgary",
"BDUW	s	University of Waterloo",
"BDUZ	c	Biological Sciences",
"BDWC	s	University of Windsor",
"BDWL	s	Wilfred Laurier University",
"BDWR	s	Bridgewater College, Biology Department",
"BEAN	s	Bridge of Allan Museum",
"BED	s	Bedford Public Library",
"BEDF	s	New England Wild Flower Society",
"BEDPL	s	Bedford Public Library",
"BEG	c	La Banque European des Glomales",
"BEGO	s	Beth Gordon Institute",
"BEI	s	American University of Beirut, Biology Department",
"BEL	s	Ulster Museum, Botany Department",
"BELC	s	Beloit College, Biology Department",
"BELZ	s	Zapovednik Belogorje State Nature Reserve",
"BENH	s	British Entomological and Natural History Society",
"BENIN	s	Universite National du Benin",
"BEO	s	Natural History Museum, Botany Department",
"BEOU	s	University of Belgrade, Faculty of Biology",
"BER	s	Orto Botanico de Bergamo \"Lorenzo Rota\"",
"BEREA	s	Berea College, Biology Department",
"BERN	s	University of Bern",
"BESM	s	Bvumbwe Experiment Station",
"BEV	s	Borough of Beverley Art Gallery and Museum",
"BEX	s	Bexhill Museum",
"BFBI	s	Biologisches Forschunsstation Burgenland",
"BFD	s	Bedfordshire Natural History Society",
"BFDL	s	Forest Products Laboratory",
"BFIC	s	Museum National d'Histoire Naturelle",
"BFT	s	Queen's University, Botany Department",
"BFUS	s	University of Sofia, Biology Faculty",
"BFY	s	John Innes Horticultural Institution",
"BG	s	University of Bergen, Botanical Museum",
"BGAAS	s	Botanical Garden of the Armenian Academy of Sciences, Flora and Vegetation Department",
"BGHan	s	Bundesanstalf fuer Geowissenschaften und Rohstoffe",
"BGM	s	Bath Geology Museum (now the Royal Literary and Scientific Institution)",
"BGR	s	Bundesanstalt fur Geowissenschaften und Rohstoffe",
"BGS	s	British Geological Survey",
"BGSC	c	Bacillus Genetic Stock Center",
"BGSU	s	Bowling Green State University, Biological Sciences Department",
"BH	s	Cornell University, Department of Plant Biology",
"BHAG	s	T. M. Bhagalpur University, Botany Department",
"BHAV	s	Central Salt and Marine Chemicals Research Institute",
"BHCB	s	Universidade Federal de Minas Gerais, Departamento de Botanica",
"BHD	s	Williamson Art Gallery and Museum",
"BHDL	s	Wirral Central Area Reference Library",
"BHDS	s	Birkenhead School",
"BHM<UK>	s	University of Birmingham, Birmingham Natural Society",
"BHM<USA-SD>	s	Black Hills Museum of Natural History",
"BHMG	s	Instituto Agronomico",
"BHMH	s	Universidade Federal de Minas Gerais, Museu de Historia Natural",
"BHO	s	Ohio University, Environmental and Plant Biology Department",
"BHSC	s	Black Hills State University, Biology Department",
"BHU	s	Humboldt-Universitaet zu Berlin, Institut fuer Biologie",
"BHUPM	s	Museum fuer Naturkunde, Institut fuer Palaeontologie",
"BHUPP	s	Banaras Hindu University, Mycology and Plant Pathology Department",
"BI	s	Istituto Ortobotanico",
"BIA	s	British Institute of Archaeology",
"BIDA	s	Boise State University",
"BIE	s	Instituto di Entomologia",
"BIEL	s	Universitaet Bielefeld, Abteilung Oekologie",
"BIGU	s	Universidad de San Carlos de Guatemala, Departamento de Botanica",
"BIL	s	Forest Research Institute, Natural Forest Department",
"BILAS	s	Institute of Botany",
"BIM	s	Birmingham Natural History and Microscopical Society",
"BING	s	State University of New York, Biological Sciences Department",
"BIO	s	Universidad del Pais Vasco/EHU, Departamento de Biologia Vegetal y Ecologia (Botanica)",
"BioCC	c	BioCC BioCen Culture Collection",
"BIOT	s	Regional Center for Tropical Biology",
"BIRA	s	Birmingham Museums and Art Gallery, Curatorial Services",
"BIRM	s	University of Birmingham",
"BISH	s	Bishop Museum, Department of Natural Sciences",
"BISHOP	s	Bernice P. Bishop Museum",
"BITU	s	Department of Biology, Faculty of Science, Toyama University",
"BIUB	s	Mongolian Academy of Sciences",
"BJA	s	University of Burundi, Biology Department",
"BJFC	s	Beijing Forestry University",
"BJM	s	Beijing Natural History Museum",
"BJTC	s	Capital Normal University, Biology Department",
"BK	s	Department of Agriculture",
"BKF	s	Royal Forest Department",
"BKL	s	Brooklyn Botanic Garden",
"BKNU	s	Kunsan National University",
"BLA	s	Fundacao Estadual de Pesquisa Agropecuaria",
"BLAT	s	St. Xavier's College, Botany Department",
"BLCU	s	Bee Biology and Systematics Laboratory",
"BLFU	s	University of the Free State, Department of Botany and Genetics",
"BLGA	s	Burgenlandisches Landesmuseum",
"BLH	s	Cranbrook Institute of Science",
"BLIH	s	Biological Laboratory Imperial Household of Japan",
"BLMLK	s	Bureau of Land Management",
"BLT	s	Belfast Natural History and Philosophical Society",
"BLUZ	s	Museo de Biologia",
"BLWG	c	Bayerische Landesanstalt fur Weinbau und Gartenbau",
"BLY	s	Harvey Institute, Barnsley Naturalist and Scientific Society",
"BM<GBR-BRISTOL>	s	Bristol Museum",
"BM<GBR-LONDON>	s	The Natural History Museum, Department of Botany",
"BMAM	s	Beijing Natural History Museum",
"BMB	s	Booth Museum of Natural History",
"BMBN<UK>	s	Booth Museum of Natural History",
"BMFM-UNAM	c	Culture Collection of Fungal Pathogens Strains from the Basic Mycology Laboratory of the Department of Microbiology and Parasitology, Faculty of Medicine, UNAM",
"BMGB	s	Barbados Museum and Historical Society",
"BMH	s	Bournemouth Natural Science Society Museum, herbarium",
"BMHP	s	Bermuda Department of Agriculture and Fisheries",
"BMKB	s	Brunei Museum",
"BMM	s	Buergermeister Mueller, Museum",
"BMNH	s	Natural History Museum",
"BMNHC	s	Burpee Museum of Natural History",
"BMPS	s	Bristol Museum and Philosophical Society",
"BMR	s	Bureau of Mineral Resources",
"BMRP	s	Burpee Museum Rockford Paleontology",
"BMSA	s	National Museum Bloemfontein",
"BMSC	s	Buffalo Museum of Science",
"BMUK	s	Bolton Museum",
"BNA<ESP>	c	National Bank of Algae",
"BNA<GBR>	s	British (Empire) Naturalists' Association",
"BNBE	s	YMCA Hostel",
"BNFF	s	Banff Museum",
"BNH	s	Nassau Botanical Gardens, Department of Agriculture",
"BNHD	s	Bengal Natural History Museum",
"BNHM<CHN>	s	Beijing Natural History Museum",
"BNHM<IND>	s	Bombay Natural History Museum",
"BNHS	s	Bombay Natural History Society",
"BNI	c	Bernhard Nocht Institute for Tropical Medicine",
"BNL	s	Bundesamt fuer Naturschutz",
"BNP	s	Banff Park Museum",
"BNPL	s	Brighton Public Library",
"BNS	s	Bristol Museum and Art Gallery",
"BNU	s	Beijing Normal University, Biology Department",
"BO	s	Herbarium Bogoriense",
"BOC	s	Bingham Oceanographic Collection",
"BOCH	s	Ruhr-Universitaet Bochum, Spezielle Botanik",
"BOD	s	University of Oxford",
"BOG	s	Universidad de La Salle",
"BOIS	s	Rocky Mountain Research Station",
"BOL	s	University of Cape Town, Botany Department",
"BOLO	s	Universita di Bologna",
"BOLV	s	Nacional Forestal Martin Cardenas",
"BON	s	Bolton Museum, Art Gallery and Aquarium",
"BONB	s	Bolton Botanical Society",
"BONL	s	Bolton Linnean Society",
"BONN	s	Botanisches Institut und Botanischer Garten der Universitaet Bonn",
"BOON	s	Appalachian State University, Biology Department",
"BOR	s	Guermonprez Museum",
"BORD	s	Jardin Botanique de la Ville de Bordeaux",
"BORH	s	Universiti Malaysia Sabah",
"BORN	s	Institute for Tropical Biology and Conservation, Borneensis",
"BOROK	c	The Collection of algae",
"BOSC	s	Boston State College, Biology Department",
"BOTU	s	Universidade Estadual Paulista, Departamento de Botanica",
"BOUM	s	Museum d'Histoire Naturelle de Bourges",
"BOZ	s	Naturmuseum Suedtirol/Museo Scienze Naturali Alto Adige",
"BP	s	Hungarian Natural History Museum, Botanical Department",
"BPBM	s	Bernice P. Bishop Museum",
"BPI<USA-MD>	s	U.S. National Fungus Collections, Systematic Botany and Mycology Laboratory",
"BPI<ZAF>	s	Bernard Price Institute for Palaeontological Research",
"BPIC	c	Benaki Phytopathological Institute Collection",
"BPL	s	Museum of Barnstaple & North Devon",
"BPM	s	Beipiao Paleontological Museum",
"BPPT-ESC	c	BPPT Ethanol-Single Cell Protein-Fructose Syrup Technical Unit",
"BPS	s	California Department of Food and Agriculture",
"BPU	s	Eoetvoes Lorand University, Department of Plant Taxonomy and Ecology",
"BR<BEL>	s	Jardin Botanique National de Belgique",
"BR<BRA>	c	Embrapa Agrobiology Diazothrophic Microbial Culture Collection",
"BRA	s	Slovak National Museum, Botany Department",
"BRAD	s	University of Bradford, Biology Department",
"BRC	s	Botanical Record Club",
"BRCC	c	USDA-ARS Rhizobium Germplasm Resource Collection",
"BRCH	s	Botanical Research Center",
"BRE	s	Universite",
"BREE	s	Braintree and Bocking Natural History Club",
"BREG	s	Vorarlberger Naturschau",
"BREM	s	Uebersee-Museum",
"BRFL	s	City of Birmingham Reference Library",
"BRG	s	University of Guyana, Biology Department",
"BRGE	s	Laboratoire de Genetique des Plantes Superieures",
"BRH	s	Ministry of Natural Resources, Local Government, and the Environment",
"BRI	s	Brisbane Botanic Gardens Mt Coot-tha",
"BRIP	s	Department of Primary Industries",
"BRIST	s	University of Bristol, Botany Department",
"BRISTM	s	Bristol Museum and Art Gallery",
"BRIT	s	Botanical Research Institute of Texas",
"BRIU	s	University of Queensland, Botany Department",
"BRL	s	Bristol City Library",
"BRLU	s	Universite Libre de Bruxelles",
"BRM	s	Alfred-Wegener-Institut fuer Polar- und Meeresforschung",
"BRMI	s	Birmingham and Midland Institute",
"BRN	s	Sexey's School",
"BRNL	s	Mendel University of Agriculture and Forestry, Department of Forest Botany, Dendrology, and Typology",
"BRNM	s	Moravian Museum, Botany Department",
"BRNU	s	Masaryk University, Department of Botany",
"BROC	s	State University of New York, Biological Sciences Department",
"BRS	s	Agriculture and Agri-Food Canada",
"BRSL	s	Wroclaw University, Botany Department",
"BRSMG	s	Department of Geology",
"BRSN	s	University of Wales, Bangor Research Unit",
"BRTN	s	Brighton Natural History Society",
"BRU	s	Brown University",
"BRUN	s	Brunei Forestry Centre",
"BRV	s	Colecciones paleontologicas del Departamento de Geociencias de la Universidad Nacional de Colombia",
"BRVU	s	Vrije Universiteit Brussel",
"BRWK	s	Berwick-upon-Tweed Museum",
"BRY	s	Brigham Young University",
"BSA	s	Botanical Survey of India, Central Circle",
"BSB	s	Freie Universitaet Berlin, Institut fuer Biologie - Systematische Botanik und Pflanzengeographie",
"BSC	s	Centro Oriental de Ecosistemas y Biodiversidad",
"BSCA	s	Anza-Borrego Desert State Park",
"BSCVC	s	Bemidji State University, Vertebrate Collections",
"BSD	s	Botanical Survey of India, Northern Circle",
"BSE	s	Moyse's Hall Museum",
"BSHC	s	Botanical Survey of India, Sikkim Himalayan Circle",
"BSI	s	Botanical Survey of India, Western Circle, Ministry of Environment and Forests",
"BSIP	s	Ministry of Natural Resources, Department of Forests, Environment, and Conservation",
"BSIS	s	Botanical Survey of India, Industrial Section",
"BSJO	s	Botanical Survey of India, Arid Zone Circle",
"BSKU	s	Kochi University",
"BSL	s	Botanical Society of London",
"BSM	s	Berliner Staatisches Museum",
"BSMB	c	Bacteriology and Soil Microbiology Branch",
"BSMP	s	Department of Agriculture, Bureau of Science",
"BSN	s	Boston University, Biology Department",
"BSNH	s	Boston Society of Natural History",
"BSNS	s	Buffalo Society of Natural Sciences",
"BSPG	s	Bayerische Staatssammlung fuer Palaeontologie und Geologie",
"BSRA	s	University of Basrah, Biology Department",
"BSRM	s	Biological Station Reference Museum at Porto Novo",
"BST	s	Belfast Naturalists' Field Club",
"BSTN	s	Boothstown Botanical Society",
"BSU	s	Belgorod State University, Department of Botany",
"BSUH	s	Ball State University, Biology Department",
"BSUMC	s	Ball State University, Mammal Collection",
"BTCC<BULG>	c	Bulgarian Type Culture Collection",
"BTCC<INDO>	c	Biotechnology Culture Collection Institution Pusat Penelitian dan Pengembangan Bioteknologi-LIPI",
"BTH	s	Museum, Bath Royal Literary and Scientific Institution",
"BTJW	s	Bridger Teton National Forest",
"BTN	s	Booth Museum of Natural History",
"BTT	s	Burton-upon-Trent Natural History and Archaeological Society",
"BTU	s	Technische Universitaet Berlin",
"BUA	s	University of Baghdad, Plant Protection Department",
"BUAG	s	University of Agronomical Sciences and Veterinary Medicine, Botany and Plant Physiology Department",
"BUC	s	Universitatea din Bucuresti",
"BUCA	s	Institute of Biology, Romanian Academy",
"BUCF	s	Forest Research and Management Institute",
"BUCM	s	Institute of Biology, Romanian Academy",
"BUCSAV	c	Biologicky Ustav",
"BUE	s	University of Baghdad, Biology Department",
"BUEN	s	Proteccion de la Naturaleza",
"BUF	s	Buffalo Museum of Science",
"BUH	s	University of Baghdad, Biology Department",
"BUHGC	s	Barton-on-Humber Grammar School",
"BUHR	s	Baysgarth Museum",
"BUL	s	Natural History Museum of Zimbabwe",
"BULQ	s	Bishop's University",
"BULU	s	Uludag University, Biology Department",
"BUNH	s	University of Baghdad",
"BUNS	s	University of Novi Sad, Department of Biology and Ecology",
"BUPL	s	Bucknell University, Biology Department",
"BURD	s	University of Burdwan, Botany Department",
"BURP	s	Burpee Museum of Natural History",
"BUS	s	University of Miami, Biology Department",
"BUT	s	Butler University",
"BUTC	s	Boston University",
"BVC	s	Buena Vista College",
"BYBS	s	Bungay Botanical Society",
"BYDG	s	Technical-Agriculture Academy, Department of Botany and Ecology",
"BYU	s	Monte L. Bean Life Science Museum",
"BZ	s	Herbarium Bogoriense",
"BZF	s	Forest Research and Development Center and Nature Conservation",
"BZT	s	Biological Institute Titograd",
"C	s	Botanical Museum, University of Copenhagen",
"CA	s	Chicago Academy of Sciences",
"CABI	c	CABI Genetic Resource Collection",
"CACA	s	Carlsbad Caverns National Park",
"CACS	s	Chicago Academy of Sciences, Department of Biology",
"CAES	s	Connecticut Agriculture Experiment Station",
"CAF	s	Chinese Academy of Forestry",
"CAFB	s	Northern Forestry Centre, Canadian Forest Service",
"CAG	s	Universita di Cagliari",
"CAH	s	University of Zimbabwe, Biological Sciences Department",
"CAHS	s	Crispus Attucks High School",
"CAHUP	s	University of the Philippines Los Banos",
"CAI	s	Cairo University, Botany Department",
"CAIA	s	Ain Shams University, Department of Botany",
"CAIH	s	Desert Research Center, Mataria, Plant Ecology",
"CAIM<EGY>	s	Agricultural Research Center",
"CAIM<MEX>	c	Collection of Aquatic Important Microorganisms",
"CAIRC	s	National Research Centre, Plant Chemistry and Systematics Department",
"CAIRCC	c	CAIRCC",
"CAIRNS	s	c/o North Queensland Naturalists' Club",
"CAL	s	Botanical Survey of India",
"CALI	s	University of Calicut, Botany Department",
"CALP	s	University of the Philippines at Los Banos",
"CALU	c	Collection of Algae in Leningrad, St. Petersburg, State University",
"CAM<AUST>	s	Central Australian Museum",
"CAM<UK>	s	St. John's College",
"CAME	s	Universita di Camerino, Dipartimento de Botanica ed Ecologia",
"CAMU	s	Cameron University, Department of Biological Sciences",
"CAN	s	Canadian Museum of Nature, Vascular Plant Section",
"CANA	s	Canadian Museum of Nature, Phycology Section",
"CANB	s	Centre for Plant Biodiversity Research",
"CANI	s	Canisius College, Biology Department",
"CANL	s	Canadian Museum of Nature, Lichenology Section",
"CANM	s	Canadian Museum of Nature, Bryology Section",
"CANT	s	South China Agricultural University, Forestry Department",
"CANTY	s	Canterbury Museum",
"CANU	s	University of Canterbury, Department of Plant and Microbial Sciences",
"CAPM	c	Collection of Animal Pathogenic Microorganisms",
"CAR	s	Museo de Historia Natural La Salle, Departamento de Botanica",
"CARD	s	Caribbean Agricultural Research Institute",
"CARE	s	Caribbean Epidemiology Centre",
"CARL	s	Carleton College, Biology Department",
"CARM	s	Carmarthen County Museum",
"CARS	s	University of Surinam, Center for Agricultural Research",
"CART	s	Carthage College, Biology Department",
"CAS	s	California Academy of Sciences",
"CAS-IU	s	California Academy of Science, Indiana University Collection",
"CAS-SU	s	California Academy of Sciences, Stanford University Collection",
"CAS:Ent	s	California Academy of Sciences, Entomology collection",
"CAS:Herp	s	California Academy of Sciences, Herpetology collection",
"CAS:Ich	s	California Academy of Sciences, Ichthyology collection",
"CASM	s	Chicago Academy of Sciences, Museum of Natural History",
"CASMB	s	Centre of Advanced Study in Marine Biology",
"CASS	s	Chinese Academy of Sciences, Shenyang",
"CAT	s	Universita di Catania, Dipartimento di Botanica e Orto Botanico",
"CATIE	s	Tropical Agricultural Research and Training Center (CATIE), Plant Production Department",
"CAU	s	China Agricultural University",
"CAUP<COL>	s	Universidad del Cauca",
"CAUP<CZH>	c	Collection of Algae of Charles University, Prague",
"CAUSC	s	Universidade Federal de Santa Catarina Centro de Agrarias",
"CAVA	s	University of California at Berkeley",
"CAY	s	Institut de Recherche pour le Developpement (IRD)",
"CAYM	s	National Trust for the Cayman Islands",
"CB<AUST>	c	The CB Rhizobium Collection",
"CB<CZE>	s	Jihoceske Muzeum",
"CBD<ITA>	s	Collezione Ittiologica Balma-Delmastro",
"CBD<USA>	c	USF Center for Biological Defense",
"CBF	s	Coleccion Boliviana de Fauna",
"CBFS	s	University of South Bohemia, Department of Botany",
"CBG	s	Australian National Botanic Gardens",
"CBM	s	Natural History Museum and Institute",
"CBM:ZC	s	Natural History Museum and Institute, Zoological Collection",
"CBMAI	c	Brazilian Collection of Microorganisms from the Environment and Industry (Colecao Brasileira de Microrganismos de Ambiente e Industria)",
"CBNM	s	Cedar Breaks National Monument",
"CBS	c	Centraalbureau voor Schimmelcultures, Fungal and Yeast Collection",
"CBSIZA	s	Caspian Biological Station Institute of Zoology",
"CBTCCCAS	c	The Cell Bank of Type Culture Collection of Chinese Academy of Sciences",
"CBU<CAN>	s	Cape Breton University",
"CBU<SKOR>	s	Chungbuk National University, School of Life Science",
"CBY	s	Royal Museum and Art Gallery",
"CC	c	CSIRO Canberra Rhizobium Collection",
"CCAC<BRZL>	s	Universidade Federal do Ceara, Centro Ciencias Agrarias",
"CCAC<GRM>	c	Culture Collection of Algae at the University of Cologne",
"CCALA	c	Culture Collection of Autotrophic Organisms",
"CCAP	c	Culture Collection of Algae and Protozoa",
"CCARM	c	Culture Collection of Antimirobial Resistant Microorganisms",
"CCAU	s	Central China Agricultural University",
"CCB<BRZ>	c	Colecao de Culturas de Basidiomicetos",
"CCB<IND>	s	Central College",
"CCBAS	c	Culture Collection of Basidiomycetes",
"CCBAU	c	Culture Collection, Beijing Agricultural University",
"CCCC	s	Carthage College",
"CCCIEB	c	Culture Collections of Microorgansisms of Center of Genetic Engineering and Biotechnology",
"CCCM	c	Canadian Center for the Culture of Microorganisms",
"CCCZ	s	University of Malawi",
"CCDM<CHN>	c	Culture Collection of Department of Microbiology",
"CCDM<CZH>	c	Culture Collection of Dairy Microorganisms Laktoflora",
"CCDMBI	c	Culture Collection, Department of Microbiology",
"CCEB	c	Culture Collection of Entomogenous Bacteria",
"CCEC	s	Museum, Centre de Conservation et d'Etude des Collections",
"CCF<CUB>	c	Colleccion de Cuttivos Finlay",
"CCF<CZE>	c	Culture Collection of Fungi",
"CCFC	c	Canadian Collection of Fungal Cultures",
"CCFHE	s	Cornwall College of Further and Higher Education, Natural Sciences Department",
"CCFL	s	Chad National Museum",
"CCFVB	c	Facultat de Veterinaria, Universitat Autonoma de Barcelona",
"CCG<CHN>	s	Chengdu College of Geology",
"CCG<GHN>	s	University of Cape Coast, Botany Department",
"CCGB	c	Cole(e7)o de Culturas do G(ea)ero Bacillus e G(ea)eros Correlatos",
"CCGVCC	c	China Centre for General Viruses Culture Collection",
"CCIAL	c	Cultures Cells for Institute Adolfo Lutz",
"CCIBSO	c	Culture Collection IBSO",
"CCIM	c	Culture Collection of Industrial Microorganisms",
"CCM-A	c	Coleccion de Cultivos Microbianos",
"CCM<CHN>	s	Changchun College of Traditional Chinese Medicine, Department of Chinese Materia Medica",
"CCM<CZE>	c	Czech Collection of Microorganisms",
"CCM<USA-MT>	s	Carter County Museum",
"CCMAC	c	Culture Collection of Macromycetes (Basidiomycotina and Ascomycotina)",
"CCMCU	c	Culture Collection of Microorganisms",
"CCMF	c	University of Portsmouth",
"CCMGE	s	Chernyshev Central Museum of Geological Explorations,Collections of the Department of Herpetology, Zoological Institute of the Russian Academy of Sciences",
"CCMI	c	Culture Collection of Industrial Microorganisms",
"CCML	s	Coleccion Ictiologica del Departamento de Ciencias Marinas de la Universidad de la Laguna",
"CCMM	c	Moroccan Coordinated Collections of Microorganisms",
"CCMP	c	Provasoli-Guillard National Center for Culture of Marine Phytoplankton",
"CCNH	s	Central Michigan University, Center for Cultural and Natural History",
"CCNL	s	Connecticut College, Botany Department",
"CCNP	s	Carlsbad, Carlsbad Caverns National Park",
"CCNU	s	Central China Normal University, Biology Department",
"CCO	s	Carleton University, Biology Department",
"CCR	s	Chichester District Museum",
"CCRI	c	Collection du Centre de Recherche en Infectiologie",
"CCSIIA	c	Culture Collection of Sichuan Industrial Institute Antibiotics",
"CCSRL	s	Centro Studi e Ricerche Ligabue",
"CCSU	s	Central Connecticut State University, Biological Sciences Department",
"CCT	c	Colecao de Culturas Tropical",
"CCTCC	c	China Center for Type Culture Collection",
"CCTM	c	Centre de Collection de Type Microbien, Institut de Microbiologie, Universite de Lausanne",
"CCTR	c	Culture Collection Trutnov",
"CCUF	s	Universidade Federal de Alagoas, Centro de Ciencias Biologicas",
"CCUG	c	Culture Collection, University of Goteborg, Department of Clinical Bacteriology",
"CCVC	s	Centenary College, Vertebrate Collection",
"CCVCC	c	China Center For Virus Culture Collection",
"CCW	s	Casper College",
"CCY<NLD>	c	Culture Collection Yerseke, Department of Marine Microbiology",
"CCY<SVK>	c	Culture Collection of Yeasts, Slovak Academy of Sciences, Institute of Chemistry",
"CDA<CAN>	c	Canadian Department of Agriculture",
"CDA<USA-CA>	s	California Department of Food and Agriculture",
"CDBB	c	Coleccion Nacional de Cepas Microbianas y Cultivos Celulares",
"CDBI	s	Chengdu Institute of Biology",
"CDC<CHN>	s	Changdu Institute for Drug Control",
"CDC<USA-GA>	c	Centers for Disease Control and Prevention",
"CDCM	s	Chengdu College of Traditional Chinese Medicine",
"CDFM	s	Cardiff Museum",
"CDFN	s	Canadian Forest Service - Atlantic",
"CDN	s	Whitgift School",
"CDRI	s	Central Drug Research Institute",
"CDRS	s	Invertebrate Collection",
"CDS	s	Charles Darwin Research Station, Botany Department",
"CEAM	s	Centro de Entomologica y Acarologia",
"CEB	s	Tadulako University",
"CEBU	s	University of San Carlos, Biology Department",
"CECT	c	Coleccion Espanola de Cultivos Tipo",
"CEDD	s	International Center for Ethnomedicine and Drug Development",
"CEEF	s	Escuela Nacional de Ciencias Forestales",
"CEET	s	El Colegio de la Frontera Sur, Colleccion de Insectos Asociados a Plantas Cultivadas en la Frontera Sur",
"CEL	s	University of Illinois, Crop Sciences Department",
"CELM	s	Coleccion Entomologica \"Luis Maria Murillo\"",
"CEMBP	s	Centre of Excellence in Marine Biology",
"CEN	s	EMBRAPA Recursos Geneticos e Biotecnologia - CENARGEN",
"CENA<BRZ>	s	Centro de Energia Nuclear na Agricultura, Universidade de Sao Paulo",
"CENA<NIC>	s	Centro Nacional de Proteccion Vegetal",
"CENACUMI	c	Centro Nacional de Cultivos Microbianos (National Center For Microbial Cultures)",
"CENG	s	Centro Experimental de Nueva Guinea",
"CEPEC	s	CEPEC, CEPLAC",
"CEPH	b	Foundation Jean Dausset (CEPH)",
"CEPIM	c	Centro per gli Enterobatteri Patogeni per l'Italia Meridionale",
"CEPM	c	CEPM- Centre d'Etudes sur le Polymorphisme des Micro-organismes",
"CERL/BIC	s	United States Army, Biological Inventory Collection",
"CERN	s	University",
"CESJ	s	Universidade Federal de Juiz de Fora, Departamento de Botanica",
"CESK	s	Muzeum Teainska",
"CEST	s	Central Experiment Station",
"CET	s	Centro de Estudios Tropicales",
"CETESB	c	Setor de Pesquisa Tecnologica de Sistemas de Tratamento de Efluentes Domesticos",
"CEU	s	Collage of Eastern Utah",
"CFB	s	Northern Forestry Centre, Canadian Forest Service",
"CFBP	c	Collection Francaise des Bacteries Phytopathogenes",
"CFMR	s	Forest Products Laboratory",
"CFN	s	Clifton College, Biology Department",
"CFNL	s	Universidad Autonoma de Nuevo Leon",
"CFQ	c	Cepario de la Facultad de Quimica",
"CFRB	s	Chinese Academy of Forestry, Forest Research Institute",
"CFS	s	Canadian Forest Service, Pacific Forest Research Centre",
"CFSHB	s	North Coast Regional Botanic Gardens",
"CFUA	s	Universidad Austral de Chile",
"CG	c	Embrapa Genetic Resources and Biotechnology Collection of Fungi of Interest to Biological Control",
"CGC	b	Caenorhabditis Genetics Center",
"CGE	s	University of Cambridge, Department of Plant Sciences",
"CGEC	s	China Entomological Research Institute",
"CGG	s	Cambridge University Botanic Garden",
"CGH	s	National Museum of Prague",
"CGMCC	c	China General Microbiological Culture Collectio Center, Chinese Academy of Sciences",
"CGMS	s	Universidade Federal de Mato Grosso do Sul, Departamento de Biologia",
"CGSC	c	E. coli Genetic Stock Center",
"CH-AG	c	Collection de Recherche",
"CHA	s	Hebei Agrotechnical Teachers College",
"CHAB	s	Far East Forestry Research Institute",
"CHAF	s	Chaffey College, Biology Department",
"CHAM	s	I.N.T.A., E.E.A. La Rioja",
"CHAP	s	Universidad Autonoma Chapingo",
"CHAPA	s	Colegio de Postgraduados, Botanica, IRENAT",
"CHARL	s	Charleston Museum",
"CHAS<USA-IL>	s	Chicago Academy of Sciences",
"CHAS<USA-SC>	s	Southern Research Station",
"CHBG	s	Christchurch Botanic Gardens",
"CHE	s	Societe Nationale des Sciences Naturelles et Mathematiques de Cherbourg",
"CHEB	s	Regional Museum Cheb",
"CHEL	s	Chelsea Physic Garden",
"CHELB	s	Cheltenham College for Boys",
"CHEP	s	Escuela Superior Politecnica del Chimborazo",
"CHER	s	Yu. Fedcovich Chernivtsi State University, Botany Department",
"CHFD	s	Chelmsford and Essex Museum",
"CHI	s	University of Illinois, Biological Sciences Department",
"CHIA	s	National Chiayi Agricultural College, Forestry Department",
"CHIC	s	Chicago Botanic Garden, Research Department",
"CHINM	s	Instituto Nacional de Microbiologia",
"CHIP	s	Instituto de Historia Natural, Departamento de Botanica",
"CHIS	s	Academy of Sciences of Moldova",
"CHISA	s	University of Agriculture",
"CHL	s	Cheltenham Grammar School",
"CHM<UK>	s	Cheltenham Art Gallery and Museum",
"CHM<USA-SC>	s	Charleston Museum",
"CHNCB	s	Centre d'Historia Natural de la Conca de Barbera",
"CHOCO	s	Universidad Tecnologica del Choco",
"CHOM	s	Okresni muzeum Chomutov",
"CHPU	s	Chelyabinsk State Pedagogical University, Botany Department",
"CHR	s	Landcare Research New Zealand Limited",
"CHRB	s	Rutgers University - Cook College",
"CHRG	s	Grosvenor Museum",
"CHSC	s	California State University, Biological Sciences Department",
"CHT	s	Cheltenham College",
"CHUG	s	Garyounis University, Botany Department",
"CHULA	c	Microbiology Department Faculty of Science",
"CHUNB	s	University of Brasilia Herpetological Collection",
"CHUR	s	Buendner Natur-Museum",
"CI	s	Carnegie Institution of Washington, Plant Biology Department",
"CIAN<MEX-AG>	s	Instituto Nacional de Investigaciones Forestales, Agricolas y Pecuarias (INIFAP)",
"CIAN<MEX-CO>	s	Centro de Investigaciones Agricolas Nortoeste",
"CIAT	b	Centro Internacional de Agricultura Tropical (International Center for Tropical Agriculture)",
"CIAT:Bean	b	Centro Internacional de Agricultura Tropical (International Center for Tropical Agriculture), CIAT Bean Collection",
"CIAT:Cassava	b	Centro Internacional de Agricultura Tropical (International Center for Tropical Agriculture), CIAT Cassava Collection",
"CIAT:Forage	b	Centro Internacional de Agricultura Tropical (International Center for Tropical Agriculture), CIAT Forages Collection",
"CIAT:Rhizobium	c	Centro Internacional de Agricultura Tropical (International Center for Tropical Agriculture), CIAT Rhizobium Collection",
"CIB<CHN>	s	Chengdu Institute of Biology",
"CIB<MEX-LaPaz>	s	Centro de Investigaciones Biologicas del Noroeste, S.C. (Mexico)",
"CIB<MEX-Veracruz>	s	Universidad Veracruzana",
"CIBC	s	International Institute of Biological Control",
"CIBM	s	Centro Invest. Biol. Noroeste",
"CIC	s	Albertson College of Idaho, Biology Department",
"CICC	c	China Center for Industrial Culture Collection",
"CICESE	s	Centro de Investigacion Cientifica y de Educacion Superior de Ensenada",
"CICIM	c	Culture and Information Centre of Industrial Microorganisms of China's Univeristies",
"CICIMAR	s	Centro Interdisciplinario de Ciencias Marinas",
"CICV	c	Centro de Investigaciones en Ciencias Veterinarias",
"CICY	s	Centro de Investigacion Cientifica de Yucatan, A.C. (CICY)",
"CIDA	s	Albertson College, Museum of Natural History",
"CIECRO	s	County Record Office, Cambridgeshire",
"CIES	c	Centro de Investigacion, Experimentacion y Servicios del  Champinon",
"CIIDIR	s	Instituto Politecnico Nacional",
"CIJC	s	Muzeului Judetean Covasna, Collection of Insects",
"CIMAP	s	Central Institute of Medicinal and Aromatic Plants",
"CIMAR	s	Universidad Catolica de Valparaiso, Centro de Investigaciones del Mar",
"CIMI	s	Centro Interdisciplinario de Investigacion para el Desarrollo Integral Regional (CIIDIR) IPN-Michoacan",
"CIMNH	s	Albertson College of Idaho, Orma J. Smith Museum of Natural History",
"CIMSC	c	Collezione Instituto di Microbiologia",
"CINC	s	University of Cincinnati, Biological Sciences Department",
"CIP<COL>	s	Centro de Investigaciones Pesqueras",
"CIP<FRA>	c	Pasteur Institute Collection, Biological Resource Center of Pasteur Institute (CRBIP)",
"CIP<PER>	b	Centro Internacional de las Papas",
"CIPDE	c	Collection of Insect Pathogens, Dept. of Entomology",
"CIPT	c	Collection Institut Pasteur Tuberculose",
"CIQR	s	El Colegio de la Frontera Sur",
"CIR	s	Royal Agricultural College",
"CIRUV	s	Coleccion Ictiologica de Referencia de la Universidad del Valle",
"CIS<CHN>	s	Academia Sinica and State Planning Commission",
"CIS<USA-CA>	s	California Insect Survey",
"CIS<USA-MI>	s	Cranbrook Institute of Science",
"CISM<MEX>	c	Verticillium dahliae from cotton",
"CISM<THAI>	c	NifTAL Rhizobium Collection (Asia Center)",
"CIT	s	Citrus Research Institute",
"CITA	s	The Citadel, Biology Department",
"CIUC	s	Centro Interdipartimentale dell'Universita Museo di Storia Naturale e del Territorio",
"CIZ	s	Centro de Investigaciones Zoologicas",
"CKE	s	Calke Abbey",
"CL	s	Universitatis Napocensis",
"CLA	s	Universitatea de Stiinte Agricole si Medicina Veterinara",
"CLCB	s	Laboratorul de Ecologie",
"CLCC	s	Augustana University College",
"CLD	s	Cleveland Literary and Philosophical Society",
"CLE	s	Tullie House Museum",
"CLEMS	s	Clemson University, Biological Sciences Department",
"CLEV	s	Cleveland Museum of Natural History",
"CLEY	s	Coastal Ecology Research Station",
"CLF	s	Institut des Universitaires et Musee Lecoq",
"CLI	s	Literary and Philosophical Institution of Chatham",
"CLIB	c	Collection de Levures d'Interet Biotechnologique Collection of Yeasts of Biotechnological Interest",
"CLM	s	Cleveland Museum of Natural History, Botany Department",
"CLMP	s	Department of Conservation & Land Management",
"CLNP	s	Crater Lake National Park",
"CLOE	s	Clitheroe Castle Museum",
"CLP	s	Forest Products Research and Development Institute, Department of Science and Technology",
"CLR	s	All Saint's Church, Colchester Borough Council",
"CLU	s	Universita della Calabria",
"CM	s	Carnegie Museum of Natural History",
"CM-IEA	s	Universidad Autonoma de Tamaulipas (Mexico)",
"CM-UMSNH	s	Universidad de Michoacan (Mexico)",
"CM:M	s	Carnegie Museum of Natural History, Section of Mammals",
"CM:O	s	Carnegie Museum of Natural History, Section of Birds",
"CM<CHN>	s	Chongqing Museum",
"CMA	s	Crayford Manor House Adult Education Centre",
"CMBGCAS	c	Collection of Marine Biological Germplasm",
"CMBK	s	The City Museum and Art Gallery, Department of Natural History",
"CMBY	s	Camberley Museum",
"CMC<NZ>	s	Canterbury Museum",
"CMC<USA-MI>	s	Central Michigan University, Department of Biology",
"CMCC	c	National Center for Medical Culture Collections",
"CMCNA	s	Museo de Ciencias Naturales y Antropologicas \"Prof. A. Serrano\"",
"CMDM-PUJ	c	Coleccion Microorganismos Departamento Microbiologia",
"CMEI	s	Clements' Museum of Exotic Insects",
"CMFRI	s	See FMRI",
"CMGP	s	Central Museum of Geological Prospecting",
"CMIZASDPRK	s	Custody Museum",
"CMKKU	c	Clinical Diagnostic Microbiology Srinagarind Hospital, Faculty of Medicine",
"CML	s	Universidad Nacional de Tucuman, Coleccion de Mamiferos Lillo (Argentina)",
"CMM	s	Bradford Art Galleries and Museums, Natural Sciences Department",
"CMMC	c	China Marine Microbe Collection",
"CMMEX	s	Universidad Autonoma de Baja California",
"CMMI	s	Chinese Academy of Traditional Medicine",
"CMML	s	Colorado State University",
"CMN	s	Canadian Museum of Nature",
"CMNAR	s	Canadian Museum of Nature, Amphibian and Reptile Collection",
"CMNC	s	Canadian Museum of Nature, Neotropical Cerambycidae Collection",
"CMNFI	s	Canadian Museum of Nature, Fish Collection",
"CMNH	s	The Cleveland Museum of Natural History",
"CMNH<USA-IL>	s	Chicago Museum of Natural History",
"CMNS	s	Museum of Natural History",
"CMNZ	s	Canterbury Museum",
"CMSK	s	City Museum",
"CMSU	s	Central Missouri State University",
"CMU	s	Chiang Mai University",
"CMUT	s	Chiang Mai University",
"CMV	s	Centre Marie-Victorin",
"CMW	c	Tree Pathology Cooperative Program",
"CMY	s	R. G. Kar Medical College, Botany Department",
"CN<FR>	s	Universite de Caen",
"CN<UK>	c	Wellcome Collection of Bacteria, Burroughs Wellcome Research Laboratories",
"CNC	s	Canadian National Collection of Insects, Arachnids, and Nematodes",
"CNCI	s	Canadian National Collection Insects",
"CNCM	c	Collection Nationale de Cultures de Microorganismes",
"CNCTC	c	Czech National Collection of Type Cultures",
"CNE	s	Victoria Jubilee Museum",
"CNEN-LABPC	c	Laboratorio de Pocos de Caldas",
"CNF	s	Croatian Mycological Society",
"CNHM<CRO>	s	Croatian Natural History Museum, Botany Department",
"CNHM<USA-OH>	s	Cincinnati Museum of Natural History",
"CNHP	s	Beijing Natural History Museum",
"CNHS	s	Croydon Natural History and Scientific Society",
"CNIN	s	Colecci'on Nacional de Insectos, Universidad Nacional Aut'onoma de M'exico",
"CNM	s	Cheltenham Naturalists' Association",
"CNMC	s	Colorado National Monument",
"CNMS	s	National Museum",
"CNPS	s	Centro Nacional de Pesquisas da Soja",
"CNRO	s	Centre National de Recherches Oceanographiques",
"CNRS	s	Centre National de la Recherche Scientifique",
"CNRZ	c	Centre National de Recherches Zootechniques",
"CNU	s	Chonbuk National University",
"CNWGRGL	b	Chinese National Waterfowl Germplasm Resources Gene Library",
"CO	s	Museum National d'Histoire Naturelle, Department of Marine Biology",
"COA	s	Universidad de Cordoba, Departamento de Ciencias y Recursos Agricolas y Forestales",
"COAH	s	Instituto Amazonico de Investigaciones Cientificas SINCHI",
"COCA	s	Comision Tecnico Consultiva de Coeficientes de Agostadero (COTECOCA)",
"COCH	s	Universidad Mayor de San Simon, Departamento de Botanica",
"COCO	s	Colorado College, Biology Department",
"CODAGEM	s	Universidad Autonoma del Estado de Mexico",
"COFC	s	Universidad de Cordoba, Departamento de Biologia Vegetal",
"COI	s	University of Coimbra",
"COL	s	Universidad Nacional de Colombia",
"COLG	s	Columbus State University, Biology Department",
"COLM	s	Colorado National Monument",
"COLO	s	University of Colorado",
"COLOM	s	Colorado State Museum",
"COM	s	Colombo Museum",
"CON	s	Bristol, Clifton and West of England Zoological Society's Gardens",
"CONC	s	Universidad de Concepcion, Departamento de Botanica",
"CONN	s	University of Connecticut, Department of Ecology and Evolutionary Biology",
"CONV	s	Converse College, Biology Department",
"COR	s	Universidade Federal de Mato Grosso do Sul, Departamento de Ciencias do Ambiente",
"CORB	s	Corchester School",
"CORD	s	Universidad Nacional de Cordoba, Facultad de Ciencias Exactas, Fisicas y Naturales",
"CORO	s	IUTAG, Departamento de Investigacion",
"CORT	s	State University of New York College at Cortland, Biological Sciences Department",
"CORU	s	Universidad Veracruzana, Campus Cordoba",
"COV	s	Herbert Art Gallery and Museum",
"COVY	s	Coventry and District Natural History Society",
"CP	s	Royal Veterinary and Agricultural University, Plant Biology Department",
"CPAC	c	Centro de Pesquisas Agropecuarias do Cerrado",
"CPAP<BRA-Belem>	s	Centro de Pesquisas Agropecuarias do Tropico Umido",
"CPAP<BRA-Corumba>	s	EMBRAPA",
"CPB	s	National Institute for the Control of Pharmaceutical and Biological Products",
"CPC	s	Commonwealth Palaeontological Collections",
"CPDC	s	Centro de Pesquisas do Cacau",
"CPF	s	KwaZulu-Natal Nature Conservation Service",
"CPH	s	University of the Pacific, Biological Sciences Department",
"CPHS	c	WHO/FAO/OIE Collaborating Centre for Reference and Research on Leptospirosis, Western Pacific Region",
"CPM	s	Christoffel Park Museum",
"CPMM	s	Dr. Alvaro de Castro Provincial Museum",
"CPNP	s	Cuc Phuong National Park",
"CPPIPP	c	Collection of Plant Pathogens",
"CPPLIP	s	Centro de Pesquisas Paleontologias Llewellyn Ivor Price",
"CPRC	s	University of Puerto Rico, Caribbean Primate Research Center Museum",
"CPRR	c	Laboratorio de Doenca de Chagas",
"CPS<USA-WA>	s	University of Puget Sound, Slater Museum of Natural History",
"CPS<USA-WY/CO>	s	Wyoming-Colorado Paleontological Society",
"CPSC	s	University of Puget Sound",
"CPSU	s	California Polytechnic State University, San Luis Obispo",
"CPU	s	China Pharmaceutical University",
"CPUN	s	Universidad Nacional de Cajamarca, Departamento de Biologia",
"CPUP	s	California Polytechnic University",
"CPZ	c	Centro Panamericano de Zoonosis",
"CQBG	s	Chongqing Botanical Garden",
"CQNM	s	Chongqing Natural History Museum",
"CR	s	Museo Nacional de Costa Rica",
"CRA-OLI	b	Centro di Ricerca per l'Olivicoltura e l'Industria Olearia",
"CRAF	s	University of Craiova, Phytopathology Department",
"CRAI	s	University of Craiova",
"CRBF	c	Collection de genomes d'organismes symbiotiques",
"CRBK	s	Cranbrook School",
"CRBY	s	Crosby Library",
"CRCA	s	Instituto dos Cereais",
"CRCM	s	Washington State University, Charles R. Conner Museum",
"CRCM:Bird		Washington State University, Charles R. Conner Museum, bird collection",
"CRD	s	Instituto Politecnico Nacional, Coleccion Cientifica de Fauna Silvestre (Mexico)",
"CRE<CRI>	s	Costa Rica Expeditions",
"CRE<USA-CA>	s	University of Southern California",
"CREG	s	Instituto Tecnologico Agropecuario de Jalisco",
"CRGF	c	Collection de Recursos Geneticos Fungicos, Instituto de Ecologia y Systematica",
"CRH	s	Centre de Recherche en Hydrobiologie",
"CRI	s	Universidade do Extremo Sul Catarinense, Bairro Universitario",
"CRK	s	University College, Plant Science Department",
"CRL	c	Centro de Referencia Para Lactobacilos",
"CRLA	s	Crater Lake National Park, Museum and Archives Collections",
"CRMC	s	College of the Redwoods, Mendocino Coast Campus, Biological Sciences Department",
"CRMM	s	Centre de Recherche sur les Mammiferes Marins",
"CRO	s	Wellington College",
"CRP	s	I.N.T.A., E.E.A. Bariloche",
"CRRHA	s	Centre Regional de Recherches en Hydrobiologie Appliquee",
"CS<AUS>	c	CSIRO Collection of Living Micro-algae",
"CS<FRA>	s	Musee des Dinosaures d'Esperaza (Aude)",
"CS<USA-CO>	s	Colorado State University, Biology Department",
"CSAT	s	Colegio de Postgraduados, Campus Tabasco",
"CSAU	s	National Agrarian University, Southern Branch \"Crimean Agrotechnological University\", Department of Botany, Plant Physiology and Genetics",
"CSB	s	St. John's University/College of Saint Benedict, Biology Department",
"CSC	s	Colegio del Sagrado Corazon",
"CSC-CLCH	c	Centro Substrati Cellulari, Cell Lines Collection and Hybridomas",
"CSCA	s	California State Collection of Arthropods",
"CSCC	s	Chadron State College",
"CSCCV	s	Chadron State College, Collection of Vertebrates",
"CSCN	s	Chadron State College",
"CSCS<PHL>	s	Cebu State College of Science and Technology, Agricultural Biology Laboratory",
"CSCS<USA-CA>	s	California State University, Turlock",
"CSDS	s	Desert Studies Center",
"CSFI	s	Central-South Forestry University",
"CSGP	s	Servicos Geologicos de Portugal",
"CSGT	s	Collegio San Giuseppe",
"CSIR	c	Council for Scientific and Industrial Research",
"CSIRO	s	Commonwealth Science & Industrial Research Organization",
"CSLA	s	California State University, Department of Biological Sciences",
"CSLB	s	California State University at Long Beach",
"CSMA	c	Centro di Studio dei Microorganismi Autotrofi - CNR",
"CSPM	s	Colegio Lasalle Palma de Mallorca",
"CSPU	s	California State Polytechnic University, Biological Sciences Department",
"CSPUP	s	California State Polytechnic University, Pomona",
"CSR	s	Caucasus State Nature Biosphere Reserve",
"CSTIU	s	Faculty of Science, University of Tokyo",
"CSU<USA-CO>	s	Colorado State University",
"CSU<USA-OK>	s	University of Central Oklahoma, Biology Department",
"CSUC	s	California State University, Chico, Vertebrate Museum",
"CSUF	s	California State University, Fresno",
"CSULB	s	California State University, Long Beach",
"CSUN	s	California State University, Northridge",
"CSUR	c	Collection de Souches de l'Unite des Rickettsies",
"CSUTC	s	Colorado State University, Mammalogy Teaching Collection",
"CSVFC	s	Caradoc and Severn Valley Field Club",
"CT	s	University of Cape Town, Botany Department",
"CTES	s	Instituto de Botanica del Nordeste",
"CTESN	s	Universidad Nacional del Nordeste",
"CTN	s	Free Library and Museum",
"CTS	s	Chongqing Teachers College",
"CTY	s	Canterbury Literary and Philosophical Institution",
"CU	s	Cornell University",
"CUAC	s	Clemson University",
"CUB	s	Chulalongkorn University",
"CUBK	s	Department of Biology, Chonbuk National University",
"CUC	c	Cepario de la Universidad de Concepcion de Chile",
"CUE	s	Cairo University",
"CUETM	c	Collection Unite Ecotoxicologie Microbienne, INSERM",
"CUFH	s	Cumhuriyet University, Biology Department",
"CUG	s	Collection Universite Poitiers",
"CUH	s	Calcutta University, Botany Department",
"CUHK	c	Biology Department, Chinese University of Hong Kong",
"CUI	s	Central College",
"CUIC	s	Cornell University, Invertebrate Collections",
"CUMV	s	Cornell University Museum of Vertebrates",
"CUMV:Herpetology	s	Cornell University Museum of Vertebrates, Herpetology Collection",
"CUMV:Ichthyology	s	Cornell University Museum of Vertebrates, Ichthyology Collection",
"CUMV:Mammalogy	s	Cornell University Museum of Vertebrates, Mammalogy Collection",
"CUMV:Ornithology	s	Cornell University Museum of Vertebrates, Ornithology Collection",
"CUMZ<CAN>	s	Carleton University, Museum of Zoology",
"CUMZ<CMR>	s	Cameroon University, Museum of Zoology",
"CUMZ<UK>	s	Cambridge University, Museum of Zoology",
"CUNRC	s	Universidad Nacional de Rio Cuarto, Coleccion de Mamiferos (Argentina)",
"CUP<CHN>	s	Catholic University of Peking",
"CUP<CZE>	s	Charles University",
"CUP<USA-NY>	s	Cornell University, Plant Pathology Department",
"CUS	s	Cusino Wildlife Research Station, Natural Resources Department",
"CUSC	s	Clemson University, Vertebrate Collections",
"CUVC<COL>	s	Universidad del Valle, Departamento de Biologia",
"CUW	s	Clark University, Biology Department",
"CUWM	s	Clark University",
"CUZ	s	Universidad Nacional San Antonio Abad del Cusco",
"CV	s	Municipal Museum of Chungking",
"CVCC<CHN-1>	c	Center for Veterinary Culture Collection",
"CVCC<CHN-2>	c	China Veterinary Culture Collection",
"CVCM	c	Centro Venezolano de Colecciones de Microorganismos",
"CVCW	s	Clinch Valley College, University of Virginia, Biology Department",
"CVM	s	City Museum, Natural History Department",
"CVRD	s	Reserva Natural da Vale do Rio Doce",
"CVUL	s	Universite Laval, Collection de Vertebres",
"CVULA	s	Coleccion Vertebrados, Facultad de Ciencias, La Hechicera, Universidad de los Andes",
"CWB	s	Kharkov State University",
"CWC	s	Central Wyoming College",
"CWDR	s	Cawdor Castle",
"CWU	s	V. N. Karasin National University",
"CY	c	Centre des Yersinia",
"CYN	s	Chipstead Valley Primary School",
"CYP	s	Ministry of Agriculture, Natural Resources and Environment, Forestry Department",
"CZAA	s	Catedra de Zoologia Agricola",
"CZACC	s	Coleccion Zoologia, Academia de Ciencias de Cuba",
"CZIP	s	Universidad de Magallanes, Instituto de la Patagonia (Chile)",
"CZL	s	Centro de Zoologia",
"CZUAA	s	Universidad Autonoma de Aguascalientes (Mexico)",
"CZUG	s	Universidad de Guadalajara,Centro de Estudios en Zoologia, Entomologia",
"DABUH	s	University of Helsinki, Department of Applied Biology",
"DABZ	s	Department of Agriculture",
"DACB	s	Bangladesh National Herbarium",
"DACL	s	London Research Centre",
"DACT	c	Dept. Agricult. Chem. Technol.",
"DAFH	s	Department of Agriculture and Fisheries",
"DAKAR	s	Universite Cheikh Anta Diop, Departement de Biologie Vegetale",
"DAL	s	Dalhousie University, Biology Department",
"DANV	s	Umweltamt Darmstadt",
"DAO	s	Agriculture and Agri-Food Canada",
"DAOM	c	Plant Research Institute, National Mycological Herbarium",
"DAR	c	Plant Pathology Herbarium",
"DARI	s	Insect Collection, New South Wales Department of Agriculture",
"DAS	s	Agriculture and Agri-Food Canada",
"DASF	s	Department of Agriculture, Stock and Fisheries",
"DAV	s	University of California, Plant Biology",
"DAVFP	s	Pacific Forestry Centre, Canadian Forest Service",
"DAVH	s	University of California, Environmental Horticulture Department",
"DBAI	s	Instituto de Ciencias Biologicas",
"DBAU	s	Universidade Santa Ursula",
"DBC	s	University College, Botany Department",
"DBCUCH	s	Universidad de Chile, Departamento de Biologia Celular y Genetica",
"DBFFEUCS	s	Departamento de Biologia de la Faculdad de Filosofia y Educacion de la Universidad de Chile",
"DBG	s	Denver Botanic Gardens",
"DBKKU1	c	Department of Biology, Faculty of Science",
"DBKKU2	c	Department of Biology, Faculty of Science",
"DBKKU3	c	Department of Biology, Faculty of Science",
"DBM	c	Department of Biochemistry and Microbiology",
"DBMU	c	Department of Biotechnology, Faculty of Science, Mahidol University",
"DBN	s	National Botanic Gardens",
"DBS	c	Department of Biological Culture Collection",
"DBSE	s	Universidade Federale Sergipe",
"DBSNU	s	Department of Biology, Shaanxi Normal University",
"DBUM-IPT	c	Department of Biochemistry, Faculty of Medicine, University of Malaya",
"DBUP	c	Algal Culture Collection",
"DBV	c	Division of Standardisation",
"DBVPG	c	Industrial Yeasts Collection",
"DBY	s	City of Derby Museum and Art Gallery",
"DCBU	s	Universidade Federal de Sao Carlos",
"DCDS	s	Dipartimento di Coltivazione e Difesa delle Specie Legnose dell'Universita, Sezione Entomologia Agraria",
"DCH	s	Davidson College, Biology Department",
"DCMB	s	Universidade do Amazonas",
"DCMD	s	Derby City Museum and Art Gallery",
"DCMP	s	Universidade Federal do Parana",
"DCN-UNRC	s	Departamento de Ciencias Naturales, Universidad Nacional de Rio Cuarto",
"DCPC	s	DominicusCirillus[deceased]",
"DCR	s	Doncaster Museum and Art Gallery",
"DD	s	Forest Research Institute, Indian Council of Forestry Research and Education, Systematic Botany Discipline",
"DDFF	s	Departamento de Defensa Fitossanitarista",
"DE	s	Debrecen University, Botany Department",
"DE-CSIRO	c	CSIRO Insect Pathogen Culture Collection",
"DEBU	s	Ontario Insect Collection, University of Guelph",
"DECA	s	Agnes Scott College, Biology Department",
"DECV	s	Douglas Ecological Consultants",
"DEE	s	McManus Galleries, Natural History Department",
"DEES	s	Universidade de Sao Paulo, Piracicaba",
"DEFS	s	Universidade de Sao Paulo",
"DEI	s	Deutsches Entomologisches Institut im ZALF",
"DEIB	s	Deutsches Entomologisches Institut",
"DEK	s	Northern Illinois University, Biological Sciences Department",
"DELS	s	University of Delaware, Plant Science Department",
"DELTA	s	Delta Waterfowl and Wetlands Research Station",
"DEN	s	Denison University, Biology Department",
"DENA	s	Watt Institute",
"DENF	s	Grand Mesa-Uncompahgre-Gunnison Natonal Forests",
"DENH	s	University of New Hampshire",
"DERM	s	Intermountain Experiment Station",
"DES	s	Desert Botanical Garden, Research Department",
"DEVA	s	Death Valley National Park",
"DEWV	s	Davis and Elkins College, Biology and Environmental Science Department",
"DEZA	s	Dipartimento di Entomologia e Zoologia Agraria dell'Universita",
"DEZC	s	Dipartimento di Entomologia e Zoologia Applicate all'Ambiente \"Carlo Vidano\"",
"DFCZ	s	Forest Research Institute",
"DFD	s	Dartford Borough Museum",
"DFEC<CHN>	s	Desert Forestry Experimental Centre",
"DFEC<USA-NY>	s	Department of Forestry and Environmental Science, State University of New York",
"DFF	c	Forest Pathology Culture Collection, Pacific Forest Research Centre",
"DFLC	s	Escola Superior de Agricultura",
"DFP	c	DFP Culture Collection",
"DFRU	s	University of New Brunswick",
"DFS	s	Dumfries and Galloway Natural History and Antiquarian Society",
"DFSM	s	Dumfries Museum",
"DFV	s	Division of Fisheres",
"DGBU	s	Department of Geology, Pusan National University",
"DGM	s	Divisao de Geologia c Mineralogia",
"DGN	s	Darlington Museum",
"DGR	b	Division of Genomic Resources, University of New Mexico",
"DGR:Bird	s	Division of Genomic Resources, University of New Mexico, bird tissue collection",
"DGR:Ento	s	Division of Genomic Resources, University of New Mexico, entomology tissue collection",
"DGR:Fish	s	Division of Genomic Resources, University of New Mexico, fish tissue collection",
"DGR:Herp	s	Division of Genomic Resources, University of New Mexico, herpetology tissue collection",
"DGR:Mamm	s	Division of Genomic Resources, University of New Mexico, mammal tissue collection",
"DGS	s	The Manx Museum",
"DGUB	c	Department of Genetics, University of Bratislava",
"DH	s	Hobart and William Smith Colleges, Biology Department",
"DHISUB	s	Department of Hydrobiology and Ichthyology, Sofia Univiversity",
"DHL	s	University of Louisville, Biology Department",
"DHM	s	University of Durham, Botany Department",
"DHMB	s	Department of Harbours and Marine",
"DHNS	s	Dunbartonshire Natural History Society",
"DI	s	Universite de Bourgogne, Laboratoire de Phytobiologie Cellulaire",
"DIA	s	Museu do Dundo",
"DIN	s	Museum National d'Histoire Naturelle",
"DINH	s	Delta Institute of Natural History",
"DINO	s	Dinosaur National Monument",
"DIS	s	Dinamation International Society",
"DISCA	s	Estacion Biologica de Rancho Grande, Ministerio del Ambiente y Recursos Naturales Renovables",
"DISKO	s	Danish Arctic Station",
"DIX	s	Dixie College, Natural History Museum",
"DKG	s	Juniper Hall Field Centre",
"DLF	s	Stetson University, Biology Department",
"DLY	s	Dudley and Midland Geological and Scientific Society and Field Club",
"DM<NZ>	s	Dominion Museum",
"DM<USA-UT>	s	The Dinosaur Museum",
"DMB	s	Durban Museum",
"DMBC	s	Dominick Moth and Butterfly Collection",
"DMBUK	c	Department of Microbiology",
"DMCCUS	c	School of Biological Sciences Culture Collection",
"DMCMU2	c	Department of Microbiology, Faculty of Medicine",
"DMCU	c	Microbiology Department, Faculty of Science",
"DMDC	s	Douala Museum",
"DMFS	s	Crichton Royal Institution Museum",
"DMIV	c	Department of Microbiology and Immunology",
"DMKKU1	c	Department of Microbiology, Faculty of Medicine",
"DMKKU2	c	Department of Microbiology, Faculty of Medical Science",
"DMKU	c	Department of Microbiology, Faculty of Science",
"DMMU1	c	Department of Microbiology, Faculty of Science",
"DMMU3	c	Department of Microbiology, Faculty of Medicine Siriraj Hospital",
"DMNH<USA-CO>	s	Denver Museum of Natural History",
"DMNH<USA-DE>	s	Delaware Museum of Natural History",
"DMNH<USA-OH>	s	Dayton Museum of Natural History, Biology Department",
"DMPMC	c	Department of Microbiology",
"DMSA	s	Durban Museum",
"DMSC	s	Medicinal Plants Research Institute, Department of Medical Sciences",
"DMSP	s	Davis Mountains State Park",
"DMSRDE	c	DMSRDE Culture Collection",
"DMST	c	Culture Collection for Medical Microorganism, Department of Medical Sciences",
"DMTH	s	Britannia Royal Naval College",
"DMU	s	Mithila University, Botany Department",
"DMUIJ	c	Department of Microbiology",
"DMUP	c	Microbiology and Biophysics Charles University",
"DMUR	c	Department of Mycology",
"DMVB	c	Department of Microbiology, Veterinary Branch of National Strain Collection",
"DNA	s	Department of Natural Resources, Environment and the Arts",
"DNATAX	b	DNA-TAX",
"DNHC	s	Denver Museum of Natural History",
"DNHM<CHN>	s	Dalian Museum of Natural History",
"DNHM<USA-UT>	s	Dinosaur Natural History Museum",
"DNPM	s	Setor de Paleontologia do Departamento Nacional de Producao Mineral",
"DNS	s	Dundee Naturalists' Society",
"DO	s	Societe d'Agriculture Sciences et Arts",
"DOMO	s	Collegio Mellerio Rosmini",
"DOR	s	Dorset County Museum",
"DORC	s	Dorset County Museum",
"DORCM	s	Dorset Royal County Museum",
"DORT	s	Botanischer Garten Rombergpark, Stadt Dortmund",
"DOV	s	Delaware State University, Department of Agriculture and Natural Resources",
"DPBA	s	Departamento de Patologia Vegetal",
"DPIC	s	Belo Horizonte, Instituto de Ciencias Biologicas",
"DPIH	s	Department of Primary Industry (formerly DAHT)",
"DPIQM	s	Department of Primary Industries",
"DPIWE-FHU	c	Fish Disease Culture Collection",
"DPMWA	s	Dorthy Page Museum of Wasilla",
"DPNC	s	Denison Pequotsepos Nature Center",
"DPPC	s	Department of Agriculture",
"DPU	s	DePauw University, Botany and Bacteriology Department",
"DPUA	c	Departamento de Patologia/ICB",
"DPUP	s	Universidade Federal de Maringa",
"DQTC	s	Daqing Teachers College, Biology Department",
"DR	s	Technische Universitaet Dresden",
"DS	s	California Academy of Sciences, Botany Department",
"DSC<USA-MS>	s	Delta State University, Biological Sciences Department",
"DSC<USA-NY>	c	Dicty Stock Center",
"DSEC	s	Universidade Federal da Paraiba",
"DSIR	s	Department of Scientific and Industrial Research",
"DSM	c	DSMZ-Deutsche Sammlung von Mikroorganismen und Zellkulturen GmbH",
"DSM<TZA>	s	University of Dar es Salaam, Botany Department",
"DSMZ	c	DSMZ-Deutsche Sammlung von Mikroorganismen und Zellkulturen GmbH",
"DSP	s	Fitzsimon's Snake Park",
"DSU	s	Dnipropetrovsk National University, Department of Geobotany, Soil, and Ecology",
"DSY	s	Dewsbury Museum",
"DTIC	s	Departamento Parasitologia",
"DTN	s	Darlington and Teesdale Naturalists' Field Club",
"DU	s	Duke University Vertebrate Collection",
"DUB	s	National Botanic Gardens",
"DUBN	s	Dublin Naturalists' Field Club",
"DUE	s	University of Dundee",
"DUF	s	University of Dicle, Biological Department, Botany",
"DUH	s	University of Delhi, Botany Department",
"DUIS	s	Universitaet Duisburg, Fachbereich 6, Botanik",
"DUKE	s	Duke University, Biology Department",
"DUL	s	University of Minnesota, Biology Department",
"DUM<IND>	c	Delhi University Mycological Herbarium",
"DUM<TUR>	s	Zooligical Museum of Science and Art Faculty",
"DUR	s	Southeastern Oklahoma State University, Biological Sciences Department",
"DUSS	s	Universitaet Duesseldorf",
"DVBID	c	Division Vector-Borne Infectious Diseases",
"DVCC	s	Diablo Valley College",
"DVCM	s	Diablo Valley College Museum",
"DVM	s	Diablo Valley College, Biology Department",
"DVNM	s	Death Valley National Monument",
"DVR	s	Dover Corporation Museum",
"DVZUT	s	Department of Vertebrate Zoology",
"DWC	s	West Chester University, Biology Department",
"DWN	s	Darwen Library",
"DWT	c	Wood Technology and Forest Research Division",
"DWU	s	Dakota Wesleyan University, Biology Department",
"DZCU	s	Calcutta University",
"DZIB	s	Universidade Estadual de Campinas",
"DZKU	s	Department of Biology, Shaanxi Normal University",
"DZMU	s	Department of Zoology, Monash University",
"DZS	s	Devizes Museum",
"DZSASP	s	Departamento de Zoologia, Secretaria da Agricultura",
"DZUC	s	Departamento de Zoologia da Universidade de Coimbra",
"DZUFRGS	s	Departamento de Zoologia da Universidade Federal do Rio Grande do Sul",
"DZUH	s	Departamento de Zoologia, Universidad de Havana",
"DZUL	s	Departamento de Zoologia, Universidad de La Laguna",
"DZUP	s	Universidade Federal do Parana, Museu de Entomologia Pe. Jesus Santiago Moure",
"DZVMLP	s	Departamento Cientifico de Zoologia de Vertebrados",
"DZVU	s	Universidad de Uruguay",
"E	s	Royal Botanic Garden",
"EA	s	National Museums of Kenya",
"EAA	s	Estonian Agricultural University",
"EAC	s	Universidade Federal do Ceara, Departamento de Biologia",
"EAN	s	Universidade Federal da Paraiba, Campus III - CCA, Departamento de Fitotecnia",
"EAP	s	Escuela Agricola Panamericana",
"EAPZ	s	Escuela Agricola Panamericana",
"EAR	s	Earlham College, Biology Department",
"EATRO	c	Uganda Trypanosomiasis Research Organization",
"EBA	s	Edinburgh Academy Field Centre",
"EBCC	s	Universidad Nacional Autonoma de Mexico, Estacion de Biologia \"Chamela\"",
"EBD	s	Estacion Biologica de Donana",
"EBDS	s	Estacion Biologica de Donana",
"EBE	s	Eastbourne Museum",
"EBF	s	Hubei Forestry Institute",
"EBH	s	Botanical Society of Edinburgh",
"EBMC	s	Universidad de Chile",
"EBMTV	s	Estacion de Biologia Marina del Instituto Tecnologico de Veracruz",
"EBNHS	s	Edinburgh Natural History Society",
"EBRG	s	Museo de la Estacion Biologia de Rancho Grande",
"EBUM	s	Universidad Michoacana de San Nicolas de Hidalgo",
"EBV	s	Laboratoire de Biologie Generale et de Botanique",
"ECACC	c	European Collection of Cell Cultures",
"ECENT	s	East Central University",
"ECH	s	Elmira College",
"ECM	s	Hubei College of Traditional Chinese Medicine, Department of Chinese Materia Medica",
"ECNB	s	Escuela Nacional Ciencias",
"ECOL	s	Collection du Laborataire d'Ecologie",
"ECON	s	Harvard University",
"ECOSUR	s	El Colegio de la Frontera Sur (Mexico)",
"ECSC	s	East Central University, Biology Department",
"ECSFI	s	East China Sea Fisheries Institute",
"EDC	s	Hubei Institute for Drug Control",
"EDH	s	Plinian Society",
"EDNC	s	Raleigh, North Carolina Department of Agriculture",
"EEBP	s	Estacao Experimental de Biologia e Piscicultura de Pirassununga",
"EELM	s	Estacion Experimental Agricola de la Molina",
"EFC	s	Escola de Florestas",
"EFCC	s	Epping Forest Conservation Centre",
"EFH	s	Forestry Commission",
"EFM	s	Epping Forest Museum, Corporation of London",
"EFWM	s	Department of Entomology",
"EGE	s	Ege University",
"EGE-MACC	c	Ege - Microalgae Culture Collection",
"EGH	s	University of Edinburgh",
"EGHB	s	University of Edinburgh",
"EGHF	s	University of Edinburgh, Forestry and Natural Resources Department",
"EGNP	s	Homestead, Everglades National Park",
"EGR	s	Eszterhazy Karoly College, Botany Department",
"EHCV	s	Emory and Henry College, Biology Department",
"EHH	s	Universite d'Etat d'Haiti",
"EI	s	Universidade Federal Rural do Rio de Janeiro",
"EIF	s	Universidad de Chile, Departamento de Silvicultura",
"EIHU	s	Hokkaido University",
"EINS	s	Ecuadorian Institute of Natural Sciences",
"EISC	s	Shaanxi Agricultural University, Entomological Institute",
"EIU	s	Eastern Illinois University, Biological Sciences Department",
"EJ	s	?Ein Yabrud collection catalogue entries at The Hebrew University",
"EKU	s	Eastern Kentucky University",
"EKY	s	Eastern Kentucky University, Biological Sciences Department",
"ELCAK	s	Entomological Laboratory, College of Agriculture",
"ELM	s	East London Museum",
"ELMF	s	Augusta, Maine Forest Service",
"ELN	s	Elgin Museum",
"ELRG	s	Central Washington University, Biological Sciences Department",
"ELS	s	Aldenham School, Biology Department",
"ELVE	s	National Station for Plant Breeding",
"EM	s	Universidade Federal de Ouro Preto",
"EMA	s	Sichuan School of Chinese Materia Medica",
"EMAG	s	Ernst-Moritz Arndt Collection, Museum der Stadt Greifswald",
"EMAU	s	Ernst-Moritz-Arndt-Universitat Greifswald",
"EMBT	s	Department of Agriculture",
"EMC	s	Eastern Michigan University, Biology Department",
"EMCC	c	Egypt Microbial Culture Collection",
"EMEC	s	Essig Museum of Entomology",
"EMET	s	Faculty of Agriculture, Entomology Museum",
"EMMA	s	Universidad Politecnica de Madrid, Unidad Docente Botanica, Departamento Silvopascicultura",
"EMPARN	c	Empresa de Pesquisa Agropecuaria do Rio Grande do Norte",
"EMU	s	Eastern Michigan University, T. L. Hankinson Vertebrate Museum",
"EMUC	s	Essig Museum, Division of Entomology, University of California",
"EMUS	s	Utah State University",
"ENAG	s	Universidad Nacional Agraria, Departamento de Ciencias Basicas",
"ENCB-IPN	c	Coleccion de cultivos de la Escuela Nacional de Ciencias Biologicas",
"ENCB<MEX-Ensenada>	s	Universidad de Autonoma de Baja California",
"ENCB<MEX-Mexico City>	s	Instituto Politecnico Nacional",
"ENG	s	Royal Holloway College, University of London, Botany Department",
"ENIH	s	National Institute of Health",
"ENMU	s	Eastern New Mexico University, Natural History Museum",
"ENMUNHM	s	Eastern New Mexico University, Natural History Museum",
"ENP	s	Everglades National Park",
"ENS	s	Hubei College for Nationalities, Forestry Department",
"ENSJ	s	Escuela Normal Superior de Jalisco",
"ENT	s	Ministry of Natural Resources",
"EONJ	s	Upsala College, Biology Department",
"EOSC	s	Eastern Oregon University, Biology Department",
"EOSCVM	s	Eastern Oregon State College, Vertebrate Museum",
"EOSTS	s	Official Seed Testing Station, Agricultural Scientific Services, Department of Agriculture and Fisheries for Scotland",
"EPAL	s	Entomology Collection, Punjab Agricultural University",
"EPHR	s	Snow College, Biology Department",
"EPM	s	Epsom College Museum",
"EPN	s	Escuela Polytecnica Nacional",
"EPRL	s	University of Puerto Rico",
"ER	s	Universitaet Erlangen-Nuernberg, Geobotanik",
"ERA	s	Universidad Nacional de Entre Rios, Botanica Sistematica",
"ERAEP	c	Radiation Ecology Section, Biological Science Division, Office of Atomic Energy for Peace",
"ERCB	s	Yerevan State University, Botany Department",
"ERCULE	c	European Rumen Ciliate Culture Collection, Rowett Research Institute",
"ERE	s	Institute of Botany of the National Academy of Sciences of Armenia, Department of Plant Taxonomy and Geography",
"EREM	s	Institute of Botany of the National Academy of Sciences of Armenia, Mycology Department",
"ERH	s	Borough of Erith Museum",
"ERHM	s	Yerevan State University, Botany Department",
"ERZ	s	Fuerstin-Eugenie-Institut fuer Arzneipflanzenforschung",
"ESA	s	Universidade de Sao Paulo, Departamento de Botanica",
"ESAL	s	Universidade Federal de Lavras, Departamento de Biologia",
"ESAP	c	Instituto Zimotecnico-Z",
"ESEC	s	Entomological Society of Egypt",
"ESK	s	Seker Enstituesue",
"ESN	s	Ecole des Sciences de Niamey",
"ESNHS	s	Scottish Natural History Society",
"ESRC	s	Nova Scotia Department Natural Resources",
"ESRN	s	Escola Superior de Agricultura",
"ESS	s	Universitaet Essen",
"ESSE	s	Anadolu University",
"ESUG	s	University of Guam",
"ESUW	s	University of Wyoming",
"ET	s	East Texas State University",
"ETE	s	El Colegio de la Frontera Sur, Coleccion de insectos Asociados a Plantas Cultivadas en la Frontera Sur",
"ETH<CHE>	c	Kultursammlungen der Eidgenosische Technische Hochschule",
"ETH<ETH>	s	Addis Ababa University, Biology Department",
"ETHZ	s	Eidgenoessische Technische Hochschule-Zentrum",
"ETN	s	Eton College Museum",
"ETST	s	Texas A&M University, Biology Department",
"ETSU	s	East Tennessee State University, Biological Sciences Department",
"EU	s	Hubei University, Biology Department",
"EUB	s	Laboratory of Biology, Faculty of Science, Ehime University",
"EUMJ	s	Ehime University",
"EUQ	s	Department of Entomology, Queensland University",
"EUSL	c	Eastern University",
"EVCV	s	Erster Vorarlberger Coleopterische Verein",
"EVMU	s	Everhart Museum, Natural History Department",
"EWH	s	Ewha Womans University",
"EWNHM	s	Ewha Womens University, Natural History Museum",
"EXN	s	Exton Hall",
"EXR	s	University of Exeter, Biological Sciences Department",
"F	s	Field Museum of Natural History, Botany Department",
"FABR	s	Harmas de J. H. Fabre",
"FACHB	c	Freshwater Algae Culture Collection",
"FACS	s	Fujian Agricultural College",
"FAK	s	Department of Fisheries, Faculty of Agriculture",
"FAKOU	s	Faculty of Agriculture, Kochi Univerisity",
"FAKU	s	Kyoto University",
"FAN	s	Museum of Fanjingshan National Nature Reserve",
"FAR	s	University of Tarbiat-Moaallem, Biology Department",
"FARM	s	Longwood University, Department of Natural Sciences",
"FAU	s	Florida Atlantic University, Biological Sciences Department",
"FAUC	s	Universidad de Caldas, Departamento de Recursos Naturales",
"FAUN	s	Universidad de Narino",
"FAVU	s	Universidade Federal do Rio Grande do Sul, Faculdade Agronomia e Veterenaria",
"FB	s	Albert-Ludwigs Universitaet, Institut fuer Biologie II",
"FBA	s	Freshwater Biological Association",
"FBC	s	University of Sierra Leone, Fourah Bay College, Botany Department",
"FBCS	s	Universidad Autonoma de Baja California Sur, Museo de Historia Natural",
"FBGMU	c	Faculty of Biology Gadjah Mada University",
"FBMN	s	Museum fuer Naturkunde",
"FBQ	s	Fisheries Branch, Departement of Primary Industries",
"FBUB	s	Universitat Bielefeld",
"FBWA	s	Forstlichen Bundsversuchsanstalt",
"FC-DPV	s	Departmento de Paleontologia, Facultad de Ciencias",
"FCAB	s	Pontificia Universidade Catolica do Rio de Janeiro, Nucleo Interdisciplinar de Meio Ambiente",
"FCAP	s	Universidade Federal do Para",
"FCBP	c	First Fungal Culture Bank of Pakistan",
"FCDA	s	Fresno County Department of Agriculture",
"FCLR	s	Fundacion Cientifica Los Roques",
"FCM	s	Facultad de Ciencias Marinas",
"FCME	s	Universidad Nacional Autonoma de Mexico, Ciudad Universitaria, Departamento de Biologia",
"FCMM	s	Universidad Nacional Autonoma de Mexico, Facultad de Ciencias",
"FCNI	s	Forest Commission of N.S.W.",
"FCO	s	Universidad de Oviedo, Departamento de Biologia de Organismos y Sistemas",
"FCQ	s	Universidad Nacional de Asuncion, Departamento de Botanica, Direccion de Investigacion",
"FCRM	s	Fisheries College Reference Museum",
"FCT	c	FCT",
"FCTH	s	Forestry Commission of Tasmania",
"FCU	s	Fukien Christian University",
"FCUG	c	Fungal Cultures University of Goteborg",
"FDA	c	US Food and Drug Administration",
"FDC	c	Forsyth Dental Center",
"FDG	s	Guyana Forestry Commission",
"FDLW	s	University of Wisconsin Center, Biology Department",
"FDNR	s	Florida Department of Natural Resources",
"FDUC	s	Fairleigh Dickinson University [collection transferred to FSCA].",
"FDVC	s	De La Villa, Francisco",
"FER	s	Universita de Ferrara, Dipartimento di Biologia - Sezione di Botanica",
"FERM	c	Patent and Bio-Resource Center, National Institute of Advanced Industrial Science and Technology (AIST)",
"FEZA	s	Universidad Nacional Autonoma de Mexico, Carrera de Biologia",
"FFB	s	Atlantic Forestry Centre, Canadian Forest Service",
"FFCL	s	Nossa Senhora do Patrocinia",
"FFR	s	Forfar Museum and Art Gallery, Meffan Institute",
"FFS	s	University of Stellenbosch",
"FFSUC	s	Faculty of Forestry Sciences",
"FG	s	Palaontologische Hauptsammlung der Bergakadmie",
"FGC	s	Grassland Research Institute, Chinese Academy of Agricultural Sciences",
"FGG	s	Faculty of Geology and Geophysis",
"FGGUB	s	Facultatea de Geologie si Geofisca",
"FGIC	s	Francois Genier",
"FGSC	c	Fungal Genetics Stock Center",
"FH	s	The Farlow Herbarium, Harvard University Herbaria",
"FH<USA-KS>	s	Fort Hays",
"FHI	s	Forestry Research Institute of Nigeria",
"FHK	s	Divisional Forest Office",
"FHKS	s	Fort Hays State University",
"FHKSC	s	Fort Hays State University",
"FHL	s	Friday Harbor Laboratories, University of Washington",
"FHO	s	University of Oxford, Department of Plant Sciences",
"FHSM	s	Fort Hays Sternberg Museum",
"FI	s	Museo di Storia Naturale dell'Universita",
"FIAF	s	Universita degli Studi di Firenze, Dipartimento di Biologia Vegetale",
"FICB	s	Forest Research Centre",
"FIDS	s	Great Lakes Forest Research Laboratory, Forest Insect and Disease Survey",
"FIEC	s	Freshwater Institute",
"FIJI	s	University of the South Pacific",
"FIOC	s	Fundacao Instituto Oswaldo Cruz",
"FIP	s	Florida Institute of Paleontology",
"FIPF	s	Universita di Firenze",
"FIPIA	s	Institut Teknologi Bandung, Jurusan Biologi",
"FJFC	s	Fujian Forestry College",
"FJSI	s	Fujian Institute of Subtropical Botany",
"FKE	s	Folk Museum",
"FKEN	s	Folkestone Natural History Society",
"FLACC	c	Free-Living Amoebae Culture Collection",
"FLAS	s	Florida Museum of Natural History Herbarium",
"FLC	s	Fort Lewis College",
"FLD	s	Fort Lewis College, Biology Department",
"FLIN	s	Flinders University",
"FLK	s	Falkirk District Council Museum",
"FLOR	s	Universidade Federal de Santa Catarina, Departamento de Botanica",
"FLSP	s	Oscar Scherer State Park",
"FM<CHN-Beijing>	s	Fan Memorial Institute of Biology",
"FM<CHN-Fujian>	s	Department of Nature, Fujian Province Museum",
"FMB	s	Instituto Alexander von Humboldt",
"FMC	s	North Museum of Natural History and Science",
"FMH	s	Goddard College",
"FMJ	c	Faculty of Medicine, Juntendo University",
"FML	s	Fundacion Miguel Lillo",
"FMM	s	Muzeum Beskyd",
"FMNH	s	Field Museum of Natural History",
"FMNH:ARTH	s	Field Museum of Natural History, Arthropod Collection",
"FMNH:AVES	s	Field Museum of Natural History, Ornithology Collection",
"FMNH:HERP	s	Field Museum of Natural History, Herpetology Collection",
"FMNH:ICHTHY	s	Field Museum of Natural History, Ichthyology Collection",
"FMNH:INVRT	s	Field Museum of Natural History, Invertebrate Collection",
"FMNH:MAMM	s	Field Museum of Natural History, Mammal Collection",
"FMNH<FIN>	s	Finnish Museum of Natural History",
"FMP	s	Fujian Academy of Traditional Chinese Medicine and Pharmacology",
"FMPC	s	Fairbanks Museum and Planetarium Collection",
"FMR	c	Facultad de Medicina",
"FMRI	s	Central Marine Fisheries Marine Research Institute",
"FMSS	s	Parque Zoological Nacional \"Finca Modelo\", Natural History Museum",
"FNCC	c	Food and Nutrition Culture Collection",
"FNFR	s	Fishlake National Forest",
"FNLO	s	Fremont National Forest",
"FNM	s	Fries Natuurmuseum",
"FNML	s	Fries Natuurhistorisch Museum",
"FNP	s	Fundy National Park",
"FNPS	s	South Florida Collections Management Center, Everglades National Park",
"FNU<CHN>	s	Fujian Normal University",
"FNU<JPN>	s	Nagasaki University - Fisheries",
"FOR	s	Forssa Museum of Natural History",
"FOSJ	s	Fisheries and Oceans Biological Station",
"FPDB	s	Universidad de Puerto Rico, Departamento de Ciencias Marinas",
"FPF	s	Rocky Mountain Research Station, USDA Forest Service",
"FPM	s	Fukui Prefectural Museum",
"FPRL	s	Building Research Establishment",
"FQH	s	Fort Qu'Appelle Herbarium",
"FR	s	Forschungsinstitut Senckenberg",
"FRC	s	Institute of Forest Genetics and Tree Breeding",
"FRCL	s	Fisheries Research Centre",
"FRCS	s	Forest Research Centre",
"FRI<AUST>	s	CSIRO",
"FRI<JPN>	c	Food Research Institute, Ministry of Agriculture, Forestry and Fisheries",
"FRIHP	s	Fisheries Research Institute of Hunan Province",
"FRIM	s	Forest Research Institute",
"FRLC	s	Forest Insect and Disease Survey Reference Collection",
"FRLH	s	Foundation for Revitalisation of Local Health Traditions, Research Department",
"FRLM	s	Faculty of Fisheries, Mie University",
"FRM	s	Friends University, Fellow-Reeve Museum of History and Science",
"FRNZ	s	Forest Research Institute",
"FRP	s	Palmengarten",
"FRR	c	Food Science Australia, Ryde",
"FRS	s	Falconer Museum",
"FRSKU	s	Kyoto University, Fisheries Research Station",
"FRU	s	National Academy of Science, Kyrgyzstan, Laboratory of Flora",
"FSAG	s	Faculte des Sciences Agronomiques de Gembloux",
"FSC<CAN>	c	Fredericton Stock Culture Collection",
"FSC<USA-CA>	s	California State University, Biology Department",
"FSCA	s	Florida State Collection of Arthropods, The Museum of Entomology",
"FSCL	s	Florida Southern College, Biology Department",
"FSFRL	s	Far Seas Fisheries Research Laboratory",
"FSIU	s	Laboratory of Fisheries, Department of Oceanography",
"FSL	s	Collections de la Faculte des Sciences de Lyon",
"FSLF	s	Rocky Mountain Forest and Range Experiment Station",
"FSMC	s	Florida State Museum",
"FSP-USP	s	Faculdade de Saude Publica, Universidade de S~ao Paulo",
"FSSR	s	Forest Service, USDA, Biological and Physical Resources Unit",
"FSU	s	Florida State University, Department of Biological Science",
"FSU<DEU>	c	Institut fuer Mikrobiologie FSU Jena",
"FSUM	s	Florida State University Museum",
"FSUMC	s	Frostburg State University, Mammal Collection",
"FT	s	Centro Studi Erbario Tropicale, Universita degli Studi di Firenze",
"FTCC	c	Food Technology Culture Collection",
"FTCMU	c	Department of Food Science and Technology, Faculty of Agriculture",
"FTG	s	Fairchild Tropical Botanic Garden",
"FTI	c	Centro de Biotecnologia e Quimica-CEBIQ",
"FTS	s	Fuzhou Teachers College, Biology Department",
"FTU	s	University of Central Florida, Biology Department",
"FU<CHN>	s	Fudan University, Department of Biology",
"FU<JPN>	s	Kyushu University, Department of Forest and Forest Products Sciences",
"FUB	s	Frei Universitat",
"FUE	s	Fukuoka University of Education",
"FUEL	s	Universidade Estadual de Londrina, Departamento de Biologia Animal e Vegetal",
"FUGR	s	Furman University, Biology Department",
"FUH	s	Firat Ueniversitesi",
"FULD	s	Verein fuer Naturkunde in Osthessen",
"FUMH	s	Ferdowsi University",
"FUMT	s	University of Tokyo",
"FUS	s	Fudan University, Biology Department",
"FUSC	c	Flinders University Smut Collection",
"FVCC	s	Flathead Valley Community College, Biology Department",
"FW	s	Texas Christian University, Biology Department",
"FWM	s	Fort Worth Museum of Science and History, Science Department",
"FWMSH	s	Fort Worth Museum of Science & History",
"FWRI	s	Florida Fish and Wildlife Research Institute",
"FWRI:Ichthyology	s	Florida Fish and Wildlife Research Institute, Ichthyology Collection",
"FWRI:Invertebrate	s	Florida Fish and Wildlife Research Institute, Invertebrate Collection",
"FWRI:SEAMAP	c	Florida Fish and Wildlife Research Institute, SEAMAP Ichthyoplankton Collection",
"FWVA	s	Fairmont State University, Biology Department",
"G	s	Conservatoire et Jardin botaniques de la Ville de Geneve",
"GA	s	University of Georgia, Plant Biology Department",
"GAB	s	National Museum, Monuments, and Art Gallery",
"GABAS	s	Centre d'Etude et de Conservation des Resources Vegetales",
"GAC	s	Guangxi Agricultural University, Forestry Department",
"GACP	s	Guizhou Agricultural College, Department of Plant Protection",
"GAES	s	Georgia Agricultural Experiment Station",
"GAFS	s	Ganzhou Forestry School",
"GAI	s	Folk Museum",
"GALW	s	National University of Ireland, Galway, Botany Department",
"GAM<USA-GA>	s	University of Georgia",
"GAM<VEN>	c	Grupo Actinomicetales Merida Facultad de Medicina",
"GAS	s	Georgia Southern University, Department of Biology",
"GAT	s	Institute of Plant Genetics and Crop Plant Research",
"GAUA	s	Guangxi University",
"GAUBA	s	Australian National University, Division of Botany and Zoology",
"GAUF	s	Gansu Agricultural University",
"GAW	s	Eastern Botanical Society of Glasgow",
"GAZI	s	Gazi Ueniversitesi, Biyoloji Boeluemue",
"GB	s	Goeteborg University, Department of Plant and Environmental Sciences",
"GBFM	s	Universidad de Panama",
"GBNM	s	Glacier Bay National Park and Preserve Museum",
"GBY	s	Grimsby Arts and Natural History",
"GC<GHN>	s	University of Ghana, Botany Department",
"GC<USA-MD>	s	Goucher College",
"GCL	c	Central Laboratories",
"GCM<GHN>	s	University College of Ghana",
"GCM<PAK>	s	Government College, Department of Zoology",
"GCNP	s	Grand Canyon National Park",
"GCRL	s	Gulf Coast Research Laboratory",
"GCTP	s	Global Colosseum",
"GDA	s	Universidad de Granada",
"GDAC	s	Universidad de Granada, Departamento de Biologia Vegetal, Botanica",
"GDGM	s	Guangdong Institute of Microbiology",
"GDMA	s	Medical University of Gdansk, Department of Biology and Pharmaceutical Botany",
"GDMM	s	Guangdong Institute of Chinese Materia Medica",
"GDMP	s	Guangdong Medical and Pharmaceutical College, Pharmacy Department",
"GDOR	s	Museo Civico di Storia Naturale Giacomo Doria",
"GE	s	Universita di Genova",
"GEIC	s	Guangdong Entomology Institute",
"GENT	s	Gent University, Biology Department",
"GEO	s	Emory University, Biology Department",
"GES	s	Gesneriad Research Foundation",
"GESU	s	State University of New York, Biology Department",
"GF	s	Guizhou Academy of Forestry",
"GFBI	s	Gymnasium der Franziskaner in Bozen [= Bolzano]",
"GFC	s	University of Great Falls, Biology Department",
"GFJP	s	Universidade do Estado de Minas Gerais",
"GFND	s	University of North Dakota, Biology Department",
"GFRC	s	Golestan Fisheries Research Centre",
"GFS	s	Guizhou Forestry School",
"GFW	s	Ernst-Moritz-Arndt-Universitaet, Botanisches Institut und Botanischer Garten",
"GGB	s	Gesneriad Gardens",
"GGM	s	Gosudarstvennyi Geologicheskii Musei - State Geological Museum",
"GGO	s	University of Strathclyde, Biology Department",
"GGW	s	Botanical Society of Glasgow",
"GH	s	Harvard University",
"GHD	s	Shipley Art Gallery and Saltwell Tower Museum",
"GHG	s	Council for Geosciences",
"GHPG	s	Harold Porter National Botanical Garden",
"GHRI	s	Guy Harvey Research Institute",
"GHS	s	George Heriot's School, Biology Department",
"GI	s	Justus-Liebig-Universitaet Giessen",
"GI-SPS	s	Geological Institute, Section of Palaeontology and Stratigraphy",
"GIUV	s	Geological Institute, University of Vienna",
"GJO	s	Steiermaerkisches Landesmuseum Joanneum, Botany Department",
"GKAR	s	Karoo National Botanical Garden",
"GL	s	University of Glasgow, Botany Department",
"GLAC	s	Glacier National Park, Glacier Collection",
"GLAHM	s	University of Glasgow, Hunterian Museum",
"GLAM	s	Art Gallery and Museum, Natural History Department",
"GLANH	s	Hunterian Museum",
"GLEN	s	Rappahannock Community College",
"GLFR	s	Great Lakes Forest Research Centre",
"GLG	s	Trinity College",
"GLLB	s	Laurel Bank School",
"GLM	s	Staatliches Museum fuer Naturkunde Goerlitz",
"GLMC	s	Guilin Medical College, Pharmacy Department",
"GLNP	s	Glacier National Park",
"GLO	s	Natural History Society of Glasglow",
"GLOW	s	Lowveld National Botanical Garden",
"GLR	s	Gloucester City Museum and Art Gallery",
"GLW	s	Andersonian Naturalists' Society",
"GM	s	Museum of Southeastern Moravia",
"GMACC	c	Laboratory of Molecular Genetics and Breeding of Edible Mushrooms",
"GMAU	s	Geological Museum of Amsterdam University",
"GMBL	s	College of Charleston",
"GMC	s	Guangxi Medical College",
"GMCE	s	Grosvenor Museum",
"GMH	s	Sammlung Jacobi des Geiseltalmuseum Halle",
"GMHH	s	Geological Museum of Heilongjang Province",
"GML<USA-IL>	s	Gorgas Memorial Laboratory",
"GML<USA-MO>	s	Gaylord Memorial Laboratory Museum",
"GMNGZ	s	Glasgow Museum and Art Galleries",
"GMNH-PV	s	Paleo-Vertebrate Collection",
"GMNH<SWTZ>	s	Museum d'Histoire Naturelle",
"GMNH<USA-GA>	s	Georgia Museum of Natural History",
"GMNP	s	Gros Morne National Park",
"GMS	s	Hopkins Marine Station, Stanford University, Biological Sciences Department",
"GMU	s	N. P. Ogariov Mordovia State University, Department of Botany and Plant Physiology",
"GMUF	s	George Mason University, Department of Environmental Science and Policy 5F2",
"GMUG	s	Universitat Gottingen, Geologisches-Palaatologisches Museum",
"GMUM	s	Institut fuer Palaeontologie und Geologisches Museum der Universitat",
"GMUV	s	Geological Museum, University of Vienna",
"GNA	s	Gannan Arboretum of Jiangxi",
"GNHM	s	Goulandris Natural History Museum",
"GNHNA	s	Gallery of Natural History and Native Art",
"GNHS	s	Guildford Natural History Society",
"GNU	s	Guangxi Normal University, Biology Department",
"GNUB	s	Guizhou Normal University, Biology Department",
"GNUC	s	Gyeongsang National University, Biology Department",
"GNUG	s	Guizhou Normal University, Geography Department",
"GO	s	Philosophical Society",
"GOD	s	Charterhouse School Museum",
"GOE<DEU>	s	Institut und Museum fuer Geologie und Palaeontologie",
"GOE<GBR>	s	Goole Scientific Society",
"GOET	s	Universitaet Goettingen, Abteilung Systematische Botanik",
"GOFS	s	Free State National Botanical Garden",
"GOW	s	Clydebank High School",
"GP	s	Instituto de Geociencias, Universidade de Sao Paulo",
"GPA	s	Grande Prairie Regional College, Science Department",
"GPI	s	Geologisch-Palaeontologisches Institut",
"GPIH	s	Geologisch-Palaeontologiches Institut der Universitt Haemburg",
"GPIM	s	Lehreinheit Palaeontologisches, Institut fuer Geowissenschaften",
"GPIT	s	Institut und Museum fur Geologie und Palaeontologie, Universitat Tuebingen",
"GPMK	s	Geologisch-Palaontologisches Institut und Museum",
"GPPT	s	Plant Protection Institute",
"GR	s	Universite J. Fourier - Grenoble I, Botanique",
"GRA	s	Albany Museum",
"GRCAMC	s	Grand Canyon National Park Museum Collection",
"GRCH	s	Colgate University, Biology Department",
"GREE	s	University of Northern Colorado, Department of Biological Sciences",
"GRI	s	Grinnell College, Biology Department",
"GRIF	s	Griffith University",
"GRJC	s	Grand Rapids Junior College",
"GRK	s	McLean Museum and Art Gallery",
"GRM	s	Museum d'Histoire Naturelle de Grenoble",
"GRMP	s	Central Geological Research Museum",
"GRO	s	State University of Groningen, Department of Plant Biology",
"GRPM	s	Public Museum of Grand Rapids",
"GRS	s	Gezira Research Station",
"GRSM	s	Great Smoky Mountains National Park",
"GRSU	s	Yanka Kupala Grodno State University, Department of Botany",
"GRSW	s	Desert Ecological Research Unit",
"GSAT	s	The Geological Survey of Alabama",
"GSC	s	Geological Survey of Canada",
"GSDNM	s	Great Sand Dunes National Monument",
"GSFS	s	Gansu Forestry School",
"GSI	s	Geological Survey of India",
"GSM	s	Geologic Museum",
"GSMNP	s	Great Smoky Mountains National Park",
"GSN	s	Geological Survey of Nambia",
"GSO	s	Glasgow Society of Field Naturalists'",
"GSP	s	Geological Survey of Portugal",
"GSU	s	F. Scorina Gomel State University, Department of Botany and Plant Physiology",
"GSW	s	Georgia Southwestern State University, Biology Department",
"GTC	c	Gifu Type Culture Collection",
"GTC-GIFU	c	Gifu Type Culture Collection (GTC), Gifu University Culture Collection (GIFU)",
"GTM	s	Grantham Museum",
"GTNP	s	Grand Teton National Park",
"GU	s	Gotland University, Department of Biology",
"GUA	s	DIVEA, DEP, FEEMA, FEEMA",
"GUAD	s	Institut National de la Recherche Agronomique and Parc National de Guadeloupe",
"GUADA	s	Universidad Autonoma de Guadalajara",
"GUAM	s	University of Guam, Biology Department",
"GUAT	s	Museo Nacional de Historia Natural",
"GUAY	s	Universidad de Guayaquil",
"GUH	s	HNB Garhwal University, Botany Department",
"GUM	s	Glasgow University Museum (Hunter Museum)",
"GUYN	s	Fundacion Jardin Botanico del Orinoco",
"GVF	s	George Vanderbilt Foundation",
"GVSC	s	Grand Valley State University, Biology Department",
"GW	s	West of Scotland College of Agriculture, Botany Department",
"GXCM	s	Guangxi Traditional Chinese Medicine University, Pharmacy Department",
"GXDC	s	Guangxi Institute for Drug Control",
"GXEM	s	Guangxi Institute of Ethnomedicine",
"GXF	s	Guangxi Institute of Forest Survey and Design",
"GXFI	s	Guangxi Forestry Institute",
"GXFS	s	Guangxi Forestry School",
"GXMG	s	Guangxi Medicinal Botanic Garden",
"GXMI	s	Guangxi Institute of Traditional Medical and Pharmaceutical Sciences",
"GXNM	s	Guangxi Natural History Museum",
"GXSP	s	Guangxi School of Pharmacy",
"GZAC	s	Guizhou Agricultural College, Forestry Department",
"GZM	s	Giessener Zoologisches Museum",
"GZTM	s	Guizhou Institute of Traditional Chinese Medicine",
"GZU	s	Karl-Franzens-Universitaet Graz",
"H	s	University of Helsinki",
"H-GSP	s	Howard University-Geological Survey of Pakistan Project",
"HA	s	Universidad del Azuay, Escuela de Biologia del Medio Ambiente",
"HABA	s	Academia de Ciencias Medicas, Fisicas y Naturales de La Habana",
"HABAYC	s	University of Mary Hardin-Baylor, Biology Department",
"HABE	s	Instituto de Biologia, Departamento de Ecologia",
"HAC	s	Instituto de Ecologia y Sistematica",
"HACC	s	Academia de Ciencias Camagueey",
"HACW	s	Department of Fishery, Huazhong Agriculture Collection",
"HAF	s	Hainan Forestry Institute",
"HAJB	s	Jardin Botanico Nacional",
"HAK	s	Hokkaido University, Faculty of Fisheries",
"HAKS	s	Hakgala Botanic Gardens",
"HAL	s	Martin-Luther-Universitaet",
"HALA	s	University of Alabama, Biological Sciences Department",
"HALLE	s	Zoologisches Institut der Martin-Luther Universitaet",
"HALLST	s	Botanische Station",
"HALX	s	Halifax Literary and Philosophical Society",
"HAM	s	Royal Botanical Gardens",
"HAMAB	s	Instituto de Pesquisas Cientificas e Tecnologicas do Estado do Amapa",
"HAMBI	c	HAMBI Culture Collection",
"HAMU	s	University of Newcastle upon Tyne",
"HAN	s	Universitaet Hannover",
"HANU	s	Harbin Normal University, Biology Department",
"HAO	s	Universidad Privada Antenor Orrego",
"HAQ	s	Institut der Technische Hochschule (RWTH), Institut fuer Biologie I",
"HAS	s	Fundacao Zoobotanica do Rio Grande do Sul",
"HAST	s	Research Center for Biodiversity, Academia Sinica",
"HASU	s	Universidade do Vale do Rio dos Sinos - CCS/ Centro 2",
"HAUH	s	Haryana Agricultural University",
"HAVI	s	Eastern Mennonite University, Biology Department",
"HAVO	s	Hawaii Volcanoes National Park",
"HAW	s	University of Hawaii, Botany Department",
"HAX	s	Belle Vue Museum",
"HAY	s	California State University, Biological Sciences Department",
"HB	s	Herbarium Bradeanum",
"HBAU	s	Hebei Agricultural University",
"HBAUD	s	Hebei Agricultural University, Handan Branch, Agriculture Department",
"HBBS	s	Museo Civico di Scienze Naturali",
"HBC	s	Henry Brockhouse Collection",
"HBDC	s	Hebei Institute for Drug Control",
"HBFC	s	Hebei Forestry College, Basic Courses Department",
"HBFH	s	Harbor Branch Oceanographic Institution, Marine Botany Department",
"HBG	s	Institut fuer Allgemeine Botanik",
"HBI	s	Institute of Hydrobiology, Chinese Academy of Sciences, Phycology Department",
"HBIL	s	Institut d'Estudis Ilerdencs",
"HBNU	s	Hebei Normal University, Biology Department",
"HBOM	s	Harbor Branch Oceanographic Museum",
"HBR	s	Universidade Federal de Santa Catarina",
"HBUM	s	College of Life Sciences Hebei Univesity, Baoding",
"HC	s	Hangchow Christian College",
"HCAT	s	University of Tabriz, Landscape Department",
"HCB	s	Universidade de Santa Cruz do Sul, Departamento de Biologia",
"HCCA	s	Hastings College",
"HCCV	s	Hastings College, Collection of Vertebrates",
"HCEN	s	Universidad Nacional del Centro del Peru",
"HCH	s	Lewis-Clark State College, Natural Sciences Department",
"HCHM	s	Hope College, Biology Department",
"HCIB	s	Centro de Investigaciones Biologicas del Noroeste, S. C.",
"HCIO	s	Indian Agricultural Research Institute",
"HCMS	s	Hampshire County Council Museums Service",
"HCMZ	s	Hope College",
"HCNHSC	s	Ohio Historical Society, Natural History Synoptic Collection",
"HCOA	s	College of the Atlantic, Biology Department",
"HCT	s	Taiwan Forestry Research Institute",
"HCTR	s	Hoogstraal Center for Tick Research",
"HDD	s	Tolson Museum, Natural History Department",
"HDOA	s	Hawaii Department of Agriculture",
"HDSM	s	University of Massachusetts Dartmouth",
"HDTC	s	Huddersfield Technical College",
"HEAC	s	Henan Agricultural University",
"HEB	s	Hebden Bridge Literary and Scientific Society",
"HEBI	s	Henan Academy of Sciences",
"HECM	s	Henan College of Traditional Chinese Medicine",
"HEFG	s	l'Ecole de Faune de Garoua",
"HEH	s	Escuela Nacional de Ciencias Forestales, Departamento de Investigacion Forestal Aplicada",
"HEID	s	Universitaet Heidelberg, Heidelberger Institut fuer Pflanzenwissenschaften",
"HEL	s	University of Helsinki, Section of Botany",
"HEMS	s	Haslemere Educational Museum",
"HENA	s	Escuela Nacional de Agricultura",
"HEND	s	Henderson State University, Biology Department",
"HENNU	s	Henan Normal University",
"HENU	s	Henan Normal University, Biology Department",
"HEPH	s	Jardim Botanico de Brasilia",
"HER<CAN>	c	Felix d'Herelle Reference Center for Bacterial Viruses",
"HER<ZAF>	s	Hermanus Botanical Society",
"HERZ	s	Herzen State Pedagogical University of Russia, Department of Botany",
"HERZU	s	Universidad del Zulia",
"HF	s	Universidade Federal do Para",
"HFB	s	Hainan Forestry Bureau",
"HFBG	s	Forestry Botanical Garden of Heilongjiang",
"HFCC	c	Flagellate Culture Collection, University of Cologne",
"HFD	s	Hereford Museum",
"HFR	s	Finnish Forest Research Institute",
"HFRI	s	Hunan Forestry Research Institute",
"HFV	s	Universidad Austral de Chile, Instituto de Produccion y Sanidad Vegetal",
"HFX	s	Bankfield Museum and Art Gallery",
"HGAS	s	Guizhou Academy of Sciences, Plant Taxonomy Group",
"HGCRL	s	Gulf Coast Research Laboratory",
"HGI	s	Universitat de Girona, Unitat de Biologia Vegetal",
"HGM	s	Hunan Geological Museum",
"HGS	s	Public Museum and Art Gallery, St. John's Place",
"HGTC	s	Huanggang Teachers College, Biology Department",
"HHBG	s	Hangzhou Botanical Garden",
"HHC	s	University of Helsinki, Horticulture Department",
"HHH	s	Hartwick College",
"HHM	s	Collyer's School",
"HHU	s	Hallym University",
"HHUA	s	Universidad Nacional de Huanuco Hermilio Valdizan",
"HHUF	s	Hirosaki University, Laboratory of Plant Pathology",
"HIB	s	Wuhan Institute of Botany",
"HIC	s	University of Kentucky, Department of Entomology, Hymenoptera Institute Collection",
"HIFP	s	French Institute",
"HILL	s	Sir Harold Hillier Gardens",
"HIMC	s	Inner Mongolia University",
"HIN	s	Hitchin Priory",
"HIP	s	Universidad de Magallanes",
"HIPC	s	Instituto Superior Pedagogico Jose Marti, Departamento de Biologia",
"HIRO	s	Hiroshima University, Biological Science Department",
"HIRU	s	Okayama University of Science",
"HITBC	s	Xishuangbanna Tropical Botanical Garden, Academia Sinica",
"HIUW	s	Hygiene-Institut der Universitaet",
"HIWNT	s	Hampshire and Isle of Wight Naturalists' Trust Ltd.",
"HJBL	s	Escuela Nacional de Ciencias Forestales",
"HJBS	s	Fundacio Jardi Botanic de Soller",
"HK	s	Agriculture, Fisheries, and Conservation Department",
"HKBU	s	Hong Kong Baptist University, Biology Department",
"HKFRS	s	Hong Kong Fisheries Research Station",
"HKGL	s	Naturwissenschaftliche Sammlungen des Kantons Glarus",
"HKU	s	University of Hong Kong, Ecology and Biodiversity Department",
"HKUCC	c	The University of Hong Kong Culture Collection",
"HL	s	Houghton Lake Wildlife Research Station, Natural Resources Department",
"HLA	s	Harold L. Lyon Arboretum",
"HLCM	s	Heilongjiang College of Traditional Chinese Medicine, Department of Chinese Materia Medica",
"HLD	s	Hessisches Landesmuseum",
"HLL	s	Queen's Gardens, College of Higher Education, Natural Science Department",
"HLMA	s	Town Docks Museum, Hull City Corporation",
"HLMD	s	Hessisches Landesmuseum Darmstadt",
"HLNM	s	Heilongjiang Provincial Museum",
"HLO	s	Vlastivedne Muzeum v Hlohovci",
"HLU	s	University of Hull, Botany Department",
"HLUC	s	Universita degli Studi della Basilicata, Dipartimento di Biologia Difesa e Biotecnologie Agroforestali",
"HLUL	s	University of Hull",
"HLX	s	Ovenden Naturalists' Society",
"HM<DEU>	s	Humbolt Museum",
"HM<USA-NE>	s	Hastings Museum",
"HMAS	s	Institute of Microbiology, Academia Sinica",
"HMC	s	Jardin Botanico de Las Tunas",
"HME	s	Haslemere Educational Museum",
"HMH	s	Hoebarth Museum Horn",
"HMLN	s	District Museum",
"HMM	s	Horsham Museum",
"HMN	s	Humbolt Museum fur Naturkunde, East Berlin",
"HMNH	s	Hayashibara Museum of Natural History",
"HMNS	s	Houston Museum of Natural Science",
"HMNT	s	Hancock Museum, Newcastle University",
"HMP	s	Hornonitrianske muzeum, Department of Natural History",
"HMS	s	Embrapa Gado de Corte",
"HMUG	s	Hunterian Museum",
"HN	s	National Center for Natural Sciences and Technology, Botany Department",
"HNBU	s	Institut de l'Environnement et de Recherche Agricola (INERA)",
"HNCMB	c	Hungarian National Collection of Medical Bacteria",
"HNH	s	Dartmouth College, Biological Sciences Department",
"HNHH	s	Heilongjiang Natural History Museum",
"HNHM<HUN>	s	Hungarian Natural History Museum (Termeszettudomanyi Muzeum)",
"HNHM<KOR>	s	Hannam University, Department of Biology",
"HNHR	s	University of California, Hastings Natural History Reservation",
"HNIP	s	Hanoi College of Pharmacy",
"HNMN	s	Universidad Centroamericana",
"HNN	s	Horniman Museum of Natural History",
"HNNU	s	Hunan Normal University, Botany Department",
"HNR	s	Heilongjiang Academy of Sciences",
"HNT	s	Huntington Botanical Gardens",
"HNTS	s	Vlastivedne muzeum",
"HNU<CHN>	s	Hunan Normal University",
"HNU<VNM>	s	Vietnam National University, Department of Botany",
"HNUB	s	Northeastern University, Biology Department",
"HNWP	s	Northwest Plateau Institute of Biology, Chinese Academy of Sciences",
"HNWU	s	Nebraska Wesleyan University, Biology Department",
"HO	s	Tasmanian Museum & Art Gallery",
"HOH	s	Universitaet Hohenheim (210)",
"HOLZ	s	Paleontological Collection",
"HOMP	s	Okresni muzeum Pribram Brezove Hory",
"HON	s	Sichuan Grassland Research Institute",
"HOU	s	University of Houston",
"HPC	s	Howard Payne University, Biology Department",
"HPD	s	Hampstead Scientific Society",
"HPDL	s	Hampstead Public Library",
"HPH	s	Monroe County Department of Parks",
"HPM	s	Houston Museum of Natural Science",
"HPP	s	University of Helsinki, Plant Biology Department",
"HPPR	s	Instituto Superior Pedagogico de Pinar del Rio, Departamento de Biologia",
"HPSU	s	Portland State University, Biology Department",
"HPU	s	High Point University, Biology Department",
"HPUJ	s	Pontificia Universidad Javeriana",
"HPVC	s	Universidad Pedagogico Felix Varela,, Departamento de Biologia",
"HR	s	Muzeum Vychodnich Cech",
"HRB	s	IBGE",
"HRCB	s	Universidade Estadual Paulista",
"HRJ	s	Universidade do Estado do Rio de Janeiro, Departamento de Biologia Animal e Vegetal",
"HRP	s	Universidad Nacional de La Patagonia, Departamento Biologia General",
"HSB	s	Universidad Mayor Real y Pontificia de San Francisco Xavier de Chuquisaca",
"HSC	s	Humboldt State University, Biological Sciences Department",
"HSCC	c	Culture Collection of the Research and Development Department",
"HSI	s	University of Helsinki, Silviculture Department",
"HSIB	s	Shanxi Institute of Biology, Botany Department",
"HSIC	s	Ministry of Natural Resources",
"HSM	s	Christ's Hospital, Biology Department",
"HSNU	s	East China Normal University, Biology Department",
"HSS	s	Development, Technological and Investigacion Service, Forest Production Department",
"HSU<USA-CA>	s	Humboldt State University",
"HSU<USA-TX>	s	Hardin-Simmons University, Biology Department",
"HSUCV	s	Hardin-Simmons University, Collection of Vertebrates",
"HSUE	s	Natural History Museum",
"HSUMZ	s	Henderson State University, Museum of Zoology",
"HTC	s	Hangzhou Normal College, Biology Department",
"HTD	s	College Natural History Society",
"HTE	s	Queen Ethelburga's School",
"HTGN	s	Norris Museum and Library",
"HTIN	s	Universidad Nacional Agraria de la Selva",
"HTN	s	Hitchin Museum",
"HTO	s	Universidade Federal do Tocantins, Nucleo de Estudos Ambientais",
"HTTU	s	Tennessee Technological University, Biology Department",
"HTU	s	University of Taiz, Biology Department",
"HU	s	University of Zhejiang",
"HUA	s	Universidad de Antioquia, Centro de Investigaciones",
"HUAA	s	Universidad Autonoma de Aquascalientes, Departamento de Biologia",
"HUAL	s	Universidad de Almeria, Departamento de Biologia Vegetal y Ecologia",
"HUAP	s	Universidad Autonoma de Puebla, Ciudad Universitaria, Vicerrectoria de Investigacion y de Posgrado",
"HUB	s	Hacettepe University, Botany Department",
"HUBE	s	Golden West College, Biology/Life Sciences Department",
"HUBO	s	Universita degli Studi di Bologna",
"HUC	s	Universidad de Cordoba, Departamento de Biologia",
"HUCM	s	Hunan College of Traditional Chinese Medicine, Department of Chinese Materia Medica",
"HUCP	s	Pontifica Universidade Catolica do Parana, Departamento de Ciencias Biologicas",
"HUDC	s	Howard University, Biology Department",
"HUE	s	Hunan Education College, Biology Department",
"HUEF	s	Hacettepe Ueniversitesi",
"HUEFS	s	Universidade Estadual de Feira de Santana, Departamento de Ciencias Biologicas",
"HUEM	s	Universidade Estadual de Maringa, Departamento de Biologia",
"HUF	s	Hunan Forestry School",
"HUFU	s	Universidade Federal de Uberlandia, Instituto de Biologia",
"HUIC	s	Hacettepe University Ichthyological Collection",
"HUIF	s	Hunan Forestry Institute",
"HUJ	s	Hebrew University",
"HUJ:INV	s	Hebrew University, Invertebrate collection",
"HUKUK	c	Culture Collection of Animal Cells",
"HUL	s	Fine Arts Museum",
"HULE	s	Universidad Nacional Autonoma de Nicaragua, Departamento de Biologia",
"HUM	s	Humbolt University Zoologischen Museum",
"HUMO	s	Universidad Autonoma del Estado de Morelos",
"HUMP	s	Muzeum v Humpolci",
"HUMZ	s	Hokkaido University, Laboratory of Marine Zoology",
"HUPG	s	State University of Ponta Grossa, Departamento de Biologia",
"HUQ	s	Universidad del Quindio",
"HURG	s	Universidade do Rio Grande, Departamento de Ciencias Morfo-Biologicas",
"HUSA	s	Universidad Nacional de San Agustin de Arequipa, Facultad de Ciencias Biologicas y Agropecuarias, Area de Biomedicas",
"HUST	s	Hunan University of Science and Technology, School of Life Sciences",
"HUT<JPN>	c	HUT Culture Collection",
"HUT<PER>	s	Herbarium Truxillense, Universidad Nacional de La Libertad-Trujillo",
"HUTB	s	Hainan University",
"HUTM	s	Hunan Academy of Traditional Chinese Medicine and Pharmacy",
"HVR	s	Universidade de Tras-os-Montes e Alto Douro",
"HWA	s	Southwest Agricultural University, Department of Biological Basic Courses",
"HWB	s	Harrow School, Biology Department",
"HWBA	s	Benedictine College, Biology Department",
"HWD	s	Hollinwood Botanists' and Field Naturalists' Society",
"HXBH	s	Fundacao CETEC",
"HXC	s	Hendrix College, Biology Department",
"HY	s	Osmania University, Botany Department",
"HYD	s	Heywood and District Botanical Society",
"HYO	s	Museum of Nature and Human Activities",
"Hyogo	s	Museum of Nature and Human Activities",
"HZM	s	Museum of Natural History (Hrvatski Zooloski Muzej)",
"HZMZ	s	Hrvatski Narodni Zooloski Muzej",
"HZTC	s	Hanzhong Teachers College, Biology Department",
"HZU	s	Zhejiang University",
"I	s	Universitatea Al. I. Cuza Iasi",
"IA	s	University of Iowa, Department of Biological Sciences",
"IAA	s	Instituto Antarctico Argentinao, Direccion Nacional del Antartico",
"IAAA	s	Instituto do Acucar e do Alcool",
"IABH	s	Al-Bayt University, Biology Department",
"IAC	s	Instituto Agronomico de Campinas",
"IACC	s	Instituto Agronomico de Campinas",
"IACM	s	Instituto Agronomico",
"IADIZA-CM	s	Instituto Argentino de Investigaciones de las Zonas Aridas",
"IAFB	c	Collection of Industrial Microorganisms",
"IAGB	s	Universitatea Al. I. Cuza Iasi",
"IAL<BRA-Bahia>	s	EMBRAPA",
"IAL<BRA-SaoPaulo>	c	Secao de Colecao de Culturas",
"IALCEL	c	Secao de Culturas Celulares",
"IALMIC	c	Micoteca do Insituto Adolfo Lutz",
"IAM<ESP>	s	Instituto Arguelogico Municipal",
"IAM<JPN>	c	IAM Culture Collection, Center for Cellular and Molecular Research",
"IAN	s	Embrapa Amazonia Oriental",
"IAPG	s	Institute of Animal Physiology and Genetics, Academy of Sciences of the Czech Republic",
"IARI	s	Indian Agricultural Research Institute",
"IASI	s	Universitatea Agronomica, Disciplina de Botanica",
"IAV	s	Institut Agronomique et Veterinaire Hassan II, Departement d'Ecologie Vegetale",
"IAVH	s	Instituto de Ivestigacion de los Recursos Biologicos Alexander von Humboldt",
"IB	s	Universitaet Innsbruck",
"IBA<ESP>	s	Instituto Asturiano de Taxonomia y Ecologia Vegetal",
"IBA<POL>	c	Collection of Microorganisms Producing Antibiotics",
"IBAUNC	s	Universidad Nacional de Cuyo, Instituto de Biologia Animal",
"IBE	s	Institute for Botanical Exploration",
"IBEF	s	Museum of the Izumi Board of Education",
"IBF	s	Tiroler Landesmuseum Ferdinandeum",
"IBGE	s	Reserva Ecologica do IBGE",
"IBH	s	Universidad Nacional Autonoma de Mexico, Instituto de Biologia",
"IBI	s	Instituto Biologico, Laboratorio de Micologia",
"IBIR	s	Institutul Agronomic",
"IBIW	s	I. D. Papanin Institute for Biology of Inland Waters",
"IBK	s	Guangxi Institute of Botany",
"IBL<ISR>	s	Independent Biological Laboratories",
"IBL<PRT>	c	Botanical Institute, Lisbon Faculty of Sciences",
"IBL<USA-MD>	c	Insect Biocontrol Laboratory (USDA-ARS, Beltsville)",
"IBLP	s	Instytut Badawczy Lesnictwa",
"IBMUNC	s	Universidad Nacional de Cuyo, Instituto de Biologia Animal",
"IBP	s	Instituto de Biologia do Parana",
"IBRP<BRA>	s	Instituto Biologico de Ribeirao Preto",
"IBRP<JPN>	s	Institute for Breeding Research, Tokyo University of Agriculture",
"IBS	s	Irish Biogeographical Society",
"IBSBF	c	Biological Institute Culture Collection of Phytopathogenic Bacteria",
"IBSC	s	South China Botanical Garden",
"IBSD	s	Dinghushan Biosphere Reserve",
"IBSP	s	Instituto Biologico de Sao Paulo",
"IBT	c	IBT Culture Collection of Fungi, Mycology Group, Technical University of Denmark",
"IBTS	s	Institutul de Biologie, Tr. Savulescu",
"IBUG	s	Universidad de Guadalajara",
"IBUNAM	s	Universidad Nacional Autonoma de Mexico",
"IBUP	s	Institute of Biology",
"IBUS	s	Universidade Federal do Rio de Janeiro",
"IBUT	s	Instituto Butanta",
"IBY	s	Botanical Institute of Guangxi",
"ICA	s	Instituto Colombiano Agropecuario, Tibaitata",
"ICBB	c	ICBB Culture Collection for Microorganisms and Cell Culture",
"ICBU	s	Bishop's University, Natural History Museum",
"ICCF	c	Collection of Industrial Microorganisms",
"ICEB	s	Eurouniversity",
"ICEL	s	Icelandic Institute of Natural History",
"ICF	s	INIFAP",
"ICFC	c	IIB-INTECH Collection of Fungal Cultures",
"ICGC	s	Istituto Calasanzio",
"ICIS	s	Idaho Museum of Natural History",
"ICM	s	Instituto de Ciencias del Mar",
"ICMP	c	International Collection of Microorganisms from Plants",
"ICN<BRA>	s	Universidade Federal do Rio Grande do Sul, Departamento de Botanica",
"ICN<COL>	s	Instituto de Ciencias Naturales, Museo de Historia Natural",
"ICP	s	Islamia College, University of Peshawar, Botany Department",
"ICPB	c	International Collection of Phytopathogenic Bacteria",
"ICPPB	s	International Collection of Plant Pathogenic Bacteria",
"ICPR	s	Biological Control Research Institute",
"ICRC	s	Insect Control and Research",
"ICRG	s	Institute of Entomology",
"ICRI	s	Zhonghan (Sun Yat-Sen) University, Research Institute of Entomology",
"ICST	s	Imperial College of Science, Technology & Medicine, Department of Biological Sciences",
"ICUI	s	University of Iowa",
"ICVI	s	The Volcani Center",
"ID	s	University of Idaho, Biological Sciences Department",
"IDEA	s	Instituto de Agronomia",
"IDF	s	University of Idaho",
"IDS	s	Idaho State University, Biological Sciences Department",
"IE	c	Cepario de Hongos del Instituto de Ecologia",
"IEA	s	Instituto DI Entomologia Agraria",
"IEAB	s	Istituto di Entomologia Agraria dell'Universita",
"IEAM	s	Istituto di Entomologia dell'Universita degli Studi [= Istituto di Entomologia Agraria dell'Universita, Milan]",
"IEAP	s	Istituto di Entomologia Agraria dell'Universita",
"IEAPM	c	Instituto de Estudos do Mar \"Almirante Paulo Moreira\"",
"IEAS	s	Institute of Entomology",
"IEAU	s	Istituto di Entomologia Agraria dell'Universita",
"IEB	s	Instituto de Ecologia, A.C.",
"IEBC	c	International Entomopathogenic Bacillus Centre (WHO)",
"IEC	s	Centre D'Etude sur les Ressources Vegetales",
"IECA	s	Czech Academy of Science, Institute of Entomology",
"IEEUACH	s	Universidad Austral de Chile, Instituto de Ecologia y Evolucion",
"IEGG	s	Universita di Bologna, Istituto di Entomologia \"Guido Grandi\"",
"IEGM	c	Regional Specialized Collection of Alkanotrophic Microorganisms",
"IEI	s	Institut d'Estudis Ilerdencs",
"IEM	c	Czech National Collection of Type Cultures",
"IEME	s	Institute for Evolution, Morphology, and Ecology of Animals",
"IEMM	s	Instituto de Ecologia",
"IENU	s	Istituto di Entomologia, Universita degli Studi",
"IEPA	s	Istituto di Entomologia Agraria dell'Universita",
"IEUC	s	Istituto di Entomologia Agraria dell'Universita Cattolica",
"IEUP	s	Istituto di Entomologia, Universita degli Studi",
"IEVB	s	Institut de Zoologie [Institut Ed. Van Beneden]",
"IFAM	c	Institut fur Allgemeine Mikrobiologie",
"IFAN	s	Institut Fondamental d'Afrique Noire",
"IFBM	c	Streptokokken Sammlung",
"IFE	s	Obafemi Awolowo University, Botany Department",
"IFFB	s	Institut fuer Forstenentomologie und Forstschutz",
"IFGD	s	Idaho Fish and Game",
"IFGH	s	Idaho Fish and Game Department",
"IFGP	s	Pacific Southwest Research Station, USDA Forest Service",
"IFI	s	Imperial Fisheries Institute",
"IFM<AUS>	c	IFM Quality Services Pty Ltd",
"IFM<JPN>	c	Research Center for Pathogenic Fungi and Microbial Toxicoses, Chiba University",
"IFO	c	Institute for Fermentation",
"IFP	s	Institute of Applied Ecology, Academia Sinica",
"IFREMER	s	Institut Francais pour l'Etude de la Mer",
"IFRI	s	Indian Forest Research Institute",
"IFRPD	c	Institute of Food Research and Product Development, Kasetsart University",
"IFSA	s	Instituto Florestal",
"IG	s	Institute of Geology",
"IGC	c	Portuguese Yeast Culture Collection",
"IGCAGS	s	Institute of geology, Chinese Academy of Geological Science",
"IGCU	s	Instituto de Geologia, Ciudad Universitaria",
"IGESALQ	c	Colecao Microorganismos",
"IGF	s	Instituto di Geologia e Paleontologia",
"IGL	s	Institute fuer Geowissenschaften Leoben",
"IGM<MEX>	s	Instituto de Geologia",
"IGM<MNG>	s	Geological Institute, Mongolian Academy of Sciences",
"IGPH	s	Institut fuer Geologie und Palaeontologie der Universitat Hannover",
"IGUS	s	Universitat des Saarlandes",
"IGWH	s	Institut fuer Geowissenschaften aus dem Martin-Lurther- Universitat",
"IH	s	Instituto de Segunda Ensenanza de La Habana",
"IHASW	s	Institute of Hydrobiology, Academia Sinica",
"IHB	s	Institute of Hydrobiology, Chinese Academy of Sciences",
"IHCAS	s	Museum of Institute of Hydrobiology, Chainese Academy of Sciences",
"IHEM	c	Scientific Institute of Public Health, Mycology Section",
"IHUG	s	Institut fuer Hygiene der Iniversitaet",
"IIBM-UNAM	c	Industrial Culture Collection",
"IIBUV	s	Investigaciones Biologicas de la Universidad Veracruzana (Mexico)",
"IICT	s	Centro de Zoologia do I.I.C.T.",
"IID	c	Laboratory Culture Collection",
"IIES	s	Instituto Investigaciones Entomologicas Salta",
"IIPB	s	Instituto de Ciencias del Mar",
"IJ	s	Institute of Jamaica",
"IJFM	c	Instituto Jaime Ferran de Microbiologia Consejo Superior de Investigaciones Cientificas",
"IJSM	s	Institute of Jamaica, Natural History Museum",
"IK	s	Zoological Institute, Ukrainian Academy of Sciences",
"ILCA	s	International Livestock Research Institute",
"ILF	s	Iflracombe Museum",
"ILH	s	Iowa Lakeside Laboratory",
"ILL	s	University of Illinois, Plant Biology Department",
"ILLS	s	Illinois Natural History Survey",
"ILMA	s	Istituto Leonardo Murialdo",
"ILPLA	s	Museo de La Plata",
"IM<CUB>	s	Instituto de Segunda Ensenanza de Matanzas",
"IM<IND>	s	Indian Museum",
"IMAGE	b	The I.M.A.G.E Consortium",
"IMARPE	s	Instituto del Mar del Peru",
"IMC	s	Sichuan Academy of Traditional Chinese Medicine and Pharmacy",
"IMCC	c	Inha Microbe Culture Collection, Inha University",
"IMD	c	Industrial Microbiology Dublin",
"IMD<CHN>	s	Institute of Medicinal Plant Development, Chinese Academy of Medical Sciences",
"IMDC	s	Inner Mongolia Institute for Drug Control",
"IMDY	s	Chinese Academy of Medical Sciences, Yunnan Branch",
"IMET	c	National Kurturensammlung fuer Mikroorganismen",
"IMFA	s	Inner Mongolia Academy of Forestry",
"IMI	c	CABI Bioscience Genetic Resource Collection",
"IML	s	Instituto Miguel Lillo",
"IMLA	s	Fundacion e Instituto Miguel Lillo",
"IMM<CHN-Bejing>	s	Chinese Academy of Medical Science",
"IMM<CHN-Hohhot>	s	Inner Mongolian Museum",
"IMMH	c	Collection of Animal Viruses",
"IMPC	s	Sichuan Academy of Traditional Chinese Medicine and Pharmacy",
"IMRU	c	Waksman Institute of Microbiology",
"IMS<TZA>	s	Institute of Marine Sciences",
"IMS<USA-NC>	s	University of North Carolina at Chapel Hill",
"IMSNU	c	Institute of Microbiology, Seoul National University",
"IMSSM	s	Instituto Mexicano del Seguro Social",
"IMT<BRA>	c	Micoteca do Instituto de Medicina Tropical de Sao Paulo",
"IMT<JPN>	s	Imperial Museum",
"IMTU	s	Institute of Medicine, Community Medicine Department",
"IMUFRJ	c	Instituto de Microbiologia of Universidade Federal do Rio de Janeiro",
"IMVS	c	IMVS Culture Collection",
"IMYZA	c	Instituto de Microbiologia y Zoologia Agricola",
"IN	s	Zoological Institute, Russian Academy of Sciences",
"INA<PRT>	s	Instituto Nun'Alvres, Departamento de Biologia",
"INA<RUS>	c	Culture Collection of the Institute of New Antibiotics",
"INALI	s	Instituto Limnologia",
"INB	s	Instituto Nacional de Biodiversidad, Departamento de Botanica",
"INBC	s	Instituto Nacional de Biodiversidad (INBio)",
"INBio	s	National Biodiversity Institute, Costa Rica",
"INBP	s	Inventorio Biologico Nacional [Museo Nacional de Historia Natural del Paraguay]",
"INC	s	Instituto Nacional de Cultura, Museo de Ciencias Naturales (Panama)",
"INCQS	c	Fundacao Oswaldo Cruz-FIOCRUZ",
"IND	s	Indiana University, Department of Biology",
"IND-AN	s	Amphibian Collection",
"IND-M	s	La Unidad de Investigacion \"Federico Medem\"-Inderena (Colombia)",
"INDRE	c	Pathogen Fungi and Actinomycetes Collection",
"INEGI	s	Instituto Nacional de Estadistica Geografia e Informatica, Departamento de Botanica",
"INER	s	Istituto Nazionale di Entomologia",
"INFYB	s	Instituto Nacional de Farmacologia y Bromatologia, Farmacobotanica",
"INH	s	Institut National d'Horticulture, Departement de Sciences Biologiques",
"INHM	s	Iraq Natural History Museum",
"INHS	s	Illinois Natural History Survey",
"INIA	s	Instituto Nacional de Investigaciones Forestales y Agropecurias",
"INIBP	s	Instituto Nacional de Investigaciones Biologico Pesqueras",
"INIF	c	Coleccion de Microhongos",
"INIFAT	c	INIFAT Fungus Collection",
"INIP	s	Instituto Nacional de Investigacao das Pescas",
"INIR	s	Coleccion de Termitas Mexicanas",
"INLA	s	INIA Subestacion Experimental Control Biologico La Cruz",
"INMI	c	Institute of Microbiology, Russian Academy of Sciences",
"INPA	c	Laboratorio de Micologia Medica Divisao de Microbiologia e Nutricao",
"INPC	s	National Pusa Collections",
"INRA	c	Microbiologie des Sols",
"INV	s	Inverness Museum and Art Gallery",
"INVA	s	Invergordon Academy",
"INVAM	c	International Culture Collection of (Vesicular) Arbuscular Mycorrhizal Fungi",
"INVEMAR	s	Instituto de Investigaciones Marinas de Punta de Betin",
"IO	s	Instituto Oceanografico da Universidade de Sao Paulo",
"IOAN	s	Shirshov Institute of Oceanography",
"IOC	c	Colecao de Culturas de Fungos do Instituto Oswaldo Cruz",
"IOCAS	s	Institute of Oceanology, Chinese Academy of Scineces",
"IOEB	c	Bacterial Collection",
"IOH	s	Academia de Ciencias",
"IOM	s	Institute of Oceanology, Academy of Sciences",
"IOPM	s	Izu Oceanic Park Museum",
"IORD	s	Takai University, Institute of Oceanic Research and Development",
"IOS	s	Institute of Oceanographic Sciences",
"IOUSP	c	Marine Microalgae Culture Collection",
"IOWA	s	University of Iowa, Museum of Natural History",
"IPA	s	Empresa Pernambucana de Pesquisa Agropecuaria, IPA",
"IPB<BRA>	s	Instituto Paranaense de Botanica",
"IPB<DEU>	s	Institut fuer Palaeontologie",
"IPBIR	b	Integrated Primate Biomaterials and Information Resource",
"IPCN	s	Instituto Patagonico de Ciencias Naturales",
"IPEF	s	Institut fuer Pflanzenschutzforschung",
"IPFUB	s	Institute for Paleontology of the Freie Universitat",
"IPHG	s	Institut fur Palaeontologie und Historische Geologie",
"IPK	s	Institut fuer Pflanzenschutzforschung",
"IPKUP	s	Paleontologiscsky Institute",
"IPMC	s	Institut Paleontologic Dr. M. Crusfafont Sabadell",
"IPMGO	s	Institut fur Paleontologie und Museum, Gottingen Universitat",
"IPMY	s	Universidad Pedagogica Libertador, Nucleo Maracay, Departamento de Biologia",
"IPN	s	Instituto Politecnico Nacional",
"IPO	s	Plant Research International",
"IPPAS	c	Culture Collection of Microalgae IPPAS",
"IPRN	s	Instituto de Pesquisas de Recursos Naturais Renovaveis",
"IPS	s	Ipswich Museum",
"IPSLI	s	Ipswich Literary Institute",
"IPSM	s	Ipswich Museum",
"IPSN	s	Instituto de Paleontologia de Sabadella",
"IPT	c	Agrupamento de Biotecnologia, Culture Collection of Microorganisms",
"IPTB	s	Instituto de Pesquisas Tecnologicas",
"IPUW	s	Institut fuer Palaeontologie der Universitaet Wien",
"IPW	s	Instutut fuer Palaeontologie der Uinversitaet Wurzburg",
"IRAG	s	Institut National de la Recherche Agronomique de Antilles et Guyane",
"IRAN	s	Plant Pests and Diseases Research Institute, Botany Department",
"IRBR	s	Universidad de Oriente, Departamento de Biologia",
"IRCW	s	Madison, University of Wisconsin",
"IRDA	s	Agriculture Department",
"IREC	s	Institut de Recherches Entomologique de la Caribe",
"IRGC	b	International Rice Genebank Collection",
"IRK	s	Siberian Institute of Plant Physiology and Biochemistry",
"IRKU	s	Irkutsk State University, Department of Botany and Genetics",
"IRP	s	Isle Royale National Park",
"IRRI	s	International Rice Research Institute",
"IRSAC	s	Institute de Recherche Scientific en Afrique Centrale",
"IRSC	s	Institut de Recherches Scientifiques au Congo",
"IRSM	s	Institut Recherche Scientifique de Madagascar",
"IRSN	s	Institut Royal des Sciences Naturelles de Belgique",
"IRSNB	s	Institut Royal des Sciences Naturelles de Belgique",
"IRVC	s	University of California, UCI Arboretum Mail",
"IS	s	University of Molise, Department of Science and Technology for Environment and Territory",
"ISAB	s	Istituto Sant'Antonio dei Padri Francescani",
"ISAR	s	Academy of Science",
"ISAS	s	Kunming Institute of Zoology",
"ISB	s	Institute of Spelology \"Emile Racovita\"",
"ISBB	s	Institutul de Stiinte Biologice",
"ISBC	s	Institute of Soil Biology, Academy of Science of the Czech Republic",
"ISC<FRA>	c	International Salmonella Centre (W.H.O.)",
"ISC<USA-IA>	s	Iowa State University, Botany Department",
"ISCM	s	Institut Scientifique Cheripen",
"ISEAK	s	Instytut Systematyki i Ewolucji Zwierz",
"ISER	s	Institutul Speologie Emil G. Racovita",
"ISH	s	Institut fuer Seefischerei",
"ISI	s	Geological Museum, Indian Statistical Institute",
"ISIS	s	Naturforschende Gesellschaft Isis",
"ISL	s	Quaid-I-Azam University, Biological Sciences Department",
"ISM	s	Illinois State Museum",
"ISMC	s	Indiana Department of Natural Resources",
"ISNHC	s	State Historical Society of Iowa",
"ISNP	s	Istituto Sperimentale per la Nutrizione delle Piante",
"ISP	c	International Cooperative Project for Description and Deposition of Type Cultures",
"ISRA	s	Royal Academy",
"ISRI	c	Indonesian Sugar Research Institute, Pusat Penelitian Perkebunan Gula Indonesia",
"ISS	c	Collection of Bacteria",
"ISTC	s	University of Northern Iowa, Biology Department",
"ISTE	s	University of Istanbul, Department of Pharmaceutical Botany",
"ISTF	s	Istanbul University, Botany Department",
"ISTO	s	University of Istanbul, Orman Fakueltesi",
"ISTPM	s	Institut Scientifique et Technique des Peches Maritimes",
"ISU<USA-IL>	s	Illinois State University, Biological Sciences Department",
"ISU<USA-IN>	s	Indiana State University",
"ISUC	s	Normal, Illinois State University",
"ISUI	s	Iowa State University",
"ISUVC	s	Indiana State University",
"ISZA	s	Istituto Sperimentale per la Zoologia Agraria",
"ISZP	s	Institute of Systematic Zoology",
"ITAE	s	Istituto Tecnico Agrario Enologico",
"ITAL	c	Banco de Fermentos Lacticos",
"ITALSL	c	Secao de Leite e Derivados",
"ITALSM	c	Secao de Microbiologia",
"ITBCC	c	Institute of Technology Bandung Culture Collection",
"ITC	b	International Transit Centre",
"ITCC<IND>	c	Indian Type Culture Collection",
"ITCC<ITA>	s	Istituto Tecnico Stattale \"Camillo Cavour\"",
"ITCO	s	Istituto Tecnico Commerciale \"Oronzio Gabriele Costa\"",
"ITCV	s	Instituto Tecnologico de Ciudad Victoria, Departamento de Micologia",
"ITD	c	Coleccion de Cepas Microbianas",
"ITDI	c	Industrial Technology Development Institute",
"ITG	c	ITG",
"ITH	c	W.H.O./F.A.O. Collaborating Centre for Reference and Research on Leptospirosis",
"ITHA	s	Instituut voor Tropische Hygiene",
"ITIC	s	Universidad de El Salvador, Escuela de Biologia",
"ITLJ	s	National Institute of Agro-environmental Sciences",
"ITMM	s	Instituto Tecnologico de Monterrey",
"IU	s	Indiana University",
"IUI	s	Inha University, Biology Department",
"IUIC	s	Indiana University",
"IUK	s	Universite de Kinshasa, Departement de Biologie",
"IUM	s	Iwate University, Biology Department",
"IUP	s	Indiana University of Pennsylvania, Biology Department",
"IUQ	s	Laboratorio de Ictiologia",
"IVAU	s	Instituut Voor Aardwetenschappen",
"IVF	s	Chinese Academy of Agricultural Sciences",
"IVPP	s	Institute of Vertebrate Paleontology and Paleoanthropology",
"IZ<BRA>	c	Departamento de Tecnologia Rural",
"IZ<CUB>	s	Instituto de Zoologia",
"IZ<KAZ>	s	Institute of Zoology",
"IZ<TUR>	s	Aegean Agricultural Research Institute, Department of Plant Genetic Resources",
"IZA	s	Universita di l'Aguila, Instituto di Zoologia",
"IZAC<ARG>	s	Universidad Nacional de La Rioja-Sede Chamical",
"IZAC<CUB>	s	Academia de Ciencias de Cuba, Instituto de Zoologia",
"IZAS	s	Institut Zoologii Akademii Nauk Ukraini - Institute of Zoology of the Academy of Sciences of Ukraine",
"IZASK	s	Institue of Zoology of the Kazakh Academy of Sciences",
"IZBE	s	Institute of Zoology and Botany",
"IZBT	s	L'Institut de Zoologie et Botanique de Tartu",
"IZCAS	s	Institute of Zoology, Chinese Academy of Sciences",
"IZCR	s	Istituto di Zoologia",
"IZEF	s	Ege Ueniversitesi, Farmasoetik Botanik Kuersuesue",
"IZGAS	s	Georgian Academy of Sciences, Insititute of Zoology",
"IZPAN	s	Zoological Institute, Polish Academy of Sciences",
"IZPC	s	Universidade do Porto",
"IZSI	s	Istituto di Zoologia",
"IZTA	s	Universidad Nacional Autonoma de Mexico, Iztacala, Jefatura de Biologia",
"IZUA	s	Universidad Austral de Chile, Instituto de Zoologia",
"IZUC	s	Universidad de Concepcion, Instituto de Zoologia",
"IZUCS	s	Universita DI Cagliari",
"IZUE	s	Universitat -Erlangen-Nurnberg",
"IZUG	s	Istituto di Zoologia dell'Universita",
"IZUI	s	Institut fuer Zoologie der Universitat Innsbruck",
"IZUM	s	Istituto di Zoologia dell'Universita",
"IZUP	s	Istituto di Zoologia dell'Universita",
"IZUW	s	Institut fuer Zoologie der Universitat Wien",
"IZW	s	Institut Zoologii",
"IZWU	s	Paleozology Department, Institute of Zoology, Worclaw University",
"J	s	University of the Witwatersrand, Botany Department",
"JA	s	Consejeria de Medio Ambiente (Junta de Andalucia), Direccion General de Gestion del Medio Natural",
"JAC	s	University of Jodhpur, Botany Department",
"JACA	s	Instituto Pirenaico de Ecologia, CSIC",
"JAEN	s	Universidad de Jaen, Botanica",
"JAS	s	Jiangxi Academy of Sciences",
"JATH	s	University of Szeged",
"JAUM	s	Jardin Botanico Joaquin Antonio Uribe",
"JAY	s	Fondation Cognacq-Jay",
"JBAG	s	Jardin Botanico Atlantico, Ayuntamiento de Gijon",
"JBG	s	Johannesburg Botanic Garden",
"JBGP	s	Jardin Botanico Guillermo Pineres",
"JBSD	s	Jardin Botanico Nacional Dr. Rafael M. Moscoso",
"JBVN	s	Jardin Botanique de la Ville de Nice, Service des Espaces Verts",
"JBWM	s	J.B. Wallis Museum of Entomology",
"JCB	s	St. Joseph's College",
"JCE	s	Jiangxi College of Education, Biology Department",
"JCM	c	Japan Collection of Microorganisms",
"JCT	c	James Cook Townsville",
"JE	s	Friedrich-Schiller-Universitaet Jena",
"JEF	s	Indiana University Southeast, Biology Department",
"JEL	s	Latvian Agricultural Academy, Plant Protection Department",
"JEPS	s	University of California",
"JESW	s	John Evelyn Society's Museum",
"JEY	s	Boys' Grammar School, Victoria College",
"JF	s	Jonkershoek Forestry Research Centre, Environment Affairs Department",
"JFBM	s	James Ford Bell Museum of Natural History",
"JHH	s	New York State Herbarium",
"JHWU	s	Wittenberg University, Biology Department",
"JIC	b	John Innes Centre",
"JII	s	John Innes Institute",
"JIU	s	Jishou University, Biology Department",
"JJF	s	Jiujiang Forestry Institute",
"JJT	s	Jiujiang Teachers College, Biology Department",
"JJU	s	Jeonju University",
"JLFC	s	Forestry College of Beihua University, Forestry Department",
"JLMP	s	Jilin Academy of Traditional Chinese Medicine and Materia Medica",
"JM	s	Jura Museum, Eichstatt",
"JMM	s	Earlham College, Joseph Moore Museum",
"JMSMC	s	Jiamusi Medical College, Department of Pharmacy",
"JMUH	s	James Madison University, Department of Biology",
"JN	s	Jinggang Mountain Nature Reserve",
"JNR	s	Jiulian Mountain Nature Reserve, Administration Department",
"JNU<CHN>	s	Ji Nan University",
"JNU<KOR>	s	Chonbuk National University, Faculty of Biological Sciences",
"JOE	s	University of Joensuu, Biology Department",
"JONK	s	Jonkershoek Herbarium",
"JP	s	Phyletisches Museum Jena",
"JPB	s	Universidade Federal da Paraiba, Cidade Universitaria, Departamento de Sistematica e Ecologia",
"JPMP	s	Janus Pannonius Museum",
"JPU	s	Janus Pannonius University, Botany Department",
"JRAU	s	University of Johannesburg, Department of Botany and Plant Biotechnology",
"JRY	s	Jersey College for Girls",
"JSHC	s	Jay S. Haft Collection",
"JSMPE	s	Joint Soviet Mongolian Paleontolgical Expedition",
"JSPC<CHN>	s	Shandong University, Biology Department",
"JSPC<CZE>	s	J. Rusek Collection",
"JSU	s	Jacksonville State University, Biology Department",
"JSY	s	Museum and Art Gallery of La Societe Jersiaise",
"JTNM	s	Joshua Tree National Monument",
"JTPC	s	Colorado Entomological Museum (formerly John T. Polhemus collection)",
"JU	s	Jinan University",
"JUA	s	Universidad Nacional de Jujuy, Facultad de Ciencias Agrarias",
"JUG	s	Jiwaji University, Botany Department",
"JUJ	s	Hebei Agricultural University",
"JVC	s	Jardin Botanico Canario Viera y Clavijo",
"JVR	s	Universidad Nacional",
"JXAU	s	Jiangxi Agricultural University, Forestry Department",
"JXCM	s	Jiangxi College of Traditional Chinese Medicine, Pharmacy Department",
"JXF	s	Jiangxi Forestry Institute",
"JXM	s	Jiangxi Institute of Materia Medica",
"JXU	s	Jiangxi University, Biology Department",
"JYV	s	University of Jyvaeskylae, Natural History Department",
"K	s	Royal Botanic Gardens",
"KA	s	Vytautas Magnus University",
"KABA	s	University of Kabul",
"KACC	c	Korean Agricultural Culture Collection, National Institute of Agricultural Biotechnology",
"KAG	s	Kagoshima University Museum",
"KAGS	s	Kagoshima University, Biology Department",
"KAI	s	Henan University, Biology Department",
"KAMA	s	Yokohama University",
"KANA	s	Kanazawa University",
"KANU	s	University of Kansas",
"KAR	s	University of Tehran, Horticulture Department",
"KARI	s	Kenya Agricultural Research Institute",
"KARS	s	Kawanda Agricultural Research Station",
"KAS	s	Universitaet Gesamthochschule Kassel, Morphologie und Systematik der Pflanzen",
"KASH	s	University of Kashmir",
"KASSEL	s	Naturkundemuseum im Ottoneum",
"KATH	s	Department of Plant Resources",
"KATO	s	Karadeniz Technical University, Department of Forest Botany",
"KAW	s	Kawanda Research Station, Department of Agriculture",
"KAZ	s	Kazan State University",
"KB	s	National Biological Resources Center, Department of Biological Sciences",
"KBAI	s	Kuban Agricultural State University, Department of Biology and Ecology",
"KBHG	s	H. M. Berbekov Kabardian-Balkarian State University, Department of Botany",
"KBRYO	s	Centre College of Kentucky",
"KBSMS	s	Kellogg Biological Station, Michigan State University",
"KBT	s	Stewarty Museum",
"KCCM	c	Korean Culture Center of Microorganisms, Department of Food Engineering",
"KCFS	s	King's College",
"KCK	s	Dick Institute",
"KCLB	c	Korean Cell Line Bank",
"KCS	s	University of London, King's College, Botany Department",
"KCTC	c	Korean Collection for Type Cultures",
"KDL	s	Kendal Museum",
"KDR	s	Bewdley Museum",
"KE	s	Kent State University, Biological Sciences Department",
"KEF	s	Kunming Edible Fungi Institute",
"KEI	s	University of Transkei, Botany Department",
"KEIU	s	Korea University",
"KEM	s	University of Kemerovo, Department of Botany",
"KEMH	c	KEMH/PMH Culture collection",
"KEN	s	Longwood Gardens, Horticulture Department",
"KEND	s	Kendal Natural History Society",
"KEP	s	Forest Research Institute Malaysia",
"KESC	s	Keene State College, Department of Biology",
"KEVO	s	University of Turku",
"KFCC	c	Korean Federation of Culture Collection",
"KFI	s	Hongnung Arboretum, Silviculture Department",
"KFRI	s	Kerala Forest Research Institute",
"KFRS	s	Kanudi Fisheries Research Station",
"KFTA	s	Saint Petersburg State S. M. Kirov Forest Technology Academy, Botany and Dendrology Department",
"KFUH	s	King Faisal University, Chemistry and Botany Department",
"KGU	s	Geology and Mineralogy Museum",
"KGY	s	Cliffe Castle Art Gallery and Museum",
"KH	s	Korea National Arboretum",
"KHA	s	Institute for Water and Ecology Problems, Far East Branch, Russian Academy of Sciences, Laboratory of Plant Ecology",
"KHD	s	Denver Botanic Gardens",
"KHER	s	Kherson Pedagogical University, Botany Department",
"KHF	s	Forest Research and Education Institute, Soba",
"KHMM	s	Kultur Historisches Museum",
"KHMS	s	Muzeum `umavy",
"KHOR	s	Pamir Biological Institute",
"KHU	s	University of Khartoum, Botany Department",
"KHUG	s	Aussenstelle der Universitaet",
"KHUS	s	Kyung Hee University, Biology Department",
"KIEL	s	Christian-Albrechts-Universitaet Kiel",
"KIFB	s	Korean Institute of Freshwater Biology",
"KIRI	s	University of Rhode Island, Department of Biological Sciences",
"KIT	c	Laboratorium voor Tropische Hygiene",
"KIUJ	s	Kyusu University",
"KIZ	s	Kunming Institute of Zoology, Chinese Academy of Sciences",
"KKM	s	Kirkleatham Museum",
"KKU	s	Herbarium, Department of Biology, Khon Kaen University",
"KKUA	s	Khon Kaen University",
"KKUK	s	Kon-Kuk University",
"KL	s	Landesmuseum fuer Kaernten",
"KLA	s	Department of Agriculture",
"KLE	s	University of Keele, Biological Sciences Department",
"KLGU	s	Kaliningrad State University, Department of Botany and Plant Ecology",
"KLH	s	K. E. Tsiolkovsky Kaluga State Pedagogical University, Department of Botany and Ecology",
"KLN	s	Lynn Museum and Art Gallery",
"KLU	s	University of Malaya",
"KM<CAN>	s	Kelowna Museum",
"KM<CZE>	s	Sprava Krkonoaskeho narodniho parku",
"KM<POL>	s	Muzeum Przyrodnicze Uniwersytetu Jagiellonskiego",
"KM<RUS>	s	Kotel'nich Museum",
"KMG	s	McGregor Museum",
"KMK<MOL>	s	Kraevedscheskii Musei Kishineva - Museum of Regional Study",
"KMK<ZAF>	s	Kaffarian Museum",
"KMKV	s	Karlovarske muzeum",
"KMM	c	Collection of Marine Microorganisms",
"KMMA	s	Royal Museum for Central Africa",
"KMMCC	c	Korea Marine Microalgae Culture Center",
"KMN	s	Adger Museum of Natural History and Botanical Garden, Botany Department",
"KMNH	s	Kitakyushu Museum and Institute of Natural History, Botany Department",
"KMU	s	Karl-Marx-Universitat Leipzeg",
"KMV	s	Kunming Municipal Museum",
"KMVC	s	Krajske Muzeum Vychodnich Cech",
"KNFY	s	Klamath National Forest, Resources Department",
"KNHM	s	The Educational Science Museum [=Kuwait Natural History Museum?]",
"KNK	s	Northern Kentucky University, Department of Biological Sciences",
"KNL	s	Kinneil Museum",
"KNOX	s	Knox College, Department of Biology",
"KNP	s	South African National Parks, Scientific Services",
"KNU	s	Kyungpook National University, Biology Department",
"KNUC	s	Kangweon National University",
"KNUH	s	Kyung-Nam University, Biology Department",
"KNY	s	Literary and Scientific Institution of Kilkenny",
"KNYA	s	Selcuk Ueniversitesi, Biyoloji Boeluemue",
"KO	s	P. J. Safarik University",
"KOCH	s	Kochi University, Department of Natural Environmental Science",
"KOELN	s	Universitaet Koeln, Arbeitsgruppe Geobotanik und Phytotaxonomie",
"KONL	s	Bodensee-Naturmuseum",
"KOR	s	Institute of Dendrology",
"KOS	c	Collection of Salmonella Microorganisms",
"KPABG	s	Polar-Alpine Botanical Garden-Institute, Department of Flora and Vegetation",
"KPBG	s	Kings Park and Botanic Garden, Botanic Gardens and Parks Authority",
"KPE	s	Kyungpook Earth, Kyungpook National University",
"KPM	s	Kanagawa Prefectural Museum of Natural History",
"KPM-NI	s	Kanagawa Prefectural Museum of Natural History",
"KPMC	s	Kalamazoo, Kalamazoo Public Museum",
"KR	s	Staatliches Museum fuer Naturkunde Karlsruhe",
"KRA	s	Jagiellonian University",
"KRAM	s	Polish Academy of Sciences, Department of Plant Systematics",
"KRAS	s	Krasnoyarsk State Pedagogical University, Department of Botany",
"KRDY	s	Kirkcaldy Museum and Art Gallery",
"KRF	s	V. N. Sukachev Institute of Forest and Wood",
"KRG	s	Westfield Museum",
"KRIB	s	Korea Research Institute of Bioscience and Biotechnology, Plant Diversity Research Center",
"KRMS	s	Sternwarte Kremsmuenster, Stift",
"KRSF	s	Koronivia Research Station",
"KRSU	s	Krasnoyarsk State University, Department of Biogeocoenology",
"KSAN	s	South African National Parks",
"KSBS	s	Lawrence, University of Kansas, State Biological Survey of Kansas",
"KSC	s	Kansas State University",
"KSEM	s	Kansas Snow Entomological Museum",
"KSHS	s	Kochi Senior High School",
"KSK	s	Fitz Part Museum and Art Gallery",
"KSO	s	Tweedside Physical and Antiquarian Society Museum",
"KSP	s	Pittsburg State University, Biology Department",
"KSRV	s	Khosrov State Reserve",
"KSTC	s	Emporia State University",
"KSU	s	King Saud University, Botany and Microbiology Department",
"KSUC	s	Kansas State University",
"KSUP	s	King Saud University",
"KTC	s	Pedagogical University, Botany Department",
"KTG	s	Kettering and District Naturalists' Society and Field Club",
"KTU	s	University of Silesia, Department of Plant Systematics",
"KTUH	s	Kuwait University, Botany and Microbiology Department",
"KU	s	University of Kansas, Museum of Natural History",
"KU:I	s	University of Kansas, Museum of Natural History, Ichthyology collection",
"KU:IT	s	University of Kansas, Museum of Natural History, Ichthyology tissue collection",
"KU<CHN>	s	Kwangsi University",
"KUBL	s	Yoshida College, Biological Laboratory",
"KUEC	s	Kyushu University",
"KUEL	s	Kyushu University, Entomology Laboratory",
"KUFC	c	Kasetsart University Fungus Collection, Department of Plant Pathology, Faculty of Agriculture",
"KUH	s	University of Karachi, Botany Department",
"KUHE	s	Kyoto University, Graduate School of Human and Environmental Studies",
"KUIC	s	Kagoshima University",
"KUKENS	c	Centre for Research and Application of Culture Collections of Microorganisms",
"KUKI	s	Kurukshetra University",
"KUM	s	Resource Management Support Center",
"KUMA	s	Kumamoto University, Biology Department",
"KUMF	s	Kasetsart University Museum of Fisheries",
"KUN	s	Kunming Institute of Botany, Chinese Academy of Sciences",
"KUNE	s	Kunming Institute of Ecology, Academia Sinica",
"KUO	s	Kuopio Natural History Museum",
"KURS	s	Kursk State University, Department of Botany",
"KUS	s	Korea University, Biology Department",
"KUU	s	University of Science and Technology",
"KUZC	s	Kyushu University, Zoological Collection",
"KVCH	s	Kivach Nature Reserve",
"KW	s	National Academy of Sciences of Ukraine",
"KWHA	s	Ukrainian National Academy of Sciences",
"KWNU	s	Kangwon National University, Biology Department",
"KWP	s	Kenelm W. Philip Collection, University of Alaska Museum of the North",
"KWP:Ento	s	Kenelm W. Philip Collection, University of Alaska Museum of the North, Lepidoptera collection",
"KY	s	University of Kentucky",
"KYM	s	University of Helsinki, Kymenlaakso Society of Naturalists",
"KYO	s	Kyoto University, Botany Department",
"L	s	Nationaal Herbarium Nederland, Leiden University branch",
"LA	s	University of California",
"LAC	s	Xian Institute of Lacquer",
"LACM	s	Natural History Museum of Los Angeles County",
"LAE	s	Papua New Guinea Forest Research Institute",
"LAF	s	University of Louisiana at Lafayette, Biology Department",
"LAGO	s	Pacific Northwest Forest and Range Experiment Station",
"LAGU	s	Asociacion Jardin Botanico La Laguna, Urbanizacion Plan de La Laguna",
"LAH	s	University of the Punjab, Botany Department",
"LAJC	s	Otero Junior College, Biology Department",
"LAM	s	Natural History Museum of Los Angeles County, Botanical Studies",
"LAME	s	Lake Mead National Recreation Area",
"LAMU	s	Lamar University, Biology Department",
"LAN	s	Lancing College, Biology Department",
"LANC	s	University of Lancaster, Department of Biological Sciences",
"LAPC	s	Pierce College, Life Sciences Department",
"LAPL	s	Lapland State Biosphere Reserve",
"LARRI	s	Living Aquatic Resources Research Institute",
"LASCA	s	The Los Angeles County Arboretum",
"LAT	s	Saint Vincent College, Biology Department",
"LATV	s	University of Latvia, Laboratory of Botany",
"LAU	s	Musee et Jardins Botaniques Cantonaux",
"LAUN	s	Launceston College",
"LAUS	s	Launceston Museum",
"LAV	s	Dauntsey's School",
"LAVO	s	Lassen Volcanic National Park",
"LBC	s	University of the Philippines at Los Banos",
"LBG<CHE>	c	Institute for Agricultural Bacteriology and Fermentation Biology",
"LBG<CHN>	s	Lushan Botanical Garden",
"LBIT	s	Laboratoire de Biologie des Insectes",
"LBL	s	M. Curie-Sklodowska University, Department of Biology and Earth Science",
"LBLC	s	M. Curie-Sklodowska University",
"LBM<BRA>	c	Laboratorio de Biologia Molecula Depto de Biologia Celular",
"LBM<JPN>	s	Lake Biwa Museum",
"LBN	s	Lembaga Biologi Nasional",
"LBRP	s	Laboratorio de Biodiversidade de Recursos Pesqueiros",
"LBUCH	s	Laboratorio de Biologia",
"LBV	s	CENAREST",
"LCC<CAN>	c	Labatt Culture Collection, Technology Development",
"LCC<POL>	c	University of Warmia and Mazury in Olsztyn",
"LCDI	s	Luther College, Biology Department",
"LCEU	s	Lane Community College",
"LCF	s	I.N.T.A.",
"LCFM	s	Musee d'Histoire Naturelle",
"LCG	s	Leamington College for Girls",
"LCH	s	Letchworth Museum and Art Gallery",
"LCM	s	Universidad de Chile, Laboratorio de Citogenetica de Mamiferos",
"LCMI	s	Loyola College",
"LCN	s	Lincoln City and County Museum",
"LCO	s	International Red Locust Control Organization for Central and Southern Africa",
"LCP	c	Fungal Strain Collection, Laboratory of Cryptogamy",
"LCR	s	Ratcliffe College",
"LCS	s	International Red Locust Control Service",
"LCU	s	Catholic University of America",
"LCVA	s	Lakeland College, Environmental Sciences Department",
"LD	s	Botanical Museum",
"LDL	s	Ludlow Museum",
"LDM	s	Latvian Natural Histotry Museum, department of Entomology",
"LDPC	s	L. Deharveng, Universite Paul Sabatier",
"LDS	s	University of Leeds",
"LDSN	s	Leeds Naturalists' Club",
"LDSP	s	Leeds Philosophical and Literary Society Museum",
"LE<BRA>	c	Servico de Microbiologia e Imunologia",
"LE<RUS>	s	V. L. Komarov Botanical Institute",
"LEA	s	University of Lethbridge, Biological Sciences Department",
"LEB<ESP>	s	Universidad de Leon, Departamento de Biologia, Botanica",
"LEB<LVA>	s	Entomological Society of Latvia",
"LEC	s	Universita degli Studi di Lecce, Dipartimento di Biologia",
"LECB	s	Saint Petersburg State University, Botany Department",
"LEF	s	Economic Forestry Institute of Liaoning Province",
"LEI	s	Leicester Literary and Philosophical Society",
"LeishCryoBank	c	International Cryobank of Leishmania",
"LEIUG	s	Department of Geology Leicester University",
"LEMA	s	Universite du Maine",
"LEMQ	s	McGill University, Lyman Entomological Museum",
"LEMT	s	Ege University, Lodos Entomological Museum",
"LENUD	s	Lenin University of Dagestan, Botany Department",
"LEP	s	All-Union Institute for Plant Protection",
"LES	s	Leeds City Museum, Natural History Department",
"LET	s	Letchworth Naturalist's Society",
"LEUN	s	Bischoefliches Gymnasium Josephinum",
"LEV	s	Ministry of Agriculture and Fisheries, Plant Protection Centre",
"LEYN	s	Leyton Reference Library",
"LFBKU	s	Laboratory of Fishery Biology",
"LFCC	s	Lord Fairfax Community College, Natural Resources Department",
"LFG	s	Centre de Recherche de la Nature, des Forets et du Bois",
"LFM	s	Merseyside County Museums (formerly Liverpool Free Museum)",
"LG	s	Universite de Liege, Departement de Botanique",
"LGBH	s	Loughborough Public Library",
"LGHF	s	Universite de Liege",
"LGICT	s	Laboratoire de Geologie de l'Institut Catholique de Toulouse",
"LGM	s	Leningrad School of Mines",
"LGM-USP	c	Departamento de Microbiologia Lab. de Genetica de Microrganismos",
"LGO	s	Columbia University",
"LGPUT	s	Laboratory of Geology and Palaeontology",
"LGU	s	Leningrad State University",
"LHT	s	Lahti City Museum",
"LI	s	Oberoesterreichischen Landesmuseums, Botanische Abteilung",
"LI:EVAR	s	Oberoesterreichischen Landesmuseums, Botanische Abteilung, Evertebrata Varia (Invertebrates other than Insects)",
"LI:INS	s	Oberoesterreichischen Landesmuseums, Botanische Abteilung, Insect collection",
"LI:VERT	s	Oberoesterreichischen Landesmuseums, Botanische Abteilung, Vertebrate collection",
"LIA	c	Cryobank of Microorganisms",
"LIAIP	s	Laboratory of Ichthyology",
"LIB	s	University of Liberia",
"LICPP	s	The Crown Prince's Palace",
"LIG	s	Sociedade de Geografia de Lisboa",
"LIH-UNAM	c	Culture Collection of Histoplasma capsulatum Strains from the Fungal Immunology Laboratory of the Department of Microbiology and Parasitology, Faculty of Medicine, UNAM",
"LIHUBA	s	Universidad de Buenos Aires, Laboratorio de Investigaciones Herpetologicas",
"LIL	s	Fundacion Miguel Lillo",
"LILLE	s	Institut Catholique de Lille, Laboratoire de Biologie Vegetale",
"LIM	s	Severoceske muzeum",
"LIMFC	s	Limerick Field Club",
"LIMK	s	Limerick Museum",
"LINC	s	Lincoln University, Plant Sciences Group",
"LINF	s	Shanxi Normal University, Biology Department",
"LINHM	s	Long Island Natural History Museum",
"LINN	s	Linnean Society of London",
"LIP	s	Universite de Lille, Departement de Botanique",
"LIPIMC	c	Lembaga Ilmu Pengetahuan Indonesia, Indonesian Institute for Sciences",
"LIPP	c	Leptospirotheque",
"LIRP	s	Laboratorio de Ictiologia, Faculdade de Filosofia",
"LISC	s	Instituto de Investigacao Cientifica Tropical",
"LISE	s	Estacao Agronomica Nacional",
"LISFA	s	Instituto Nacional de Investigacao Agraria, Departamento Ecologia, Recursos Naturais e Ambiente",
"LISI	s	Instituto Superior de Agronomia",
"LISJC	s	Jardim-Museu Agricola Tropical",
"LISM	s	Missao de Estudos Agronomicos do Ultramar",
"LISU	s	Museu Nacional de Historia Natural",
"LISVA	s	Ministerio da Educacao",
"LIT	s	Okresni vlastivedne muzeum",
"LIV	s	Liverpool Museum",
"LIVC	s	Liverpool Botanic Garden",
"LIVCM	s	Liverpool County Museum",
"LIVNFC	s	Liverpool Naturalists' Field Club",
"LIVU	s	Hartley Botanical Laboratories",
"LJM	s	Prirodoslovni Muzej Slovenije",
"LJU	s	University of Ljubljana, Botany Department",
"LKHD	s	Lakehead University, Biology Department",
"LL	s	University of Texas at Austin, Plant Resources Center",
"LLC	s	Our Lady of the Lake University, Biology Department",
"LLN	s	Lincolnshire Naturalists' Union",
"LLO	s	Lloyd Library and Museum",
"LM	s	Seccao de Botanica e Ecologia",
"LMA	s	National Institute of Agronomic Research, Botany Department",
"LMAD	s	Lobbecke Museum und Aquazoo",
"LMC	s	Instituto de Investigacao Cientifica de Mozambique",
"LMCH	c	Laboratoire de Microbiologie",
"LMD	c	Laboratorium voor Microbiologie der Landbouwhogeschool",
"LMG	c	Belgian Coordinated Collections of Microorganisms/ LMG Bacteria Collection",
"LMJ<AUT>	s	Landesmuseum Joanneum Graz",
"LMJ<MOZ>	s	Centro de Investigacao Cientifica Algodoeira, Botanical Department",
"LMKG	s	Landesmuseum fur Karnten",
"LMND	s	Landessammlungen fuer Naturkunde",
"LMNH	s	Museum d'Histoire naturelle",
"LMRZ	s	Livingstone Museum",
"LMS	c	Carolina Biological Supply Company",
"LMSZ	s	Latvian University, Museum of Systemic Zoology",
"LMU	s	Eduardo Mondlane University, Department of Biological Sciences",
"LMZG	s	Local Museum",
"LNAF	s	Liaoning Academy of Forestry",
"LNCM	s	Liaoning College of Traditional Chinese Medicine",
"LNHS	s	London Natural History Society",
"LNK	s	Landessammlungen fuer Naturkunde",
"LNKD	s	Landessammlung fuer Naturkunde",
"LNMD	s	Landessammlungen fuer Naturkunde",
"LNMO	s	Oldenburg, Landesmuseum Natur und Mensch",
"LNNU	s	Liaoning Normal University, Biology Department",
"LO	s	Type Collection",
"LOB	s	California State University, Biological Sciences Department",
"LOC	s	Occidental College, Biology Department",
"LOCK	c	Centre of Industrial Microorganisms Collection",
"LOD	s	Lodz University, Department of Geobotany and Plant Ecology",
"LOJA	s	Universidad Nacional de Loja, Departamento de Botanica y Ecologia",
"LOMA	s	La Sierra University, Biology Department",
"LON	s	Lembaga Oseanologie Nasional",
"LOU	s	C.I.T.A.-Xunta de Galicia",
"LP<ARG>	s	Museo de La Plata",
"LP<RUS>	s	Laboratory of Palaeontology",
"LPA	s	Jardin Botanico Canario Viera y Clavijo",
"LPAG	s	Universidad Nacional de La Plata, Area de Botanica",
"LPB	s	Herbario Nacional de Bolivia",
"LPD	s	Laboratorio de Botanica de la Direccion de Agricultura",
"LPFU	s	Lehrstuhl fur Palaontologie",
"LPL	s	The University",
"LPN	s	Littlehampton Museum",
"LPS	s	Universidad Nacional de La Plata, Instituto de Botanica Carlos Spegazzini",
"LPSP	s	Lumus Pond State Park, Whale Wallow Nature Center",
"LPUB	s	Laboratorul de Paleontologie",
"LPUP	s	Laboratoire de Parasitologie",
"LPVM	s	Laboratoire de Paleontologie des Vertebres et de Paleontologie",
"LPVPH	s	Laboratoire de Paleontologie des Vertebres et de Paleontologie Humaine",
"LR	s	Museum d'Histoire Naturelle",
"LRL	s	Historic Society of Lancashire and Cheshire",
"LRS	s	Agriculture Research Center, Land Resource Sciences Section",
"LRTE	s	La Retraite (Convent School)",
"LRU	s	University of Arkansas at Little Rock, Biology Department",
"LS<CUB>	s	Colegio de La Salle",
"LS<GBR>	s	Linnean Society of London",
"LSA	s	Lytham St. Annes Public Library",
"LSAM	s	Louisiana State Arthropod Museum",
"LSC	s	Lyndon State College, Natural Sciences Department",
"LSCR	s	Organization for Tropical Studies, La Selva Biological Station",
"LSDC	s	Liangshan Institute for Drug Control",
"LSHB	c	Biochemistry Department",
"LSHI	s	Universite Nationale du Zaire",
"LSN	s	Lord Wandsworth College Natural History Museum",
"LSP	s	Lake Superior Provincial Park",
"LSR	s	Leicestershire Museums Service",
"LSRFC	s	Leicestershire Flora Committee",
"LSSC	s	Lake Superior State College",
"LSTM	c	Department of Parasitology",
"LSU	s	Louisiana State University, Biological Sciences Department",
"LSUHC	s	La Sierra University, Herpetological Collection",
"LSUM	s	Louisiana State University, Biological Sciences Department",
"LSUMZ	s	Louisiana State University, Museum of Natural Science",
"LSUS	s	Museum of Life Sciences, Louisiana State University",
"LT	s	Universite de Montreal",
"LTB	s	La Trobe University, Botany Department",
"LTCC-IOC	c	Leishmania Type Culture Collection",
"LTH	s	Museum of Louth Naturalists' Antiquity and Literary Society",
"LTHP	s	Louth Public Library",
"LTI	c	Cryobank of Microorganisms-Destructors",
"LTM	s	Tekovske muzeum",
"LTN	s	Luton Museum and Art Gallery",
"LTR	s	University of Leicester, Biology Department",
"LTU	s	Louisiana Tech University",
"LU<CHN>	s	Lingnan University",
"LU<RUS>	s	St. Petersburg University",
"LUA	s	Instituto de Investigacao Agronomica",
"LUAI	s	ex-Centro Nacional de Investigacao Cientifica (CNIC)",
"LUB	s	Naturhistorisches Museum zu Luebeck",
"LUBA	s	Instituto Superior de Ciencias da Educacao",
"LUBEE	b	Lubee Bat Conservancy",
"LUCAS	s	Francesc de Lucas i Alcover",
"LUCCA	s	Comune di Lucca",
"LUCH	s	Musee du Pays de Luchon",
"LUD	s	Ludlow Natural History Society",
"LUG	s	Museo cantonale di storia naturale",
"LUGO	s	Universidad de Satniago de Compostela, Departamento de Produccion Vegetal",
"LUH	s	University of Lagos, Biological Sciences Department, Botany Unit",
"LUH<NLD>	c	Leiden University Medical Center",
"LUNZ	s	Lincoln University",
"LUQ	s	Laval University",
"LUS	s	Lushan Botanical Garden",
"LUW	s	Landbouwuniversiteit Wageningen, Department of Entomology",
"LUX	s	Musee national d'histoire naturelle",
"LV	s	Catholic University of Leuven, Laboratory of Plant Systematics",
"LVNP	s	Lassen Volcanic National Park",
"LVP-GSC	s	Laboratory of Vertebrate Paleontology",
"LW	s	Ivan Franko National University of Lviv, Botany Department",
"LWA	s	Agricultural Experiment Station",
"LWD	s	Museum Dzieduszyckich",
"LWG	s	National Botanical Research Institute",
"LWKS	s	Institute of Ecology of the Carpathians",
"LWS	s	Museum of Natural History",
"LWSM	s	Lewes Museum, Ann of Cleves House",
"LWU	s	University of Lucknow, Botany Department",
"LY	c	Laboratoire de Mycologie associe au CNRS",
"LYAC	s	Laiyang Agricultural College, Department of Basic Courses",
"LYCC	c	Lallemand Yeast Culture Collection",
"LYD	s	Mpumalanga Parks Board",
"LYJB	s	Jardin Botanique",
"LYN	s	Lynchburg College, Biology Department",
"LZ	s	Universitaet Leipzig",
"LZAH	s	Lanzhou Institute of Animal Science, Chinese Academy of Agricultural Sciences",
"LZD	s	Lanzhou Institute of Desert Research, Academia Sinica",
"LZFD	s	Laboratoire Zoologique",
"LZLP	s	Universidade de Lisboa",
"LZU	s	Lanzhou University",
"LZUH	s	Laboratoire de Zoologie, Universite de Hanoi",
"M	s	Botanische Staatssammlung Muenchen",
"MA	s	Real Jardin Botanico",
"MAA	s	Escuela Tecnica Superior de Ingenieros Agronomos, Departamento de Produccion Vegetal",
"MAAS	s	Natuurhistorisch Museum Maastricht, Botany Department",
"MAC	s	Instituto do Meio Ambiente",
"MAC-APU	s	Ministerio de Agricultura y Cria",
"MAC-PAY	s	Ministerio de Agricultura y Cria",
"MACA	s	Parque da Reserva de Siac Pai van Coloane Island",
"MACB	s	Universidad Complutense, Departamento de Biologia Vegetal 1",
"MACF	s	California State University, Biological Science Department",
"MACLPI	s	Ministerio de Agricultura y Cria, Seccion de Pesca Interior y Piscicultura",
"MACN	s	Museo Argentino de Ciencias Naturales Bernardino Rivadavia",
"MACN-RN	s	Museo Argentino de Cicencis Naturales, Coleccion Rio Negro",
"MACNCH	s	Museo Argentino de Ciencias Naturales",
"MACO	s	Marlborough College, Biology Department",
"MAD<IND>	s	Madras Museum",
"MAD<USA-WI>	s	Forest Products Laboratory",
"MADJ	s	Jardim Botanico da Madeira",
"MADM	s	Museu Municipal do Funchal",
"MADS	s	Museu de Historia Natural do Seminario do Funchal",
"MAF	s	Universidad Complutense, Departamento de Biologia Vegetal II",
"MAFF<FJI>	s	Colo-i-Suva Silvicultural Station",
"MAFF<JPN>	c	MAFF Genebank, Ministry of Agriculture Forestry and Fisheries",
"MAFI	s	Magyar Allami Foeldtani Intezet, Budapest - Hungarian Geological Survey",
"MAFST	s	Instituto Forestal de la Moncloa",
"MAG	s	Institute of Biological Problems of the North",
"MAGB	s	National Museum and Art Gallery",
"MAGD	s	Northern Territory Museum of Arts and Sciences",
"MAH	s	Department of Agricultural Research",
"MAIA	s	Instituto Nacional de Investigaciones Agrarias, Departamento de Ecologia",
"MAIC	s	Mediterranean Agronomic Institute of Chania, Department of Natural Products",
"MAINE	s	University of Maine, Department of Biological Sciences",
"MAIS	s	Institut d'Elevage et de Medecine Veterinaire des Pays Tropicaux, Departement de Botanique",
"MAK	s	Tokyo Metropolitan University",
"MAKAR	s	Institut Planina i More",
"MAKFUNGI	s	Fungi Macedonici",
"MAL	s	Botanic Gardens of Malawi",
"MALA	s	Malaspina University, Biology Department",
"MALC	s	Museu Municipal de la Vila d'Alcover",
"MALS	s	Manti-LaSal National Forest",
"MAMU	s	University of Sydney, Macleay Museum",
"MAN	s	Universitas Cenderawasih",
"MANCH	s	University of Manchester",
"MAND	s	Agricultural College and Research Institute",
"MANK	s	Minnesota State University-Mankato, Department of Biological Sciences",
"MAO	c	Mircen Afrique Ouest",
"MAPA	s	Museu Anchieta Porto Alegra",
"MAPR	s	University of Puerto Rico, Mayagueez Campus, Biology Department",
"MAR	c	Grasslands Rhizobium Collection",
"MARE	s	Marmara University, Department of Pharmaceutical Botany",
"MARO	s	Marylhurst College",
"MARS	s	Universite de Provence Centre St-Charles, case 4",
"MARSSJ	s	Universite d'Aix-Marseille III, Laboratoire de Botanique et Ecologie Mediterraneenne",
"MARY	s	University of Maryland",
"MASE	s	Maseru Experiment Station",
"MASS	s	University of Massachusetts, Biology Department",
"MATSU	s	Ehime University, Forestry Department",
"MAU	s	Mauritius Sugar Industry Research Institute",
"MAY	s	Adygean State University, Department of Botany",
"MB<DEU-Berlin>	s	Museum of Natural History of Humboldt-University",
"MB<DEU-Marburg>	s	Philipps-Universitaet, Spezielle Botanik",
"MB<PRT>	s	Universidade de Lisboa, Museu Bocage",
"MBA	s	Environmental Protection Agency",
"MBAB	s	Museo Biblioteca Archivio",
"MBAC	s	Museo del Dipartimento di Biologia Animale dell'Universita",
"MBAP	s	Museo del Dipartimento di Biologia Animale dell'Universita",
"MBBJ	s	Museum Zoologicum Bogoriense",
"MBCG	s	Museo di Scienze Naturali \"Enrico Caffi\"",
"MBCSC	s	Marine Biodiversity Collection of South China Sea, Chinese Academy of Sciences",
"Mbg	s	Fachberich Geowissenschaften",
"MBH	s	Marlborough College",
"MBIC	c	Marine Biotechnology Institute Culture Collection",
"MBK	s	Kochi Prefectural Makino Botanical Garden, Botany Department",
"MBL	s	Museu Nacional de Historia Natural",
"MBM<BRA>	s	Museu Botanico Municipal",
"MBM<USA-CA>	s	San Jose State University, Museum of Birds and Mammals",
"MBML	s	Museu de Biologia Mello Leitao",
"MBMU	c	Institute of Molecular Biology and Genetics (IMBG), Mahidol University",
"MBR	s	Museo Argention de Ciencias Naturales \"Bernardino Rivadavia\"",
"MBS	s	Manchester Banksian Society",
"MBSL	s	Royal Medico-Botanical Society",
"MBSN	s	Museo Brembano di Scienze Naturali",
"MBUCV	s	Museo de Biologia de la Universidad Central de Venezuela",
"MBZH	s	Museo y Biblioteca de la Zoologia",
"MC	s	Museo de Cipolleti",
"MCA	s	Muhlenberg College, Biology Department",
"MCAS	s	Museo Civico Archeologico e di Scienze Naturali \"F. Eusebio\"",
"MCBR	s	Museo Civico \"Baldassarre Romano\"",
"MCC-UPLB	c	Microbial Culture Collection",
"MCCB	s	Museo Civico \"Craveri\"",
"MCCI	s	Museo Civico di Storia Natural de Carmognola",
"MCCM<DEU>	c	Medical Culture Collection Marburg",
"MCCM<IND>	s	Madras Christian College",
"MCD	s	Muzeul Civilizatiei Dacice si Romane Deva",
"MCES	s	Museum of the Center for Entomological Studies",
"MCF-PVPH	s	Museo Carmen Funes",
"MCFB	s	Museo de la Cienica Fundacion",
"MCFM	s	Museo Civico \"Francesco Mina Palumbo\"",
"MCFS	s	Museo Civico di Storia Naturale",
"MCG	s	Museo Civico DI Storia Naturale 'Giacomo Doria'",
"McGMK	s	McGregor Memorial Museum",
"MCGS	s	Museo Civico \"Giuseppe Scarabelli\"",
"MCITM	c	Bacterial Culture Collection",
"MCIZ	s	Museo Cambria, Istituto di Zoologia dell'Universita",
"MCJ	s	Missouri Southern State College, Biology Department",
"MCLSBB	s	Museo Colegio La Salle Bonanova de Barcelona",
"MCM(CMFRI)	s	Reference Collection",
"MCM<CAN>	s	Hamilton College, McMaster University, Biology Department",
"MCM<FRA>	s	Institut de Paleontologie, Museum d'Histoire naturelle",
"MCM<IND>	c	MACS Collection of Microorganisms",
"MCM<PRT>	s	Museu Carlos Machado",
"MCMC	s	Museo de Historia Natural de la Ciudad de Mexico",
"McMJ	s	Mc Master University",
"MCMS	s	Museo Civico di Storia Naturale",
"MCN	s	McNeese State University, Biology Department",
"MCNA	s	Museo de Ciencias naturals de Alava",
"MCNC	s	Museo de Ciencias Naturales",
"MCNG	s	Museo de Ciencias Naturales de la UNELLEZ en Guanare",
"MCNPV	s	Fundacao Zoobotanica do Rio Grande do Sul",
"MCNS	s	Universidad Nacional de Salta, Facultad de Ciencias Naturales",
"MCNV	s	Museo Civico di Storia Naturale",
"MCNZ	s	Porto Alegre, Museu de Ciencias Naturais da Fundacao Zoo-Botanica do Rio Grande do Sul",
"MCP<BRA>	s	Pontificia Universidade Catolica do Rio Grande do Sul",
"MCP<USA-MA>	s	Massachusetts College of Pharmacy and Allied Health Sciences, Biological Sciences Department",
"MCPM	s	Milwaukee City Public Museum",
"MCPPV	s	Museu de Ciencias e Tecnologia",
"MCPUCRGS	s	Museu de Ciencias da Pontificia Universidade Catolica do Rio Grande do Sul",
"MCR	s	Manchester Literary and Philosophical Society",
"MCRA	s	Sezione Archeologia, Storia e Scienze Naturali",
"MCRBS	s	Manchester Botanical and Horticultural Society",
"MCSB	s	Museo Civico di Scienze Naturali",
"MCSC	s	Colorado Springs, May Natural History Museum",
"MCSF	s	Museo Civico di Scienze Naturali",
"MCSG	s	Museo Civico di Storia Naturale",
"MCSN<ITA-Genova>	s	Museo Civico di Storia Naturale \"Giacomo Doria\"",
"MCSN<ITA-Verona>	s	Museo Civico di Storia Naturale",
"MCSNC	s	Museo Civico di Storia Naturale",
"MCSNIO	s	Museo Civico di Scienze Naturali di Induno Olona",
"MCST	s	Museo Civico di Storia Naturale",
"MCT	s	Michigan Technological University, Biological Sciences Department",
"MCTC	s	Michigan Technological University, Biological Sciences Department",
"MCTF	s	Michigan Technological University",
"MCTP	s	Museu de Ciencias",
"MCVE	s	Museo Civico di Storia Naturale",
"MCVM	s	Museo Civico, Villa Mirabello",
"MCW	s	Milton College, Biology Department",
"MCZ	s	Museum of Comparative Zoology, Harvard University",
"MCZ:A	s	Museum of Comparative Zoology, Harvard University, Herpetology Amphibian Collection",
"MCZ:HERP	s	Museum of Comparative Zoology, Harvard University, Herpetology Collection",
"MCZ:R	s	Museum of Comparative Zoology, Harvard University, Herpetology Reptile Collection",
"MCZR	s	Museo Civico di Zoologia",
"MD<AGO>	s	Museu Regional do Dundo",
"MD<DEU>	s	Museum Donaueschingen",
"MDE	s	Musee des Dinosaures in Esperaza",
"MDFW	s	Massachusetts Division of Fisheries and Wildlife",
"MDH<GBR>	s	Dorman Museum",
"MDH<USA-MI>	c	Michigan Department of Health",
"MDKY	s	Morehead State University, Biological and Environmental Sciences Department",
"MDLA	s	Museu do Dundo",
"MDM	s	Mifune Dinosaur Museum",
"MDNR	s	Manitoba Conservation",
"MDP	s	Museum de Poligny",
"MDRG	s	Museum voor Dierkunde, Rijksuniversiteit",
"MDTN	s	Middleton Botanical Society",
"MDUG	s	Universidad Guanajuato, Museo Alfredo Duges",
"MDZAU	s	Museum Deptartment of Zoology",
"MECB	s	Universidade Federal de Pelotas, Museu Entomologico Ceslau Biezanko",
"MECG	s	Medical Entomology Collection Gallery",
"MECN	s	Museo Ecuadoriano de Ciencias Naturales",
"MEDEL	s	Universidad Nacional de Colombia - Sede de Medellin, Departamento de Biologia",
"MEFLG	s	Museo Entomologico Franciaco Luis Gallego",
"MEL	s	Royal Botanic Gardens",
"MELG	s	Geology Department, University of Melbourne",
"MELU	s	University of Melbourne",
"MEM	s	University of Memphis, Biology Department",
"MEMO	s	Instituto Tecnologico y de Estudios Superiores de Monterrey, Departamento de Recursos Naturales",
"MEN	s	UNCuyo, Catedra de Botanica Agricola, Departamento de Ciencias Biologicas",
"MEPAN	s	Museum of Evolution, Polish Academy of Sciences",
"MER	s	Universidad de Los Andes",
"MERC	s	Universidad de Los Andes, Centro Jardin Botanico",
"MERF	s	Universidad de Los Andes",
"MERL	s	Instituto Argentino de Investigaciones de las Zonas Aridas (CRICYTME)",
"MESA	s	Mesa State College, Biology Department",
"MEUC	s	Universidad de Chile",
"MEX	s	Museo de Historia Natural de la Ciudad de Mexico",
"MEXU	s	Universidad Nacional Autonoma de Mexico, Departamento de Botanica",
"MFA	s	Museo Provincial de Ciencias Naturales Florentino Ameghino, Seccion Botanica",
"MFA-ZV-M	s	Museo Florentino Ameghino, Coleccion de Mastozoologia (Argentina)",
"MFAP	s	Archaeology and Palaeontology",
"MFB	s	Southern Research Station",
"MFC	c	Matsushima Fungus Collection",
"MFGC	c	Margot Forde Germplasm Centre, AgResearch GrasslandsWar",
"MFLB	s	Marine Fisheries Laboratory",
"MFNB	s	Museo Friulano di Storia Naturale",
"MFP	s	Museo Felipe Poey",
"MFRU	s	Malawi Fisheries Research Unit",
"MFS	s	Museo dei Fisiocritici",
"MFSN	s	Museo Friulano di Storia Naturale of Udine",
"MFU	s	Museo Friulano di Storia Naturale",
"MFUW	s	Chinzombo Research Station, Chinzombo Wildlife Research Station",
"MFW	s	Museum Freriks",
"MG<BRA>	s	Museu Paraense Emilio Goeldi, Departamento de Botanica",
"MG<CHN>	s	Museum of Zoology",
"MGA	s	Instituto Pedagogico de Varones",
"MGAB	s	Muzeul de Istorie Naturala \"Grigore Antipa\"",
"MGAP	s	Museu Anchieta",
"MGB	s	Museo de Geologia (del Seminario Diocesano) de Barcelona",
"MGC	s	Universidad de Malaga, Departamento de Biologia Vegetal",
"MGDL	s	Museum d'Histoire Naturalle du Grand-Duchy de Luxembourg",
"MGF	s	Museum George Frey",
"MGFT	s	Museum G. Frey",
"MGH	s	Museum Godeffroy",
"MGHF	s	Museo Geologico H. Fuenzalida",
"MGHNL	s	Musee Guimet d'Histoire Naturelle de Lyon",
"MGHSJ	s	Matuyama Girl's High School",
"MGI	s	Geological Institute of the Mongolian Academy of Sciences",
"MGL	s	Musee Geologique de Lausanne",
"MGR	s	University of Michigan",
"MGRI	s	Moscow Geological Prospecting Institute",
"MGS	s	Upper Silesian Museum, Department of Natural History",
"MGSI	s	Museum of the Geological Survey of Iran",
"MGSP	s	Museum of the Geological Survey of Portugal",
"MGUG	s	Museum fuer Geologie und Palaontologie der Georg-August-Universitat",
"MGUH	s	Museum Geologicum Universitatis Hafniensis",
"MGUP	s	Museo Geologico della Universita Pisa",
"MGUV	s	Museo del Departamento de Geologia, Universidad de Valencia",
"MGUWR	s	Institute of Geological Sciences, University of Wroclaw",
"MH<CHE>	s	Naturhistorisches Museum",
"MH<IND>	s	Tamil Nadu Agricultural University",
"MHA	s	Main Botanical Garden",
"MHH	c	Institute of Virology",
"MHL	s	Mildenhall and District Museum",
"MHM	s	Malham Tarn Field Centre",
"MHMN	s	Museu Historic Municipal de Novelda",
"MHNA	s	Museum d'Histoire Naturelle d'Autun",
"MHNB	s	Museum d'Histoire Naturelle de Bale",
"MHNC<CHE>	s	Musee d'Histoire Naturelle - La Chaux-de-Fonds",
"MHNC<CHL>	s	Museo de Historia Natural de Concepcion (Chile)",
"MHNCI	s	Museu de Historia Natural Capao de Imbuia (Brazil)",
"MHNCSJ	s	Museo de Historia Natural",
"MHND	s	Museo Nacional de Historia Natural",
"MHNES	s	Museo de Historia Natural de El Salvador",
"MHNG	s	Natural History Museum of Geneva",
"MHNG:Herp	s	Natural History Museum of Geneva, Herpetology collection",
"MHNI	s	Museu Hist. Naturales Universidade Federal Minas Gerais",
"MHNJP	s	Universidad Nacional Mayor de San Marcos",
"MHNL	s	Musee Guimet d'Histoire Naturelle de Lyon",
"MHNLR	s	Museum d'Histoire Naturelle",
"MHNLS	s	Coleccion de Mastozoologia, Museo de Historia Natural de La Salle",
"MHNM	s	Museo de Historia Natural de Montevideo",
"MHNN<CHE>	s	Neuchatel Musee d'Histoire Naturel",
"MHNN<FRA>	s	Musee d'Histoire Naturalle",
"MHNNICE	s	Mueusm d'Histoire Naturelle de Nice",
"MHNP	s	Museum d'Histoire Naturelle Perpignan",
"MHNSM	s	Museo de Historia Natural, Universidad Nacional Mayor de San Marcos",
"MHNT	s	Museum d'Histoire Naturelle Toulouse",
"MHNUNC	s	Departamento de Ictiologia del Museo de Historia Natural de la Universidad Nacional de Colombia",
"MHNV	s	Museo de Historia Natural de Valparaiso",
"MHP	s	Fort Hays State University, Sternberg Museum of Natural History",
"MHU	s	Makerere University, Botany Department",
"MHV	s	Musee de Haute Volta",
"MHWK	s	Much Wenlock Museum",
"MI	s	Universita degli Studi di Milano, Dipartimento di Biologia",
"MIB	s	University of Milano - Bicocca, Department of Biotechnology and Biosciences",
"MIB:ZPL	s	University of Milano - Bicocca, Department of Biotechnology and Biosciences, ZooPlantLab",
"MICH	s	University of Michigan",
"MICKKU	c	MICKKU Culture Collection",
"MID	s	Middlebury College, Biology Department",
"MII	s	Museum of Irish Industry",
"MIKU	s	Marine Biological Institute, Kyoto University",
"MIL	s	Milwaukee Public Museum",
"MIM	s	Minusinsk N. M. Martjanov Regional Museum",
"MIMB	s	Museum of the Institute of Marine Biology",
"MIMM	s	Mauritius Institute",
"MIN	s	University of Minnesota",
"MINC	s	Universidad Politecnica",
"MINI	s	Muzeul de Istoria Naturala",
"MIP	s	Museo de La Plata",
"MIPV	s	Universita degli Studi di Milano, Laboratorio di Micologia e Batteriologia Fitopathologica",
"MISR	s	Macaulay Land Use Research Institute",
"MISS	s	University of Mississippi, Department of Biology",
"MISSA	s	Mississippi State University, Department of Biological Sciences",
"MIT	c	Massachusetts Institute of Technology",
"MIWG	s	Museum of he Isle of Wight Geology",
"MIZA	s	Museuo del Instituto de Zoologia Agricola",
"MIZL	s	Musee de l'Institut de Zoologie",
"MIZT	s	Universita di Torino",
"MJ	s	Muzeum Vysociny",
"MJCM	s	Museo de Ciencias Naturales y Antropologicas \"Prof. Juan C.Moyano\" (Argentina)",
"MJG<ARG>	s	Museo Jorge Gerhold",
"MJG<AUT>	s	Landesmuseum Joanneum",
"MJG<DEU>	s	Johannes Gutenberg-Universitaet",
"MJH	s	Muzeul Judetean Hunedoara",
"MJMO	s	Universidad Centro Occidental, Decanato de Agronomia",
"MJS	s	Xiaolongshan Forestry Experiment Bureau",
"MJSD	s	Museum-Jardin des Sciences",
"MK	s	National Museum of Kenya",
"ML	s	Musee de Lectoure",
"MLAV	s	Musees de Laval",
"MLLD	c	Microbiological Research Laboratory, Soil and Water Section, Department of Land Development",
"MLMJI	c	Department of Plant Protection, Faculty of Agricultural Production",
"MLP	s	Museo La Plata, Collection Herpetologia",
"MLRU	c	Microbiology Laboratory, Department of Biology, Faculty of Science",
"MLS<AUS>	s	Marine Laboratory Sydney",
"MLS<COL>	s	Museo del Instituto de La Salle",
"MLS<GBR>	s	Lathallan Preparatory School",
"MLUH	s	Martin Luther Universitaet",
"MLY	s	Arboretum Mlynany",
"MLZ	s	Occidental College, Moore Laboratory of Zoology",
"MM<CAN>	s	Manitoba Museum",
"MM<COL>	s	Museo del Mar",
"MM<DEU>	s	Magdeburg Museum",
"MM<FRA>	s	University of Montpellier",
"MMB	s	Moravske Muzeum",
"MMBC	s	Moravske Muzeum [Moravian Museum]",
"MMBS	s	Mukaishima Marine Biological Station",
"MMChPV	s	Museo Municipal El Chocon",
"MMCM	s	Museum of Malawi",
"MMF	s	Museu Municipal do Funchal",
"MMG	s	Museo Marino de la Isla de Gorgona",
"MMH	s	Municipal Museum",
"MMI	s	Regionalni muzeum",
"MMK	s	McGregor Museum",
"MMKZ	s	Alexander McGregor Memorial Museum",
"MML	c	Medical Microbiological Laboratory",
"MMMN	s	Manitoba Museum of Man and Nature, Botany Department",
"MMMZ	s	Mutare Museum",
"MMNH<MNG>	s	Mongolian Museum of Natural History",
"MMNH<USA-MN>	s	Bell Museum of Natural History",
"MMNHS	s	Macedonian Museum of Natural History",
"MMNS	s	Mississippi Museum of Natural Science",
"MMP	s	Museo de Mar del Plata (Argentina)",
"MMS	s	Montshire Museum of Science",
"MMTT	s	Iran National Museum of Natural History",
"MMUE	s	Museum of Manchester University",
"MMUS	s	Macleay Museum, University of Sydney",
"MN	s	Museu Nacional, Universidade Federal do Rio de Janeiro",
"MNA	s	Museum of Northern Arizona",
"MNAV	s	Museo Naturalistico-Archeologico",
"MNB	s	Museum fuer Naturkunde der Humboldt-Universitaet",
"MNCE	s	Museu de Historia Natural Capao da Embuia",
"MNCN	s	Museo Nacional de Ciencias Naturales",
"MNCR	s	Museo Nacional de Costa Rica",
"MND	s	Museum Natura Docet",
"MNDG	s	Museo Nacional \"David J. Guzman\"",
"MNE	s	Maidstone Museum and Art Gallery",
"MNFD	s	Museum fuer Naturkunde",
"MNG	s	Sammlung Eisfeld des Museums der Natur Gotha",
"MNGA	s	Muzeul National de Istorie Natural \"Grigore Antipa\"",
"MNGC	s	Museo Nacional de Historia Natural",
"MNH	s	Musei Nacionalis Hungarici",
"MNHC	s	Museo Nacional de Historia Natural",
"MNHCI	s	Museu de Historia Natural Capao da Imbuia",
"MNHD	s	Museo Nacional de Historia Natural",
"MNHM<DEU>	s	Naturhistorisches Museum Mainz/Landessammlung fuer Naturkunde Rheinland-Pfalz",
"MNHM<USA-CO>	s	John May Museum of Natural History",
"MNHN	s	Museum National d'Histoire Naturelle",
"MNHN<CUB>	s	Museo Nacional de Historia Natural, Departamento de Colecciones",
"MNHNCH	s	Museo Nacional de Historia Natural de Chile",
"MNHNCU	s	Museo Nacional de Historia Natural",
"MNHNJP	s	Universidad Nacional Mayor de San Marcos",
"MNHNLES	s	Museum National d'Histoire Naturelle Lesotho",
"MNHNM	s	Museo Nacional de Historia Natural",
"MNHNP	s	Museo Nacional de Historia Natural del Paraguay",
"MNHNSD	s	Museo Nacional de Historia Natural",
"MNHNUL	s	Museu Nacional de Historia Natural de Universidade de Lisboa",
"MNHP	s	Princeton University",
"MNHS	s	Manchester Natural History Society",
"MNK	s	Museo de Historia Natural \"Noel Kempff Mercado\"",
"MNKS	s	Milton Keynes Development Corporation",
"MNMB	s	Magyar Nenzeti Museum",
"MNMS	s	Museo Nacional de Ciencias Naturales",
"MNN	s	Musee National du Niger",
"MNNC	s	Museo Nacional de Historia Natural",
"MNNHN	s	Museum National d'Historie Naturelle",
"MNNW	s	Museum fuer Naturkunde",
"MNR	s	Ministry of Natural Resources",
"MNRJ	s	Museu Nacional/Universidade Federal de Rio de Janeiro",
"MNSB	s	Museum of Natural Sciences",
"MNSL	s	Museum of Natural Sciences",
"MNUFR	s	Mongolian National University",
"MNVL	s	Museum d'Histoire Naturelle de Ville de Lille",
"MNZ	s	Museum of New Zealand Te Papa Tongarewa",
"MO	s	Missouri Botanical Garden",
"MOAR	s	Morris Arboretum, University of Pennsylvania, Botany Department",
"MOC	s	Western Oregon University, Biology Department",
"MOD	s	Universita degli Studi di Modena e Reggio Emilia, Dipartimento de Biologia Animale",
"MODNR	s	Division of State Parks, Department of Natural Resources",
"MOFUNG	s	Museu Oceanogr. Fundacao Univerdidade Rio Grande",
"MOG	s	National Range Agency",
"MOL	s	Universidad Nacional Agraria La Molina, Departamento Academico de Biologia",
"MOLA	c	Microbial Observatory of the Laboratoire Arago",
"MOM	s	Musee Oceanographique Monaco",
"MONA	s	Musee Oceanographique de Monaco",
"MONT	s	Montana State University",
"MONTU	s	University of Montana",
"MONZ	s	Museum of New Zealand",
"MOR<USA-IL>	s	Morton Arboretum, Research Department",
"MOR<USA-MT>	s	Museum of the Rockies",
"MOS	s	College of Agriculture and Forestry",
"MOSG	s	Muzeul Orasului Sf. Gheorghe",
"MOSI	s	Museum of Science and Industry",
"MOSM	s	All-Russian Research Institute of Medicinal and Aromatic Plants",
"MOSN	s	Museo Ornitologico e di Scienze Naturali",
"MOSP	s	Moscow State Pedagogical University, Botany Department",
"MOT	s	Mote Marine Laboratory",
"MOTH	s	Museum of the Hemispheres",
"MOVC	s	Cornell College, Biology Department",
"MOVI	s	Museu Oceanografico do Vale do Itajai",
"MP<CZE>	s	Vychodoceske muzeum Pardubice",
"MP<USA-NY>	s	Mohonk Preserve, Inc.",
"MP<ZAF>	s	Transvaal Museum",
"MPA	s	Ecole National Superieure Agronomique, Biologie et Pathologie Vegetales",
"MPC	s	Monterey Peninsula College, Life Science Museum",
"MPCA	s	Museo Provincial \"Carlos Ameghino\"",
"MPCRM	s	Museo Paleontologico Cittadino della Rocca",
"MPE	s	F. R. Long Herbarium",
"MPEF-PV	s	Muso Paleontologico Egidio Fergulio",
"MPEG	s	Museu Paraense Emilio Goeldi",
"MPGB	s	Museum of Portuguese Guinea",
"MPKV	c	Biological Nitrogen Fixation Project College of Agriculture",
"MPL	s	Musee de Port Louis",
"MPLN	s	Museo Provinciale di Storia Naturale",
"MPM	s	Milwaukee Public Museum",
"MPMP	s	National Museum of the Philippines",
"MPN	s	Massey University, Ecology Group",
"MPPD	s	University of Minnesota, Plant Pathology Department",
"MPPE	s	Paletnologica ed Etnologico dei Padri Francescani",
"MPR	s	Mount Makulu Pasture Research Station",
"MPSC	s	Museu de Paleontologia de Santana do Cariri",
"MPSN	s	Museo Provinciale di Scienze Naturali",
"MPSP	s	Museu Paulista",
"MPSU	c	Department of Microbiology",
"MPT	s	Museuo Provincial de Teurel",
"MPU	s	Universite Montpellier II",
"MPUC	s	Pontificia Universidade Catolica do Rio Grande do Sul, Laboratorio de Botanica",
"MPUM	s	Museo Paleontologia Universita degli Studi di Milano",
"MPUN	s	Museo Paleontologicom",
"MPUNR	s	Departamento de Geologia, Universidad de Chile",
"MPV	s	Museo Paleontologico Municipal de Valencia",
"MPZ	s	Museo Paleontologico de la Universidad de Zaragoza",
"MQ	s	Gansu Institute of Desert Control",
"MQU	s	Macquarie University",
"MRA	s	Museo Requieu",
"MRAC	s	Musee Royal de l'Afrique Centrale",
"MRC<TUR>	c	TUBITAK Marmara Research Center Culture Collection",
"MRC<USA-MT>	s	Rocky Mountain Research Station",
"MRC<ZAF>	c	National Research Institute for Nutritional Diseases",
"MRCA	s	Musee Royal de l'Afrique Centrale",
"MRCN	s	Museu Rio-Grandense de Ciencias Naturais",
"MRD	s	Moorhead State University, Biology Department",
"MRF	s	Museum of Histoire naturelle",
"MRGS	s	Museu do Rio Grande do Sul",
"MRI	s	Murray Royal Institution",
"MRNP	s	Mount Rainier National Park",
"MRSC	s	Mount Makulu Central Research Station",
"MRSH	s	Matopos Research Station",
"MRSN	s	Museo Regionale di Scienze Naturali",
"MRSP	s	Museo Regionale di Scienze Naturali",
"MRST	s	Museo Regionale di Storia Naturale",
"MS	s	Universita di Messina, Dipartimento di Scienze Botaniche",
"MSA	s	Museum of Science and Art",
"MSB	s	Museum of Southwestern Biology",
"MSB:Bird	s	Museum of Southwestern Biology, bird collection",
"MSB:Fishes	s	Museum of Southwestern Biology, fish collection",
"MSB:Mamm	s	Museum of Southwestern Biology, mammal collection",
"MSB:Para	s	Museum of Southwestern Biology, Parasitology Collection",
"MSB<BGR>	s	Museum Sophia",
"MSB<DEU>	s	Ludwig-Maximilians-Universitaet",
"MSC	s	Michigan State University, Botany and Plant Pathology Department",
"MSCL	c	Microbial Strain Collection of Latvia",
"MSCMU	c	Microbiology Section, Chiang Mai University (MSCMU)",
"MSCW	s	Mississippi University for Women",
"MSDB	s	Museo di Storia Naturale \"Don Bosco\"",
"MSDS	c	Microbiology Section, Biological Science Division, Department of Science Services",
"MSE	s	Angus Museums",
"MSEM	s	Museu Geologic del Seminari de Barcelona",
"MSEN	s	Montrose Natural History and Antiquarian Society",
"MSF	s	Sauriermuseum Frick",
"MSGP	s	Nusee des Services Geologiques du Portugal",
"MSIE	s	Museum of Shanghai",
"MSINR	s	Museum Sichuan Institute of Natural Resources",
"MSIR	s	Mauritius Sugar Industry",
"MSJC	s	St. Joseph's College, Natural History Museum",
"MSK	s	National Academy of Sciences of Belarus, Flora and Systematic Laboratory",
"MSKH	s	Central Botanical Garden",
"MSKU	s	Belarusian State University, Botany Department",
"MSL	s	Royal Medical Society of London",
"MSLH	s	Chinese University of Hong Kong, Marine Sciences Laboratory",
"MSLY	s	Mossley Botanical Society",
"MSM<JPN>	s	Marine Science Museum, Tokai Univ.",
"MSM<PRI>	s	University of Puerto Rico, Marino Puertorriqueno",
"MSNA	s	Museo di Storia Naturale e Arte Archeologica",
"MSNC	s	Museo Civico di Storia Naturale",
"MSNG	s	Museo Civico di Storia Naturale di Genova \"Giacomo Doria\"",
"MSNM	s	Museo Civico di Storia Naturale di Milano",
"MSNO	s	Museum des Sciences Naturelles",
"MSNP	s	Museo di Scienze Naturali",
"MSNT<ITA-Torino>	s	Museo Regionale di Scienze Naturali",
"MSNT<ITA-Turin>	s	Museo Civico DI Storia Naturale DI Torino",
"MSNU	s	Museo di Storia Naturale dell'Universita",
"MSNV	s	Museo Civico di Storia Naturale di Venezia",
"MSNVR	s	Museo Civico di Storia Naturale di Verona",
"MSPC	s	Museo di Storia Naturale \"Pietro Calderini\"",
"MSPP	c	Mycology Section, Plant Pathology and Microbiology Division, Department of Agricultural Science",
"MSSC	s	Midwestern State University",
"MSTFM	s	Middle School of the Third Factory Machinery",
"MSTR	s	Westfaelisches Museum fuer Naturkunde",
"MSU<THA>	c	Acetobacter",
"MSU<USA-MI>	s	Michigan State University Museum",
"MSUB	s	Montana State University",
"MSUD	s	I. I. Mecynikov State University of Odessa, Department of Morphology and Systematics of Plants",
"MSUH	s	University of Mosul, Biology Department",
"MSUMC	s	Murray State University",
"MSUMZ	s	Memphis State University",
"MSUN	s	Westfaelische Wilhelms-Universitaet",
"MSUT	s	Museum of Natural History",
"MSUZ	s	Mississippi State University, Zoological Collections",
"MSV	s	Museum der Stadt Villach",
"MT<CAN>	s	Universite de Montreal",
"MT<RUS>	s	Mus. Tinro, Vladyvostok",
"MTA	s	Maden Tetkik ve Arama Enstituesue",
"MTCC	c	Microbial Type Culture Collection & Gene Bank",
"MTD	s	Museum fuer Tierkunde Dresden",
"MTEC	s	Montana State University",
"MTJB	s	Jardin botanique de Montreal",
"MTKD	s	Staatliches Museum fuer Tierkunde",
"MTKKU	c	Department of Clinical Microbiology, Faculty of Medical Technology",
"MTMG	s	McGill University, Macdonald Campus, Plant Science Department",
"MTN	s	Malton Field Naturalists' Society",
"MTQA	s	Museum of Tropical Queensland",
"MTSN	s	Museo Tridentino di Scienze Naturali",
"MTSU	s	Middle Tennessee State University, Biology Department",
"MTUF	s	University Museum, Tokyo University of Fisheries",
"MU<TUR>	c	Mugla University Collection of Microorganisms",
"MU<USA-OH>	s	Miami University, Botany Department, Willard Sherman Turrell Herbarium",
"MU<USA-TX>	s	Midwestern University",
"MUACC	c	Murdoch University Algal Culture Collection",
"MUAF	c	Culture collection of Mendel University of Agriculture and Forestry in Brno",
"MUB	s	Universidad de Murcia, Departamento de Biologia Vegetal, Botanica",
"MUCC	c	Murdoch University Culture Collection",
"MUCL	c	Mycotheque de l'Universite Catholique de Louvain",
"MUCPv	s	Museo de la Universidad Nacional del Comahue",
"MUCR	s	Museo de Insectos",
"MUCV	s	Monash University, Biological Sciences Department",
"MUDH	s	The Hague, Museon",
"MUFE	s	University of Marmara, Department of Botany",
"MUFM	s	Manipur University Fish Museum",
"MUFS	s	Department of Animal Science",
"MUGM	s	Museo de Ciencias Naturales, Coleccion \"Gustavo Orces\" (Ecuador)",
"MUHW	s	Marshall University, Biological Sciences Department",
"MUJ	s	Museo Javeriano de Historia Natural, Laboratoriao de Entomologia",
"MUL	c	Department of Microbiology MUL-B 250",
"MULU	s	Museum Ludovicae Ulricae, Zoology Institute of the University of Uppsala",
"MUM	c	Micoteca da Universidade do Minho",
"MUMF	s	Department of Life Sciences",
"MUMZ	s	University of Missouri, Museum of Zoology",
"MUNC	s	St. John's, Memorial University of Newfoundland",
"MUNZ	s	Massey University",
"MUO	s	Stovall Museum of Science and History",
"MUP	s	Universidade do Porto, Museu do Historia Natural",
"MUR	s	Murray State University, Department of Biological Sciences",
"MURD	s	Murdoch University",
"MURU	s	Murdoch University",
"MUS	s	Muskingum College, Biology Department",
"MUSA	s	Universidad Nacional de San Agustin, Museo de Historia Natural (Peru)",
"MUSK	s	Muskegon Community College, Life Science Department",
"MUSM	s	Museo de Historia Natural",
"MUSN	s	Museo Universitario di Storia Naturale e della Strumentazione Scientifica",
"MV	s	University of Montana Museum",
"MVC	s	University of Charleston, Natural Sciences Department",
"MVDA	s	Ministerio de Ganaderia y Agricultura",
"MVEN	s	Naturhistorisch Museum",
"MVFA	s	Universidad de la Republica, Laboratorio de Botanica",
"MVFQ	s	Universidad de la Republica, Catedra de Botanica Farmaceutica",
"MVHC	s	Universidad de la Republica, Seccion Micologia",
"MVJB	s	Museo y Jardin Botanico",
"MVM	s	Museo Nacional de Historia Natural, Departamento de Botanica",
"MVMA	s	Museum of Victoria",
"MVN	s	Public Library",
"MVNP	s	Mesa Verde National Park",
"MVP	s	Museum of Vicotria",
"MVSC	s	Millersville University, Biology Department",
"MVUP	s	Museo de Vertebrados, Universidad de Panama",
"MVZ	s	Museum of Vertebrate Zoology, University of California at Berkeley",
"MVZ:Bird	s	Museum of Vertebrate Zoology, University of California at Berkeley, Bird Collection",
"MVZ:Egg	s	Museum of Vertebrate Zoology, University of California at Berkeley, Egg Collection",
"MVZ:Herp	s	Museum of Vertebrate Zoology, University of California at Berkeley, Herpetology Collection",
"MVZ:Hild	s	Museum of Vertebrate Zoology, University of California at Berkeley, Milton Hildebrand collection",
"MVZ:Img	b	Museum of Vertebrate Zoology, University of California at Berkeley, Image Collection",
"MVZ:Mamm	s	Museum of Vertebrate Zoology, University of California at Berkeley, Mammal Collection",
"MVZ:Page	b	Museum of Vertebrate Zoology, University of California at Berkeley, Notebook Page Collection",
"MW<NLD>	s	Museum Wasmann",
"MW<RUS>	s	Moscow State University",
"MWC	s	Museum of Western Colorado",
"MWCF	s	Mary Washington College, Department of Biology",
"MWFB	s	University of California, Davis, Museum of Wildlife and Fisheries Biology",
"MWG	s	Moscow State University, Department of Biogeography",
"MWI	s	Western Illinois University, Biological Sciences Department",
"MWNH	s	Museum Wiesbaden, Department of Natural Science",
"MWSJ	s	Missouri Western State College, Biology Department",
"MWSU	s	Midwestern State University",
"MY	s	Universidad Central de Venezuela, Botanica",
"MYDC	s	Mianyang Institute for Drug Control",
"MYF	s	Universidad Central de Venezuela",
"MZ-ICACH	s	Instituto de Ciencias y Artes de Chiapas, Museo Zoologico (Mexico)",
"MZ<CZE>	s	Jihomoravske muzeum Znojmo",
"MZ<POL>	s	Museum of the Earth, Polish Academy of Sciences",
"MZAF	s	Museo Zoologico dell'Accademia dei Fisiocritici",
"MZB	s	Museum Zoologicum Bogoriense",
"MZBL	s	Museo de Zoologica",
"MZBS	s	Museo Zoologia",
"MZCP	s	Universidade de Coimbra",
"MZCR	s	Museo de Zoologia",
"MZFC	s	Museo de Zoologia \"Alfonso L. Herrera\"",
"MZFN	s	Museo Zoologico dell'Universita \"Federico II\"",
"MZGZ	s	Museum Zoologia del Giardino Zoologico",
"MZH	s	Zoolgical Museum, Finnish Museum of Natural History",
"MZKI	c	Microbial Culture Collection of National Institute of Chemistry",
"MZL	s	Musee Zoologique",
"MZLS	s	Musee Zoologique",
"MZLU	s	Lund University",
"MZN	s	Musee Zoologie",
"MZNA	s	Universidad de Navarra, Museum of Zoology",
"MZNC	s	Universidad Nacional de Cordoba, Museo de Zoolog&iacutea",
"MZP	s	Muzeum Ziemi Polska Akademia Nauk",
"MZPW	s	Polish Academy of Science, Museum of the Institute of Zoology",
"MZRF	s	Museo Zangheri di Storia Naturale della Romagna",
"MZRO	s	Museo Civico di Storia Naturale \"P. Zangheri\"",
"MZS	s	Universite de Strasbourg, Musee de Zoologie",
"MZSF	s	Universite de Strasbourg, Museum Zoologique",
"MZSP	s	Sao Paulo, Museu de Zoologia da Universidade de Sao Paulo",
"MZTG	s	Museum Zoologia",
"MZUABCS	s	Museo de Zoologia de la Universidad Automica de Baja California Sur",
"MZUB	s	Museo di Zoologia",
"MZUC<CHL>	s	Museo de Zoologia, Universidad de Concepcion",
"MZUC<ITA>	s	Universita di Cagliari",
"MZUCR	s	Universidad de Costa Rica, Museo de Zoologia",
"MZUF	s	Museo Zoologico La Specola, Universita di Firenze",
"MZUL	s	Museo di Zoologia dell'Universita \"La Sapienza\"",
"MZUN	s	Museo Zoologico di Universita degli Studi",
"MZUP<ITA-Padova>	s	Museo Zoologia",
"MZUP<ITA-Parma>	s	Museo Zoologico di Universita degli Studi",
"MZUS	s	Musee de Zoologie de l'Universite de Strasbourg",
"MZUSP	s	Museu de Zoologia da Universidade de Sao Paulo",
"MZUT<ITA-Torino>	s	Museo di Zoologia, Instituto di Zoologia e Anatomia Comparata Universita di Torino",
"MZUT<ITA-Turin>	s	Museo e Instituto DI Zoologia Sistematica dell' UniversitaDI Torino",
"MZUTH	s	Museum of Zoology, University of Thessaloniki",
"MZUVN	s	Musee Zool. Univ. et Ville de Nancy",
"MZV<POL>	s	Muzeum i Instytut Zoologii",
"MZV<RUS>	s	Zoological Museum Varsovite",
"MZYU	s	Museum of Zoology, Yunnan University",
"N	s	Nanjing University, Biology Department",
"NA	s	United States National Arboretum, USDA/ARS",
"NABG	s	Botanical Garden",
"NAC	s	Nagano Nature Conservation Research Institute",
"NAI	s	University of Nairobi, Botany Department",
"NAIC	s	National Agricultural Insect Collection",
"NAM	s	Facultes Universitaires Notre-Dame de la Paix",
"NAN	s	Nantong Teachers College, Biology Department",
"NAP<CHN>	s	Institute of Zoology, Academia Sinica (formerly National Academy of Peiping)",
"NAP<ITA>	s	Universita Degli Studi di Napoli Federico II, Dipartimento di Biologia Vegetale",
"NARA	s	National Aquatic Resources Agency",
"NARI	s	National Agricultural Research Institute",
"NARL	s	National Agricultural Research Laboratories",
"NAS	s	Institute of Botany, Jiangsu Province and Chinese Academy of Sciences",
"NASC<UK>	b	European Arabidopsis Stock Centre",
"NASC<USA-MA>	s	Massachusetts College of Liberal Arts, Biology Department",
"NAT	s	Seale-Hayne Agricultural College",
"NATC	s	Northwestern State University, Biological Sciences Department",
"NAU	s	Nanjing Agricultural University, Department of Plant Science",
"NAUF	s	Northern Arizona University",
"NAUJ	s	Nanjing, Nanjing Agricultural University",
"NAUVM	s	Northern Arizona University, Museum of Vertebrates",
"NAVA	s	Navajo Natural Heritage Program, Navajo Department of Fish & Wildlife",
"NBAIM	c	National Bureau of Agriculturally Important Microorganisms",
"NBFGR	s	National Bureau of Fish Genetic Resources (Indian Council of Agricultural Research)",
"NBG	s	National Botanical Institute",
"NBIMCC	c	National Bank for Industrial Microorganisms and Cell Cultures",
"NBM	s	New Brunswick Museum",
"NBMB	s	St. John's, New Brunswick Museum",
"NBME	s	National Butterfly Museum (Saruman Museum)",
"NBNM	s	National Park Service",
"NBPC	s	National Birds of Prey Centre",
"NBRC	c	NITE Biological Resource Center",
"NBSB	s	National Biomonitoring Specimen Bank, U.S. Geological Survey",
"NBSB:Bird		National Biomonitoring Specimen Bank, U.S. Geological Survey, bird collection",
"NBSB:Mamm		National Biomonitoring Specimen Bank, U.S. Geological Survey, mammal tissue collection",
"NBSI	s	Biologische Station Neusiedler See",
"NBV	s	Koninklijke Nederlandse Botanische Vereniging",
"NBY	s	Newbury District Museum",
"NBYC	s	Newberry College, Department of Biology",
"NCAIM	c	National Collection of Agricultural and Industrial Microorganisms",
"NCAM	c	National Collection of Agricultural Microorganisms",
"NCAS	s	Rutgers University, Biological Sciences Department",
"NCATG	s	North Carolina A & T State University, Biology Department",
"NCAW	s	North-West College of Agriculture",
"NCB	c	National Culture Bank",
"NCBS	s	Yale University, Connecticut Botanical Society",
"NCC<FRA>	c	Nantes Culture Collection",
"NCC<USA-CA>	s	Sonoma State University, Biology Department",
"NCCB<GBR>	s	Nature Conservancy Council, Banbury",
"NCCB<NLD>	c	The Netherlands Culture Collection of Bacteria (formerly LMD and Phabagen Collection)",
"NCCBH	s	Nature Conservancy Council, Balloch",
"NCCE	s	Nature Conservancy Council, Edinburgh",
"NCCH	s	Nature Conservancy Council",
"NCCN	s	Nature Conservancy Council, Norwich",
"NCCP	c	National Culture Collection for Pathogens",
"NCDC	c	National Collection of Dairy Cultures",
"NCE	s	University of Newcastle upon Tyne, School of Biological Sciences",
"NCFB	c	National Collection of Food Bacteria",
"NCH	s	Norwich Botanical Society",
"NCHU	s	National Chung Hsing University",
"NCIM	c	National Collection of Industrial Microorganisms",
"NCIMB	c	National Collections of Industrial Food and Marine Bacteria (incorporating the NCFB)",
"NCIP<IDN>	s	Pusat Penelitian dan Pengembangan Oseanologi",
"NCIP<ZAF>	s	National Collection of Insects",
"NCKU	s	National Cheng-Kung University, Biology Department",
"NCLN	s	New College",
"NCMA	s	Raleigh, North Carolina Department of Environmental Health and Natural Resources",
"NCMH	c	The North Carolina Memorial Hostital",
"NCMK	s	Norwich Castle Museum",
"NCPF	c	National Collection of Pathogenic Fungi",
"NCPPB	c	National Collection of Plant Pathogenic Bacteria",
"NCPV	c	National Collection of Pathogenic Viruses",
"NCS	s	North Carolina State University",
"NCSC<THA>	c	National Center of Streptococcus Collection, Department of Microbiology, Faculty of Medical Science",
"NCSC<USA-NC>	s	North Carolina State University, Botany Department",
"NCSM	s	North Carolina State Museum of Natural Sciences",
"NCSU	s	North Carolina State University Insect Collection",
"NCTC	c	National Collection of Type Cultures",
"NCU	s	University of North Carolina, North Carolina Botanical Garden",
"NCUT	s	Nicolaus Copernicus University",
"NCWRF	c	National Collection of Wood Rotting Fungi",
"NCY	s	Conservatoire et Jardins Botaniques de Nancy",
"NCYC	c	National Collection of Yeast Cultures",
"ND	s	University of Notre Dame, Department of Biological Sciences",
"NDA	s	North Dakota State University, Animal and Range Sciences Department",
"NDAA	s	Orange, New South Wales Department of Agriculture",
"NDAT	s	Department of Agriculture",
"NDFC	s	Newfoundland Department of Forestry",
"NDG	s	University of Notre Dame, Department of Biological Sciences",
"NDO	s	Division of Forest Research, Forest Department",
"NDSR	s	National Drosophila Species Resource Center",
"NDSU	s	North Dakota State University",
"NDTC	s	Ningde Teachers College, Biology Department",
"NE	s	University of New England",
"NEB	s	University of Nebraska State Museum",
"NEBC	s	Harvard University",
"NEFI	s	Northeastern Forestry University, Forestry Department",
"NEMO	s	Truman State University",
"NEMSU	s	Truman State University",
"NEMU	s	Newark Museum",
"NENU	s	Northeast Normal University, Biology Department",
"NEPCC	c	North East Pacific Culture Collection",
"NESH	s	University of Nevada",
"NEU	s	Universite de Neuchatel, Laboratoire de botanique evolutive",
"NEW	s	University of Newcastle",
"NEWHM	s	Hancock Museum",
"NEZ	s	Museum, Zoology Department, University of New England",
"NF	s	Nanjing Forestry University, Forest Resources and Environment",
"NFCCP	c	National Fungal Culture Collection of Pakistan",
"NFLD	s	Memorial University of Newfoundland, Biology Department",
"NFM	s	Newfoundland Museum",
"NFO	s	Niagara Parks Botanical Gardens and School of Horticulture",
"NFRC	s	Northern Forest Research Centre",
"NFRDI	c	National Fisheries Research & Development Institute",
"NFRN	s	Canadian Forest Service, NRCan",
"NGI	s	Nanjing Geographical Institute",
"NGM	s	Bromley House Library",
"NGMC	s	National Geological Museum of China",
"NGR	c	Plant Pathology",
"NH	s	South African National Biodiversity Institute",
"NHA	s	University of New Hampshire, Plant Biology Department",
"NHES	s	Connecticut Agricultural Experiment Station, Entomology Department",
"NHG	s	Naturhistorische Gesellschaft e. V., Abteilung Botanik",
"NHIC	s	Ontario Ministry of Natural Resources",
"NHL	c	National Institute of Hygienic Sciences",
"NHM	s	Natural History Museum",
"NHM<CHE>	s	Naturhistorisches Museum",
"NHM<GBR-Nottingham>	s	University of Nottingham, Botany Department",
"NHMA	s	Natural History Museum",
"NHMB<CHE>	s	Naturhistorisches Museum",
"NHMB<HUN>	s	Natural History Museum Bucharest",
"NHMBe	s	Naturhistorisches Museum Bern",
"NHMC<GRC>	s	Natural History Museum of Crete, University of Crete, Department of Botany",
"NHMC<MMR>	s	Natural History Museum",
"NHME	s	Natuurhistorisch Museum",
"NHMG	s	Naturhistoriska Museet",
"NHMK	s	Landesmuseum fuer Karnten",
"NHML	s	Natural History Museum",
"NHMM	s	Natuurhistorische Museum Maastricht",
"NHMM-LS	s	Naturhistorisches Museum/Landessammlung fuer Naturkunde Rheinland-Pfalz",
"NHMN	s	Nottingham Natural History Museum (Wollaton Hall)",
"NHMR<ISL>	s	Natural History Museum",
"NHMR<NLD>	s	Natuurhistorisch Museum",
"NHMUK	s	Natural History Museum",
"NHMW	s	Naturhistorisches Museum",
"NHNC	s	La Chaux-de-Fons",
"NHNE	s	New England College, Biology Department",
"NHRI	s	Islandic Museum of Natural History",
"NHRM	s	Naturhistoriska Rijkmuseet",
"NHRS	s	Swedish Museum of Natural History, Entomology Collections",
"NHSD	s	Natural History Society of Dublin",
"NHST	s	Museum of Natural History",
"NHT	s	Tropical Pesticides Research Institute",
"NHV	s	Institut fuer Landwirtschaftliche Botanik",
"NI<JPN>	c	Nagao Institute",
"NI<SVK>	s	Slovenska pol'nohospodarska Univerzita, Katedra botaniky",
"NIAES	c	National Institute of Agro-Environmental Sciences",
"NIAH	c	National Institute of Animal Health",
"NIBH	c	National Institute of Bioscience and Human-Technology",
"NIBR	s	National Institute of Biological Resources",
"NICC	s	National Insect Collection",
"NICD	s	Malaria Research Center",
"NICE	s	Museum d'Histoire Naturelle",
"NICH	s	Hattori Botanical Laboratory",
"NIES	c	Microbial Culture Collection",
"NIFI	s	National Inland Fisheries Institute",
"NIG	s	Nanjing Geographical Institute",
"NIGL	s	Nanjing Institute of Geography and Limnology",
"NIGP	s	Naking Institute of Geology and Palaeontology",
"NINF	s	Newfoundland Insectarium",
"NIO	s	National Institute of Oceanography",
"NIOCC	c	National Institute of Oceanography Culture Collection",
"NIPR	s	National Institute of Polar Research, Biological Data Department",
"NIT	s	Jardim Botanico de Niteroi",
"NIVA	c	Culture Collection of Algae (NIVA)",
"NIWA	s	National Institute of Water and Atmospheric Research",
"NJ	s	Njala University College",
"NJM<CZE>	s	Okresni vlastivedne muzeum",
"NJM<JPN>	c	Nippon Veterinary and Animal Science University",
"NJNU	s	Nanjing Normal University, Biology Department",
"NJSM	s	New Jersey State Museum",
"NKA	c	Nationales Konsiliarlabor fur Adenoviren",
"NKMC	s	National Kweiyang Medical College",
"NKME	s	Naturkundemuseum Erfurt",
"NKMU	s	Nankai University Museum",
"NKU	s	Nankai University, Biology Department",
"NKUM	s	Nankai University",
"NLEC	s	Neal L. Evenhuis",
"NLH	s	Agricultural University of Norway, Department of Biology and Nature Conservation",
"NLHD	s	Niedersachsisches Landesmuseum",
"NLPS	s	Nottingham Literary and Philosophical Society",
"NLSN	s	Notre Dame University, Biological Sciences Department",
"NLU	s	University of Louisiana at Monroe, Museum of Natural History",
"NLUH	s	University of the Philippines College Baguio",
"NM	s	Northern Michigan University, Biology Department",
"NMAC	s	Inner Mongolia Agricultural University, Department of Pratacultural Science",
"NMAG	s	Naturhistorisches Museum",
"NMB<CHE>	s	Naturhistorishes Museum",
"NMB<ZAF>	s	National Museum, Botany Department",
"NMBA<ARG>	s	National Museum",
"NMBA<AUT>	s	Naturhistorisches Museum der Benediktiner-Abtei",
"NMBA<CHE>	s	Naturhistorisches Museum Basel",
"NMBE	s	Naturhistorisches Museum der Burgergemeinde Bern",
"NMBO	s	National Museum",
"NMBZ	s	Natural History Museum of Zimbabwe",
"NMC	s	New Mexico State University, Department of Biology",
"NMC<CAN>	s	Canadian Museum of Nature",
"NMCL	s	Naturkunde-Museum",
"NMCR	s	New Mexico State University, Department of Animal and Range Sciences",
"NME	s	Sammlung des Naturkundemseum Erfurt",
"NMED	s	New Mexico Environment Department",
"NMEG	s	Naturkundesmuseum",
"NMFC	s	Inner Mongolia Forestry College, Desert Control and Utilization Department",
"NMFSH	s	National Marine Fisheries Service",
"NMG	s	Naturhistoriska Riksmuseet",
"NMI	s	National Museum of Ireland",
"NMID	s	National Museum of Ireland",
"NMK	s	National Museum",
"NMKE	s	National Museum of Kenya",
"NMKL	s	National Museum",
"NML-HCCC	c	National Microbiology Laboratory Health Canada Culture Collections",
"NMLS	s	Natur-Museum Luzern",
"NMLU	s	Natur-Museum Luzern, Botany Department",
"NMM	s	National Museum of Victoria",
"NMMA	s	Nantucket Maria Mitchell Association, Natural Sciences Department",
"NMMBP	s	Natl. Mus. Mar. Biol./Aquarium",
"NMMH	s	North Manchurian Museum",
"NMML	s	National Marine Mammal Laboratory",
"NMMNH	s	New Mexico Museum of Natural History and Science",
"NMN	s	Northamptonshire Natural History Society",
"NMND	s	National Museum of Natural History",
"NMNH<IND>	s	National Museum of Natural History",
"NMNHI	s	National Museum of Natural History",
"NMNK	s	National Museum of Nepal",
"NMNS	s	National Museum of Natural Science",
"NMNW	s	National Museum of Namibia",
"NMNZ	s	National Museum of New Zealand",
"NMP	s	Natal Museum",
"NMP<CZE>	s	Narodni Museum Praha",
"NMPC	s	National Museum Prague",
"NMPG<CHN>	s	Zhejiang Institute of Traditional Chinese Medicine",
"NMPG<DEU>	s	Museum der Natur-Gotha",
"NMPI	s	Division of Plant Industry",
"NMQR	s	National Museum",
"NMS	s	National Museum",
"NMS-G	s	National Museums of Scotland - Geology & Zoology",
"NMSA	s	Natal Museum",
"NMSL	s	National Museum",
"NMSR	s	Naturhistorisches Museum im Thueringer Landes museum Heidecksburg zu Rudolstadt",
"NMSU<USA-MO>	s	Northwest Missouri State University, Biology Department",
"NMSU<USA-NM>	s	New Mexico State University",
"NMSZ	s	National Museums of Scotland",
"NMTC	s	Inner Mongolia Normal University, Biology Department",
"NMTT	s	National Museum and Art Gallery",
"NMV<AUS>	s	Museum Victoria",
"NMV<JPN>	s	Nakagawa Museum at Nakagawa-cho",
"NMVM	s	National Museum of Victoria",
"NMW<AUT>	s	Naturhistorisches Museum",
"NMW<GBR>	s	National Museums & Galleries of Wales, Department of Biodiversity and Systematic Biology",
"NMWC	s	National Museum of Wales",
"NMWZ	s	National Museum of Wales",
"NMZB	s	National Museum of Zimbabwe",
"NMZL	s	National Museum of Zambia",
"NNA	s	Nanning Arboretum",
"NNHMK	s	National Natural History Museum of the Ukraine",
"NNKN	s	Noordbrabants Natuurmuseum",
"NNM	s	Nationaal Natuurhistroisch Museum",
"NNMN	s	Nationaal Natuurhistorisch Museum Naturalis",
"NNSU	s	N. I. Lobachevsky Nizhni Novgorod State University, Department of Botany",
"NO	s	Tulane University, Department of Ecology and Evolutionary Biology",
"NOAS	s	New Orleans Academy of Science",
"NODCAR	c	marwa mokhtar Abd Rabo",
"NoF	c	The Fungus Culture Collection of the Northern Forestry Centre",
"NOI	s	Nanhai Oceanographic Institute",
"NOLS	s	University of New Orleans, Biological Sciences Department",
"NOSU	s	Northeastern State University, Natural Sciences and Mathematics Department",
"NOT	s	Nottingham City Natural History Museum",
"NOTM	s	University of Nottingham, Manuscript Department",
"NOU	s	Institut de Recherche pour le Developpement, Botany and Applied Ecology Department",
"NPA	s	Nanjing Institute of Geology and Paleontology, Academia Sinica",
"NPB	s	Natal Parks, Game, and Fish Preservation Board, Research Section",
"NPC	s	National Pusa Collection",
"NPIB	s	Northwest Plateau Institute of Biology",
"NPP	c	N.P.P",
"NPRI	s	Seoul National University",
"NPSC	s	Northern Prairie Science Center",
"NPT	s	Newport Museum and Art Gallery",
"NPWRC	s	Northern Prairie Research Center",
"NRC	c	Division of Biological Sciences, National Research Council of Canada",
"NRCC	s	National Research Council of Canada",
"NRIBAS	s	National Research Institute of Biology, Academia Sinica",
"NRIC	c	NODAI Research Institute Culture Collection",
"NRL	c	Neisseria Reference Laboratory",
"NRM	s	Swedish Museum of Natural History",
"NRN	s	Nairn Literary Society Library, Public Library",
"NRNZ	s	Northland Regional Museum",
"NRPSU	c	Department of Agro-industry, Faculty of Natural Resources",
"NRRL	c	Agricultural Research Service Culture Collection",
"NRS	s	Naturhistoriska Riksmuseet",
"NRWC	s	N. R. Whitney Collection",
"NRZM	c	German Reference Center for Meningococci",
"NS	s	Central Siberian Botanical Garden",
"NSAC	s	Nova Scotia Agricultural College, Department of Environmental Sciences",
"NSCA	s	North Scotland College of Agriculture",
"NSCNFB	c	Novi Sad Collection of Nitrogen Fixing Bacteria",
"NSDA	s	Nevada Division of Agriculture",
"NSK	s	Siberian Central Botanical Garden, Laboratory for Plant Systematics and Floristic Genesis",
"NSM<CAN>	s	Nova Scotia Museum of Natural History",
"NSM<CHN>	s	Sun Yat-Sen Tomb and Memorial Park Commission",
"NSM<JPN>	s	NSM-PV, National Science Museum",
"NSMC<CAN>	s	Nova Scotia Museum",
"NSMC<USA-NV>	s	Nevada State Museum",
"NSMHS	s	Nevada State Museum and Historical Society",
"NSMK	s	National Science Museum",
"NSMT	s	National Science Museum (Natural History)",
"NSMW	s	Naturwissenschftlich Sammlung, Museum Wiesbaden",
"NSNR	s	Nova Scotia Department of Natural Resources",
"NSPM	s	Nova Scotia Museum of Natural History",
"NSRF	s	Nova Scotia Research Foundation",
"NSS	s	University of Liverpool Botanic Gardens",
"NSU	s	Northeastern State University, Biological Collections",
"NSUL	s	Northwestern State University of Louisiana",
"NSW	s	Royal Botanic Gardens",
"NSWA	s	New South Wales Department of Agriculture",
"NSWF	s	State Forests of New South Wales",
"NSWGS	s	Geological Survey of New South Wales",
"NSYU	s	National Sun Yat-sen University",
"NT	s	Department of Natural Resources, Environment and the Arts",
"NTCCI	c	Culture Collection, Microbiology and Cell Biology Laboratory",
"NTDPIF	s	Northern Territory Department of Primary Industry and Fisheries",
"NTLN	s	County Record Office, County Archives Department",
"NTM<AUS>	s	Museum and Art Galleries of the Northern Territory",
"NTM<FRA>	s	Museum d'Histoire Naturelle de Nantes",
"NTN	s	Central Museum and Art Gallery",
"NTNUB	s	National Taiwan Normal University",
"NTOU	c	Institute of Marine Biology, National Taiwan Ocean University",
"NTS	s	Nevada Operations Office, U.S. Department of Energy",
"NTSC	s	University of North Texas, Biological Sciences Department",
"NTUC	s	National Taiwan University",
"NTUF	s	National Taiwan University, Forestry Department",
"NTUM	s	National Taiwan University",
"NTUMA	s	National Taiwan University",
"NU<THA>	c	Department of Microbiology, Faculty of Science",
"NU<ZAF>	s	University of Natal, School of Botany and Zoology",
"NUA	c	Department of Microbiology, National University of Athens",
"NUM	s	Nagoya University",
"NUSDM	c	Department of Microbiology",
"NUSMBS	s	Niigata University, Sado Marine Biological Station",
"NUV	s	Norwich University, Biology and Life Sciences Department",
"NUVC	s	Northeastern University, Vertebrate Collection",
"NVDA	s	Nevada State Department of Agriculture",
"NVMC	s	Nevada State Museum",
"NVRL	s	Naturforschende Verein in Riga",
"NVRW	s	Naturhistorisches Verein der Preussische Rheinland und Westfalens",
"NWAU	s	North-West Agricultural University",
"NWC	s	University of Northern Iowa, Nixon Wilson Collection",
"NWFC	s	Northwest University of Agriculture Forestry Science & Technology",
"NWH	s	Norfolk Museums and Archaeology Service, Natural History Department",
"NWK	s	Newark District Council Museum",
"NWMSU	s	Northwest Missouri State University",
"NWOSU	s	Northwestern Oklahoma State University, Biology Department",
"NWSW	s	Naturwissenschaftliche Sammlungen der Stadt Winterthur",
"NWT	s	Harper Adams Agricultural College",
"NWTC	s	Northwest Normal University",
"NWU	s	Northwestern University, Botany Department",
"NWUB	s	Northwest Normal University, Biology Department",
"NX	s	Universidade do Estado de Mato Grosso - Campus de Nova Xavantina, Departamento de Ciencias Biologicas",
"NXAC	s	Ningxia Agricultural College",
"NXF	s	Ningxia Academy of Agriculture and Forestry Sciences",
"NY	s	New York Botanical Garden",
"NYA	s	Nanyue Arboretum",
"NYAS	s	Silvicultural Research Station",
"NYBG	s	New York Botanical Garden",
"NYS	s	New York State Museum",
"NYSM	s	New York State Museum",
"NYZS	s	New York Zoological Society",
"NZAC	s	New Zealand Arthropod Collection",
"NZCS	s	University, National Zoological Collection of Suriname",
"NZFRI	s	New Zealand Forest Research Institute Limited",
"NZFS	c	Forest Research Culture Collection",
"NZOI	s	New Zealand Oceanographic Institute",
"NZRD	c	New Zealand Reference Culture Collection of Microorganisms, Dairy Section",
"NZRM	c	New Zealand Reference Culture Collection, Medical Section",
"NZRP	c	New Zealand Reference Culture Collection and Soil Section",
"NZSI	s	Zoological Survey of India, National Zoological Collection",
"O	s	Botanical Museum",
"OAC	s	Botany Department, University of Guelph",
"OAKL	s	Oakland Museum of California, Natural Sciences Department",
"OAMB	s	Open Air Museum of Ethnography and Natural Sciences",
"OAX	s	Instituto Politecnico Nacional (CIIDIR-Oax., I.P.N.)",
"OAXM	s	Centro Interdisciplinario de Estudios, Coleccion Mastozoologica (Mexico)",
"OBI	s	California Polytechnic State University, Biological Sciences Department",
"OBPF	s	Planting Fields Arboretum State Historic Park",
"OC	s	Oberlin College, Biology Department",
"OCHA	s	Ochanomizu University",
"OCLA	s	University of Science and Arts of Oklahoma",
"OCM	c	Oregon Collection of Methanogens",
"OCNF	s	Ochoco National Forest",
"OCSA	s	Veterinary Research Institute",
"ODAC	s	Oregon Department of Agriculture",
"ODU	s	Old Dominion University, Department of Biological Sciences",
"OFC	s	Orielton Field Centre",
"OGDF	s	Forest Service Region 4, USDA",
"OGU	s	Odesskij Gosudarstvennij Universitet",
"OH	s	Agricultural Museum of Praha",
"OHBR	s	Ontario Hydro",
"OHM	s	Oldham Microscopical and Natural History Society",
"OHN	s	Regionherbariet i Oskarshamn",
"OHSC	s	Ohio Historical Society",
"OKA	s	Oksky State Biosphere Reserve",
"OKAY	s	Okayama University of Science, Department of Biosphere-Geosphere System Science",
"OKL	s	University of Oklahoma, Botany and Microbiology Department/ Oklahoma Biological Survey",
"OKLA	s	Oklahoma State University, Botany Department",
"OL	s	Palacky University, Botany Department",
"OLAN	s	Ministerio de Recursos Naturales",
"OLD	s	Universitaet Oldenburg, Fachbereich 7",
"OLDM	s	Libraries, Art Galleries and Museums",
"OLDS	s	Olds College, Horticulture Department",
"OLE	s	Oundle School, Biology Department",
"OLM	s	Vlastivedne muzeum v Olomouci",
"OLML	s	Oberoesterreichisches Landesmuseum",
"OLP	s	Univerzity Palackeho, Katedra biologie",
"OLTC	s	Teachers Training College, Botany Department",
"OLV	s	Olivet College, Biology Department",
"OM	s	Otago Museum",
"OMA	s	University of Nebraska Omaha, Biology Department",
"OMC<NZL>	s	Catalogues in Otago Museum",
"OMC<USA>	s	Mills College, Biology Department",
"OMJ	s	Okresni muzeum a galerie",
"OMKH	s	Oblastni muzeum Kutna Hora",
"OMNH<JPN>	s	Osaka Museum of Natural History",
"OMNH<USA-OK>	s	Oklahoma Museum of Natural History",
"OMNHN	s	The Sam Noble Oklahoma State Museum of Natural History",
"OMNHO	s	Osaka Museum of Natural History",
"OMNO	s	Oklahoma Museum of Natural History",
"OMNZ	s	Otago Museum",
"OMP	s	Polabske muzeum v Podebradech",
"OMPB	s	Osservatorio per le Malattie delle Piante per la Regione Emilia-Romagna",
"OMPG	s	Osservatorio per le Malattie delle Piante per le Province di Genova e La Spezia",
"OMPS	s	Osservatorio per le Malattie delle Piante per la Sardegna",
"OMSKM	s	Omsk State Agrarian University, Department of Forestry and Plant Conservation",
"OMUB	s	Ondokuz Mayis University, Biology Department",
"ON	s	Oman Natural History Museum",
"ONNC	s	Office de la Recherche Scientifique et Technique d'Outre-Mer",
"ONP	s	Olympic National Park",
"ONPC	s	Olympic National Park",
"OOM	s	Kameyama Botanical Garden",
"OP	s	Silesian Museum",
"OPANM	s	Otdel Paleontologii i Biostratigrafii Akademii Nauk Moldovkoi Republiki",
"OPM	s	Okinawa Prefectual Museum",
"ORE	s	University of Oregon, Biology Department",
"OREB	s	Karolinska Hoegre Allmaenna Laeroverket",
"ORI	s	Ocean Research Institute",
"ORIS	s	Institute of Steppe of the Ural branch of Russian Academy of Sciences",
"ORIT	s	University of Tokyo",
"ORM	s	Musee des Sciences Naturelles, Departement de Botanique",
"ORSC	s	Office de la Recherche Scientifique et Technique d'Outre-Mer",
"ORST	s	Office de la Recherche Scientifique et Technique d'Outre-Mer",
"ORSTOM	s	Office de la Recherche scientifique et Technique Outre-mer",
"ORT	s	Instituto Canario de Investigaciones Agrarias (ICIA)",
"ORTN	s	Orton Hall",
"ORU	s	Oral Roberts University, Biology Department",
"OS<USA-OH>	s	Ohio State University",
"OS<USA-OR>	s	Oregon State University",
"OSA	s	Osaka Museum of Natural History",
"OSAC	s	Oregon State Arthropod Collection",
"OSAL	s	Ohio State University Acarology Laboratory",
"OSB	s	Society of Botanists",
"OSBU	s	Universitaet Osnabrueck, Spezielle Botanik",
"OSC	s	Oregon State University, Botany and Plant Pathology Department",
"OSEC	s	K.C Emerson Museum",
"OSH	s	University of Wisconsin Oshkosh, Biology and Microbiology Department",
"OSI	s	Ordnance Survey of Ireland",
"OSM<CZE>	s	Ostravske muzeum",
"OSM<USA-OH>	s	Ohio State University Museum",
"OSMC	s	St. Martin's College, Biology Department",
"OSN	s	Museum am Schoelerberg, Natur und Umwelt",
"OSU<OSU-OK>	s	Oklahoma State University, Collection of Vertebrates",
"OSU<USA-OH>	s	Ohio State University",
"OSUC	s	Oregon State University",
"OSUF	s	Oregon State University, Department of Wood Science and Engineering",
"OSUFW	s	Oregon State University, Department of Fisheries and Wildlife Mammal Collection",
"OSUMZ	s	Ohio State University, Museum of Biological Diversity",
"OSUO	s	Oregon State University, School of Oceanography",
"OSUS	s	Oklahoma State University",
"OSWY	s	Oswestry Museum",
"OTA	s	University of Otago, Botany Department",
"OTF	s	Canadian Forest Service",
"OTM<NZL>	s	Otago Museum",
"OTM<USA-MT>	s	Old Trail Museum",
"OTSC	s	Organization for Tropical Studies",
"OTT	s	University of Ottawa, Biology Department",
"OU	s	Fossil Catalgoue in the Geology Museum",
"OUA	s	Universite de Ouagadougou",
"OULU	s	University of Oulu, Biology Department",
"OUM	s	Oxford University Museum",
"OUPR	s	Universidade Federal de Ouro Preto, Campus Universitario",
"OUSM	s	Oklahoma University Stovall Museum",
"OUT	c	Department of Biotechnology",
"OUVC	s	Ohio University Vertebrate Collection",
"OVMB	s	Okresni vlastivedne muzeum",
"OWU	s	Ohio Wesleyan University, Botany-Microbiology Department",
"OXD	s	Wadham College",
"OXF	s	University of Oxford, Department of Plant Sciences",
"OXM	s	Magdalen College Library",
"P<FRA>	s	Museum National d'Histoire Naturelle, Departement de Systematique et Evolution",
"P<SWE>	s	Paleontological Collections, Royal Swedish Natural History Museum",
"PAC	s	Pennsylvania State University, Biology Department",
"PACA	s	Instituto Anchietano de Pesquisas/UNISINOS",
"PACMA	s	Pennsylvania State University, Biology Department",
"PAD	s	Universita degli Studi di Padova, Centro Interdipartimentale Musei Scientifici",
"PADA	s	Pennsylvania Department of Agriculture",
"PAE	s	Universitaet Basel",
"PAL	s	Universita degli Studi di Palermo, Dipartimento de Scienze Botaniche",
"PALEON	s	Wyoming Dinosaur International Society",
"PAM	s	Pennsylvania Department of Agriculture",
"PAMG	s	Empresa de Pesquisa Agropecuaria de Minas Gerais (EPAMIG), Departamento de Pesquisa",
"PAMP	s	Universidad de Navarra, Departamento de Botanica",
"PAN	s	Panjab University, Botany Department",
"PAP	s	Musee de Tahiti et des Iles",
"PAR	s	Museo de Ciencias Naturales y Antropologicas Prof. Antonio Serrano, Departamento de Botanica",
"PARMA	s	Universita degli Studi di Parma",
"PAS	s	Java Sugar Experimental Station",
"PASA	s	Pasadena City College, Life Sciences Department",
"PASG	s	Paleontological Section of the Georgian Academy of Sciences",
"PASM	s	Palomar College, Life Sciences Department",
"PASSM	s	Peabody Academy of Science",
"PAT	s	Museum National d'Histoire Naturelle",
"PAUH	s	University of Texas-Pan American, Biology Department",
"PAUMC	s	Pan American University, Mammal Collection",
"PAUP	s	Punjab Agricultural University",
"PAV	s	Universita di Pavia, Dipartimento de Ecologia del Territorio",
"PAY	s	Paisley Philosophical Institute",
"PBF	c	Perum Bio Farma",
"PBH	s	Peterborough City Museum",
"PBL	s	Botanical Survey of India, Andaman & Nicobar Circle",
"PBM	s	Mahidol University, Department of Pharmaceutical Botany",
"PBP	s	Patagonia Botanical Park",
"PBS	s	Chambers Institute, Tweeddale Museum",
"PBZT	s	Parc Botanique et Zoologique de Tsimbazaza",
"PC	s	Museum National d'Histoire Naturelle",
"PCC	c	Pasteur Culture Collection of Cyanobacteria",
"PCH	s	Prestwich and Pilkington Botanical Society",
"PCM<IND>	s	Presidency College, Botany Department",
"PCM<POL>	c	Polish Collection of Microorganisms",
"PCMB	b	The Pacific Center for Molecular Biodiversity",
"PCNZ	s	Lincoln Plant Health Station",
"PCU<FRA>	s	Museum National d'Histoire Naturelle",
"PCU<THA>	c	Department of Microbiology, Faculty of Pharmaceutical Sciences",
"PD	c	Culture Collection of Plant Pathogenic Bacteria",
"PDA	s	Royal Botanic Gardens, Department of Agriculture",
"PDD	s	Landcare Research",
"PDTFAU	s	Paleoantropoloji Dil ve Tarih Cografya Facueltesi",
"PE	s	Institute of Botany, Chinese Academy of Sciences",
"PECA	s	Pratt Education Center and Aquarium",
"PECS	s	Janus Pannonius Museum, Natural History Department",
"PEFO	s	Petrified Forest",
"PEI	s	Agriculture and Agri-Food Canada",
"PEL	s	Universidade Federal de Pelotas, Departamento de Botanica",
"PEM<CHN>	s	Beijing Medical University, Botany Department",
"PEM<ZAF>	s	Port Elizabeth Museum",
"PEN	s	Penrith Museum",
"PENN	s	University of Pennsylvania",
"PER	s	City Museum",
"PERM	s	University of Perm, Botany Department",
"PERTH	s	Western Australian Herbarium",
"PERU	s	Universita di Perugia, Dipartimento di Biologia Vegetale",
"PES	s	University of Peshawar, Natural Drug Division",
"PESA	s	Centro Ricerche Floristiche Marche",
"PET	s	National University of Peking Teachers' College",
"PEU	s	University of Port Elizabeth, Botany Department",
"PEUFR	s	Universidade Federal Rural de Pernambuco, Departamento de Biologia",
"PEY	s	Peking University, Biology Department",
"PFC	s	Pfeiffer University, Biology Department",
"PFCA	s	Pacific Forestry Centre Arthropod Reference Collection",
"PFES	s	Petawawa National Forestry Institute, Canadian Forest Service",
"PFI	s	Percy FitzPatrick Institute of African Ornithology",
"PFRA	s	Tree Nursery",
"PFRS	s	Pacific Southwest Forest and Range Experiment Station",
"PFSS	s	Petrified Forest National Park",
"PGC	c	Peterhof Genetic Collection of Microalgae",
"PGFA	s	Pyatigorsk State Pharmaceutical Academy, Botany Department",
"PGL	s	Preussiche Geologische Landesanstalt",
"PGM	s	Pacific Grove Museum of Natural History",
"PGMNH	s	Pacific Grove Museum of Natural History",
"PGR	c	Plant Gene Resources of Canada national and international base and active collections",
"PH	s	Academy of Natural Sciences, Botany Department",
"PHA	s	Pharmaceutical Society of Great Britain",
"PHBL	c	Philip Harris Biological Ltd.",
"PHG	s	Peper Harow",
"PHIL	s	University of the Sciences in Philadelphia, Biological Sciences Department",
"PI<DEU>	s	Institut und Museum fuer Geologie und Palaeontologie",
"PI<ITA>	s	Universita di Pisa, Dipartimento di Scienze Botaniche",
"PI<RUS>	s	Paleontological Institute",
"PIHG	s	Florida Department of Agriculture and Consumer Services",
"PIHU	s	Paleontological Institut of Helsingfors",
"PIKN	s	Koronivia Research Station",
"PIMUI	s	Palaeontologische Institut und Museum der Universitaet in Innsbruck",
"PIMUZ	s	Palaontologisches Institut und Museum der Universitat Zurich",
"PIN<GBR>	s	Philosophical Institution of Newport",
"PIN<RUS>	s	Paleontological Institute, Russian Academy of Sciences",
"PINN	s	Pinnacles National Monument",
"PIR	c	Bulgarian Research Culture Collection",
"PIUU	s	Paleontological Institut, University of Uppsala",
"PKDC	s	Divisao de Museu de Historia Natural",
"PKM	s	V. G. Belinsk Pedagogical Institute of Penza",
"PL	s	Zapadoceske muzeum",
"PLFV	s	Principality of Liechtenstein",
"PLH	s	Plymouth City Museum and Art Gallery",
"PLY	s	Plymouth Institution and Athenaeum",
"PLYMOUTH	c	Plymouth Culture Collection",
"PLYP	s	University of Plymouth, Department of Biological Sciences",
"PM	s	Peabody Essex Museum, Natural History Department",
"PM<USA-AK>	s	Pratt Museum",
"PM<USA-IA>	s	Putnam Museum of History and Natural Science",
"PMA<CAN>	s	Provincial Museum of Alberta",
"PMA<PAN>	s	Universidad de Panama",
"PMAA	s	Palaeontological Collections, Provincial Museum and Achieves of Alberta",
"PMAE	s	Royal Alberta Museum",
"PMAG	s	Perth Museum and Art Gallery",
"PMAM	s	Beijing Natural History Museum",
"PMAU	s	Peterborough Museum and Art Gallery",
"PMB	s	Prirodnjacki Muzej Srpske Zemije",
"PMBC	s	Phuket Marine Biological Centre",
"PMFP	s	Papeete Museum",
"PMG<BRA>	s	Horto Florestal",
"PMG<GBR>	s	Paisley Museum of Geological Collection",
"PMH	s	City Museum and Records Office",
"PMHPS	s	Portsmouth Philosophical Society",
"PMHU	s	Palaontologisches Museum",
"PMIG	s	Phyletisches Museum",
"PMJ	s	Phyletisches Museum",
"PMK<RUS>	s	Pugachev Regional Museum",
"PMK<SVK>	s	Podunajske muzeum",
"PMNH<PAK>	s	Pakistan Museum of Natural History",
"PMNH<USA-CT>	s	Peabody Museum of Natural History",
"PMNH<USA-MA>	s	Pratt Museum of Natural History",
"PMQ	s	Public Museum",
"PMR	s	Prirodoslovni muzej Rijeka",
"PMS<MKD>	s	Prirodonamen Muzej Skopje",
"PMS<USA-CA>	s	Pacific Marine Station",
"PMS<USA-MA>	s	Peabody Essex Museum",
"PMSD	s	Pettigrew Museum",
"PMSL	s	Slovenian Museum of Natural History (Prirodosloveni Muzej Slovenije)",
"PMSP	s	Prefeitura do Municipio de Sao Paulo, Departamento de Parques e Areas Verdes",
"PMU<RUS>	s	Paleontological Museum of Undory",
"PMU<SWE>	s	Paleontological Museum of Uppsala",
"PMV	s	Provincial Museum",
"PNBG	s	University of the Philippines",
"PNCM-BIOTECH	c	Philippine National Collection of Microorganisms",
"PNG	s	Division of Primary Industry",
"PNGM	s	National Museum and Art Gallery",
"PNH	s	National Museum",
"PNICMM-INP	s	Instituto Nacional de la Pesca (Mexico)",
"PNL	s	Polytechnic of North London, Food and Biological Sciences Department",
"PNM	s	Philippine National Museum",
"PNPC	s	Chickasaw National Recreational Area [formerly Platt National Park]",
"PNZ	s	Penlee House Museum",
"PO<PRT>	s	Universidade do Porto, Departamento de Botanica",
"PO<RUS>	s	Collection of the Zoological Institute of the Russian Academy of Sciences",
"POFS	s	Forest Service, USDA",
"POKM<RUS-Penza>	s	Penza Regional Local History Museum",
"POKM<RUS-Perm>	s	Perm Regional Lore Museum",
"POL-F	s	Pollichia, Pfalzmuseum fur Naturkunde Thallichtenberg",
"POLL	s	Pfalzmuseum fuer Naturkunde",
"POM	s	Pomona College",
"POP	s	Tatranske muzeum v Poprade",
"POR	s	Universita degli Studi di Napoli",
"PORE	s	Point Reyes National Seashore",
"PORT	s	BioCentro-UNELLEZ",
"PORUN	s	Universita degli Studi di Napoli, Dip Ar Bo Pa Ve - Sezione Botanica",
"POZ	s	Adam Mickiewicz University, Department of Plant Taxonomy",
"POZG	s	Adam Mickiewicz University, Department of Geobotany",
"POZM	s	Adam Mickiewicz University, Department of Plant Ecology and Environment Protection",
"POZNB	s	Agricultural Academy, Botany Department",
"POZW	s	Adam Mickiewicz University",
"PPCC<AUS>	c	Plant Pathology Culture Collection",
"PPCC<NZL>	s	Plant Protection Centre Collection",
"PPCD	s	West Virginia Department of Agriculture",
"PPDD	s	Ministry of Agriculture",
"PPFI	s	Pakistan Forest Institute",
"PPHM	s	Panhandle-Plains Historical Museum",
"PPI	s	National Pingtung University of Science and Technology, Department of Forestry",
"PPIHAS	c	Mycology Collection",
"PPIU	s	M. Utemisov Western Kazakhstanian State University, V. V. Ivanov Department of Botany",
"PPKMI	c	Plant Production Technology Department, Faculty of Agricultural Technology",
"PPKU1	c	Department of Plant Pathology, Faculty of Agriculture",
"PPKU2	c	Department of Plant Pathology, Faculty of Agriculture",
"PPKU3	c	Department of Plant Pathology, Faculty of Agriculture",
"PPKU4	c	Department of Plant Pathology, Faculty of Agriculture",
"PPKU5	c	Department of Plant Pathology, Faculty of Agriculture",
"PPKU6	c	Department of Plant Pathology, Faculty of Agriculture",
"PPL	s	Agricultural Development and Advisory Service, Harpenden Laboratory",
"PPNP	s	Point Pelee National Park",
"PPPO	s	Pusat Penelitian dan Pengembangan Oseanologi",
"PPPPB	c	South African Plant Pathogenic and Plant Protecting Bacteria",
"PPRI	c	ARC-Plant Protection Research Institute, National Collection of Fungi: Culture Collection",
"PPRZ	s	Plant Protection Research Institute",
"PPSIO	s	P. P. Shirshov Institute of Oceanology",
"PR	s	National Museum in Prague, Department of Botany",
"PRA	s	Institute of Botany, Academy of Sciences",
"PRB	s	Prague Botanical Garden",
"PRC	s	Charles University, Botany Department",
"PRE	s	National Botanical Institute",
"PREM	s	Plant Protection Research Institute, Biosystematics Division, Mycology Unit",
"PRF	s	South African Forestry Research Institute, Environment Affairs Department",
"PRG	s	Universidad Nacional Pedro Ruiz Gallo",
"PRH	s	Perthshire Society of Natural Science",
"PRI	s	College of Eastern Utah, Biology Department",
"PRICO	s	University of Puerto Rico",
"PRL	c	Prairie Regional Laboratory",
"PRM	s	National Museum, Mycological Department",
"PROIMI	c	Planta Piloto de Procesos Industriales Microbiologicos",
"PRU	s	University of Pretoria, Botany Department",
"PRV	s	Porvoo Museum of Natural History",
"PSAE	s	Alberta Environmental Centre",
"PSGB	s	University of Bradford, Pharmacy Department",
"PSM	s	University of Puget Sound, James R. Slater Museum of Natural History",
"PSO	s	Universidad de Narino",
"PSP	s	Parasitic Seed Plants",
"PSS<GBR>	s	Philosophical Society of Southampton",
"PSS<MNG>	s	Paleontology and Stratigraphic Section of the Geological Institute of the Mongolian Academy of Sciences",
"PSU<THA>	s	Prince of Songkla University, Biology Department",
"PSU<USA-OR>	s	Portland State University, Vertebrate Biology Museum",
"PSU<USA-OR>:Mamm		Portland State University, Vertebrate Biology Museum, Mammal Collection",
"PSU<USA-PA>	s	Pennsylvania State University",
"PSUB	s	University of Botswana",
"PSUC	s	Frost Entomological Museum",
"PSUMC	s	Pittsburg State University",
"PSY	s	Paisley Museum",
"PTBG	s	National Tropical Botanical Garden",
"PTCC	c	Pakistan Type Culture Collections",
"PTCCI	c	Persian Type Culture Collection",
"PTH	s	Perth Museum and Art Gallery",
"PTHL	s	Literary and Antiquarian Society of Perth",
"PTIS	s	Potato Introduction Station",
"PTN	s	Ellesmere Chambers",
"PTTC	c	Pranakorn Teacher Training College, Department of Biology, Faculty of Science and Technology",
"PTZ	s	Karelian Scientific Centre, Russian Academy of Sciences",
"PU	s	Princeton University",
"PU-HI	s	Department of Palaeontology, Universidad Complutense of Madrid",
"PUA	s	Pacific Union College, Biology Department",
"PUC<CHN>	s	Beijing University",
"PUC<ZAF>	s	North-West University",
"PUCMNH	s	Pacific Union College, Museum of Natural History",
"PUCP	s	Punjab University",
"PUH	s	University of the Philippines, Institute of Biology",
"PUL	s	Purdue University, Department of Botany and Plant Pathology",
"PUM	s	Universita DI Pisa",
"PUN	s	Punjabi University, Botany Department",
"PUO	s	Portland University",
"PUP	s	University of Peshawar, Botany Department",
"PUR	s	Purdue University, Department of Botany and Plant Pathology",
"PURC	s	Purdue University",
"PUS	s	Puslinch House",
"PUSC	s	University of Southern Colorado, Life Sciences Department",
"PVF	c	Pusat Veterinaria Farma",
"PVGB	c	Plant Virus GenBank",
"PVHR	s	Universite Paris VI",
"PVL	s	Paleontologia de Vertebrados Lillo",
"PVNH	s	ORSTOM",
"PVPH	s	Paleontolga de Vertebrados",
"PVSJ	s	Museo do Ciencias Naturles",
"PW	s	Paleontological Collections",
"PWRC	s	Patuxent Wildlife Research Center",
"PY	s	Centro de Estudios y Colecciones Biologicas para la Conservacion",
"PYCC	c	Portuguese Yeast Culture Collection",
"PYU	s	Yunnan University, Laboratory of Pteridophyta",
"PZL	s	Penzance Library",
"PZV	s	Petrozavodsk State University, Department of Botany and Plant Physiology",
"Q	s	Universidad Central",
"QAME	s	Direccion Nacional Forestal, Ministerio de Agricultura y Ganaderia",
"QAP	s	Universidad Central",
"QBG	s	Queen Sirikit Botanic Garden",
"QC	s	National Museum of Natural History",
"QCA	s	Pontificia Universidad Catolica del Ecuador, Departamento de Biologia",
"QCAZ	s	Museo de Zoologia, Pontifica Universidad Catolica del Ecuador",
"QCNE	s	Museo Ecuatoriano de Ciencias Naturales",
"QD	s	Ocean University of Qingdao, Marine Biology Department",
"QDC	s	Qinghai Institute for Drug Control",
"QDPC	s	Queensland Department of Primary Industries",
"QDPI	s	Queensland Department of Primary Industries",
"QEF	s	Universite Laval, Laboratoire d'ecologie forestiere",
"QF	s	Institute of Forestry",
"QFA	s	Universite Laval",
"QFB	s	Laurentian Forestry Centre, Canadian Forest Service",
"QFBE	s	Laurentian Forestry Centre, Canadian Forest Service",
"QFMQ	s	Queenstown Museum",
"QFS	s	Universite Laval",
"QG	s	General Grassland Station of Qinghai",
"QIBX	s	Qinghai Institute of Biology",
"QIMR	s	Queensland Institute of Medical Research",
"QK	s	Queen's University, Biology Department",
"QM	s	Queensland Museum",
"QM<USA-MA>	s	U.S. Army Natick Research and Development Center",
"QM<ZWE>	s	Queen Victoria Museum",
"QMB	s	Queensland Museum",
"QMC	s	Queen Mary College",
"QMEX	s	Universidad Autonoma de Queretaro, Centro Universitario",
"QMF	s	Queensland Museum",
"QMOR	s	Collection Entomologique Ouellet-Robert",
"QMP	s	Musee du Quebec",
"QPAR	s	Ministere du Tourisme de la Chasse et de la Peche du Quebec, Laboratoire de Recherches",
"QPH	s	Seminaire de Quebec",
"QPIM	s	Department of Primary Industries",
"QPLS	s	Biblioteca Ecuatoriana Aurelio Espinosa Polit",
"QPNRA	s	Programa Nacional de Regionalizacion, Ministerio de Agricultura y Ganaderia, Departamento de Ecologia",
"QRS	s	CSIRO",
"QSA	s	Institut de technologie agroalimentaire",
"QTC	s	Qiqihar Teachers College, Biology Department",
"QUA	s	Qinghai University, Agriculture Department",
"QUC	s	Universite Laval, Departement de biologie",
"QUE	s	Complexe scientifique",
"QUSF	s	Universidad San Francisco de Quito, Biology Department",
"QVG	s	Queen Victoria Museum",
"QVM	s	Queen Victoria Museum",
"QVMS	s	Queen Victoria Memorial Museum",
"QWA	s	University of the North, Qwa Qwa Campus",
"QYTC	s	Qingyang Teachers College, Biology Department",
"R<BRA>	s	Universidade Federal do Rio de Janeiro, Departamento de Botanica",
"R<CHL>	s	Departamento de Geologia, Universidad de Chile",
"RAB	s	Institut Scientifique, Departement de Botanique et d'Ecologie Vegetale",
"RAF	s	Forest Research Institute",
"RAM	s	Ramsey Public Library",
"RAME	s	Royal Albert Memorial Museum",
"RAMK	s	Ramkhamhaeng University, Biology Department",
"RAMM	s	Royal Albert Memorial Museum, Leisure and Tourism Department",
"RANG	s	Yangon University",
"RARS	s	Regina Research Station",
"RAS	s	Union of Burma Applied Research Institute, Pharmaceutical Department",
"RAU	s	Laboratoire de Biologie Vegetale",
"RAW	s	Pakistan Agricultural Research Council",
"RB	s	Jardim Botanico do Rio de Janeiro",
"RBCM	s	Rouyal British Columbia Museum",
"RBE	s	Universidade Federal Rural do Rio de Janeiro",
"RBF	c	Raiffeisen Bioforschung",
"RBINS	s	Royal Belgian Institute of natural sciences",
"RBR	s	Universidade Federal Rural do Rio de Janeiro, Departamento de Botanica",
"RBS	s	Royal Botanic Society",
"RBY	s	East Divisional Library",
"RCAM	s	Rutgers University, Biology Department",
"RCAT	c	Regional Collection of Animal Viruses and Tissue Cultures",
"RCB	c	RIKEN Cell Bank",
"RCC	c	Roscoff Culture Collection",
"RCDM	c	Republican Centre for Deposition of Microorganisms of the National Academy of Sciences and Ministry of Education and Science of Armenia",
"RCHM	s	Rochdale Museum Service",
"RCHP	s	Rochdale Equitable Pioneers' Memorial Museum",
"RCMC	s	Rockford College",
"RCN	s	Richmond Naturalists' Field Club",
"RCR	s	Rochester Public Museum",
"RCS	s	Royal College of Surgeons",
"RCSL	s	Royal College of Surgeons",
"RDAF	s	Research Department South East Asian Fisheries Development Centre",
"RDG	s	Reading Museum and Archive Service",
"RDS	s	Royal Dublin Society",
"RE	s	Liaoning Reed Science Institute",
"RED	s	University of Redlands, Biology Department",
"REDM	s	Reading Museum and Archive Service",
"REG	s	Universitaet Regensburg, Regensburgische Botanische Gesellschaft",
"RELC	s	INIFAP-SAGAR",
"REN	s	Campus Scientifique de Beaulieu, Laboratoire de Botanique",
"RENO	s	University of Nevada, Environmental and Resource Sciences Department",
"RENZ	s	Universitaet Basel, The Swiss Orchid Foundation",
"REP	s	Desert Experiment Station of the W.I.R.",
"RESC	s	Shasta College",
"REU	s	Universite de la Reunion",
"RFA	s	Universidade Federal do Rio de Janeiro, Departamento de Botanica",
"RFE	s	Radcliffe Literary and Scientific Society Museum",
"RFNS	s	Rochdale Field Naturalists' Society",
"RGM	s	Nationaal Natuurhistorisch Museum Leiden",
"RGMC	s	Musee Royal de l'Afrique Centrale",
"RGY	s	Rugby School Natural History Society Museum",
"Rh-EF	s	Museum of Grannat",
"RHLCY	s	Reservoir of Heilongtan",
"RHM	s	Public Reference Library",
"RHMC	s	Red House Museum",
"RHMD	s	National Institute of Science Communication",
"RHMU	c	Department of Pathology, Faculty of Medical Science, Ramathibordi Hospital",
"RHT	s	St. Joseph's College",
"RI	s	Rudjer Boskovic Institute",
"RIA	c	The Russia Research Institute for Antibiotics Culture Collection",
"RIB	c	National Research Institute of Brewing, Tax Administration Agency",
"RIBM	c	Research Institute for Brewing and Malting",
"RICK	s	Brigham Young University - Idaho, Department of Biology",
"RIFFL	s	Research Institute of Freshwater Fisheries",
"RIFY	c	Institute of Enology and Viticulture, Yamanashi University",
"RIG	s	University of Latvia, Department of Botany and Ecology",
"RIGB	s	Latvian State Center of Plant Protection",
"RIGG	s	University of Latvia",
"RIM	s	L'Ecole d'Agriculture",
"RIMD	c	Research Institute for Microbial Diseases, Research Center for Emerging Infectious Diseases",
"RIN	s	Research Institute for Nature Management, Botany Department",
"RIOC	s	Universidad Nacional de Rio Cuarto",
"RIPO	c	Plant Virus Collection",
"RITFC	c	Research Institute for Tobacco and Fibre Crops",
"RIVE<SVK>	c	Research Institute for Viticulture and Enology",
"RIVE<USA-WI>	s	University of Wisconsin, Biology Department",
"RIY	s	National Agriculture and Water Research Center",
"RIZ	s	Instituto de Zootecnia",
"RLS	s	Royal Latin School",
"RM<CAN>	s	McGill University, Redpath Museum",
"RM<NZL>	c	Rumen Microorganisms",
"RM<SGP>	s	Raffles Museum",
"RM<USA-WY>	s	University of Wyoming, Department of Botany, Dept. 3165",
"RMBL	s	Rocky Mountain Biological Laboratory",
"RMBR	s	Raffles Museum of Biodiversity Research",
"RMCA	s	Royal Museum for Central Africa",
"RMDRC	s	Rocky Mountain Dinosaur Resource Center",
"RMF	c	Rocky Mountain Herbarium, Fungi",
"RMFM	s	Richmond Marine Fossils Museum",
"RMNH	s	Nationaal Natuurhistorisch Museum",
"RMNH:ACA	s	Nationaal Natuurhistorisch Museum, Acari collection",
"RMNH:AVES	s	Nationaal Natuurhistorisch Museum, bird collection",
"RMNH:BRA	s	Nationaal Natuurhistorisch Museum, brachiopod collection",
"RMNH:BRY	s	Nationaal Natuurhistorisch Museum, bryozoan collection",
"RMNH:COEL	s	Nationaal Natuurhistorisch Museum, Coelomate collection",
"RMNH:CRUS	s	Nationaal Natuurhistorisch Museum, crustacean collection",
"RMNH:CRUS.D	s	Nationaal Natuurhistorisch Museum, decapod collection",
"RMNH:INS	s	Nationaal Natuurhistorisch Museum, Insect collection",
"RMNH:MOL		Nationaal Natuurhistorisch Museum, mollusc collection",
"RMNH:PISC	s	Nationaal Natuurhistorisch Museum, fish collection",
"RMNH:POR	s	Nationaal Natuurhistorisch Museum, poriferan collection",
"RMNP	s	Riding Mountain National Park",
"RMRC	b	Regional Medical Research Centre",
"RMS	s	University of Wyoming, Department of Botany, Dept. 3165",
"RMSC	s	Rocky Mountain Forest and Range Experiment Station",
"RMWC	s	Randolph-Macon Woman's College, Biology Department",
"RNG	s	University of Reading",
"RNMUT	s	Republic Nature Museum of Uzbekistan",
"RO	s	Universita degli Studi di Roma La Sapienza, Dipartimento di Biologia Vegetale",
"ROAN	s	Virginia Western Community College, Biology Department",
"ROCH	s	Rochester Academy of Science",
"ROHB	s	Cattedra di Micologia, Dipartimento di Biologia Vegetale",
"ROIG	s	Estacion Experimental de Plantas Medicinales Dr. Juan T. Roig",
"ROM	s	Royal Ontario Museum",
"ROM:ENT	s	Royal Ontario Museum, Entomology collection",
"ROM:HERP	s	Royal Ontario Museum, Herpetology collection",
"ROM:ICH	s	Royal Ontario Museum, Fish collection",
"ROM:MAMM	s	Royal Ontario Museum, Mammal Collection",
"ROME	s	Royal Ontario Museum",
"ROML	s	National University of Lesotho, Biology Department",
"ROMO	s	Rocky Mountain National Park",
"ROO	s	Agricultural Research Council-Range and Forage Institute",
"ROPA	s	Sonoma State University, Biology Department",
"ROPV	s	Istituto Sperimentale per la Patologia Vegetale",
"ROST	s	Universitaet Rostock",
"ROV	s	Museo Civico di Rovereto",
"ROZ	s	Stredoceske muzeum",
"RPM	s	Reading Public Museum",
"RPMH	s	Roemer Pelizaeus Museum Hildesheim",
"RPN	s	Ripon Mechanics Institute",
"RPPMC	s	Rondeau Provincial Park",
"RPPR	s	Internationl Institute of Tropical Forestry",
"RPSC	s	Rio Palenque Science Center",
"RPSP	s	Universidade de Sao Paulo",
"RPTN	s	Repton School, Biology Department",
"RRCBI	s	Regional Research Centre (Ay.)",
"RRIASR	c	Fungal Pathogens of Hevea Rubber in Sri Lanka",
"RRJ	c	RRL , Jammu INDIA",
"RRLB	s	Regional Research Laboratory",
"RRLH	s	Regional Research Laboratory",
"RSA	s	Rancho Santa Ana Botanic Garden",
"RSCS	c	Medical Culture Collection",
"RSDR	s	Desert Institute, Turkmenistan Academy of Sciences",
"RSKK	c	Refik Saydam National Type Culture Collection-RSKK",
"RSM<CAN>	s	Royal Saskatchewan Museum",
"RSM<GBR>	s	Royal Scottish Museum",
"RSME	s	National Museums of Scotland",
"RSY	s	Buteshire Natural History Society, The Museum",
"RTE	s	Holmesdale Natural History Club Museum",
"RTHM	s	Munidipal Museum and Art Gallery",
"RTMP	s	Royal Tyrell Museum of Paleontology",
"RU	s	University of Reading, Agricultural Botany Department",
"RUBL	s	University of Rajasthan, Botany Department",
"RUDZ	s	Rhodes University",
"RUEB	s	Eidgenoessische Technische Hochschule ETH",
"RUH	s	Rhodes University, Botany Department",
"RUHV	s	Radford University, Biology Department",
"RUIC	s	the State University",
"RUNYON	s	Robert Runyon Herbarium",
"RUSI	s	J.L.B. Smith Institute of Ichthyology (formerly of Rhodes University)",
"RUSU	s	Universidade Santa Ursula",
"RUT	s	Douglass College, Rutgers University, Biological Sciences Department",
"RUTPP	s	Cook College, Rutgers University, Plant Pathology Department",
"RUY	s	Mid-Staffordshire Field Club",
"RV<ITA>	c	Collection of Leptospira Strains",
"RV<RUS>	s	Molotov State University of Rostov, Botany Department",
"RVP	s	Museo Provincial de Historia Natural de La Pampa (Argentina)",
"RWBG	s	Rostov State University",
"RWC	s	State University of New York, Roosevelt Wildlife Collection",
"RWDN	s	University of Glasgow Field Station",
"RWPM	s	Roger Williams Park Museum of Natural History",
"RYCC	c	Roseworthy Yeast Culture Collection",
"RYD	s	School of Art",
"RYU	s	University of the Ryukyus, Biology Department",
"S	s	Swedish Museum of Natural History, Botany Departments",
"SA	s	Museum national d'Histoire Naturelle (MNHN)",
"SAAR	s	Zentrum fuer Biodokumentation des Saarlandes",
"SAAS	s	Saasveld, Port Elizabeth Technikon",
"SAB	s	Society of Amateur Botanists",
"SACA	s	South-west Agricultural University",
"SACC	s	St. Ambrose College",
"SACL	s	Santa Clara University, Biology Department",
"SACON	s	Salim Ali Centre for Ornithology and Natural History",
"SACS	s	Shenyang Agricultural College",
"SACT	s	California State University, Biological Sciences Department",
"SAFB	s	University of Saskatchewan, Forestry Department",
"SAFU	s	University of San Francisco",
"SAG	c	Sammlung von Algenkulturen at Universitat Gottingen",
"SAIAB	s	South African Institute of Aquatic Biodiversity",
"SAIM	s	South African Institute for Medical Research",
"SAIMR	s	South African Institute for Medical Research",
"SAITP	c	School of Pharmacy and Medical Sciences, University of South Australia",
"SAK	s	Institute of Marine Geology and Geophysics, Far East Branch, Island Ecological Problems Department",
"SAKH	s	Sakhalin Botanical Garden",
"SAL	s	Kansas Wesleyan University, Biology Department",
"SALA	s	Universidad de Salamanca, Departamento de Botanica",
"SALAF	s	Universidad de Salamanca",
"SALF	s	Salford Royal Free Museum and Library",
"SALFM	s	Salford Natural History Museum, Salford City Council",
"SALLE	s	Instituto Geobiologico La Salle",
"SAM	s	South African Museum",
"SAMA	s	South Australia Museum",
"SAMC	s	Iziko Museum of Capetown",
"SAMU	s	Savaria Museum, Department of Natural History",
"SAN	s	Forest Research Centre, Forest Department",
"SANBI	s	South African National Biodiversity Institute",
"SANC	s	South African National Collection of Insects",
"SANT	s	Universidad de Santiago de Compostela",
"SANT:Algae	s	Universidad de Santiago de Compostela, algae collection",
"SANT:Bryo	s	Universidad de Santiago de Compostela, bryophyte collection",
"SANT:Lich	s	Universidad de Santiago de Compostela, lichen collection",
"SANU	s	Shaanxi Normal University, Biology Department",
"SAO	s	Sammlung Oberli",
"SAP	s	Herbarium of Graduate School of Science, Hokkaido University",
"SAPA	s	Hokkaido University Museum",
"SAPCL	s	St. Andrews Presbyterian College, Biology Department",
"SAPS	s	Hokkaido University Museum",
"SAR	s	Department of Forestry",
"SARA	s	Zemaljski Muzej Bosne I. Herzegovine",
"SARAT	s	Department of Morphology and Systematic Botany",
"SARC	s	Roanoke College, Biology Department",
"SAS	s	Sammlung Arnhardt des Museums Schloss Wilhelmsburg Schmalkalden",
"SASK	s	University of Saskatchewan, Plant Sciences Department",
"SASSA	s	Universita di Sassari, Dipartimento di Scienze del Farmaco",
"SASSC	b	SENDAI Arabidopsis Seed Stock Center",
"SASY	s	Institute for Biological Problems of Cryolithozone",
"SAT	s	Angelo State University, Biology Department",
"SAU<CHN>	s	Sichuan Agricultural University, Department of Basic Courses",
"SAU<USA-AR>	s	Southern Arkansas University",
"SAUF	s	Sichuan Agricultural University, Forestry Department",
"SAUT	s	Sichuan Agricultural University",
"SAV	s	Institute of Botany, Slovak Academy of Sciences",
"SAWV	s	Salem International University, Department of Bioscience",
"SB	s	Saint Bernard Abbey",
"SBBG	s	Santa Barbara Botanic Garden",
"SBC	s	University of California, Biological Sciences Department",
"SBCC	s	Santa Barbara City College, Department of Biological Sciences",
"SBCM	s	San Bernardino County Museum",
"SBDE	s	Sino-Belgian Dinosaur expedition",
"SBG	s	Botanical Garden of Stavropol State University, Botany Department",
"SBKA	s	Stiftssammlungen des Benediktinerstiftskrems-Munster",
"SBM<MYS>	s	Sabah Museum",
"SBM<USA-CA>	s	Santa Barbara Museum of Natural History",
"SBMN	s	Santa Barbara Museum of Natural History",
"SBMNH	s	Santa Barbara Museum of Natural History",
"SBNHM	s	Santa Barbara Natural History Museum",
"SBSC	s	Robert A. Vines Environmental Science Center",
"SBSFU	c	School of Biological Sciences",
"SBT	s	Bergius Foundation",
"SBU	s	Saint Bonaventure University, Biology Department",
"SBY	s	Salisbury and South Wiltshire Museum",
"SC	s	Salem College, Biology Department",
"SCA	s	Limbe Botanical & Zoological Gardens",
"SCAC	s	South China Agricultural College",
"SCAR	s	Wood End Museum",
"SCARB	s	Boys' High School",
"SCB	s	Station Centrale de Boukoko",
"SCCBC	s	Selkirk College, Environmental Sciences and Technologies Department",
"SCDH	s	South Carolina Department of Health and Environmental Control",
"SCFI	s	Sichuan Academy of Forestry",
"SCFQ	s	Service canadien de la faune",
"SCH	s	Museum zu Allerheiligen",
"SCHG	s	George Museum",
"SCHN	s	Smith College, Biological Sciences Department",
"SCL	s	St. Cloud State University, Department of Biological Sciences",
"SCM	s	Sheffield City Museums",
"SCN	s	Sociedad de Ciencias Naturales \"La Salle\"",
"SCNHM	s	Southwestern College, Natural History Museum",
"SCNU	s	Sichuan Normal University, Biology Department",
"SCPS	s	Scarborough Philosophical and Archaeological Society Museum",
"SCR	s	Scripps Institution of Oceanography, University of California",
"SCS	s	Agriculture and Agri-Food Canada, Semiarid Prairie",
"SCSC	s	Saint Cloud State University",
"SCSFRI	s	South China Sea Fisheries Research Institute",
"SCSM	s	South Carolina State Museum",
"SCT	s	Friend's School",
"SCU	s	Shandong Christian University",
"SCUF	s	Universidade Federal de Santa Catarina",
"SCZ	s	Smithsonian Tropical Research Institute",
"SD<USA-CA>	s	San Diego Natural History Museum",
"SDAKS	s	South Dakota State University",
"SDAU	s	Shandong Agricultural University",
"SDC	s	South Dakota State University, Department of Biology",
"SDCM	s	Shandong College of Traditional Chinese Medicine",
"SDFI	s	Shandong Forestry Institute",
"SDFS	s	Shandong Forestry School",
"SDI	s	Southend Institute",
"SDM<GBR>	s	Stroud and District Museum",
"SDM<USA-CA>	s	San Diego Mesa College, Botany Department",
"SDMC	s	San Diego Natural History Museum",
"SDMP	s	Shandong Institute of Traditional Chinese Medicine and Materia Medica",
"SDN	s	Borough of Thamesdown Museum and Art Gallery",
"SDNH<SWZ>	s	Malkerns Agricultural Research Station",
"SDNH<USA-CA>	s	Natural History Museum",
"SDNHM	s	San Diego Natural History Museum",
"SDNU	s	Shandong Normal University, Biology Department",
"SDSM	s	South Dakota School of Mines and Technology",
"SDSU<USA-CA>	s	San Diego State University, Department of Biology",
"SDSU<USA-SD>	s	Severin-McDaniel Insect Collection",
"SDU	s	University of South Dakota, Department of Biology",
"SEAC	s	Estacion Experimental de Agricolas de la Campara",
"SEAMEO	c	Seameo-Biotrop",
"SEAN	s	Museo Entomologico",
"SEBR	c	Sanofi ELF Biorecherches",
"SECM	s	Science Education Center",
"SEFES	s	Southeastern Forest Experiment Station",
"SEFSC	s	Southeast Fisheries Science Center",
"SEFSC:MMMGL	s	Southeast Fisheries Science Center, Marine Mammal Molecular Genetics Laboratory",
"SEIG	s	Societa Entomologica Italiana",
"SEL	s	Marie Selby Botanical Gardens",
"SELU	s	Southeastern Louisiana University, Biological Sciences Department",
"SEMC	s	Snow Entomological Museum",
"SEMIA	c	Colecao de Culturas de Rhizobium da Fepagro",
"SEMK	s	Snow Entomological Museum",
"SEMM	s	Station Experimental de la Maboke",
"SEMO	s	Southeast Missouri State University, Department of Biology",
"SERG	s	Institut de Recherche Agronomique de Guinee",
"SERO	s	Sociedad para el Estudio de los Recursos Bioticos de Oaxaca",
"SES	s	Southeastern Shanxi Teachers School, Biochemistry Department",
"SETON	s	Philmont Scout Ranch, Seton Memorial Library",
"SEV	s	Universidad de Sevilla, Departamento de Biologia Vegetal y Ecologia",
"SEVF	s	Universidad de Sevilla, Departamento de Botanica",
"SEY	s	Seychelles Natural History Museum",
"SF	s	Universidad Nacional del Litoral",
"SFAC	s	Stephen F. Austin State University",
"SFB	s	Salgues Foundation of Brignoles for Development of Biological Sciences",
"SFC	s	Laboratory of Fishes",
"SFD	s	Sheffield Galleries and Museums Trust, City Museum",
"SFDK	s	Sarawak Forestry Department",
"SFDL	s	Sheffield Literary and Philosophical Society",
"SFDN	s	Sheffield Naturalists' Club",
"SFI	b	Slovenian Forestry Institute",
"SFPA	s	Fundacao Estadual de Pesquisa Agropecuaria",
"SFRF	s	Forest Service, Region 5, USDA",
"SFRP	s	Southern Research Station",
"SFRS	s	Sea Fisheries Research Station",
"SFS	s	Universite de Sherbrooke, Departement de biologie",
"SFSU	s	San Francisco State University, Department of Biology",
"SFT	s	Stowlangtoft Hall",
"SFU	s	Shanghai Fisheries University",
"SFUC	s	Simon Fraser University",
"SFUV	s	Simon Fraser University, Biological Sciences Department",
"SFV	s	California State University, Department of Biology",
"SFVS	s	San Fernando Valley State University",
"SG	s	Shanghai Botanical Garden",
"SGBH	s	Museum of Staffordshire County",
"SGE	s	Stamford Park Museum",
"SGEL	s	Stalybridge Library",
"SGGS	s	The Museum",
"SGMA	s	Universidade Nova de Lisboa",
"SGO	s	Museo Nacional de Historia Natural",
"SGSC	c	Salmonella Genetic Stock Centre",
"SGWG	s	Sammlung der Sektion Geologische Wissenschaften der Ernst-Moritz-Arndt University",
"SH	s	Academia Sinica",
"SHB	s	Shanghai Baptist College",
"SHC	s	Sacred Heart College",
"SHCT	s	Shanghai Teachers College of Technology, Biology Department",
"SHD	s	University of Sheffield, Botany Department",
"SHDC	s	Shanghai Institute for Drug Control",
"SHG	s	Sohag University, Botany Department",
"SHI	s	Shihezi Agricultural College, Biology Department",
"SHIN	s	Shinshu University",
"SHJ	s	St. John's University",
"SHM<CHN>	s	Shanghai Museum of Natural History, Botanical Department",
"SHM<USA-SD>	s	Siouxland Heritage Museum",
"SHMC	s	Luther College, Sherman A. Hoslett Museum of Natural History",
"SHMH	s	Universite l'Aurore, Musee Heude",
"SHMI	s	Shanghai Institute of Materia Medica, Chinese Academy of Sciences, Phytochemistry Department",
"SHMU	s	Shanghai Medical University",
"SHOR	s	Shorter College, Biology Department",
"SHSND	s	North Dakota Heritage Center",
"SHST	s	Sam Houston State University, Department of Biological Sciences",
"SHSU	s	Sam Houston State University, Vertebrate Natural History Collection",
"SHTC	s	California State University, Biology Department",
"SHTU	s	Shanghai Teachers University, Biology Department",
"SHY	s	Rowley's House Museum",
"SHYAN	s	Shropshire Archaeological and Natural History Society",
"SHYB	s	Shrewsbury School",
"SHYL	s	Shrewsbury School",
"SHYN	s	Shrewsbury Natural History Society",
"SHYP	s	Shrewsbury Public Library",
"SI	s	Instituto de Botanica Darwinion",
"SIAC<CHN>	s	Sichuan Institute of Agriculture",
"SIAC<ITA>	s	Accademia dei Fisiocritici Onlus",
"SIB	s	Sibiu Natural History Museum",
"SIBAC	s	Southwest Institute of Biology",
"SICH	s	Simpson College, Biology and Environmental Sciences Department",
"SIEMEA	s	Severtsov Insitute for Evolutionary Morphology and Animal Ecology",
"SIENA	s	Universita di Siena, Dipartimento di Scienze Ambientali \"G. Sarfatti\"",
"SIF	s	Senckenbergisches Institut",
"SIFS	s	Sichuan Forestry School",
"SIGMA	s	Station Internationale de Geobotanique Mediterraneenne et Alpine",
"SIIS	s	Staten Island Institute of Arts and Sciences",
"SIM	s	Staten Island Institute of Arts and Sciences, Science Department",
"SIMF	s	Taurida National University, Botany Department",
"SIMS	s	Sherkin Island Marine Station",
"SING	s	Singapore Botanic Gardens",
"SINU	s	National University of Singapore, Biological Sciences Department",
"SIO	s	Scripps Institution of Oceanography",
"SIO:BIC	s	Scripps Institution of Oceanography, Benthic Invertebrates Collection",
"SITC	s	Sichuan Teachers College, Biology Department",
"SIU	s	Southern Illinois University, Plant Biology Department",
"SIUC	s	Research Museum of Zoology",
"SIUCM	s	Southern Illinois University",
"SIUE	s	Southern Illinois University, Edwardsville",
"SIZK	s	Schmaulhausen Institute of Zoology",
"SJ	s	Departamento de Recursos Naturales y Ambientales",
"SJAC	s	San Joaquin County Agriculture Commissioner",
"SJC	s	Sir John Cass College, Chemistry and Biology Department",
"SJCA	s	St. John's College",
"SJCRY	s	St. John's College of Ripon and York",
"SJER	s	United States Forest Service, San Joaquin Experimental Range",
"SJFM	s	Fairbanks Museum and Planetarium",
"SJNM	s	San Juan College",
"SJPC	s	Sergei J. Paramonov personal collection -- destroyed",
"SJRP	s	UNESP, Campus Sao Jose Rio Preto, Departamento Zoologia e Botanica",
"SJSC	s	San Jose State University, J. Gordon Edwards Museum of Entomology",
"SJSU	s	San Jose State University, Biological Sciences Department",
"SJUBC	s	Saint John's University, Biology Collections",
"SK	s	Katedralskolan",
"SKK	s	Sung Kyun Kwan University, Biological Sciences Department",
"SKM	s	Skokholm Field Centre",
"SKN	s	Craven Museum Service",
"SKR	s	Latvian Research Institute of Agriculture, Plant Protection Department",
"SKT	s	Stockport Heritage Services",
"SKU	s	Sri Krishnadevaraya University, Botany Department",
"SKUK	c	Simpanan Kultur Universiti Kebangsaan",
"SL	s	University of Sierra Leone, Njala University College, Biological Sciences Department",
"SLBI	s	South London Botanical Institute",
"SLC	s	East High School, Science Department",
"SLFC	s	Slapton Ley Field Centre",
"SlgInnsb	s	Paleontological Collection",
"SLJG	s	Steiermarkisches Landesmuseum Joanneum",
"SLL	s	Societe Linneenne de Lyon",
"SLO	s	Komenskeho University, Katedra botaniky",
"SLPM	s	Universidad Autonoma de San Luis Potosi",
"SLRO	s	Slippery Rock University, Biology Department",
"SLSC	s	St. Louis, St. Louis Science Center",
"SLSK	s	St. Leonard's and St. Katherine's Schools",
"SLTC	s	Teachers College, Botany Department",
"SLU<CAN>	s	Laurentian University, Biology Department",
"SLU<USA-LA>	s	Southeastern Louisiana University, Vertebrate Museum",
"SLUB	s	St. Louis University Museum",
"SM<CHN>	s	Chongqing Municipal Academy of Chinese Materia Medica",
"SM<DEU-Frankfurt>	s	Senckenberg Museum",
"SM<DEU-Langenaltheim>	s	Schwegler Museum",
"SM<MYS>	s	Sarawak Museum",
"SM<USA-FL>	s	Sanford Museum Collections",
"SM<USA-TX>	s	Strecker Museum, Baylor University",
"SMAO	s	Simao Forestry Bureau",
"SMB	s	Marianske Muzeum, Natural History Department",
"SMBB	s	Stredoslovenske muzeum",
"SMBL	s	Seto Marine Biological Laboratory, Kyoto University",
"SMC	s	Sedgwick Museum",
"SMCC	c	Subsurface Microbial Culture Collection",
"SMCC-W	c	Subsurface Microbial Culture Collection--Western Branch",
"SMCW	s	Saint Michael's College, Biology Department",
"SMDB	s	Universidade Federal de Santa Maria, Departamento de Biologia",
"SME<FRA>	s	Station Marine d'Endoume",
"SME<GBR>	s	Sedgwick Museum of Geology",
"SMF<DEU>	s	Forschungsinstitut und Natur-Museum Senckenberg",
"SMF<PER>	s	Universidad Nacional Mayor de San Marcos",
"SMH	s	Saint Meinrad College of Liberal Arts, Biology Department",
"SMI	s	Prince Rupert Forest Region, Research Section",
"SMIP	c	Secao de Maricultura",
"SMJM	s	Sabah Museum",
"SMK	s	Sarawak Museum",
"SMKM	s	Selangor Museum",
"SMM	s	Science Museum of Minnesota",
"SMMC	s	Second Military Medical College",
"SMN	s	Simao District National Medical and Pharmaceutical Institute",
"SMNG	s	Staatliches Museum fuer Naturkunde",
"SMNH<CAN>	s	Saskatchewan Museum of Natural History",
"SMNH<SWE>	s	Department of Paleozoology, Swedish Museum of Natural History",
"SMNH<USA-KS>	s	Schmidt Museum of Natural History, Emporia State University",
"SMNK	s	Staatliches Museum fuer Naturkunde Karlsruhe (State Museum of Natural History)",
"SMNS	s	Staatliches Museum fuer Naturkund Stuttgart",
"SMOC	s	Slezske Muzeum Opava",
"SMP<SUR>	s	Surinaams Museum",
"SMP<USA-PA>	s	The State Museum of Pennsylvania",
"SMPM	s	Science Museum of Minnesota",
"SMR	s	Samara State University, Department for Ecology, Botany, and Nature Protection",
"SMRG	c	Soil Microbiology Research Group, Division of Soil Science, Department of Agriculture",
"SMRS	s	Stavropol Museum of Regional Studies",
"SMS	s	Missouri State University, Department of Biology",
"SMSM	s	Sarawak Museum",
"SMTWA	c	School of Medical Technology Western Australia",
"SMU	s	St. Mary's University",
"SMU<KOR>	s	Sangmiung University",
"SMU<USA-TX>	s	Shuler Museum of Paleontology, Southern Methodist University",
"SMVM	s	National Archives and Museum",
"SMW<GBR>	s	School of Medicine for Women",
"SMW<NAM>	s	State Museum",
"SMWN	s	State Museum",
"SMWU	s	Sang Miung Women's University",
"SN	s	South China Normal University, Biology Department",
"SNC	s	Saint Norbert College",
"SNCBSH	s	State of North Carolina Biological Station",
"SNGM	s	Coleccion Paleontologica",
"SNHM	s	Sudan Natural History Museum",
"SNHS	s	Guildford Museum",
"SNM<SVK>	s	Slovak National Museum",
"SNM<USA-NM>	s	Western New Mexico University, Department of Natural Sciences",
"SNMB	s	Staatliches Naturhistorisches Museum",
"SNMBR	s	Staatliches Naturhistorisches Museum in Braunschweig",
"SNMC	s	Slovenske Narodne Muzeum",
"SNMG	s	Staatliches Museum fuer Naturkunde",
"SNMNH	s	Saudi Arabian National Museum of Natural History",
"SNP<MYS>	s	Sabah Parks, Botany Section",
"SNPH	s	Sehlabathebe National Park",
"SNU	s	Seoul National University, School of Biological Sciences",
"SNUA	s	Seoul National University, The Arboretum",
"SNW	s	Shropshire and North Wales Natural History and Antiquarian Society",
"SO	s	Sofia University \"St. Kliment Ohridski\", Botany Department",
"SOA	s	Agricultural University of Plovdiv, Botany Department",
"SOB	s	Husite Museum Tabor",
"SOC	s	Southern Oregon University, Biology Department",
"SOFM	s	National Museum of Natural History",
"SOGS	s	Pal. Coll, Sokoto State Government Palaeontological Collection",
"SOIC	s	Natural History Museum, National Insect Collection",
"SOKO	s	Okresni muzeum Sokolov (Regional Muzeum), Botany Department",
"SOM	s	Bulgarian Academy of Sciences",
"SOMF	s	Bulgarian Academy of Sciences",
"SOSCMVNH	s	Southern Oregon State College, Museum of Vertebrate Natural History",
"SOSN	s	Silesian Medical School in Katowice, Department of Pharmaceutical Botany",
"SOTO	s	College of the Ozarks, Biology Department",
"SOTON	s	Southampton University",
"SOUT	s	Long Island University",
"SP	s	Instituto de Botanica",
"SPA	s	Swedish Museum of Natural History",
"SPAL	s	Municipio di Reggio Emilia, Musei Civici",
"SPB	s	Universidade de Sao Paulo",
"SPC	s	Seattle Pacific University, Biology Department, Suite 205",
"SPF	s	Universidade de Sao Paulo, Departamento de Botanica",
"SPFR	s	Universidade de Sao Paulo, Departamento de Biologia",
"SPH	s	Fox Research Forest",
"SPI	s	Stavropol Pedagogical Institute, Botany Department",
"SPL	s	Palynological Laboratory",
"SPLT	s	South Plains College, Science Department",
"SPM	s	Sabah Parks",
"SPMCC	c	Sungei Putih Microbial Culture Collection",
"SPMO	c	Salt Plains Microbial Observatory",
"SPMS	s	University of South Florida",
"SPN	s	Southampton University, Biology Department",
"SPNRI	s	Sichuan Province, Natural Resources Institute",
"SPR	s	Springfield Science Museum, Natural Science Department",
"SPRY	s	Burton Constable Foundation",
"SPSF	s	Instituto Florestal",
"SPT	s	Botanic Gardens Museum",
"SPTS	s	Southport Scientific Society",
"SPWH	s	Marine Biological Laboratory",
"SQF	s	Universidad de Chile, Laboratorio de Botanica, Escuela de Quimica y Farmacia",
"SR	s	Sichuan Institute of Natural Resources",
"SRAICC	c	SRAI's culture collection",
"SRCG	s	Baylor University",
"SRD	s	Passmore Edwards Museum",
"SRF	s	Shangrao Forestry Institute",
"SRFA	s	Universidad Nacional de La Pampa",
"SRGH	s	Botanic Garden",
"SRI	s	Serengetti Research Institute",
"SRNP	s	Insects of the Area de Conservacion Guanacaste (ACG), northwestern Costa Rica",
"SRP	s	Boise State University, Biology Department",
"SRR	s	Koninklijke Shell (Shell Research N.V.)",
"SRRC	c	Southern Regional Research Center, Agricultural Research Service, United States Department of Agriculture",
"SRSC	s	Sul Ross State University, Department of Biology",
"SRSU	s	Sul Ross State University",
"SS	s	Universita di Sassari, Dipartimento di Botanica ed Ecologia Vegetale",
"SSC	s	Sacramento State University",
"SSCMU	c	Soil Science and Conservation Department Faculty of Agriculture",
"SSCN	s	Musum of the Biological Laboratory",
"SSD	s	Sammlung Simon des Stattlichen Museum fur Mineralogie und Geologie Dresden",
"SSF	s	Sammlung des Senckenbrug-Museum",
"SSIC	c	Collaborating Centre for Reference and Research on Escherichia and Klebsiella",
"SSJC	s	San Joaquin County, Agriculture Department",
"SSKKU	c	Department of Soil Science, Faculty of Agriculture",
"SSL	s	Sammlung Langenhan an der Sektion Geophysik der Karl-Marx-Universitat Lepzig",
"SSLP	s	Rocky Mountain Research Station",
"SSM<USA-GA>	s	Savannah Science Museum",
"SSM<USA-MA>	s	Springfield Science Museum",
"SSMF	s	Great Lakes Forestry Centre, Canadian Forest Service",
"SSMJI	c	Science Section, Department of General Education, Faculty of Agricultural Business",
"SSMM	s	Shanxi School of Chinese Materia Medica",
"SSMS	s	Suriname State Museum",
"SSNR	s	Societa per GL Studi Naturalistica della Romagna",
"SSOFM	s	Sanabe Shizenkan Open Field Museum",
"SSPW	s	Perivale Wood Nature Reserve",
"SSU	s	Saratov State University",
"SSUC	s	Pontificia Universidad Catolica de Chile, Departamento de Ecologia",
"ST	s	Suzhou Teachers College, Biology Department",
"STA	s	University of St. Andrews, School of Environmental and Evolutionary Biology",
"STAL	s	Verulamium Museum",
"STAR	s	Arkansas State University, Biological Sciences Department",
"STASH	s	St. Beuno's College",
"STB	s	St. Bartholomew's Hospital",
"STC	s	Sichuan Teacher's College",
"STCR	s	Universite de la Reunion",
"STD	s	Prittlewell Priory Museum",
"STDCM	s	Southend Central Museum",
"STE	s	National Botanical Institute",
"STEU	s	University of Stellenbosch, Botany Department",
"STFX	s	St. Francis Xavier University, Biology Department",
"STG	s	St. Martin's Convent",
"STI	s	Stirling Smith Art Gallery and Museum",
"STIU	s	University of Stirling, Biological Sciences Department",
"STK	s	Stoke-on-Trent Athenaeum",
"STL	s	Instituto Nacional de Limnologia, Departamento Macrofitas",
"STM<DEU>	s	Stettinger Museum",
"STM<GBR>	s	Streatham Antiquarian and Natural History Society",
"STMC	s	School of Tropical Medicine",
"STO	s	The Potteries Museum & Art Gallery",
"STP	s	La Societe Guernesiaise, Priaulx Library",
"STPCM	s	Island Museum, Candie Gardens",
"STPE	s	Florida Marine Research Institute, Florida Department of Environmental Protection",
"STPS	s	St. Paul's School",
"STR	s	Institut de Botanique",
"STRI	s	Smithsonian Tropical Research Institute",
"STS	s	Stromness Museum",
"STT	s	St. Thomas's Hospital Medical School Library",
"STU	s	Staatliches Museum fuer Naturkunde",
"STUM	s	Santo Tomas University Museum",
"SU<CHN>	s	Suzhou University",
"SU<USA-CA>	s	Stanford University",
"SU<USA-OR>	s	Oregon State University",
"SUA	s	Sokoine University of Agriculture, Forest Biology Department",
"SUB	s	Universitat Bonn",
"SUCEA	s	The University at Albany",
"SUCH	s	Sukhumi Botanical Garden of Georgian Academy of Sciences",
"SUCN	s	State University of California",
"SUCO	s	State University of New York, College at Oneonta, Biology Department",
"SUD	s	Stroud and District Museum",
"SUEL	s	Natural History Museum of Bakony Mountains",
"SUF	s	Shimonoseki University of Fisheries",
"SUHC	s	Salisbury University, Department of Biology",
"SUM<CZE>	s	Okresni vlastivedne muzeum v Sumperku",
"SUM<ZAF>	s	Stellenbosch University",
"SUN	s	Sunderland Museum",
"SUND	s	Sunderland Natural History and Antiquarian Society",
"SUNIV	s	University of Stockholm",
"SUNYO	s	State University of New York at Oneonta",
"SUVA	s	University of the South Pacific",
"SUVM	s	Shippensburg University, Vertebrate Museum",
"SUWS	s	University of Wisconsin-Superior, Department of Biology and Earth Science",
"SV	s	Antigua Estacion Experimental Agronomica",
"SVCK	c	Sammlung von Conjugaten Kulturen",
"SVER	s	Institute of Plant and Animal Ecology, Laboratory of Plant Ecology and Geobotany",
"SVG	s	Arkeologisk museum i Stavanger",
"SVIEC	c	Secao de Virus",
"SVVC	s	Seminario Vescovile",
"SWA	s	Swansea Museum",
"SWAU	s	Southwest Agricultural University, Horticulture Department",
"SWBR	s	Sweet Briar College, Biology Department",
"SWC<GBR>	s	Sammlung des Cambridge, University of Zoology",
"SWC<USA-PA>	s	Swarthmore College, Biology Department",
"SWCTU	s	Southwest Teachers University, Biology Department",
"SWE	s	Chandos House, Stowe School",
"SWF	s	Florida Gulf Coast University",
"SWFC	s	Southwest Forestry College",
"SWFSC	s	Southwest Fisheries Science Center",
"SWIBASC	s	Academia Sinica",
"SWMT	s	Rhodes College, Biology Department",
"SWN	s	Saffron Walden Museum",
"SWNHS	s	Saffron Walden Horticultural Society",
"SWRS	s	Southwestern Research Station",
"SWSL	s	USDA/ARS, Southern Weed Science Research Unit",
"SWT	s	Southwest Texas State University, Department of Biology",
"SWTN	s	Swinton and Pendlebury Botanical Society",
"SWU2	c	Department of Biology, Faculty of Science",
"SXAU	s	Shanxi Agricultural University, Forestry Department",
"SXDC	s	Shaanxi Institute for Drug Control",
"SXMP	s	Shaanxi Academy of Traditional Chinese Medicine and Pharmacology",
"SXU	s	Shanxi University, Biology Department",
"SY	s	Shenyang Municipal Academy of Landscape Gardening",
"SYAU	s	Shenyang Agricultural University",
"SYAUF	s	Shenyang Agricultural University, Forestry Department",
"SYD	s	University of Sydney",
"SYKO	s	Komi Scientific Centre, Ural Division, Russian Academy of Sciences, Department of Geobotany and Plant Cover Restoration",
"SYKT	s	Syktyvkar State University, Botany Department",
"SYPC	s	Shenyang College of Pharmacy, Pharmaceutical Botany Department",
"SYR	s	Syracuse University, Plant Sciences Department",
"SYRF	s	State University of New York",
"SYS	s	Zhongshan (Sun Yatsen) University, Biology Department",
"SYSU	s	National Sun Yat-Sen University, Department of Biological Sciences",
"SYT	s	Stonyhurst College",
"SZ	s	Sichuan University, Biological Department",
"SZB	s	Haus der Natur",
"SZCU	s	Department of Systematic Zoology",
"SZE<HUN>	s	Mora Ferenc Museum, Natural Science Department",
"SZE<TUR>	s	Zoology Department, Aegean University, Science Faculty",
"SZG	s	Shenzhen Fairy Lake Botanical Garden",
"SZL	s	Landesherbar von Salzburg",
"SZM	s	Saitama Zoogeographical Museum",
"SZMC	c	Szeged Microbiological Collection",
"SZMN	s	Institute of Animal Systematics and Ecology, Siberian Zoological Museum",
"SZPT	s	Shenzhen Polytechnic",
"SZPT:ENT	s	Shenzhen Polytechnic, Entomology Collection",
"SZPT:ENT	s	Shenzhen Polytechnic, Entomology Collection",
"SZU	s	University of Salzburg, Department of Organismic Biology",
"T	s	Tavera, Department of Geology and Geophysics",
"TA	s	Timescale Adventures Research and Interpretive Center",
"TAA	s	Estonian Agricultural University, Institute of Agricultural and Environmental Sciences",
"TAC	s	Tarleton State University, Biological Sciences Department",
"TAD	s	Botanical Institute of the Tajikistan Academy of Sciences, Department of Flora and Systematics of Higher Plants",
"TAES	s	Texas A&M University, Department of Rangeland Ecology and Management",
"TAFIRI	s	Tanzania Fisheries Research Institute",
"TAI	s	National Taiwan University, Institute of Ecology and Evolutionary Biology",
"TAIC	s	Texas A&M University-Kingsville, Department of Biology",
"TAIF	s	Taiwan Forestry Research Institute",
"TAIM	s	Taiwan Museum",
"TAIU	s	Texas A&M University - Kingsville, Texas A&I Collections",
"TAK	s	Lenin State University",
"TALE	s	Laboratoire Geologique",
"TALL	s	Tallinn Botanic Garden, Department of Environmental Education",
"TAM	s	Estonian Museum of Natural History, Botany Department",
"TAMA	c	Mycology & Metabolic Diversity Research Center, Tamagawa University Research Institute",
"TAMU	s	Texas A&M University, Biology Department",
"TAN	s	Parc de Tsimbazaza, Departement Botanique",
"TANE	s	Tanta University, Botany Department",
"TAR	s	Consiglio Nazionale delle Ricerche",
"TARI<CHN>	s	Taiwan Agricultural Research Institute",
"TARI<IRN>	s	Research Institute of Forests and Rangelands, Botanical Department",
"TASH	s	National Academy of Science, Uzbekistan",
"TASM	s	Uzbek Academy of Sciences, Laboratory of Mycology",
"TAU<GRC>	s	Aristotle University of Thessaloniki, Biology Department",
"TAU<ISR>	s	Tel-Aviv University",
"TAUF	s	Aristotle University of Thessaloniki, Department of Forestry and Natural Environment",
"TB	s	Tbilisi State University, Botany Department",
"TBGT	s	Tropical Botanic Garden and Research Institute",
"TBI	s	Georgian Academy of Sciences",
"TBIP	s	Research Institute of Plant Protection",
"TBY	s	Tenby Museum",
"TCB	s	National Chung Hsing University, Botany Department",
"TCC/USP	c	Trypanosomatid Culture Collection, University of Sao Paulo",
"TCD	s	Trinity College",
"TCDL	s	Trinity College Library, Manuscript Department",
"TCDU<IRL>	b	Trinity College, Dublin University, Department of Zoology DNA repository",
"TCDU<UGA>	s	Ministry of Animal Industry and Fisheries",
"TCF	s	National Chung Hsing University, Forestry Department",
"TCMM	c	Thai Collection of Medical Microorganism, Department of Pathology, Faculty of Veterinary Science",
"TCNM	s	Timpanogos Cave National Monument",
"TCSW	s	Texas Women's University, Biology Department",
"TCWC	s	Texas Cooperative Wildlife Collection",
"TDA	s	Department of Agriculture",
"TDAH	s	Tasmanian Department of Agriculture",
"TDMP	s	Ta-Dzong Museum",
"TDN	s	Todmorden Botanical Society",
"TDY	s	Tyldesley Natural History Society",
"TEA	s	Tea Research Institute",
"TEB	s	Teberda State Reserve",
"TECLA	s	Centro Nacional de Tecnologia Agropecuaria",
"TEF	s	Centre National de la Recherche Appliquee au Developement Rural, Departement des Recherches Forestieres et Piscicoles",
"TEFH	s	Universidad Nacional Autonoma de Honduras, Departamento de Biologia",
"TEH	s	University of Tehran",
"TELA	s	Tel Aviv University, Botany Department",
"TELY	s	Tate Library",
"TENHS	s	Toynbee Natural History Society",
"TENN	s	University of Tennessee, Botany Department",
"TEPB	s	Universidade Federal do Piaui, Departamento de Biologia",
"TER	s	Indiana State University, Life Science Department",
"TESC	s	The Evergreen State College",
"TEU	s	Teikyo University, Education Department",
"TEX	s	University of Texas at Austin, Plant Resources Center",
"TEXA	s	Blackland Experiment Station",
"TF<JPN>	s	Forestry and Forest Products Research Institute",
"TF<THA>	s	Department of Mineral Resources",
"TFA	s	Forestry and Forest Products Research Institute",
"TFAV	s	Servicio Autonomo para el Desarrollo Ambiental del Estado Amazonas",
"TFC	s	Universidad de La Laguna, Departamento de Biologia Vegetal (Botanica)",
"TFC<EST>	c	Tartu Fungal Culture Collection",
"TFD	s	Tanzania Forestry Research Institute",
"TFDA	s	Tasmanian Fisheries Development Authority",
"TFIC	s	Tasmanian Forest Insect Collection",
"TFM	s	Forestry and Forest Products Research Institute",
"TFMC	s	Museo de Ciencias Naturales de Santa Cruz de Tenerife",
"TFRI	s	Taiwan Fisheries Research Institute",
"TGM	s	Janashia State Museum of Georgia",
"TGPI	s	Tiraspolskij Gosudarstvennij Pedagogiceskij Institut",
"TGRC	b	C.M. Rick Tomato Genetics Resource Center",
"TH	s	University of Tokyo",
"THBC	s	Technische Hochschule",
"THIB	s	Nicholls State University, Department of Biological Sciences",
"THIM	s	National Biodiversity Centre",
"THL	s	Grierson Museum",
"THO	s	Robert Dick Museum Library",
"THRI	s	Sequoia and Kings Canyon National Parks",
"THS	s	Tsumura Laboratory",
"THUP	s	Tunghai University",
"TI	s	Herbarium of the Department of Botany, University of Tokyo",
"TIC	s	California Department of Fish and Game",
"TIE	s	Tianjin Natural History Museum, Botany Department",
"TIK	s	Agricultural Research Centre, Plant Pathology Department",
"TIMGP	s	Institut und Museum fuer Geologie und Palaeontologie der Unversitaet",
"TIMJ	s	Tainai Insect Museum",
"TIMM	c	Institute of Medical Mycology",
"TIPR	s	Institute of Pharmaceutical Research",
"TISTR	c	TISTR Culture Collection Bangkok MIRCEN",
"TIU	s	Tokyo Imperial University, Science College Museum",
"TJDC	s	Tianjin Municipal Institute for Drug Control, Department of Traditional Chinese Medicine",
"TJMP	s	Tianjin Institute of Medical and Pharmaceutical Sciences",
"TK	s	Tomsk State University",
"TKB	s	University of Tsukuba",
"TKNM	s	Twickenham Girls' School",
"TKPM	s	Tokushima Prefectural Museum",
"TKU	s	Tokyo Kyoiku University",
"TL	s	Universite Paul Sabatier",
"TLA	s	Ecole Nationale Superieure Agronomique",
"TLF	s	Universite Paul Sabatier",
"TLHR	s	Thueringer Landesmuseum Heidecksburg",
"TLJ	s	Universite Paul-Sabatier",
"TLM	s	Museum d'Histoire Naturelle de Toulouse",
"TLMF	s	Tiroler Landesmuseum Ferdinandeum",
"TLON	s	Museum d'Histoire Naturelle",
"TLP	s	Faculte de Medecine, Chaire de Botanique",
"TLS	s	Tunbridge Wells Museum and Art Gallery",
"TLXM	s	Universidad Autonoma de Tlaxcala",
"TM<DEU>	s	Teylers Museum, Paleontologische",
"TM<SVK>	s	Slovak National Museum",
"TM<ZAF>	s	Transvaal Museum",
"TMAG	s	Tasmanian Museum & Art Gallery",
"TMAL	s	Tameside Metropolitan Borough Museum",
"TMBS	s	Tatsuo Tanaka Memorial Biological Laboratory",
"TMC<AUS>	s	Tate Museum Collection",
"TMC<USA-CO>	c	Trudeau Mycobacterial Culture Collection, Trudeau Institute",
"TMC<USA-ID>	c	The Mollicutes Collection",
"TMDU	s	Tokyo Medical and Dental University",
"TMFE	s	Elasmobranchii Collection of the Department of Fisheries, Tokai University",
"TMH	s	Tasmanian Museum and Art Gallery",
"TMHN	s	Teyler Museum",
"TMI	s	Tottori Mycological Institute",
"TMM	s	Texas Memorial Museum",
"TMMC	s	Texas Memorial Museum",
"TMNH	s	Tianjin Museum of Natural History",
"TMP<FIN>	s	Tampere Museums",
"TMP<ZAF>	s	Transvaal Museum",
"TMS	s	Toleco Museum of Health and Natural History",
"TMSA	s	Transvaal Museum",
"TMTC	s	Taiwan Provincial Museum",
"TNAU	s	Tamil Nadu Agricultural University",
"TNFC	s	Tynside Naturalists' Field Club",
"TNFS	s	USDA Forest Service, Alaska Region",
"TNHC	s	Texas Memorial Museum, Texas Natural History Collection",
"TNHM	s	University of Texas",
"TNM	s	National Museum of Natural Science, Botany Department",
"TNP	s	Museum of Tatra National Park",
"TNS	s	National Science Museum, Department of Botany",
"TNSC<BEL>	s	Thierry Neef de Sainval",
"TNSC<USA-NJ>	s	Trailside Nature and Science Center",
"TNU	s	National Taiwan Normal University, Biology Department",
"TNZ	s	Tianjin Nat. Hist. Mus.",
"TO	s	Universita degli Studi di Torino, Dipartimento di Biologia Vegetale",
"TOD	s	Todmorden Free Library",
"TOFO	s	University of Tokyo, Section of Forest Botany",
"TOGO	s	Universite du Lome, Laboratoire de Botanique et Ecologie Vegetale",
"TOGR	s	Museo di Storia Naturale Don Bosco",
"TOHO	s	Toho University",
"TOKE	s	Tokyo University of Education",
"TOLI	s	Universidad del Tolima, Departamento de Biologia",
"TOM	s	Istituto Missioni Consolata",
"TONG	s	Tonghua Teachers College, Biology Department",
"TOR	s	Torquay Museum",
"TOYA	s	Toyama Science Museum, Botany Department",
"TPII	s	Thanksgiving Point Institute",
"TPNG	s	Department of Primary Industry",
"TPV	s	Prairie View A & M University, Biology Department",
"TR	s	Museo Tridentino di Scienze Naturali",
"TRA	s	American Plant Life Society",
"TRD	s	Ancient House Museum",
"TRE	s	Trencianske muzeum, Scientific Department",
"TRES	s	Tresco Abbey",
"TRH	s	Norwegian University of Science and Technology, Department of Natural History",
"TRIN	s	The National Herbarium of Trinidad and Tobago",
"TRM	s	Vlastivedne muzeum Trutnov",
"TRN	s	N. Copernicus University",
"TRO	s	Royal Horticultural Society of Cornwall",
"TROM	s	University of Tromsoe, Botanical Department",
"TROY	s	Troy State University, Department of Biological and Environmental Sciences",
"TRT	s	Royal Ontario Museum, Department of Natural History",
"TRTC	s	Royal Ontario Museum, Center for Biodiversity and Conservation Biology",
"TRTE	s	Erindale College, University of Toronto, Department of Biology",
"TRTS	s	Scarborough College, University of Toronto, Botany Department",
"TRU	s	Royal Cornwall Museum",
"TRV	s	Transvaal Museum",
"TS	s	National University of Shandong, Biology Department",
"TSB	s	Universita degli Studi di Trieste, Dipartimento di Biologia",
"TSC	s	Tarleton State University, Tarleton State Collection",
"TsGM	s	Central Geological Museum",
"TSM	s	Museo Civico di Storia Naturale",
"TSMHN	s	Teylers Strichtina Museum",
"TsNIGRI	s	Tsentralny Nauchno-Issledovatelskii Geolgo-Razvedochni Muzei (Chernyshev's Central Museum of Geological Exploration)",
"TSSMC	s	Teton Science School",
"TSTN	s	Troston Hall",
"TSU	s	Miye University",
"TSY	c	Laboratory of Mycology, Division of Microbiology",
"TTC	s	Texas Tech University, Biological Sciences Department",
"TTCC	s	Texas Tech University",
"TTMB	s	Termeszettudomanyi Muzeum",
"TTN	s	Somerset County Museum",
"TTRS	s	Tall Timbers Research Station, Fire Ecology Laboratory",
"TTU	s	Texas Tech University, Museum",
"TTY	s	Westonbirt School",
"TU<DEU>	s	Institut fur Geologie und Palaontologie",
"TU<EST>	s	University of Tartu",
"TU<USA-LA>	s	Tulane University, Museum of Natural History",
"TUAT	s	Tokyo University of Agriculture",
"TUB	s	Eberhard-Karls-Universitaet Tuebingen, Institut fuer Biologie I",
"TUC	s	University of Arizona, Ecology and Evolutionary Biology Department",
"TUCH	s	Tribhuvan University, Central Department of Botany",
"TUFIL	s	Tokyo University of Fisheries, Ichthyological Laboratory",
"TUFT	s	Tufts University, Biology Department",
"TUH	s	Tehran University, Department of Biology",
"TULE	s	Tokyo University of Agriculture & Technology",
"TULS	s	University of Tulsa",
"TULV	s	Jardin Botanico Juan Maria Cespedes",
"TUMH	s	Tottori Fungus/Mushroom Resource and Research Center",
"TUN	s	Universite de Tunis, Laboratoire de Biologie Vegetale",
"TUNG	s	Tunghai University, Biology Department",
"TUP	s	Trent University, Biology Department",
"TUPH	s	Institute of Public Health Research",
"TUR	s	University of Turku",
"TURA	s	Aabo Akademi University, Biology Department",
"TURP	s	Turpan Eremophytes Botanical Garden",
"TUS	s	Tohoku University, Biological Institute",
"TUSG	s	Tohoku University",
"TUT	s	Daejeon University, Department of Biology",
"TUTC	s	Tunghai University",
"TV	s	Centro de Estratigrafia e Paleobiologia da Universidade Nova de Lisboa",
"TVBG	s	Tver State University",
"TVY	s	Turvey Abbey",
"TWC	s	Texas Wesleyan College, Museum of Zoology",
"TWRA	s	Tennessee Wildlife Resources Agency",
"TYF	s	Shangxi Forestry Institute",
"TZM	s	National Science Museum",
"U	s	Nationaal Herbarium Nederland, Utrecht University branch",
"UA<GRC>	s	Department of Historical Geology and Paleontology",
"UA<USA-AZ>	s	University of Arizona",
"UAAAC	s	University of Alaska Anchorage Avian Collection",
"UAAH	s	University of Alaska Anchorage, Department of Biological Sciences",
"UAAM	s	The Arthropod Museum, University of Arkansas",
"UAB<COL>	s	Universidad de Los Andes",
"UAB<ESP>	s	Universidad Autonoma de Barcelona",
"UABC	s	Universidad Autonoma de Baja California",
"UABCS	s	Universidad Nacional Autonoma de Baja California Sur (Mexico)",
"UABD	s	University of Alabama",
"UAC	s	University of Calgary, Department of Biological Sciences",
"UACC	s	Univeridad Autonoma de Chapingo",
"UADBA	s	University dAntananarivo, Department de Biologie Animale",
"UADY	s	Universidad Autonoma de Yucatan, Departamento de Botanica",
"UAEM	s	Univeridad Autonoma de Morelos",
"UAGC	s	Universidad Autonoma de Guerrero, Area de Ciencias Naturales",
"UAIC<CIV>	s	University of Abidjan",
"UAIC<USA-AL>	s	University of Alabama, Ichthyological Collection",
"UAIC<USA-AZ>	s	University of Arizona",
"UALRVC	s	University of Arkansas at Little Rock, Vertebrate Collection",
"UAM	s	University of Alaska, Museum of the North",
"UAM:Bird	s	University of Alaska, Museum of the North, Bird Collection",
"UAM:Bryo	s	University of Alaska, Museum of the North, Bryozoan Collection",
"UAM:Crus	s	University of Alaska, Museum of the North, Marine Arthropod Collection",
"UAM:Ento	s	University of Alaska, Museum of the North, Insect Collection",
"UAM:ES	s	University of Alaska, Museum of the North, Earth Science",
"UAM:Fish	s	University of Alaska, Museum of the North, Fish Collection",
"UAM:Herb	s	University of Alaska, Museum of the North, UAM Herbarium",
"UAM:Herp	s	University of Alaska, Museum of the North, Amphibian and Reptile Collection",
"UAM:Mamm	c	University of Alaska, Museum of the North, Mammal Collection",
"UAM:Moll	s	University of Alaska, Museum of the North, Mollusc Collection",
"UAM:Paleo	s	University of Alaska, Museum of the North, paleontology collection",
"UAM<USA-AR>	s	University of Arkansas at Monticello",
"UAM<VEN>	s	Universidad de los Andes, Facultad de Ciencias",
"UAMH	c	University of Alberta Microfungus Collection and Herbarium",
"UAMI	s	Universidad Autonoma Metropolitana, Unidad Iztapalapa (Mexico)",
"UAMIZ	s	Universidad Autonoma Metropolitana, Iztapalapa, Departamento de Biologia",
"UAMM	s	Universidad de los Andes",
"UAMZ	s	University of Alberta Museum of Zoology",
"UAMZC	s	University of Arkansas, Museum Zoological Collections",
"UANL	s	Universidad Autonoma de Nuevo Leon",
"UARK	s	University of Arkansas",
"UAS	s	Universidad Autonoma de Sinaloa",
"UASB	s	University of Agricultural Sciences",
"UASC	s	Museo de Historia Natural \"Noel Kempff Mercado\"",
"UASK	s	Ukrainian Academy of Science",
"UASM	s	University of Alberta, E.H. Strickland Entomological Museum",
"UAT	s	Universidad Autonoma de Tamaulipas",
"UAVP	s	University of Alberta, Laboratory for Vertebrate Paleontology",
"UAY	s	Universidad Autonoma de Yucatan, Facultad de Medicina Veterinaria y Zootecnia",
"UAZ	s	University of Arizona",
"UB<BRA>	s	Universidade de Brasilia, Departamento de Botanica",
"UB<FRA>	s	Laboratoire de Biostratigraphie",
"UBA	s	Mongolian Academy of Sciences",
"UBC	s	University of British Columbia",
"UBC<botany>	s	University of British Columbia, Botany Department",
"UBCC	c	University of Barcelona Culture Collection",
"UBCZ	s	University of British Columbia, Spencer Museum",
"UBJTL	s	Universidad Bogota Jorge Tadeo Lozano",
"UBL	s	Universite du Benin",
"UBT	s	Oekologisch-Botanischer Garten",
"UBU	s	Mongolian State University, Botany Department",
"UC<USA-CA>	s	University of California",
"UC<USA-MI>	c	Upjohn Culture Collection",
"UCAC	s	University of Central Arkansas, Department of Biology",
"UCAM	s	Universidad Autonoma de Campeche",
"UCB	s	University of California",
"UCBG	s	University of Botswana, Department of Biological Sciences",
"UCBL	s	Centre de Paleontologie Stratigraphique et Paleoecologie",
"UCC	s	University of Cincinnati",
"UCC<IRL>	s	University College Cork",
"UCCC	s	Universidad de Concepcion, Museo de Zoologia",
"UCCM	c	University of Calabar Collection of Microorganisms",
"UCD	s	University of California, Davis",
"UCDBA	s	University of Chicago",
"UCFC	s	University of Central Florida",
"UCGC	s	University of Colorado, Geological Museum",
"UCGE	c	Unit Cell of Genetic Engineering, Department of Biochemistry",
"UCHT	s	University of Tennessee, Chattanooga, Department of Biological and Environmental Sciences",
"UCI	s	University of Ibadan, Botany and Microbiology Department",
"UCJ	s	Universite d'Abidjan, Departemente de Botanique",
"UCL<BEL>	c	Catholic University of Louvain",
"UCL<GBR>	s	University College London",
"UCLA	s	University of California at Los Angeles",
"UCLAF	c	HMR/Romainville",
"UCLGMZ	s	Grant Museum of Zoology and Comparative Anatomy",
"UCLZ	s	University College London",
"UCM<ESP>	s	Universidad Complutense Madrid",
"UCM<UKR>	c	Ukrainian Collection of Microorganisms, Zabolotny Institute of Microbiology and Virology",
"UCM<USA-CO>	s	University of Colorado Museum",
"UCMC	s	University of Colorado Museum",
"UCME	s	Faculdad de Biologia, Departamento de Zoologia",
"UCMM	s	Pontificia Universidad Catolica Madre y Maestra",
"UCMP	s	University of California Museum of Paleontology",
"UCMS	s	Storrs, University of Connecticut",
"UCNW	s	University of Wales",
"UCNZ	s	University of Canterbury",
"UCOB	s	Universidad Centroccidental Lisandro Alvarado, Departamento de Ciencias Biologicas",
"UCOCV	s	University of Central Oklahoma, Collection of Vertebrates",
"UCONN	s	University of Connecticut",
"UCP	s	Universidad del Cauca",
"UCPC	s	Universidad del Cauca",
"UCR<CRI>	s	Universidad de Costa Rica, Museo de Zoologia",
"UCR<USA-CA>	s	University of California, Riverside",
"UCS<USA-CT>	s	University of Connecticut",
"UCS<USA-NY>	s	Union College, Department of Biological Sciences",
"UCSA	s	University College of Swansea, Botany Department",
"UCSB	s	University of California, Santa Barbara",
"UCSC	s	University of California, Department of Environmental Studies",
"UCSW	s	University College, Botany Department",
"UCVC	s	Universidad Catolica de Valparaiso",
"UCWI	s	University of the West Indies, Department of Life Sciences",
"UDBC	s	Universidad Distrital",
"UDCC	s	University of Delaware",
"UDEL	s	University of Delaware",
"UDM	s	Museo Friulano di Storia Naturale",
"UDO	s	Universidad de Oriente",
"UDONECI	s	Universidad de Oriente",
"UDSM	s	University of Dar es Salaam",
"UDU	s	Udmurt State University, Department of Biology and Chemistry",
"UDW	s	University of Durban-Westville, Botany Department",
"UEA	s	University of East Anglia",
"UEC	s	Universidade Estadual de Campinas, Departamento de Botanica",
"UEFS	s	Laboratorio de Ictiologia",
"UESS	s	Universidad de El Salvador",
"UEVH	s	Universidade de Evora, Departamento de Biologia",
"UF	s	Florida Museum of Natural History",
"UF/FGS	s	Florida Geological Survey",
"UF:Herpetology	s	Florida Museum of Natural History, Herpetology Collection",
"UF:Ichthyology	s	Florida Museum of Natural History, Fish Collection",
"UF:Invertebrate	s	Florida Museum of Natural History, Invertebrate Zoology and Malacology Collection",
"UF:Mammalogy	s	Florida Museum of Natural History, Mammalogy Collection",
"UF:Ornithology	s	Florida Museum of Natural History, Ornithology Skins and Skeletons Collection",
"UFA	s	Ufa Scientific Centre, Russian Academy of Sciences",
"UFC	s	Universidade Federal do Ceara, Departamento de Biologia",
"UFES	s	Departamento de Biologia, Colecao Entomologica",
"UFG	s	Universidade Federal de Goias, Unidade de Conservacao",
"UFH	s	University of Fort Hare, Botany Department",
"UFHNH	s	Utah Field House of Natural History State Park",
"UFJF	s	Universidade Federal de Juiz de Fora",
"UFMA	s	Universidade Federal do Maranhao, Curso de Farmacia",
"UFMI	s	Universidade Federal de Mato Grosso, Instituto de Biociencias",
"UFMT	s	Universidade Federal de Mato Grosso",
"UFNH	s	Utah Field House Natural History [address unknown]",
"UFP	s	Universidade Federal de Pernambuco, Departamento de Botanica",
"UFPB	s	Departamento de Sistematica e Ecologia",
"UFPEDA	c	Universidade Federal de Pernambuco",
"UFRG	s	Instituto de Biologia",
"UFRGS	s	Universidade Federale do Rio Grande do Sul",
"UFRJ	s	Universidade Federal Rural do Rio de Janeiro, Area de Fitopatologia, Departamento de Entomologia e Fitopatologia",
"UFRJIM	c	Departamento de Microbiologia Medica",
"UFS	s	Nyabyeya Forestry College, Department of Environmental Forestry",
"UFScarCC	c	Freshater Microalgae Collection Cultures",
"UFVB	s	Vicosa, Universidade Federal de Vicosa, Museum of Entomology",
"UG<ESP>	s	Museo del Departamento de Estratigrafia y Paleontologia",
"UG<GHA>	s	University of Ghana",
"UGAMNH	s	University of Georgia Museum of Natural History",
"UGCA	s	University of Georgia",
"UGDA	s	Gdansk University, Department of Plant Taxonomy and Nature Conservation",
"UGDZ	s	University of Guelph",
"UGG	s	University of Guam",
"UGGE	s	Universidad de Guayaquil",
"UGGG	s	University of Guyana",
"UGM	s	University of Guam",
"UH	s	University of Hawaii",
"UHI	s	Ussishkin House, Botany Department",
"UHM	s	Manoa, College of Tropical Agriculture, Department of Entomology",
"UI<NGA>	s	University of Ibadan",
"UI<USA-UT>	s	Bureau of Land Management",
"UICC	c	University of Indonesia Culture Collection",
"UIDA	s	University of Idaho, Bird and Mammal Museum",
"UIM	s	University of Idaho",
"UIMNH	s	University of Illinois, Museum of Natural History",
"UIS	s	Universidad Industrial de Santander, Departamento de Biologia",
"UJAT	s	Universidad Juarez Autonoma de Tabasco",
"UJB	c	University of Jaffna Botany",
"UJIM	s	University of Jordan Insect Museum",
"UK<USA-KY>	s	University of Kentucky",
"UKEN	s	University of Kentucky",
"UKKP	c	Universiti Kebangsaan Kultur Perubatan",
"UKKY	s	University of Kunming",
"UKMB	s	Universiti Kebangsaan Malaysia, Botany Department",
"UKMS<MYS>	s	Universiti Kebangsaan Malaysia, Kampus Sabah",
"UKMS<SDN>	s	Sudan Natural History Museum",
"UKS	s	University of Khartoum",
"UKSPI	s	Ust-Kamenogorsk State Pedagogical Institute, Botany Department",
"UL	s	University of Louisville",
"ULABG	s	Universidad de los Andes, Laboratorio de Biogeografia",
"ULCI	s	Universidad de la Laguna",
"ULF	s	Universite Laval, Departement des Sciences forestieres",
"ULKY	s	University of Louisville",
"ULLZ	s	University of Louisiana at Layafette, Zoological Collection",
"ULM	s	Universitaet Ulm, Abteilung Systematische Botanik und Oekologie",
"ULMG	s	University of Leipzig",
"ULN	s	University of Lagos",
"ULQC	s	University of Laval",
"ULS	s	Universidad de La Serena, Departamento de Biologia",
"ULT	s	Al-Faateh University, Botany Department",
"ULV	s	Universidad Central de Las Villas",
"UM<DEU>	s	University of Marburg",
"UM<USA-TN>	s	University of Memphis, Mammal Collection",
"UM<ZWE>	s	Umtali Museum",
"UMA	s	University of Massachusetts, Museum of Zoology",
"UMAN	s	University of Manitoba, Zoological Collection",
"UMB	s	Uebersee-Museums",
"UMBB	s	Uebersee-Museum, Bremen or Department of Zoology, University of Bremen",
"UMBC	s	Univeristy of Malawi",
"UMBS	s	University of Michigan",
"UMD	s	University of Minnesota, Duluth",
"UMDC	s	University of Maryland",
"UMDE	s	University of Maine",
"UME	s	Umeaa University",
"UMEC	s	University of Massachusetts",
"UMED	s	University of Moi",
"UMF<USA-FL>	s	University of Miami",
"UMF<USA-ME>	s	University of Maine, Farmington",
"UMF<USA-MI>	s	University of Michigan, Biology Department",
"UMFFTD	c	Food and Fermentation Technology Division, University of Mumbai",
"UMFK	s	University of Maine at Fort Kent, Biology Department",
"UMH	s	Universidad Miguel Hernandez, Departamento de Biologia Aplicada",
"UMHB	s	University of Mary Hardin-Baylor",
"UMIC	s	University of Mississippi",
"UMIM	s	Univeristy of Miami Ichthyological Museum",
"UMIP	c	Collection de Champignons et Actinomycetes Pathogenes",
"UMKC	s	University of Missouri",
"UMKL	s	University of Malaysia",
"UMKU	s	Uganda Museum",
"UMML	s	University of Miami Marine Laboratory",
"UMMP	s	University of Michigan",
"UMMZ	s	University of Michigan, Museum of Zoology",
"UMNH	s	Utah Museum of Natural History",
"UMO<GBR>	s	University Museum of Natural History",
"UMO<USA-ME>	s	University of Maine",
"UMO<USA-MO>	s	University of Missouri, Museum Support Center",
"UMOC	s	University of Missouri, Museum of Zoology",
"UMRC	c	University of Minnesota Rhizobium Collection",
"UMRM	s	W.R. Enns Entomology Museum",
"UMS	s	Universiti Malaysia Sabah",
"UMSA	s	Instituto de Ecologia",
"UMSP	s	University of Minnesota",
"UMT	s	Mutare Museum",
"UMUT	s	University Museum, University of Tokyo",
"UMUTZ	s	Department of Zoology, University Museum",
"UMZ	s	Univesity Museum of Zoology, Cambridge University",
"UMZC	s	University Museum of Zoology Cambridge",
"UMZM	s	University of Montana, Zoological Museum",
"UN	s	University of Nebraska",
"UNA	s	University of Alabama, Department of Biological Sciences",
"UNAB	s	Universidad Nacional, Facultad de Agronomia",
"UNAC	s	Universidad Nacional Agraria",
"UNAD	s	Universidad Nacional Agraria",
"UNAF	s	University of North Alabama, Department of Biology",
"UNAM	s	Universidad Nacional Autonoma de Mexico",
"UNAN	s	Universidad Nacional Autonoma de Nicaragua",
"UNB	s	University of New Brunswick, Biology Department",
"UNC-B	s	University of Northern Colorado",
"UNCB	s	Universidad Nacional de Colombia, Insituto de Ciencias Naturales de la Universidad Nacional",
"UNCC<COL>	s	Universidad Nacional de Caldas, Museo de Historia Natural",
"UNCC<USA-NC>	s	University of North Carolina, Biology Department",
"UNCM	s	Museo de Entomologia \"Francisco Luis Gallego\"",
"UNCP	s	Universidad Nacional de Colombia",
"UNCW	s	University of North Carolina at Wilmington",
"UND	s	University of North Dakota, Vertebrate Museum",
"UNDH	s	University of Natal Durban",
"UNEFM	s	Universidad Experimental Francisco de Miranda",
"UNEVR	s	University of Nevada, Museum of Biology",
"UNEX	s	Universidad de Extremadura, Departamento de Botanica",
"UNH	s	University of New Hampshire",
"UNI	s	University of Northern Iowa",
"UNIN	s	University of the North, Botany Department",
"UNIP	s	Universidade Paulista, Laboratorio de Botanica",
"UNIQEM	c	Institute of Microbiology, Russian Academy of Sciences",
"UNL<MEX>	s	Universidad Autonoma de Nuevo Leon",
"UNL<PRT>	s	Centro de Estratigrafia e Paleobiologia da Universidade Nova de Lisboa",
"UNL<USA-NE>	s	University of Nebraska State Museum",
"UNLO	s	Universidad Nacional Experimental de los Llanos Occidental",
"UNLV	s	University of Nevada, Las Vegas, Department of Biological Sciences",
"UNM	s	University of New Mexico, Department of Biology",
"UNMC	s	University of New Mexico",
"UNN	s	University of Nigeria, Botany Department",
"UNNF	s	Universite de Nancy",
"UNO	s	University of Nebraska at Omaha",
"UNOAL	s	University of Northern Alabama",
"UNOVC	s	University of New Orleans",
"UNPSJB-Pv	s	Universidad Nacional de la Patagonia",
"UNR<ARG>	s	Universidad Nacional de Rosario, Botanica y Ecologia Vegetal",
"UNR<USA-NV>	s	University of Nevada, Museum of Biology",
"UNSA	s	University of Natal",
"UNSL	s	Universidad Nacional de San Luis",
"UNSM	s	University of Nebraska State Museum",
"UNSW	c	Microbiology Culture Collection, University of New South Wales",
"UNT	s	Universidad nacional de Tucumn",
"UNWH	s	University of North-West, Biological Sciences Department",
"UO	s	University of Oklahoma",
"UOG	s	University of Guelph",
"UOG:BIO	b	University of Guelph, Biodiversity Institute of Ontario",
"UOG:DEBU	s	University of Guelph, Ontario Insect Collection",
"UOIC	s	University of Oregon",
"UOJ	s	Universidad del Oriente, Departamento de Agronomia",
"UOM	s	University of Missouri",
"UOMNH	s	University of Oregon, Museum of Natural History",
"UOMZ	s	University of Oklahoma, Stovall Museum of Zoology",
"UOP	s	University of Opole",
"UOPJ	s	Osaka Prefecture University",
"UOS	s	University of the South, Biology Department",
"UP	s	University of Papua and New Guinea",
"UPA	s	University of Patras, Department of Plant Biology",
"UPCB	s	Universidade Federal do Parana, Departamento de Botanica",
"UPCC	c	Natural Sciences Research Institute Culture Collection",
"UPEI	s	University of Prince Edward Island, Biology Department",
"UPF	s	Universite de Polynesie Francaise, Laboratoire Terre-Ocean",
"UPIE	b	Unidad de Patologia Infecciosa y Epidemiologia",
"UPLB	s	Museum of Natural History",
"UPM<FRA>	s	Departement des Siences de la Terre",
"UPM<MYS>	s	Universiti Pertanian Malaysia, Biology Department",
"UPM<RUS>	s	Udory Paleontological Museum",
"UPMR	c	Rhizobium Collection",
"UPMSI	s	Marine Science Institute",
"UPNA	s	Universidad Publica de Navarra, Departamento de Ciencias del Medio Natural",
"UPNG	s	University of Papua New Guinea, Division of Biological Sciences",
"UPOS	s	Universidad Pablo de Olavide, Ciencias Ambientales (Botanica)",
"UPP	s	Uppingham School Museum",
"UPPC	s	University of the Philippines",
"UPR	s	Puerto Rico Botanic Garden, University of Puerto Rico",
"UPRG	s	Universidad Nacional \"Pedro Ruiz Gallo\"",
"UPRM	c	University of Puerto Rico at Mayagueez, Rhizobium Culture Collection",
"UPRP	s	University of Puerto Rico at Rio Piedras",
"UPRRP	s	University of Puerto Rico, Biology Department",
"UPS	s	Uppsala University, Museum of Evolution, Botany Section (Fytoteket)",
"UPSA	s	University of Pretoria",
"UPSC	c	Fungal Culture Collection at the Botanical Museum",
"UPSU	s	Ulyanovsk State Pedagogical University, Department of Botany",
"UPSV	s	Uppsala University, Department of Plant Ecology",
"UPTC	s	Universidad Pedogogica y Tecnologica de Colombia, Escuela de Ciencias Biologicas",
"UPVB	s	Departamento de Geologia, Universidad del Pais Vasco",
"UPVLP	s	Laboratorio de Paleontolgia of the Universdad del Pais Vasco/Euskal Herriko Unibersitatea",
"UQAM	s	Universite du Quebec a Montreal, Departement des Sciences biologiques",
"UQAR	s	Universite du Quebec a Rimouski, Departement de biologie",
"UQIC	s	University of Queensland Insect Collection",
"UQTR	s	Universite du Quebec a Trois-Rivieres, Departement de chimie-biologie",
"URB	s	Ryukyu University Department of Zoology",
"URIC	s	University of Rhode Island",
"URIMC	s	University of Rhode Island, Mammal Collection",
"URM<BRA>	c	Universidade Federal de Pernambuco",
"URM<JPN>	s	University of the Ryukyus",
"URMU	s	Museo Nacional de Historia Natural",
"URO	s	University of the Ryukyus",
"URP	s	Museo de Historia Natural, Universidad Ricardo Palma",
"URT	s	Universita degli Studi di Roma Tre, Dipartimento di Biologia",
"URV	s	University of Richmond, Biology Department",
"US	s	Smithsonian Institution, Department of Botany",
"USA	s	University of South Alabama",
"USAC	s	University of Western Australia",
"USAM	s	University of South Alabama, Department of Biological Sciences",
"USANHC	s	University of South Alabama, Vertebrate Natural History Collection",
"USAS	s	University of Regina, Biology Department",
"USBCF	s	U. S. Bureau of Commercial Fisheries",
"USBS	s	School of Biological Sciences, University of Science",
"USC	s	University of Southern California, Biological Sciences Department",
"USCC	s	University of Southern Colorado",
"USCG	s	Universidad de San Carlos de Guatemala",
"USCH	s	University of South Carolina, Department of Biological Sciences",
"USCP	s	University of San Carlos",
"USCS	s	University of South Carolina, Science and Mathematics Department",
"USCWH	s	United Services College",
"USD<DOM>	s	Universidad Autonoma de Santo Domingo",
"USD<USA-SD>	s	University of South Dakota",
"USDA	s	United States Department of Agriculture",
"USDA:GRIN	b	United States Department of Agriculture, Germplasm Resources Information Network",
"USDAK	s	W. H. Over State Museum",
"USF	s	University of South Florida, Biology Department",
"USF:CBD	c	University of South Florida, Biology Department, Center for Biological Defense",
"USFC	s	U. S. Fish Commission",
"USFS	s	Rocky Mountain Forest and Range Experiment Station",
"USH	s	Ushaw College",
"USI	s	University of Southern Indiana",
"USJ	s	Universidad de Costa Rica",
"USLH	s	University of Louisiana Lafayette, Department of Renewable Resources",
"USM<MYS>	s	Universiti Sains Malaysia",
"USM<MYS>:VCRU	s	Universiti Sains Malaysia, Vector Control Research Unit",
"USM<PER>	s	Universidad Nacional Mayor de San Marcos, Division de Botanica",
"USMS	s	University of Southern Mississippi, Department of Biological Sciences",
"USNC	s	Smithsonian Institution, Paleobiology Department",
"USNM	s	National Museum of Natural History, Smithsonian Institution",
"USNM:ENT	s	National Museum of Natural History, Smithsonian Institution, Entomology Collection",
"USNM:Fish	s	National Museum of Natural History, Smithsonian Institution, National Fish Collection",
"USNM:LAB	b	National Museum of Natural History, Smithsonian Institution, Laboratories of Analytical Biology",
"USNTC	s	U.S. National Tick Collection",
"USON	s	Universidad de Sonora, Departamento de Investigaciones Cientificas y Technologicas",
"USP<ESP>	s	Universidad San Pablo-CEU, Departamento de Biologia Vegetal (seccion Botanica)",
"USP<FJI>	s	University of the South Pacific",
"USPIY	s	K. D. Ushinsky Yaroslavl State Pedagogical University, Department of Botany",
"USRC	s	University of Regina",
"USRCB	c	Ukrainian Scientific-Research Cell Bank.",
"USSC	s	U.S. Soil Conservation Service",
"USTF	s	Technologische Faculteit",
"USTK	s	University of Science and Technology, Museum of Natural History",
"USU	s	United States Department of Agriculture",
"USZ	s	Universidad Autonoma Gabriel Rene Moreno",
"UT<USA-TN>	s	University of Tennessee",
"UT<USA-UT>	s	University of Utah",
"UTA	s	University of Texas at Arlington",
"UTAI	s	Tel Aviv University",
"UTC	s	Utah State University, Biology Department",
"UTCC	c	University of Toronto Culture Collection of Algae and Cyanobacteria",
"UTD	s	University of Texas, Plant Resources Center",
"UTE	s	University of Tartu",
"UTEP	s	University of Texas at El Paso, Centennial Museum",
"UTEX	c	The Culture Collection of Algae at the University of Texas Austin",
"UTG	s	University of Tuebingen",
"UTGD	s	Geology Department, The University of Tasmania",
"UTKI	s	University of Teheran",
"UTLH	s	University of Technology, Human Sciences Department",
"UTLPA	s	University of Texas at Austin, Laboratory of Physical Anthropology",
"UTMC	s	Universidad del Magdalena",
"UTMZ	s	University of Tennessee, Museum of Zoology",
"UTV	s	Universita degli Studi della Tuscia, Dipartimento di Agrobiologia e Agrochimica",
"UTZM	s	University of Tsukubo",
"UU<SWE>	s	University of Uppsala",
"UU<UKR>	s	Uzhgorod State University, Botany Department",
"UU<USA-UT>	s	University of Utah",
"UUC	c	Janet A. Robertson Collection of Ureaplasma urealyticum Cultures",
"UUH	s	Institute of General and Experimental Biology, Department of Floristics and Geobotany",
"UUVP	s	University of Utah",
"UUZM	s	Uppsala University, Zoological Museum",
"UV<CHL>	s	University of Valparaiso",
"UV<COL>	s	Departamento de Biologia de la Universidad del Valle",
"UVA	s	Geological Institute of the University of Amsterdam",
"UVAL	s	Universidad del Valle de Guatemala",
"UVC	s	Museo de Vertebrados, Universidad del Valle",
"UVCC	s	University of Vermont",
"UVCE	s	Colecao Entomologica, Laboratorio de Entomologia Sistematica",
"UVCO	s	Universidad de Valle",
"UVG	s	Universidad del Valle",
"UVGC	s	Collecion de Artropodos",
"UVIC	s	University of Victoria, Biology Department",
"UVP	s	Utah State Vertebrate Paleontology Collection",
"UVSC	s	Utah Valley State College, Biology Department",
"UVST	s	Southwest Texas Junior College, Biology Department",
"UVV	s	Universita di Venezia, Dipartimento de Scienze Ambientali",
"UW<POL>	s	Instytut Nauk Geologicznych Uniwersitetu Wroclawskiego - Institute of Geological Sciences of the University of Wroclaw (Breslau)",
"UW<USA-WA>	s	University of Washington Fish Collection",
"UW<USA-WY>	s	University of Wyoming",
"UWA	s	University of Western Australia, Botany Department",
"UWBM	s	University of Washington, Burke Museum",
"UWC	s	University of the Western Cape, Botany Department",
"UWCP	s	University of Wroclaw",
"UWEC	s	University of Wisconsin, Department of Biology",
"UWFP	s	University of West Florida, Department of Biology",
"UWGB	s	University of Wisconsin-Green Bay, MAC 212",
"UWI	s	University of the West Indies (Trinidad and Tobago)",
"UWIC	s	University of the West Indies, Trinidad and Tobago",
"UWIJ	s	University of the West Indies, Jamaica",
"UWJ	s	University of Wisconsin, Biology Department",
"UWL	s	University of Wisconsin, Biology Department",
"UWM	s	University of Wisconsin, Biological Sciences Department",
"UWMA	s	University of Wisconsin, Milwaukee, Department of Anthropology",
"UWMIL	s	University of Wisconsin, Milwaukee, Department of Biological Sciences",
"UWO	c	University of Western Ontario",
"UWOC	s	University of Western Ontario",
"UWP	s	University of Wrocklaw",
"UWPG	s	University of Winnipeg, Biology Department",
"UWSP	s	University of Wisconsin-Stevens Point,",
"UWW	s	University of Wisconsin - Whitewater, Biological Sciences Department",
"UWZM	s	University of Wisconsin, Zoological Museum",
"UYIC	s	Instituto de Biologia",
"UZA	s	Unidad de Zoologia Aplicada, Departamento de Ecologia",
"UZIU	s	Uppsala University",
"UZL	s	University of Zambia, Biological Sciences Department",
"UZMC	s	Universidad del Zulia",
"UZMH	s	University Museum (Zoology)",
"UZMO	s	Zoologisk Museum",
"V	s	Royal British Columbia Museum",
"VA	s	University of Virginia",
"VAB	s	Universitat de Valencia, Departamento de Biologia Vegetal",
"VAIC	s	Victorian Agricultural Insect Collection",
"VAL	s	Universitat de Valencia",
"VALA	s	Universidad Politecnica, Departamento de Botanica",
"VALD	s	Universidad Austral de Chile, Instituto de Botanica",
"VALLE	s	Universidad Nacional de Colombia, Departamento de Ciencias Basicas",
"VALPL	s	Universidad de Playa Ancha, Departamento de Biologia y Quimica",
"VAN	s	Societe Polymathique du Morbihan",
"VANF	s	Yuezuencue Yil University, Biology Section",
"VAS	s	Vassar College, Biology Department",
"VBCM	s	Universidad Complutense de Madrid",
"VBGI	s	Botanical Garden-Institute",
"VBI	s	Institute of Ecology and Botany of the Hungarian Academy of Sciences, Botanical Department",
"VCRC	c	Volcani Center Rhizobium Collection (VCRC)",
"VCU	s	Virginia Commonwealth University, Biology Department",
"VDAC	s	Virginia Department of Agriculture and Consumer Services",
"VDAM	s	Institute of Plant Science",
"VDB	s	Vanderbilt University, Department of Biological Sciences",
"VEN	s	Fundacion Instituto Botanico de Venezuela Dr. Tobias Lasser",
"VENDA	s	Thohoyandou Botanical Gardens, Department of Agriculture, Land & Environment",
"VER	s	Museo Civico di Storia Naturale",
"VF	s	Universidad de Valencia, Departamento de Biologia Vegetal, Botanica",
"VFWD	s	Vermont Fish and Wildlife Department",
"VGZ	s	Voronezh State Biosphere Reserve, Research Department",
"VI<NOR>	c	Mykotektet, National Veterinary Institute",
"VI<SWE>	s	Gotlands Fornsal",
"VIA	s	FONAIAP-CENIAP",
"VIAM	c	Institute of Applied Microbiology, University of Agricultural Sciences",
"VIAY	s	Veterinarian Institute of Armenia, Botany Department",
"VIC	s	Universidade Federal de Vicosa, Departamento de Biologia Vegetal",
"VICA	s	Agriculture Department",
"VICF	s	Forestry Department",
"VICH	s	Plant Protection Department",
"VIL	s	Universite de Paris-Sud",
"VIMS	s	Virginia Institute of Marine Science",
"VIST	s	University of the Virgin Islands, Natural Resources Program",
"VIT	s	Museo de Ciencias Naturales de Alava, Departamento de Botanica",
"VIZR	c	Collection for plant protection, All-Russian Institute of Plant Protection",
"VKM	c	All-Russian Collection of Microorganisms",
"VKPM	c	Russian National Collection of Industrial Microorganisms",
"VLA	s	Far Eastern Branch, Russian Academy of Sciences, Botany Department",
"VM	s	Okresni vlastivedne muzeum",
"VMIL	s	Virginia Military Institute, Biology Department",
"VMKSC	s	Kearney State University, Vertebrate Museum",
"VMM	s	Vanderbilt Marine Museum",
"VMNH	s	Virginia Museum of Natural History",
"VMSL	s	I.N.T.A., E.E.A. San Luis, Pastizales Naturales",
"VNC	s	Los Angeles Valley College, Life Sciences Department",
"VNGA	s	Vorarlberger Naturschau",
"VNIRO	s	Institute of Oceanography",
"VOA	s	Ostrobothnian Museum",
"VOR	s	Voronezh State University, Biology and Plant Ecology Department",
"VORG	s	Voronezh State University, Faculty of Geography and Geoecology",
"VPB	c	Veterinary Pathology and Bacteriology Collection",
"VPCI	c	Fungal Culture Collection",
"VPI	s	Virginia Polytechnic Institute and State University",
"VPIC	s	Virginia Polytechnic Institute and State University",
"VPIMM	s	Virginia Polytechnic University, Mammal Museum",
"VPM	s	Volgograd Provincial Museum",
"VPRI	c	Victoria Department of Primary Industries, Plant Disease Herbarium",
"VRLI	c	Department of Virology",
"VSC	s	Valdosta State University, Biology Department",
"VSC-L	s	Lyndon State College, Mammal Collection",
"VSCA	s	Visayas State College of Agriculture",
"VSM<NOR>	s	Det Kgl. Norske Videnskabers Selskab Museet",
"VSM<SVK>	s	Eastern Slovakian (Vychodoslovenske) Museum, Natural History Department",
"VSRI	s	N.I. Vavilov All-Russian Scientific Research Instiutte of Plant Industry",
"VSUH	s	Virginia State University, Life Sciences Department",
"VT	s	University of Vermont, Botany Department",
"VTB	c	Banco de Celulas Humanas e Animais Laboratorio de Patologia Celular e Molecular",
"VTCC	c	Vietnam Type Culture Collection, Center of Biotechnology",
"VTT	c	VTT Biotechnology, Culture Collection",
"VU	s	Voronezh State University",
"VUT	c	School of Veterinary Medicine, Faculty of Agriculture",
"VUW	s	Victoria University",
"VUWE	s	Victoria University",
"VYH	c	Finnish Environment Institute (SYKE)",
"VYM	s	Muzeum Vyakovska",
"W	s	Naturhistorisches Museum Wien, Department of Botany",
"WA	s	Warsaw University, Department of Plant Systematics and Geography",
"WAB	s	Wabash College, Biological Sciences Department",
"WAC	c	Department of Agriculture Western Australia Plant Pathogen Collection",
"WACA<USA-AZ>	s	Walnut Canyon National Monument",
"WACA<USA-OR>	s	Work Amber Collection",
"WACC	c	Western Australian Culture Collection",
"WADA	s	Western Australia Department of Agriculture",
"WAG	s	Wageningen University",
"WAHO	s	Institute of Horticultural Plant Breeding, Department of Biosystematics",
"WAIK	s	University of Waikato, Biological Sciences Department",
"WAITE	c	Insect Pathology Pathogen Collection",
"WAL	c	Wadsworth Anaerobe Laboratory, Wadsworth Hospital Center",
"WAM	s	Western Australia Museum",
"WAMP	s	Western Australian Museum",
"WAN	s	Forest Research Station",
"WANF	s	Wasatch-Cache National Forest",
"WAR	s	Warwickshire Museum, Natural History Department",
"WARC	c	New Zealand Reference Culture Collection",
"WARK	s	Western Illinois University, Biology Department",
"WARM	s	Central Missouri State University, Biology Department",
"WARMS	s	Warwickshire Museum, Natural History Department",
"WARS	s	Wildlife Advisory and Research Service",
"WASH	s	Washburn University, Biology Department",
"WAT	s	University of Waterloo, Biology Department",
"WAU	s	Wau Ecology Institute",
"WAUF	s	Warsaw Agricultural University, Department of Plant Pathology",
"WAVI	s	Colby College, Biology Department",
"WB<DEU>	s	Universitaet Wuerzburg",
"WB<USA-WI>	c	Department of Bacteriology, University of Wisconsin",
"WBCH	s	Wisbech and Fenland Museum",
"WBM	s	Universitaet Wuerzburg",
"WBR	s	Laboratoire de Paleontologie, Unversite de Montpellier",
"WBS	s	Landbouwuniversiteit",
"WCE	s	University of London, Westfield College, Biology Department",
"WCH	s	Greenwich Borough Museum",
"WCL	s	Willesden Borough Council",
"WCP	s	Walla Walla College, Biological Sciences Department",
"WCR	s	Winchester City Museum",
"WCRP	s	Winchester Public Library",
"WCSU	s	Western Connecticut State University, Department of Biological and Environmental Sciences",
"WCU	s	West China University of Medical Sciences",
"WCUH	s	Western Carolina University, Department of Biology",
"WCUM	c	Working Collection",
"WCUUM	s	West China Union University",
"WCW	s	Whitman College, Department of Biology",
"WDds	c	Raul Lopez Sanchez",
"WDNE	s	Bureau of Land Management, Winnemucca District",
"WECO	s	Wesleyan University, Biology Department",
"WEIC	s	Wau Ecology Institute",
"WELC	s	Wellesley College, Biological Sciences Department",
"WELT	s	Museum of New Zealand, Botany Department",
"WELTU	s	Victoria University of Wellington",
"WERN	s	Werneth Park Study Centre and Natural History Museum",
"WET	s	Wartburg College, Biology Department",
"WFBM	s	W.F. Barr Entomological Collection",
"WFBVA	s	Federal Forest Research Centre Vienna, Department of Vegetation Science",
"WFIS	s	Wagner Free Institute of Science",
"WFU	s	Wake Forest University, Biology Department",
"WFUVC	s	Wake Forest University, Vertebrate Collection",
"WGC	s	State University of West Georgia, Biology Department",
"WGD	s	Washington Game Department",
"WGMM	s	Woodspring Museum",
"WGRS	s	Western Ghat Regional Station of the Zoological Survey of India at Calicut",
"WH	s	Wuhan University",
"WHB	s	Universitaet fuer Bodenkultur",
"WHIT	s	Whittier College, Biology Department",
"WHM	s	West Highland Museum",
"WHN	s	Whitehaven Museum",
"WHOI	s	Woods Hole Oceanographic Institution",
"WHY	s	Whitby Museum",
"WHYNC	s	Whytby Naturalists' Club",
"WI	s	Vilnius University, Botany and Genetics Department",
"WIAP	s	Wistar Institute of Anatomy",
"WIB	s	Ministry of Environment and Parks, Resource Quality Section",
"WIBF	s	West Indian Beetle Fauna Project Collection",
"WIBG	s	Windward Islands Banana Grower's Association",
"WICA	s	Wind Cave National Park",
"WIES	s	Museum Wiesbaden",
"WII	s	Wildlife Institute of India, Department of Habitat Ecology",
"WILLI	s	The College of William and Mary, Department of Biology",
"WILLU	s	Willamette University",
"WIN	s	University of Manitoba, Botany Department",
"WINC	s	Waite Insect & Nematode Collection",
"WIND	s	National Botanical Research Institute",
"WINDM	s	Delta Marsh Field Station (University of Manitoba)",
"WINF	s	Forestry and Rural Developmen Department",
"WINFM	s	Forestry and Rural Developmen Department",
"WINO	s	Saint Mary's College, Biology Department",
"WIR	s	N. I. Vavilov Institute of Plant Industry, Department of Introduction and Systematics",
"WIS	s	University of Wisconsin, Botany Department",
"WIU	s	Western Illinois University, Museum of Natural History",
"WIUC	s	Western Illinois University",
"WJC	s	William Jewell College, Biology Department",
"WKD	s	Wakefield Museum",
"WKDS	s	Wakefield Grammar School",
"WKSU	s	Western Kentucky State University",
"WKU	s	Western Kentucky University, Department of Biology",
"WL	s	Wolong Nature Reserve",
"WLH	s	Wilberforce Library",
"WLK	s	British Columbia Ministry of Forests",
"WLMH	s	West Lake Musuem",
"WLU	s	Wilfrid Laurier University, Biology Department",
"WM	s	Gezira Research Station",
"WMGC	s	Gordon College, Biology Department",
"WMM	s	Witte Memorial Museum",
"WMNH	s	Wakayama Prefectural Museum of Natural History",
"WMS	s	Wakes Museum",
"WMU	s	Western Michigan University, Biological Sciences Department",
"WMW	s	Vestry House Museum",
"WNC	s	University of North Carolina Wilmington, Department of Biology and Marine Biology",
"WNHM	s	Oklahoma Baptist University, Webster Natural History Museum",
"WNLM	s	Niederoesterreichisches Landesmuseum",
"WNMU	s	Western New Mexico University Museum",
"WNMU:Bird		Western New Mexico University Museum, bird collection",
"WNMU:Fish		Western New Mexico University Museum, fish collection",
"WNMU:Mamm		Western New Mexico University Museum, mammal collection",
"WNRE	s	Whiteshell Nuclear Research Establishment",
"WNS	s	Wiesbaden Naturwissenschaftliche Sammlung der Stadt",
"WNU	s	Northwest University, Biology Department",
"WOCB	s	University of Windsor, Biological Sciences Department",
"WOH	s	Southwestern Oklahoma State University, Biology Department",
"WOLL	s	University of Wollongong, Department of Biological Sciences",
"WOS	s	City Museum and Art Gallery",
"WOSNH	s	Worcestershire Natural History Society Museum",
"WPBS	c	WPBS Rhizobium Collection",
"WPH	s	Waterton Lakes National Park",
"WPL	s	Whitechapel Museum",
"WPMM	s	State Museum of Pennsylvania",
"WRHM	s	Institute of Terrestrial Ecology",
"WRL	c	The Wellcome Bacterial Collection",
"WRN	s	Warrington Museum and Art Gallery",
"WRNFC	s	Warrington Field Naturalists' Club",
"WRO	s	University of Bristol, Long Ashton Research Station",
"WRSL	s	Wroclaw University, Botany Department",
"WS	s	Washington State University",
"WSBC	s	Wichita State University",
"WSC	s	Westfield State College, Museum and Herbarium",
"WSCH	s	Westfield State College, Biology Department",
"WSCO	s	Weber State University, Botany Department",
"WSF	c	Wisconsin Soil Fungi Collection",
"WSFA	s	Wilmington College, Biology Department",
"WSM	s	Weston-super-Mare Museum and Art Gallery",
"WSNM	s	White Sands National Monument",
"WSP	s	Washington State University, Plant Pathology Department",
"WSRP	s	University of Podlasie, Botany Department",
"WSU<USA-UT>	s	Weber State University, Bird and Mammal Collection",
"WSU<USA-WA>	s	Washington State University",
"WSUMNH	s	Wayne State University, Museum of Natural History",
"WSY	s	Royal Horticultural Society's Gardens",
"WTR	s	Winchester College, Biology Department",
"WTS	s	West Texas A&M University, Department of Life, Earth and Environmental Sciences",
"WTSU	s	West Texas A&M University, Natural History Collection",
"WTU	s	University of Washington",
"WTUH	s	University of Washington Botanic Gardens, College of Forest Resources",
"WU	s	Universitaet Wien",
"WU<USA-TX>	s	Wayland University",
"WUD	s	Wayne State University, Biological Sciences Department",
"WUH	s	Wuhu School of Traditional Chinese Medicine",
"WUK	s	Northwestern Institute of Botany",
"WUM	s	University of Witwatersrand",
"WUME	s	Willamette University",
"WVA	s	West Virginia University, Biology Department",
"WVBS	s	West Virginia Biological Survey",
"WVDH	c	West Virginia Hygienic Laboratory",
"WVIT	s	West Virginia University, Biology Department",
"WVMS	s	Marshall University, West Virginia Mammal Survey",
"WVN	s	Whitehaven Scientific Association",
"WVUC	s	West Virginia University",
"WVW	s	West Virginia Wesleyan College, Biology Department",
"WWB	s	Western Washington University, Biology Department",
"WWF	s	Welder Wildlife Foundation",
"WWM	s	Werner Wildlife Museum",
"WWSP	s	Weymouth Woods Sandhills Nature Preserve",
"WXDC	s	Wanxian Institute of Drug Control",
"WXM	s	North East Wales Institute, Department of Natural Science",
"WYAC	s	University of Wyoming, Range Ecology and Watershed Management Department",
"WYCO	s	Wytheville Community College, Biology Department",
"WYE	s	University of London, Wye College",
"XAG	s	Xinjiang Academy of Animal Sciences",
"XAL	s	Instituto de Ecologia, A.C.",
"XALU	s	Universidad Veracruzana",
"XBGH	s	Xian Botanical Garden",
"XCH	s	St. Xavier's College, Botany Department",
"XFCFC	s	Xiamen Fisheries College",
"XIAS	s	Xichang Agricultural School",
"XIN	s	Southwestern Guizhou Institute of Forestry",
"XJA	s	Xinjiang Agricultural University",
"XJBI	s	Xinjiang Institute of Ecology and Geography",
"XJDC	s	Xinjiang Institute for Drug Control",
"XJFA	s	Xinjiang Academy of Forestry Sciences",
"XJNU	s	Xinjiang Normal University, Biology Department",
"XJU	s	Xinjiang University, Biology Department",
"XJUG	s	Xinjiang University, Geography Department",
"XM	s	Xinjiang Medical College, Pharmacy Department",
"XNC	s	Department of Biology, Xinxiang Normal College",
"XOLO	s	Universidad Autonoma Chapingo, Departamento de Fitotecnia",
"XTNM	s	Xinjiang Institute of Traditional Chinese and Minorities Medicine",
"XUM	s	Hope Department of Entomology",
"XYTC	s	Xinyang Teachers College, Biology Department",
"XZ	s	Tibet Plateau Institute of Biology",
"XZDC	s	Tibet Institute for Drug Control",
"XZTC	s	Xuzhou Teachers College, Biology Department",
"Y	s	Yale University, Samuel Jones Record Memorial Collection",
"YA	s	National Herbarium of Cameroon",
"YAF	s	Yunnan Academy of Forestry",
"YAI	s	Armenian Agricultural Academy, Botany Department",
"YAK	s	Forest Survey and Design Institute",
"YALT	s	The State Nikita Botanical Gardens, Flora and Vegetation",
"YAM	s	Yamaguchi University, Plant Pathology Department",
"YAR	s	Yaroslavl State University, Department of Biology and Ecology",
"YBDC	s	Yibin Institute for Drug Control",
"YBI	s	Institut National pour l'Etude et la Recherche Agronomique, Departement de Botanique",
"YBLF	c	Yamanouchi Pharmaceutical Co., Ltd.",
"YCE	s	Yunnan College of Education, Biology Department",
"YCH	s	Yavapai College, Biology Department",
"YCM	s	Yokosuka City Museum",
"YCP	s	Yunnan Laboratory for Conservation of Rare, Endangered & Endemic Forest Plants, State Forestry Administration",
"YDC	s	Yunnan Institute for Drug Control",
"YELLO	s	Yellowstone National Park",
"YEO	s	Yeovil Museum",
"YF	s	Yanbei Forestry Institute",
"YFS	s	Yunnan Forestry School",
"YH	s	Lutheran College",
"YIM	s	Yunnan Institute of Pharmacology",
"YK	s	Bootham School",
"YKN	s	Yorkshire Naturalists' Trust Limited",
"YL	s	Northwest Sci-tech University of Agriculture and Forestry",
"YLD	s	Yulin Institute of Desert Control Research",
"YM<CHN>	c	Strains Collection of Yunnan Institute of Microbiology, Yunnan University, China",
"YM<GBR>	s	York Museum",
"YM<USA-CA>	s	National Park Service, Yosemite National Park",
"YMF	s	Key Laboratory of Industrial Microbiology & Fermentation Technology",
"YMUK	s	The Yorkshire Museum",
"YNP	s	The Yosemite Museum",
"YNU	s	Yokohama National University",
"YNUB	s	Yunnan Normal University, Biology Department",
"YNUGI	s	Yokohama National University - Geological Institute",
"YNUH	s	Yeungnam University, Biology Department",
"YOLA	s	Mari State University, Department of Plant Biology",
"YPM	s	Yale Peabody Museum of Natural History",
"YPM/PU	s	Princeton University Collection in Yale Peabody Museum",
"YPMC	s	Yellowstone National Park",
"YRK	s	Yorkshire Museum, Biology Department",
"YU<CHN>	s	Yunnan University",
"YU<JOR>	s	Department of Earth and Environmental Sciences, Yarmouk University",
"YU<USA-CT>	s	Yale University, Botany Division",
"YUC	s	INIREB",
"YUKU	s	Yunnan University, Biology Department",
"YUO	s	Youngstown State University, Biological Sciences Department",
"YUTO	s	York University, Biology Department",
"YXDC	s	Yuxi District Institute for Drug Control",
"YZU	s	Yuzhou University",
"Z	s	Universitaet Zuerich",
"ZA	s	University of Zagreb, Botany Department",
"ZAD	s	Mount Makulu Research Station",
"ZAHO	s	University of Zagreb, Botany Department",
"ZAR	s	Institut de Paleontologie du Museum National d'Historie Naturelle",
"ZAU	s	Zhejiang Agricultural University",
"ZAUC	s	Zhejian Agricultural University",
"ZBMM	s	Maharaja's College, Zoology and Botany Museum",
"ZCA	s	Zhelimu College of Animal Husbandry, Range Science Department",
"ZCM	s	Shetland Museum",
"ZDC	s	Zhejiang Institute for Drug Control",
"ZDEU	s	Zoology Department, Ege University",
"ZDKU	s	Kharkiv National University",
"ZDM	s	Zigong dinosaur Museum",
"ZEA	s	Universidad de Guadalajara, Centro Universitario de la Costa Sur, Departamento de Ecologia y Recursos Naturales",
"ZFMK	s	Zoologisches Forschungsinstitut und Museum Alexander Koenig",
"ZFSN	s	Laboratoire de Zoologie de la Faculte Des Sciences",
"ZGLC	s	Natural History Museum",
"ZHAN	s	Zhanjiang Teachers College, Biology Department",
"ZhM	s	Zhejinag Museum",
"ZIA	s	National Academy of Sciences of Armenia",
"ZIAN	s	Zoological Institute, Academy of Sciences",
"ZICUP	s	Zoological Institute Charles University",
"ZIHU	s	Hiroshima University, Zoological Institute",
"ZIK	s	Ukrainian Academy of Sciences, Zoological Institute",
"ZIKU	s	Zoological Institute",
"ZIL	s	Academy of Sciences, Zoological Institute",
"ZIM	c	ZIM Culture Collection of Industrial Microorganisms",
"ZIN	s	Russian Academy of Sciences, Zoological Institute",
"ZIS	s	Universitaet Saarbruecken",
"ZISB	s	Institute of Zoology",
"ZISP	s	Zoological Institute, Russian Academy of Sciences",
"ZIT	s	Grusinian Academy of Sciences",
"ZITIU	s	Zoological Institute",
"ZIUG	s	Zoologisches Institut",
"ZIUL	s	Zoologisches Institut der Universitaet",
"ZIUN	s	Universita DI Napoli",
"ZIUS<AUT>	s	Zoologisches Institute der Universitat",
"ZIUS<SWE>	s	Zoologiska Institutionen",
"ZIUT	s	Department of Zoology, Faculty of Science, University of Tokyo",
"ZIUU	s	Uppsala Universitet, Zoologiska Museum",
"ZIUW	s	Universitaet Wien, Zoologisches Institut",
"ZIUZ	s	Zagreb University",
"ZJFC	s	Zhejiang Forestry College, Forestry Department",
"ZJFI	s	Zhejiang Forestry Institute, Bamboo Department",
"ZJMA	s	Zhejiang Academy of Medical Sciences",
"ZLMU	s	Meijo University",
"ZLSYU	s	Zoological Laboratory",
"ZM	s	Zhejiang Museum of Natural History",
"ZMA	s	Universiteit van Amsterdam, Zoologisch Museum",
"ZMAN	s	Instituut voor Taxonomische Zoologie, Zoologisch Museum",
"ZMAU	s	Zoological Museum Andhra University",
"ZMB	s	Zoologisches Museum der Humboldt-Universitaet zu Berlin",
"ZMBJ	s	Banding Zoological Museum",
"ZMBN	s	Museum of Zoology at the University of Bergen, Invertebrate Collection",
"ZMC	s	Deptment of Biology, Zunyi Medical College",
"ZMFMIB	s	Zoologial Museum Fan Memorial Institute of Biology",
"ZMG<DEU-Gottingen>	s	Zoologisches Museum der Universitat Gottingen",
"ZMG<DEU-Griefswald>	s	Zoologischen Museums Greifswald",
"ZMH	s	Zoologisches Museum Hamburg",
"ZMHB	s	Museum fuer Naturkunde der Humboldt-Universitat",
"ZMHU	s	Zoologisches Museum der Humboldt Universitaet",
"ZMJU	s	Zoological Museum, Jagiellonian University",
"ZMK<DEU>	s	Zoologisches Museum der Universitaet Kiel",
"ZMK<DNK>	s	Zoological Museum, Copenhagen",
"ZMK<NOR>	s	Zoological Musem, Kristiania",
"ZMKR	s	Koenigsberg Zoologisches Museum",
"ZMKU	s	Kiev Zoological Museum",
"ZML	s	St Petersburg State University",
"ZMLP	s	University of Punjab",
"ZMLU	s	Lunds Universitet, Zoologiska Institutionen",
"ZMM	s	M. V. Lomonosov Moscow State University, Zoological Museum",
"ZMMGU	s	Zoological Museum",
"ZMMU	s	Zoological Museum, Moscow University",
"ZMNH	s	Zhejiang Museum of Natural History",
"ZMO	s	Zoology Museum, Oxford University",
"ZMSZ	s	Zemaljski Mujski",
"ZMT<CZE>	s	Zapadomoravske muzeum v Trebici",
"ZMT<GEO>	s	Georgian State Museum, Zoological Section",
"ZMU	s	Zhejiang Medical University, Pharmacy Department",
"ZMUA<CHN>	s	Zooligal Museum, University of Amoy",
"ZMUA<GRC>	s	Zoological Museum, University of Athens",
"ZMUAS	s	Zoological Museum Ukrainian Academy of Sciences",
"ZMUB	s	Museum of Zoology at the University of Bergen, Vertebrate collections",
"ZMUB:BIRD	s	Museum of Zoology at the University of Bergen, Vertebrate collections, Bird collection",
"ZMUB:HERP	s	Museum of Zoology at the University of Bergen, Vertebrate collections, Herptile collection",
"ZMUB:ICHT	s	Museum of Zoology at the University of Bergen, Vertebrate collections, Fish Collections",
"ZMUB:MAMM	s	Museum of Zoology at the University of Bergen, Vertebrate collections, Mammal collection",
"ZMUC	s	Zoological Museum, University of Copenhagen",
"ZMUD	s	University of Dhaka, Zoology Museum",
"ZMUH<DEU>	s	Zoologisches Institut und Zoologisches Museum, Universitat Hamburg",
"ZMUH<VNM>	s	Zoological Museum, University of Hanoi",
"ZMUI	s	Zoological Museum, University of Istanbul",
"ZMUL	s	Universitetets Lund, Zoologiska Museet",
"ZMUM	s	Moscow State University",
"ZMUN	s	Zoology, Natural History Museum, University of Oslo",
"ZMUO<FIN>	s	University of Oulu Zoological Museum",
"ZMUO<NOR>	s	Universitetets I Oslo, Zoologisk Museum",
"ZMUT	s	University of Tokyo, Department of Zoology",
"ZMUU	s	Uppsala Universitet, Zoologiska Museet",
"ZMUZ	s	Zoologisches Museum der Universitaet Zuerich",
"ZNM	s	Zhejiang Natural Museum",
"ZNP	s	Zion National Park",
"ZNPC	s	Springdale, Zion National Park",
"ZNU	s	Zhejiang Normal University, Biology Department",
"ZOM	s	Department of Agriculture",
"ZPAL	s	Zoological Institute of Paleobiology, Polish Academy of Sciences",
"ZPB	s	Institute of Conservation and Natural History of the Soutpansberg",
"ZRC	s	Zoological Reference Collection, National University of Singapore",
"ZSBS	s	Zoologische Sammlung des Bayerischen Staates",
"ZSI-CRS	s	Zoological Survey of India, Central Regional Station",
"ZSI-E	s	Zoological Survey of India",
"ZSI-M	s	Zoological Survey of India",
"ZSI-NRS	s	Zoological Survey of India",
"ZSI-SRS	s	Zoological Survey of India, Southern Regional Station",
"ZSI-WRS	s	Zoological Survey of India",
"ZSIC	s	Zoological Survey of India",
"ZSLC	s	Zoological Society of London",
"ZSM	s	Zoologisches Staatssammlung Muenchen",
"ZSM/CMK	s	Zoologische Museum Staatssammlung",
"ZSM/LIPI	s	Zoologische Museum Staatssammlung",
"ZSMC	s	Zoologische Staatssammlung",
"ZSP<PAK>	s	Zoological Survey of Pakistan",
"ZSP<USA-PA>	s	Zoological Society of Philadelphia",
"ZSS	s	Sukkulenten-Sammlung Zuerich",
"ZT	s	Eidgenoessische Technische Hochschule Zuerich",
"ZTNH	s	University of Vermont, Zadock Thompson Natural History Collections",
"ZTS	s	Institute of Nature Conservation, Polish Academy of Sciences, Tatra Field Station",
"ZUAB	s	Zoologia--Universidad Autonoma de Barcelona",
"ZUAC	s	University of Antananarivo",
"ZUEC	s	Museu de Historia Natural",
"ZUFES	s	Universidad Federal do Espirito Santo",
"ZUFRJ	s	Departamento de Zoolgia, Instituto de Biologia",
"ZULU	s	University of Zululand, Botany Department",
"ZUMT	s	Department of Zoology, University Museum",
"ZV	s	Technical University, Department of Phytology",
"ZVC	s	Depto. de Zoologia Vertebrados de la Facultad de Humanidades y Ciencias",
"ZVS	s	Bundesamt fuer Naturschutz",
"ZYTC	s	Zhangye Teachers College, Chemistry-Biology Department",
"ZZN	c	Zavod za naravoslovje",
"ZZSZ	s	Zoology Department, Faculty of Natural Sciences, University of Zagreb"
};


static void s_ProcessInstitutionCollectionCodeLine(const CTempString& line)
{
	vector<string> tokens;
	NStr::Tokenize(line, "\t", tokens);
	if (tokens.size() != 3) {
        ERR_POST_X(1, Warning << "Bad format in institution_codes.txt entry " << line
                   << "; disregarding");
	} else {
		switch (tokens[1].c_str()[0]) {
			case 'b':
                s_BiomaterialInstitutionCodeMap[tokens[0]] = tokens[2];
				break;
			case 'c':
				s_CultureCollectionInstitutionCodeMap[tokens[0]] = tokens[2];
				break;
			case 's':
				s_SpecimenVoucherInstitutionCodeMap[tokens[0]] = tokens[2];
				break;
			default:
				ERR_POST_X(1, Warning << "Bad format in institution_codes.txt entry " << line
						   << "; unrecognixed subtype (" << tokens[1] << "); disregarding");
				break;
		}
		s_InstitutionCodeTypeMap[tokens[0]] = tokens[1];
	}
}


static void s_InitializeInstitutionCollectionCodeMaps(void)
{
    CFastMutexGuard GUARD(s_InstitutionCollectionCodeMutex);
    if (s_InstitutionCollectionCodeMapInitialized) {
        return;
    }
    string dir;
	CRef<ILineReader> lr;
    if (CNcbiApplication* app = CNcbiApplication::Instance()) {
        dir = app->GetConfig().Get("NCBI", "Data");
        if ( !dir.empty()  
            && CFile(CDirEntry::MakePath(dir, "institution_codes.txt")).Exists()) {
            // file exists
			lr.Reset(ILineReader::New
                 (CDirEntry::MakePath(dir, "institution_codes.txt")));
        } else {
            dir.erase();
        }
    }

	if (lr.Empty()) {
        ERR_POST_X(2, Info << "s_InitializeECNumberMaps: "
                   "falling back on built-in data.");
        for (const char* const* p = kInstitutionCollectionCodeList;  *p;  ++p) {
            s_ProcessInstitutionCollectionCodeLine(*p);
        }
	} else {
        for (++*lr; !lr->AtEOF(); ++*lr) {
            s_ProcessInstitutionCollectionCodeLine(**lr);
        }
	}

    s_InstitutionCollectionCodeMapInitialized = true;
}


void CValidError_imp::ValidateOrgModVoucher(const COrgMod& orgmod, const CSerialObject& obj, const CSeq_entry *ctx)
{
	if (!orgmod.IsSetSubtype() || !orgmod.IsSetSubname() || NStr::IsBlank(orgmod.GetSubname())) {
		return;
	}

    int subtype = orgmod.GetSubtype();
	string val = orgmod.GetSubname();

    if (subtype == COrgMod::eSubtype_culture_collection
		&& NStr::Find(val, ":") == string::npos) {
		PostObjErr(eDiag_Error, eErr_SEQ_DESCR_UnstructuredVoucher, 
			       "Culture_collection should be structured, but is not",
				   obj, ctx);
		return;
	}

    string inst_code = "";
	string coll_code = "";
	string inst_coll = "";
	string id = "";
	if (!COrgMod::ParseStructuredVoucher(val, inst_code, coll_code, id)) {
		if (NStr::IsBlank(inst_code)) {
			PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadInstitutionCode, 
						"Voucher is missing institution code",
						obj, ctx);
		}
		if (NStr::IsBlank(id)) {
			PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadVoucherID, 
						"Voucher is missing specific identifier",
						obj, ctx);
		}
		return;
	}

	if (NStr::IsBlank (coll_code)) {
		inst_coll = inst_code;
	} else {
		inst_coll = inst_code + ":" + coll_code;
	}	

    s_InitializeInstitutionCollectionCodeMaps();

	// first, check combination of institution and collection (if collection found)
	TInstitutionCodeMap::iterator it = s_InstitutionCodeTypeMap.find(inst_coll);
	if (it != s_InstitutionCodeTypeMap.end()) {
		if (NStr::EqualCase (it->first, inst_coll)) {
			char type_ch = it->second.c_str()[0];
			if ((subtype == COrgMod::eSubtype_bio_material && type_ch != 'b') ||
				(subtype == COrgMod::eSubtype_culture_collection && type_ch != 'c') ||
				(subtype == COrgMod::eSubtype_specimen_voucher && type_ch != 's')) {
				if (type_ch == 'b') {
					PostObjErr (eDiag_Info, eErr_SEQ_DESCR_WrongVoucherType, 
						        "Institution code " + inst_coll + " should be bio_material", 
								obj, ctx);
				} else if (type_ch == 'c') {
					PostObjErr (eDiag_Info, eErr_SEQ_DESCR_WrongVoucherType, 
						        "Institution code " + inst_coll + " should be culture_collection",
								obj, ctx);
				} else if (type_ch == 's') {
					PostObjErr (eDiag_Info, eErr_SEQ_DESCR_WrongVoucherType, 
						        "Institution code " + inst_coll + " should be specimen_voucher",
								obj, ctx);
				}
			}
			return;
		} else if (NStr::EqualNocase(it->first, inst_coll)) {
			PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadCollectionCode, 
				        "Institution code " + inst_coll + " exists, but correct capitalization is " + it->first,
						obj, ctx);
			return;
		} 
	} else if (NStr::StartsWith(inst_coll, "personal", NStr::eNocase)) {
		// ignore personal collections	
		return;
	} else {
		// check for partial match
		bool found = false;
		if (NStr::Find(inst_coll, "<") == string::npos) {
			string check = inst_coll + "<";
			it = s_InstitutionCodeTypeMap.begin();
			while (!found && it != s_InstitutionCodeTypeMap.end()) {
				if (NStr::StartsWith(it->first, check, NStr::eNocase)) {
					found = true;
					break;
				}
				++it;
			}
		}
		if (found) {
			PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadInstitutionCode, 
				        "Institution code " + inst_coll + " needs to be qualified with a <COUNTRY> designation",
						obj, ctx);
			return;
		} else if (NStr::IsBlank(coll_code)) {
			PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadInstitutionCode, 
				        "Institution code " + inst_coll + " is not in list",
						obj, ctx);
			return;
		} else {
            it = s_InstitutionCodeTypeMap.find(inst_code);
	        if (it != s_InstitutionCodeTypeMap.end()) {
				if (NStr::Equal (coll_code, "DNA")) {
					// DNA is a valid collection for any institution (using bio_material)
					if (it->second.c_str()[0] != 'b') {
						PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_WrongVoucherType, 
							        "DNA should be bio_material",
									obj, ctx);
					}
				} else {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadCollectionCode, 
						        "Institution code " + inst_code + " exists, but collection "
								+ inst_coll + " is not in list",
								obj, ctx);
				}
				return;
			} else {
				found = false;
				if (NStr::Find(inst_code, "<") == string::npos) {
					string check = inst_code + "<";
					it = s_InstitutionCodeTypeMap.begin();
					while (!found && it != s_InstitutionCodeTypeMap.end()) {
						if (NStr::StartsWith(it->first, check, NStr::eNocase)) {
							found = true;
							break;
						}
						++it;
					}
				}
				if (found) {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadInstitutionCode, 
								"Institution code in " + inst_coll + " needs to be qualified with a <COUNTRY> designation",
								obj, ctx);
				} else {
					PostObjErr (eDiag_Warning, eErr_SEQ_DESCR_BadInstitutionCode, 
						        "Institution code " + inst_coll + " is not in list",
								obj, ctx);
				}
			}
		}
	}	
}



END_SCOPE(validator)
END_SCOPE(objects)
END_NCBI_SCOPE
