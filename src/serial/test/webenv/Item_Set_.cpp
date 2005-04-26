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
 *   'twebenv.asn'.
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
#include "Item_Set.hpp"

// generated classes

void CItem_Set_Base::ResetItems(void)
{
    m_Items.clear();
    m_set_State[0] &= ~0x3;
}

void CItem_Set_Base::Reset(void)
{
    ResetItems();
    ResetCount();
}

BEGIN_NAMED_BASE_CLASS_INFO("Item-Set", CItem_Set)
{
    SET_CLASS_MODULE("NCBI-Env");
    ADD_NAMED_MEMBER("items", m_Items, STL_CHAR_vector, (char))->SetSetFlag(MEMBER_PTR(m_set_State[0]));
    ADD_NAMED_STD_MEMBER("count", m_Count)->SetSetFlag(MEMBER_PTR(m_set_State[0]));
    info->RandomOrder();
}
END_CLASS_INFO

// constructor
CItem_Set_Base::CItem_Set_Base(void)
    : m_Count(0)
{
    memset(m_set_State,0,sizeof(m_set_State));
}

// destructor
CItem_Set_Base::~CItem_Set_Base(void)
{
}



