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
 * Author:  Aleksey Grichenko
 *
 * File Description:
 *   Wrapper for testing modules in MT environment
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.1  2001/04/06 15:53:08  grichenk
 * Initial revision
 *
 *
 * ===========================================================================
 */

#include "test_mt.hpp"

BEGIN_NCBI_SCOPE


static CFastMutex    s_GlobalLock;
static CThreadedApp* s_Application;

// Default values
unsigned int  s_NumThreads    = 40;
int           s_SpawnBy       = 13;

// Next test thread index
static unsigned int  s_NextIndex     = 0;


/////////////////////////////////////////////////////////////////////////////
// Test thread
//

class CTestThread : public CThread
{
public:
    CTestThread(int id);
protected:
    ~CTestThread(void);
    virtual void* Main(void);
    virtual void  OnExit(void);
private:
    int m_Idx;
};


CTestThread::CTestThread(int idx)
    : m_Idx(idx)
{
    if ( s_Application != 0 )
        s_Verify(s_Application->Thread_Init(m_Idx));
}


CTestThread::~CTestThread(void)
{
    if ( s_Application != 0 )
        s_Verify(s_Application->Thread_Destroy(m_Idx));
}


void CTestThread::OnExit(void)
{
    if ( s_Application != 0 )
        s_Verify(s_Application->Thread_Exit(m_Idx));
}

CRef<CTestThread> thr[c_NumThreadsMax];

void* CTestThread::Main(void)
{
    // Spawn more threads
    for (int i = 0; i<s_SpawnBy; i++) {
        int idx;
        {{
            CFastMutexGuard spawn_guard(s_GlobalLock);
            if (s_NextIndex >= s_NumThreads) {
                break;
            }
            idx = s_NextIndex;
            s_NextIndex++;
        }}
        thr[idx] = new CTestThread(idx);
        thr[idx]->Run();
    }

    // Run the test
    if ( s_Application != 0 )
        s_Verify(s_Application->Thread_Run(m_Idx));

    return 0;
}



/////////////////////////////////////////////////////////////////////////////
//  Test application


CThreadedApp::CThreadedApp(void)
{
    s_Application = this;
}


CThreadedApp::~CThreadedApp(void)
{
    s_Application = 0;
}


void CThreadedApp::Init(void)
{
    // Prepare command line descriptions
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    // sNumThreads
    arg_desc->AddDefaultKey
        ("threads", "NumThreads",
         "Total number of threads to create and run",
         CArgDescriptions::eInteger, NStr::IntToString(s_NumThreads));
    arg_desc->SetConstraint
        ("threads", new CArgAllow_Integers(c_NumThreadsMin, c_NumThreadsMax));

    // sSpawnBy
    arg_desc->AddDefaultKey
        ("spawnby", "SpawnBy",
         "Threads spawning factor",
         CArgDescriptions::eInteger, NStr::IntToString(s_SpawnBy));
    arg_desc->SetConstraint
        ("spawnby", new CArgAllow_Integers(c_SpawnByMin, c_SpawnByMax));


    string prog_description =
        "MT-environment test";
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              prog_description, false);

    SetupArgDescriptions(arg_desc.release());
}


int CThreadedApp::Run(void)
{
    // Process command line
    const CArgs& args = GetArgs();

    s_NumThreads = args["threads"].AsInteger();
    s_SpawnBy    = args["spawnby"].AsInteger();

    s_Verify(TestApp_Init());

    // Create and run threads
    for (int i=0; i<s_SpawnBy; i++) {
        int idx;
        {{
            CFastMutexGuard spawn_guard(s_GlobalLock);
            if (s_NextIndex >= s_NumThreads) {
                break;
            }
            idx = s_NextIndex;
            s_NextIndex++;
        }}

        thr[idx] = new CTestThread(idx);
        thr[idx]->Run();
    }

    // Wait for all threads
    for (unsigned int i=0; i<s_NumThreads; i++) {
        thr[i]->Join();
    }

    s_Verify(TestApp_Exit());

    // Destroy all threads
    for (unsigned int i=0; i<s_NumThreads; i++) {
        thr[i].Reset();
    }

    return 0;
}


bool CThreadedApp::Thread_Init(int /*idx*/)
{
    return true;
}


bool CThreadedApp::Thread_Run(int /*idx*/)
{
    return true;
}


bool CThreadedApp::Thread_Exit(int /*idx*/)
{
    return true;
}


bool CThreadedApp::Thread_Destroy(int /*idx*/)
{
    return true;
}


bool CThreadedApp::TestApp_Init(void)
{
    return true;
}


bool CThreadedApp::TestApp_Exit(void)
{
    return true;
}



END_NCBI_SCOPE
