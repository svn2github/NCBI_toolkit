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
 *   using the following specifications:
 *   'seqfeat.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>
#include <serial/enumvalues.hpp>

// generated includes
#include <objects/seqfeat/SubSource.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSubSource::~CSubSource(void)
{
}


void CSubSource::GetLabel(string* str) const
{
    typedef pair<TSubtype, const char*> TPair;
    static const TPair sc_Pairs[] = {
        TPair(eSubtype_chromosome,              "chromosome"),
        TPair(eSubtype_map,                     "map"),
        TPair(eSubtype_clone,                   "clone"),
        TPair(eSubtype_subclone,                "subclone"),
        TPair(eSubtype_haplotype,               "haplotype"),
        TPair(eSubtype_genotype,                "genotype"),
        TPair(eSubtype_sex,                     "sex"),
        TPair(eSubtype_cell_line,               "cell_line"),
        TPair(eSubtype_cell_type,               "cell_type"),
        TPair(eSubtype_tissue_type,             "tissue_type"),
        TPair(eSubtype_clone_lib,               "clone_lib"),
        TPair(eSubtype_dev_stage,               "dev_stage"),
        TPair(eSubtype_frequency,               "frequency"),
        TPair(eSubtype_germline,                "germline"),
        TPair(eSubtype_rearranged,              "rearranged"),
        TPair(eSubtype_lab_host,                "lab_host"),
        TPair(eSubtype_pop_variant,             "pop_variant"),
        TPair(eSubtype_tissue_lib,              "tissue_lib"),
        TPair(eSubtype_plasmid_name,            "plasmid_name"),
        TPair(eSubtype_transposon_name,         "transposon_name"),
        TPair(eSubtype_insertion_seq_name,      "insertion_seq_name"),
        TPair(eSubtype_plastid_name,            "plastid_name"),
        TPair(eSubtype_country,                 "country"),
        TPair(eSubtype_segment,                 "segment"),
        TPair(eSubtype_endogenous_virus_name,   "endogenous_virus_name"),
        TPair(eSubtype_transgenic,              "transgenic"),
        TPair(eSubtype_environmental_sample,    "environmental_sample"),
        TPair(eSubtype_isolation_source,        "isolation_source"),
        TPair(eSubtype_lat_lon,                 "lat_lon"),
        TPair(eSubtype_collection_date,         "collection_date"),
        TPair(eSubtype_collected_by,            "collected_by"),
        TPair(eSubtype_identified_by,           "identified_by"),
        TPair(eSubtype_fwd_primer_seq,          "fwd_primer_seq"),
        TPair(eSubtype_rev_primer_seq,          "rev_primer_seq"),
        TPair(eSubtype_fwd_primer_name,         "fwd_primer_name"),
        TPair(eSubtype_rev_primer_name,         "rev_primer_name"),
        TPair(eSubtype_other,                   "other")
    };
    typedef CStaticArrayMap<TSubtype, const char*> TSubtypeMap;
    DEFINE_STATIC_ARRAY_MAP(TSubtypeMap, sc_Map, sc_Pairs);

    *str += '/';
    TSubtypeMap::const_iterator iter = sc_Map.find(GetSubtype());
    if (iter != sc_Map.end()) {
        *str += iter->second;
    } else {
        *str += "unknown";
    }
    *str += '=';
    *str += GetName();
    if (IsSetAttrib()) {
        *str += " (";
        *str += GetAttrib();
        *str += ")";
    }
}


CSubSource::TSubtype CSubSource::GetSubtypeValue(const string& str)
{
    string name = NStr::TruncateSpaces(str);
    NStr::ToLower(name);
    replace(name.begin(), name.end(), '_', '-');

    return ENUM_METHOD_NAME(ESubtype)()->FindValue(str);
}


// =============================================================================
//                                 Country Names
// =============================================================================


// legal country names
const string CCountries::sm_Countries[] = {
    "Afghanistan",
    "Albania",
    "Algeria",
    "American Samoa",
    "Andorra",
    "Angola",
    "Anguilla",
    "Antarctica",
    "Antigua and Barbuda",
    "Arctic Ocean",
    "Argentina",
    "Armenia",
    "Aruba",
    "Ashmore and Cartier Islands",
    "Atlantic Ocean",
    "Australia",
    "Austria",
    "Azerbaijan",
    "Bahamas",
    "Bahrain",
    "Baker Island",
    "Bangladesh",
    "Barbados",
    "Bassas da India",
    "Belarus",
    "Belgium",
    "Belize",
    "Benin",
    "Bermuda",
    "Bhutan",
    "Bolivia",
    "Bosnia and Herzegovina",
    "Botswana",
    "Bouvet Island",
    "Brazil",
    "British Virgin Islands",
    "Brunei",
    "Bulgaria",
    "Burkina Faso",
    "Burundi",
    "Cambodia",
    "Cameroon",
    "Canada",
    "Cape Verde",
    "Cayman Islands",
    "Central African Republic",
    "Chad",
    "Chile",
    "China",
    "Christmas Island",
    "Clipperton Island",
    "Cocos Islands",
    "Colombia",
    "Comoros",
    "Cook Islands",
    "Coral Sea Islands",
    "Costa Rica",
    "Cote d'Ivoire",
    "Croatia",
    "Cuba",
    "Cyprus",
    "Czech Republic",
    "Democratic Republic of the Congo",
    "Denmark",
    "Djibouti",
    "Dominica",
    "Dominican Republic",
    "East Timor",
    "Ecuador",
    "Egypt",
    "El Salvador",
    "Equatorial Guinea",
    "Eritrea",
    "Estonia",
    "Ethiopia",
    "Europa Island",
    "Falkland Islands (Islas Malvinas)",
    "Faroe Islands",
    "Fiji",
    "Finland",
    "France",
    "French Guiana",
    "French Polynesia",
    "French Southern and Antarctic Lands",
    "Gabon",
    "Gambia",
    "Gaza Strip",
    "Georgia",
    "Germany",
    "Ghana",
    "Gibraltar",
    "Glorioso Islands",
    "Greece",
    "Greenland",
    "Grenada",
    "Guadeloupe",
    "Guam",
    "Guatemala",
    "Guernsey",
    "Guinea-Bissau",
    "Guinea",
    "Guyana",
    "Haiti",
    "Heard Island and McDonald Islands",
    "Honduras",
    "Hong Kong",
    "Howland Island",
    "Hungary",
    "Iceland",
    "India",
    "Indian Ocean",
    "Indonesia",
    "Iran",
    "Iraq",
    "Ireland",
    "Isle of Man",
    "Israel",
    "Italy",
    "Jamaica",
    "Jan Mayen",
    "Japan",
    "Jarvis Island",
    "Jersey",
    "Johnston Atoll",
    "Jordan",
    "Juan de Nova Island",
    "Kazakhstan",
    "Kenya",
    "Kerguelen Archipelago",
    "Kingman Reef",
    "Kiribati",
    "Kosovo",
    "Kuwait",
    "Kyrgyzstan",
    "Laos",
    "Latvia",
    "Lebanon",
    "Lesotho",
    "Liberia",
    "Libya",
    "Liechtenstein",
    "Lithuania",
    "Luxembourg",
    "Macau",
    "Macedonia",
    "Madagascar",
    "Malawi",
    "Malaysia",
    "Maldives",
    "Mali",
    "Malta",
    "Marshall Islands",
    "Martinique",
    "Mauritania",
    "Mauritius",
    "Mayotte",
    "Mexico",
    "Micronesia",
    "Midway Islands",
    "Moldova",
    "Monaco",
    "Mongolia",
    "Montenegro",
    "Montserrat",
    "Morocco",
    "Mozambique",
    "Myanmar",
    "Namibia",
    "Nauru",
    "Navassa Island",
    "Nepal",
    "Netherlands Antilles",
    "Netherlands",
    "New Caledonia",
    "New Zealand",
    "Nicaragua",
    "Niger",
    "Nigeria",
    "Niue",
    "Norfolk Island",
    "North Korea",
    "North Sea",
    "Northern Mariana Islands",
    "Norway",
    "Oman",
    "Pacific Ocean",
    "Pakistan",
    "Palau",
    "Palmyra Atoll",
    "Panama",
    "Papua New Guinea",
    "Paracel Islands",
    "Paraguay",
    "Peru",
    "Philippines",
    "Pitcairn Islands",
    "Poland",
    "Portugal",
    "Puerto Rico",
    "Qatar",
    "Republic of the Congo",
    "Reunion",
    "Romania",
    "Russia",
    "Rwanda",
    "Saint Helena",
    "Saint Kitts and Nevis",
    "Saint Lucia",
    "Saint Pierre and Miquelon",
    "Saint Vincent and the Grenadines",
    "Samoa",
    "San Marino",
    "Sao Tome and Principe",
    "Saudi Arabia",
    "Senegal",
    "Serbia and Montenegro",
    "Serbia",
    "Seychelles",
    "Sierra Leone",
    "Singapore",
    "Slovakia",
    "Slovenia",
    "Solomon Islands",
    "Somalia",
    "South Africa",
    "South Georgia and the South Sandwich Islands",
    "South Korea",
    "Spain",
    "Spratly Islands",
    "Sri Lanka",
    "Sudan",
    "Suriname",
    "Svalbard",
    "Swaziland",
    "Sweden",
    "Switzerland",
    "Syria",
    "Taiwan",
    "Tajikistan",
    "Tanzania",
    "Thailand",
    "Togo",
    "Tokelau",
    "Tonga",
    "Trinidad and Tobago",
    "Tromelin Island",
    "Tunisia",
    "Turkey",
    "Turkmenistan",
    "Turks and Caicos Islands",
    "Tuvalu",
    "Uganda",
    "Ukraine",
    "United Arab Emirates",
    "United Kingdom",
    "Uruguay",
    "USA",
    "Uzbekistan",
    "Vanuatu",
    "Venezuela",
    "Viet Nam",
    "Virgin Islands",
    "Wake Island",
    "Wallis and Futuna",
    "West Bank",
    "Western Sahara",
    "Yemen",
    "Yugoslavia",
    "Zambia",
    "Zimbabwe",
};

const string CCountries::sm_Former_Countries[] = {
    "Belgian Congo",
    "British Guiana",
    "Burma",
    "Czechoslovakia",
    "Ivory Coast",
    "Serbia and Montenegro",
    "Siam",
    "USSR",
    "Yugoslavia",
    "Zaire",
};

bool CCountries::IsValid(const string& country)
{
    string name = country;
    size_t pos = country.find(':');

    if ( pos != string::npos ) {
        name = country.substr(0, pos);
    }

    // try current countries
    const string *begin = sm_Countries;
    const string *end = &(sm_Countries[sizeof(sm_Countries) / sizeof(string)]);

    return find(begin, end, name) != end;
}


bool CCountries::WasValid(const string& country)
{
    string name = country;
    size_t pos = country.find(':');

    if ( pos != string::npos ) {
        name = country.substr(0, pos);
    }

    // try formerly-valid countries
    const string *begin = sm_Former_Countries;
    const string *end = &(sm_Former_Countries[sizeof(sm_Former_Countries) / sizeof(string)]);

    return find(begin, end, name) != end;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 65, chars: 1891, CRC32: 7724f0c5 */
