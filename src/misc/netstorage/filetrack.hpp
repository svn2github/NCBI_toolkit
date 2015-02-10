#ifndef CONNECT_SERVICES___FILETRACK_HPP
#define CONNECT_SERVICES___FILETRACK_HPP

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
 * Author:  Dmitry Kazimirov
 *
 * File Description:
 *   FileTrack API declarations.
 *
 */

#include <connect/services/netstorage_impl.hpp>

#include <connect/ncbi_conn_stream.hpp>

#include <corelib/reader_writer.hpp>

#include <util/random_gen.hpp>

BEGIN_NCBI_SCOPE

NCBI_PARAM_DECL(string, filetrack, site);
typedef NCBI_PARAM_TYPE(filetrack, site) TFileTrack_Site;

NCBI_PARAM_DECL(string, filetrack, api_key);
typedef NCBI_PARAM_TYPE(filetrack, api_key) TFileTrack_APIKey;

struct SFileTrackAPI;

struct SFileTrackRequest : public CObject
{
    SFileTrackRequest(SFileTrackAPI* storage_impl,
            CNetStorageObjectLoc* object_loc,
            const string& url,
            const string& user_header = kEmptyStr,
            FHTTP_ParseHeader parse_header = 0);

    CJsonNode ReadJsonResponse();

    ERW_Result Read(void* buf, size_t count, size_t* bytes_read);
    void FinishDownload();

    void CheckIOStatus();

    SFileTrackAPI* m_FileTrackAPI;
    CNetStorageObjectLoc* m_ObjectLoc;
    string m_URL;
    CConn_HttpStream m_HTTPStream;
    int m_HTTPStatus;
    size_t m_ContentLength;
    bool m_FirstRead;
};

struct SFileTrackPostRequest : public SFileTrackRequest
{
    SFileTrackPostRequest(SFileTrackAPI* storage_impl,
            CNetStorageObjectLoc* object_loc,
            const string& url, const string& boundary,
            const string& user_header = kEmptyStr,
            FHTTP_ParseHeader parse_header = 0);

    void SendContentDisposition(const char* input_name);
    void SendFormInput(const char* input_name, const string& value);
    void SendEndOfFormData();

    void Write(const void* buf, size_t count, size_t* bytes_written);
    void FinishUpload();

    string m_Boundary;
};

struct SFileTrackAPI
{
    SFileTrackAPI();

    string LoginAndGetSessionKey(CNetStorageObjectLoc* object_loc);

    CJsonNode GetFileInfo(CNetStorageObjectLoc* object_loc);

    string GetFileAttribute(CNetStorageObjectLoc* object_loc,
            const string& attr_name);
    void SetFileAttribute(CNetStorageObjectLoc* object_loc,
            const string& attr_name, const string& attr_value);

    CRef<SFileTrackPostRequest> StartUpload(CNetStorageObjectLoc* object_loc);
    CRef<SFileTrackRequest> StartDownload(CNetStorageObjectLoc* object_loc);

    void Remove(CNetStorageObjectLoc* object_loc);

    Uint8 GetRandom();
    string GenerateUniqueBoundary();
    string MakeMutipartFormDataHeader(const string& boundary);

    CRandom m_Random;
    CFastMutex m_RandomMutex;

    STimeout m_WriteTimeout;
    STimeout m_ReadTimeout;
};

END_NCBI_SCOPE

#endif  /* CONNECT_SERVICES___FILETRACK_HPP */
