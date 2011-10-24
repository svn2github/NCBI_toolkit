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
 *   'soap_11.xsd'.
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
#include <serial/soap/soap_body.hpp>

BEGIN_NCBI_SCOPE
// generated classes

void CSoapBody_Base::ResetAnyContent(void)
{
    m_AnyContent.clear();
    m_set_State[0] &= ~0x3;
}

void CSoapBody_Base::Reset(void)
{
    ResetAnyContent();
}

BEGIN_NAMED_BASE_CLASS_INFO("Body", CSoapBody)
{
    SET_CLASS_MODULE("soap_11");
    SET_NAMESPACE("http://schemas.xmlsoap.org/soap/envelope/")->SetNsQualified(false);
    ADD_NAMED_MEMBER("AnyContent", m_AnyContent, STL_list, (STL_CRef, (STD, (ncbi::CAnyContentObject))))->SetOptional()->SetSetFlag(MEMBER_PTR(m_set_State[0]))->SetNoPrefix()->SetNotag();
    info->RandomOrder();
}
END_CLASS_INFO

// constructor
CSoapBody_Base::CSoapBody_Base(void)
{
    memset(m_set_State,0,sizeof(m_set_State));
}

// destructor
CSoapBody_Base::~CSoapBody_Base(void)
{
}

END_NCBI_SCOPE
