#ifndef CONNECT___NCBI_LBOS__C
#define CONNECT___NCBI_LBOS__C
/*
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
 * Authors:  Dmitriy Elisov
 * @file
 * File Description:
 *   A service discovery API based on LBOS. LBOS is an adapter for ZooKeeper
 *   and tells hosts which can provide with specific service
 */


#include "ncbi_ansi_ext.h"
#include <connect/ncbi_http_connector.h>
#include "ncbi_lbosp.h"
#include "ncbi_priv.h"
#include <stdlib.h>


#define            kHostportStringLength      (16+1+5)/**<
                                                     strlen("255.255.255.255")+
                                                     strlen(":") +
                                                     strlen("65535")         */
#define            kMaxLineSize               1024  /**< used to read
                                                         from socket         */

#define            NCBI_USE_ERRCODE_X         Connect_LBSM /**< Used in
                                                                CORE_LOG*_X  */
static const size_t kLBOSAddresses   =        7; /**< How many addresses of
                                                      LBOS to expect initially,
                                                      plus one for NULL.     */
static const char* kRoleFile         =        "/etc/ncbi/role";
static const char* kDomainFile       =        "/etc/ncbi/domain";
static const char* kLBOSQuery        =        "/lbos/text/mlresolve?name=";
static const
unsigned short int kDefaultLBOSPort  =        8080;  /**< if by
                                                     /etc/ncbi{role, domain} */
static const int   kInitialCandidatesCount =  1; /**< For initial memory
                                                      allocation             */
static       int   s_LBOS_TurnedOn   =        1; /* If LBOS cannot resolve
                                                    even itself,
                                                    we turn it off for
                                                    performance              */

static       int   s_LBOS_Init       =        0; /* If mapper has already
                                                     been initialized        */
/*
 * Declarations of functions we need to be mocking
 */
static SSERV_Info**   s_LBOS_ResolveIPPort (const char*   lbos_address,
                                            const char*   serviceName,
                                            SConnNetInfo* net_info);
static char *         s_LBOS_UrlReadAll    (SConnNetInfo* net_info,
                                            const char*   url);
static void           s_LBOS_FillCandidates(SLBOS_Data*   data,
                                            const char*   service);
static SLBOS_Data*    s_LBOS_ConstructData (size_t        candidatesCapacity);
static void           s_LBOS_DestroyData   (SLBOS_Data*   data);
static SSERV_Info*    s_LBOS_GetNextInfo   (SERV_ITER     iter,
                                            HOST_INFO*    host_info);
static void           s_LBOS_Initialize    (void);
static int/*bool*/    s_LBOS_Feedback      (SERV_ITER     a,
                                            double        b,
                                            int           c);
static void           s_LBOS_Close         (SERV_ITER     iter);
static void           s_LBOS_Reset         (SERV_ITER     iter);
static int/*bool*/    s_LBOS_Update        (SERV_ITER     iter,
                                            const char*   text,
                                            int           code);
/*LBOS is intended to answer in 0.5 sec, or is considered dead*/
static const STimeout   s_LBOS_Timeout       =  {2, 500000};
static char**           s_LBOS_InstancesList =  NULL;/* Not to get 404 errors
                                                        on every resolve, we
                                                        will remember the last
                                                        successful LBOS and
                                                        put it first         */


static SLBOS_Functions s_LBOS_funcs = {
                s_LBOS_ResolveIPPort,
                CONN_Read,
                g_LBOS_ComposeLBOSAddress,
                s_LBOS_FillCandidates,
               /* s_LBOS_ConstructData,*/ /* delete on 6/1/2015 */
                s_LBOS_DestroyData,
                s_LBOS_GetNextInfo,
                s_LBOS_Initialize
                /*g_LBOS_AnnounceEx,*/
                /* g_LBOS_Deannounce*/
};




static const SSERV_VTable s_lbos_op =  {
        s_LBOS_GetNextInfo, /**< Use open()'ed iterator to get next found
                             *   server*/
        s_LBOS_Feedback,    /**< Not used                                    */
        s_LBOS_Update,      /**< Not used                                    */
        s_LBOS_Reset,       /**< Free iterator's 'data'                      */
        s_LBOS_Close,       /**< Close iterator and free its 'data'          */
        "LBOS"              /**< name of mapper                              */
};


int/*bool*/ g_LBOS_StringIsNullOrEmpty(const char* str)
{
    if ((str == NULL) || (strlen(str) == 0)) {
        return (1);
    }
    return (0);
}


SLBOS_Functions* g_LBOS_UnitTesting_GetLBOSFuncs(void)
{
    return &s_LBOS_funcs;
}


/** Check whether LBOS mapper is turned ON or OFF
 * @return
 *  address of static variable s_LBOS_TurnedOn
 * @see
 *  SERV_LBOS_Open()                                                         */
int* g_LBOS_UnitTesting_PowerStatus(void)
{
    return &s_LBOS_TurnedOn;
}


/**  Check whether LBOS mapper is turned ON or OFF
 *  @return
 *   Address of static variable s_LBOS_Init
 *  @see
 *   SERV_LBOS_Open()                                                        */
int* g_LBOS_UnitTesting_InitStatus(void)
{
    return &s_LBOS_Init;
}


/**  List of addresses of LBOS that is maintained in actual state.
 *  @return
 *   address of static variable s_LBOS_InstancesList.
 *  @see
 *   SERV_LBOS_Open(), s_LBOS_FillCandidates()                               */
char** g_LBOS_UnitTesting_InstancesList(void)
{
    return s_LBOS_InstancesList;
}


/**  We compose LBOS hostname based on /etc/ncbi/domain and /etc/ncbi/role.  */
char* g_LBOS_ComposeLBOSAddress(void)
{
    char* site = calloc(sizeof(char*), kMaxLineSize), *read_result;
    char *role, *domain;
    size_t len;
    FILE* domain_file, * role_file;

    if ((role_file = fopen(kRoleFile, "r")) == NULL) {
        /* No role recognized */
        CORE_LOGF(eLOG_Warning, ("s_LBOS_ComposeLBOSAddress"
                ": could not open role file %s",
                kRoleFile));
        free(site);
        return NULL;
    }
    char str[kMaxLineSize];
    /*
     * Read role
     */
    read_result = fgets(str, sizeof(str), role_file);
    fclose(role_file);
    if (read_result == NULL) {
        CORE_LOGF(eLOG_Warning, ("s_LBOS_ComposeLBOSAddress"
                ": could not read from role file %s",
                kRoleFile));
        free(site);
        return NULL;
    } else {
        len= strlen(str);
        assert(len);
        /*We remove unnecessary '/n' and probably '/r'   */
        if (str[len - 1] == '\n') {
            if (--len  &&  str[len - 1] == '\r')
                --len;
            str[len] = '\0';
        }
        if (strncasecmp(str, "try", 3) == 0)
            role = "try";
        else  {
            if (strncasecmp(str, "qa", 2) == 0)
                role = "qa";
            else {
                if (strncasecmp(str, "dev", 3) == 0)
                    role = "dev";
                else {
                    if (strncasecmp(str, "prod", 4) == 0)
                        role = "prod";
                    else {
                        /* No role recognized */
                        CORE_LOGF(eLOG_Warning, ("s_LBOS_ComposeLBOSAddress"
                                ": could not recognize role [%s] in %s",
                                str, kRoleFile));
                        free(site);
                        return NULL;
                    }
                }
            }
        }
    }
    /*
     * Read domain
     */
    if ((domain_file=fopen(kDomainFile, "r")) == NULL) {
        CORE_LOGF(eLOG_Warning, ("s_LBOS_ComposeLBOSAddress"
                ": could not open domain file %s", kDomainFile));
        free(site);
        return NULL;
    }
    read_result = fgets(str, sizeof(str), domain_file);
    fclose(domain_file);
    if (read_result == NULL){
        CORE_LOGF(eLOG_Warning, ("s_LBOS_ComposeLBOSAddress"
                ": could not read from domain file %s", kDomainFile));
        free(site);
        return NULL;
    } else {
        len = strlen(str);
        assert(len);
        /*We remove unnecessary '/n' and probably '/r'   */
        if (str[len - 1] == '\n') {
            if (--len  &&  str[len - 1] == '\r')
                --len;
            str[len] = '\0';
        }
        if (g_LBOS_StringIsNullOrEmpty(str))
        {
            /* No domain recognized */
            CORE_LOGF(eLOG_Warning, ("s_LBOS_ComposeLBOSAddress"
                    ": domain file %s is empty, skipping this "
                    "method", kDomainFile));
            free(site);
            return NULL;
        }
        domain = str;
    }
    sprintf(site, "lbos.%s.%s", role, domain);
    /*Release unneeded resources for this C-string */
    char * realloc_result;
    if ((realloc_result = realloc(site,
                                 sizeof(char) * (strlen(site) + 1))) == NULL) {
        CORE_LOG(eLOG_Warning, "s_LBOS_ComposeLBOSAddress: problem with "
                "realloc while trimming string, using untrimmed string");
    }
    else {
        site = realloc_result;
    }
    return (site);
}


/** Checks iterator, fact that iterator belongs to this mapper, iterator data.
 * Only debug function.                                                      */
int/*bool*/ g_LBOS_CheckIterator(SERV_ITER              iter,
                         ELBOSIteratorCheckType should_have_data)
{
    if (iter == NULL)
        return 0;
    if (should_have_data == 1 && iter->data == NULL) {
        return 0;
    }
    if (should_have_data == 0 && iter->data != NULL) {
        return 0;
    }
    if (strcmp(iter->op->mapper, s_lbos_op.mapper) != 0) {
        return 0;
    }
    return 1;
}

/* UNIT TESTING*/
int/*bool*/ g_LBOS_UnitTesting_SetLBOSFindMethod (SERV_ITER iter,
        ELBOSFindMethod method)
{
    if (!g_LBOS_CheckIterator(iter, 1)) {
        CORE_LOG_X(1, eLOG_Error, "Could not set LBOS find method, problem "
                "with iterator.");
        return (0);
    }
    SLBOS_Data* data = (SLBOS_Data*) iter->data;
    data->find_method = method;
    return (1);
}


/* UNIT TESTING*/
int/*bool*/ g_LBOS_UnitTesting_SetLBOSRoleAndDomainFiles (const char* roleFile,
        const char* domainFile)
{
    if (roleFile != NULL) {
        kRoleFile = strdup(roleFile);
    }
    if (domainFile != NULL) {
        kDomainFile = strdup(domainFile);
    }
    return 1;
}


/* UNIT TESTING*/
/**  Set custom address for LBOS. Can be both hostname:port and IP:port.
 *  Intended mostly for testing.
 * @param[in] iter
 *  Where to set address for LBOS. The settings is made for only one iterator*/
int/*bool*/ g_LBOS_UnitTesting_SetLBOSaddress (SERV_ITER iter, char* address) {
    if (!g_LBOS_CheckIterator(iter, 1)) {
        CORE_LOG_X(1, eLOG_Error, "Could not set LBOS address, problem with "
                "iterator.");
        return (0);
    }
    SLBOS_Data* data = (SLBOS_Data*) iter->data;
    data->lbos_addr = address;
    return (1);
}


/**  This function test existence of the application that should always be
 *  found - LBOS itself. If it is not found, we turn LBOS off.               */
static void s_LBOS_Initialize(void)
{
    const char* service = "/lbos";
    SConnNetInfo* net_info;
    SERV_ITER iter = calloc(sizeof(*iter), 1);
    if (iter == NULL) {
        CORE_LOG_X(1, eLOG_Warning, "RAM error. "
            "Turning LBOS off in this process.");
        s_LBOS_TurnedOn = 0;
        return;
    }
    iter->name = service;
    net_info = ConnNetInfo_Create(service);
    iter->op = SERV_LBOS_Open(iter, net_info, NULL);
    if (iter->op == NULL) {
        CORE_LOG_X(1, eLOG_Warning, "Turning LBOS off in this process.");
        s_LBOS_TurnedOn = 0;
    } else {
        s_LBOS_TurnedOn = 1;
    }
}


/** After we receive answer from dispd.cgi, we parse header
 * to get all hosts.                                                         */
static EHTTP_HeaderParse s_LBOS_ParseHeader(const char*      header,
                                            void* /* int* */ response,
                                            int              server_error)
{
    int* response_output = (int*)response;
    int code = 0/*success code if any*/;
    if (server_error) {
        if (response != NULL)
            *response_output = server_error;
        return eHTTP_HeaderError;
    }
    if (sscanf(header, "%*s %d", &code) < 1) {
        if (response != NULL)
            *response_output = 503; /* Some kind of server error */
        return eHTTP_HeaderError;
    }
    /* check for empty document */
    if (response != NULL)
        *response_output = code;
    return eHTTP_HeaderSuccess;
}


/** Constructor for SLBOS_Data. Returns pointer to new empty initialized
 * SLBOS_Data.                                                              */
static SLBOS_Data* s_LBOS_ConstructData(size_t candidatesCapacity)
{
    SLBOS_Data* data;

    if (!(data = (SLBOS_Data*) calloc(sizeof(SLBOS_Data), 1)))
    {
        CORE_LOG_X(1, eLOG_Error, "Could not allocate memory for LBOS mapper");
        return NULL;
    }
    /*
     * We consider that there will never be more than 20 candidates, which
     * is not many for us
     */
    data->a_cand = candidatesCapacity;
    data->pos_cand = 0;
    data->n_cand = 0;
    data->lbos_addr = NULL;
    data->find_method = eLBOSFindMethod_None;
    data->cand = (SLBOS_Candidate*)calloc(sizeof(SLBOS_Candidate),
                                                        candidatesCapacity);
    return data;
}


/** Destructor for SLBOS_Data.
 * @param[in] data_to_delete
 *  Please note that you have to set pointer to NULL yourself.               */
static void s_LBOS_DestroyData(SLBOS_Data* data)
{
    if (data == NULL) {
        return;
    }
    if (data->cand) {
        size_t i;
        for (i = data->pos_cand; i < data->n_cand; i++) {
            if (data->cand != NULL && data->cand[i].info != NULL) {
                free(data->cand[i].info);
            }
        }
        free(data->cand);
    }
    if (data->net_info) {
        ConnNetInfo_Destroy(data->net_info);
    }
    free(data);
}


static void s_LBOS_Reset(SERV_ITER iter)
{
    size_t i;
    /*
     * First, we need to check if it is our iterator. It would be bad
     * to mess with other mapper's iterator
     */
    if (!g_LBOS_CheckIterator(iter, 1)) {
        CORE_LOGF_X(1, eLOG_Error,
                ("Trying to reset iterator  of [%s] with [%s]",
                        iter->op->mapper, s_lbos_op.mapper));
        return;
    }
    /*
     * Check passed, it is LBOS's iterator. Now we can be sure that we know
     * how to reset it
     */
    SLBOS_Data* data = (SLBOS_Data*) iter->data;
    if (data != NULL) {
        if (data->cand) {
            for (i = data->pos_cand; i < data->n_cand; i++)
                free(data->cand[i].info);
            free(data->cand);
            /*There will hardly be more than 20 candidates, so we allocate
             * memory for candidates straight away, no array of pointers
             */
            data->cand = (SLBOS_Candidate*)calloc(sizeof(SLBOS_Candidate),
                data->a_cand);
        }
#ifndef _DEBUG
        /** We check both that it returns zero and does not crash accessing
         * allocated memory  */
        for (i = 0; i < data->n_cand; i++) {
            assert(data->cand[i].info == NULL);
            data->cand[i].info = (SSERV_Info*)malloc(sizeof(SSERV_Info));
            free(data->cand[i].info);
            data->cand[i].info = NULL;
        }
#endif
        data->n_cand = 0;
        data->pos_cand = 0;
    }
    return;
}


/** Not implemented in LBOS mapper. */
static int/*bool*/ s_LBOS_Feedback (SERV_ITER a, double b, int c)
{
    return 0;
}


/** Not implemented in LBOS mapper. */
static int/*bool*/ s_LBOS_Update(SERV_ITER iter, const char* text, int code)
{
    return 1;
}


static void s_LBOS_Close (SERV_ITER iter)
{
    /*
     * First, we need to check if it is our iterator. It would be bad
     * to mess with other mapper's iterator
     */
    if (!g_LBOS_CheckIterator(iter, 1)) {
        CORE_LOGF_X(1, eLOG_Error,
                ("Trying to close iterator of [%s] with %s",
                        iter->op->mapper, s_lbos_op.mapper));
        return;
    }
    SLBOS_Data* data = (SLBOS_Data*) iter->data;
    if(data->n_cand > 0) {
        /*s_Reset() has to be called before*/
        s_LBOS_Reset(iter);
    }
    s_LBOS_DestroyData(iter->data);
    iter->data = NULL;
    return;
}


/** @brief Just connect and return connection
 *
 *  Internal function to create connection, which simplifies process
 *  by ignoring many settings and making them default
 */
static CONN s_LBOS_ConnectURL(SConnNetInfo* net_info, const char* url,
                              int* responseCode)
{
    THTTP_Flags         flags        = fHTTP_AutoReconnect | fHTTP_Flushable;
    CONN                conn;
    CONNECTOR           connector;

    CORE_LOGF(eLOG_Note, ("Parsing URL \"%s\"", url));
    if (!ConnNetInfo_ParseURL(net_info, url)) {
        CORE_LOG(eLOG_Warning, "Cannot parse URL");
        return NULL;
    }
    CORE_LOGF(eLOG_Note, ("Creating HTTP%s connector",
            &"S"[net_info->scheme != eURL_Https]));
    if (!(connector = HTTP_CreateConnectorEx(net_info, flags,
                                             s_LBOS_ParseHeader,
                                             responseCode/*data*/, 0,
                                             0/*cleanup*/))) {
        CORE_LOG(eLOG_Warning, "Cannot create HTTP connector");
        return NULL;
    }
    CORE_LOG(eLOG_Note, "Creating connection");

    if (CONN_Create(connector, &conn) != eIO_Success) {
        CORE_LOG(eLOG_Warning, "Cannot create connection");
        return NULL;
    }
    /*
     * We define very little timeout so that we can quicly iterate
     * through LBOSes (they should answer in no more than 0.5 second)
     */
    CONN_SetTimeout(conn, eIO_Open,      &s_LBOS_Timeout);
    CONN_SetTimeout(conn, eIO_ReadWrite, &s_LBOS_Timeout);
    return (conn);
}


/** If you are sure that all answer will fit in one char*, use this function
 * to read all input. Must be free()'d by the caller.
 */
static char * s_LBOS_UrlReadAll(SConnNetInfo* net_info, const char* url)
{
    /* Connector to LBOS. We do not know if LBOS really exists at url */
    CONN          connection       = s_LBOS_ConnectURL(net_info, url, NULL);
    if (connection == NULL) {
        return NULL;
    }
    /* Status of reading*/
    EIO_Status    status;
    /* how much how been read to the moment*/
    size_t        totalRead        = 0;
    /* how much we suppose one line takes*/
    size_t        oneLineSize      = kMaxLineSize;
    /* how much bytes was read in one turn*/
    size_t        bytesRead        = 0;
    char*         buf              = (char*)calloc(sizeof(char), oneLineSize);
    /* Total length of buffer. We already add one line because of calloc */
    size_t           totalBufSize     = oneLineSize;
    char*         realloc_result;
    if (buf == NULL)
    {
        CORE_LOG(eLOG_Warning, "s_LBOS_UrlReadAll: No RAM. "
            "Returning NULL.");
        return NULL;
    }
    do {
        /* If there is no LBOS, we will know about it here */
        status = g_LBOS_UnitTesting_GetLBOSFuncs()->
                          Read(connection, buf + strlen(buf),
                               totalBufSize - totalRead,
                               &bytesRead, eIO_ReadPlain);
        totalRead += bytesRead;
        /*If there is next line coming, let's finish this one*/
        if (status == eIO_Success)
        {
            /* Add space to buffer, if needed  */
            if (totalBufSize < totalRead * 2) {
                realloc_result = (char*) realloc(buf, sizeof(char) *
                                                 (totalBufSize * 2));
                if (realloc_result == NULL) {
                    CORE_LOG(eLOG_Warning, "s_LBOS_UrlReadAll: Buffer "
                             "overflow. Returning string at its maximum size");
                    return (buf);
                } else {
                    buf = realloc_result;
                    totalBufSize *= 2;
                }
            }
        }
    } while (status == eIO_Success && bytesRead >= oneLineSize);
    /*In the end we cut buffer to the minimal needed size*/
    if (!(realloc_result = (char*) realloc(buf,
                                          sizeof(char) * (strlen(buf) + 1)))) {
        CORE_LOG(eLOG_Warning, "s_LBOS_UrlReadAll: Buffer shrink error, using "
                "original stirng");
    }  else  {
        buf = realloc_result;
    }
    CONN_Close(connection);
    return (buf);
}


/*   Takes original string and returns URL-encoded string. Original string
 *  is untouched.
 */
char* s_LBOS_URLEncode (const char* to_encode)
{
    /* If all symbols are escape, our string will take triple space */
    size_t encoded_string_buf_size = strlen(to_encode)*3 + 1;
    char* encoded_string = (char*)calloc(sizeof(char),
        encoded_string_buf_size);
    size_t src_read, dst_written; /*strange things needed by URL_Encode*/
    URL_Encode(to_encode, strlen(to_encode), &src_read,
               encoded_string, encoded_string_buf_size, &dst_written);
    return encoded_string;
}

/**   Resolve service name to one of the hosts implementing service.
 *   Uses LBZK at specified IP and port.
 */
static SSERV_Info** s_LBOS_ResolveIPPort(const char* lbos_address,
        const char* serviceName, SConnNetInfo* net_info)
{
    /*Access server at the specified IP and port and receive answer to
     * std::stringstream*/
    SSERV_Info** infos = (SSERV_Info**)calloc(sizeof(SSERV_Info*), 2);
    size_t infos_count = 0;
    size_t infos_capacity = 1;
    char* servicename_url_encoded = s_LBOS_URLEncode(serviceName);
    /*encode service
       namename to url encoding (change ' ' to %20, '/' to %2f, etc.)*/
    size_t url_length = strlen("http://") + strlen(lbos_address) +
            strlen(kLBOSQuery) + strlen(servicename_url_encoded);
    char * url = (char*)malloc(sizeof(char) * url_length + 1); /** to make up
                                                   lbos query URI to connect*/
    if (url == NULL)
    {
        CORE_LOG(eLOG_Warning, "s_LBOS_ResolveIPPort: No RAM. "
            "Returning NULL.");
        return NULL;
    }
    char * lbzk_answer; /*to write down lbos's answer*/
    sprintf(url, "%s%s%s%s", "http://", lbos_address, kLBOSQuery,
            servicename_url_encoded);
    lbzk_answer = s_LBOS_UrlReadAll(net_info, url);
    free(url);
    free(servicename_url_encoded);
    /*
     * We read all the answer, find host:port in the answer and fill
     * 'hostports', occasionally reallocating array, if needed
     */
    if (!g_LBOS_StringIsNullOrEmpty(lbzk_answer)) {
        char* token = NULL, *saveptr = NULL, *str = NULL;
        for(str = lbzk_answer  ;  ;  str = NULL) {
#ifdef NCBI_COMPILER_MSVC
            token = strtok_s(str, "\n", &saveptr);
#else
            token = strtok_r(str, "\n", &saveptr);
#endif
            if (token == NULL) break;
            SSERV_Info * info = SERV_ReadInfoEx(token, serviceName, 0);
            /* Occasionally, the info returned by LBOS can be invalid. */
            if (info == NULL) continue;
            /* We check if we have at least two more places: one for current
             * info and one for finalizing NULL
             */
            if (infos_capacity <= infos_count + 1) {
                SSERV_Info** realloc_result = realloc(infos,
                        sizeof(SSERV_Info*) * (infos_capacity*2 + 1));
                if (realloc_result == NULL) {
                    /* If error with realloc, return as much as could allocate
                     * for*/
                    infos_count--; /*Will just rewrite last info with NULL to
                                     mark end of array*/
                    break;
                } else { /*If realloc successful */
                    infos = realloc_result;
                    infos_capacity = infos_capacity*2 + 1;
                }
            }
            infos[infos_count++] = info;
            char hostport[kHostportStringLength]; //just for debug
            SOCK_HostPortToString(info->host, info->port, hostport,
                    kHostportStringLength);
            /*Copy IP from stream to the result char array*/
            CORE_LOGF(eLOG_Note, ("Resolved to: [%s]", hostport));
        }
    }
    if (lbzk_answer) {
        free(lbzk_answer);
    }
    if (infos_count > 0) {
        /* ... and set this NULL, finalizing the array ...*/
        infos[infos_count] = NULL;
    } else {
        /*... or just deleting it, if nothing was found*/
        free(infos);
        infos = NULL;
    }

    return infos; /*notice that if LBOS did not answer, this is NULL*/
}


/**  This function is needed to get LBOS hostname in different situations.
 *  @param priority_find_method[in]
 *   The first method to try to find LBOS. If it fails, default order of
 *   methods will be used to find LBOS.
 *   Default order is:
 *   1) registry value of [CONN]->LBOS;
 *   2) 127.0.0.1:8080;
 *   3) lbos for current /etc/ncbi/{role, domain}.
 *   To not specify default method, use eLBOSFindMethod_None.
 *  @param lbos[in]
 *   If priority_find_method is eLBOSFindMethod_CustomHost, then lbos is
 *   first looked for at hostname:port specified in this variable.           */
char** g_LBOS_GetLBOSAddressesEx (ELBOSFindMethod priority_find_method,
                                  const char* lbos_addr)
{
    char** addresses = (char**)malloc(sizeof(char*) *
            kLBOSAddresses);
    /* Section for custom-set address*/
    CORE_LOG_X(1, eLOG_Trace, "Getting LBOS addresses...");
    char* lbosaddress = NULL;
    size_t lbosaddresses_count = 0;
    ELBOSFindMethod find_method_order[] = { /*We will change first item since
                                           it is not what we want, but static
                                           array is a luxury we will not miss*/
               priority_find_method /* eLBOSFindMethod_None, if not specified*/
               , eLBOSFindMethod_Registry
               , eLBOSFindMethod_127001
               , eLBOSFindMethod_EtcNcbiDomain
               };
    size_t find_method_iter;
    size_t find_method_count = sizeof(find_method_order)/sizeof(ELBOSFindMethod);
    for (find_method_iter = 0; find_method_iter < find_method_count;
            ++find_method_iter) {
        switch (find_method_order[find_method_iter]) {
        case eLBOSFindMethod_None :
            break;
        case eLBOSFindMethod_CustomHost :
            if (g_LBOS_StringIsNullOrEmpty(lbos_addr)) {
                CORE_LOG_X(1, eLOG_Warning, "Use of custom LBOS address was "
                        "asked for, but no custom address was supplied. Using "
                        "default LBOS.");
            } else {
                lbosaddress = strdup(lbos_addr);
                if (lbosaddress != NULL) {
                    addresses[lbosaddresses_count++] = lbosaddress;
                }
            }
            break;
        case eLBOSFindMethod_127001 :
            lbosaddress = calloc(sizeof(char), kHostportStringLength);
            if (lbosaddress != NULL) {
                sprintf(lbosaddress, "127.0.0.1:8080");
                addresses[lbosaddresses_count++] = lbosaddress;
            }
            break;
        case eLBOSFindMethod_EtcNcbiDomain :
#ifndef NCBI_OS_MSWIN
            lbosaddress = g_LBOS_UnitTesting_GetLBOSFuncs()->
                                                          ComposeLBOSAddress();
            if (g_LBOS_StringIsNullOrEmpty(lbosaddress)) {
                CORE_LOG_X(1, eLOG_Warning, "Trying to find LBOS using "
                        "/etc/ncbi{role, domain} failed");
            } else {
                addresses[lbosaddresses_count++] = lbosaddress;
                /* Also we must check alternative port, 8080 */
                char* lbos_alternative = calloc(sizeof(char),
                                                strlen(lbosaddress) + 8);
                if (lbos_alternative == NULL)  {
                    CORE_LOG_X(1, eLOG_Warning,
                               "Memory allocation problem while construction "
                               "LBOS {role, domain} address");
                } else {
                    sprintf(lbos_alternative, "%s:%hu", lbosaddress,
                            kDefaultLBOSPort);
                    addresses[lbosaddresses_count++] = lbos_alternative;
                }
            }
#endif
            break;
        case eLBOSFindMethod_Registry:
            lbosaddress = calloc(sizeof(char), kMaxLineSize);
            CORE_REG_GET("CONN", "LBOS", lbosaddress, kMaxLineSize, NULL);
            if (g_LBOS_StringIsNullOrEmpty(lbosaddress)) {
                CORE_LOG_X(1, eLOG_Warning, "Trying to find LBOS using "
                        "registry failed");
            } else {
                addresses[lbosaddresses_count++] = lbosaddress;
            }
            break;
        }
    }
    addresses[lbosaddresses_count++] = NULL;
    return addresses;
}

/* Get possible addresses of LBOS in default order */
char** g_LBOS_GetLBOSAddresses(void)
{
    return g_LBOS_GetLBOSAddressesEx(eLBOSFindMethod_None, NULL);
}

/** Given an empty iterator which has service name in its "name" field
 * we fill it with all servers which were found by asking LBOS
 * @param[in,out] iter
 *  We need name from it, then set "data" field with all servers. All
 *  previous "data" will be overwritten, causing possible memory leak.
 */
static void s_LBOS_FillCandidates(SLBOS_Data* data, const char* service)
{
    SSERV_Info** hostports_array = 0;
    /*
     * We iterate through known LBOSes to find working instance
     */
    size_t i = 0;

    /* We copy all LBOS addresses to  */
    char** lbos_addresses = NULL;
    size_t addresses_count = 0;
    /* We suppose that number of addresses is constant (and so
       is position of NULL), so no mutex is necessary */
    while (s_LBOS_InstancesList[++addresses_count] != NULL) continue;
    lbos_addresses = calloc(sizeof(char*), addresses_count + 1);
    /* We can either delete this list or assign to s_LBOS_InstancesList,
     * but anyway we have to prepare */
    char** to_delete = lbos_addresses;
    CORE_LOCK_READ;
    for (i = 0; i < addresses_count; i++) {
        lbos_addresses[i] = strdup(s_LBOS_InstancesList[i]);
    }
    CORE_UNLOCK;
    lbos_addresses[addresses_count] = NULL;
    do
    {
        CORE_LOGF_X(1, eLOG_Trace, ("Trying to find servers of \"%s\" with "
                "LBOS at %s", service, lbos_addresses[i]));
        hostports_array = g_LBOS_UnitTesting_GetLBOSFuncs()->ResolveIPPort(
            lbos_addresses[i], service,
                data->net_info);
        i++;
    } while (hostports_array == NULL  &&  lbos_addresses[i] != NULL);
    size_t j = 0;
    if (hostports_array != NULL) {
        while (hostports_array[++j] != NULL) continue;
        CORE_LOGF_X(1, eLOG_Trace, ("Found %hu servers of \"%s\" with "
            "LBOS at %s", j, service, lbos_addresses[i - 1]));
        if (i != 1) {
            /* If we did not succeed with first LBOS in list, let's swap
             * first LBOS with good LBOS. We will use CORE_LOCK_WRITE only
             in the very last moment */
            char* temp = lbos_addresses[i-1];
            lbos_addresses[i-1] = lbos_addresses[0];
            lbos_addresses[0] = temp;
            if (strcmp(lbos_addresses[0], s_LBOS_InstancesList[0])  !=  0) {
                /* By default, we free() locally created list */
                int/*bool*/ swapped = 0; /* to see which thread actually
                                            did swap */
                CORE_LOCK_WRITE;
                if (strcmp(lbos_addresses[0], s_LBOS_InstancesList[0]) != 0) {
                /* If nobody has swapped static list before us, We assign
                 * static list with our list, and delete previous static list
                 * instead of local list */
                    to_delete = s_LBOS_InstancesList;
                    swapped = 1;
                    s_LBOS_InstancesList = lbos_addresses;
                }
                CORE_UNLOCK;
                if (swapped) {
                    CORE_LOGF_X(1, eLOG_Trace, (
                        "Swapping LBOS order, changing \"%s\" (good) and "
                        "%s (bad)",
                        lbos_addresses[i - 1],
                        lbos_addresses[0]));
                }
            }
        }
    } else {
        CORE_LOGF_X(1, eLOG_Trace, ("Ho servers of \"%s\" found by LBOS",
                service));
    }
    for (j = 0; to_delete[j] != NULL; j++) {
        free(to_delete[j]);
    }
    free(to_delete);
    /*If we received answer from LBOS, we fill candidates */
    if (hostports_array != NULL) {
        /* To allocate space once and forever, let's quickly find the number of
         * received addresses...
         */
        for (i = 0;  hostports_array[i] != NULL;  ++i) continue;
        /* ...and allocate space */
        SLBOS_Candidate* realloc_result = realloc(data->cand,
                                            sizeof(SLBOS_Candidate) * (i + 1));
        if (realloc_result == NULL) {
            CORE_LOGF_X(1, eLOG_Warning, ("s_LBOS_FillCandidates: Could not "
                    "allocate space for all candidates, will use as much as "
                    "was allocated initially: %du",
                    (unsigned int)data->a_cand));
        } else {
            data->cand = realloc_result;
            data->a_cand = i + 1;
        }
        for (i = 0;  hostports_array[i] != NULL && i < data->a_cand;  i++) {
            data->cand[i].info = hostports_array[i];
            data->n_cand++;
        }
        free(hostports_array);
    }
    /*If we did not find answer from LBOS, we just finish*/
}


/** 1) If iterator is valid - find and return next server;
 *  2) If iterator is outdated or empty - we update it and fill it with hosts
 *     and go to 1)                                                          */
static SSERV_Info* s_LBOS_GetNextInfo(SERV_ITER iter, HOST_INFO* host_info)
{
    /*
     * We store snapshot of LBOS answer in memory. So we just go from last
     * known position. Position is stored in n_skip.
     * If we want to update list - well, we update all the list and start from
     * the beginning.
     */

    /* host_info is implemented only for lbsmd mapper, so we just set it to
     * NULL, if it even exists.
     */
    if (host_info) {
        *host_info = NULL; /*No host data*/
    }

    if (iter == NULL) { /* we can do nothing if this happens */
        return NULL;
    }

    if (iter->data == NULL) {
        iter->data = s_LBOS_ConstructData(kInitialCandidatesCount);
    }

    SLBOS_Data* data = (SLBOS_Data*)(iter->data); /** set typed variable
                                                     for convenience */

    if (data->n_cand == 0) { /**< this is possible after reset()*/
        g_LBOS_UnitTesting_GetLBOSFuncs()->FillCandidates(data, iter->name);
    }
    /*
     * If there are some servers left to show, we move iterator and show next
     * Otherwise, return NULL
     */
    if (data->pos_cand < data->n_cand) {
        data->pos_cand++;
        return (SSERV_Info*)(data->cand[data->pos_cand-1].info);
    }  else  {
        return NULL;
    }
}


/** Creates iterator and fills it with found servers.
 * @param[in,out] iter
 *  Pointer to iterator. It is read and rewritten
 * @param[out] info
 *  Pointer to variable to return pointer to info about server.
 * @param[out] host_info
 *  To return info about host
 * @param[in[ no_dispd_follows
 *  To tell whether there will be dispd if all other methods fail.           */
const SSERV_VTable* SERV_LBOS_Open( SERV_ITER            iter,
                                    const SConnNetInfo*  net_info,
                                    SSERV_Info**         info     )
{
    if (s_LBOS_Init == 0) {
        CORE_LOCK_WRITE;
            if (s_LBOS_InstancesList == NULL) {
                s_LBOS_InstancesList = g_LBOS_GetLBOSAddresses();
            }
        CORE_UNLOCK;
        s_LBOS_TurnedOn = 1; /* To ensure that initialization does
                                not depend on this variable */
        s_LBOS_Init = 1;
        s_LBOS_funcs.Initialize();
    }
    if (s_LBOS_TurnedOn == 0) {
        return NULL;
    }
    /*
     * First, we need to check arguments
     */
    if(iter == NULL) {
        CORE_LOG(eLOG_Warning, "Parameter \"iter\" is null, not able "
                "to continue SERV_LBOS_Open");
        return NULL;
    }
    if (iter->name == NULL) {
        CORE_LOG(eLOG_Warning, "\"iter->name\" is null, not able "
            "to continue SERV_LBOS_Open");
        return NULL;
    }
    /*
     * Arguments OK, start work
     */
    if (info != NULL) {
        *info = NULL;
    }
    SLBOS_Data* data = s_LBOS_ConstructData(kInitialCandidatesCount);
    if(net_info == NULL) {
        CORE_LOG(eLOG_Warning,
                 "Parameter \"net_info\" is null, creating net info. "
                 "Please, fix the code and provide net_info.");
        data->net_info = ConnNetInfo_Create(NULL);
    } else {
        data->net_info = ConnNetInfo_Clone(net_info);
    }
    g_LBOS_UnitTesting_GetLBOSFuncs()->FillCandidates(data, iter->name);
    /* Connect to LBOS, read what is needed, build iter, info, host_info
     */
    if (!data->n_cand) {
        s_LBOS_DestroyData(data);
        return NULL;
    }
    /* Something was found, now we can use iter */

    /*Just explicitly mention here to do something with it*/
    iter->data = data;

    return &s_lbos_op;
}

#ifdef ANNOUNCE_DEANNOUNCE_TEST

static const char*      s_LBOS_MyInstance    =  NULL; /* LBOS which handles
                                                         our service*/
static const char*      s_LBOS_MyService     =  NULL; /* service which we
                                                         announce*/
static const char*      s_LBOS_MyVersion     =  NULL; /* version of service
                                                         which we announce*/
static unsigned short   s_LBOS_MyPort        =  0;    /* port of service
                                                         which we announce*/


/* For announcement we search for a LBOS which can handle our request.
 *Search starts with default order of LBOS.
 * @param service[in]
 *  Name of service as it will appear in ZK. For services this means that
 *  name should start with '/'
 * @param version[in]
 *  Any string that will help to identify certain version of service. A good
 *  idea is to use semantic versioning (http://semver.org/) like '4.7.2'
 * @param port[in]
 *  Port for service. Can differ from healthcheck port
 * @param healthcheck_url[in]
 *  Full absolute URL starting with "http://" or "https://". Should include
 *  hostname or IP and port, if necessary.
 * @param LBOS_answer[out]
 *  This variable will be assigned a pointer to char* with exact answer of
 *  LBOS, or NULL. Must be free()'d by the caller. If
 *  eLBOSAnnounceResult_Success is returned, LBOS answer contains "host:port"
 *  of LBOS that was used for announce. If something else is returned, LBOS
 *  answer contains human-readable error message.
 */
/*ELBOSAnnounceResult g_LBOS_AnnounceEx(const char* service,
                     const char* version,
                     unsigned short port,
                     const char* healthcheck_url,
                     char** LBOS_answer)
{*/
    /*
     * First we check input arguments
     */
   /* if (    (strstr(healthcheck_url, "http://") == NULL)
            &&
            (strstr(healthcheck_url, "https://") == NULL)) {
        CORE_LOG(eLOG_Critical, "Error with announcement, missing http:// or "
                "https:// in the beginning of healthcheck URL.");
        return eLBOSAnnounceResult_InvalidArgs;
    }
    if (port < 1 || port > 65535) {
        CORE_LOG(eLOG_Critical, "Error with announcement, incorrect port.");
        return eLBOSAnnounceResult_InvalidArgs;
    }
    if (version == NULL) {
        CORE_LOG(eLOG_Critical, "Error with announcement, "
                "no version specified.");
        return eLBOSAnnounceResult_InvalidArgs;
    }
    if (service == NULL) {
        CORE_LOG(eLOG_Critical, "Error with announcement, "
                "no service name specified.");
        return eLBOSAnnounceResult_InvalidArgs;
    }*/
    /*
     * Pre-assign variables
     */
    /**LBOS_answer = NULL;
    SConnNetInfo* net_info = ConnNetInfo_Create(service);
    net_info->req_method = eReqMethod_Post;
    char* healthcheck_encoded = s_LBOS_URLEncode(healthcheck_encoded);
    char** lbos_addresses = g_LBOS_GetLBOSAddresses();
    char* buf = calloc(sizeof(char), kMaxLineSize);
    int response_code = 0;
    int i = 0;
    for (i = 0;  lbos_addresses[i] != NULL && response_code != 200;  ++i)
    {
        char query [kMaxLineSize];
        sprintf(query,
                "http://%s/lbos/json/announce?name=%s&"
                "version=%s&port=%hu&check=%s",
                lbos_addresses[i], service, version, port, healthcheck_encoded);
        CONN conn = s_LBOS_ConnectURL(net_info,  query, &response_code);*/
        /* Let's check if LBOS exists at this address */
       /* size_t bytesRead;
        g_LBOS_UnitTesting_GetLBOSFuncs()->Read(conn, buf,
                          kMaxLineSize, &bytesRead, eIO_ReadPlain);
        CONN_Close(conn);
    }*/
    /* Cleanup */
    /*for (i = 0;  lbos_addresses[i] != NULL; ++i) {
        free(lbos_addresses[i]);
    }
    free(lbos_addresses);
    free(healthcheck_encoded);
    if (!g_LBOS_StringIsNullOrEmpty(buf)) {
        LBOS_answer = buf;
    } else {
        free(buf);
    }*/
    /* If no LBOS found */
    /*if (response_code == 0) {
        CORE_LOG(eLOG_Warning, "g_LBOS_Announce: could not announce!"
                "No LBOS found.");
        return eLBOSAnnounceResult_NoLBOS;
    }*/
    /* If we could not announce, it is really bad */
    /*if (response_code != 200) {
        CORE_LOG(eLOG_Warning, "g_LBOS_Announce: could not announce!"
                "LBOS returned error code. "
                "LBOS answer was written to LBOS_answer.");
        return eLBOSAnnounceResult_LBOSError;
    }
    return eLBOSAnnounceResult_Success;
}*/


/** Announce this process. In this case, you can use "0.0.0.0" as IP, and
 * it will be converted to local machine's IP
 * @param healthcheck_url[in]
 *  If you want to announce a service that is on the same machine that
 *  announces it (i.e., if service announces itself), you can write
 *  "0.0.0.0" for IP (this is convention with LBOS). You still have to
 *  provide port, even if you write "0.0.0.0".                               */
/*ELBOSAnnounceResult g_LBOS_AnnounceSelf(const char* service,
                     const char* version,
                     unsigned short port,
                     const char* healthcheck_url)
{*/
    /* We said that we will not touch healthcheck_url, but actually we need
     * to be able to change it, so we copy it */
    /*char* my_healthcheck_url = calloc(sizeof(char), kMaxLineSize);
    strcpy(my_healthcheck_url, healthcheck_url);
    char* replace_pos;*//* to check if we need to replace 0.0.0.0 with hostname */
    /*if ((replace_pos = strstr(my_healthcheck_url, "0.0.0.0")) != NULL) {
        char  *temp = calloc(sizeof(char), kMaxLineSize),
                *realloc_result,
                *hostname = calloc(sizeof(char), kMaxLineSize);
        size_t chars_to_copy = replace_pos - my_healthcheck_url;
        strncpy(temp, my_healthcheck_url, chars_to_copy);
        if (SOCK_gethostbyaddrEx(0, hostname, kMaxLineSize - 1, eOn) == NULL) {
            CORE_LOG(eLOG_Warning, "Error with announcement, "
                     "cannot find local IP.");
            return eLBOSAnnounceResult_DNSResolveError;
        }
        strcat(temp, strlwr(hostname));
        strcat(temp, replace_pos + strlen("0.0.0.0"));
        free(my_healthcheck_url);
        my_healthcheck_url = temp;
        free(hostname);
    }
    char* lbos_answer;
    ELBOSAnnounceResult result = g_LBOS_AnnounceEx(service,
                                                   version,
                                                   port,
                                                   my_healthcheck_url,
                                                   &lbos_answer);
    free(my_healthcheck_url);
    if (result = eLBOSAnnounceResult_Success) {
        s_LBOS_MyService = strdup(service);
        sscanf(lbos_answer, "{\"watcher\":\"%[^\"]\"}", s_LBOS_MyInstance);
        if (LBOS_hostport != NULL) {
            LBOS_hostport = strdup(s_LBOS_MyInstance);
        }
        s_LBOS_MyVersion = strdup(version);
        s_LBOS_MyPort = port;
    }
    return result;
}*/


/** Deannounce previously announced service. 0 means error, 1 means success.
 * @param lbos_hostport[in]  Address of the same LBOS that was used for
 *  announcement of the service now being deannounced
 * @param service[in]
 *  Name of service to be deannounced
 * @param version[in]
 *  Version of service to be deannounced
 * @param port[in]
 *  Port of service to be deannounced
 * @param[in]
 *  IP or hostname of service to be deannounced                              */
/*int*//*bool*/ /*g_LBOS_Deannounce(const char*       lbos_hostport,
                              const char*       service,
                              const char*       version,
                              unsigned short    port,
                              const char*       ip)
{
    SConnNetInfo* net_info = ConnNetInfo_Create(s_LBOS_MyService);
    net_info->req_method = eReqMethod_Post;
    char buf[kMaxLineSize];
    char query[kMaxLineSize];
    sprintf(query,
            "http://%s/lbos/json/conceal?name=%s&version=%s&port=%hu&ip=%s",
            lbos_hostport, service, version, port,
            ip);
    int response_code = 0;
    CONN conn = s_LBOS_ConnectURL(net_info, query, &response_code);*/
    /* Let's check if LBOS exists at this address */
    /*int bytesRead;
    g_LBOS_UnitTesting_GetLBOSFuncs()->Read(conn, buf,
                      kMaxLineSize, &bytesRead, eIO_ReadPlain);*/

    /*
     * Cleanup
     */
    /*CONN_Close(conn);*/

    /*
     * If we could not announce, it is really bad
     */
    /*if (response_code == 0) {
        CORE_LOG(eLOG_Warning, "g_LBOS_Announce: could not deannounce myself! "
                               "LBOS not found.");
        return 0;
    }
    if (response_code != 200) {
        CORE_LOG(eLOG_Warning, "g_LBOS_Announce: could not deannounce myself! "
                               "LBOS returned error code.");
        return 0;
    }
    return 1;
}*/


/*  To deannounce we need to go the very same LBOS which we used for
 * announcement. If it is dead, that means that we are anyway not visible for
   ZK, so we can just exit deannouncement                                    */
/*int*//*bool*/ /*g_LBOS_DeannounceSelf(void)
{*/
    /* IF we did not announce ourself, exit with error */
    /*if (s_LBOS_MyInstance == NULL) {
        CORE_LOG(eLOG_Warning, "g_LBOS_DeannounceSelf: "
                "Did not announce myself, cannot deannounce!");
    }
    char* myhost = calloc(sizeof(char), kMaxLineSize);
    if (SOCK_gethostbyaddrEx(0, myhost, kMaxLineSize-1, eOn) == NULL) {
        CORE_LOG(eLOG_Fatal, "Error with self-deannouncement, "
                 "cannot find local IP.");
        return 0;
    }*/
    /*int*//*bool*/ /*result = g_LBOS_UnitTesting_GetLBOSFuncs()->
                    Deannounce(s_LBOS_MyInstance, s_LBOS_MyService,
                             s_LBOS_MyVersion, s_LBOS_MyPort,
                             myhost);
    free(myhost);
    return result;
}*/

#endif


#undef NCBI_USE_ERRCODE_X

#endif /*CONNECT___NCBI_LBOS__C*/