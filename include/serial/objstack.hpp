#ifndef OBJSTACK__HPP
#define OBJSTACK__HPP

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
* Revision 1.11  2002/10/25 14:49:29  vasilche
* NCBI C Toolkit compatibility code extracted to libxcser library.
* Serial streams flags names were renamed to fXxx.
*
* Names of flags
*
* Revision 1.10  2002/10/15 13:40:33  gouriano
* added "skiptag" flag
*
* Revision 1.9  2002/09/26 18:12:27  gouriano
* added HasMemberId method
*
* Revision 1.8  2001/05/17 14:59:47  lavr
* Typos corrected
*
* Revision 1.7  2000/10/20 15:51:28  vasilche
* Fixed data error processing.
* Added interface for constructing container objects directly into output stream.
* object.hpp, object.inl and object.cpp were split to
* objectinfo.*, objecttype.*, objectiter.* and objectio.*.
*
* Revision 1.6  2000/09/18 20:00:07  vasilche
* Separated CVariantInfo and CMemberInfo.
* Implemented copy hooks.
* All hooks now are stored in CTypeInfo/CMemberInfo/CVariantInfo.
* Most type specific functions now are implemented via function pointers instead of virtual functions.
*
* Revision 1.5  2000/09/01 13:16:02  vasilche
* Implemented class/container/choice iterators.
* Implemented CObjectStreamCopier for copying data without loading into memory.
*
* Revision 1.4  2000/08/15 19:44:41  vasilche
* Added Read/Write hooks:
* CReadObjectHook/CWriteObjectHook for objects of specified type.
* CReadClassMemberHook/CWriteClassMemberHook for specified members.
* CReadChoiceVariantHook/CWriteChoiceVariant for specified choice variants.
* CReadContainerElementHook/CWriteContainerElementsHook for containers.
*
* Revision 1.3  2000/06/07 19:45:44  vasilche
* Some code cleaning.
* Macros renaming in more clear way.
* BEGIN_NAMED_*_INFO, ADD_*_MEMBER, ADD_NAMED_*_MEMBER.
*
* Revision 1.2  2000/06/01 19:06:58  vasilche
* Added parsing of XML data.
*
* Revision 1.1  2000/05/24 20:08:14  vasilche
* Implemented XML dump.
*
* ===========================================================================
*/

#define VIRTUAL_MID_LEVEL_IO 1
//#undef VIRTUAL_MID_LEVEL_IO

#ifdef VIRTUAL_MID_LEVEL_IO
# define MLIOVIR virtual
#else
# define MLIOVIR
#endif

#include <corelib/ncbistd.hpp>
#include <serial/memberid.hpp>
#include <serial/typeinfo.hpp>

BEGIN_NCBI_SCOPE

class CObjectStack;

class CObjectStackFrame
{
public:
    enum EFrameType {
        eFrameOther,
        eFrameNamed,
        eFrameArray,
        eFrameArrayElement,
        eFrameClass,
        eFrameClassMember,
        eFrameChoice,
        eFrameChoiceVariant
    };

    void Reset(void);
    
    EFrameType GetFrameType(void) const;
    TTypeInfo GetTypeInfo(void) const;
    bool HasMemberId(void) const;
    const CMemberId& GetMemberId(void) const;
    void SetMemberId(const CMemberId& memberid);

    void SetSkipTag(void);
    bool GetSkipTag(void);

private:
    friend class CObjectStack;

    EFrameType m_FrameType;
    TTypeInfo m_TypeInfo;
    const CMemberId* m_MemberId;
    bool m_SkipTag;
};

class CObjectStack
{
public:
    typedef CObjectStackFrame TFrame;
    typedef TFrame::EFrameType EFrameType;

    CObjectStack(void);
    virtual ~CObjectStack(void);

    size_t GetStackDepth(void) const;

    TFrame& PushFrame(EFrameType type, TTypeInfo typeInfo);
    TFrame& PushFrame(EFrameType type, const CMemberId& memberId);
    TFrame& PushFrame(EFrameType type);

    void PopFrame(void);
    void PopErrorFrame(void);

    void SetTopMemberId(const CMemberId& memberId);

protected:
    bool StackIsEmpty(void) const;

    void ClearStack(void);

    string GetStackTraceASN(void) const;

    const TFrame& TopFrame(void) const;
    TFrame& TopFrame(void);

    TFrame& FetchFrameFromTop(size_t index);
    const TFrame& FetchFrameFromTop(size_t index) const;
    const TFrame& FetchFrameFromBottom(size_t index) const;

    virtual void UnendedFrame(void);

private:
    TFrame& PushFrame(void);
    TFrame& PushFrameLong(void);

    TFrame* m_Stack;
    TFrame* m_StackPtr;
    TFrame* m_StackEnd;
};

#include <serial/objstack.inl>

#define BEGIN_OBJECT_FRAME_OFx(Stream, Args) \
    (Stream).PushFrame Args; \
    try {
#define END_OBJECT_FRAME_OF(Stream) \
    } catch (...) { \
        (Stream).PopErrorFrame(); \
        throw; \
    } \
    (Stream).PopFrame()

#define BEGIN_OBJECT_FRAME_OF(Stream, Type) \
    BEGIN_OBJECT_FRAME_OFx(Stream, (CObjectStackFrame::Type))
#define BEGIN_OBJECT_FRAME_OF2(Stream, Type, Arg) \
    BEGIN_OBJECT_FRAME_OFx(Stream, (CObjectStackFrame::Type, Arg))

#define BEGIN_OBJECT_FRAME(Type) BEGIN_OBJECT_FRAME_OF(*this, Type)
#define BEGIN_OBJECT_FRAME2(Type, Arg) BEGIN_OBJECT_FRAME_OF2(*this, Type, Arg)
#define END_OBJECT_FRAME() END_OBJECT_FRAME_OF(*this)

#define BEGIN_OBJECT_2FRAMES_OFx(Stream, Args) \
    (Stream).In().PushFrame Args; \
    (Stream).Out().PushFrame Args; \
    try {
#define END_OBJECT_2FRAMES_OF(Stream) \
    } catch (...) { \
        (Stream).Out().PopFrame(); \
        (Stream).Out().SetFailFlagsNoError(CObjectOStream::fInvalidData); \
        (Stream).In().PopErrorFrame(); \
        throw; \
    } \
    (Stream).Out().PopFrame(); \
    (Stream).In().PopFrame()

#define BEGIN_OBJECT_2FRAMES_OF(Stream, Type) \
    BEGIN_OBJECT_2FRAMES_OFx(Stream, (CObjectStackFrame::Type))
#define BEGIN_OBJECT_2FRAMES_OF2(Stream, Type, Arg) \
    BEGIN_OBJECT_2FRAMES_OFx(Stream, (CObjectStackFrame::Type, Arg))
#define BEGIN_OBJECT_2FRAMES(Type) BEGIN_OBJECT_2FRAMES_OF(*this, Type)
#define BEGIN_OBJECT_2FRAMES2(Type, Arg) BEGIN_OBJECT_2FRAMES_OF2(*this, Type, Arg)
#define END_OBJECT_2FRAMES() END_OBJECT_2FRAMES_OF(*this)

END_NCBI_SCOPE

#endif  /* OBJSTACK__HPP */
