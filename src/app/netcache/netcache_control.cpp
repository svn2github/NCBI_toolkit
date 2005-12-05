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
 * Authors:  Anatoliy Kuznetsov
 *
 * File Description:  NetCache administration utility
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbireg.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/ncbimisc.hpp>

#include <connect/services/netcache_client.hpp>
#include <connect/ncbi_socket.hpp>


USING_NCBI_SCOPE;

/// Proxy class to open ShutdownServer in CNetCacheClient
///
/// @internal
class CNetCacheClient_Control : public CNetCacheClient 
{
public:
    CNetCacheClient_Control(const string&  host,
                            unsigned short port)
      : CNetCacheClient(host, port, "netcache_control")
    {}

    void ShutdownServer() { CNetCacheClient::ShutdownServer(); }
    void Logging(bool on_off) { CNetCacheClient::Logging(on_off); }
    virtual void CheckConnect(const string& key)
    {
        CNetCacheClient::CheckConnect(key);
    }
    void PrintConfig(CNcbiOstream & out)
    {
        CNetCacheClient::PrintConfig(out);
    }
    void PrintStat(CNcbiOstream & out)
    {
        CNetCacheClient::PrintStat(out);
    }
    void DropStat() { CNetCacheClient::DropStat(); }
};

///////////////////////////////////////////////////////////////////////


/// Netcache stop application
///
/// @internal
///
class CNetCacheControl : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};



void CNetCacheControl::Init(void)
{
    // Setup command line arguments and parameters

    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "NCBI NetCache control.");
    
    arg_desc->AddPositional("hostname", 
                            "NetCache host name.", CArgDescriptions::eString);

    arg_desc->AddPositional("port",
                            "Port number.", 
                            CArgDescriptions::eInteger);

    arg_desc->AddFlag("s", "Shutdown server");
    arg_desc->AddFlag("v", "Server version");
    arg_desc->AddFlag("c", "Server config");
    arg_desc->AddFlag("t", "Server statistics");
    arg_desc->AddFlag("d", "Drop server statistics");

    arg_desc->AddOptionalKey("log",
                             "server_logging",
                             "Switch server side logging",
                             CArgDescriptions::eBoolean);

    arg_desc->AddOptionalKey("retry",
                             "retry",
                             "Number of re-try attempts if connection failed",
                             CArgDescriptions::eInteger);

    arg_desc->AddOptionalKey("owner",
                             "owner",
                             "Get BLOB's owner",
                             CArgDescriptions::eString);


    SetupArgDescriptions(arg_desc.release());
}



int CNetCacheControl::Run(void)
{
    CArgs args = GetArgs();
    const string& host  = args["hostname"].AsString();
    unsigned short port = args["port"].AsInteger();

    CNetCacheClient_Control nc_client(host, port);

    if (args["retry"]) {
        int retry = args["retry"].AsInteger();
        if (retry < 0) {
            ERR_POST(Error << "Invalid retry count: " << retry);
        }
        for (int i = 0; i < retry; ++i) {
            try {
                nc_client.CheckConnect(kEmptyStr);
                break;
            } 
            catch (exception&) {
                SleepMilliSec(5 * 1000);
            }
        }
    }


    if (args["log"]) {  // logging control
        bool on_off = args["log"].AsBoolean();
        nc_client.Logging(on_off);
        NcbiCout << "Logging turned " 
                 << (on_off ? "ON" : "OFF") << " on the server" << NcbiEndl;
    }

    if (args["owner"]) {  // BLOB's owner
        string key = args["owner"].AsString();
        string owner = nc_client.GetOwner(key);
        NcbiCout << "BLOB belongs to: [" << owner << "]" << NcbiEndl;
    }


    if (args["c"]) {  // config
        nc_client.PrintConfig(NcbiCout);
    }

    if (args["t"]) {  // statistics
        nc_client.PrintStat(NcbiCout);
    }
    if (args["d"]) {  // drop stat
        nc_client.DropStat();
        NcbiCout << "Drop statistics request has been sent to server" << NcbiEndl;
    }

    if (args["v"]) {
        string version = nc_client.ServerVersion();
        if (version.empty()) {
            NcbiCout << "NetCache server communication error." << NcbiEndl;
            return 1;
        }
        NcbiCout << version << NcbiEndl;
    }

    if (args["s"]) {  // shutdown
        nc_client.ShutdownServer();
        NcbiCout << "Shutdown request has been sent to server" << NcbiEndl;
        return 0;
    }

    return 0;
}


int main(int argc, const char* argv[])
{
    return CNetCacheControl().AppMain(argc, argv, 0, eDS_Default, 0);
}


/*
 * ===========================================================================
 * $Log$
 * Revision 1.10  2005/12/05 14:00:06  kuznets
 * New options to control statistics collection
 *
 * Revision 1.9  2005/08/08 14:55:05  kuznets
 * Minor tweak
 *
 * Revision 1.8  2005/08/01 16:53:45  kuznets
 * Added -t key for statistics
 *
 * Revision 1.7  2005/07/28 14:58:10  kuznets
 * Added PrintConfig()
 *
 * Revision 1.6  2005/05/02 16:47:18  kuznets
 * + -retry cmd arg
 *
 * Revision 1.5  2005/03/22 18:55:18  kuznets
 * Reflecting changes in connect library layout
 *
 * Revision 1.4  2005/01/05 17:47:13  kuznets
 * Use correct client name
 *
 * Revision 1.3  2004/12/27 16:32:36  kuznets
 * + logging control
 *
 * Revision 1.2  2004/12/20 13:29:53  kuznets
 * Worked around protected NetCacheClient::ShutdownServer()
 *
 * Revision 1.1  2004/10/25 16:07:43  kuznets
 * netcache_stop renamed to netcache_control
 *
 * Revision 1.1  2004/10/21 15:40:33  kuznets
 * Added netcache_stop utility
 *
 *
 * ===========================================================================
 */
