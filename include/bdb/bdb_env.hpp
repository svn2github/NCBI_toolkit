#ifndef BDB_ENV__HPP
#define BDB_ENV__HPP

/* $Id$
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
 * File Description: Wrapper around Berkeley DB environment
 *
 */

/// @file bdb_env.hpp
/// Wrapper around Berkeley DB environment structure

#include <bdb/bdb_types.hpp>
#include <bdb/bdb_trans.hpp>

#include <stdio.h>

BEGIN_NCBI_SCOPE

/** @addtogroup BDB
 *
 * @{
 */

class CBDB_Transaction;

/// BDB environment object a collection including support for some or 
/// all of caching, locking, logging and transaction subsystems.

class NCBI_BDB_EXPORT CBDB_Env
{
public:
    enum EEnvOptions {
        eThreaded = (1 << 0),          ///< corresponds to DB_THREAD 
        eRunRecovery = (1 << 1),       ///< Run DB recovery first
        eRunRecoveryFatal = (1 << 2),  ///< Run DB recovery first
        ePrivate = (1 << 3)            ///< Create private directory
    };
    
    /// OR-ed combination of EEnvOptions    
    typedef unsigned int  TEnvOpenFlags;
    
public:
    CBDB_Env();

    /// Construct CBDB_Env taking ownership of external DB_ENV. 
    ///
    /// Presumed that env has been opened with all required parameters.
    /// Class takes the ownership on the provided DB_ENV object.
    CBDB_Env(DB_ENV* env);

    ~CBDB_Env();

    /// Open environment
    ///
    /// @param db_home destination directory for the database
    /// @param flags - Berkeley DB flags (see documentation on DB_ENV->open)
    void Open(const string& db_home, int flags);

    /// Open environment with database locking (DB_INIT_LOCK)
    ///
    /// @param db_home destination directory for the database
    void OpenWithLocks(const string& db_home);

    /// Open-create private environment
    void OpenPrivate(const string& db_home);

    /// Open environment with CDB locking (DB_INIT_CDB)
    ///
    /// @param db_home destination directory for the database
    void OpenConcurrentDB(const string& db_home);

    /// Open environment using transaction
    ///
    /// @param db_home 
    ///    destination directory for the database
    /// @param flags
    
    void OpenWithTrans(const string& db_home, TEnvOpenFlags opt = 0);

    /// Open error reporting file for the environment
    ///
    /// @param 
    ///    file_name - name of the error file
    ///    if file_name == "stderr" or "stdout" 
    ///    all errors are redirected to that device
    void OpenErrFile(const string& file_name);

    /// Join the existing environment
    ///
    /// @param 
    ///    db_home destination directory for the database
    /// @param 
    ///    opt environment options (see EEnvOptions)
    /// @sa EEnvOptions
    void JoinEnv(const string& db_home, TEnvOpenFlags opt = 0);

    /// Return underlying DB_ENV structure pointer for low level access.
    DB_ENV* GetEnv() { return m_Env; }

    /// Set cache size for the environment.
    void SetCacheSize(unsigned int cache_size);
    
    /// Start transaction (DB_ENV->txn_begin)
    /// 
    /// @param parent_txn
    ///   Parent transaction
    /// @param flags
    ///   Transaction flags
    /// @return 
    ///   New transaction handler
    DB_TXN* CreateTxn(DB_TXN* parent_txn = 0, unsigned int flags = 0);

    /// Return TRUE if environment has been open as transactional
    bool IsTransactional() const;

    /// Flush the underlying memory pools, logs and data bases
    void TransactionCheckpoint();

    /// Turn off buffering of databases (DB_DIRECT_DB)
    void SetDirectDB(bool on_off);

    /// Turn off buffering of log files (DB_DIRECT_LOG)
    void SetDirectLog(bool on_off);

    /// If set, Berkeley DB will automatically remove log files that are no 
    /// longer needed. (Can make catastrofic recovery impossible).
    void SetLogAutoRemove(bool on_off);

    /// Set maximum size of LOG files
    void SetLogFileMax(unsigned int lg_max);

    /// Set the size of the in-memory log buffer, in bytes.
    void SetLogBSize(unsigned lg_bsize);

    /// Configure environment for non-durable in-memory logging
    void SetLogInMemory(bool on_off);

    bool IsLogInMemory() const { return m_LogInMemory; }

    /// Set max number of locks in the database
    ///
    /// see DB_ENV->set_lk_max_locks for more details
    void SetMaxLocks(unsigned locks);

    /// Get max locks
    unsigned GetMaxLocks();

    /// see DB_ENV->set_lk_max_objects for more details
    void SetMaxLockObjects(unsigned lock_obj_max);

    /// Remove all non-active log files
    void CleanLog();

    /// Set timeout value for locks in microseconds (1 000 000 in sec)
    void SetLockTimeout(unsigned timeout);

    /// Set timeout value for transactions in microseconds (1 000 000 in sec)
    void SetTransactionTimeout(unsigned timeout);

    /// Set number of active transaction
    /// see DB_ENV->set_tx_max for more details
    void SetTransactionMax(unsigned tx_max);

    /// Specify that test-and-set mutexes should spin tas_spins times 
    /// without blocking
    void SetTasSpins(unsigned tas_spins);

    /// Configure the total number of mutexes
    void MutexSetMax(unsigned max);

    /// Get number of mutexes
    unsigned MutexGetMax();

    /// Non-force removal of BDB environment. (data files remains intact).
    /// @return
    ///   FALSE if environment is busy and cannot be deleted
    bool Remove();

    /// Force remove BDB environment.
    void ForceRemove();

    /// Try to Remove the environment, if DB_ENV::remove returns 0, but fails
    /// files ramain on disk anyway calls ForceRemove
    /// @return
    ///   FALSE if environment is busy and cannot be deleted
    bool CheckRemove();

    /// Close the environment;
    void Close();

    /// Reset log sequence number
    void LsnReset(const char* file_name);

    /// Get default syncronicity setting
    CBDB_Transaction::ETransSync GetTransactionSync() const
    {
        return m_TransSync;
    }

    /// Set default syncronicity level
    void SetTransactionSync(CBDB_Transaction::ETransSync sync);

    
private:
    /// Opens BDB environment returns error code
    /// Throws no exceptions.
    int x_Open(const char* db_home, int flags);
    
private:
    CBDB_Env(const CBDB_Env&);
    CBDB_Env& operator=(const CBDB_Env&);
private:
    DB_ENV*  m_Env;
    bool     m_Transactional; ///< TRUE if environment is transactional
    FILE*    m_ErrFile;
    string   m_HomePath;
    bool     m_LogInMemory;
    CBDB_Transaction::ETransSync m_TransSync;

};

/* @} */

END_NCBI_SCOPE

#endif  /* BDB_ENV__HPP */
