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
 * Authors:  Christiam Camacho
 *
 * File Description:
 *   Main driver for blast2sequences C++ interface
 *
 */

#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>

#include <serial/iterator.hpp>

#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/seq_vector.hpp>
#include <objmgr/gbloader.hpp>
#include <objmgr/util/sequence.hpp>

#include <objects/seq/seq__.hpp>
#include <objects/seqloc/seqloc__.hpp>
#include <objects/seqset/seqset__.hpp>

#include <corelib/ncbitime.hpp>
#include <objtools/readers/fasta.hpp>

#include <Bl2Seq.hpp>
#include <ctools/asn_converter.hpp>

// C includes for C formatter
#include <objalign.h>
#include <sqnutils.h>
#include <txalign.h>
#include <accid1.h>

USING_NCBI_SCOPE;
USING_SCOPE(objects);


/////////////////////////////////////////////////////////////////////////////
//  CBlast2seqApplication::


class CBlast2seqApplication : public CNcbiApplication
{
private:
    virtual void Init(void);
    virtual int  Run(void);
    virtual void Exit(void);

    void InitScope(void);
    CBlastOption::EProgram GetBlastProgramNum(const string& prog);

    // needed for debugging only
    FILE* GetOutputFilePtr(void);

    CRef<CObjectManager>    m_ObjMgr;
    CRef<CScope>            m_Scope;
};

void CBlast2seqApplication::Init(void)
{
    HideStdArgs(fHideLogfile | fHideConffile | fHideVersion);
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "Compares 2 sequence using the BLAST algorithm");

    // Program type
    arg_desc->AddKey("program", "p", "Type of BLAST program",
            CArgDescriptions::eString);
    arg_desc->SetConstraint
        ("program", &(*new CArgAllow_Strings, 
                "blastp", "blastn", "blastx", "tblastn", "tblastx"));

    // Query sequence
    arg_desc->AddDefaultKey("query", "q", "Query file name",
            CArgDescriptions::eInputFile, "-", CArgDescriptions::fPreOpen);

    // Subject(s) sequence(s)
    arg_desc->AddKey("subject", "s", "Subject(s) file name",
            CArgDescriptions::eInputFile, CArgDescriptions::fPreOpen);

    // File to write Seq-align object
    arg_desc->AddDefaultKey("seqalign", "filename", 
            "File name for writting resulting Seq-align",
            CArgDescriptions::eString, "seqalign.out");

    // Copied from blast_app
    arg_desc->AddDefaultKey("strand", "strand", 
        "Query strands to search: 1 forward, 2 reverse, 0,3 both",
        CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("filter", "filter", "Filtering option",
                            CArgDescriptions::eString, "T");
    arg_desc->AddDefaultKey("lcase", "lcase", "Should lower case be masked?",
                            CArgDescriptions::eBoolean, "F");
    arg_desc->AddDefaultKey("lookup", "lookup", 
        "Type of lookup table: 0 default, 1 megablast",
        CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("matrix", "matrix", "Scoring matrix name",
                            CArgDescriptions::eString, "BLOSUM62");
    arg_desc->AddDefaultKey("mismatch", "penalty", "Penalty score for a mismatch",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("match", "reward", "Reward score for a match",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("word", "wordsize", "Word size",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("templen", "templen", 
        "Discontiguous word template length",
        CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("templtype", "templtype", 
        "Discontiguous word template type",
        CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("thresh", "threshold", 
        "Score threshold for neighboring words",
        CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("window","window", "Window size for two-hit extension",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("ag", "ag", 
        "Should AG method be used for scanning the database?",
        CArgDescriptions::eBoolean, "T");
    arg_desc->AddDefaultKey("varword", "varword", 
        "Should variable word size be used?",
        CArgDescriptions::eBoolean, "F");
    arg_desc->AddDefaultKey("stride","stride", "Database scanning stride",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("xungap", "xungapped", 
        "X-dropoff value for ungapped extensions",
        CArgDescriptions::eDouble, "0");
    arg_desc->AddDefaultKey("ungapped", "ungapped", 
        "Perform only an ungapped alignment search?",
        CArgDescriptions::eBoolean, "F");
    arg_desc->AddDefaultKey("greedy", "greedy", 
        "Use greedy algorithm for gapped extensions?",
        CArgDescriptions::eBoolean, "F");
    arg_desc->AddDefaultKey("gopen", "gapopen", "Penalty for opening a gap",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("gext", "gapext", "Penalty for extending a gap",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("xgap", "xdrop", 
        "X-dropoff value for preliminary gapped extensions",
        CArgDescriptions::eDouble, "0");
    arg_desc->AddDefaultKey("xfinal", "xfinal", 
        "X-dropoff value for final gapped extensions with traceback",
        CArgDescriptions::eDouble, "0");
    arg_desc->AddDefaultKey("evalue", "evalue", 
        "E-value threshold for saving hits",
        CArgDescriptions::eDouble, "0");
    arg_desc->AddDefaultKey("searchsp", "searchsp", 
        "Virtual search space to be used for statistical calculations",
        CArgDescriptions::eDouble, "0");
    arg_desc->AddDefaultKey("perc", "percident", 
        "Percentage of identities cutoff for saving hits",
        CArgDescriptions::eDouble, "0");
    arg_desc->AddDefaultKey("descr", "descriptions",
        "How many matching sequence descriptions to show?",
        CArgDescriptions::eInteger, "500");
    arg_desc->AddDefaultKey("align", "alignments", 
        "How many matching sequence alignments to show?",
        CArgDescriptions::eInteger, "250");
    arg_desc->AddDefaultKey("out", "out", "File name for writing output",
        CArgDescriptions::eOutputFile, "-", CArgDescriptions::fPreOpen);
    arg_desc->AddDefaultKey("format", "format", 
        "How to format the results?",
        CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("html", "html", "Produce HTML output?",
                            CArgDescriptions::eBoolean, "F");
    arg_desc->AddDefaultKey("gencode", "gencode", "Query genetic code",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("dbgencode", "dbgencode", "Database genetic code",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("maxintron", "maxintron", 
                            "Longest allowed intron length for linking HSPs",
                            CArgDescriptions::eInteger, "0");
    arg_desc->AddDefaultKey("frameshift", "frameshift",
                            "Frame shift penalty (blastx only)",
                            CArgDescriptions::eInteger, "0");

    // Debug parameters
    arg_desc->AddFlag("trace", "Tracing enabled?", true);

    SetupArgDescriptions(arg_desc.release());
}

void 
CBlast2seqApplication::InitScope(void)
{
    if (m_Scope.Empty()) {
        m_ObjMgr.Reset(new CObjectManager());
        m_ObjMgr->RegisterDataLoader(*new CGBDataLoader("ID", 0, 2),
                CObjectManager::eDefault);

        m_Scope.Reset(new CScope(*m_ObjMgr));
        m_Scope->AddDefaults();
        _TRACE("Blast2seqApp: Initializing scope");
    }
}

CBlastOption::EProgram
CBlast2seqApplication::GetBlastProgramNum(const string& prog)
{
    if (prog == "blastp")
        return CBlastOption::eBlastp;
    if (prog == "blastn")
        return CBlastOption::eBlastn;
    if (prog == "blastx")
        return CBlastOption::eBlastx;
    if (prog == "tblastn")
        return CBlastOption::eTblastn;
    if (prog == "tblastx")
        return CBlastOption::eTblastx;
    return CBlastOption::eBlastUndef;
}

FILE*
CBlast2seqApplication::GetOutputFilePtr(void)
{
    FILE *retval = NULL;

    if (GetArgs()["out"].AsString() == "-")
        retval = stdout;    
    else
        retval = fopen((char *)GetArgs()["out"].AsString().c_str(), "a");

    _ASSERT(retval);
    return retval;
}

/*****************************************************************************/
// Should go into something like blast_input.cpp
vector< CConstRef<CSeq_loc> >
BLASTGetSeqLocFromStream(CNcbiIstream& in, CRef<CScope>& scope, 
        CSeq_loc* lcase_mask = NULL)
{
    _ASSERT(scope != NULL);
    vector< CConstRef<CSeq_loc> > retval;
    CRef<CSeq_entry> seq_entry;

    if ( !(seq_entry = ReadFasta(in, fReadFasta_AllSeqIds, lcase_mask)))
        throw runtime_error("Could not retrieve seq entry");

    for (CTypeConstIterator<CBioseq> itr(ConstBegin(*seq_entry)); itr; ++itr) {

        CSeq_loc* sl = new CSeq_loc();
        sl->SetWhole(*(const_cast<CSeq_id*>(&*itr->GetId().front())));
        CConstRef<CSeq_loc> seqlocref(sl);
        retval.push_back(seqlocref);

        // Check if this seqentry has been added to the scope already
        CBioseq_Handle bh = scope->GetBioseqHandle(*seqlocref);
        if (!bh)
            scope->AddTopLevelSeqEntry(*seq_entry);
    }

    return retval;
}


/*****************************************************************************/

int CBlast2seqApplication::Run(void)
{
    CStopWatch sw;
    InitScope();
    CArgs args = GetArgs();
    if (args["trace"])
        SetDiagTrace(eDT_Enable);

    CNcbiOstream& out = args["out"].AsOutputFile();

    // Retrieve input sequences
    vector< CConstRef<CSeq_loc> > query_loc =
        BLASTGetSeqLocFromStream(args["query"].AsInputFile(), m_Scope);

    vector< CConstRef<CSeq_loc> > subject_loc =
        BLASTGetSeqLocFromStream(args["subject"].AsInputFile(), m_Scope);

    // Get program name
    CBlastOption::EProgram prog =
        GetBlastProgramNum(args["program"].AsString());

    sw.Start();
    CBl2Seq blaster(query_loc.front(), subject_loc, prog, &*m_Scope);
    CRef<CSeq_align_set> seqalign = blaster.Run();
    double t = sw.Elapsed();
    cerr << "CBl2seq run took " << t << " seconds" << endl;
    if (seqalign->Get().empty()) {
        out << "No hits found" << endl;
    } else {

        DECLARE_ASN_CONVERTER(CSeq_align, SeqAlign, converter);
        SeqAlignPtr salp = NULL, tmp = NULL, tail = NULL;

        ITERATE(list< CRef<CSeq_align> >, itr, seqalign->Get()) {
            tmp = converter.ToC(**itr);

            if (!salp)
                salp = tail = tmp;
            else {
                tail->next = tmp;
                while (tail->next)
                    tail = tail->next;
            }
        }

        if (args["seqalign"]) {
            AsnIoPtr aip = AsnIoOpen(
                    (char*)args["seqalign"].AsString().c_str(), (char*)"w");
            GenericSeqAlignSetAsnWrite(salp, aip);
            AsnIoReset(aip);
            AsnIoClose(aip);
        }

        // Display w/ C formatter
        UseLocalAsnloadDataAndErrMsg();
        if (!SeqEntryLoad()) return 1;
        if (!ID1BioseqFetchEnable(const_cast<char*>("bl2seq"), true)) {
            cerr << "could not initialize entrez sequence retrieval" << endl;
            return 1;
        }

        SeqAnnotPtr seqannot = SeqAnnotNew();
        seqannot->type = 2;
        AddAlignInfoToSeqAnnot(seqannot, (Uint1)prog);
        seqannot->data = salp;

        Uint4 align_opts = TXALIGN_MATRIX_VAL | TXALIGN_SHOW_QS
            | TXALIGN_COMPRESS | TXALIGN_END_NUM;

        FILE *fp = GetOutputFilePtr();
        ShowTextAlignFromAnnot(seqannot, 60, fp, NULL, NULL, align_opts, 
                NULL, NULL, FormatScoreFunc);
        seqannot = SeqAnnotFree(seqannot);

    }

    return 0;
}

void CBlast2seqApplication::Exit(void)
{
    SetDiagStream(0);
}


int main(int argc, const char* argv[])
{
    return CBlast2seqApplication().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 1.1  2003/07/10 18:35:58  camacho
 * Initial revision
 *
 * Revision 1.1  2002/04/18 16:05:09  ucko
 * Add centralized tree for sample apps.
 *
 * Revision 6.4  2002/04/16 18:49:07  ivanov
 * Centralize threatment of assert() in tests.
 * Added #include <test/test_assert.h>. CVS log moved to end of file.
 *
 * Revision 6.3  2001/06/01 15:36:21  vakatov
 * Fixed for the case when "logfile" is not provided in the cmd.line
 *
 * Revision 6.2  2001/06/01 15:17:57  vakatov
 * Workaround a bug in SUN WorkShop 5.1 compiler
 *
 * Revision 6.1  2001/05/31 16:32:51  ivanov
 * Initialization
 *
 * ===========================================================================
 */
