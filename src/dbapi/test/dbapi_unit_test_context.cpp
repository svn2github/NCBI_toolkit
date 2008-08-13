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
 * Author: Sergey Sikorskiy
 *
 * File Description: DBAPI unit-test
 *
 * ===========================================================================
 */

#include <ncbi_pch.hpp>

#include <dbapi/driver/impl/dbapi_driver_utils.hpp>

#define NCBI_BOOST_NO_AUTO_TEST_MAIN
#include "dbapi_unit_test.hpp"


BEGIN_NCBI_SCOPE

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_SetLogStream)
{
    try {
        CNcbiOfstream logfile("dbapi_unit_test.log");

        GetDS().SetLogStream(&logfile);
        GetDS().SetLogStream(&logfile);

        // Test block ...
        {
            // No errors ...
            {
                auto_ptr<IConnection> auto_conn(GetDS().CreateConnection());
                BOOST_CHECK(auto_conn.get() != NULL);

                auto_conn->Connect(GetArgs().GetConnParams());

                auto_ptr<IStatement> auto_stmt(auto_conn->GetStatement());
                auto_stmt->SendSql("SELECT name FROM sysobjects");
                BOOST_CHECK(auto_stmt->HasMoreResults());
                DumpResults(auto_stmt.get());
            }

            GetDS().SetLogStream(&logfile);

            // Force errors ...
            {
                auto_ptr<IConnection> auto_conn(GetDS().CreateConnection());
                BOOST_CHECK(auto_conn.get() != NULL);

                auto_conn->Connect(GetArgs().GetConnParams());

                auto_ptr<IStatement> auto_stmt(auto_conn->GetStatement());
                try {
                    auto_stmt->SendSql("SELECT oops FROM sysobjects");
                    BOOST_CHECK( auto_stmt->HasMoreResults() );
                } catch(const CDB_Exception&) {
                    auto_stmt->Cancel();
                }
            }

            GetDS().SetLogStream(&logfile);
        }

        // Install user-defined error handler (eTakeOwnership)
        {
            {
                I_DriverContext* drv_context = GetDS().GetDriverContext();

                if (GetArgs().IsODBCBased()) {
                    drv_context->PushCntxMsgHandler(new CDB_UserHandler_Exception_ODBC,
                                                    eTakeOwnership
                                                    );
                    drv_context->PushDefConnMsgHandler(new CDB_UserHandler_Exception_ODBC,
                                                       eTakeOwnership
                                                       );
                } else {
                    CRef<CDB_UserHandler> hx(new CDB_UserHandler_Exception);
                    CDB_UserHandler::SetDefault(hx);
                    drv_context->PushCntxMsgHandler(new CDB_UserHandler_Exception,
                                                    eTakeOwnership);
                    drv_context->PushDefConnMsgHandler(new CDB_UserHandler_Exception,
                                                       eTakeOwnership);
                }
            }

            // Test block ...
            {
                // No errors ...
                {
                    auto_ptr<IConnection> auto_conn(GetDS().CreateConnection());
                    BOOST_CHECK(auto_conn.get() != NULL);

                    auto_conn->Connect(GetArgs().GetConnParams());

                    auto_ptr<IStatement> auto_stmt(auto_conn->GetStatement());
                    auto_stmt->SendSql("SELECT name FROM sysobjects");
                    BOOST_CHECK(auto_stmt->HasMoreResults());
                    DumpResults(auto_stmt.get());
                }

                GetDS().SetLogStream(&logfile);

                // Force errors ...
                {
                    auto_ptr<IConnection> auto_conn(GetDS().CreateConnection());
                    BOOST_CHECK(auto_conn.get() != NULL);

                    auto_conn->Connect(GetArgs().GetConnParams());

                    auto_ptr<IStatement> auto_stmt(auto_conn->GetStatement());
                    try {
                        auto_stmt->SendSql("SELECT oops FROM sysobjects");
                        BOOST_CHECK( auto_stmt->HasMoreResults() );
                    } catch(const CDB_Exception&) {
                        auto_stmt->Cancel();
                    }
                }

                GetDS().SetLogStream(&logfile);
            }
        }

        // Install user-defined error handler (eNoOwnership)
        {
            CMsgHandlerGuard handler_guard(*GetDS().GetDriverContext());

            // Test block ...
            {
                // No errors ...
                {
                    auto_ptr<IConnection> auto_conn(GetDS().CreateConnection());
                    BOOST_CHECK(auto_conn.get() != NULL);

                    auto_conn->Connect(GetArgs().GetConnParams());

                    auto_ptr<IStatement> auto_stmt(auto_conn->GetStatement());
                    auto_stmt->SendSql("SELECT name FROM sysobjects");
                    BOOST_CHECK(auto_stmt->HasMoreResults());
                    DumpResults(auto_stmt.get());
                }

                GetDS().SetLogStream(&logfile);

                // Force errors ...
                {
                    auto_ptr<IConnection> auto_conn(GetDS().CreateConnection());
                    BOOST_CHECK(auto_conn.get() != NULL);

                    auto_conn->Connect(GetArgs().GetConnParams());

                    auto_ptr<IStatement> auto_stmt(auto_conn->GetStatement());
                    try {
                        auto_stmt->SendSql("SELECT oops FROM sysobjects");
                        BOOST_CHECK( auto_stmt->HasMoreResults() );
                    } catch(const CDB_Exception&) {
                        auto_stmt->Cancel();
                    }
                }

                GetDS().SetLogStream(&logfile);
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}



END_NCBI_SCOPE

