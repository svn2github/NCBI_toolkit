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
 * Authors:  Josh Cherry
 *
 * File Description:  Simple pattern matching for sequences
 *
 */


#include <corelib/ncbistd.hpp>
#include "seq_match.hpp"

BEGIN_NCBI_SCOPE



char CSeqMatch::IupacToNcbi8na(char in)
{
    char A = 1, C = 2, G = 4, T = 8;
    char out;
    switch (in) {
    case 'A':
        out = A;
        break;
    case 'G':
        out = G;
        break;
    case 'C':
        out = C;
        break;
    case 'T':
        out = T;
        break;
    case 'R':
        out = A | G;
        break;
    case 'Y':
        out = T | C;
        break;
    case 'S':
        out = G | C;
        break;
    case 'W':
        out = A | T;
        break;
    case 'M':
        out = A | C;
        break;
    case 'K':
        out = G | T;
        break;
    case 'B':
        out = G | C | T;
        break;
    case 'D':
        out = A | G | T;
        break;
    case 'H':
        out = A | C | T;
        break;
    case 'V':
        out = A | G | C;
        break;
    case 'N':
        out = A | G | C | T;
        break;
    default:
        throw runtime_error(string("couldn't covert ") + in +
                            " to ncbi8na: invalid IUPAC code ");
        break;
    }
    return out;
}        


void CSeqMatch::IupacToNcbi8na(const string& in, string& out)
{
    out.resize(in.size());
    for (unsigned int i = 0;  i < in.length();  i++) {
        out[i] = IupacToNcbi8na(in[i]);
    }
}


char CSeqMatch::CompNcbi8na(char in)
{
    char A = 1, C = 2, G = 4, T = 8;
    char out;

    out = bool(in & A) * T + bool(in & C) * G
        + bool(in & G) * C + bool(in & T) * A;

    return out;
}        


void CSeqMatch::CompNcbi8na(string& seq)
{
    reverse(seq.begin(), seq.end());
    NON_CONST_ITERATE (string, base, seq) {
        *base = CompNcbi8na(*base);
    }
}

END_NCBI_SCOPE


/*
 * ===========================================================================
 * $Log$
 * Revision 1.1  2003/08/12 18:52:58  jcherry
 * Initial version
 *
 * ===========================================================================
 */

