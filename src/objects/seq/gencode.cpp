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
 * Author:  Clifford Clausen
 *          (also reviewed/fixed/groomed by Denis Vakatov)
 *
 * File Description:
 *   
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.9  2003/11/21 14:45:03  grichenk
 * Replaced runtime_error with CException
 *
 * Revision 6.8  2003/06/24 14:16:33  vasilche
 * Moved auto_ptr<> variable after class definition.
 *
 * Revision 6.7  2003/04/16 11:46:25  dicuccio
 * Implemented delayed instantiation for internal interface class
 *
 * Revision 6.6  2003/03/11 15:53:25  kuznets
 * iterate -> ITERATE
 *
 * Revision 6.5  2002/05/03 21:28:13  ucko
 * Introduce T(Signed)SeqPos.
 *
 * Revision 6.4  2001/11/13 12:13:33  clausen
 * Added Codon2Idx and Idx2Codon. Modified type of code_breaks
 *
 * Revision 6.3  2001/09/07 14:16:50  ucko
 * Cleaned up external interface.
 *
 * Revision 6.2  2001/09/06 20:45:27  ucko
 * Fix scope of i_out (caught by gcc 3.0.1).
 *
 * Revision 6.1  2001/08/24 00:32:45  vakatov
 * Initial revision
 *
 * ===========================================================================
 */

#include <objects/seq/gencode.hpp>

#include <serial/serial.hpp>
#include <serial/objistr.hpp>

#include <objects/seq/Seq_data.hpp>
#include <objects/seq/IUPACna.hpp>
#include <objects/seq/NCBIeaa.hpp>
#include <objects/seqfeat/Genetic_code_table.hpp>
#include <objects/seqfeat/Genetic_code.hpp>

#include <memory>


BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE

// Singleton object holding needed state.

class CGencode_implementation
{
public:
    CGencode_implementation();
    ~CGencode_implementation();

    // see CGencode::Translate()
    void Translate
    (const CSeq_data&             in_seq,
     CSeq_data*                   out_seq,
     const CGenetic_code&         genetic_code,
     const CGencode::TCodeBreaks& code_breaks,
     TSeqPos                      uBeginIdx,
     TSeqPos                      uLength,
     bool                         bCheck_first,
     bool                         bPartial_start,
     ENa_strand                   eStrand,
     bool                         bStop,
     bool                         bRemove_trailing_x)
        const;

    unsigned int Codon2Idx(const string& codon) const;
    const string& Idx2Codon(unsigned int Idx) const;
    
private:
    // Holds gc.prt Genetic-code-table
    static const char* sm_StrGcAsn[];

    // Genetic code table data
    CRef<CGenetic_code_table> m_GcTable;
    
    // Hold map from gcIndex to gcCodon
    vector<string> m_Idx2Codon;
    
    // Hold map from gcCodon to gcIdx
    map<string, unsigned int> m_Codon2Idx;
    
    // Initialize 
    void InitIdxCodon();

    // Initialize genetic codes.
    void InitGcTable();

    // Table to ensure that all codon codes are
    // one of TCAGN and to translate these to 0-4,
    // respectively. Result used as an index to second
    // dimension in m_AaIdx
    unsigned char m_Tran[256];

    // Same as m_Tran but for complements
    // AGTCN translate to 0-4, respectively
    unsigned char m_CTran[256];

    // Table used to determine index into genetic code.
    // First dimension is na residue position in triplet
    // Second dimenstion is 0=T, 1=C, 2=A, 3=G, 4=X
    // Three values are bitwise ORed to determine index
    // into Ncbieaa or Sncbieaa string.
    unsigned char m_AaIdx[3][5];

    // Function to initialize m_Tran, m_CTran, and m_AaIdx
    void InitTran();

    // Function to get requested ncbieaa and sncbieaa strings
    void GetGeneticCode
    (const CGenetic_code& genetic_code,
     bool                 bCheck_first, // false => sncbieaa not needed
     string*              gc,           // ncbieaa string
     string*              sc)           // sncbieaa string
        const;

    // Functions to get ncbieaa genetic code string
    const string& GetNcbieaa(int id) const;
    const string& GetNcbieaa(const string& name) const;

    // Functions to get start code
    const string& GetSncbieaa(int id) const;
    const string& GetSncbieaa(const string& name) const;
};


auto_ptr<CGencode_implementation> CGencode::sm_Implementation;


// Implement the public interface.
void CGencode::Translate
(const CSeq_data&     in_seq,
 CSeq_data*           out_seq,
 const CGenetic_code& genetic_code,
 const TCodeBreaks&    code_breaks,
 TSeqPos              uBeginIdx,
 TSeqPos              uLength,
 bool                 bCheck_first,
 bool                 bPartial_start,
 ENa_strand           eStrand,
 bool                 bStop,
 bool                 bRemove_trailing_x)
{
    x_GetImplementation().Translate
        (in_seq,
         out_seq,
         genetic_code,
         code_breaks,
         uBeginIdx,
         uLength,
         bCheck_first,
         bPartial_start,
         eStrand,
         bStop,
         bRemove_trailing_x);
}

unsigned int CGencode::Codon2Idx(const string& codon)
{
    return x_GetImplementation().Codon2Idx(codon);
}

const string& CGencode::Idx2Codon(unsigned int idx)
{
    return x_GetImplementation().Idx2Codon(idx);
}


void CGencode::x_InitImplementation(void)
{
    sm_Implementation.reset(new CGencode_implementation());
}


// Constructor.
CGencode_implementation::CGencode_implementation()
{
    InitGcTable();
    InitTran();
    InitIdxCodon();
}


// Destructor. All memory allocation on the free store
// is wrapped in smart pointers and does not need
// deallocation.
CGencode_implementation::~CGencode_implementation()
{
    return;
}



// Translate na to aa
void CGencode_implementation::Translate
(const CSeq_data&             in_seq,
 CSeq_data*                   out_seq,
 const CGenetic_code&         genetic_code,
 const CGencode::TCodeBreaks& code_breaks,
 TSeqPos                      uBeginIdx,
 TSeqPos                      uLength,
 bool                         bCheck_first,
 bool                         bPartial_start,
 ENa_strand                   eStrand,
 bool                         bStop,
 bool                         bRemove_trailing_x)
    const
{
    // Check that in_seq is Iupacna
    if(in_seq.Which() != CSeq_data::e_Iupacna) {
        NCBI_THROW(CException, eUnknown,
                   "CGencode::Translate: Input sequence must be "
                   "coded as Iupacna.");
    }

    // Genetic code and start codon strings
    string gc;            // Genetic code in an Ncbieaa string
    string sc;            // Start codons in an Sncbieaa string

    // Get genetic code
    GetGeneticCode(genetic_code, bCheck_first, &gc, &sc);

    // Check that strand is either plus or minus. If it isn't
    // throw an exception with a useful message.
    if((eStrand != eNa_strand_plus)  &&  (eStrand != eNa_strand_minus)) {
        NCBI_THROW(CException, eUnknown,
                   "e_strand must be eNa_strand_plus or eNa_strand_minus.");
    }

    // Get the input sequence data
    const string& in_seq_data = in_seq.GetIupacna().Get();

    // Set out_seq to Ncbieaa
    out_seq->Reset();
    string& out_seq_data = out_seq->SetNcbieaa().Set();

    // Check that uBeginIdx not beyond end of input sequence
    if(uBeginIdx >= in_seq_data.size())
        return;

    // Adjust uLength if necessary
    if((uLength == 0) || ((uBeginIdx + uLength) > in_seq_data.size()))
        uLength = in_seq_data.size() - uBeginIdx;

    // Calculate in_seq length such that it is the greatest
    // number divisible by 3 that is less than uLength
    TSeqPos uLenMod3 = 3 * (uLength/3);

    // Allocate memory for out_seq
    size_t out_size = uLength/3;
    if((uLength % 3) != 0)
        out_size++;
    out_seq_data.resize(out_size);

    // If in_seq size < 3, skip translation and go
    // directly to special handling below
    if(uLength < 3)
        {
            out_seq_data[0] = 'X';
            goto SpecialHandling;
        }

    // if uLength not multiple of 3,
    // then put 'X' in last postion of out_seq_data
    // because there won't be enough residues at end
    // of sequence to translate (need 3)
    if((uLength % 3) != 0)
        out_seq_data[out_seq_data.size()-1] = 'X';

    {{
	// Declare iterator for out_seq_data
	string::iterator i_out;

	// Get just before beginning of out_seq_data.
	// i_out is incremented by one below before first use.
	i_out = out_seq_data.begin()-1;

	// Translate.
	// m_AaIdx is used to calculate an index for gc (genetic code in
	// Ncbieaa) and sc (start codon string in Sncbieaa). The index to gc
	// and sc are determined from a triplet of na residues. m_Tran and
	// m_CTran are used to determine an index from an na
	// residue. This index is used in the second dimension of m_AaIdx.
	// The indices are: 0=T, 1=C; 2=A, 3=G, and 4=X.
	// The first dimension to m_AaIdx correspondes to the na residue
	// position in a triplet (0, 1, or 2). m_Tran is used for the
	// plus strand, and m_CTran is used for the minus strand (reverse
	// complement of in_seq_data. The 3 values determined from m_AaIdx
	// are bitwise ORed together to create an index into gc/sc.
	if (eStrand == eNa_strand_plus) {
	    // Declare forward iterators
	    string::const_iterator i_in;
	    string::const_iterator i_in_begin;
	    string::const_iterator i_in_end;

	    // Get iterator, begin, and end for in_seq
	    i_in_begin = in_seq_data.begin() + uBeginIdx;
	    i_in_end   = i_in_begin + uLenMod3;

	    // Translate in_seq as is
	    if (bStop) {
		// Variable to track output sequence size
		TSeqPos nSize = 0;

		// Check for stop codon, and quit if found
		for (i_in=i_in_begin; i_in != i_in_end; i_in += 3) {
		    char aa =
			gc[
			   m_AaIdx[0]
			   [m_Tran[static_cast<unsigned char>(*i_in)]] |
			   m_AaIdx[1]
			   [m_Tran[static_cast<unsigned char>(*(i_in+1))]]  |
			   m_AaIdx[2]
			   [m_Tran[static_cast<unsigned char>(*(i_in+2))]]
			];

		    if(aa == '*') {
			break;
		    } else {
			*(++i_out) = aa;
			nSize++;
		    }
		}

		// Shrink out_seq_data in case stop codon found
		// before uLenMod3 na residues were decoded.
		out_seq_data.resize(nSize);

	    }
	    else { // !bStop
		// Translate through stop codons
		for(i_in=i_in_begin; i_in != i_in_end; i_in += 3) {
		    *(++i_out) =
			gc[
			   m_AaIdx[0]
			   [m_Tran[static_cast<unsigned char>(*i_in)]]  |
			   m_AaIdx[1]
			   [m_Tran[static_cast<unsigned char>(*(i_in+1))]]  |
			   m_AaIdx[2]
			   [m_Tran[static_cast<unsigned char>(*(i_in+2))]]
			];
		}
	    }
	}
	else { // eStrand == eNa_strand_minus
	    // Declare reverse iterators
	    string::const_reverse_iterator i_in;
	    string::const_reverse_iterator i_in_begin;
	    string::const_reverse_iterator i_in_end;

	    // Get reverse iterator, begin, and end for in_seq
	    i_in_begin = in_seq_data.rbegin() + in_seq_data.size() -
		uBeginIdx - uLength;
	    i_in_end = i_in_begin + uLenMod3;


	    // Translate on the complementary strand to in_seq
	    if(bStop) {
		// Variable to track output sequence size
		TSeqPos nSize = 0;

		// Check for stop codon, and quit if found
		for(i_in=i_in_begin; i_in != i_in_end; i_in += 3) {
		    char aa =
			gc[
			   m_AaIdx[0]
			   [m_CTran[static_cast<unsigned char>(*i_in)]]  |
			   m_AaIdx[1]
			   [m_CTran[static_cast<unsigned char>(*(i_in+1))]]  |
			   m_AaIdx[2]
			   [m_CTran[static_cast<unsigned char>(*(i_in+2))]]
			];

		    if (aa == '*') {
			break;
		    } else {
			*(++i_out) = aa;
			nSize++;
		    }
		}

		// Shrink out_seq_data in case stop codon found
		// before uLenMod3 na residues were decoded.
		out_seq_data.resize(nSize);
	    }
	    else {  // !bStop
		// Translate through stop codons.
		for(i_in=i_in_begin; i_in != i_in_end; i_in += 3) {
		    *(++i_out) =
			gc[
			   m_AaIdx[0]
			   [m_CTran[static_cast<unsigned char>(*i_in)]]  |
			   m_AaIdx[1]
			   [m_CTran[static_cast<unsigned char>(*(i_in+1))]]  |
			   m_AaIdx[2]
			   [m_CTran[static_cast<unsigned char>(*(i_in+2))]]
			];
		}
	    }
	}
    }}

 SpecialHandling: ;

    // Do special handling of first codon, if requested
    if(bCheck_first && (uLength >= 3)) {
        char saa;
        if(eStrand == eNa_strand_plus) {
            // Get start codon
            TSeqPos u = uBeginIdx;

            // Get the start aa for start in_seq codon (Use Sncbieaa string)
            saa =
                sc[
                   m_AaIdx[0]
                   [m_Tran[static_cast<unsigned char>(in_seq_data[u])]]  |
                   m_AaIdx[1]
                   [m_Tran[static_cast<unsigned char>(in_seq_data[u+1])]]  |
                   m_AaIdx[2]
                   [m_Tran[static_cast<unsigned char>(in_seq_data[u+2])]]
                ];

        }
        else {  // eStrand == eNa_strand_minus
            // Get index of start codon
            TSeqPos u = uBeginIdx + uLength - 1;

            // Get the start aa for start in_seq codon (Use Sncbieaa string)
            saa =
                sc[
                   m_AaIdx[0]
                   [m_CTran[static_cast<unsigned char>(in_seq_data[u])]]  |
                   m_AaIdx[1]
                   [m_CTran[static_cast<unsigned char>(in_seq_data[u-1])]]  |
                   m_AaIdx[2]
                   [m_CTran[static_cast<unsigned char>(in_seq_data[u-2])]]
                ];
        }

        // If the start aa is not a gap ('-') then use
        // the start codon (sncbieaa). If the start aa is a gap, and
        // there is a possible partial sequence at the start
        // then use the aa determined from the genetic code, otherwise
        // put in the gap sybmol.
        if(saa != '-')
            out_seq_data[0] = saa;
        else if(!bPartial_start)
            out_seq_data[0] = saa;
    }

    // Apply code_breaks, if any. Expected to be rare.
    for (CGencode::TCodeBreaks::const_iterator i_cb = code_breaks.begin();
         i_cb != code_breaks.end();  ++i_cb) {
        if (i_cb->first < out_seq_data.size())
            out_seq_data[i_cb->first] = i_cb->second;
    }

    // Remove trailing Xs, if requested.
    if(bRemove_trailing_x)
        {
            SIZE_TYPE nSize;
            bool bLoop = true;
            nSize = out_seq_data.size();
            while(bLoop)
                {
                    if(out_seq_data[nSize-1] == 'X')
                        nSize--;
                    else
                        bLoop = false;
                }
            out_seq_data.resize(nSize);
        }
} // End of Translate()



// Implement private methods

// Initialize CGenetic_code_table object from "sm_StrGcAsn"
// (see its initialization at the very end of this file)
void CGencode_implementation::InitGcTable()
{
    // Compose a long-long string
    string str;
    for (size_t i = 0;  sm_StrGcAsn[i];  i++) {
        str += sm_StrGcAsn[i];
    }

    // Create an in memory stream on sm_StrGcAsn
    CNcbiIstrstream is(str.c_str(), strlen(str.c_str()));
    auto_ptr<CObjectIStream>
        asn_codes_in(CObjectIStream::Open(eSerial_AsnText, is));

    // Create m_GcTable and initialize from asn_codes
    m_GcTable = new CGenetic_code_table;
    *asn_codes_in >> *m_GcTable;
}


// Function to initialize m_Tran, m_CTran, and m_AaIdx
void CGencode_implementation::InitTran()
{
    int i;

    // Initialize m_Tran and m_CTran;
    for(i=0; i<256; i++)
        {
            m_Tran[i] = 4;
            m_CTran[i] = 4;
        }
    m_Tran['A'] = 2; m_CTran['A'] = 0;
    m_Tran['C'] = 1; m_CTran['C'] = 3;
    m_Tran['G'] = 3; m_CTran['G'] = 1;
    m_Tran['T'] = 0; m_CTran['T'] = 2;

    // Initialize m_AaIdx
    for(i=0; i< 4; i++)
        {
            m_AaIdx[0][i] = i*16;
            m_AaIdx[1][i] = i*4;
            m_AaIdx[2][i] = i;
        }
    for(i=0; i< 3; i++)
        m_AaIdx[i][4] = 64;
}


// Initialize m_Idx2Codon and m_Codon2Idx
void CGencode_implementation::InitIdxCodon()
{
    string res("TCAG");
    int i, j, k; 
    unsigned int idx = 0;
    
    m_Idx2Codon.resize(4*4*4);
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            for(k = 0; k < 4; k++) {
                string tmp = 
                    res.substr(i,1) + res.substr(j,1) + res.substr(k,1);
                m_Codon2Idx[tmp] = idx;
                m_Idx2Codon[idx++] = tmp;
            }
        }
    }
}


// Convert a codon triplet to an index
unsigned int CGencode_implementation::Codon2Idx(const string& codon) const
{
    if( m_Codon2Idx.find(codon) != m_Codon2Idx.end() ) {
        return m_Codon2Idx.find(codon)->second;
    } else {
        NCBI_THROW(CException, eUnknown,
            "Cannot find index for " + codon);
    }
}

// Convert an index to a codon triplet
const string& CGencode_implementation::Idx2Codon(unsigned int idx) const
{
    if( idx < m_Idx2Codon.size() ) {
        return m_Idx2Codon[idx];
    } else {
        NCBI_THROW(CException, eUnknown,
            "Cannot find codon for " + NStr::IntToString(idx));
    }
}           

// Fuction to return Ncbieaa string (genetic code)
// given a genetic code id.
const string& CGencode_implementation::GetNcbieaa(int id) const
{
    // Get list of genetic codes
    const list <CRef<CGenetic_code> >& gcl = m_GcTable->Get();

    // Loop through list and find required genetic code
    ITERATE (list<CRef<CGenetic_code> >, gitor, gcl) {
        const list<CRef<CGenetic_code::C_E> >& cel = (**gitor).Get();
        list<CRef<CGenetic_code::C_E> >::const_iterator citor;

        // Find the id and check if it is the requested id
        bool found = false;
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if ((**citor).IsId()  &&  id == (**citor).GetId()) {
                found = true;
                break;
            }
        }
        if ( !found )
            continue;

        // Find ncbieaa string and return a reference
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if ( (**citor).IsNcbieaa() ) {
                return (**citor).GetNcbieaa();
            }
        }
    }

    NCBI_THROW(CException, eUnknown,
               "Requested genetic code, Id = " + NStr::IntToString(id) +
               ", not found.");
}


// Fuction to return Ncbieaa string (genetic code)
// given a genetic code name.
const string& CGencode_implementation::GetNcbieaa(const string& name) const
{
    // Get list of genetic codes
    const list<CRef<CGenetic_code> >& gcl = m_GcTable->Get();

    // Loop through list and find required genetic code
    ITERATE (list<CRef<CGenetic_code> >, gitor, gcl) {
        const list<CRef<CGenetic_code::C_E> >& cel = (**gitor).Get();
        list<CRef<CGenetic_code::C_E> >::const_iterator citor;

        // Find the name and check if it is the requested name
        bool found = false;
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if((**citor).IsName()  &&  name == (**citor).GetName()) {
                found = true;
                break;
            }
        }
        if ( !found )
            continue;

        // Find ncbieaa string and return a reference
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if ((**citor).IsNcbieaa()) {
                return (**citor).GetNcbieaa();
            }
        }
    }

    NCBI_THROW(CException, eUnknown,
               "Genetic code, name = " + name + ", not found.");
}


// Fuction to return Sncbieaa string (start codon string)
// given a genetic code id.
const string& CGencode_implementation::GetSncbieaa(int id) const
{
    // Get list of genetic codes
    const list<CRef<CGenetic_code> >& gcl = m_GcTable->Get();

    // Loop through list and find required genetic code
    ITERATE (list<CRef<CGenetic_code> >, gitor, gcl) {
        const list<CRef<CGenetic_code::C_E> >& cel = (**gitor).Get();
        list<CRef<CGenetic_code::C_E> >::const_iterator citor;

        // Find the id and check if it is the requested id
        bool found = false;
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if ((**citor).IsId()  &&  id == (**citor).GetId()) {
                found = true;
                break;
            }
        }
        if ( !found )
            continue;


        // Find sncbieaa string and return a reference
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if ( (**citor).IsSncbieaa() ) {
                return (**citor).GetSncbieaa();
            }
        }
    }

    NCBI_THROW(CException, eUnknown,
               "Requested start codon string, Id = " + NStr::IntToString(id)
               + ", not found.");
}


// Fuction to return Sncbieaa string (start codon string)
// given a genetic code name.
const string& CGencode_implementation::GetSncbieaa(const string& name) const
{
    // Get list of genetic codes
    const list<CRef<CGenetic_code> >& gcl = m_GcTable->Get();

    // Loop through list and find required genetic code
    ITERATE (list<CRef<CGenetic_code> >, gitor, gcl) {
        const list<CRef<CGenetic_code::C_E> >& cel = (**gitor).Get();
        list<CRef<CGenetic_code::C_E> >::const_iterator citor;

        // Find the name and check if it is the requested name
        bool found = false;
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if ((**citor).IsName()  &&  name == (**citor).GetName()) {
                found = true;
                break;
            }
        }
        if ( !found )
            continue;


        // Find sncbieaa string and return a reference
        for (citor = cel.begin();  citor != cel.end();  ++citor) {
            if( (**citor).IsSncbieaa() ) {
                return (**citor).GetSncbieaa();
            }
        }
    }

    NCBI_THROW(CException, eUnknown,
               "Start codon string, name = " + name + ", not found.");
}


// Function to get requested ncbieaa and sncbieaa strings
void CGencode_implementation::GetGeneticCode
(const CGenetic_code&       genetic_code,
 bool                       bCheck_first,   // false => sncbieaa not needed
 string*                    gc,             // ncbieaa string
 string*                    sc)             // sncbieaa string
    const
{
    bool bGc_found = false; // Indicates if requested gc found
    bool bSc_found = false; // Indicates if requested sc found

    // Get the genetic code request (id, name, ncibeaa, and/or sncibeaa)
    const list<CRef<CGenetic_code::C_E> >& lst = genetic_code.Get();

    // Loop through the list and figure out what genetic code
    // is being requested
    ITERATE (list<CRef<CGenetic_code::C_E> >, i_lst, lst) {
        try {
            switch((*i_lst)->Which()){

            case CGenetic_code::C_E::e_Name:

                // Genetic code by name was specified
                if(!bGc_found)
                    {
                        *gc = GetNcbieaa((*i_lst)->GetName());
                        bGc_found = true;
                        if(gc->size() < 128)
                            gc->append(128 - gc->size(), 'X');
                    }
                if(!bSc_found)
                    {
                        *sc = GetSncbieaa((*i_lst)->GetName());
                        bSc_found = true;
                        if(sc->size() < 128)
                            sc->append(128 - sc->size(), '-');
                    }
                break;

            case CGenetic_code::C_E::e_Id:

                // Genetic code by id was specified
                if(!bGc_found)
                    {
                        *gc = GetNcbieaa((*i_lst)->GetId());
                        bGc_found = true;
                        if(gc->size() < 128)
                            gc->append(128 - gc->size(), 'X');
                    }
                if(!bSc_found)
                    {
                        *sc = GetSncbieaa((*i_lst)->GetId());
                        bSc_found = true;
                        if(sc->size() < 128)
                            sc->append(128 - sc->size(), '-');
                    }
                break;

            case CGenetic_code::C_E::e_Ncbieaa:

                // A genetic code string is provided in genetic_code parameter
                *gc = (*i_lst)->GetNcbieaa();
                bGc_found = true;
                if(gc->size() < 128)
                    gc->append(128 - gc->size(), 'X');
                break;

            case CGenetic_code::C_E::e_Sncbieaa:

                // A start codons string is provided in genetic_code parameter
                *sc = (*i_lst)->GetSncbieaa();
                bSc_found = true;
                if(sc->size() < 128)
                    sc->append(128 - sc->size(), '-');
                break;

            default:
                break;
            }
        }
        STD_CATCH("");
    }

    // If genetic code not found
    if ( !bGc_found ) {
        NCBI_THROW(CException, eUnknown,
            "No genetic code found.");
    }

    // If start codon string not found
    if (!bSc_found  &&  bCheck_first) {
        NCBI_THROW(CException, eUnknown,
            "No start codon string found.");
    }
}



/////////////////////////////////////////////////////////////////////////////
//  CGencode_implementation::sm_StrGcAsn  --  some very long and ugly string
//
//  NOTE:  because of the weird MSVC++ compiler limitations on the string
//         length (2096), it had to be split into parts.
//

const char* CGencode_implementation::sm_StrGcAsn[] =
{
    " --**************************************************************************\n--  This is the NCBI genetic code table\n--  Initial base data set from Andrzej Elzanowski while at PIR International\n--  Addition of Eubacterial and Alternative Yeast by J.Ostell at NCBI\n--  Base 1-3 of each codon have been added as comments to facilitate\n--    readability at the suggestion of Peter Rice, EMBL\n--\n--  Version 3.0 - 1994\n--  Updated as per Andrzej Elzanowski at NCBI\n--     Complete documentation in NCBI toolkit documentation\n--  Note: 2 genetic codes have been deleted\n--\n--   Old id   Use id	 - Notes\n--\n--   id 7      id 4      - Kinetoplast code now merged in code id 4\n--                       - 7 still here, but is just a copy of 4\n--                       -    will be removed soon \n--   id 8      id 1      - all plant chloroplast differences due to RNA edit\n--\n--*************************************************************************\n\nGenetic-code-table ::= {\n	{\n		name \"Standard\" ,\n		name \"SGC0\" ,\n		id 1 ,\n		ncbieaa  \"FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Vertebrate Mitochondrial\" ,\n		name \"SGC1\" ,\n		id 2 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSS**VVVVAAAADDEEGGGG\",\n		sncbieaa \"--------------------------------MMMM---------------M------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n",
        "        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Yeast Mitochondrial\" ,\n		name \"SGC2\" ,\n		id 3 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWTTTTPPPPHHQQRRRRIIMMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Mold, Protozoan, Coelenterate Mitochondrial and Mycoplasma/Spiroplasma\" ,\n		name \"SGC3\" ,\n		id 4 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"--MM---------------M------------MMMM---------------M------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Invertebrate Mitochondrial\" ,\n		name \"SGC4\" ,\n		id 5 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSSSSVVVVAAAADDEEGGGG\",\n		sncbieaa \"---M----------------------------M-MM----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Ciliate Macronuclear and Daycladacean\" ,\n		name \"SGC5\" ,\n		id 6 ,\n		ncbieaa  \"FFLLSSSSYYQQCC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n",
        "        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name ",
        "\"Mold, Protozoan, Coelenterate Mitochondrial and Mycoplasma/Spiroplasma\" ,\n		name \"SGC3\" ,\n		id 7 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"--MM---------------M------------MMMM---------------M------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Echinoderm Mitochondrial\" ,\n		name \"SGC8\" ,\n		id 9 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNNKSSSSVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Alternative Ciliate Macronuclear\" ,\n		name \"SGC9\" ,\n		id 10 ,\n		ncbieaa  \"FFLLSSSSYY*QCCCWLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Eubacterial\" ,\n		id 11 ,\n		ncbieaa  \"FFLLSSSSYY**CC*WLLLLPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"---M---------------M------------M--M---------------M------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n",
        "        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Alternative Yeast\" ,\n		id 12 ,\n		ncbieaa  \"FFLLSSSSYY**CC*WLLLSPPPPHHQQRRRRIIIMTTTTNNKKSSRRVVVVAAAADDEEGGGG\",\n		sncbieaa \"-------------------M---------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Ascidian Mitochondrial\" ,\n		id 13 ,\n		ncbieaa  \"FFLLSSSSYY**CCWWLLLLPPPPHHQQRRRRIIMMTTTTNNKKSSGGVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	} ,\n	{\n		name \"Flatworm Mitochondrial\" ,\n		id 14 ,\n		ncbieaa  \"FFLLSSSSYYY*CCWWLLLLPPPPHHQQRRRRIIIMTTTTNNNKSSSSVVVVAAAADDEEGGGG\",\n		sncbieaa \"-----------------------------------M----------------------------\"\n        -- Base1  TTTTTTTTTTTTTTTTCCCCCCCCCCCCCCCCAAAAAAAAAAAAAAAAGGGGGGGGGGGGGGGG\n        -- Base2  TTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGGTTTTCCCCAAAAGGGG\n        -- Base3  TCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAGTCAG\n	}\n}\n\n",
        0  // to indicate that there is no more data
};


END_objects_SCOPE
END_NCBI_SCOPE
