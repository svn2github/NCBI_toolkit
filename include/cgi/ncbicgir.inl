#if defined(NCBICGIR__HPP)  &&  !defined(NCBICGIR__INL)
#define NCBICGIR__INL

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
* Author: Eugene Vasilchenko
*
* File Description:
*  Inline methods of CGI response generator class
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.1  1998/12/09 20:18:11  vasilche
* Initial implementation of CGI response generator
*
* ===========================================================================
*/

void CCgiResponse::SetContentType(const string& type)
{
    m_ContentType = type;
}

void CCgiResponse::SetDate(const tm& date)
{
    m_Date = date;
}

void CCgiResponse::SetLastModified(const tm& date)
{
    m_LastModified = date;
}

void CCgiResponse::SetExpires(const tm& date)
{
    m_Expires = date;
}

bool CCgiResponse::GetContentType(string& type) const
{
    return s_GetString(type, m_ContentType);
}

bool CCgiResponse::GetDate(string &date) const
{
    return s_GetDate(date, m_Date);
}

bool CCgiResponse::GetLastModified(string &date) const
{
    return s_GetDate(date, m_LastModified);
}

bool CCgiResponse::GetExpires(string &date) const
{
    return s_GetDate(date, m_Expires);
}

bool CCgiResponse::GetDate(tm &date) const
{
    return s_GetDate(date, m_Date);
}

bool CCgiResponse::GetLastModified(tm &date) const
{
    return s_GetDate(date, m_LastModified);
}

bool CCgiResponse::GetExpires(tm &date) const
{
    return s_GetDate(date, m_Expires);
}

#endif /* def NCBICGIR__HPP  &&  ndef NCBICGIR__INL */
