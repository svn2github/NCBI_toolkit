#ifndef CONN_SERVICES___NETSCHEDULE_API_IMPL__HPP
#define CONN_SERVICES___NETSCHEDULE_API_IMPL__HPP

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
 * Authors:  Anatoliy Kuznetsov, Maxim Didenko, Victor Joukov, Dmitry Kazimirov
 *
 * File Description:
 *   NetSchedule client API implementation details.
 *
 */

#include "netservice_api_impl.hpp"

#include <connect/services/netschedule_api.hpp>
#include <connect/services/error_codes.hpp>


#define NCBI_USE_ERRCODE_X   ConnServ_NetSchedule

BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
//
///@internal


template<typename Pred>
bool s_WaitNotification(unsigned wait_time,
                        unsigned short udp_port, Pred pred)
{
    _ASSERT(wait_time);

    EIO_Status status;

    STimeout to;
    to.sec = wait_time;
    to.usec = 0;

    CDatagramSocket  udp_socket;
    udp_socket.SetReuseAddress(eOn);
    STimeout rto;
    rto.sec = rto.usec = 0;
    udp_socket.SetTimeout(eIO_Read, &rto);

    status = udp_socket.Bind(udp_port);
    if (eIO_Success != status) {
        return false;
    }
    time_t curr_time, start_time, end_time;

    start_time = time(0);
    end_time = start_time + wait_time;

    for (;;) {
        curr_time = time(0);
        if (curr_time >= end_time)
            break;
        to.sec = (unsigned int) (end_time - curr_time);

        status = udp_socket.Wait(&to);
        if (eIO_Success != status) {
            continue;
        }
        size_t msg_len;
        string   buf(1024/sizeof(int),0);
        status = udp_socket.Recv(&buf[0], buf.size(), &msg_len, NULL);
        _ASSERT(status != eIO_Timeout); // because we Wait()-ed
        if (eIO_Success == status) {
            buf.resize(msg_len);
            if( pred(buf) )
                return true;
        }
    } // for
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////
//

#define SERVER_PARAMS_ASK_MAX_COUNT 100

template<typename T> struct ToStr { static string Convert(T t); };

template<> struct ToStr<string> {
    static string Convert(const string& val) {
        return '\"' + NStr::PrintableString(val) + '\"';
    }
};
template<> struct ToStr<unsigned int> {
    static string Convert(unsigned int val) {
        return NStr::UIntToString(val);
    }
};

template<> struct ToStr<int> {
    static string Convert(int val) {
        return NStr::IntToString(val);
    }
};

struct SNetScheduleAPIImpl : public CObject
{
    class CNetScheduleServerListener : public INetServerConnectionListener
    {
    public:
        CNetScheduleServerListener() : m_WorkerNodeCompatMode(false) {}

        void SetAuthString(SNetScheduleAPIImpl* impl);

        void MakeWorkerNodeInitCmd(const string& uid,
            unsigned short control_port);

        virtual void OnInit(CObject* api_impl,
            CConfig* config, const string& config_section);
        virtual void OnConnected(CNetServerConnection::TInstance conn);
        virtual void OnError(const string& err_msg, SNetServerImpl* server);

        string m_Auth;
        string m_WorkerNodeInitCmd;
        bool m_WorkerNodeCompatMode;
    };

    SNetScheduleAPIImpl(CConfig* config, const string& section,
        const string& service_name, const string& client_name,
        const string& queue_name);

    string x_SendJobCmdWaitResponse(const string& cmd, const string& job_key)
    {
        return GetServer(job_key).ExecWithRetry(cmd + ' ' + job_key).response;
    }
    template<typename Arg1>
    string x_SendJobCmdWaitResponse(const string& cmd,
        const string& job_key, Arg1 arg1)
    {
        string tmp = cmd;
        if (!job_key.empty())
            tmp += ' ' + job_key + ' ';
        tmp += ToStr<Arg1>::Convert(arg1);
        return GetServer(job_key).ExecWithRetry(tmp).response;
    }
    template<typename Arg1, typename Arg2>
    string x_SendJobCmdWaitResponse(const string& cmd, const string& job_key,
                                    Arg1 arg1, Arg2 arg2)
    {
        string tmp = cmd;
        if (!job_key.empty())
            tmp += ' ' + job_key + ' ';
        tmp += ToStr<Arg1>::Convert(arg1) + ' ' + ToStr<Arg2>::Convert(arg2);
        return GetServer(job_key).ExecWithRetry(tmp).response;
    }
    template<typename Arg1, typename Arg2, typename Arg3>
    string x_SendJobCmdWaitResponse(const string& cmd, const string& job_key,
                                    Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        string tmp = cmd;
        if (!job_key.empty())
            tmp += ' ' + job_key + ' ';
        tmp += ToStr<Arg1>::Convert(arg1) + ' '
            + ToStr<Arg2>::Convert(arg2) + ' ' + ToStr<Arg3>::Convert(arg3);
        return GetServer(job_key).ExecWithRetry(tmp).response;
    }

    const CNetScheduleAPI::SServerParams& GetServerParams();

    CNetServer GetServer(const string& job_key)
    {
        CNetScheduleKey nskey(job_key);
        return m_Service->GetServer(nskey.host, nskey.port);
    }

    CNetScheduleAPI::EJobStatus x_GetJobStatus(
        const string& job_key,
        bool submitter);

    CNetService m_Service;

    static CNetScheduleExceptionMap sm_ExceptionMap;
    string            m_Queue;
    string            m_ProgramVersion;

    auto_ptr<CNetScheduleAPI::SServerParams> m_ServerParams;
    long m_ServerParamsAskCount;
    CFastMutex m_FastMutex;
};


struct SNetScheduleSubmitterImpl : public CObject
{
    SNetScheduleSubmitterImpl(CNetScheduleAPI::TInstance ns_api_impl);

    string SubmitJobImpl(CNetScheduleJob& job,
        unsigned short udp_port, unsigned wait_time);

    void ExecReadCommand(const char* cmd_start,
        const char* cmd_name,
        const string& batch_id,
        const vector<string>& job_ids,
        const string& error_message);

    CNetScheduleAPI m_API;
};

inline SNetScheduleSubmitterImpl::SNetScheduleSubmitterImpl(
    CNetScheduleAPI::TInstance ns_api_impl) :
    m_API(ns_api_impl)
{
}

struct SNetScheduleExecuterImpl : public CObject
{
    SNetScheduleExecuterImpl(CNetScheduleAPI::TInstance ns_api_impl,
        unsigned short control_port);

    bool GetJobImpl(const string& cmd, CNetScheduleJob& job);

    void x_RegUnregClient(const string& cmd);

    CNetScheduleAPI m_API;
    string m_UID;
    unsigned short m_ControlPort;
};

inline SNetScheduleExecuterImpl::SNetScheduleExecuterImpl(
    CNetScheduleAPI::TInstance ns_api_impl, unsigned short control_port) :
    m_API(ns_api_impl),
    m_UID(GetDiagContext().GetStringUID()),
    m_ControlPort(control_port)
{
    CFastMutexGuard g(ns_api_impl->m_FastMutex);

    static_cast<SNetScheduleAPIImpl::CNetScheduleServerListener*>(
        ns_api_impl->m_Service->m_Listener.GetPointer())->MakeWorkerNodeInitCmd(
            m_UID, control_port);
}

struct SNetScheduleAdminImpl : public CObject
{
    SNetScheduleAdminImpl(CNetScheduleAPI::TInstance ns_api_impl);

    typedef map<pair<string, unsigned int>, string> TIDsMap;

    CNetScheduleAPI m_API;
};

inline SNetScheduleAdminImpl::SNetScheduleAdminImpl(
    CNetScheduleAPI::TInstance ns_api_impl) :
    m_API(ns_api_impl)
{
}


END_NCBI_SCOPE


#endif  /* CONN_SERVICES___NETSCHEDULE_API_IMPL__HPP */
