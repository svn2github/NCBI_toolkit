#if defined(MEMBER__HPP)  &&  !defined(MEMBER__INL)
#define MEMBER__INL

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
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.12  2002/09/09 18:13:59  grichenk
* Added CObjectHookGuard class.
* Added methods to be used by hooks for data
* reading and skipping.
*
* Revision 1.11  2000/10/13 20:22:46  vasilche
* Fixed warnings on 64 bit compilers.
* Fixed missing typename in templates.
*
* Revision 1.10  2000/09/29 16:18:13  vasilche
* Fixed binary format encoding/decoding on 64 bit compulers.
* Implemented CWeakMap<> for automatic cleaning map entries.
* Added cleaning local hooks via CWeakMap<>.
* Renamed ReadTypeName -> ReadFileHeader, ENoTypeName -> ENoFileHeader.
* Added some user interface methods to CObjectIStream, CObjectOStream and
* CObjectStreamCopier.
*
* Revision 1.9  2000/09/19 14:10:24  vasilche
* Added files to MSVC project
* Updated shell scripts to use new datattool path on MSVC
* Fixed internal compiler error on MSVC
*
* Revision 1.8  2000/09/18 20:00:02  vasilche
* Separated CVariantInfo and CMemberInfo.
* Implemented copy hooks.
* All hooks now are stored in CTypeInfo/CMemberInfo/CVariantInfo.
* Most type specific functions now are implemented via function pointers instead of virtual functions.
*
* Revision 1.7  2000/08/15 19:44:39  vasilche
* Added Read/Write hooks:
* CReadObjectHook/CWriteObjectHook for objects of specified type.
* CReadClassMemberHook/CWriteClassMemberHook for specified members.
* CReadChoiceVariantHook/CWriteChoiceVariant for specified choice variants.
* CReadContainerElementHook/CWriteContainerElementsHook for containers.
*
* Revision 1.6  2000/02/01 21:44:35  vasilche
* Added CGeneratedChoiceTypeInfo for generated choice classes.
* Added buffering to CObjectIStreamAsn.
* Removed CMemberInfo subclasses.
* Added support for DEFAULT/OPTIONAL members.
*
* Revision 1.5  1999/09/01 17:38:01  vasilche
* Fixed vector<char> implementation.
* Added explicit naming of class info.
* Moved IMPLICIT attribute from member info to class info.
*
* Revision 1.4  1999/08/31 17:50:04  vasilche
* Implemented several macros for specific data types.
* Added implicit members.
* Added multimap and set.
*
* Revision 1.3  1999/08/13 15:53:42  vasilche
* C++ analog of asntool: datatool
*
* Revision 1.2  1999/06/30 16:04:22  vasilche
* Added support for old ASN.1 structures.
*
* Revision 1.1  1999/06/24 14:44:39  vasilche
* Added binary ASN.1 output.
*
* ===========================================================================
*/

inline
const CClassTypeInfo* CMemberInfo::GetClassType(void) const
{
    return m_ClassType;
}

inline
bool CMemberInfo::Optional(void) const
{
    return m_Optional;
}

inline
TConstObjectPtr CMemberInfo::GetDefault(void) const
{
    return m_Default;
}

inline
bool CMemberInfo::HaveSetFlag(void) const
{
    return m_SetFlagOffset != eNoOffset;
}

inline
bool CMemberInfo::GetSetFlag(TConstObjectPtr object) const
{
    return CTypeConverter<bool>::Get(Add(object, m_SetFlagOffset));
}

inline
bool& CMemberInfo::GetSetFlag(TObjectPtr object) const
{
    return CTypeConverter<bool>::Get(Add(object, m_SetFlagOffset));
}

inline
void CMemberInfo::UpdateSetFlag(TObjectPtr object, bool value) const
{
    TPointerOffsetType setFlagOffset = m_SetFlagOffset;
    if ( setFlagOffset != eNoOffset )
        CTypeConverter<bool>::Get(Add(object, setFlagOffset)) = value;
}

inline
bool CMemberInfo::CanBeDelayed(void) const
{
    return m_DelayOffset != eNoOffset;
}

inline
CDelayBuffer& CMemberInfo::GetDelayBuffer(TObjectPtr object) const
{
    return CTypeConverter<CDelayBuffer>::Get(Add(object, m_DelayOffset));
}

inline
const CDelayBuffer& CMemberInfo::GetDelayBuffer(TConstObjectPtr object) const
{
    return CTypeConverter<const CDelayBuffer>::Get(Add(object, m_DelayOffset));
}

inline
TConstObjectPtr CMemberInfo::GetMemberPtr(TConstObjectPtr classPtr) const
{
    return m_GetConstFunction(this, classPtr);
}

inline
TObjectPtr CMemberInfo::GetMemberPtr(TObjectPtr classPtr) const
{
    return m_GetFunction(this, classPtr);
}

inline
void CMemberInfo::ReadMember(CObjectIStream& stream,
                             TObjectPtr classPtr) const
{
    m_ReadHookData.GetCurrentFunction().m_Main(stream, this, classPtr);
}

inline
void CMemberInfo::ReadMissingMember(CObjectIStream& stream,
                                    TObjectPtr classPtr) const
{
    m_ReadHookData.GetCurrentFunction().m_Missing(stream, this, classPtr);
}

inline
void CMemberInfo::WriteMember(CObjectOStream& stream,
                              TConstObjectPtr classPtr) const
{
    m_WriteHookData.GetCurrentFunction()(stream, this, classPtr);
}

inline
void CMemberInfo::CopyMember(CObjectStreamCopier& stream) const
{
    m_CopyHookData.GetCurrentFunction().m_Main(stream, this);
}

inline
void CMemberInfo::CopyMissingMember(CObjectStreamCopier& stream) const
{
    m_CopyHookData.GetCurrentFunction().m_Missing(stream, this);
}

inline
void CMemberInfo::SkipMember(CObjectIStream& stream) const
{
    if ( !m_ReadHookData.HaveHooks() ) {
        m_SkipFunction(stream, this);
    }
    else {
        TObjectPtr object = CreateClass();
        ReadMember(stream, object);
    }
}

inline
void CMemberInfo::SkipMissingMember(CObjectIStream& stream) const
{
    if ( !m_ReadHookData.HaveHooks() ) {
        m_SkipMissingFunction(stream, this);
    }
    else {
        TObjectPtr object = CreateClass();
        ReadMissingMember(stream, object);
    }
}

inline
void CMemberInfo::DefaultReadMember(CObjectIStream& stream,
                                    TObjectPtr classPtr) const
{
    m_ReadHookData.GetDefaultFunction().m_Main(stream, this, classPtr);
}

inline
void CMemberInfo::DefaultReadMissingMember(CObjectIStream& stream,
                                           TObjectPtr classPtr) const
{
    m_ReadHookData.GetDefaultFunction().m_Missing(stream, this, classPtr);
}

inline
void CMemberInfo::DefaultWriteMember(CObjectOStream& stream,
                                     TConstObjectPtr classPtr) const
{
    m_WriteHookData.GetDefaultFunction()(stream, this, classPtr);
}

inline
void CMemberInfo::DefaultCopyMember(CObjectStreamCopier& stream) const
{
    m_CopyHookData.GetDefaultFunction().m_Main(stream, this);
}

inline
void CMemberInfo::DefaultCopyMissingMember(CObjectStreamCopier& stream) const
{
    m_CopyHookData.GetDefaultFunction().m_Missing(stream, this);
}

inline
void CMemberInfo::DefaultSkipMember(CObjectIStream& stream) const
{
    m_SkipFunction(stream, this);
}

inline
void CMemberInfo::DefaultSkipMissingMember(CObjectIStream& stream) const
{
    m_SkipMissingFunction(stream, this);
}

#endif /* def MEMBER__HPP  &&  ndef MEMBER__INL */
