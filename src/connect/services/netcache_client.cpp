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
 * Author:  Anatoliy Kuznetsov
 *
 * File Description:
 *   Implementation of net cache client.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbitime.hpp>
#include <corelib/plugin_manager_impl.hpp>
#include <connect/ncbi_conn_exception.hpp>
#include <connect/services/netcache_client.hpp>
#include <memory>


BEGIN_NCBI_SCOPE


const string kNetCache_KeyPrefix = "NCID";


CNetCache_Key::CNetCache_Key(const string& key_str)
{
    ParseBlobKey(key_str);
}


void CNetCache_Key::ParseBlobKey(const string& key_str)
{
    // NCID_01_1_MYHOST_9000

    const char* ch = key_str.c_str();
    hostname = prefix = kEmptyStr;

    // prefix

    for (;*ch && *ch != '_'; ++ch) {
        prefix += *ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetCacheException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    if (prefix != kNetCache_KeyPrefix) {
        NCBI_THROW(CNetCacheException, eKeyFormatError, 
                                       "Key syntax error. Invalid prefix.");
    }

    // version
    version = atoi(ch);
    while (*ch && *ch != '_') {
        ++ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetCacheException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    // id
    id = atoi(ch);
    while (*ch && *ch != '_') {
        ++ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetCacheException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    // hostname
    for (;*ch && *ch != '_'; ++ch) {
        hostname += *ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetCacheException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    // port
    port = atoi(ch);
}


void CNetCache_Key::GenerateBlobKey(string*        key,
                                    unsigned int   id,
                                    const string&  host,
                                    unsigned short port)
{
    string tmp;
    *key = "NCID_01";    // NetCacheId prefix plus version

    NStr::IntToString(tmp, id);
    *key += "_";
    *key += tmp;

    *key += "_";
    *key += host;    

    NStr::IntToString(tmp, port);
    *key += "_";
    *key += tmp;

    NStr::IntToString(tmp, time(0));
    *key += "_";
    *key += tmp;
}


unsigned int CNetCache_Key::GetBlobId(const string& key_str)
{
    unsigned id = 0;

    const char* ch = key_str.c_str();

    if (*ch == 0) {
err_throw:
        NCBI_THROW(CNetCacheException, eKeyFormatError, "Key syntax error.");
    }

    // prefix
    int prsum = (ch[0] - 'N') +
                (ch[1] - 'C') +
                (ch[2] - 'I') +
                (ch[3] - 'D');
    if (prsum != 0) {
        goto err_throw;
    }
    ch += 4;
    if (*ch != '_') {
        goto err_throw;
    }

    // version
    ch += 3;
    if (*ch != '_') {
        goto err_throw;
    }
    ++ch;

    // id
    id = (unsigned) atoi(ch);
    return id;
}


CNetCacheClientBase::CNetCacheClientBase(const string& client_name)
    : CNetServiceClient(client_name)
{
}


CNetCacheClientBase::CNetCacheClientBase(const string&  host,
                                         unsigned short port,
                                         const string&  client_name)
    : CNetServiceClient(host, port, client_name)
{
}


void CNetCacheClientBase::RegisterSession(unsigned pid)
{
    _ASSERT(m_Sock);

    char hostname[256];
    string host;
    int status = SOCK_gethostname(hostname, sizeof(hostname));
    if (status != 0) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Cannot get host name");
    }
    string cmd = "SMR ";
    cmd.append(hostname);
    cmd.push_back(' ');
    cmd.append(NStr::UIntToString(pid));
    WriteStr(cmd.c_str(), cmd.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}


void CNetCacheClientBase::UnRegisterSession(unsigned pid)
{
    _ASSERT(m_Sock);

    char hostname[256];
    string host;
    int status = SOCK_gethostname(hostname, sizeof(hostname));
    if (status != 0) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Cannot get host name");
    }
    string cmd = "SMU ";
    cmd.append(hostname);
    cmd.push_back(' ');
    cmd.append(NStr::UIntToString(pid));
    WriteStr(cmd.c_str(), cmd.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}


bool CNetCacheClientBase::CheckAlive()
{
    _ASSERT(m_Sock);
	try {
		WriteStr("A?", 3);
		WaitForServer();
    	if (!ReadStr(*m_Sock, &m_Tmp)) {
			delete m_Sock;m_Sock = 0;
            return false;
    	}
		if (m_Tmp[0] != 'O' && m_Tmp[1] != 'K') {
			delete m_Sock; m_Sock = 0;
			return false;
		}
	}
	catch (exception&) {
		delete m_Sock; m_Sock = 0;
		return false;
	}
    return true;
}


void CNetCacheClientBase::CheckOK(string* str) const
{
    _ASSERT(0);
}


void CNetCacheClientBase::TrimPrefix(string* str) const
{
    _ASSERT(0);
}


CNetCacheClient::CNetCacheClient(const string&  client_name)
    : TParent(client_name),
      m_PutVersion(2)
{
}


CNetCacheClient::CNetCacheClient(const string&  host,
                                 unsigned short port,
                                 const string&  client_name)
    : TParent(host, port, client_name),
      m_PutVersion(2)
{
}


CNetCacheClient::CNetCacheClient(CSocket*      sock, 
                                 const string& client_name)
    : CNetServiceClient(sock, client_name),
      m_PutVersion(2)
{
}


CNetCacheClient::~CNetCacheClient()
{
}


void CNetCacheClient::CheckConnect(const string& key)
{
    if (m_Sock && (eIO_Success == m_Sock->GetStatus(eIO_Open))) {
        return; // we are connected, nothing to do
    }

    // not connected

    if (!m_Host.empty()) { // we can restore connection
        CreateSocket(m_Host, m_Port);
        return;
    }

    // no primary host information

    if (key.empty()) {
        NCBI_THROW(CNetServiceException, eCommunicationError,
           "Cannot establish connection with a server. Unknown host name.");
    }

    CNetCache_Key blob_key(key);
    CreateSocket(blob_key.hostname, blob_key.port);
}


string CNetCacheClient::PutData(const void*  buf,
                                size_t       size,
                                unsigned int time_to_live)
{
    return CNetCacheClient::PutData(kEmptyStr, buf, size, time_to_live);
}


void CNetCacheClient::PutInitiate(string*   key, 
                                  unsigned  time_to_live,
                                  unsigned  put_version)
{
    _ASSERT(key);

    string& request = m_Tmp;
    string put_str = "PUT";
    if (put_version) {
        put_str += NStr::IntToString(put_version);
    }
    put_str.push_back(' ');

    MakeCommandPacket(&request, put_str);
    
    request += NStr::IntToString(time_to_live);
    request += " ";
    request += *key;
   
    WriteStr(request.c_str(), request.length() + 1);
    WaitForServer();

    // Read BLOB_ID answer from the server
    if (!ReadStr(*m_Sock, key)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }

    if (NStr::FindCase(*key, "ID:") != 0) {
        // Answer is not in "ID:....." format
        string msg = "Unexpected server response:";
        msg += *key;
        NCBI_THROW(CNetServiceException, eCommunicationError, msg);
    }
    key->erase(0, 3);
    
    if (key->empty()) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Invalid server response. Empty key.");
    }
}


string  CNetCacheClient::PutData(const string& key,
                                 const void*   buf,
                                 size_t        size,
                                 unsigned int  time_to_live)
{
    _ASSERT(m_PutVersion == 0 || m_PutVersion == 2);

    CheckConnect(key);
    CSockGuard sg(*m_Sock);

    string k(key);
    PutInitiate(&k, time_to_live , m_PutVersion);

    m_Sock->DisableOSSendDelay(false);
    
    if (m_PutVersion == 2) {
        TransmitBuffer((const char*) buf, size);
    } else {
        WriteStr((const char*) buf, size);
    }

    return k;
}


IWriter* CNetCacheClient::PutData(string* key, unsigned int time_to_live)
{
    _ASSERT(key);
    _ASSERT(m_PutVersion == 0 || m_PutVersion == 2);

    CheckConnect(*key);

    PutInitiate(key, time_to_live, m_PutVersion);

    m_Sock->DisableOSSendDelay(false);

    CNetCacheSock_RW* writer = new CNetCacheSock_RW(m_Sock);
    if (m_PutVersion == 2) {

        CNetCache_WriterErrCheck* err_writer =
            new CNetCache_WriterErrCheck(writer, eTakeOwnership, this);
/*
        CTransmissionWriter* twriter = 
            new CTransmissionWriter(writer, eTakeOwnership);
*/
        return err_writer;
    }
    return writer;
}


bool CNetCacheClient::IsAlive()
{
    string version = ServerVersion();
    return !version.empty();
}


string CNetCacheClient::ServerVersion()
{
    CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    string& request = m_Tmp;
    string version;

    MakeCommandPacket(&request, "VERSION ");
    
    WriteStr(request.c_str(), request.length() + 1);
    WaitForServer();

    // Read BLOB_ID answer from the server
    ReadStr(*m_Sock, &version);
    if (NStr::FindCase(version, "OK:") != 0) {
        // error message in version string
        string msg = version;
        if (msg.empty()) {
            msg = "Empty version string.";
        }
        NCBI_THROW(CNetServiceException, eCommunicationError, msg);
    }

    version.erase(0, 3);
    return version;
}


CVersionInfo CNetCacheClient::ServerVersionInfo()
{
    string version = ServerVersion();

    string::size_type pos = version.find("version=");
    if (pos == string::npos) {
        return CVersionInfo::kAny;
    }
    pos += 8;
    version.erase(0, pos);

    CVersionInfo info(version, "NetCache");
    return info;
}


string CNetCacheClient::GetCommErrMsg()
{
    string ret(m_CommErrMsg);
    m_CommErrMsg.erase();
    return ret;
}


void CNetCacheClient::SetCommErrMsg(const string& msg)
{
    m_CommErrMsg = msg;
}


void CNetCacheClient::SendClientName()
{
    unsigned client_len = m_ClientName.length();
    if (client_len < 3) {
        NCBI_THROW(CNetCacheException, 
                   eAuthenticationError, "Client name too small or empty");
    }
    const char* client = m_ClientName.c_str();
    WriteStr(client, client_len + 1);
}


void CNetCacheClient::MakeCommandPacket(string* out_str, 
                                        const string& cmd_str)
{
    _ASSERT(out_str);

    if (m_ClientName.length() < 3) {
        NCBI_THROW(CNetCacheException, 
                   eAuthenticationError, "Client name is too small or empty");
    }

    *out_str = m_ClientName;
    const string& client_name_comment = GetClientNameComment();
    if (!client_name_comment.empty()) {
        out_str->append(" ");
        out_str->append(client_name_comment);
    }
    out_str->append("\r\n");
    out_str->append(cmd_str);
}


void CNetCacheClient::Remove(const string& key)
{
    CheckConnect(key);
    CSockGuard sg(*m_Sock);

    string& request = m_Tmp;
    MakeCommandPacket(&request, "REMOVE ");    
    request += key;
    WriteStr(request.c_str(), request.length() + 1);
}



bool CNetCacheClient::IsLocked(const string& key)
{
    CheckConnect(key);
    CSockGuard sg(*m_Sock);

    string& request = m_Tmp;
    MakeCommandPacket(&request, "ISLK ");
    request += key;
    WriteStr(request.c_str(), request.length() + 1);

    WaitForServer();

    string& answer = m_Tmp;
    
    bool locked = false;

    bool res = ReadStr(*m_Sock, &answer);
    if (res) {
        bool blob_found = x_CheckErrTrim(answer);
        if (!blob_found) {
            return false;
        }
        const char* ans = answer.c_str();
        if (*ans == '1') {
            locked = true;
        }
    } else {
        NCBI_THROW(CNetServiceException, 
                   eCommunicationError, "Communication error");
    }

    return locked;
}


string CNetCacheClient::GetOwner(const string& key)
{
    CheckConnect(key);
    CSockGuard sg(*m_Sock);

    string& request = m_Tmp;
    MakeCommandPacket(&request, "GBOW ");
    request += key;
    WriteStr(request.c_str(), request.length() + 1);

    WaitForServer();

    string& answer = m_Tmp;
    bool res = ReadStr(*m_Sock, &answer);
    if (res) {
        x_CheckErrTrim(answer);
        return answer;
    } else {
        NCBI_THROW(CNetServiceException, 
                   eCommunicationError, "Communication error");
    }
    return kEmptyStr;
}


IReader* CNetCacheClient::GetData(const string& key, 
                                  size_t*       blob_size,
                                  ELockMode     lock_mode)
{
    CheckConnect(key);
    CSockGuard sg(*m_Sock);

    string& request = m_Tmp;
    MakeCommandPacket(&request, "GET2 ");
    
    request += key;

    switch (lock_mode) {
    case eLockNoWait:
        request += " NW";  // no-wait mode
        break;
    case eLockWait:
        break;
    default:
        _ASSERT(0);
        break;
    }

    WriteStr(request.c_str(), request.length() + 1);

    WaitForServer();

    string answer;
    size_t bsize = 0;
    bool res = ReadStr(*m_Sock, &answer);
    if (res) {
        bool blob_found = x_CheckErrTrim(answer);

        if (!blob_found) {
            return NULL;
        }
        string::size_type pos = answer.find("SIZE=");
        if (pos != string::npos) {
            const char* ch = answer.c_str() + pos + 5;
            bsize = (size_t)atoi(ch);
        
            if (blob_size) {
                *blob_size = bsize;
            }
        }
    } else {
        NCBI_THROW(CNetServiceException, 
                   eCommunicationError, "Communication error");
    }

    sg.Release();
    return new CNetCacheSock_RW(m_Sock, bsize);
}


CNetCacheClient::EReadResult 
CNetCacheClient::GetData(const string& key, SBlobData& blob_to_read)
{
    size_t blob_size;
    blob_to_read.blob_size = 0;
    auto_ptr<IReader> reader(GetData(key, &blob_size));
    if (reader.get() == 0)
        return eNotFound;
    
    // allocate

    blob_to_read.blob.reset(new unsigned char[blob_size + 1]);

    unsigned char* ptr = blob_to_read.blob.get();
    size_t to_read = blob_size;
    
    for (;;) {
        size_t bytes_read;
        ERW_Result res = reader->Read(ptr, to_read, &bytes_read);
        switch (res) {
        case eRW_Success:
            to_read -= bytes_read;
            if (to_read == 0) {
                goto ret;
            }
            ptr += bytes_read;
            break;
        case eRW_Eof:
            goto ret;
        default:
            NCBI_THROW(CNetServiceException, eCommunicationError, 
                       "Error while reading BLOB");
        } // switch
    } // for
ret:
    blob_to_read.blob_size = blob_size;
    return eReadComplete;
}


CNetCacheClient::EReadResult
CNetCacheClient::GetData(const string&  key,
                         void*          buf, 
                         size_t         buf_size, 
                         size_t*        n_read,
                         size_t*        blob_size)
{
    _ASSERT(buf && buf_size);

    CheckConnect(key);
    CSockGuard sg(*m_Sock);

    size_t x_blob_size = 0;
    size_t x_read = 0;

    try {
        auto_ptr<IReader> reader(GetData(key, &x_blob_size));
        if (blob_size)
            *blob_size = x_blob_size;

        if (reader.get() == 0)
            return CNetCacheClient::eNotFound;

        size_t buf_avail = buf_size;
        unsigned char* buf_ptr = (unsigned char*) buf;

        if (n_read)
            *n_read = 0;

        while (buf_avail) {
            size_t bytes_read;
            ERW_Result rw_res = reader->Read(buf_ptr, buf_avail, &bytes_read);
            switch (rw_res) {
            case eRW_Success:
                x_read += bytes_read;
                if (n_read)
                    *n_read += bytes_read;
                buf_avail -= bytes_read;
                buf_ptr   += bytes_read;
                break;
            case eRW_Eof:
                if (x_read == 0)
                    return CNetCacheClient::eNotFound;
                if (n_read)
                    *n_read = x_read;
                return CNetCacheClient::eReadComplete;
            case eRW_Timeout:
                break;
            default:
                return CNetCacheClient::eNotFound;
            } // switch
        } // while

    } 
    catch (CNetCacheException& ex) {
        const string& str = ex.what();
        if (str.find_first_of("BLOB not found") > 0) {
            return CNetCacheClient::eNotFound;
        }
        throw;
    }

    if (x_read == x_blob_size) {
        return CNetCacheClient::eReadComplete;
    }

    return CNetCacheClient::eReadPart;
}


void CNetCacheClient::ShutdownServer()
{
    CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    SendClientName();

    const char command[] = "SHUTDOWN";
    WriteStr(command, sizeof(command));
}


void CNetCacheClient::PrintConfig(CNcbiOstream& out)
{
    CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    SendClientName();

    const char command[] = "GETCONF";
    WriteStr(command, sizeof(command));
    PrintServerOut(out);    
}


void CNetCacheClient::PrintStat(CNcbiOstream& out)
{
    CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    SendClientName();

    const char command[] = "GETSTAT";
    WriteStr(command, sizeof(command));
    PrintServerOut(out);
}


void CNetCacheClient::DropStat()
{
    CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    SendClientName();

    const char command[] = "DROPSTAT";
    WriteStr(command, sizeof(command));
}


void CNetCacheClient::Logging(bool on_off)
{
    CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    SendClientName();
    const char* command;
    if (on_off)
        command = "LOG ON";
    else
        command = "LOG OFF";

    WriteStr(command, strlen(command));
}


bool CNetCacheClient::IsError(const char* str)
{
    int cmp = NStr::strncasecmp(str, "ERR:", 4);
    return cmp == 0;
}


void CNetCacheClient::TransmitBuffer(const char* buf, size_t len)
{
    _ASSERT(m_Sock);

    CNetCacheSock_RW   wrt(m_Sock);
    CNetCache_WriterErrCheck err_wrt(&wrt, eNoOwnership, this);

    const char* buf_ptr = buf;
    size_t size_to_write = len;
    while (size_to_write) {
        size_t n_written;
        ERW_Result io_res =
            err_wrt.Write(buf_ptr, size_to_write, &n_written);
        NCBI_IO_CHECK_RW(io_res);

        size_to_write -= n_written;
        buf_ptr       += n_written;
    } // while
    err_wrt.Flush();
}


bool CNetCacheClient::x_CheckErrTrim(string& answer)
{
    if (NStr::strncmp(answer.c_str(), "OK:", 3) == 0) {
        answer.erase(0, 3);
    } else {
        string msg = "Server error:";
        if (NStr::strncmp(answer.c_str(), "ERR:", 4) == 0) {
            answer.erase(0, 4);
        }
        if (NStr::strncmp(answer.c_str(), "BLOB not found", 14) == 0) {
            return false;
        }
        if (NStr::strncmp(answer.c_str(), "BLOB locked", 11) == 0) {
            msg += answer;
            NCBI_THROW(CNetCacheException, eBlobLocked, msg);
        }

        msg += answer;
        NCBI_THROW(CNetCacheException, eServerError, msg);
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////


CNetCacheSock_RW::CNetCacheSock_RW(CSocket* sock) 
: CSocketReaderWriter(sock),
  m_Parent(0),
  m_BlobSizeControl(false),
  m_BlobBytesToRead(0)
{
}

CNetCacheSock_RW::CNetCacheSock_RW(CSocket* sock, size_t blob_size) 
: CSocketReaderWriter(sock),
  m_Parent(0),
  m_BlobSizeControl(true),
  m_BlobBytesToRead(blob_size)
{
}


CNetCacheSock_RW::~CNetCacheSock_RW() 
{
    try {
        FinishTransmission();
    } catch(exception& ex) {
        ERR_POST("Exception in CNetCacheSock_RW::~CNetCacheSock_RW():" << ex.what());
    } catch(...) {
        ERR_POST("Unknown Exception in CNetCacheSock_RW::~CNetCacheSock_RW()");
    }
}

void CNetCacheSock_RW::FinishTransmission()
{ 
    if (m_Sock) { 
        if (m_Parent) {
            m_Parent->ReturnSocket(m_Sock);
            m_Sock = 0;
        } else {
            if (m_Sock->GetStatus(eIO_Open) == eIO_Success)
                m_Sock->Close();
        }
    }
}


void CNetCacheSock_RW::OwnSocket() 
{ 
    m_IsOwned = eTakeOwnership; 
}


void CNetCacheSock_RW::SetSocketParent(CNetServiceClient* parent)
{
    m_Parent = parent;
}
/*

void CNetCacheSock_RW::SetBlobSize(size_t blob_size)
{
    m_BlobSizeControl = true;
    m_BlobBytesToRead = blob_size;
}
*/

ERW_Result CNetCacheSock_RW::PendingCount(size_t* count)
{
    if ( m_BlobSizeControl && m_BlobBytesToRead == 0) {
        *count = 0;
        return eRW_Success;
    } 
    return TParent::PendingCount(count);       
}

ERW_Result CNetCacheSock_RW::Read(void*   buf,
                                  size_t  count,
                                  size_t* bytes_read)
{
    _ASSERT(m_BlobSizeControl);
    if (!m_BlobSizeControl) {
        return TParent::Read(buf, count, bytes_read);
    }
    ERW_Result res = eRW_Eof;
    if ( m_BlobBytesToRead == 0) {
        if ( bytes_read ) {
            *bytes_read = 0;
        }
        return res;
    }
    if ( m_BlobBytesToRead < count ) {
        count = m_BlobBytesToRead;
    }
    size_t nn_read = 0;
    if ( count ) {
        res = TParent::Read(buf, count, &nn_read);
    }
    
    if ( bytes_read ) {
        *bytes_read = nn_read;
    }
    m_BlobBytesToRead -= nn_read;
    if (m_BlobBytesToRead == 0) {
        FinishTransmission();
    }
    return res;
}


///////////////////////////////////////////////////////////////////////////////


CNetCache_WriterErrCheck::CNetCache_WriterErrCheck
                                        (CNetCacheSock_RW* wrt, 
                                         EOwnership        own_writer,
                                         CNetCacheClient*  parent,
                                         CTransmissionWriter::ESendEofPacket send_eof)
: CTransmissionWriter(wrt, own_writer, send_eof),
  m_RW(wrt),
  m_NC_Client(parent)
{
    _ASSERT(wrt);
}


CNetCache_WriterErrCheck::~CNetCache_WriterErrCheck()
{
}


ERW_Result CNetCache_WriterErrCheck::Write(const void* buf,
                                           size_t      count,
                                           size_t*     bytes_written)
{
    if (!m_RW) {  // error detected! block transmission
        if (bytes_written) *bytes_written = 0;
        return eRW_Error;
    }

    ERW_Result res = TParent::Write(buf, count, bytes_written);
    if (res == eRW_Success) {
        CheckInputMessage();
    }
    return res;
}


ERW_Result CNetCache_WriterErrCheck::Flush(void)
{
    if (!m_RW) {
        return eRW_Error;
    }
    ERW_Result res = TParent::Flush();
    if (res == eRW_Success) {
        CheckInputMessage();
    }
    return res;
}


void CNetCache_WriterErrCheck::CheckInputMessage()
{
    _ASSERT(m_RW);

    CSocket& sock = m_RW->GetSocket();

    string msg;

    STimeout to = {0, 0};
    EIO_Status io_st = sock.Wait(eIO_Read, &to);
    switch (io_st) {
    case eIO_Success:
        {
            io_st = sock.ReadLine(msg);
            if (io_st == eIO_Closed) {
                goto closed_err;
            }
            if (!msg.empty()) {
                CNetCacheClient::TrimErr(&msg);
                goto throw_err_msg;
            }
        }
        break;
    case eIO_Closed:
        goto closed_err;
    default:
        break;
    }
    return;
closed_err:
    msg = "Server closed communication channel (timeout?)";

throw_err_msg:
    if (m_NC_Client) {
        m_NC_Client->SetCommErrMsg(msg);
    }
    m_RW = 0;
    NCBI_THROW(CNetServiceException, eCommunicationError, msg);
}


///////////////////////////////////////////////////////////////////////////////
const char* kNetCacheDriverName = "netcache_client";

/// @internal
class CNetCacheClientCF : public IClassFactory<CNetCacheClient>
{
public:

    typedef CNetCacheClient                 TDriver;
    typedef CNetCacheClient                 IFace;
    typedef IFace                           TInterface;
    typedef IClassFactory<CNetCacheClient>  TParent;
    typedef TParent::SDriverInfo   TDriverInfo;
    typedef TParent::TDriverList   TDriverList;

    /// Construction
    ///
    /// @param driver_name
    ///   Driver name string
    /// @param patch_level
    ///   Patch level implemented by the driver.
    ///   By default corresponds to interface patch level.
    CNetCacheClientCF(const string& driver_name = kNetCacheDriverName, 
                      int patch_level = -1)
        : m_DriverVersionInfo
        (ncbi::CInterfaceVersion<IFace>::eMajor,
         ncbi::CInterfaceVersion<IFace>::eMinor,
         patch_level >= 0 ?
            patch_level : ncbi::CInterfaceVersion<IFace>::ePatchLevel),
          m_DriverName(driver_name)
    {
        _ASSERT(!m_DriverName.empty());
    }

    /// Create instance of TDriver
    virtual TInterface*
    CreateInstance(const string& driver  = kEmptyStr,
                   CVersionInfo version = NCBI_INTERFACE_VERSION(IFace),
                   const TPluginManagerParamTree* params = 0) const
    {
        TDriver* drv = 0;
        if (params && (driver.empty() || driver == m_DriverName)) {
            if (version.Match(NCBI_INTERFACE_VERSION(IFace))
                                != CVersionInfo::eNonCompatible) {

            CConfig conf(params);
            string client_name = 
                conf.GetString(m_DriverName, 
                               "client_name", CConfig::eErr_Throw, "noname");
            string service = 
                conf.GetString(m_DriverName, 
                               "service", CConfig::eErr_NoThrow, kEmptyStr);
            NStr::TruncateSpacesInPlace(service);

            if (!service.empty()) {
                unsigned int rebalance_time = conf.GetInt(m_DriverName, 
                                                "rebalance_time",
                                                CConfig::eErr_NoThrow, 10);
                unsigned int rebalance_requests = conf.GetInt(m_DriverName,
                                                "rebalance_requests",
                                                CConfig::eErr_NoThrow, 100);
                unsigned int rebalance_bytes = conf.GetInt(m_DriverName,
                                                "rebalance_bytes",
                                                CConfig::eErr_NoThrow, 5*1024000);
                drv = new CNetCacheClient_LB(client_name, service,
                                             rebalance_time, 
                                             rebalance_requests,
                                             rebalance_bytes );
            } else { // non lb client
                string host = 
                    conf.GetString(m_DriverName, 
                                  "host", CConfig::eErr_Throw, kEmptyStr);
                NStr::TruncateSpacesInPlace(host);
                if( host.empty()) {
                    string msg = "Cannot init plugin " + m_DriverName +
                     ", missing parameter: host or service";
                    NCBI_THROW(CConfigException, eParameterMissing, msg);
                }

                unsigned int port = conf.GetInt(m_DriverName,
                                               "port",
                                               CConfig::eErr_Throw, 9001);

                drv = new CNetCacheClient(host, port, client_name);
            }
            }                               
        }
        return drv;
    }

    void GetDriverVersions(TDriverList& info_list) const
    {
        info_list.push_back(TDriverInfo(m_DriverName, m_DriverVersionInfo));
    }
protected:
    CVersionInfo  m_DriverVersionInfo;
    string        m_DriverName;

};


void NCBI_XCONNECT_EXPORT NCBI_EntryPoint_xnetcache(
     CPluginManager<CNetCacheClient>::TDriverInfoList&   info_list,
     CPluginManager<CNetCacheClient>::EEntryPointRequest method)
{
       CHostEntryPointImpl<CNetCacheClientCF>::
       NCBI_EntryPointImpl(info_list, method);

}


END_NCBI_SCOPE


/*
 * ===========================================================================
 * $Log$
 * Revision 1.69  2006/07/13 14:23:27  didenko
 * Added CNetCacheSock_RW::PendingCount method
 *
 * Revision 1.68  2006/06/15 15:10:43  didenko
 * Improved streams error handling
 *
 * Revision 1.67  2006/05/01 16:36:18  vasilche
 * Fixed error in netcache communication protocol.
 *
 * Revision 1.66  2006/04/18 20:38:04  kuznets
 * Fixed access to NULL address
 *
 * Revision 1.65  2006/03/20 19:15:22  didenko
 * Modified the GET method in a way that the client side initiates
 * SOCKET connection closing.
 *
 * Revision 1.64  2006/02/15 18:38:41  lavr
 * Remove inclusion of unnecessary header files
 *
 * Revision 1.63  2006/01/26 17:50:12  vasilche
 * Do not read beyond blob. Always update bytes_read.
 *
 * Revision 1.62  2006/01/26 16:08:02  kuznets
 * Added BLOB size control to socket based reader
 *
 * Revision 1.61  2006/01/25 17:49:18  kuznets
 * catch possible exception in ~CNetCacheSock_RW
 *
 * Revision 1.60  2006/01/17 17:13:03  kuznets
 * Removed debugging code
 *
 * Revision 1.59  2006/01/17 16:51:07  kuznets
 * Added base class for all NC derived clients, +session management
 *
 * Revision 1.58  2006/01/10 14:44:34  kuznets
 * Save sockets: + connection pool
 *
 * Revision 1.57  2006/01/03 15:36:44  kuznets
 * Added network ICache client
 *
 * Revision 1.56  2005/12/05 13:42:45  kuznets
 * +DropStat()
 *
 * Revision 1.55  2005/11/28 15:22:22  kuznets
 * +GetOwner() - get BLOB's owner
 *
 * Revision 1.54  2005/11/16 17:40:25  kuznets
 * +GetServerVersionInfo()
 *
 * Revision 1.53  2005/10/26 19:09:35  kuznets
 * Bug fix: disconnect from server when throwing exception
 *
 * Revision 1.52  2005/10/25 19:11:09  kuznets
 * Added non blocking blob get
 *
 * Revision 1.51  2005/10/25 14:29:09  kuznets
 * + IsLocked() - BLOB lock detection
 *
 * Revision 1.50  2005/09/21 18:21:14  kuznets
 * Made a class for key manipulation service functions
 *
 * Revision 1.49  2005/08/11 20:45:05  kuznets
 * Improved error processing
 *
 * Revision 1.48  2005/08/11 19:33:16  kuznets
 * Added communication error from IWriter
 *
 * Revision 1.47  2005/08/11 18:19:04  kuznets
 * Improved error checking
 *
 * Revision 1.46  2005/08/11 17:51:24  kuznets
 * Added IWriter implementation with transmission checking
 *
 * Revision 1.45  2005/08/09 16:00:56  kuznets
 * Added GetData(), allocating memory for BLOB (C++ style)
 *
 * Revision 1.44  2005/08/01 16:52:38  kuznets
 * +PrintStat()
 *
 * Revision 1.43  2005/07/27 18:14:12  kuznets
 * PrintConfig()
 *
 * Revision 1.42  2005/06/06 15:32:49  kuznets
 * Use data transfer protocol 2
 *
 * Revision 1.41  2005/05/06 15:13:55  didenko
 * Fixed possible uninitialized variable bugs
 *
 * Revision 1.40  2005/04/19 17:42:07  kuznets
 * Fixed net protocol bug
 *
 * Revision 1.39  2005/04/19 14:17:48  kuznets
 * Alternative put version
 *
 * Revision 1.38  2005/04/13 13:37:10  didenko
 * Changed NetCache PluginManager driver name to netcache_client
 *
 * Revision 1.37  2005/03/22 18:54:07  kuznets
 * Changed project tree layout
 *
 * Revision 1.36  2005/03/22 14:59:22  kuznets
 * Enable TCP/IP buffering when using IWriter
 *
 * Revision 1.35  2005/03/21 16:46:35  didenko
 * + creating from PluginManager
 *
 * Revision 1.34  2005/02/07 13:01:03  kuznets
 * Part of functionality moved to netservice_client.hpp(cpp)
 *
 * Revision 1.33  2005/01/28 19:25:22  kuznets
 * Exception method inlined
 *
 * Revision 1.32  2005/01/28 14:55:14  kuznets
 * GetCommunicatioTimeout() declared const
 *
 * Revision 1.31  2005/01/28 14:46:58  kuznets
 * Code clean-up, added PutData returning IWriter
 *
 * Revision 1.30  2005/01/19 12:21:47  kuznets
 * Fixed bug in restoring host name fron connection
 *
 * Revision 1.29  2005/01/03 19:11:30  kuznets
 * Code cleanup: polishing resource management
 *
 * Revision 1.28  2004/12/27 16:30:42  kuznets
 * + logging control function
 *
 * Revision 1.27  2004/12/20 13:14:10  kuznets
 * +NetcacheClient::SetSocket()
 *
 * Revision 1.26  2004/12/16 17:43:50  kuznets
 * + methods to change comm.timeouts
 *
 * Revision 1.25  2004/11/16 17:01:35  kuznets
 * Close connection after receiving server version
 *
 * Revision 1.24  2004/11/16 14:00:04  kuznets
 * Code cleanup: made use of CSocket::ReadLine() instead of ad-hoc code
 *
 * Revision 1.23  2004/11/10 18:47:30  kuznets
 * fixed connection in IsAlive()
 *
 * Revision 1.22  2004/11/10 13:37:21  kuznets
 * Code cleanup
 *
 * Revision 1.21  2004/11/10 12:40:30  kuznets
 * Bug fix: missing return
 *
 * Revision 1.20  2004/11/09 20:04:47  kuznets
 * Fixed logical errors in GetData
 *
 * Revision 1.19  2004/11/09 19:07:30  kuznets
 * Return not found code in GetData instead of exception
 *
 * Revision 1.18  2004/11/04 21:50:42  kuznets
 * CheckConnect() in Shutdown()
 *
 * Revision 1.17  2004/11/02 17:30:15  kuznets
 * Implemented reconnection mode and no-default server mode
 *
 * Revision 1.16  2004/11/02 13:51:42  kuznets
 * Improved diagnostics
 *
 * Revision 1.15  2004/11/01 16:02:26  kuznets
 * GetData now returns BLOB size
 *
 * Revision 1.14  2004/11/01 14:39:45  kuznets
 * Implemented BLOB update
 *
 * Revision 1.13  2004/10/28 16:16:20  kuznets
 * +CNetCacheClient::Remove()
 *
 * Revision 1.12  2004/10/27 14:16:59  kuznets
 * BLOB key parser moved from netcached
 *
 * Revision 1.11  2004/10/25 14:36:39  kuznets
 * New methods IsAlive(), ServerVersion()
 *
 * Revision 1.10  2004/10/21 15:52:56  kuznets
 * removed unused variable
 *
 * Revision 1.9  2004/10/20 14:51:05  kuznets
 * Optimization in networking
 *
 * Revision 1.8  2004/10/20 13:46:22  kuznets
 * Optimized networking
 *
 * Revision 1.7  2004/10/13 14:46:38  kuznets
 * Optimization in the networking
 *
 * Revision 1.6  2004/10/08 12:30:35  lavr
 * Cosmetics
 *
 * Revision 1.5  2004/10/06 16:49:10  ucko
 * CNetCacheClient::ReadStr: clear str with erase(), since clear() isn't
 * 100% portable.
 *
 * Revision 1.4  2004/10/06 15:27:24  kuznets
 * Removed unused variable
 *
 * Revision 1.3  2004/10/05 19:02:05  kuznets
 * Implemented ShutdownServer()
 *
 * Revision 1.2  2004/10/05 18:18:46  kuznets
 * +GetData, fixed bugs in protocol
 *
 * Revision 1.1  2004/10/04 18:44:59  kuznets
 * Initial revision
 *
 * ===========================================================================
 */
