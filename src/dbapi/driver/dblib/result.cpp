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
 * Author:  Vladimir Soussov
 *
 * File Description:  DBLib Results
 *
 */
#include <ncbi_pch.hpp>

#include <dbapi/driver/dblib/interfaces.hpp>
#include <dbapi/driver/dblib/interfaces_p.hpp>

#include <dbapi/driver/util/numeric_convert.hpp>


// SYBUNIQUE is defined in <tds.h>
// Unfortunately we cannot include <tds.h> because of conflicts with <sybdb.h>
#ifdef FTDS_IN_USE
#define SYBUNIQUE 36
#endif

BEGIN_NCBI_SCOPE


// Aux. for s_*GetItem()
static CDB_Object* s_GenericGetItem(EDB_Type data_type, CDB_Object* item_buff,
                                    EDB_Type b_type, const BYTE* d_ptr, DBINT d_len)
{
    switch (data_type) {
    case eDB_VarBinary: {
        if (item_buff) {
            switch (b_type) {
            case eDB_VarBinary:
                ((CDB_VarBinary*) item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_Binary:
                ((CDB_Binary*)    item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_VarChar:
                ((CDB_VarChar*)   item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_Char:
                ((CDB_Char*)      item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_LongBinary:
                ((CDB_LongBinary*)item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_LongChar:
                ((CDB_LongChar*)  item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            default:
                DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
            }
            return item_buff;
        }

        return d_ptr ? new CDB_VarBinary((const void*) d_ptr, (size_t) d_len)
            : new CDB_VarBinary();
    }

    case eDB_Bit: {
        DBBIT* v = (DBBIT*) d_ptr;
        if (item_buff) {
            if (v) {
                switch (b_type) {
                case eDB_Bit:
                    *((CDB_Bit*)      item_buff) = (int) *v;
                    break;
                case eDB_TinyInt:
                    *((CDB_TinyInt*)  item_buff) = *v ? 1 : 0;
                    break;
                case eDB_SmallInt:
                    *((CDB_SmallInt*) item_buff) = *v ? 1 : 0;
                    break;
                case eDB_Int:
                    *((CDB_Int*)      item_buff) = *v ? 1 : 0;
                    break;
                default:
                    DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                }
            } else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_Bit((int) *v) : new CDB_Bit;
    }

    case eDB_VarChar: {
        if (item_buff) {
            switch (b_type) {
            case eDB_VarChar:
                ((CDB_VarChar*)   item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_Char:
                ((CDB_Char*)      item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_LongChar:
                ((CDB_LongChar*)  item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_VarBinary:
                ((CDB_VarBinary*) item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_Binary:
                ((CDB_Binary*)    item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_LongBinary:
                ((CDB_LongBinary*)item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            default:
                DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
            }
            return item_buff;
        }

        return d_ptr ? new CDB_VarChar((const char*) d_ptr, (size_t) d_len)
            : new CDB_VarChar();
    }

    case eDB_DateTime: {
        DBDATETIME* v = (DBDATETIME*) d_ptr;
        if (item_buff) {
            if (b_type != eDB_DateTime) {
                DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
            }
            if (v)
                ((CDB_DateTime*) item_buff)->Assign(v->dtdays, v->dttime);
            else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_DateTime(v->dtdays, v->dttime) : new CDB_DateTime;
    }

    case eDB_SmallDateTime: {
        DBDATETIME4* v = (DBDATETIME4*)d_ptr;
        if (item_buff) {
            if (v) {
                switch (b_type) {
                case eDB_SmallDateTime:
                    ((CDB_SmallDateTime*) item_buff)->Assign
                        (DBDATETIME4_days(v),             DBDATETIME4_mins(v));
                    break;
                case eDB_DateTime:
                    ((CDB_DateTime*)      item_buff)->Assign
                        ((int) DBDATETIME4_days(v), (int) DBDATETIME4_mins(v)*60*300);
                    break;
                default:
                    DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                }
            } else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ?
            new CDB_SmallDateTime(DBDATETIME4_days(v), DBDATETIME4_mins(v)) : new CDB_SmallDateTime;
    }

    case eDB_TinyInt: {
        DBTINYINT* v = (DBTINYINT*) d_ptr;
        if (item_buff) {
            if (v) {
                switch (b_type) {
                case eDB_TinyInt:
                    *((CDB_TinyInt*)  item_buff) = (Uint1) *v;
                    break;
                case eDB_SmallInt:
                    *((CDB_SmallInt*) item_buff) = (Int2)  *v;
                    break;
                case eDB_Int:
                    *((CDB_Int*)      item_buff) = (Int4)  *v;
                    break;
                default:
                    DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                }
            } else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_TinyInt((Uint1) *v) : new CDB_TinyInt;
    }

    case eDB_SmallInt: {
        DBSMALLINT* v = (DBSMALLINT*) d_ptr;
        if (item_buff) {
            if (v) {
                switch (b_type) {
                case eDB_SmallInt:
                    *((CDB_SmallInt*) item_buff) = (Int2) *v;
                    break;
                case eDB_Int:
                    *((CDB_Int*)      item_buff) = (Int4) *v;
                        break;
                default:
                    DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                }
            } else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_SmallInt((Int2) *v) : new CDB_SmallInt;
    }

    case eDB_Int: {
        DBINT* v = (DBINT*) d_ptr;
        if (item_buff) {
            if (v) {
                if (b_type == eDB_Int)
                    *((CDB_Int*) item_buff) = (Int4) *v;
                else {
                    DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                }
            } else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_Int((Int4) *v) : new CDB_Int;
    }

    case eDB_Double: {
        if (item_buff && b_type != eDB_Double) {
            DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 130020 );
        }
        DBFLT8* v = (DBFLT8*) d_ptr;
        if (item_buff) {
            if (v)
                *((CDB_Double*) item_buff) = (double) *v;
            else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_Double((double)*v) : new CDB_Double;
    }

    case eDB_Float: {
        if (item_buff && b_type != eDB_Float) {
            DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 130020 );
        }
        DBREAL* v = (DBREAL*) d_ptr;
        if (item_buff) {
            if (v)
                *((CDB_Float*) item_buff) = (float) *v;
            else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ? new CDB_Float((float)*v) : new CDB_Float;
    }

    case eDB_LongBinary: {
        if (item_buff) {
            switch (b_type) {
            case eDB_LongBinary:
                ((CDB_LongBinary*)item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_LongChar:
                ((CDB_LongChar*)  item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;

            default:
                DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
            }
            return item_buff;
        }

        return d_ptr ? new CDB_LongBinary((size_t) d_len, (const void*) d_ptr,
                                          (size_t) d_len)
            : new CDB_LongBinary();
    }

    case eDB_LongChar: {
        if (item_buff) {
            switch (b_type) {
            case eDB_LongChar:
                ((CDB_LongChar*)  item_buff)->SetValue((const char*) d_ptr,
                                                       (size_t) d_len);
                break;
            case eDB_LongBinary:
                ((CDB_LongBinary*)item_buff)->SetValue((const void*) d_ptr,
                                                       (size_t) d_len);
                break;
            default:
                DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
            }
            return item_buff;
        }

        return d_ptr ? new CDB_LongChar((size_t) d_len, (const char*) d_ptr)
            : new CDB_LongChar();
    }

    default:
        return 0;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//  CDBL_RowResult::
//


CDBL_RowResult::CDBL_RowResult(CDBL_Connection& conn,
                               DBPROCESS* cmd,
                               unsigned int* res_status,
                               bool need_init) :
    CDBL_Result(conn, cmd),
    m_CurrItem(-1),
    m_EOR(false),
    m_ResStatus(res_status),
    m_Offset(0)
{
    if (!need_init)
        return;

    m_NofCols = Check(dbnumcols(cmd));
    m_CmdNum = DBCURCMD(cmd);

    m_ColFmt = new SDBL_ColDescr[m_NofCols];
    for (unsigned int n = 0; n < m_NofCols; n++) {
        m_ColFmt[n].max_length = Check(dbcollen(GetCmd(), n + 1));
        m_ColFmt[n].data_type = GetDataType(n + 1);
        const char* s = Check(dbcolname(GetCmd(), n + 1));
        m_ColFmt[n].col_name = s ? s : "";
    }
}


EDB_ResType CDBL_RowResult::ResultType() const
{
    return eDB_RowResult;
}


unsigned int CDBL_RowResult::NofItems() const
{
    return m_NofCols;
}


const char* CDBL_RowResult::ItemName(unsigned int item_num) const
{
    return item_num < m_NofCols ? m_ColFmt[item_num].col_name.c_str() : 0;
}


size_t CDBL_RowResult::ItemMaxSize(unsigned int item_num) const
{
    return item_num < m_NofCols ? m_ColFmt[item_num].max_length : 0;
}


EDB_Type CDBL_RowResult::ItemDataType(unsigned int item_num) const
{
    return item_num < m_NofCols
        ? m_ColFmt[item_num].data_type : eDB_UnsupportedType;
}


bool CDBL_RowResult::Fetch()
{
    m_CurrItem = -1;
    if (!m_EOR) {
        switch (Check(dbnextrow(GetCmd()))) {
        case REG_ROW:
            m_CurrItem = 0;
            m_Offset = 0;
            return true;
        case NO_MORE_ROWS:
            m_EOR = true;
            break;
        case FAIL:
            DATABASE_DRIVER_ERROR( "error in fetching row", 230003 );
        case BUF_FULL:
            DATABASE_DRIVER_ERROR( "buffer is full", 230006 );
        default:
            *m_ResStatus|= 0x10;
            m_EOR = true;
            break;
        }
    }
    return false;
}


int CDBL_RowResult::CurrentItemNo() const
{
    return m_CurrItem;
}


int CDBL_RowResult::GetColumnNum(void) const
{
    return static_cast<int>(m_NofCols);
}


CDB_Object* CDBL_RowResult::GetItem(CDB_Object* item_buff)
{
    if ((unsigned int) m_CurrItem >= m_NofCols) {
        return 0;
    }
    CDB_Object* r = GetItemInternal(m_CurrItem + 1,
                                    &m_ColFmt[m_CurrItem],
                                    item_buff);
    ++m_CurrItem;
    m_Offset = 0;
    return r;
}


size_t CDBL_RowResult::ReadItem(void* buffer, size_t buffer_size,bool* is_null)
{
    if ((unsigned int) m_CurrItem >= m_NofCols) {
        if (is_null)
            *is_null = true;
        return 0;
    }

    const BYTE* d_ptr = Check(dbdata (GetCmd(), m_CurrItem + 1));
    DBINT d_len = Check(dbdatlen(GetCmd(), m_CurrItem + 1));

    if (d_ptr == 0 || d_len < 1) { // NULL value
        ++m_CurrItem;
        m_Offset = 0;
        if (is_null)
            *is_null = true;
        return 0;
    }

    if (is_null)
        *is_null = false;
    if ((size_t) (d_len - m_Offset) < buffer_size) {
        buffer_size = d_len - m_Offset;
    }

    if (buffer)
        memcpy(buffer, d_ptr + m_Offset, buffer_size);
    m_Offset += buffer_size;
    if (m_Offset >= (size_t) d_len) {
        m_Offset = 0;
        ++m_CurrItem;
    }

    return buffer_size;
}


I_ITDescriptor* CDBL_RowResult::GetImageOrTextDescriptor()
{
    if ((unsigned int) m_CurrItem >= m_NofCols)
        return 0;
    return new CDBL_ITDescriptor(GetConnection(), GetCmd(), m_CurrItem+1);
}


bool CDBL_RowResult::SkipItem()
{
    if ((unsigned int) m_CurrItem < m_NofCols) {
        ++m_CurrItem;
        m_Offset = 0;
        return true;
    }
    return false;
}


CDBL_RowResult::~CDBL_RowResult()
{
    try {
        if (m_ColFmt) {
            delete[] m_ColFmt;
            m_ColFmt = 0;
        }
        if (!m_EOR)
            Check(dbcanquery(GetCmd()));
    }
    NCBI_CATCH_ALL( kEmptyStr )
}



/////////////////////////////////////////////////////////////////////////////
//
//  CDBL_BlobResult::


CDBL_BlobResult::CDBL_BlobResult(CDBL_Connection& conn, DBPROCESS* cmd) :
    CDBL_Result(conn, cmd),
    m_CurrItem(-1),
    m_EOR(false)
{
    m_CmdNum = DBCURCMD(cmd);

    m_ColFmt.max_length = Check(dbcollen(GetCmd(), 1));
    m_ColFmt.data_type = GetDataType(1);
    const char* s = Check(dbcolname(GetCmd(), 1));
    m_ColFmt.col_name = s ? s : "";
}

EDB_ResType CDBL_BlobResult::ResultType() const
{
    return eDB_RowResult;
}


unsigned int CDBL_BlobResult::NofItems() const
{
    return 1;
}


const char* CDBL_BlobResult::ItemName(unsigned int item_num) const
{
    return item_num ? 0 : m_ColFmt.col_name.c_str();
}


size_t CDBL_BlobResult::ItemMaxSize(unsigned int item_num) const
{
    return item_num ? 0 : m_ColFmt.max_length;
}


EDB_Type CDBL_BlobResult::ItemDataType(unsigned int) const
{
    return m_ColFmt.data_type;
}


bool CDBL_BlobResult::Fetch()
{
    if (m_EOR)
        return false;

    STATUS s;
    if (m_CurrItem == 0) {
        while ((s = Check(dbreadtext(GetCmd(), m_Buff, (DBINT) sizeof(m_Buff)))) > 0)
            ;
        switch (s) {
        case 0:
            break;
        case NO_MORE_ROWS:
            m_EOR = true;
            return false;
        default:
            DATABASE_DRIVER_ERROR( "error in fetching row", 280003 );
        }
    }
    else {
        m_CurrItem = 0;
    }
    s = Check(dbreadtext(GetCmd(), m_Buff, (DBINT) sizeof(m_Buff)));
    if (s == NO_MORE_ROWS)
        return false;
    if (s < 0) {
        DATABASE_DRIVER_ERROR( "error in fetching row", 280003 );
    }
    m_BytesInBuffer= s;
    m_ReadedBytes= 0;
    return true;
}


int CDBL_BlobResult::CurrentItemNo() const
{
    return m_CurrItem;
}


int CDBL_BlobResult::GetColumnNum(void) const
{
    return 0;
}


CDB_Object* CDBL_BlobResult::GetItem(CDB_Object* item_buff)
{
    if (m_CurrItem)
        return 0;

    EDB_Type b_type = item_buff ? item_buff->GetType() : eDB_UnsupportedType;

    if (item_buff && b_type != eDB_Text && b_type != eDB_Image) {
        DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
    }

    if (item_buff == 0) {
        item_buff = m_ColFmt.data_type == eDB_Text
            ? (CDB_Object*) new CDB_Text : (CDB_Object*) new CDB_Image;
    }

    CDB_Text* val = (CDB_Text*) item_buff;

    // check if we do have something in buffer
    if(m_ReadedBytes < m_BytesInBuffer) {
        val->Append(m_Buff + m_ReadedBytes, m_BytesInBuffer - m_ReadedBytes);
        m_ReadedBytes= m_BytesInBuffer;
    }

    if(m_BytesInBuffer == 0) {
        return item_buff;
    }


    STATUS s;
    while ((s = Check(dbreadtext(GetCmd(), m_Buff, (DBINT) sizeof(m_Buff)))) > 0)
        val->Append(m_Buff, (size_t(s) < sizeof(m_Buff))? size_t(s) : sizeof(m_Buff));

    switch (s) {
    case NO_MORE_ROWS:
        m_EOR = true;
    case 0:
        m_CurrItem = 1;
        break;
    default:
        DATABASE_DRIVER_ERROR( "dbreadtext failed", 280003 );
    }

    return item_buff;
}


size_t CDBL_BlobResult::ReadItem(void* buffer, size_t buffer_size,
                                 bool* is_null)
{
    if(m_BytesInBuffer == 0)
        m_CurrItem= 1;

    if (m_CurrItem != 0) {
        if (is_null)
            *is_null = true;
        return 0;
    }

    size_t l= 0;
    // check if we do have something in buffer
    if(m_ReadedBytes < m_BytesInBuffer) {
        l= m_BytesInBuffer - m_ReadedBytes;
        if(l >= buffer_size) {
            memcpy(buffer, m_Buff + m_ReadedBytes, buffer_size);
            m_ReadedBytes+= buffer_size;
            if (is_null)
                *is_null = false;
            return buffer_size;
        }
        memcpy(buffer, m_Buff + m_ReadedBytes, l);
    }

    STATUS s = Check(dbreadtext(GetCmd(), (void*)((char*)buffer + l), (DBINT)(buffer_size-l)));
    switch (s) {
    case NO_MORE_ROWS:
        m_EOR = true;
    case 0:
        m_CurrItem = 1;
        break;
    case -1:
        DATABASE_DRIVER_ERROR( "dbreadtext failed", 280003 );
    default:
        break;
    }

    if (is_null) {
        *is_null = (m_BytesInBuffer == 0  &&  s <= 0);
    }
    return (size_t) s + l;
}


I_ITDescriptor* CDBL_BlobResult::GetImageOrTextDescriptor()
{
    if (m_CurrItem != 0)
        return 0;
    return new CDBL_ITDescriptor(GetConnection(), GetCmd(), 1);
}


bool CDBL_BlobResult::SkipItem()
{
    if (m_EOR || m_CurrItem)
        return false;

    STATUS s;
    while ((s = Check(dbreadtext(GetCmd(), m_Buff, sizeof(m_Buff)))) > 0)
        continue;

    switch (s) {
    case NO_MORE_ROWS:
        m_EOR = true;
    case 0:
        m_CurrItem = 1;
        break;
    default:
        DATABASE_DRIVER_ERROR( "dbreadtext failed", 280003 );
    }
    return true;
}


CDBL_BlobResult::~CDBL_BlobResult()
{
    try {
        if (!m_EOR)
            Check(dbcanquery(GetCmd()));
    }
    NCBI_CATCH_ALL( kEmptyStr )
}


/////////////////////////////////////////////////////////////////////////////
//
//  CDBL_ParamResult::
//

CDBL_ParamResult::CDBL_ParamResult(CDBL_Connection& conn,
                                   DBPROCESS* cmd,
                                   int nof_params) :
    CDBL_RowResult(conn, cmd, 0, false)
{

    m_NofCols = nof_params;
    m_CmdNum = DBCURCMD(cmd);

    m_ColFmt = new SDBL_ColDescr[m_NofCols];
    for (unsigned int n = 0; n < m_NofCols; n++) {
        m_ColFmt[n].max_length = 255;
        m_ColFmt[n].data_type = RetGetDataType(n + 1);
        const char* s = Check(dbretname(GetCmd(), n + 1));
        m_ColFmt[n].col_name = s ? s : "";
    }
    m_1stFetch = true;
}


EDB_ResType CDBL_ParamResult::ResultType() const
{
    return eDB_ParamResult;
}


bool CDBL_ParamResult::Fetch()
{
    if (m_1stFetch) { // we didn't get the items yet;
        m_1stFetch = false;
        m_CurrItem= 0;
        return true;
    }
    m_CurrItem= -1;
    return false;
}


CDB_Object* CDBL_ParamResult::GetItem(CDB_Object* item_buff)
{
    if ((unsigned int) m_CurrItem >= m_NofCols) {
        return 0;
    }

    CDB_Object* r = RetGetItem(m_CurrItem + 1,
                               &m_ColFmt[m_CurrItem],
                               item_buff);
    ++m_CurrItem;
    m_Offset = 0;
    return r;
}


size_t CDBL_ParamResult::ReadItem(void* buffer, size_t buffer_size,
                                  bool* is_null)
{
    if ((unsigned int) m_CurrItem >= m_NofCols) {
        if (is_null)
            *is_null = true;
        return 0;
    }

    const BYTE* d_ptr = Check(dbretdata(GetCmd(), m_CurrItem + 1));
    DBINT d_len = Check(dbretlen (GetCmd(), m_CurrItem + 1));

    if (d_ptr == 0 || d_len < 1) { // NULL value
        ++m_CurrItem;
        m_Offset = 0;
        if (is_null)
            *is_null = true;
        return 0;
    }

    if (is_null)
        *is_null = false;
    if ((size_t) (d_len - m_Offset) < buffer_size) {
        buffer_size = d_len - m_Offset;
    }

    memcpy(buffer, d_ptr + m_Offset, buffer_size);
    m_Offset += buffer_size;
    if (m_Offset >= (size_t) d_len) {
        m_Offset = 0;
        ++m_CurrItem;
    }
    return buffer_size;
}


I_ITDescriptor* CDBL_ParamResult::GetImageOrTextDescriptor()
{
    return 0;
}


CDBL_ParamResult::~CDBL_ParamResult()
{
    try {
        if (m_ColFmt) {
            delete[] m_ColFmt;
            m_ColFmt = 0;
        }
    }
    NCBI_CATCH_ALL( kEmptyStr )
}


/////////////////////////////////////////////////////////////////////////////
//
//  CTL_ComputeResult::
//

CDBL_ComputeResult::CDBL_ComputeResult(CDBL_Connection& conn,
                                       DBPROCESS* cmd,
                                       unsigned int* res_stat) :
    CDBL_RowResult(conn, cmd, res_stat, false)
{
    m_ComputeId = DBROWTYPE(cmd);
    if (m_ComputeId == REG_ROW || m_ComputeId == NO_MORE_ROWS) {
        DATABASE_DRIVER_ERROR( "no compute row found", 270000 );
    }
    m_NofCols = Check(dbnumalts(cmd, m_ComputeId));
    if (m_NofCols < 1) {
        DATABASE_DRIVER_ERROR( "compute id is invalid", 270001 );
    }
    m_CmdNum = DBCURCMD(cmd);
    m_ColFmt = new SDBL_ColDescr[m_NofCols];
    for (unsigned int n = 0; n < m_NofCols; n++) {
        m_ColFmt[n].max_length = Check(dbaltlen(GetCmd(), m_ComputeId, n+1));
        m_ColFmt[n].data_type = AltGetDataType(m_ComputeId, n + 1);
        int op = Check(dbaltop(GetCmd(), m_ComputeId, n + 1));
        const char* s = op != -1 ? Check(dbprtype(op)) : "Unknown";
        m_ColFmt[n].col_name = s ? s : "";
    }
    m_1stFetch = true;
}


EDB_ResType CDBL_ComputeResult::ResultType() const
{
    return eDB_ComputeResult;
}


bool CDBL_ComputeResult::Fetch()
{
    if (m_1stFetch) { // we didn't get the items yet;
        m_1stFetch = false;
        m_CurrItem= 0;
        return true;
    }

    STATUS s = Check(dbnextrow(GetCmd()));
    switch (s) {
    case REG_ROW:
    case NO_MORE_ROWS:
        *m_ResStatus ^= 0x10;
        break;
    case FAIL:
        DATABASE_DRIVER_ERROR( "error in fetching row", 270003 );
    case BUF_FULL:
        DATABASE_DRIVER_ERROR( "buffer is full", 270006 );
    default:
        break;
    }
    m_EOR = true;
    m_CurrItem= -1;
    return false;
}


int CDBL_ComputeResult::CurrentItemNo() const
{
    return m_CurrItem;
}


CDB_Object* CDBL_ComputeResult::GetItem(CDB_Object* item_buff)
{
    if ((unsigned int) m_CurrItem >= m_NofCols) {
        return 0;
    }
    CDB_Object* r = AltGetItem(m_ComputeId,
                               m_CurrItem + 1,
                               &m_ColFmt[m_CurrItem],
                               item_buff);
    ++m_CurrItem;
    m_Offset = 0;
    return r;
}


size_t CDBL_ComputeResult::ReadItem(void* buffer, size_t buffer_size,
                                    bool* is_null)
{
    if ((unsigned int) m_CurrItem >= m_NofCols) {
        if (is_null)
            *is_null = true;
        return 0;
    }

    const BYTE* d_ptr = Check(dbadata(GetCmd(), m_ComputeId, m_CurrItem + 1));
    DBINT d_len = Check(dbadlen(GetCmd(), m_ComputeId, m_CurrItem + 1));

    if (d_ptr == 0 || d_len < 1) { // NULL value
        ++m_CurrItem;
        m_Offset = 0;
        if (is_null)
            *is_null = true;
        return 0;
    }

    if (is_null)
        *is_null = false;
    if ((size_t) d_len - m_Offset < buffer_size)
        buffer_size = (size_t) d_len - m_Offset;
    memcpy(buffer, d_ptr + m_Offset, buffer_size);
    m_Offset += buffer_size;
    if (m_Offset >= (size_t) d_len) {
        m_Offset = 0;
        ++m_CurrItem;
    }
    return buffer_size;
}


I_ITDescriptor* CDBL_ComputeResult::GetImageOrTextDescriptor()
{
    return 0;
}


CDBL_ComputeResult::~CDBL_ComputeResult()
{
    try {
        if (m_ColFmt) {
            delete[] m_ColFmt;
            m_ColFmt = 0;
        }
        while (!m_EOR) {
            Fetch();
        }
    }
    NCBI_CATCH_ALL( kEmptyStr )
}



/////////////////////////////////////////////////////////////////////////////
//
//  CTL_StatusResult::
//


CDBL_StatusResult::CDBL_StatusResult(CDBL_Connection& conn, DBPROCESS* cmd) :
    CDBL_Result(conn, cmd),
    m_Offset(0),
    m_1stFetch(true)
{
    m_Val = dbretstatus(cmd);
    CheckFunctCall();
}


EDB_ResType CDBL_StatusResult::ResultType() const
{
    return eDB_StatusResult;
}


unsigned int CDBL_StatusResult::NofItems() const
{
    return 1;
}


const char* CDBL_StatusResult::ItemName(unsigned int) const
{
    return 0;
}


size_t CDBL_StatusResult::ItemMaxSize(unsigned int) const
{
    return sizeof(DBINT);
}


EDB_Type CDBL_StatusResult::ItemDataType(unsigned int) const
{
    return eDB_Int;
}


bool CDBL_StatusResult::Fetch()
{
    if (m_1stFetch) {
        m_1stFetch = false;
        return true;
    }
    return false;
}


int CDBL_StatusResult::CurrentItemNo() const
{
    return m_1stFetch? -1 : 0;
}


int CDBL_StatusResult::GetColumnNum(void) const
{
    return 1;
}


CDB_Object* CDBL_StatusResult::GetItem(CDB_Object* item_buff)
{
    if (!item_buff)
        return new CDB_Int(m_Val);

    if (item_buff->GetType() != eDB_Int) {
        DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
    }

    CDB_Int* i = (CDB_Int*) item_buff;
    *i = m_Val;
    return item_buff;
}


size_t CDBL_StatusResult::ReadItem(void* buffer, size_t buffer_size,
                                   bool* is_null)
{
    if (is_null)
        *is_null = false;

    if (sizeof(m_Val) <= m_Offset)
        return 0;

    size_t l = sizeof(int) - m_Offset;
    char* p = (char*) &m_Val;

    if (buffer_size > l)
        buffer_size = l;
    memcpy(buffer, p + m_Offset, buffer_size);
    m_Offset += buffer_size;
    return buffer_size;
}


I_ITDescriptor* CDBL_StatusResult::GetImageOrTextDescriptor()
{
    return 0;
}


bool CDBL_StatusResult::SkipItem()
{
    return false;
}


CDBL_StatusResult::~CDBL_StatusResult()
{
    return;
}



/////////////////////////////////////////////////////////////////////////////
//
//  CTL_CursorResult::
//

CDBL_CursorResult::CDBL_CursorResult(CDBL_Connection& conn, CDB_LangCmd* cmd) :
    CDBL_Result(conn, NULL),
    m_Cmd(cmd),
    m_Res(0)
{
    try {
        GetCmd().Send();
        while (GetCmd().HasMoreResults()) {
            SetResultSet( GetCmd().Result() );
            if (GetResultSet() && GetResultSet()->ResultType() == eDB_RowResult) {
                return;
            }
            if ( GetResultSet() ) {
                while (GetResultSet()->Fetch())
                    continue;
                ClearResultSet();
            }
        }
    } catch ( const CDB_Exception& e ) {
        DATABASE_DRIVER_ERROR_EX( e, "failed to get the results", 222010 );
    }
}


EDB_ResType CDBL_CursorResult::ResultType() const
{
    return eDB_CursorResult;
}


unsigned int CDBL_CursorResult::NofItems() const
{
    return GetResultSet()? GetResultSet()->NofItems() : 0;
}


const char* CDBL_CursorResult::ItemName(unsigned int item_num) const
{
    return GetResultSet() ? GetResultSet()->ItemName(item_num) : 0;
}


size_t CDBL_CursorResult::ItemMaxSize(unsigned int item_num) const
{
    return GetResultSet() ? GetResultSet()->ItemMaxSize(item_num) : 0;
}


EDB_Type CDBL_CursorResult::ItemDataType(unsigned int item_num) const
{
    return GetResultSet() ? GetResultSet()->ItemDataType(item_num) : eDB_UnsupportedType;
}


bool CDBL_CursorResult::Fetch()
{
    if ( !GetResultSet() )
        return false;

    try {
        if (GetResultSet()->Fetch())
            return true;
    }
    catch (CDB_ClientEx& ex) {
        if (ex.GetDBErrCode() == 200003) {
//             ClearResultSet();
            m_Res = NULL;
        } else {
            DATABASE_DRIVER_ERROR( "Failed to fetch the results", 222011 );
        }
    }

    // try to get next cursor result
    try {
        // finish this command
//         ClearResultSet();
        if ( m_Res ) {
            delete m_Res;
            m_Res = NULL;
        }

        while (m_Cmd->HasMoreResults()) {
            DumpResultSet();
        }
        // send the another "fetch cursor_name" command
        m_Cmd->Send();
        while (m_Cmd->HasMoreResults()) {
            SetResultSet( m_Cmd->Result() );
            if (GetResultSet() && GetResultSet()->ResultType() == eDB_RowResult) {
                return GetResultSet()->Fetch();
            }
            FetchAllResultSet();
        }
    } catch ( const CDB_Exception& e ) {
        DATABASE_DRIVER_ERROR_EX( e, "Failed to fetch the results", 222011 );
    }
    return false;
}


int CDBL_CursorResult::CurrentItemNo() const
{
    return GetResultSet() ? GetResultSet()->CurrentItemNo() : -1;
}


int CDBL_CursorResult::GetColumnNum(void) const
{
    return GetResultSet() ? GetResultSet()->GetColumnNum() : -1;
}


CDB_Object* CDBL_CursorResult::GetItem(CDB_Object* item_buff)
{
    return GetResultSet() ? GetResultSet()->GetItem(item_buff) : 0;
}


size_t CDBL_CursorResult::ReadItem(void* buffer, size_t buffer_size,
                                   bool* is_null)
{
    if (GetResultSet()) {
        return GetResultSet()->ReadItem(buffer, buffer_size, is_null);
    }
    if (is_null)
        *is_null = true;
    return 0;
}


I_ITDescriptor* CDBL_CursorResult::GetImageOrTextDescriptor()
{
    return GetResultSet() ? GetResultSet()->GetImageOrTextDescriptor() : 0;
}


bool CDBL_CursorResult::SkipItem()
{
    return GetResultSet() ? GetResultSet()->SkipItem() : false;
}


CDBL_CursorResult::~CDBL_CursorResult()
{
    try {
        ClearResultSet();
    }
    NCBI_CATCH_ALL( kEmptyStr )
}



/////////////////////////////////////////////////////////////////////////////
//
//  CDBL_ITDescriptor::
//

CDBL_ITDescriptor::CDBL_ITDescriptor(CDBL_Connection& conn,
                                     DBPROCESS* dblink,
                                     int col_num) :
CDBL_Result(conn, dblink)
{
#if defined(MS_DBLIB_IN_USE) || defined(FTDS_IN_USE)

    DBCOL dbcol;

    memset(&dbcol, 0, sizeof(DBCOL));
    RETCODE res = Check(dbcolinfo(GetCmd(), CI_REGULAR, col_num, 0, &dbcol ));

    CHECK_DRIVER_ERROR(
        res == FAIL,
        "Cannot get the DBCOLINFO*",
        280000 );

    if ( dbcol.TableName && *dbcol.TableName ) {
        m_ObjName += dbcol.TableName;
        m_ObjName += ".";
        m_ObjName += dbcol.ActualName;
    } else {
        m_ObjName.erase();
    }

#else

    // !!! This is a hack !!!
    // dbcolname returns char*
    DBCOLINFO* col_info = (DBCOLINFO*) Check(dbcolname(GetCmd(), col_num));

    CHECK_DRIVER_ERROR(
        col_info == 0,
        "Cannot get the DBCOLINFO*",
        280000 );

    if (!x_MakeObjName(col_info)) {
        m_ObjName.erase();
    }

#endif

    DBBINARY* p = Check(dbtxptr(GetCmd(), col_num));
    if (p) {
        memcpy(m_TxtPtr, p, DBTXPLEN);
        m_TxtPtr_is_NULL = false;
    } else
        m_TxtPtr_is_NULL = true;

    p = Check(dbtxtimestamp(GetCmd(), col_num));
    if (p) {
        memcpy(m_TimeStamp, p, DBTXTSLEN);
        m_TimeStamp_is_NULL = false;
    } else
        m_TimeStamp_is_NULL = true;
}

CDBL_ITDescriptor::CDBL_ITDescriptor(CDBL_Connection& conn,
                                     DBPROCESS* dblink,
                                     const CDB_ITDescriptor& inp_d) :
CDBL_Result(conn, dblink)
{
    m_ObjName= inp_d.TableName();
    m_ObjName+= ".";
    m_ObjName+= inp_d.ColumnName();


    DBBINARY* p = Check(dbtxptr(GetCmd(), 1));
    if (p) {
        memcpy(m_TxtPtr, p, DBTXPLEN);
        m_TxtPtr_is_NULL = false;
    } else
        m_TxtPtr_is_NULL = true;

    p = Check(dbtxtimestamp(GetCmd(), 1));
    if (p) {
        memcpy(m_TimeStamp, p, DBTXTSLEN);
        m_TimeStamp_is_NULL = false;
    } else
        m_TimeStamp_is_NULL = true;
}


int CDBL_ITDescriptor::DescriptorType() const
{
#ifndef MS_DBLIB_IN_USE
    return CDBL_ITDESCRIPTOR_TYPE_MAGNUM;
#else
    return CMSDBL_ITDESCRIPTOR_TYPE_MAGNUM;
#endif
}

CDBL_ITDescriptor::~CDBL_ITDescriptor()
{
}

#if !defined(MS_DBLIB_IN_USE) && !defined(FTDS_IN_USE)
bool CDBL_ITDescriptor::x_MakeObjName(DBCOLINFO* col_info)
{
    if (!col_info || !col_info->coltxobjname)
        return false;
    m_ObjName = col_info->coltxobjname;

    // check if we do have ".colname" suffix already
    char* p_dot = strrchr(col_info->coltxobjname, '.');
    if (!p_dot || strcmp(p_dot + 1, col_info->colname) != 0) {
        // we don't have a suffix. Add it now
        m_ObjName += '.';
        m_ObjName += col_info->colname;
    }
    return true;
}
#endif


/////////////////////////////////////////////////////////////////////////////

EDB_Type CDBL_Result::GetDataType(int n)
{
    switch (Check(dbcoltype(GetCmd(), n))) {
    case SYBBINARY:    return (Check(dbcollen(GetCmd(), n)) > 255) ? eDB_LongBinary :
                                                         eDB_VarBinary;
#if 0
    case SYBBITN:
#endif
    case SYBBIT:       return eDB_Bit;
    case SYBCHAR:      return (Check(dbcollen(GetCmd(), n)) > 255) ? eDB_LongChar :
                                                         eDB_VarChar;
    case SYBDATETIME:  return eDB_DateTime;
    case SYBDATETIME4: return eDB_SmallDateTime;
    case SYBINT1:      return eDB_TinyInt;
    case SYBINT2:      return eDB_SmallInt;
    case SYBINT4:      return eDB_Int;
#ifdef FTDS_IN_USE
    case SYBINT8:      return eDB_BigInt;
#endif
    case SYBDECIMAL:
    case SYBNUMERIC:   break;
    case SYBFLT8:      return eDB_Double;
    case SYBREAL:      return eDB_Float;
    case SYBTEXT:      return eDB_Text;
    case SYBIMAGE:     return eDB_Image;
#ifdef FTDS_IN_USE
    case SYBUNIQUE:    return eDB_VarBinary;
#endif
    default:           return eDB_UnsupportedType;
    }

#ifdef MS_DBLIB_IN_USE
    DBCOL dbcol; dbcol.SizeOfStruct = sizeof(DBCOL);
    RETCODE res = Check(dbcolinfo(GetCmd(), CI_REGULAR, n, 0, &dbcol ));
    return dbcol.Scale == 0 && dbcol.Precision < 20 ? eDB_BigInt : eDB_Numeric;
#else
    DBTYPEINFO* t = Check(dbcoltypeinfo(GetCmd(), n));
    return t->scale == 0 && t->precision < 20 ? eDB_BigInt : eDB_Numeric;
#endif

}

// Aux. for CDBL_RowResult::GetItem()
CDB_Object*
CDBL_Result::GetItemInternal(int item_no,
                             SDBL_ColDescr* fmt,
                             CDB_Object* item_buff)
{
    EDB_Type b_type = item_buff ? item_buff->GetType() : eDB_UnsupportedType;
    const BYTE* d_ptr = Check(dbdata (GetCmd(), item_no));
    DBINT d_len = Check(dbdatlen(GetCmd(), item_no));

    CDB_Object* val = s_GenericGetItem(fmt->data_type, item_buff,
                                       b_type, d_ptr, d_len);
    if (val)
        return val;

    switch (fmt->data_type) {
    case eDB_BigInt: {
#ifdef FTDS_IN_USE
        if(Check(dbcoltype(GetCmd(), item_no)) == SYBINT8) {
          Int8* v= (Int8*) d_ptr;
          if(item_buff) {
            if(v) {
              if(b_type == eDB_BigInt) {
                *((CDB_BigInt*) item_buff)= *v;
              }
              else {
                DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
              }
            }
            else
                item_buff->AssignNULL();
            return item_buff;
          }

          return v ?
            new CDB_BigInt(*v) : new CDB_BigInt;
        }
#endif
        DBNUMERIC* v = (DBNUMERIC*) d_ptr;
        if (item_buff) {
            if (v) {
                if (b_type == eDB_Numeric) {
                    ((CDB_Numeric*) item_buff)->Assign
                        ((unsigned int)   v->precision,
                         (unsigned int)   v->scale,
                         (unsigned char*) DBNUMERIC_val(v));
                } else if (b_type == eDB_BigInt) {
                    *((CDB_BigInt*) item_buff) = numeric_to_longlong
                        ((unsigned int) v->precision, DBNUMERIC_val(v));
                } else {
                    DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                }
            } else
                item_buff->AssignNULL();
            return item_buff;
        }

        return v ?
            new CDB_BigInt(numeric_to_longlong((unsigned int) v->precision,
                                               DBNUMERIC_val(v))) : new CDB_BigInt;
    }

    case eDB_Numeric: {
        DBNUMERIC* v = (DBNUMERIC*) d_ptr;
        if (item_buff) {
                if (v) {
                    if (b_type == eDB_Numeric) {
                        ((CDB_Numeric*) item_buff)->Assign
                            ((unsigned int)   v->precision,
                             (unsigned int)   v->scale,
                             (unsigned char*) DBNUMERIC_val(v));
                    } else {
                        DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 230020 );
                    }
                } else
                    item_buff->AssignNULL();
                return item_buff;
        }

        return v ?
            new CDB_Numeric((unsigned int)   v->precision,
                            (unsigned int)   v->scale,
                            (unsigned char*) DBNUMERIC_val(v)) : new CDB_Numeric;
    }

    case eDB_Text: {
        if (item_buff && b_type != eDB_Text && b_type != eDB_Image) {
            DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 130020 );
        }
        CDB_Text* v = item_buff ? (CDB_Text*) item_buff : new CDB_Text;
        v->Append((char*) d_ptr, (int) d_len);
        return v;
    }

    case eDB_Image: {
        if (item_buff && b_type != eDB_Text && b_type != eDB_Image) {
            DATABASE_DRIVER_ERROR( "wrong type of CDB_Object", 130020 );
        }
        CDB_Image* v = item_buff ? (CDB_Image*) item_buff : new CDB_Image;
        v->Append((void*) d_ptr, (int) d_len);
        return v;
    }

    default:
        DATABASE_DRIVER_ERROR( "unexpected result type", 130004 );
    }
}


EDB_Type CDBL_Result::RetGetDataType(int n)
{
    switch (Check(dbrettype(GetCmd(), n))) {
    case SYBBINARY:    return (Check(dbretlen(GetCmd(), n)) > 255)? eDB_LongBinary :
                                                        eDB_VarBinary;
    case SYBBIT:       return eDB_Bit;
    case SYBCHAR:      return (Check(dbretlen(GetCmd(), n)) > 255)? eDB_LongChar :
                                                        eDB_VarChar;
    case SYBDATETIME:  return eDB_DateTime;
    case SYBDATETIME4: return eDB_SmallDateTime;
    case SYBINT1:      return eDB_TinyInt;
    case SYBINT2:      return eDB_SmallInt;
    case SYBINT4:      return eDB_Int;
    case SYBFLT8:      return eDB_Double;
    case SYBREAL:      return eDB_Float;
#ifdef FTDS_IN_USE
    case SYBUNIQUE:    return eDB_VarBinary;
#endif
    default:           return eDB_UnsupportedType;
    }
}


// Aux. for CDBL_ParamResult::GetItem()
CDB_Object* CDBL_Result::RetGetItem(int item_no,
                                    SDBL_ColDescr* fmt,
                                    CDB_Object* item_buff)
{
    EDB_Type b_type = item_buff ? item_buff->GetType() : eDB_UnsupportedType;
    const BYTE* d_ptr = Check(dbretdata(GetCmd(), item_no));
    DBINT d_len = Check(dbretlen (GetCmd(), item_no));
    CDB_Object* val = s_GenericGetItem(fmt->data_type, item_buff,
                                       b_type, d_ptr, d_len);
    if (!val) {
        DATABASE_DRIVER_ERROR( "unexpected result type", 230004 );
    }
    return val;
}


EDB_Type CDBL_Result::AltGetDataType(int id, int n)
{
    switch (Check(dbalttype(GetCmd(), id, n))) {
    case SYBBINARY:    return (Check(dbaltlen(GetCmd(), id, n)) > 255)? eDB_LongBinary :
                                                        eDB_VarBinary;
    case SYBBIT:       return eDB_Bit;
    case SYBCHAR:      return (Check(dbaltlen(GetCmd(), id, n)) > 255)? eDB_LongChar :
                                                        eDB_VarChar;
    case SYBDATETIME:  return eDB_DateTime;
    case SYBDATETIME4: return eDB_SmallDateTime;
    case SYBINT1:      return eDB_TinyInt;
    case SYBINT2:      return eDB_SmallInt;
    case SYBINT4:      return eDB_Int;
    case SYBFLT8:      return eDB_Double;
    case SYBREAL:      return eDB_Float;
#ifdef FTDS_IN_USE
    case SYBUNIQUE:    return eDB_VarBinary;
#endif
    default:           return eDB_UnsupportedType;
    }
}


// Aux. for CDBL_ComputeResult::GetItem()
CDB_Object* CDBL_Result::AltGetItem(int id,
                                    int item_no,
                                    SDBL_ColDescr* fmt,
                                    CDB_Object* item_buff)
{
    EDB_Type b_type = item_buff ? item_buff->GetType() : eDB_UnsupportedType;
    const BYTE* d_ptr = Check(dbadata(GetCmd(), id, item_no));
    DBINT d_len = Check(dbadlen(GetCmd(), id, item_no));
    CDB_Object* val = s_GenericGetItem(fmt->data_type, item_buff,
                                       b_type, d_ptr, d_len);
    if (!val) {
        DATABASE_DRIVER_ERROR( "unexpected result type", 130004 );
    }
    return val;
}



END_NCBI_SCOPE



/*
 * ===========================================================================
 * $Log$
 * Revision 1.38  2006/09/20 20:34:58  ssikorsk
 * Removed extra check with dbretstatus.
 *
 * Revision 1.37  2006/07/12 16:29:30  ssikorsk
 * Separated interface and implementation of CDB classes.
 *
 * Revision 1.36  2006/05/04 20:12:17  ssikorsk
 * Implemented classs CDBL_Cmd, CDBL_Result and CDBLExceptions;
 * Surrounded each native dblib call with Check;
 *
 * Revision 1.35  2006/02/24 19:33:29  ssikorsk
 * Added implementation comment
 *
 * Revision 1.34  2006/02/16 19:37:42  ssikorsk
 * Get rid of compilation warnings
 *
 * Revision 1.33  2005/12/06 19:31:15  ssikorsk
 * Revamp code to use GetResultSet/SetResultSet/ClearResultSet
 * methods instead of raw data access.
 *
 * Revision 1.32  2005/11/02 14:16:59  ssikorsk
 * Rethrow catched CDB_Exception to preserve useful information.
 *
 * Revision 1.31  2005/10/31 12:20:42  ssikorsk
 * Do not use separate include files for msdblib.
 *
 * Revision 1.30  2005/10/27 12:56:46  ssikorsk
 * Improved table and column name retrieval in CDBL_ITDescriptor::CDBL_ITDescriptor
 *
 * Revision 1.29  2005/09/26 15:25:51  ssikorsk
 * Use dbcolinfo within CDBL_ITDescriptor::CDBL_ITDescriptor in case of MSDBLIB and TreeTDS
 * instead of an unsafe "offset in some undocumented structure obtained with the help
 * of a debugger".
 *
 * Revision 1.28  2005/09/26 12:58:39  ssikorsk
 * Handle SYBUNIQUE data type as eDB_VarBinary in case of the FDS driver
 *
 * Revision 1.27  2005/09/19 14:19:05  ssikorsk
 * Use NCBI_CATCH_ALL macro instead of catch(...)
 *
 * Revision 1.26  2005/09/15 11:00:01  ssikorsk
 * Destructors do not throw exceptions any more.
 *
 * Revision 1.25  2005/09/07 11:06:32  ssikorsk
 * Added a GetColumnNum implementation
 *
 * Revision 1.24  2005/07/07 19:12:55  ssikorsk
 * Improved to support a ftds driver
 *
 * Revision 1.23  2005/04/04 13:03:57  ssikorsk
 * Revamp of DBAPI exception class CDB_Exception
 *
 * Revision 1.22  2004/05/18 18:30:36  gorelenk
 * PCH <ncbi_pch.hpp> moved to correct place .
 *
 * Revision 1.21  2004/05/17 21:12:41  gorelenk
 * Added include of PCH ncbi_pch.hpp
 *
 * Revision 1.20  2004/04/08 19:05:51  gorelenk
 * Changed declalation of s_GenericGetItem (added 'const' to 'BYTE* d_ptr')
 * and fixed compilation errors on MSVC 7.10 .
 *
 * Revision 1.19  2003/04/29 21:16:21  soussov
 * new datatypes CDB_LongChar and CDB_LongBinary added
 *
 * Revision 1.18  2003/01/31 16:50:12  lavr
 * Remove unused variable "e" from catch() clause
 *
 * Revision 1.17  2003/01/06 16:40:49  soussov
 * sets m_CurrItem = -1 for all result types if no fetch was called
 *
 * Revision 1.16  2003/01/03 21:48:08  soussov
 * set m_CurrItem = -1 if fetch failes
 *
 * Revision 1.15  2002/07/18 14:59:10  soussov
 * fixes bug in blob result
 *
 * Revision 1.14  2002/07/02 16:05:50  soussov
 * splitting Sybase dblib and MS dblib
 *
 * Revision 1.13  2002/05/29 22:04:58  soussov
 * Makes BlobResult read ahead
 *
 * Revision 1.12  2002/03/26 15:37:52  soussov
 * new image/text operations added
 *
 * Revision 1.11  2002/02/06 22:27:54  soussov
 * fixes the arguments order in numeric assign
 *
 * Revision 1.10  2002/01/11 20:11:43  vakatov
 * Fixed CVS logs from prev. revision that messed up the compilation
 *
 * Revision 1.9  2002/01/10 22:05:52  sapojnik
 * MS-specific workarounds needed to use blobs via I_ITDescriptor
 * (see Text,Image)
 *
 * Revision 1.8  2002/01/08 18:10:18  sapojnik
 * Syabse to MSSQL name translations moved to interface_p.hpp
 *
 * Revision 1.7  2002/01/03 17:01:56  sapojnik
 * fixing CR/LF mixup
 *
 * Revision 1.6  2002/01/03 15:46:23  sapojnik
 * ported to MS SQL (about 12 'ifdef NCBI_OS_MSWIN' in 6 files)
 *
 * Revision 1.5  2001/11/06 17:59:58  lavr
 * Formatted uniformly as the rest of the library
 *
 * Revision 1.4  2001/10/24 16:38:42  lavr
 * Explicit casts (where necessary) to eliminate 64->32 bit compiler warnings
 *
 * Revision 1.3  2001/10/24 05:19:24  vakatov
 * Fixed return type for ItemMaxSize()
 *
 * Revision 1.2  2001/10/22 16:28:01  lavr
 * Default argument values removed
 * (mistakenly left while moving code from header files)
 *
 * Revision 1.1  2001/10/22 15:19:55  lavr
 * This is a major revamp (by Anton Lavrentiev, with help from Vladimir
 * Soussov and Denis Vakatov) of the DBAPI "driver" libs originally
 * written by Vladimir Soussov. The revamp follows the one of CTLib
 * driver, and it involved massive code shuffling and grooming, numerous
 * local API redesigns, adding comments and incorporating DBAPI to
 * the C++ Toolkit.
 *
 * ===========================================================================
 */
