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
 * Author:  Yuri Kapustin
 *
 * File Description: Splign application
 *                   
*/

#include <ncbi_pch.hpp>

#include "splign_app.hpp"
#include "splign_app_exception.hpp"

#include <corelib/ncbistd.hpp>
#include <corelib/ncbi_system.hpp>

#include <serial/objostrasn.hpp>
#include <serial/serial.hpp>

#include <algo/align/nw/nw_spliced_aligner16.hpp>
#include <algo/align/splign/splign_cmdargs.hpp>
#include <algo/align/util/hit_comparator.hpp>

#include <algo/blast/api/bl2seq.hpp>
#include <algo/blast/api/local_blast.hpp>
#include <algo/blast/api/objmgr_query_data.hpp>
#include <algo/blast/api/seqsrc_seqdb.hpp>

#include <objmgr/object_manager.hpp>
#include <objmgr/seq_vector.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seqloc/Seq_loc.hpp>

#include <objtools/readers/fasta.hpp>
#include <objtools/readers/reader_exception.hpp>
#include <objtools/lds/lds_admin.hpp>
#include <objtools/data_loaders/lds/lds_dataloader.hpp>
#include <objtools/data_loaders/blastdb/bdbloader.hpp>

#include <algorithm>
#include <memory>


namespace {
    const char kDirSense[]     = "sense";
    const char kDirAntisense[] = "antisense";
    const char kDirBoth[]      = "both";
    const char kDirAuto[]      = "auto";
}


BEGIN_NCBI_SCOPE

void CSplignApp::Init()
{
    HideStdArgs( fHideLogfile | fHideConffile | fHideVersion);

    SetVersion(CVersionInfo(1, 25, 0, "Splign"));  
    auto_ptr<CArgDescriptions> argdescr(new CArgDescriptions);

    string program_name ("Splign v.1.25");

#ifdef GENOME_PIPELINE
    program_name += 'p';
#endif

    argdescr->SetUsageContext(GetArguments().GetProgramName(), program_name);
    
    argdescr->AddOptionalKey
        ("hits", "hits",
         "[Batch mode] Externally computed input blast hits. "
         "This file defines the set of sequences to align and "
         "is also used to guide alignments. "
         "The file must be collated by subject and query "
         "(e.g. sort -k 2,2 -k 1,1).",
         CArgDescriptions::eInputFile);
   
#ifdef GENOME_PIPELINE
    argdescr->AddOptionalKey
        ("comps", "comps",
         "[Batch mode] Externally computed input blast compartments "
         "specified in blast tabular format with one empty line as separator. "
         "No built-in compartmentization will occur. "
         "The hits must be collated by subject and query.",
         CArgDescriptions::eInputFile);
#endif

    argdescr->AddOptionalKey
        ("mklds", "mklds",
         "[Batch mode] "
         "Make LDS DB under the specified directory "
         "with cDNA and genomic FASTA files or symlinks.",
         CArgDescriptions::eString);

    argdescr->AddOptionalKey
        ("ldsdir", "ldsdir",
         "[Batch mode] Directory holding LDS subdirectory.",
         CArgDescriptions::eString);

    argdescr->AddOptionalKey
        ("query", "query",
         "[Pairwise mode] FASTA file with the spliced sequence. "
         "Must be used with -subj.",
         CArgDescriptions::eInputFile);
    
    argdescr->AddOptionalKey
        ("subj", "subj",
         "[Pairwise mode] FASTA file with the genomic sequence(s). "
         "Must be used with -query. ",
         CArgDescriptions::eInputFile);
    
    argdescr->AddDefaultKey
        ("W", "mbwordsize", "[Pairwise mode] Megablast word size",
         CArgDescriptions::eInteger,
         "28");

    CSplignArgUtil::SetupArgDescriptions(argdescr.get());

#ifdef GENOME_PIPELINE

    argdescr->AddOptionalKey
        ("querydb", "querydb",
         "[Batch mode - no external hits] "
         "Pathname to the blast database of query (cDNA) "
         "sequences. To create one, use formatdb -o T -p F",
         CArgDescriptions::eString);

    argdescr->AddOptionalKey
        ("subjdb", "subjdb",
         "[Incremental mode - no external hits] "
         "Pathname to the blast database of subject "
         "sequences. To create one, use formatdb -o T -p F",
         CArgDescriptions::eString);

#endif
    
    argdescr->AddDefaultKey
        ("log", "log", "Splign log file",
         CArgDescriptions::eOutputFile,
         "splign.log");
    
    argdescr->AddOptionalKey
        ("asn", "asn", "ASN.1 output file name", 
         CArgDescriptions::eOutputFile);

    argdescr->AddOptionalKey
        ("aln", "aln", "Pairwise alignment output file name", 
         CArgDescriptions::eOutputFile);
    
    argdescr->AddFlag("cross",
                      "[Pairwise mode] Use discontiguous megablast to facilitate "
                      "alignment of more divergent sequences such as those "
                      "from different organisms (cross-species alignment).");

    argdescr->AddDefaultKey
        ("direction", 
         "direction", 
         "Query sequence orientation", 
         CArgDescriptions::eString,
         kDirSense);
    
    CArgAllow_Strings* constrain_direction = new CArgAllow_Strings;
    constrain_direction->Allow(kDirSense)->Allow(kDirAntisense)
        ->Allow(kDirBoth)->Allow(kDirAuto);
    
    argdescr->SetConstraint("direction", constrain_direction);

    SetupArgDescriptions(argdescr.release());
}


CSplign::THitRef CSplignApp::s_ReadBlastHit(const string& m8)
{
    THitRef rv (new CBlastTabular(m8.c_str()));

#ifdef SPLIGNAPP_UNDECORATED_ARE_LOCALS
    // make seq-id local if no type specified in the original m8
    string::const_iterator ie = m8.end(), i0 = m8.begin(), i1 = i0;
    while(i1 != ie && *i1 !='\t') ++i1;
    if(i1 != ie) {
        string::const_iterator i2 = ++i1;
        while(i2 != ie && *i2 !='\t') ++i2;
        if(i2 != ie) {
            if(find(i0, i1, '|') == i1) {
                const string strid = rv->GetQueryId()->GetSeqIdString(true);
                CRef<CSeq_id> seqid (new CSeq_id(CSeq_id::e_Local, strid));
                rv->SetQueryId(seqid);
            }
            if(find(i1, i2, '|') == i2) {
                const string strid = rv->GetSubjId()->GetSeqIdString(true);
                CRef<CSeq_id> seqid (new CSeq_id(CSeq_id::e_Local, strid));
                rv->SetSubjId(seqid);
            }
            return rv;
        }
    }
    const string errmsg = string("Incorrectly formatted blast hit:\n") + m8;
    NCBI_THROW(CSplignAppException, eBadData, errmsg);
#else
    return rv;
#endif
}


bool CSplignApp::x_GetNextPair(const THitRefs& hitrefs, THitRefs* hitrefs_pair)
{
    USING_SCOPE(objects);
    
    hitrefs_pair->resize(0);
    
    const size_t dim = hitrefs.size();
    if(dim == 0) {
        return false;
    }
    
    if(m_CurHitRef == dim) {
        m_CurHitRef = numeric_limits<size_t>::max();
        return false;
    }
 
    if(m_CurHitRef == numeric_limits<size_t>::max()) {
        m_CurHitRef = 0;
    }
    
    CConstRef<CSeq_id> query (hitrefs[m_CurHitRef]->GetQueryId());
    CConstRef<CSeq_id> subj  (hitrefs[m_CurHitRef]->GetSubjId());
    while(m_CurHitRef < dim 
          && hitrefs[m_CurHitRef]->GetQueryId()->Match(*query)
           && hitrefs[m_CurHitRef]->GetSubjId()->Match(*subj)  ) 
    {
        hitrefs_pair->push_back(hitrefs[m_CurHitRef++]);
    }
    return true;
}


bool CSplignApp::x_GetNextPair(istream& ifs, THitRefs* hitrefs)
{
    hitrefs->resize(0);

    if(!m_PendingHits.size() && !ifs ) {
        return false;
    }
    
    if(!m_PendingHits.size()) {

        THit::TId query, subj;

        if(m_firstline.size()) {

            THitRef hitref (s_ReadBlastHit(m_firstline));
            query = hitref->GetQueryId();
            subj  = hitref->GetSubjId();
            m_PendingHits.push_back(hitref);
        }

        char buf [1024];
        while(ifs) {

            buf[0] = 0;
            CT_POS_TYPE pos0 = ifs.tellg();
            ifs.getline(buf, sizeof buf, '\n');
            CT_POS_TYPE pos1 = ifs.tellg();
            if(pos1 == pos0) break; // GCC hack
            if(buf[0] == '#') continue; // skip comments
            const char* p = buf; // skip leading spaces
            while(*p == ' ' || *p == '\t') ++p;
            if(*p == 0) continue; // skip empty lines
            
            THitRef hit (s_ReadBlastHit(p));
            if(query.IsNull()) {
                query = hit->GetQueryId();
            }
            if(subj.IsNull()) {
                subj = hit->GetSubjId();
            }
            if(hit->GetQueryStrand() == false) {
                hit->FlipStrands();
            }
            if(hit->GetSubjStop() == hit->GetSubjStart()) {
                // skip single bases
                continue;
            }
            
            if(hit->GetQueryId()->Match(*query) == false || 
               hit->GetSubjId()->Match(*subj) == false) {

                m_firstline = p;
                break;
            }
            
            m_PendingHits.push_back(hit);
        }
    }

    const size_t pending_size = m_PendingHits.size();
    if(pending_size) {

        THit::TId query = m_PendingHits[0]->GetQueryId();
        THit::TId subj  = m_PendingHits[0]->GetSubjId();
        size_t i = 1;
        for(; i < pending_size; ++i) {

            THitRef h = m_PendingHits[i];
            if(h->GetQueryId()->Match(*query) == false || 
               h->GetSubjId()->Match(*subj) == false) {
                break;
            }
        }
        hitrefs->resize(i);
        copy(m_PendingHits.begin(), m_PendingHits.begin() + i, 
             hitrefs->begin());
        m_PendingHits.erase(m_PendingHits.begin(), m_PendingHits.begin() + i);
    }
    
    return hitrefs->size() > 0;
}


bool CSplignApp::x_GetNextComp(istream& ifs, THitRefs* hitrefs,
                               THit::TCoord* psubj_min, THit::TCoord* psubj_max)
{
    hitrefs->resize(0);

    if(!ifs) {
        return false;
    }

    THit::TCoord & smin = *psubj_min;
    THit::TCoord & smax = *psubj_max;

    smin = 0;
    smax = numeric_limits<THit::TCoord>::max();

    static THit::TId query, subj;
    static bool strand (true);

    static THit::TCoord last_coord (0);

    if(m_firstline.size()) {

        THitRef hit (s_ReadBlastHit(m_firstline));

        if(last_coord) {

            // no change in query, subj and strand
            if(strand) {
                smin = last_coord;
                smax = hit->GetSubjStop();
            }
            else {
                smax = last_coord;
                smin = hit->GetSubjStop();
            }
            last_coord = 0;
        }
        else {
            
            // at least one <q,s,s> component has changed
            query  = hit->GetQueryId();
            subj   = hit->GetSubjId();
            strand = hit->GetSubjStrand();

            if(strand) {
                smin = 0;
                smax = hit->GetSubjStop();
            }
            else {
                smin = hit->GetSubjStop();
                smax = numeric_limits<THit::TCoord>::max();
            }
        }

        hitrefs->push_back(hit);

        m_firstline.resize(0);
    }
    
    char buf [1024];
    bool first_line_only (false);
    while(ifs) {

        buf[0] = 0;
        CT_POS_TYPE pos0 = ifs.tellg();
        ifs.getline(buf, sizeof buf, '\n');
        CT_POS_TYPE pos1 = ifs.tellg();
        if(pos1 == pos0) break; // GCC hack
        if(buf[0] == '#') continue; // skip comments
        const char* p = buf; // skip leading spaces
        while(*p == ' ' || *p == '\t') ++p;
        if(*p == 0) {
            first_line_only = true;
            continue;
        }

        THitRef hit (s_ReadBlastHit(p));

        const THit::TId curr_query (hit->GetQueryId());
        const THit::TId curr_subj (hit->GetSubjId());
        if(query.IsNull()) {
            query = curr_query;
            subj  = curr_subj;
            strand = hit->GetSubjStrand();
        }
        
        const bool new_triple (hit->GetSubjStrand() != strand ||
                               curr_query->Match(*query) == false ||
                               curr_subj->Match(*subj) == false);

        if(first_line_only) {
            
            if(new_triple) {
                if(strand) {
                    smax = numeric_limits<THit::TCoord>::max();
                }
                else {
                    smin = 0;
                }
                last_coord = 0;
            }
            else {
                if(strand) {
                    last_coord = smax;
                    smax = hit->GetSubjStart();
                }
                else {
                    last_coord = smin;
                    smin = hit->GetSubjStart();
                }
            }

            m_firstline = p;
            break;
        }

        if(new_triple) {

            // reset smin, smax
            query  = hit->GetQueryId();
            subj   = hit->GetSubjId();
            strand = hit->GetSubjStrand();

            if(strand) {
                smin = 0;
                smax = hit->GetSubjStop();
            }
            else {
                smin = hit->GetSubjStop();
                smax = numeric_limits<THit::TCoord>::max();
            }
        }
        else {

            if(strand) {
                smax = hit->GetSubjStop();
            }
            else {
                smin = hit->GetSubjStop();
            }
        }

        hitrefs->push_back(hit);

    } // while(ifs)

    if(!ifs && last_coord == 0) {
        if(strand) {
            smax = numeric_limits<THit::TCoord>::max();
        }
        else {
            smin = 0;
        }
    }

    return hitrefs->size() > 0;
}


void CSplignApp::x_LogStatus(size_t model_id, 
                             bool query_strand,
                             const THit::TId& query, 
                             const THit::TId& subj, 
			     bool error, 
                             const string& msg)
{
    string error_tag (error? "Error: ": "");
    if(model_id == 0) {
        *m_logstream << '-';
    }
    else {
        *m_logstream << (query_strand? '+': '-') << model_id;
    }
    
    *m_logstream << '\t' << query->GetSeqIdString(true) 
                 << '\t' << subj->GetSeqIdString(true)
                 << '\t' << error_tag << msg << endl;
}


CRef<blast::CBlastOptionsHandle> 
CSplignApp::x_SetupBlastOptions(bool cross)
{
    USING_SCOPE(blast);

    m_BlastProgram = cross? eDiscMegablast: eMegablast;

    CRef<CBlastOptionsHandle> blast_options_handle
        (CBlastOptionsFactory::Create(m_BlastProgram));

    blast_options_handle->SetDefaults();

    CBlastOptions& blast_opt = blast_options_handle->SetOptions();

    if(!cross) {

        const CArgs& args = GetArgs();
        blast_opt.SetWordSize(args["W"].AsInteger());
        blast_opt.SetMaskAtHash(true);
        blast_opt.SetDustFiltering(false);
    }

    if(blast_options_handle->Validate() == false) {
        NCBI_THROW(CSplignAppException,
                   eInternal,
                   "Incorrect blast setup");
    }

    return blast_options_handle;
}


enum ERunMode {
    eNotSet,
    ePairwise, // single query vs single subj
    eBatch1,   // use external raw blast hits
    eBatch2,   // use pre-computed compartments
    eBatch3,   // run blast internally using external blast db of queries
    eIncremental // run blast internally using external blastdb of subjects
};


const string kSplignLdsDb ("splign.ldsdb");

string GetLdsDbDir(const string& fasta_dir)
{
    string lds_db_dir = fasta_dir;
    const char sep = CDirEntry::GetPathSeparator();
    const size_t fds = fasta_dir.size();
    if(fds > 0 && fasta_dir[fds-1] != sep) {
        lds_db_dir += sep;
    }
    lds_db_dir += "_SplignLDS_";
    return lds_db_dir;
}


void CSplignApp::x_GetDbBlastHits(const string& dbname,
                                  blast::TSeqLocVector& queries,
                                  THitRefs* phitrefs,
                                  size_t chunk, size_t total_chunks)
{
    USING_SCOPE(objects);
    USING_SCOPE(blast);

    CRef<IQueryFactory> query_factory(new CObjMgr_QueryFactory(queries));
    CSeqDB seqdb (dbname, CSeqDB::eNucleotide);
    CBlastSeqSrc seq_src(SeqDbBlastSeqSrcInit(&seqdb));
    CLocalBlast blast (query_factory, m_BlastOptionsHandle, seq_src);
    CSearchResultSet results (*blast.Run());
    phitrefs->resize(0);

    const size_t num_results (results.GetNumResults());
    for(size_t query_index (0); query_index != num_results; ++query_index) {
        
        CConstRef<objects::CSeq_align_set> ref_sas0 (results[query_index].
                                                     GetSeqAlign());
        if(ref_sas0->IsSet()) {
            
            const CSeq_align_set::Tdata & sas0 (ref_sas0->Get());
            ITERATE(CSeq_align_set::Tdata, sa_iter, sas0) {
                
                THitRef hitref (new CBlastTabular(**sa_iter, false));
                phitrefs->push_back(hitref);
                
                THit::TId id (hitref->GetSubjId());
                int oid (-1);
                seqdb.SeqidToOid(*id, oid);
                id = seqdb.GetSeqIDs(oid).back();
                hitref->SetSubjId(id);
            }
        }
    }

//    BlastSeqSrcFree(seq_src);
}


void CSplignApp::x_DoIncremental(void)
{
    USING_SCOPE(objects);
    USING_SCOPE(blast);

    const CArgs& args = GetArgs();
    const string dbname = args["subjdb"].AsString();

    CRef<CObjectManager> objmgr (CObjectManager::GetInstance());
    CBlastDbDataLoader::RegisterInObjectManager(
        *objmgr, dbname, CBlastDbDataLoader::eNucleotide, 
        CObjectManager::eDefault);

    CRef<ILineReader> line_reader;
    const CArgValue& argval (args["query"]);
    try {
        line_reader.Reset(
            new CMemoryLineReader(new CMemoryFile(argval.AsString()),
                                  eTakeOwnership));
    }
    
    catch (...) { // fall back to streams
        line_reader.Reset(new CStreamLineReader(argval.AsInputFile()));
    }

    try {
        CFastaReader fasta_reader(* line_reader, CFastaReader::fAssumeNuc);
        for(CRef<CSeq_entry> se (fasta_reader.ReadOneSeq());
            se.NotEmpty(); se = fasta_reader.ReadOneSeq()) 
        {
            CRef<CScope> scope (new CScope(*objmgr));
            scope->AddDefaults();
            scope->AddTopLevelSeqEntry(*se);

            m_Splign->SetScope() = scope;

            const CSeq_entry::TSeq& bioseq = se->GetSeq();    
            const CSeq_entry::TSeq::TId& qids = bioseq.GetId();
            CRef<CSeq_id> seqid_query (qids.back());

            TSeqLocVector queries;
            CRef<CSeq_loc> sl (new CSeq_loc);
            sl->SetWhole().Assign(*seqid_query);
            queries.push_back(SSeqLoc(*sl, *scope));
            
            THitRefs hitrefs;
            x_GetDbBlastHits(dbname, queries, &hitrefs, 0, 0);
            typedef CHitComparator<CBlastTabular> THitComparator;
            THitComparator hc (THitComparator::eSubjIdQueryId);
            stable_sort(hitrefs.begin(), hitrefs.end(), hc);

            THitRefs hitrefs_pair;
            m_CurHitRef = numeric_limits<size_t>::max();
            while(x_GetNextPair(hitrefs, &hitrefs_pair)) {
                x_ProcessPair(hitrefs_pair, args); 
            }
        }
    }

    catch (CObjReaderParseException& e) {
        if(e.GetErrCode() != CObjReaderParseException::eEOF) {
            throw;
        }
    }
}


void CSplignApp::x_DoBatch2(void)
{
    USING_SCOPE(objects);
    USING_SCOPE(blast);

    const CArgs& args = GetArgs();
    const string dbname = args["querydb"].AsString();
    const size_t W = args["W"].AsInteger();
    
    CRef<CObjectManager> objmgr (CObjectManager::GetInstance());
    CBlastDbDataLoader::RegisterInObjectManager(
        *objmgr, dbname, CBlastDbDataLoader::eNucleotide, 
        CObjectManager::eDefault);

    CRef<ILineReader> line_reader;
    try {
        line_reader.Reset(
                     new CMemoryLineReader(new CMemoryFile(args["subj"].AsString()),
                                           eTakeOwnership));
    } catch (...) { // fall back to streams
        line_reader.Reset(new CStreamLineReader(args["subj"].AsInputFile()));
    }
    CFastaReader fasta_reader(* line_reader, CFastaReader::fAssumeNuc);
    for(CRef<CSeq_entry> se (fasta_reader.ReadOneSeq());
        se.NotEmpty(); se = fasta_reader.ReadOneSeq()) 
    {
        CRef<CScope> scope (new CScope(*objmgr));
        scope->AddDefaults();
        scope->AddTopLevelSeqEntry(*se);
        m_Splign->SetScope() = scope;

        const CSeq_entry::TSeq& bioseq = se->GetSeq();    
        const CSeq_entry::TSeq::TId& sids = bioseq.GetId();
        CRef<CSeq_id> seqid_dna (sids.back());
        if(bioseq.IsNa() == false) {
            string errmsg;
            errmsg = "Subject sequence not nucleotide: " 
                + seqid_dna->GetSeqIdString(true);
            NCBI_THROW(CSplignAppException, eBadData, errmsg);
        }

        TSeqPos offset = 0, len = bioseq.GetInst().GetLength();
        const TSeqPos step1m = 1024*1024;
        const TSeqPos step500k = 500*1024;

        THitRefs pending;
        while(offset < len || pending.size() > 0) {

            TSeqPos from = offset, to = offset + step500k;

            THitRefs hitrefs;
            if(from < len) {

                if(to >= len) {
                    to = len - 1;
                }

                CRef<CSeq_loc> sl (new CSeq_loc (*seqid_dna, from, to));
                TSeqLocVector queries;
                queries.push_back(SSeqLoc(*sl, *scope));

                x_GetDbBlastHits(dbname, queries, &hitrefs, 0, 0);
                NON_CONST_ITERATE(THitRefs, ii, hitrefs) {
                
                    THitRef h = *ii;
                    h->SwapQS();
                    if(h->GetQueryStrand() == false) {
                        h->FlipStrands();
                    }
                }
                copy(pending.begin(), pending.end(), back_inserter(hitrefs));
                pending.resize(0);
                
                set<string> pending_ids;
                NON_CONST_ITERATE(THitRefs, ii, hitrefs) {
                    
                    const string id = (*ii)->GetQueryId()->AsFastaString();
                    bool bp = pending_ids.find(id) != pending_ids.end();
                    if(bp || (*ii)->GetSubjMin() + step1m > to) {
                        pending.push_back(*ii);
                        ii->Reset(NULL);
                        if(!bp) {
                            pending_ids.insert(id);
                        }
                    }
                }

                NON_CONST_ITERATE(THitRefs, ii, hitrefs) {
                    if(ii->NotNull()) {
                        const string id = (*ii)->GetQueryId()->AsFastaString();
                        if(pending_ids.find(id) != pending_ids.end()) {
                            pending.push_back(*ii);
                            ii->Reset(NULL);
                        }
                    }
                }

                size_t i = 0, n = hitrefs.size();
                for(size_t j = 0; j < n; ++j) {
                    if(hitrefs[j].NotNull()) {
                        if(i < j) {
                            hitrefs[i++] = hitrefs[j];
                        }
                        else {
                            ++i;
                        }
                    }
                }
                if(i < n) {
                    hitrefs.erase(hitrefs.begin() + i, hitrefs.end());
                }

            }
            else {
                hitrefs = pending;
                pending.resize(0);
            }

            typedef CHitComparator<CBlastTabular> THitComparator;
            THitComparator hc (THitComparator::eQueryId);
            stable_sort(hitrefs.begin(), hitrefs.end(), hc);            

            THitRefs hitrefs_pair;
            m_CurHitRef = numeric_limits<size_t>::max();
            while(x_GetNextPair(hitrefs, &hitrefs_pair)) {
                
                x_ProcessPair(hitrefs_pair, args); 
            }

            offset = (to + 1 >= len)? len: (to - 2*W);
        }
    }
}



CRef<objects::CSeq_id> CSplignApp::x_ReadFastaSetId(const CArgValue& argval,
                                                    CRef<objects::CScope> scope)
{
    USING_SCOPE(objects);

    CRef<ILineReader> line_reader;
    try {
        line_reader.Reset(
            new CMemoryLineReader(new CMemoryFile(argval.AsString()),
                                  eTakeOwnership));
    } catch (...) { // fall back to streams
        line_reader.Reset(new CStreamLineReader(argval.AsInputFile()));
    }
    CFastaReader fasta_reader(* line_reader,
                              CFastaReader::fAssumeNuc | CFastaReader::fOneSeq);
    CRef<CSeq_entry> se (fasta_reader.ReadOneSeq());

    scope->AddTopLevelSeqEntry(*se);
    const CSeq_entry::TSeq& bioseq = se->GetSeq();    
    const CSeq_entry::TSeq::TId& seqid = bioseq.GetId();
    return seqid.back();
}


int CSplignApp::Run()
{ 
    USING_SCOPE(objects);

    const CArgs& args = GetArgs();    

    // check that modes aren't mixed

    const bool is_mklds   = args["mklds"];
    const bool is_ldsdir  = args["ldsdir"];

    const bool is_hits    = args["hits"];
    const bool is_query   = args["query"];
    const bool is_subj    = args["subj"];

#ifdef GENOME_PIPELINE
    const bool is_comps   = args["comps"];
    const bool is_querydb = args["querydb"];
    const bool is_subjdb  = args["subjdb"];
#else
    const bool is_comps (false);
    const bool is_querydb = false;
    const bool is_subjdb = false;
#endif

    const bool is_cross   = args["cross"];

    if(is_mklds) {

        // create LDS DB and exit
        string fa_dir = args["mklds"].AsString();
        if(CDirEntry::IsAbsolutePath(fa_dir) == false) {
            string curdir = CDir::GetCwd();
            const char sep = CDirEntry::GetPathSeparator();            
            const size_t curdirsize = curdir.size();
            if(curdirsize && curdir[curdirsize-1] != sep) {
                curdir += sep;
            }
            fa_dir = curdir + fa_dir;
        }
        const string lds_db_dir = GetLdsDbDir(fa_dir);
        CLDS_Database ldsdb (lds_db_dir, kSplignLdsDb, kSplignLdsDb);
        CLDS_Management ldsmgt (ldsdb);
        ldsmgt.Create();
        ldsmgt.SyncWithDir(fa_dir,
                           CLDS_Management::eRecurseSubDirs,
                           CLDS_Management::eNoControlSum);
        return 0;
    }

    // determine mode and verify arguments
    ERunMode run_mode (eNotSet);
    
    if(is_query && is_subj && !(is_hits || is_comps || is_querydb || is_ldsdir)) {
        run_mode = ePairwise;
    }
    else if(is_subj && is_querydb
            && !(is_query || is_hits || is_comps || is_ldsdir)) 
    {
        run_mode = eBatch3;
    }
    else if(is_query && is_subjdb && !(is_subj || is_hits || is_comps || is_ldsdir)) {
        run_mode = eIncremental;
    }
    else if(is_hits && is_ldsdir && !(is_comps ||is_query || is_subj || is_querydb)) {
        run_mode = eBatch1;
    }
    else if(is_comps && is_ldsdir && !(is_hits ||is_query || is_subj || is_querydb)) {
        run_mode = eBatch2;
    }

    if(run_mode == eNotSet) {
        NCBI_THROW(CSplignAppException,
                   eBadParameter,
                   "Incomplete or inconsistent set of input parameters." );
    }   

    // open log stream
    m_logstream = & args["log"].AsOutputFile();
    
    // open asn output stream, if any
    m_AsnOut = args["asn"]? & args["asn"].AsOutputFile(): NULL;
    
    // open paiwise alignment output stream, if any
    m_AlnOut = args["aln"]? & args["aln"].AsOutputFile(): NULL;
    
    // in pairwise, batch 2 or incremental mode, setup blast options
    if(run_mode != eBatch1 && run_mode != eBatch2) {
        m_BlastOptionsHandle = x_SetupBlastOptions(is_cross);
    }

    // splign and formatter setup    
    m_Splign.Reset(new CSplign);
    CSplignArgUtil::ArgsToSplign(m_Splign, args);

    m_Splign->SetStartModelId(1);

    // splign formatter object    
    m_Formatter.Reset(new CSplignFormatter(*m_Splign));

    // do mode-specific preparations
    CRef<CObjectManager> objmgr = CObjectManager::GetInstance();
    CRef<CScope> scope;
    CRef<CSeq_id> seqid_query, seqid_subj;
    if(run_mode == ePairwise) {

        scope.Reset (new CScope(*objmgr));
        scope->AddDefaults();
        seqid_query = x_ReadFastaSetId(args["query"], scope);
        seqid_subj  = x_ReadFastaSetId(args["subj"] , scope);
    }
    else if(run_mode == eBatch1 || run_mode == eBatch2) {
        
        const string fasta_dir = args["ldsdir"].AsString();
        const string ldsdb_dir = GetLdsDbDir(fasta_dir);
        CLDS_Database* ldsdb (
              new CLDS_Database(ldsdb_dir, kSplignLdsDb, kSplignLdsDb));
        m_LDS_db.reset(ldsdb);
        m_LDS_db->Open();
        CLDS_DataLoader::RegisterInObjectManager(
            *objmgr, *ldsdb, CObjectManager::eDefault);
        scope.Reset (new CScope(*objmgr));
        scope->AddDefaults();
    }
    else if(run_mode == eIncremental) {
        x_DoIncremental();
    }
    else if(run_mode == eBatch3) {
        x_DoBatch2();
    }
    else {
        NCBI_THROW(CSplignAppException,
                   eGeneral,
                   "Requested mode not implemented." );
    }

    m_Splign->SetScope() = scope;

    // run splign in selected mode 
    if(run_mode == ePairwise) {

        THitRefs hitrefs;
        x_GetBl2SeqHits(seqid_query, seqid_subj, scope, &hitrefs);
        x_ProcessPair(hitrefs, args);
    }
    else if (run_mode == eBatch1) {

        THitRefs hitrefs;
        CNcbiIstream& hit_stream = args["hits"].AsInputFile();
        while(x_GetNextPair(hit_stream, &hitrefs) ) {
            x_ProcessPair(hitrefs, args);
        }
    }
    else if (run_mode == eBatch2) {

        CNcbiIstream& hit_stream = args["comps"].AsInputFile();
        THitRefs hitrefs;
        THit::TCoord subj_min, subj_max;

        while(x_GetNextComp(hit_stream, &hitrefs, &subj_min, &subj_max) ) {
            x_ProcessPair(hitrefs, args, subj_min, subj_max);
        }
    }
    else if (run_mode == eIncremental || run_mode == eBatch3) {
        // done at the preparation step
    }
    else {
        NCBI_THROW(CSplignAppException,
                   eInternal,
                   "Mode not implemented");
    }
        
    return 0;
}


void CSplignApp::x_GetBl2SeqHits(
    CRef<objects::CSeq_id> seqid_query,  
    CRef<objects::CSeq_id> seqid_subj, 
    CRef<objects::CScope>  scope,  
    THitRefs* phitrefs)
{
    USING_SCOPE(blast);
    USING_SCOPE(objects);

    phitrefs->resize(0);
    phitrefs->reserve(100);

    CRef<CSeq_loc> seqloc_query (new CSeq_loc);
    seqloc_query->SetWhole().Assign(*seqid_query);
    CRef<CSeq_loc> seqloc_subj (new CSeq_loc);
    seqloc_subj->SetWhole().Assign(*seqid_subj);

    CBl2Seq Blast( SSeqLoc(seqloc_query.GetNonNullPointer(),
                           scope.GetNonNullPointer()),
                   SSeqLoc(seqloc_subj.GetNonNullPointer(),
                           scope.GetNonNullPointer()),
                   m_BlastProgram);
    
    Blast.SetOptionsHandle() = *m_BlastOptionsHandle;

    TSeqAlignVector blast_output (Blast.Run());

    ITERATE(TSeqAlignVector, ii, blast_output) {
        if((*ii)->IsSet()) {
            const CSeq_align_set::Tdata &sas0 = (*ii)->Get();
            ITERATE(CSeq_align_set::Tdata, sa_iter, sas0) {
                    CRef<CBlastTabular> hitref (new CBlastTabular(**sa_iter));
                    if(hitref->GetQueryStrand() == false) {
                        hitref->FlipStrands();
                    }
                    phitrefs->push_back(hitref);
            }
        }
    }
}

void CSplignApp::x_RunSplign(bool raw_hits, THitRefs* phitrefs, 
                             THit::TCoord smin, THit::TCoord smax,
                             CSplign::TResults * psplign_results)
{
    if(raw_hits) {
        m_Splign->Run(phitrefs);
        const CSplign::TResults& results (m_Splign->GetResult());
        copy(results.begin(), results.end(), back_inserter(*psplign_results));
    }
    else {
        CSplign::SAlignedCompartment ac;
        m_Splign->AlignSingleCompartment(phitrefs, smin, smax, &ac);
        psplign_results->push_back(ac);
    }
}


void CSplignApp::x_ProcessPair(THitRefs& hitrefs, const CArgs& args,
                               THit::TCoord smin, THit::TCoord smax)
{

#ifdef GENOME_PIPELINE
    const CSplignFormatter::EFlags flags (CSplignFormatter::fNone);
    const bool raw_hits (! args["comps"]);
#else
    const CSplignFormatter::EFlags flags (CSplignFormatter::fNoExonScores);
    const bool raw_hits (true);
#endif

    if(hitrefs.size() == 0) {
        return;
    }

    // skip void compartments but obey their bounds
    if(hitrefs.front()->GetScore() < 0) {
        return;
    }

    THit::TId query (hitrefs.front()->GetQueryId());
    THit::TId subj  (hitrefs.front()->GetSubjId());
    
    m_Formatter->SetSeqIds(query, subj);

    const string strand (args["direction"].AsString());
    CSplign::TResults splign_results;

    if(strand == kDirSense) {

        m_Splign->SetStrand(true);
        x_RunSplign(raw_hits, &hitrefs, smin, smax, &splign_results);
    }
    else if(strand == kDirAntisense) {
            
        m_Splign->SetStrand(false);
        x_RunSplign(raw_hits, &hitrefs, smin, smax, &splign_results);
    }
    else if(strand == kDirBoth) {

        // save original hits
        THitRefs hits0;
        ITERATE(THitRefs, ii, hitrefs) {
            const THitRef & h0 (*ii);
            THitRef h1 (new THit (*h0));
            hits0.push_back(h1);
        }

        static size_t mid (1);
        size_t mid_plus, mid_minus;
        {{
            m_Splign->SetStrand(true);
            m_Splign->SetStartModelId(mid);
            x_RunSplign(raw_hits, &hitrefs, smin, smax, &splign_results);
            mid_plus = m_Splign->GetNextModelId();
        }}
        {{
            m_Splign->SetStrand(false);
            m_Splign->SetStartModelId(mid);
            x_RunSplign(raw_hits, &hits0, smin, smax, &splign_results);
            mid_minus = m_Splign->GetNextModelId();
        }}
        mid = max(mid_plus, mid_minus);
    }
    else {
        NCBI_THROW(CSplignAppException, eGeneral, 
                   "Auto strand not yet implemented");
    }
    
    cout << m_Formatter->AsExonTable(&splign_results, flags);
        
    if(m_AsnOut) {
        
        *m_AsnOut << MSerial_AsnText 
                  << *(m_Formatter->AsSeqAlignSet(&splign_results))
                  << endl;
    }
    
    if(m_AlnOut) {
        
        *m_AlnOut << m_Formatter->AsAlignmentText(m_Splign->GetScope(),
                                                  &splign_results);
    }
        
    ITERATE(CSplign::TResults, ii, splign_results) {
        x_LogStatus(ii->m_id, ii->m_QueryStrand, query, subj,
                    ii->m_error, ii->m_msg);
    }
    
    if(splign_results.size() == 0) {
        //x_LogStatus(0, true, query, subj, false, "No compartment found");
    }
}


END_NCBI_SCOPE

/////////////////////////////////////

USING_NCBI_SCOPE;

#ifdef GENOME_PIPELINE

// make old-style Splign index file
void MakeIDX( istream* inp_istr, const size_t file_index, ostream* out_ostr )
{
    istream * inp = inp_istr? inp_istr: &cin;
    ostream * out = out_ostr? out_ostr: &cout;
    inp->unsetf(IOS_BASE::skipws);
    char c0 = '\n', c;
    while(inp->good()) {
        c = inp->get();
        if(c0 == '\n' && c == '>') {
            CT_OFF_TYPE pos = inp->tellg() - CT_POS_TYPE(1);
            string s;
            *inp >> s;
            *out << s << '\t' << file_index << '\t' << pos << endl;
        }
        c0 = c;
    }
}
#endif

int main(int argc, const char* argv[]) 
{

#ifdef GENOME_PIPELINE

    // pre-scan for mkidx0
    for(int i = 1; i < argc; ++i) {
        if(0 == strcmp(argv[i], "-mkidx0")) {
            
            if(i + 1 == argc) {
                char err_msg [] = 
                    "ERROR: No FastA files specified to index. "
                    "Please specify one or more FastA files after -mkidx0. "
                    "Your system may support wildcards "
                    "to specify multiple files.";
                cerr << err_msg << endl;
                return 1;
            }
            else {
                ++i;
            }
            vector<string> fasta_filenames;
            for(; i < argc; ++i) {
                fasta_filenames.push_back(argv[i]);
                // test
                ifstream ifs (argv[i]);
                if(ifs.fail()) {
                    cerr << "ERROR: Unable to open " << argv[i] << endl;
                    return 1;
                }
            }
            
            // write the list of files
            const size_t files_count = fasta_filenames.size();
            cout << "# This file was generated by Splign." << endl;
            cout << "$$$FI" << endl;
            for(size_t k = 0; k < files_count; ++k) {
                cout << fasta_filenames[k] << '\t' << k << endl;
            }
            cout << "$$$SI" << endl;
            for(size_t k = 0; k < files_count; ++k) {
                ifstream ifs (fasta_filenames[k].c_str());
                MakeIDX(&ifs, k, &cout);
            }
            
            return 0;
        }
    }    

#endif

    return CSplignApp().AppMain(argc, argv, 0, eDS_Default, 0);
}
