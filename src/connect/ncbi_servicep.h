#ifndef CONNECT___NCBI_SERVICEP__H
#define CONNECT___NCBI_SERVICEP__H

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
 * Author:  Anton Lavrentiev, Denis Vakatov
 *
 * File Description:
 *   Private API to define server iterator structure.
 *
 */

#include "ncbi_server_infop.h"
#include <connect/ncbi_service.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Table of iterator "virtual functions"
 */
typedef struct {
    void        (*Reset)(SERV_ITER iter);
    SSERV_Info* (*GetNextInfo)(SERV_ITER iter, HOST_INFO* host_info);
    int/*bool*/ (*Update)(SERV_ITER iter, TNCBI_Time now, const char* text);
    int/*bool*/ (*Penalize)(SERV_ITER iter, double penalty);
    void        (*Close)(SERV_ITER iter);
    const char* name;
} SSERV_VTable;


/* Iterator structure
 */
struct SSERV_IterTag {
    const char*  service;        /* requested service name                 */
    TSERV_Type   type;           /* requested server type(s)               */
    unsigned int preferred_host; /* preferred host to select, network b.o. */
    double       preference;     /* range [0..100] %%                      */
    SSERV_Info** skip;           /* servers to skip                        */
    size_t       n_skip;         /* number of servers in the array         */
    size_t       n_max_skip;     /* number of allocated slots in the array */
    SSERV_Info*  last;           /* last server info taken out             */

    const SSERV_VTable* op;      /* table of virtual functions             */

    void*        data;           /* private data field                     */
};


/* Modified 'fast track' routine for one-shot obtaining of a service info.
 * Please see <connect/ncbi_service.h> for explanations [SERV_GetInfoEx()].
 * For now, this call is to exclusively support MYgethostbyname() replacement
 * of standard gethostbyname() libcall in apache Web daemon (see in daemons/).
 */
SSERV_Info* SERV_GetInfoP
(const char*         service,       /* service name                          */
 TSERV_Type          types,         /* mask of type(s) of servers requested  */
 unsigned int        preferred_host,/* preferred host to use service on, nbo */
 double              preference     /* [0=min..100=max] preference in %%     */
 );


/* Private interface: update mapper information from the given text
 * (<CR><LF> separated lines, usually taken from HTTP header).
 */
int/*bool*/ SERV_Update(SERV_ITER iter, const char* text);


/* Private interface: print and return the HTTP-compliant header portion
 * (<CR><LF> separated lines, including the last line) out of the information
 * contained in the iterator; to be used in mapping requests to DISPD.
 * Return value must be 'free'd.
 */
char* SERV_Print(SERV_ITER iter);


/* Get name of underlying service mapper.
 */
const char* SERV_MapperName(SERV_ITER iter);


/* Get final service name, using CONN_SERVICE_NAME_service environment
 * variable, then (if not found) registry section [service] and a key
 * CONN_SERVICE_NAME. Return resulting name (perhaps, an exact copy of
 * "service" if no override name was found in environment/registry), which
 * is to be freed by a caller when no longer needed. Return NULL on error.
 * NOTE: This procedure does not detect cyclical redefinitions.
 */
char* SERV_ServiceName(const char* service);


/* Get configuration file name. Returned '\0'-terminated string
 * is to be free()'d by a caller when no longer needed.
 * Return NULL if no configuration file name available.
 */
char* SERV_GetConfig(void);


/* Given the status gap and wanted preference, calculate
 * acceptable stretch for the gap (the number of candidates is n).
 */
double SERV_Preference(double pref, double gap, unsigned int n);


#ifdef __cplusplus
}  /* extern "C" */
#endif


/*
 * --------------------------------------------------------------------------
 * $Log$
 * Revision 6.20  2003/02/28 14:49:09  lavr
 * SERV_Preference(): redeclare last argument 'unsigned'
 *
 * Revision 6.19  2003/02/13 21:37:28  lavr
 * Comment SERV_Preference(), change last argument
 *
 * Revision 6.18  2003/01/31 21:19:41  lavr
 * +SERV_Preference()
 *
 * Revision 6.17  2002/10/28 20:16:00  lavr
 * Take advantage of host info API
 *
 * Revision 6.16  2002/10/11 19:48:25  lavr
 * +SERV_GetConfig()
 * const dropped in return value of SERV_ServiceName()
 *
 * Revision 6.15  2002/09/19 18:08:43  lavr
 * Header file guard macro changed; log moved to end
 *
 * Revision 6.14  2002/05/06 19:17:04  lavr
 * +SERV_ServiceName() - translation of service name
 *
 * Revision 6.13  2001/09/28 20:50:41  lavr
 * Update VT method changed - now called on per-line basis
 *
 * Revision 6.12  2001/09/24 20:23:39  lavr
 * Reset() method added to VT
 *
 * Revision 6.11  2001/06/25 15:38:00  lavr
 * Heap of services is now not homogeneous, but can
 * contain entries of different types. As of now,
 * Service and Host entry types are introduced and defined
 *
 * Revision 6.10  2001/05/11 15:30:02  lavr
 * Correction in comment
 *
 * Revision 6.9  2001/04/26 14:18:45  lavr
 * SERV_MapperName moved to the private header
 *
 * Revision 6.8  2001/04/24 21:33:58  lavr
 * Added members of mapper V-table: penalize(method) and name(data).
 * Service iterator has got new field 'last' to keep the latest given info.
 *
 * Revision 6.7  2001/03/06 23:57:49  lavr
 * Minor beautifications
 *
 * Revision 6.6  2000/12/29 18:12:51  lavr
 * SERV_Print added to private interface
 *
 * Revision 6.5  2000/12/06 22:21:27  lavr
 * SERV_Print added to private interface
 *
 * Revision 6.4  2000/10/20 17:22:55  lavr
 * VTable changed to have 'Update' method
 * 'SERV_Update' added to private interface
 *
 * Revision 6.3  2000/10/05 21:37:51  lavr
 * Mapper-specific private data field added
 *
 * Revision 6.2  2000/05/22 16:53:12  lavr
 * Rename service_info -> server_info everywhere (including
 * file names) as the latter name is more relevant
 *
 * Revision 6.1  2000/05/12 18:38:16  lavr
 * First working revision
 *
 * ==========================================================================
 */

#endif /* CONNECT___NCBI_SERVICEP__H */
