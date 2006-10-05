#if defined(OBJISTRXML__HPP)  &&  !defined(OBJISTRXML__INL)
#define OBJISTRXML__INL

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
*   !!! PUT YOUR DESCRIPTION HERE !!!
*/

inline
bool CObjectIStreamXml::InsideTag(void) const
{
    return m_TagState == eTagInsideOpening || m_TagState == eTagInsideClosing;
}

inline
bool CObjectIStreamXml::InsideOpeningTag(void) const
{
    return m_TagState == eTagInsideOpening;
}

inline
bool CObjectIStreamXml::InsideClosingTag(void) const
{
    return m_TagState == eTagInsideClosing;
}

inline
bool CObjectIStreamXml::OutsideTag(void) const
{
    return (m_TagState == eTagOutside) || m_Attlist;
}

inline
bool CObjectIStreamXml::SelfClosedTag(void) const
{
    return m_TagState == eTagSelfClosed;
}

inline
void CObjectIStreamXml::Found_lt(void)
{
    _ASSERT(OutsideTag());
    m_TagState = eTagInsideOpening;
}

inline
void CObjectIStreamXml::Back_lt(void)
{
    _ASSERT(InsideOpeningTag());
    m_TagState = eTagOutside;
}

inline
void CObjectIStreamXml::Found_lt_slash(void)
{
    _ASSERT(OutsideTag());
    m_TagState = eTagInsideClosing;
}

inline
void CObjectIStreamXml::Found_gt(void)
{
    _ASSERT(InsideTag());
    m_TagState = eTagOutside;
}

inline
void CObjectIStreamXml::Found_slash_gt(void)
{
    _ASSERT(InsideOpeningTag());
    m_TagState = eTagSelfClosed;
}

inline
void CObjectIStreamXml::EndSelfClosedTag(void)
{
    _ASSERT(SelfClosedTag());
    m_TagState = eTagOutside;
}

inline
void CObjectIStreamXml::EndOpeningTag(void)
{
    _ASSERT(InsideOpeningTag());
    EndTag();
}

inline
void CObjectIStreamXml::EndClosingTag(void)
{
    _ASSERT(InsideClosingTag());
    EndTag();
}

inline
void CObjectIStreamXml::BeginData(void)
{
    if ( InsideOpeningTag() )
        EndOpeningTag();
    _ASSERT(OutsideTag());
}

inline
CLightString CObjectIStreamXml::SkipTagName(CLightString tag, const char* s)
{
    return SkipTagName(tag, s, strlen(s));
}

inline
CLightString CObjectIStreamXml::SkipTagName(CLightString tag, const string& s)
{
    return SkipTagName(tag, s.data(), s.size());
}

inline
void CObjectIStreamXml::OpenTag(TTypeInfo type)
{
    _ASSERT(!type->GetName().empty());
    OpenTag(type->GetName());
}

inline
void CObjectIStreamXml::CloseTag(TTypeInfo type)
{
    _ASSERT(!type->GetName().empty());
    CloseTag(type->GetName());
}

#endif /* def OBJISTRXML__HPP  &&  ndef OBJISTRXML__INL */



/* ---------------------------------------------------------------------------
* $Log$
* Revision 1.1  2006/10/05 19:23:37  gouriano
* Moved from parent folder
*
* Revision 1.3  2002/12/23 18:38:51  dicuccio
* Added WIn32 export specifier: NCBI_XSERIAL_EXPORT.
* Moved all CVS logs to the end.
*
* Revision 1.2  2002/11/14 20:51:27  gouriano
* added support of attribute lists
*
* Revision 1.1  2000/09/18 20:00:06  vasilche
* Separated CVariantInfo and CMemberInfo.
* Implemented copy hooks.
* All hooks now are stored in CTypeInfo/CMemberInfo/CVariantInfo.
* Most type specific functions now are implemented via function pointers instead of virtual functions.
*
* ===========================================================================
*/
