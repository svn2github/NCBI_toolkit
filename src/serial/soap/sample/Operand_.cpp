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
 * File Description:
 *   This code was generated by application DATATOOL
 *   using the following specifications:
 *   'samplesoap.dtd'.
 *
 * ATTENTION:
 *   Don't edit or commit this file into CVS as this file will
 *   be overridden (by DATATOOL) without warning!
 * ===========================================================================
 */

// standard includes
#include <ncbi_pch.hpp>
#include <serial/serialimpl.hpp>

// generated includes
#include "Operand.hpp"

// generated classes

BEGIN_NAMED_ENUM_IN_INFO("", COperand_Base::C_Attlist::, EAttlist_operation, false)
{
    ADD_ENUM_VALUE("add", eAttlist_operation_add);
    ADD_ENUM_VALUE("subtract", eAttlist_operation_subtract);
}
END_ENUM_INFO

void COperand_Base::C_Attlist::Reset(void)
{
    ResetOperation();
}

BEGIN_NAMED_CLASS_INFO("", COperand_Base::C_Attlist)
{
    ADD_NAMED_ENUM_MEMBER("operation", m_Operation, EAttlist_operation)->SetSetFlag(MEMBER_PTR(m_set_State[0]))->SetNoPrefix();
    info->SetRandomOrder(true);
}
END_CLASS_INFO

// constructor
COperand_Base::C_Attlist::C_Attlist(void)
    : m_Operation(EAttlist_operation(0))
{
    memset(m_set_State,0,sizeof(m_set_State));
}

// destructor
COperand_Base::C_Attlist::~C_Attlist(void)
{
}


void COperand_Base::ResetAttlist(void)
{
    (*m_Attlist).Reset();
}

void COperand_Base::SetAttlist(COperand_Base::C_Attlist& value)
{
    m_Attlist.Reset(&value);
}

void COperand_Base::ResetX(void)
{
    m_X.erase();
    m_set_State[0] &= ~0xc;
}

void COperand_Base::ResetY(void)
{
    m_Y.erase();
    m_set_State[0] &= ~0x30;
}

void COperand_Base::Reset(void)
{
    ResetAttlist();
    ResetX();
    ResetY();
}

BEGIN_NAMED_BASE_CLASS_INFO("Operand", COperand)
{
    SET_CLASS_MODULE("samplesoap");
    ADD_NAMED_REF_MEMBER("Attlist", m_Attlist, C_Attlist)->SetNoPrefix()->SetAttlist();
    ADD_NAMED_STD_MEMBER("x", m_X)->SetSetFlag(MEMBER_PTR(m_set_State[0]))->SetNoPrefix();
    ADD_NAMED_STD_MEMBER("y", m_Y)->SetSetFlag(MEMBER_PTR(m_set_State[0]))->SetNoPrefix();
    info->RandomOrder();
}
END_CLASS_INFO

// constructor
COperand_Base::COperand_Base(void)
    : m_Attlist(new C_Attlist())
{
    memset(m_set_State,0,sizeof(m_set_State));
}

// destructor
COperand_Base::~COperand_Base(void)
{
}



