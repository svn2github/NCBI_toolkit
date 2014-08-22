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
 *   NetSchedule API - job reader.
 *
 */

#include <ncbi_pch.hpp>

#include "netschedule_api_impl.hpp"

#define NCBI_USE_ERRCODE_X   ConnServ_NetSchedule

#define READJOB_TIMEOUT 10

BEGIN_NCBI_SCOPE

void CNetScheduleJobReader::SetJobGroup(const string& group_name)
{
    m_Impl->m_JobGroup = group_name;
}

void CNetScheduleJobReader::SetAffinity(const string& affinity)
{
    m_Impl->m_Affinity = affinity;
}

SNotificationTimelineEntry*
    SNetScheduleJobReaderImpl::x_GetTimelineEntry(SNetServerImpl* server_impl)
{
    SNotificationTimelineEntry search_pattern(
            server_impl->m_ServerInPool->m_Address, 0);

    TTimelineEntries::iterator it(
            m_TimelineEntryByAddress.find(&search_pattern));

    if (it != m_TimelineEntryByAddress.end())
        return *it;

    SNotificationTimelineEntry* new_entry = new SNotificationTimelineEntry(
            search_pattern.m_ServerAddress, m_DiscoveryIteration);

    m_TimelineEntryByAddress.insert(new_entry);

    return new_entry;
}

// True if a job is returned.
static bool s_ParseReadJobResponse(const string& response,
        CNetScheduleJob& job,
        CNetScheduleAPI::EJobStatus* job_status,
        bool* no_more_jobs)
{
    *no_more_jobs = false;

    if (response.empty())
        return false;

    enum {
        eJobKey,
        eJobStatus,
        eAuthToken,
        eNumberOfRequiredFields
    };
    int job_fields = 0;
    CUrlArgs url_parser(response);
    ITERATE(CUrlArgs::TArgs, field, url_parser.GetArgs()) {
        switch (field->name[0]) {
        case 'j':
            if (field->name == "job_key") {
                job.job_id = field->value;
                job_fields |= (1 << eJobKey);
            }
            break;

        case 's':
            if (field->name == "status") {
                *job_status = CNetScheduleAPI::StringToStatus(field->value);
                job_fields |= (1 << eJobStatus);
            }
            break;

        case 'a':
            if (field->name == "auth_token") {
                job.auth_token = field->value;
                job_fields |= (1 << eAuthToken);
            } else if (field->name == "affinity")
                job.affinity = field->value;
            break;

        case 'c':
            if (field->name == "client_ip")
                job.client_ip = field->value;
            else if (field->name == "client_sid")
                job.session_id = field->value;
            break;

        case 'n':
            if (field->name == "ncbi_phid")
                job.page_hit_id = field->value;
            else if (field->name == "no_more_jobs")
                *no_more_jobs = NStr::StringToBool(field->value.c_str());
        }
    }

    return job_fields == (1 << eNumberOfRequiredFields) - 1;
}

bool SNetScheduleJobReaderImpl::x_ReadJob(SNetServerImpl* server,
        const CDeadline* timeout, CNetScheduleJob& job,
        CNetScheduleAPI::EJobStatus* job_status,
        bool* no_more_jobs)
{
    string cmd(m_NotificationHandler.CmdAppendTimeoutAndClientInfo("READ",
            timeout));

    CNetServer::SExecResult exec_result;

    server->ConnectAndExec(cmd, exec_result);

    return s_ParseReadJobResponse(exec_result.response, job,
            job_status, no_more_jobs);
}

bool SNetScheduleJobReaderImpl::x_PerformTimelineAction(
        SNetScheduleJobReaderImpl::TNotificationTimeline& timeline,
        CNetScheduleJob& job,
        CNetScheduleAPI::EJobStatus* job_status,
        CNetScheduleJobReader::EReadNextJobResult* result)
{
    SNotificationTimelineEntry::TRef timeline_entry;

    timeline.Shift(timeline_entry);

    if (timeline_entry->IsDiscoveryAction()) {
        ++m_DiscoveryIteration;
        for (CNetServiceIterator it = m_NetScheduleAPI.GetService().Iterate(
                CNetService::eIncludePenalized); it; ++it) {
            SNotificationTimelineEntry* srv_entry = x_GetTimelineEntry(*it);
            srv_entry->m_DiscoveryIteration = m_DiscoveryIteration;
            if (!srv_entry->IsInTimeline())
                m_ImmediateActions.Push(srv_entry);
        }

        timeline_entry->ResetTimeout(READJOB_TIMEOUT);
        m_Timeline.Push(timeline_entry);
        return false;
    }

    // Skip servers that disappeared from LBSM.
    if (timeline_entry->m_DiscoveryIteration != m_DiscoveryIteration)
        return false;

    CNetServer server(m_NetScheduleAPI->m_Service.GetServer(
            timeline_entry->m_ServerAddress));

    timeline_entry->ResetTimeout(READJOB_TIMEOUT);

    try {
        bool no_more_jobs;

        if (x_ReadJob(server, &timeline_entry->GetTimeout(),
                job, job_status, &no_more_jobs)) {
            // A job has been returned; add the server to
            // m_ImmediateActions because there can be more
            // jobs in the queue.
            m_ImmediateActions.Push(timeline_entry);
            *result = no_more_jobs ? CNetScheduleJobReader::eRNJ_NoMoreJobs :
                    CNetScheduleJobReader::eRNJ_JobReady;
            return true;
        } else {
            // No job has been returned by this server;
            // query the server later.
            m_Timeline.Push(timeline_entry);
            if (no_more_jobs) {
                *result = CNetScheduleJobReader::eRNJ_NoMoreJobs;
                return true;
            }
            return false;
        }
    }
    catch (CNetSrvConnException& e) {
        // Because a connection error has occurred, do not
        // put this server back to the timeline.
        LOG_POST(Warning << e.GetMsg());
        return false;
    }
}

void SNetScheduleJobReaderImpl::x_ProcessRequestJobNotification()
{
    CNetServer server;

    if (m_NotificationHandler.CheckRequestJobNotification(
            m_NetScheduleAPI.GetExecutor(), &server))
        x_GetTimelineEntry(server)->MoveTo(&m_ImmediateActions);
}

CNetScheduleJobReader::EReadNextJobResult CNetScheduleJobReader::ReadNextJob(
        CNetScheduleJob* job,
        CNetScheduleAPI::EJobStatus* job_status,
        CTimeout* timeout)
{
    char deadline_object[sizeof(CDeadline)];

    CDeadline* timeout_expiration_time = timeout == NULL ? NULL :
            new (deadline_object) CDeadline(*timeout);

    EReadNextJobResult result;

    for (;;) {
        while (!m_Impl->m_ImmediateActions.IsEmpty()) {
            if (m_Impl->x_PerformTimelineAction(m_Impl->m_ImmediateActions,
                    *job, job_status, &result))
                return result;

            while (!m_Impl->m_Timeline.IsEmpty() && m_Impl->m_Timeline.GetHead()->TimeHasCome())
                m_Impl->m_Timeline.MoveHeadTo(&m_Impl->m_ImmediateActions);

            // Check if there's a notification in the UDP socket.
            while (m_Impl->m_NotificationHandler.ReceiveNotification())
                m_Impl->x_ProcessRequestJobNotification();
        }

        // FIXME Check for interrupt if (CGridGlobals::GetInstance().IsShuttingDown())
            // return eRNJ_Interrupt;

        if (timeout_expiration_time != NULL &&
                timeout_expiration_time->GetRemainingTime().IsZero())
            return eRNJ_Timeout;

        if (!m_Impl->m_Timeline.IsEmpty()) {
            const CDeadline& next_event_time =
                    m_Impl->m_Timeline.GetHead()->GetTimeout();

            if (timeout_expiration_time != NULL &&
                            !(next_event_time < *timeout_expiration_time)) {
                if (!m_Impl->m_NotificationHandler.WaitForNotification(
                        *timeout_expiration_time))
                    return eRNJ_Timeout;
                m_Impl->x_ProcessRequestJobNotification();
            } else if (m_Impl->m_NotificationHandler.WaitForNotification(
                    next_event_time))
                m_Impl->x_ProcessRequestJobNotification();
            else if (m_Impl->x_PerformTimelineAction(m_Impl->m_Timeline,
                    *job, job_status, &result))
                return result;
        }
    }
}

void CNetScheduleJobReader::InterruptReading()
{
    /*try {
        m_Impl->m_Semaphore.Post();
    }
    catch (...) {
    }*/
}

END_NCBI_SCOPE