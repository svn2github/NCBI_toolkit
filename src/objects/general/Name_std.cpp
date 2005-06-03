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
 *   'general.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>
#include <objects/general/cleanup_utils.hpp>

// generated includes
#include <objects/general/Name_std.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CName_std::~CName_std(void)
{
}


const CName_std::TSuffixes& CName_std::GetStandardSuffixes(void)
{
    static const string sfxs[] = {"II", "III", "IV", "Jr.", "Sr.", "V", "VI"};
    static const TSuffixes suffixes(sfxs, sizeof(sfxs));

    return suffixes;
}





//=========================================================================//
//                             Basic Cleanup                               //
//=========================================================================//

void CName_std::BasicCleanup(bool fix_initials)
{
    // before cleanup, get information from initials
    if (IsSetInitials()) {
        if (!IsSetSuffix()) {
            x_ExtractSuffixFromInitials();
        }
        x_FixEtAl();
    }

    // do strings cleanup
    CLEAN_STRING_MEMBER(Last);
    CLEAN_STRING_MEMBER(First);
    CLEAN_STRING_MEMBER(Middle);
    CLEAN_STRING_MEMBER(Full);
    CLEAN_STRING_MEMBER(Initials);
    CLEAN_STRING_MEMBER(Suffix);
    CLEAN_STRING_MEMBER(Title);

    if (IsSetSuffix()) {
        x_FixSuffix();
    }

    // regenerate initials from first name
    if (fix_initials) {
        x_FixInitials();
    }
}


void CName_std::x_FixEtAl(void)
{
    _ASSERT(IsSetInitials());

    if (IsSetLast()  &&  NStr::Equal(GetLast(), "et")  &&
        (NStr::Equal(GetInitials(), "al")  ||  NStr::Equal(GetInitials(), "al."))  &&
        (!IsSetFirst()  ||  NStr::IsBlank(GetFirst()))) {
        ResetFirst();
        ResetInitials();
        SetLast("et al.");
    }
}


static void s_FixInitials(string& initials)
{
    string fixed;

    string::iterator it = initials.begin();
    while (it != initials.end()  &&  !isalpha((unsigned char)(*it))) {  // skip junk
        ++it;
    }
    char prev = '\0';
    while (it != initials.end()) {
        char c = *it;
    
        if (c == ',') {
            *it = c = '.';
        }
        if (isalpha((unsigned char) c)) {
            if (prev != '\0'  &&  isupper((unsigned char) c)  &&  isupper((unsigned char) prev)) {
                fixed += '.';
            }
        } else if (c == '-') {
            if (prev != '\0'  &&  prev != '.') {
                fixed += '.';
            }
        } else if (c == '.') {
            if (prev == '-'  ||  prev == '.') {
                ++it;
                continue;
            }
        } else {
            ++it;
            continue;
        }
        fixed += c;

        prev = c;
        ++it;
    }
    if (!fixed.empty()  &&  fixed[fixed.length() - 1] == '-') {
        fixed.resize(fixed.length() - 1);
    }
    initials = fixed;
}


void CName_std::x_FixInitials(void)
{
    if (IsSetInitials()) {
        s_FixInitials(SetInitials());
        
        if (SetInitials().empty()) {
            ResetInitials();
        }
    }

    if (IsSetFirst()  &&  !IsSetInitials()) {
        SetInitials(x_GetInitialsFromFirst());
        return;
    }
    if (!IsSetInitials()) {
        return;
    }

    if (!IsSetFirst()) {
        return;
    }
    _ASSERT(IsSetFirst()  &&  IsSetInitials());

    TInitials& initials = SetInitials();
    string f_initials = x_GetInitialsFromFirst();
    if (initials == f_initials) {
        return;
    }
    
    string::iterator it1 = f_initials.begin();
    string::iterator it2 = initials.begin();
    while (it1 != f_initials.end()  &&  it2 != initials.end()  &&
        toupper((unsigned char)(*it1)) == toupper((unsigned char)(*it2))) {
        ++it1;
        ++it2;
    }
    if (it2 != initials.end()  &&  it1 != f_initials.end()  &&  *it1 == '.'  &&  islower((unsigned char)(*it2))) {
        f_initials.erase(it1);
    }
    while (it2 != initials.end()) {
        f_initials += *it2++;
    }
    initials.swap(f_initials);
}


// mapping of wrong suffixes to the correct ones.
typedef pair<string, string> TStringPair;
static const TStringPair bad_sfxs[] = {
    TStringPair("1d"  , "I"),
    TStringPair("1st" , "I"),
    TStringPair("2d"  , "II"),
    TStringPair("2nd" , "II"),
    TStringPair("3d"  , "III"),
    TStringPair("3rd" , "III"),
    TStringPair("4th" , "IV"),
    TStringPair("5th" , "V"),
    TStringPair("6th" , "VI"),
    //TStringPair("I."  , "I"),
    TStringPair("II." , "II"),
    TStringPair("III.", "III"),
    TStringPair("IV." , "IV"),
    TStringPair("Jr"  , "Jr."),
    TStringPair("Sr"  , "Sr."),    
    //TStringPair("V."  , "V"),
    TStringPair("VI." , "VI")
};
typedef CStaticArrayMap<string, string> TSuffixMap;
static const TSuffixMap sc_BadSuffixes(bad_sfxs, sizeof(bad_sfxs));

// move the suffix from the initials field to the suffix field.
void CName_std::x_ExtractSuffixFromInitials(void)
{
    _ASSERT(IsSetInitials()  &&  !IsSetSuffix());

    string& initials = SetInitials();
    size_t len = initials.length();

    if (initials.find('.') == NPOS) {
        return;
    }

    // look for standard suffixes in intials
    const TSuffixes& suffixes = GetStandardSuffixes();
    TSuffixes::const_iterator best = suffixes.end();
    ITERATE (TSuffixes, it, GetStandardSuffixes()) {
        if (NStr::EndsWith(initials, *it)) {
            if (best == suffixes.end()) {
                best = it;
            } else if (best->length() < it->length()) {
                best = it;
            }
        }
    }
    if (best != suffixes.end()) {
        initials.resize(len - best->length());
        SetSuffix(*best);
        return;
    }

    // look for bad suffixes in initails
    ITERATE (TSuffixMap, it, sc_BadSuffixes) {
        if (NStr::EndsWith(initials, it->first)  &&
            initials.length() > it->first.length()) {
            initials.resize(len - it->first.length());
            SetSuffix(it->second);
            return;
        }
    }
}


void CName_std::x_FixSuffix(void)
{
    _ASSERT(IsSetSuffix());

    TSuffix& suffix = SetSuffix();

    ITERATE (TSuffixMap, it, sc_BadSuffixes) {
        if (NStr::EqualNocase(suffix, it->first)) {
            SetSuffix(it->second);
            break;
        }
    }
}


CName_std::TInitials CName_std::x_GetInitialsFromFirst(void) const
{
    _ASSERT(IsSetFirst());

    const TFirst& first = GetFirst();
    if (NStr::IsBlank(first)) {
        return kEmptyStr;
    }

    bool up =
        IsSetInitials()  &&  !GetInitials().empty()  &&  isupper((unsigned char) GetInitials()[0]);

    string initials;

	string::const_iterator end = first.end();
	string::const_iterator it = first.begin();
	while (it != end) {
        // skip "junk"
        while (it != end  &&  !isalpha((unsigned char)(*it))) {
            ++it;
        }
        // copy the first character of each word
        if (it != end) {
            if (!initials.empty()) {
                char c = *initials.rbegin();
                if (isupper((unsigned char)(*it))  &&  c != '.'  &&  c != '-') {
                    initials += '.';
                }
            }
            initials += up ? toupper((unsigned char)(*it)) : *it;
        }
        // skip the rest of the word
        while (it != end  &&  !isspace((unsigned char)(*it))  &&  *it != ','  &&  *it != '-') {
            ++it;
        }
        if (it != end  &&  *it == '-') {
            initials += '.';
            initials += *it++;
        }
        up = false;
    }
    if (!initials.empty()  &&  !(*initials.rbegin() == '-')) {
        initials += '.';
    }

    return initials;
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 6.2  2005/06/03 16:52:09  lavr
* Explicit (unsigned char) casts in ctype routines
*
* Revision 6.1  2005/05/20 13:31:29  shomrat
* Added BasicCleanup()
*
*
* ===========================================================================
*/
/* Original file checksum: lines: 65, chars: 1888, CRC32: daed53d2 */

