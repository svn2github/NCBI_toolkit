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
 * Author:  Anton Lavrentiev
 *
 * File Description:
 *   Standard test for named service resolution facility
 *
 */

#include "../ncbi_ansi_ext.h"
#include "../ncbi_lbsmd.h"
#include "../ncbi_priv.h"               /* CORE logging facilities */
#include "../ncbi_servicep.h"
#include <connect/ncbi_heapmgr.h>
#include <stdlib.h>
/* This header must go last */
#include "test_assert.h"


/* One can define env.var. 'service'_CONN_HOST to reroute dispatching
 * information to particular dispatching host (instead of default).
 */
int main(int argc, const char* argv[])
{
    const char* service = argc > 1 ? argv[1] : "bounce";
    SConnNetInfo* net_info;
    const SSERV_Info* info;
    int n_found = 0;
    SERV_ITER iter;

    CORE_SetLOGFormatFlags(fLOG_None          | fLOG_Level   |
                           fLOG_OmitNoteLevel | fLOG_DateTime);
    CORE_SetLOGFILE(stderr, 0/*false*/);
    if (argc > 2) {
        if (strcasecmp(argv[2],"heap") == 0 || strcasecmp(argv[2],"all") == 0){
            HEAP_Options(eOff, eDefault);
            CORE_LOG(eLOG_Note, "Using slow heap access (w/checks)");
        }
        if (strcasecmp(argv[2],"lbsm") == 0 || strcasecmp(argv[2],"all") == 0){
#ifdef NCBI_OS_MSWIN
            if (strcasecmp(argv[2],"lbsm") == 0) {
                CORE_LOG(eLOG_Warning,
                         "Option \"lbsm\" has no useful effect on MS-Windows");
            }
#else
            LBSMD_FastHeapAccess(eOn);
            CORE_LOG(eLOG_Note, "Using live (faster) LBSM heap access");
#endif /*NCBI_OS_MSWIN*/
        }
        if (strcasecmp(argv[2],"lbsm") != 0  &&
            strcasecmp(argv[2],"heap") != 0  &&
            strcasecmp(argv[2],"all")  != 0)
            CORE_LOGF(eLOG_Fatal, ("Unknown option `%s'", argv[2]));
    }

    CORE_LOGF(eLOG_Note, ("Looking for service `%s'", service));
    net_info = ConnNetInfo_Create(service);
    CORE_LOG(eLOG_Trace, "Opening service mapper");
    iter = SERV_OpenP(service, (fSERV_All & ~fSERV_Firewall) |
                      (strpbrk(service, "?*") ? fSERV_Promiscuous : 0),
                      SERV_LOCALHOST, 0/*port*/, 0.0/*preference*/,
                      net_info, 0/*skip*/, 0/*n_skip*/,
                      0/*external*/, 0/*arg*/, 0/*val*/);
    ConnNetInfo_Destroy(net_info);
    if (iter) {
        HOST_INFO hinfo;
        CORE_LOGF(eLOG_Trace,("%s service mapper has been successfully opened",
                              SERV_MapperName(iter)));
        while ((info = SERV_GetNextInfoEx(iter, &hinfo)) != 0) {
            char* info_str = SERV_WriteInfo(info);
            CORE_LOGF(eLOG_Note, ("Server #%d `%s' = %s", ++n_found,
                                  SERV_CurrentName(iter), info_str));
            if (hinfo) {
                double array[2];
                const char* e = HINFO_Environment(hinfo);
                const char* a = HINFO_AffinityArgument(hinfo);
                const char* v = HINFO_AffinityArgvalue(hinfo);
                CORE_LOG(eLOG_Note, "  Host info available:");
                CORE_LOGF(eLOG_Note, ("    Number of CPUs: %d",
                                      HINFO_CpuCount(hinfo)));
                CORE_LOGF(eLOG_Note, ("    Number of tasks: %d",
                                      HINFO_TaskCount(hinfo)));
                if (HINFO_LoadAverage(hinfo, array)) {
                    CORE_LOGF(eLOG_Note, ("    Load averages: %f, %f (BLAST)",
                                          array[0], array[1]));
                } else
                    CORE_LOG (eLOG_Note,  "    Load average: unavailable");
                if (a) {
                    assert(*a);
                    CORE_LOGF(eLOG_Note, ("    Affinity argument: %s", a));
                }
                if (a  &&  v)
                    CORE_LOGF(eLOG_Note, ("    Affinity value:    %s%s%s",
                                          *v ? "" : "\"", v, *v ? "" : "\""));
                CORE_LOGF(eLOG_Note, ("    Host environment: %s%s%s",
                                      e? "\"": "", e? e: "NULL", e? "\"": ""));
                free(hinfo);
            }
            free(info_str);
        }
        CORE_LOG(eLOG_Trace, "Resetting service mapper");
        SERV_Reset(iter);
        CORE_LOG(eLOG_Trace, "Service mapper has been reset");
        if (n_found && !(info = SERV_GetNextInfo(iter)))
            CORE_LOG(eLOG_Fatal, "Service not found after reset");
        CORE_LOG(eLOG_Trace, "Closing service mapper");
        SERV_Close(iter);
    }

    if (n_found != 0)
        CORE_LOGF(eLOG_Note, ("Test complete: %d server(s) found", n_found));
    else
        CORE_LOG(eLOG_Fatal, "Requested service not found");

#if 0
    {{
        SConnNetInfo* net_info;
        net_info = ConnNetInfo_Create(service);
        iter = SERV_Open(service, fSERV_Http, SERV_LOCALHOST, net_info);
        ConnNetInfo_Destroy(net_info);
    }}

    if (iter != 0) {
        while ((info = SERV_GetNextInfo(iter)) != 0) {
            char* info_str = SERV_WriteInfo(info);
            CORE_LOGF(eLOG_Note, ("Service `%s' = %s", service, info_str));
            free(info_str);
            n_found++;
        }
        SERV_Close(iter);
    }
#endif

    CORE_SetLOG(0);
    return 0;
}
