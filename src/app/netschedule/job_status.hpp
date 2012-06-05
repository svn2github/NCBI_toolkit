#ifndef NETSCHEDULE_JOB_STATUS__HPP
#define NETSCHEDULE_JOB_STATUS__HPP

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
 * Authors:  Anatoliy Kuznetsov, Victor Joukov
 *
 * File Description:
 *   Net schedule job status states.
 *
 */

/// @file job_status.hpp
/// NetSchedule job status tracker.
///
/// @internal

#include <map>

#include <corelib/ncbimtx.hpp>

#include "ns_types.hpp"

BEGIN_NCBI_SCOPE


const CNetScheduleAPI::EJobStatus
        g_ValidJobStatuses[] = { CNetScheduleAPI::ePending,
                                 CNetScheduleAPI::eRunning,
                                 CNetScheduleAPI::eCanceled,
                                 CNetScheduleAPI::eFailed,
                                 CNetScheduleAPI::eDone,
                                 CNetScheduleAPI::eReading,
                                 CNetScheduleAPI::eConfirmed,
                                 CNetScheduleAPI::eReadFailed };
const size_t
        g_ValidJobStatusesSize = sizeof(g_ValidJobStatuses) /
                                 sizeof(CNetScheduleAPI::EJobStatus);



/// In-Memory storage to track status of all jobs
/// Syncronized thread safe class
///
/// @internal
class CJobStatusTracker
{
public:
    typedef vector<TNSBitVector*> TStatusStorage;

    /// Status to number of jobs map
    typedef map<TJobStatus, unsigned> TStatusSummaryMap;

public:
    CJobStatusTracker();
    ~CJobStatusTracker();

    TJobStatus GetStatus(unsigned job_id) const;

    /// Add closed interval of ids to pending status
    void AddPendingBatch(unsigned job_id_from, unsigned job_id_to);

    unsigned int  GetPendingJobFromSet(const TNSBitVector &  candidate_set);

    /// Provides a job id (or 0 if none) which is in the given state and is not
    /// in the unwanted jobs list
    unsigned int  GetJobByStatus(TJobStatus            status,
                                 const TNSBitVector &  unwanted_jobs,
                                 const TNSBitVector &  group_jobs) const;

    TNSBitVector  GetJobs(const vector<CNetScheduleAPI::EJobStatus> &  statuses) const;
    TNSBitVector  GetJobs(CNetScheduleAPI::EJobStatus  status) const;

    /// Logical AND of candidates and pending jobs
    /// (candidate_set &= pending_set)
    void PendingIntersect(TNSBitVector* candidate_set) const;

    /// Logical AND with statuses ORed cleans up a list from non-existing
    /// jobs
    void GetAliveJobs(TNSBitVector& ids);

    /// TRUE if we have pending jobs
    bool AnyPending() const;

     /// Get next job in the specified status, or first if job_id is 0
    unsigned GetNext(TJobStatus status, unsigned job_id) const;

    /// Set job status without logic control.
    /// @param status
    ///     Status to set (all other statuses are cleared)
    ///     Non existing status code clears all statuses
    void SetStatus(unsigned   job_id,
                   TJobStatus status);

    void AddPendingJob(unsigned int  job_id);

    // Erase the job
    void Erase(unsigned job_id);

    /// Set job status without any protection
    void SetExactStatusNoLock(
        unsigned   job_id,
        TJobStatus status,
        bool       set_clear);

    // Return number of jobs in specified status/statuses
    unsigned int  CountStatus(TJobStatus  status) const;
    unsigned int  CountStatus(const vector<CNetScheduleAPI::EJobStatus> &  statuses) const;

    // Count all jobs in any status
    unsigned int  Count(void) const;

    bool  AnyJobs(void) const;
    bool  AnyJobs(TJobStatus  status) const;
    bool  AnyJobs(const vector<CNetScheduleAPI::EJobStatus> &  statuses) const;

    void StatusStatistics(TJobStatus                status,
                          TNSBitVector::statistics* st) const;

    /// Specified status is OR-ed with the target vector
    void StatusSnapshot(TJobStatus    status,
                        TNSBitVector* bv) const;

    /// Clear status storage
    ///
    /// @param bv
    ///    If not NULL all ids from the matrix are OR-ed with this vector
    ///    (bv is not cleared)
    void ClearAll(TNSBitVector* bv = 0);

    /// Optimize bitvectors memory
    void OptimizeMem();

private:
    TJobStatus x_GetStatusNoLock(unsigned job_id) const;

    /// Check if job is in specified status
    /// @return -1 if no, status value otherwise
    TJobStatus
    x_IsStatusNoLock(unsigned   job_id,
                     TJobStatus st1,
                     TJobStatus st2 = CNetScheduleAPI::eJobNotFound,
                     TJobStatus st3 = CNetScheduleAPI::eJobNotFound
        ) const;

    void x_ReportInvalidStatus(unsigned   job_id,
                               TJobStatus status,
                               TJobStatus old_status);
    void x_SetClearStatusNoLock(unsigned   job_id,
                                TJobStatus status,
                                TJobStatus old_status);
    void x_IncDoneJobs(void);

private:
    CJobStatusTracker(const CJobStatusTracker&);
    CJobStatusTracker& operator=(const CJobStatusTracker&);

private:
    TStatusStorage          m_StatusStor;
    mutable CRWLock         m_Lock;

    // Done jobs counter
    unsigned                m_DoneCnt;

};


END_NCBI_SCOPE

#endif /* NETSCHEDULE_JOB_STATUS__HPP */

