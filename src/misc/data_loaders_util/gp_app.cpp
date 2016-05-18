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
 * Authors:  Mike DiCuccio
 *
 * File Description:
 *
 */

#include <ncbi_pch.hpp>

#include <corelib/request_ctx.hpp>

#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objtools/data_loaders/genbank/readers.hpp>
#include <objtools/data_loaders/blastdb/bdbloader.hpp>

#include <objtools/data_loaders/lds2/lds2_dataloader.hpp>
#include <objtools/lds2/lds2.hpp>

#include <objtools/data_loaders/asn_cache/asn_cache_loader.hpp>

#include <dbapi/driver/drivers.hpp>
#include <dbapi/simple/sdbapi.hpp>

#include <sra/data_loaders/csra/csraloader.hpp>
#include <sra/data_loaders/wgs/wgsloader.hpp>

#include <misc/data_loaders_util/gp_app.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);


void CGPipeAppUtil::DiagRequestStart()
{
    GetDiagContext().GetRequestContext().SetRequestID();
    GetDiagContext().GetRequestContext().GetRequestTimer().Start();
    CDiagContext_Extra extra = GetDiagContext().PrintRequestStart();
    PrintArgs(CNcbiApplication::Instance()->GetArgs(), extra);
}

void CGPipeAppUtil::DiagRequestStop(int status)
{
    GetDiagContext().GetRequestContext().SetRequestStatus(status);
    GetDiagContext().PrintRequestStop();
}

/// Provide standard logging of arguments
void CGPipeAppUtil::PrintArgs(const CArgs& args,
                              CDiagContext_Extra& extra)
{
    vector< CRef<CArgValue> > all_vals = args.GetAll();
    ITERATE (vector< CRef<CArgValue> >, it, all_vals) {
        extra.Print((*it)->GetName(), (*it)->AsString());
    }
}


void CGPipeAppUtil::AddArgumentDescriptions(CArgDescriptions& arg_desc)
{
    arg_desc.SetCurrentGroup("Data source and object manager options");

    // Local Data Store indexes
    arg_desc.AddOptionalKey("lds2", "LDSDatabases",
                            "Comma-separated list of LDS2 databases to use.",
                            CArgDescriptions::eString,
                            CArgDescriptions::fAllowMultiple);

    // BLAST database retrieval
    arg_desc.AddOptionalKey("blastdb", "BlastDatabases",
                            "Comma-separated list of BLAST databases to use. "
                            "Use na: prefix for nucleotide, "
                            "or aa: for protein.",
                            CArgDescriptions::eString,
                            CArgDescriptions::fAllowMultiple);
    if(!arg_desc.Exist("d")) arg_desc.AddAlias("d","blastdb");

    // ASN Cache options
    arg_desc.AddOptionalKey("asn-cache", "AsnCache",
                            "Comma-separated list of ASN Cache databases to use.",
                            CArgDescriptions::eString,
                            CArgDescriptions::fAllowMultiple);


    // ID retrieval options
    if(!arg_desc.Exist("nogenbank")) {
        arg_desc.AddFlag("nogenbank",
                         "Do not use GenBank data loader.");
    }
    if(!arg_desc.Exist("override-idload")) {
        arg_desc.AddFlag("override-idload",
                         "Use IDMAIN if replication is suspended (not to be used on the farm!)");
    }

    arg_desc.AddFlag("vdb", "Use VDB data loader.");
    arg_desc.AddFlag("novdb", "Do not use VDB data loader.");
    arg_desc.SetDependency("vdb",
                           CArgDescriptions::eExcludes,
                           "novdb");
    arg_desc.AddOptionalKey("vdb-path", "Path",
                            "Root path for VDB look-up",
                            CArgDescriptions::eString);

    // SRA options
    arg_desc.AddFlag("sra", "Add the SRA data loader with no options.");

    arg_desc.AddOptionalKey("sra-acc", "AddSra", "Add the SRA data loader, specifying an accession.",
        CArgDescriptions::eString);

    arg_desc.AddOptionalKey("sra-file", "AddSra", "Add the SRA data loader, specifying an sra file.",
        CArgDescriptions::eString);

    arg_desc.SetDependency("sra", CArgDescriptions::eExcludes, "sra-acc");
    arg_desc.SetDependency("sra", CArgDescriptions::eExcludes, "sra-file");
    arg_desc.SetDependency("sra-acc", CArgDescriptions::eExcludes, "sra-file");

    // All remaining arguments as added by the C++ Toolkit application
    // framework are grouped after the above, by default. If the caller
    // doesn't like this, merely set the current group to the desired
    // value (this one, being empty, will be ignored).
    arg_desc.SetCurrentGroup("General application arguments");
}


void CGPipeAppUtil::SetupObjectManager(const CArgs& args, CObjectManager& obj_mgr)
{
    SetupObjectManager(args, obj_mgr, args.Exist("override-idload") && args["override-idload"]);
}

void CGPipeAppUtil::SetupObjectManager(const CArgs& args, CObjectManager& obj_mgr,
                                        bool overrideIdServerOnReplicationSuspended)
{
    //
    // Object Manager Setup
    //
    // Data loaders are configured at priority 1 through N for the
    // first N data loaders other than ID, and ID at priority 16,000
    // (except in the unlikely event of N being that large).

    int priority = 10;
    typedef vector<string> TDbs;

    //
    // Local Data Store: highest priority
    //
    // Justification:
    // A local storage is often set up with altered
    // sequence annotation, and thus may be preferred over
    // less specialized data stores.

    CArgValue::TStringArray lds2dbstr;
    if (args.Exist("lds2") && args["lds2"]) {
        lds2dbstr = args["lds2"].GetStringList();
    }

    ITERATE (CArgValue::TStringArray, it, lds2dbstr) {
        TDbs dbs;
        NStr::Tokenize(*it, ",", dbs);
        ITERATE(TDbs, i, dbs) {
            string db = NStr::TruncateSpaces(*i);
            if (db.empty()) {
                continue;
            }
            db = CDirEntry::CreateAbsolutePath(db);
            db = CDirEntry::NormalizePath(db);
            if ( !CFile(db).Exists() ) {
                ERR_POST(Warning << "LDS2 path not found: omitting: " << db);
                continue;
            }

            string alias("lds2_");
            alias += NStr::NumericToString(priority);
            CLDS2_DataLoader::RegisterInObjectManager(obj_mgr,
                                                      db,
                                                      -1 /* fasta_flags */,
                                                      CObjectManager::eDefault,
                                                      priority);
            LOG_POST(Info << "added loader: LDS2: " << db
                     << " (" << priority << ")");
            ++priority;
        }
    }

    //
    // ASN Cache: 2nd priority
    //
    // Justification:
    // The ASN Cache provides very fast access to annotated
    // sequences, and should be consulted first, with the exception
    // of any locally altered sequence annotation.

    CArgValue::TStringArray asn_cache_str;
    CNcbiRegistry& reg = CNcbiApplication::Instance()->GetConfig();
    string cachePaths = reg.Get("ASN_CACHE","ASN_CACHE_PATH");
    if(cachePaths.size() > 0) {
        asn_cache_str.push_back(cachePaths);
        LOG_POST(Info << "From config: ASNCache:ASN_CACHE_PATH " << cachePaths);
    }

    if (args.Exist("asn-cache") && args["asn-cache"]) {
        asn_cache_str = args["asn-cache"].GetStringList();
    }
    ITERATE (CArgValue::TStringArray, it, asn_cache_str) {
        TDbs dbs;
        NStr::Tokenize(*it, ",", dbs);
        ITERATE(TDbs, i, dbs) {
            string db = NStr::TruncateSpaces(*i);
            if (db.empty()  ||  NStr::EqualNocase(db, "NONE")) {
                continue;
            }
            if (CDir(db).Exists()) {
                try {
                    CAsnCache_DataLoader::RegisterInObjectManager
                        (obj_mgr, db,
                         CObjectManager::eDefault,
                         priority);
                    LOG_POST(Info << "added loader: ASNCache: " << db
                             << " (" << priority << ")");
                    ++priority;
                }
                catch (CException& e) {
                    ERR_POST(Error << "failed to add ASN cache path: "
                             << db << ": " << e);
                }
            } else {
                ERR_POST(Warning << "ASNCache path not found: omitting: " << db);
            }
        }
    }

    //
    // BLAST database retrieval: 3rd priority
    //
    // Justification:
    // Any BLAST databases provide very fast access to sequences,
    // but this data loader does not provide annotation and has
    // limited descriptors (basically, just deflines and taxid).

    CArgValue::TStringArray blastdbstr;
    if (args.Exist("blastdb") && args["blastdb"]) {
        blastdbstr = args["blastdb"].GetStringList();
    }

    ITERATE (CArgValue::TStringArray, it, blastdbstr) {
        TDbs dbs;
        NStr::Tokenize(*it, ",", dbs);
        ITERATE(TDbs, i, dbs) {
            string db = NStr::TruncateSpaces(*i);
            if (db.empty()) {
                continue;
            }
            CBlastDbDataLoader::EDbType dbtype(CBlastDbDataLoader::eUnknown);
            if (NStr::StartsWith(db, "na:", NStr::eNocase)) {
                db.erase(0, 3);
                dbtype = CBlastDbDataLoader::eNucleotide;
            } else if (NStr::StartsWith(db, "aa:", NStr::eNocase)) {
                db.erase(0, 3);
                dbtype = CBlastDbDataLoader::eProtein;
            }
            CBlastDbDataLoader::RegisterInObjectManager(obj_mgr,
                                                        db, dbtype, true,
                                                        CObjectManager::eDefault,
                                                        priority);
            LOG_POST(Info << "added loader: BlastDB: "
                     << (dbtype == CBlastDbDataLoader::eNucleotide ?
                         "nucl" : "protein")
                     << ": " << db
                     << " (" << priority << ")");
            ++priority;
        }
    }

    //SRA
    if(args.Exist("sra") && args["sra"]) {
        CCSRADataLoader::RegisterInObjectManager(obj_mgr, CObjectManager::eDefault, priority++);
    }
    else if(args.Exist("sra-acc") && args["sra-acc"]) {
        CCSRADataLoader::RegisterInObjectManager(obj_mgr,
                                                 args["sra-acc"].AsString(),
                                                 CObjectManager::eDefault,
                                                 priority++);
    }
    else if(args.Exist("sra-file") && args["sra-file"]) {
        CDirEntry file_entry(args["sra-file"].AsString());
        CCSRADataLoader::RegisterInObjectManager(obj_mgr,
                                                 file_entry.GetDir(),
                                                 file_entry.GetName(),
                                                 CObjectManager::eDefault,
                                                 priority++);
    }

    // Added a new Dataloader for VDB - but only if specified in config files.
    {{
         bool use_vdb_loader = args.Exist("vdb") && args["vdb"];
         if ( !use_vdb_loader ) {
             string vdbEnabled = reg.Get("Gpipe","enable_vdb");
             if (vdbEnabled == "Y") {
                 use_vdb_loader = true;
             }
         }
         if (args.Exist("novdb") && args["novdb"]) {
             use_vdb_loader = false;
         }
         if (use_vdb_loader) {
             string vdbpath = reg.Get("Gpipe", "VDB_PATH");
             if (args.Exist("vdb-path") && args["vdb-path"]) {
                 vdbpath = args["vdb-path"].AsString();
             }
             if(vdbpath.empty()){
                 CWGSDataLoader::RegisterInObjectManager
                     (obj_mgr, CObjectManager::eDefault, priority++);
             } else {
                 CWGSDataLoader::SLoaderParams params;
                 params.m_WGSVolPath = vdbpath;
                 CWGSDataLoader::RegisterInObjectManager
                     (obj_mgr,params,CObjectManager::eDefault, priority++);
             }
         }
    }}

    //
    // ID retrieval: last priority
    //
    // Justification:
    // Although ID provides access to fully annotated sequences,
    // present and historic/archival, the retrieval performance is
    // significantly lower when compared to the above sources.
    // (There may be exceptions such as when sequences exist in
    // and ICache service, but even then, performance is comparable
    // so there is limited penalty for being last priority.)
    bool nogenbank = args.Exist("nogenbank") && args["nogenbank"];
    if ( ! nogenbank ) {
        // pubseqos* drivers require this
        DBAPI_RegisterDriver_FTDS();

        if(overrideIdServerOnReplicationSuspended) {
            string regSection = "genbank/"+reg.Get("genbank","loader_method");
            string configuredServer = reg.Get(regSection,"server");
            // If a server is configured in config files, it may be configured
            //  to use a server other than production, so skip the check.
            if(configuredServer.size() == 0) {
                if(IsIdReplSuspended()) {
                    ERR_POST(Warning << "Replication seems to be suspended or delayed. Switching over to IDPROD_OS server.");
                    // CNcbiRegistry& reg = CNcbiApplication::Instance()->GetConfig();
                    reg.Set(regSection,"server", "IDPROD_OS");  // Use the Id Main.
                    reg.Set("PubseqAccess","server", "IDPROD_OS");  // Use the Id Main.
                }
            } else {
                ERR_POST(Warning << "ID Replication backup check skipped, since server name for genbank loader is explicitly set in config files.");
            }
        }


#ifdef HAVE_PUBSEQ_OS
        // we may require PubSeqOS readers at some point, so go ahead and make
        // sure they are properly registered
        GenBankReaders_Register_Pubseq();
        GenBankReaders_Register_Pubseq2();
#endif

        // In order to allow inserting additional data loaders
        // between the above high-performance loaders and this one,
        // leave a gap in the priority range.
        priority = max(priority, 16000);
        CGBDataLoader::RegisterInObjectManager(obj_mgr,
                                               0,
                                               CObjectManager::eDefault,
                                               priority);

        LOG_POST(Info << "added loader: GenBank: "
                 << " (" << priority << ")");
        ++priority;
    }
}

void CGPipeAppUtil::SetupObjectManager2(const CArgs& args, CObjectManager& obj_mgr,
                                        bool overrideIdServerOnReplicationSuspended)
{
    //
    // Object Manager Setup
    //
    // Data loaders are configured to update ID or VDB
    // ID has priority higher than VDB since VDB data is replicated from ID if
    //

    int priority = 10;
    typedef vector<string> TDbs;
    CNcbiRegistry& reg = CNcbiApplication::Instance()->GetConfig();
#if 0 // no lds,asn-cache, sra, local file for update
    //
    // Local Data Store: highest priority
    //
    // Justification:
    // A local storage is often set up with altered
    // sequence annotation, and thus may be preferred over
    // less specialized data stores.

    CArgValue::TStringArray lds2dbstr;
    if (args.Exist("lds2") && args["lds2"]) {
        lds2dbstr = args["lds2"].GetStringList();
    }

    ITERATE (CArgValue::TStringArray, it, lds2dbstr) {
        TDbs dbs;
        NStr::Tokenize(*it, ",", dbs);
        ITERATE(TDbs, i, dbs) {
            string db = NStr::TruncateSpaces(*i);
            if (db.empty()) {
                continue;
            }
            db = CDirEntry::CreateAbsolutePath(db);
            db = CDirEntry::NormalizePath(db);
            if ( !CFile(db).Exists() ) {
                ERR_POST(Warning << "LDS2 path not found: omitting: " << db);
                continue;
            }

            string alias("lds2_");
            alias += NStr::NumericToString(priority);
            CLDS2_DataLoader::RegisterInObjectManager(obj_mgr,
                                                      db,
                                                      -1 /* fasta_flags */,
                                                      CObjectManager::eDefault,
                                                      priority);
            LOG_POST(Info << "added loader: LDS2: " << db
                     << " (" << priority << ")");
            ++priority;
        }
    }

    //
    // ASN Cache: 2nd priority
    //
    // Justification:
    // The ASN Cache provides very fast access to annotated
    // sequences, and should be consulted first, with the exception
    // of any locally altered sequence annotation.

    CArgValue::TStringArray asn_cache_str;
    CNcbiRegistry& reg = CNcbiApplication::Instance()->GetConfig();
    string cachePaths = reg.Get("ASN_CACHE","ASN_CACHE_PATH");
    if(cachePaths.size() > 0) {
        asn_cache_str.push_back(cachePaths);
        LOG_POST(Info << "From config: ASNCache:ASN_CACHE_PATH " << cachePaths);
    }

    if (args.Exist("asn-cache") && args["asn-cache"]) {
        asn_cache_str = args["asn-cache"].GetStringList();
    }
    ITERATE (CArgValue::TStringArray, it, asn_cache_str) {
        TDbs dbs;
        NStr::Tokenize(*it, ",", dbs);
        ITERATE(TDbs, i, dbs) {
            string db = NStr::TruncateSpaces(*i);
            if (db.empty()) {
                continue;
            }
            if (CDir(db).Exists()) {
                try {
                    CAsnCache_DataLoader::RegisterInObjectManager
                        (obj_mgr, db,
                         CObjectManager::eDefault,
                         priority);
                    LOG_POST(Info << "added loader: ASNCache: " << db
                             << " (" << priority << ")");
                    ++priority;
                }
                catch (CException& e) {
                    ERR_POST(Error << "failed to add ASN cache path: "
                             << db << ": " << e);
                }
            } else {
                ERR_POST(Warning << "ASNCache path not found: omitting: " << db);
            }
        }
    }

    //
    // BLAST database retrieval: 3rd priority
    //
    // Justification:
    // Any BLAST databases provide very fast access to sequences,
    // but this data loader does not provide annotation and has
    // limited descriptors (basically, just deflines and taxid).

    CArgValue::TStringArray blastdbstr;
    if (args.Exist("blastdb") && args["blastdb"]) {
        blastdbstr = args["blastdb"].GetStringList();
    }

    ITERATE (CArgValue::TStringArray, it, blastdbstr) {
        TDbs dbs;
        NStr::Tokenize(*it, ",", dbs);
        ITERATE(TDbs, i, dbs) {
            string db = NStr::TruncateSpaces(*i);
            if (db.empty()) {
                continue;
            }
            CBlastDbDataLoader::EDbType dbtype(CBlastDbDataLoader::eUnknown);
            if (NStr::StartsWith(db, "na:", NStr::eNocase)) {
                db.erase(0, 3);
                dbtype = CBlastDbDataLoader::eNucleotide;
            } else if (NStr::StartsWith(db, "aa:", NStr::eNocase)) {
                db.erase(0, 3);
                dbtype = CBlastDbDataLoader::eProtein;
            }
            CBlastDbDataLoader::RegisterInObjectManager(obj_mgr,
                                                        db, dbtype, true,
                                                        CObjectManager::eDefault,
                                                        priority);
            LOG_POST(Info << "added loader: BlastDB: "
                     << (dbtype == CBlastDbDataLoader::eNucleotide ?
                         "nucl" : "protein")
                     << ": " << db
                     << " (" << priority << ")");
            ++priority;
        }
    }

    //SRA
    if(args.Exist("sra") && args["sra"]) {
        CCSRADataLoader::RegisterInObjectManager(obj_mgr, CObjectManager::eDefault, priority++);
    }
    else if(args.Exist("sra-acc") && args["sra-acc"]) {
        CCSRADataLoader::RegisterInObjectManager(obj_mgr,
                                                 args["sra-acc"].AsString(),
                                                 CObjectManager::eDefault,
                                                 priority++);
    }
    else if(args.Exist("sra-file") && args["sra-file"]) {
        CDirEntry file_entry(args["sra-file"].AsString());
        CCSRADataLoader::RegisterInObjectManager(obj_mgr,
                                                 file_entry.GetDir(),
                                                 file_entry.GetName(),
                                                 CObjectManager::eDefault,
                                                 priority++);
    }
#endif
    //
    // ID retrieval: last priority
    //
    // Justification:
    // Although ID provides access to fully annotated sequences,
    // present and historic/archival, the retrieval performance is
    // significantly lower when compared to the above sources.
    // (There may be exceptions such as when sequences exist in
    // and ICache service, but even then, performance is comparable
    // so there is limited penalty for being last priority.)
    bool nogenbank = args.Exist("nogenbank") && args["nogenbank"];
    if ( ! nogenbank ) {
        // pubseqos* drivers require this
        DBAPI_RegisterDriver_FTDS();

        if(overrideIdServerOnReplicationSuspended) {
            string regSection = "genbank/"+reg.Get("genbank","loader_method");
            string configuredServer = reg.Get(regSection,"server");
            // If a server is configured in config files, it may be configured
            //  to use a server other than production, so skip the check.
            if(configuredServer.size() == 0) {
                if(IsIdReplSuspended()) {
                    ERR_POST(Warning << "Replication seems to be suspended or delayed. Switching over to IDPROD_OS server.");
                    // CNcbiRegistry& reg = CNcbiApplication::Instance()->GetConfig();
                    reg.Set(regSection,"server", "IDPROD_OS");  // Use the Id Main.
                    reg.Set("PubseqAccess","server", "IDPROD_OS");  // Use the Id Main.
                }
            } else {
                ERR_POST(Warning << "ID Replication backup check skipped, since server name for genbank loader is explicitly set in config files.");
            }
        }


#ifdef HAVE_PUBSEQ_OS
        // we may require PubSeqOS readers at some point, so go ahead and make
        // sure they are properly registered
        GenBankReaders_Register_Pubseq();
        GenBankReaders_Register_Pubseq2();
#endif

        // In order to allow inserting additional data loaders
        // between the above high-performance loaders and this one,
        // leave a gap in the priority range.
        //priority = max(priority, 16000);
        CGBDataLoader::RegisterInObjectManager(obj_mgr,
                                               0,
                                               CObjectManager::eDefault,
                                               priority);

        LOG_POST(Info << "added loader: GenBank: "
                 << " (" << priority << ")");
        ++priority;
    }

    // Added a new Dataloader for VDB - but only if specified in config files.
    {{
         bool use_vdb_loader = args.Exist("vdb") && args["vdb"];
         if ( !use_vdb_loader ) {
             string vdbEnabled = reg.Get("Gpipe","enable_vdb");
             if (vdbEnabled == "Y") {
                 use_vdb_loader = true;
             }
         }
         if (args.Exist("novdb") && args["novdb"]) {
             use_vdb_loader = false;
         }
         if (use_vdb_loader) {
             string vdbpath = reg.Get("Gpipe", "VDB_PATH");
             if (args.Exist("vdb-path") && args["vdb-path"]) {
                 vdbpath = args["vdb-path"].AsString();
             }
             if(vdbpath.empty()){
                 CWGSDataLoader::RegisterInObjectManager
                     (obj_mgr, CObjectManager::eDefault, priority++);
             } else {
                 CWGSDataLoader::SLoaderParams params;
                 params.m_WGSVolPath = vdbpath;
                 CWGSDataLoader::RegisterInObjectManager
                     (obj_mgr,params,CObjectManager::eDefault, priority++);
             }
         }
    }}
}

CRef<objects::CScope> CGPipeAppUtil::GetDefaultScope(const CArgs& args)
{
    CRef<objects::CObjectManager> object_manager = objects::CObjectManager::GetInstance();
    CGPipeAppUtil::SetupObjectManager(args, *object_manager);
    CRef<objects::CScope> scope(new objects::CScope(*object_manager));
    scope->AddDefaults();
    return scope;
}


void CGPipeAppUtil::AddSerialInputFormatArgumentDescription(CArgDescriptions& a_arg_desc) {
    a_arg_desc.AddFlag("it", "Input data streams are ASN.1 text, not binary.");
}


ESerialDataFormat CGPipeAppUtil::GetSerialInputFormat(const CArgs& args) {
    return args["it"] ? eSerial_AsnText : eSerial_AsnBinary;
}


void CGPipeAppUtil::AddSerialOutputFormatArgumentDescription(CArgDescriptions& a_arg_desc) {
    a_arg_desc.AddFlag("ot", "Output data streams are ASN.1 text, not binary.");
}


ESerialDataFormat CGPipeAppUtil::GetSerialOutputFormat(const CArgs& args) {
    return args["ot"] ? eSerial_AsnText : eSerial_AsnBinary;
}


void CGPipeAppUtil::AddSerialFormatArgumentDescriptions(CArgDescriptions& a_arg_desc)
{
    AddSerialInputFormatArgumentDescription(a_arg_desc);
    AddSerialOutputFormatArgumentDescription(a_arg_desc);
    a_arg_desc.AddFlag("t",
                       "Both input and output data streams are "
                       "ASN.1 text, not binary.");
}


void CGPipeAppUtil::GetSerialFormat(const CArgs& a_args,
                                    ESerialDataFormat& a_input_format,
                                    ESerialDataFormat& a_output_format)
{
    if(a_args["t"]) {

        a_input_format = a_output_format = eSerial_AsnText;

    } else {

        a_input_format = GetSerialInputFormat(a_args);
        a_output_format = GetSerialOutputFormat(a_args);

    }
}


void CGPipeAppUtil::AddIBioseqSourceFormatArgumentDescriptions(CArgDescriptions& arg_desc, const string& arg_name)
{
    arg_desc.AddKey(arg_name, "SeqeunceInputFormat",
                    "Seqeunce Input Format: seq-ids|fasta|seq-entries",
                    CArgDescriptions::eString);
}


EBioseqSourceType CGPipeAppUtil::GetIBioseqSourceInputFormat(const CArgs& args, const string& arg_name)
{
    const string& val = args[arg_name].AsString();
    if (val == "seq-ids") {
        return EBioseqSourceType::eSeqIdList;
    } else if (val == "fasta") {
        return EBioseqSourceType::eFasta;
    } else if (val == "seq-entries-b") {
        return EBioseqSourceType::eAsnBinary;
    } else if (val == "seq-entries-t") {
        return EBioseqSourceType::eAsnText;
    } else if (val == "sra") {
        return EBioseqSourceType::eCSra;
    } else {
        NCBI_THROW(CException, eUnknown,
                   "Unknown sequence format '"+val+"' specified in argument '"+arg_name+"'");
    }
}


bool CGPipeAppUtil::IsIdReplSuspended()
{
    const int kReplDelayThreshold=3;
    int idReplDelay = -1;
    try {
        CDatabase idDatabase("dbapi://anyone:allowed@ID_RS_INFO/id_rs_info");
        idDatabase.Connect();

        CQuery idReplStatusQuery = idDatabase.NewQuery();
        idReplStatusQuery.ExecuteSP("get_db_rep_delay");
        if (idReplStatusQuery.HasMoreResultSets()) {
            CQuery::iterator qit = idReplStatusQuery.begin();
            idReplDelay = qit["max_rep_delay_minutes"].AsInt4();
        }

        ERR_POST(Info << "Id Replication delay (as reported by get_db_rep_delay): " << idReplDelay);
    } catch(CSDB_Exception&) {
        ERR_POST(Warning << "Caught exception while checking Id replication status. Check skipped.");
    }

    return (idReplDelay >= kReplDelayThreshold);
    // return (idReplStatus != 0);
}


END_NCBI_SCOPE
