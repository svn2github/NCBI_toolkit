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
* Author:  Sergey Sikorskiy
*
*/

#include <ncbi_pch.hpp>

#include <dbapi/driver/dbapi_conn_factory.hpp>
#include <dbapi/driver/dbapi_svc_mapper.hpp>
#include <dbapi/driver/public.hpp>
#include <dbapi/error_codes.hpp>
#include <corelib/ncbiapp.hpp>


#define NCBI_USE_ERRCODE_X   Dbapi_ConnFactory

BEGIN_NCBI_SCOPE


///////////////////////////////////////////////////////////////////////////////
CDBConnectionFactory::CDBConnectionFactory(IDBServiceMapper::TFactory svc_mapper_factory,
                                           const IRegistry* registry,
                                           EDefaultMapping def_mapping) :
m_MapperFactory(svc_mapper_factory, registry, def_mapping),
m_MaxNumOfConnAttempts(1),
m_MaxNumOfValidationAttempts(1),
m_MaxNumOfServerAlternatives(32),
m_MaxNumOfDispatches(0),
m_ConnectionTimeout(0),
m_LoginTimeout(0)
{
    ConfigureFromRegistry(registry);
}

CDBConnectionFactory::~CDBConnectionFactory(void)
{
}

void
CDBConnectionFactory::ConfigureFromRegistry(const IRegistry* registry)
{
    const string section_name("DB_CONNECTION_FACTORY");

    // Get current registry ...
    if (!registry && CNcbiApplication::Instance()) {
        registry = &CNcbiApplication::Instance()->GetConfig();
    }

    if (registry) {
        m_MaxNumOfConnAttempts =
            registry->GetInt(section_name, "MAX_CONN_ATTEMPTS", 1);
        m_MaxNumOfValidationAttempts =
            registry->GetInt(section_name, "MAX_VALIDATION_ATTEMPTS", 1);
        m_MaxNumOfServerAlternatives =
            registry->GetInt(section_name, "MAX_SERVER_ALTERNATIVES", 32);
        m_MaxNumOfDispatches =
            registry->GetInt(section_name, "MAX_DISPATCHES", 0);
        m_ConnectionTimeout =
            registry->GetInt(section_name, "CONNECTION_TIMEOUT", 0);
        m_LoginTimeout =
            registry->GetInt(section_name, "LOGIN_TIMEOUT", 0);
    } else {
        m_MaxNumOfConnAttempts = 1;
        m_MaxNumOfServerAlternatives = 32;
        m_MaxNumOfDispatches = 0;
        m_ConnectionTimeout = 0;
        m_LoginTimeout = 0;
    }
}

void
CDBConnectionFactory::Configure(const IRegistry* registry)
{
    CFastMutexGuard mg(m_Mtx);

    ConfigureFromRegistry(registry);
}

void
CDBConnectionFactory::SetMaxNumOfConnAttempts(unsigned int max_num)
{
    CFastMutexGuard mg(m_Mtx);

    m_MaxNumOfConnAttempts = max_num;
}

void
CDBConnectionFactory::SetMaxNumOfValidationAttempts(unsigned int max_num)
{
    CFastMutexGuard mg(m_Mtx);

    m_MaxNumOfValidationAttempts = max_num;
}

void
CDBConnectionFactory::SetMaxNumOfServerAlternatives(unsigned int max_num)
{
    CFastMutexGuard mg(m_Mtx);

    m_MaxNumOfServerAlternatives = max_num;
}

void
CDBConnectionFactory::SetMaxNumOfDispatches(unsigned int max_num)
{
    CFastMutexGuard mg(m_Mtx);

    m_MaxNumOfDispatches = max_num;
}

void
CDBConnectionFactory::SetConnectionTimeout(unsigned int timeout)
{
    CFastMutexGuard mg(m_Mtx);

    m_ConnectionTimeout = timeout;
}

void
CDBConnectionFactory::SetLoginTimeout(unsigned int timeout)
{
    CFastMutexGuard mg(m_Mtx);

    m_LoginTimeout = timeout;
}

unsigned int
CDBConnectionFactory::CalculateConnectionTimeout
(const I_DriverContext& ctx) const
{
    unsigned int timeout = 3;

    if (GetConnectionTimeout()) {
        timeout = GetConnectionTimeout();
    } else if (ctx.GetTimeout()) {
        timeout = ctx.GetTimeout();
    }

    return timeout;
}

unsigned int
CDBConnectionFactory::CalculateLoginTimeout(const I_DriverContext& ctx) const
{
    unsigned int timeout = 3;

    if (GetLoginTimeout()) {
        timeout = GetLoginTimeout();
    } else if (ctx.GetLoginTimeout()) {
        timeout = ctx.GetLoginTimeout();
    }

    return timeout;
}

CDBConnectionFactory::CRuntimeData&
CDBConnectionFactory::GetRuntimeData(IConnValidator* validator)
{
    string validator_name;

    if (validator) {
        validator_name = validator->GetName();
    }

    TValidatorSet::iterator it = m_ValidatorSet.find(validator_name);
    if (it != m_ValidatorSet.end()) {
        return it->second;
    }

    return m_ValidatorSet.insert(TValidatorSet::value_type(
        validator_name,
        CRuntimeData(*this, CRef<IDBServiceMapper>(m_MapperFactory.Make()))
        )).first->second;
}

CDB_Connection*
CDBConnectionFactory::MakeDBConnection(
    I_DriverContext& ctx,
    const I_DriverContext::SConnAttr& conn_attr,
    IConnValidator* validator)
{
    CFastMutexGuard mg(m_Mtx);

    CDB_Connection* t_con = NULL;
    CRuntimeData& rt_data = GetRuntimeData(validator);
    TSvrRef dsp_srv = rt_data.GetDispatchedServer(conn_attr.srv_name);

    // Store original query timeout ...
    unsigned int query_timeout = ctx.GetTimeout();

    // Set "validation timeouts" ...
    ctx.SetTimeout(CalculateConnectionTimeout(ctx));
    ctx.SetLoginTimeout(CalculateLoginTimeout(ctx));

    if ( dsp_srv.Empty() ) {
        // We are here either because server name was never dispatched or
        // because a named coonnection pool has been used before.
        // Dispatch server name ...

        t_con = DispatchServerName(ctx, conn_attr, validator);
    } else {
        // Server name is already dispatched ...

        // We probably need to redispatch it ...
        if (GetMaxNumOfDispatches() &&
            rt_data.GetNumOfDispatches(conn_attr.srv_name) >= GetMaxNumOfDispatches()) {
            // We definitely need to redispatch it ...

            // Clean previous info ...
            rt_data.SetDispatchedServer(conn_attr.srv_name, TSvrRef());
            t_con = DispatchServerName(ctx, conn_attr, validator);
        } else {
            // We do not need to redispatch it ...

            IConnValidator::EConnStatus conn_status =
                IConnValidator::eInvalidConn;

            // Try to connect.
            try {
                I_DriverContext::SConnAttr cur_conn_attr(conn_attr);

                cur_conn_attr.srv_name = dsp_srv->GetName();

                // MakeValidConnection may return NULL here because a newly
                // created connection may not pass validation.
                t_con = MakeValidConnection(ctx,
                                            cur_conn_attr,
                                            validator,
                                            conn_status);

            } catch(const CDB_Exception& ex) {
                if (validator) {
                    conn_status = validator->ValidateException(ex);
                }
            }

            if (!t_con) {
                // We couldn't connect ...

                // Server might be temporarily unavailable ...
                // Check conn_status ...
                if (conn_status == IConnValidator::eTempInvalidConn) {
                    rt_data.IncNumOfValidationFailures(conn_attr.srv_name,
                                                       dsp_srv);
                }

                // Redispach ...
                t_con = DispatchServerName(ctx, conn_attr, validator);
            } else {
                // Dispatched server is already set, but calling of this method
                // will increase number of succesful dispatches.
                rt_data.SetDispatchedServer(conn_attr.srv_name, dsp_srv);
            }
        }
    }

    // Restore original connection timeout ...
    ctx.SetTimeout(query_timeout);

    // Restore original query timeout ...
    // The method below is NOT supported by almost all drivers.
    // if (t_con) {
    //     t_con->SetTimeout(query_timeout);
    // }

    return t_con;
}

CDB_Connection*
CDBConnectionFactory::DispatchServerName(
    I_DriverContext& ctx,
    const I_DriverContext::SConnAttr& conn_attr,
    IConnValidator* validator)
{
    CDB_Connection* t_con = NULL;
    I_DriverContext::SConnAttr curr_conn_attr(conn_attr);

    CRuntimeData& rt_data = GetRuntimeData(validator);

    // Try to connect up to a given number of alternative servers ...
    unsigned int alternatives = GetMaxNumOfServerAlternatives();
    for ( ; !t_con && alternatives > 0; --alternatives ) {
        TSvrRef dsp_srv;

        // It is possible that a server name won't be provided.
        // This is possible when somebody uses a named connection pool.
        // In this case we even won't try to map it.
        if (!conn_attr.srv_name.empty()) {
            dsp_srv = rt_data.GetDBServiceMapper().GetServer(conn_attr.srv_name);

            if (dsp_srv.Empty()) {
                return NULL;
            }

            curr_conn_attr.srv_name = dsp_srv->GetName();
        } else if (conn_attr.pool_name.empty()) {
            DATABASE_DRIVER_ERROR
                ("Neither server name nor pool name provided.", 111000);
        }

        // Try to connect up to a given number of attempts ...
        unsigned int attepmpts = GetMaxNumOfConnAttempts();
        IConnValidator::EConnStatus conn_status =
            IConnValidator::eInvalidConn;

        // We don't check value of conn_status inside of a loop below by design.
        for (; !t_con && attepmpts > 0; --attepmpts) {
            try {
                t_con = MakeValidConnection(ctx,
                                            curr_conn_attr,
                                            validator,
                                            conn_status);
            } catch(const CDB_Exception& ex) {
                if (validator) {
                    conn_status = validator->ValidateException(ex);
                }
            }
        }

        if (!t_con) {
            // Restore previous server name.
            curr_conn_attr.srv_name = conn_attr.srv_name;

            // Server might be temporarily unavailable ...
            // Check conn_status ...
            if (conn_status == IConnValidator::eTempInvalidConn) {
                rt_data.IncNumOfValidationFailures(conn_attr.srv_name, dsp_srv);
            } else {
                // conn_status == IConnValidator::eInvalidConn
                 rt_data.GetDBServiceMapper().Exclude(conn_attr.srv_name, dsp_srv);
            }
        } else {
            rt_data.SetDispatchedServer(conn_attr.srv_name, dsp_srv);
        }
    }

    return t_con;
}

CDB_Connection*
CDBConnectionFactory::MakeValidConnection(
    I_DriverContext& ctx,
    const I_DriverContext::SConnAttr& conn_attr,
    IConnValidator* validator,
    IConnValidator::EConnStatus& conn_status) const
{
    auto_ptr<CDB_Connection> conn(CtxMakeConnection(ctx, conn_attr));

    if (conn.get() && validator) {
        try {
            conn_status = validator->Validate(*conn);
            if (conn_status != IConnValidator::eValidConn) {
                return NULL;
            }
        } catch (const CDB_Exception& ex) {
            conn_status = validator->ValidateException(ex);
            if (conn_status != IConnValidator::eValidConn) {
                return NULL;
            }
        } catch (const CException& ex) {
            ERR_POST_X(1, Warning << ex.ReportAll() << " when trying to connect to "
                       << "server '" << conn_attr.srv_name << "' as user '"
                       << conn_attr.user_name << "'");
            return NULL;
        } catch (...) {
            ERR_POST_X(2, Warning << "Unknown exception when trying to connect to "
                       << "server '" << conn_attr.srv_name << "' as user '"
                       << conn_attr.user_name << "'");
            throw;
        }
    }
    return conn.release();
}



///////////////////////////////////////////////////////////////////////////////
CDBConnectionFactory::CRuntimeData::CRuntimeData(
    const CDBConnectionFactory& parent,
    const CRef<IDBServiceMapper>& mapper
    ) :
m_Parent(&parent),
m_DBServiceMapper(mapper)
{
}

TSvrRef
CDBConnectionFactory::CRuntimeData::GetDispatchedServer(
    const string& service_name
    )
{
    return m_DispatchedSet[service_name];
}

void
CDBConnectionFactory::CRuntimeData::SetDispatchedServer(
    const string& service_name,
    const TSvrRef& server
    )
{
    if (server.Empty()) {
        m_DispatchNumMap[service_name] = 0;
    } else {
        ++m_DispatchNumMap[service_name];
    }

    m_DispatchedSet[service_name] = server;
}

unsigned int
CDBConnectionFactory::CRuntimeData::GetNumOfDispatches(
    const string& service_name
    )
{
    return m_DispatchNumMap[service_name];
}

unsigned int
CDBConnectionFactory::CRuntimeData::GetNumOfValidationFailures(
    const string& service_name
    )
{
    return m_ValidationFailureMap[service_name];
}

void
CDBConnectionFactory::CRuntimeData::IncNumOfValidationFailures(
    const string& server_name,
    const TSvrRef& dsp_srv
    )
{
    ++m_ValidationFailureMap[server_name];

    if (GetParent().GetMaxNumOfValidationAttempts() &&
        GetNumOfValidationFailures(server_name) >=
            GetParent().GetMaxNumOfValidationAttempts()) {
        // It is time to finish with this server ...
        GetDBServiceMapper().Exclude(server_name, dsp_srv);
    }
}

///////////////////////////////////////////////////////////////////////////////
CDBConnectionFactory::CMapperFactory::CMapperFactory(
    IDBServiceMapper::TFactory svc_mapper_factory,
    const IRegistry* registry,
    EDefaultMapping def_mapping
    ) :
    m_SvcMapperFactory(svc_mapper_factory),
    m_Registry(registry),
    m_DefMapping(def_mapping)
{
    CHECK_DRIVER_ERROR(!m_SvcMapperFactory && def_mapping != eUseDefaultMapper,
                       "Database service name to server name mapper was not "
                       "defined properly.",
                       0);
}


IDBServiceMapper*
CDBConnectionFactory::CMapperFactory::Make(void) const
{
    if (m_DefMapping == eUseDefaultMapper) {
        CRef<CDBServiceMapperCoR> mapper(new CDBServiceMapperCoR());

        mapper->Push(CRef<IDBServiceMapper>(new CDBDefaultServiceMapper()));
        if (m_SvcMapperFactory) {
            mapper->Push(CRef<IDBServiceMapper>(m_SvcMapperFactory(m_Registry)));
        }

        return mapper.Release();
    } else {
        if (m_SvcMapperFactory) {
            return m_SvcMapperFactory(m_Registry);
        }
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
CDBGiveUpFactory::CDBGiveUpFactory(IDBServiceMapper::TFactory svc_mapper_factory,
                                   const IRegistry* registry,
                                   EDefaultMapping def_mapping)
: CDBConnectionFactory(svc_mapper_factory, registry, def_mapping)
{
    SetMaxNumOfConnAttempts(1); // This value is supposed to be default.
    SetMaxNumOfServerAlternatives(1); // Do not try other servers.
}

CDBGiveUpFactory::~CDBGiveUpFactory(void)
{
}

///////////////////////////////////////////////////////////////////////////////
CDBRedispatchFactory::CDBRedispatchFactory(IDBServiceMapper::TFactory svc_mapper_factory,
                                           const IRegistry* registry,
                                           EDefaultMapping def_mapping)
: CDBConnectionFactory(svc_mapper_factory, registry, def_mapping)
{
    SetMaxNumOfDispatches(1);
    SetMaxNumOfValidationAttempts(0); // Unlimited ...
}

CDBRedispatchFactory::~CDBRedispatchFactory(void)
{
}


///////////////////////////////////////////////////////////////////////////////
CConnValidatorCoR::CConnValidatorCoR(void)
{
}

CConnValidatorCoR::~CConnValidatorCoR(void)
{
}

IConnValidator::EConnStatus
CConnValidatorCoR::Validate(CDB_Connection& conn)
{
    CFastMutexGuard mg(m_Mtx);

    NON_CONST_ITERATE(TValidators, vr_it, m_Validators) {
        if ((*vr_it)->Validate(conn) == eInvalidConn) {
            return eInvalidConn;
        }
    }
    return eValidConn;
}

string
CConnValidatorCoR::GetName(void) const
{
    string result("CConnValidatorCoR");

    CFastMutexGuard mg(m_Mtx);

    ITERATE(TValidators, vr_it, m_Validators) {
        result += (*vr_it)->GetName();
    }

    return result;
}

void
CConnValidatorCoR::Push(const CRef<IConnValidator>& validator)
{
    if (validator.NotNull()) {
        CFastMutexGuard mg(m_Mtx);

        m_Validators.push_back(validator);
    }
}

void
CConnValidatorCoR::Pop(void)
{
    CFastMutexGuard mg(m_Mtx);

    m_Validators.pop_back();
}

CRef<IConnValidator>
CConnValidatorCoR::Top(void) const
{
    CFastMutexGuard mg(m_Mtx);

    return m_Validators.back();
}

bool
CConnValidatorCoR::Empty(void) const
{
    CFastMutexGuard mg(m_Mtx);

    return m_Validators.empty();
}

///////////////////////////////////////////////////////////////////////////////
CTrivialConnValidator::CTrivialConnValidator(const string& db_name,
                                             int attr) :
m_DBName(db_name),
m_Attr(attr)
{
}

CTrivialConnValidator::~CTrivialConnValidator(void)
{
}

IConnValidator::EConnStatus
CTrivialConnValidator::Validate(CDB_Connection& conn)
{
    // Try to change a database ...
    {
        auto_ptr<CDB_LangCmd> set_cmd(conn.LangCmd("use " + GetDBName()));
        set_cmd->Send();
        set_cmd->DumpResults();
    }

    if (GetAttr() & eCheckSysobjects) {
        auto_ptr<CDB_LangCmd> set_cmd(conn.LangCmd("SELECT id FROM sysobjects"));
        set_cmd->Send();
        set_cmd->DumpResults();
    }

    // Go back to the original (master) database ...
    if (GetAttr() & eRestoreDefaultDB) {
        auto_ptr<CDB_LangCmd> set_cmd(conn.LangCmd("use master"));
        set_cmd->Send();
        set_cmd->DumpResults();
    }

    // All exceptions are supposed to be caught and processed by
    // CDBConnectionFactory ...
    return eValidConn;
}

string
CTrivialConnValidator::GetName(void) const
{
    string result("CTrivialConnValidator");

    result += (GetAttr() == eCheckSysobjects ? "CSO" : "");
    result += GetDBName();

    return result;
}

END_NCBI_SCOPE
