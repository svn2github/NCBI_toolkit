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
* Revision 1.25  2000/02/17 20:02:44  vasilche
* Added some standard serialization exceptions.
* Optimized text/binary ASN.1 reading.
* Fixed wrong encoding of StringStore in ASN.1 binary format.
* Optimized logic of object collection.
*
* Revision 1.24  2000/01/10 19:46:40  vasilche
* Fixed encoding/decoding of REAL type.
* Fixed encoding/decoding of StringStore.
* Fixed encoding/decoding of NULL type.
* Fixed error reporting.
* Reduced object map (only classes).
*
* Revision 1.23  2000/01/05 19:43:54  vasilche
* Fixed error messages when reading from ASN.1 binary file.
* Fixed storing of integers with enumerated values in ASN.1 binary file.
* Added TAG support to key/value of map.
* Added support of NULL variant in CHOICE.
*
* Revision 1.22  1999/12/17 19:05:03  vasilche
* Simplified generation of GetTypeInfo methods.
*
* Revision 1.21  1999/10/19 15:41:03  vasilche
* Fixed reference to IOS_BASE
*
* Revision 1.20  1999/10/18 20:21:41  vasilche
* Enum values now have long type.
* Fixed template generation for enums.
*
* Revision 1.19  1999/10/08 21:00:43  vasilche
* Implemented automatic generation of unnamed ASN.1 types.
*
* Revision 1.18  1999/10/04 16:22:17  vasilche
* Fixed bug with old ASN.1 structures.
*
* Revision 1.17  1999/09/24 18:19:18  vasilche
* Removed dependency on NCBI toolkit.
*
* Revision 1.16  1999/09/23 21:16:08  vasilche
* Removed dependance on asn.h
*
* Revision 1.15  1999/09/23 18:56:59  vasilche
* Fixed bugs with overloaded methods in objistr*.hpp & objostr*.hpp
*
* Revision 1.14  1999/09/22 20:11:55  vasilche
* Modified for compilation on IRIX native c++ compiler.
*
* Revision 1.13  1999/09/14 18:54:18  vasilche
* Fixed bugs detected by gcc & egcs.
* Removed unneeded includes.
*
* Revision 1.12  1999/08/16 16:08:31  vasilche
* Added ENUMERATED type.
*
* Revision 1.11  1999/08/13 20:22:57  vasilche
* Fixed lot of bugs in datatool
*
* Revision 1.10  1999/08/13 15:53:51  vasilche
* C++ analog of asntool: datatool
*
* Revision 1.9  1999/07/26 18:31:37  vasilche
* Implemented skipping of unused values.
* Added more useful error report.
*
* Revision 1.8  1999/07/22 20:36:38  vasilche
* Fixed 'using namespace' declaration for MSVC.
*
* Revision 1.7  1999/07/22 19:40:56  vasilche
* Fixed bug with complex object graphs (pointers to members of other objects).
*
* Revision 1.6  1999/07/22 17:33:52  vasilche
* Unified reading/writing of objects in all three formats.
*
* Revision 1.5  1999/07/21 20:02:54  vasilche
* Added embedding of ASN.1 binary output from ToolKit to our binary format.
* Fixed bugs with storing pointers into binary ASN.1
*
* Revision 1.4  1999/07/21 14:20:05  vasilche
* Added serialization of bool.
*
* Revision 1.3  1999/07/09 16:32:54  vasilche
* Added OCTET STRING write/read.
*
* Revision 1.2  1999/07/07 21:15:02  vasilche
* Cleaned processing of string types (string, char*, const char*).
*
* Revision 1.1  1999/07/02 21:31:56  vasilche
* Implemented reading from ASN.1 binary format.
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>
#include <serial/objistrasnb.hpp>
#include <serial/classinfo.hpp>
#include <serial/member.hpp>
#include <serial/memberid.hpp>
#include <serial/enumerated.hpp>
#include <serial/memberlist.hpp>
#if HAVE_NCBI_C
# include <asn.h>
#endif

using namespace NCBI_NS_NCBI::CObjectStreamAsnBinaryDefs;

BEGIN_NCBI_SCOPE

CObjectIStreamAsnBinary::CObjectIStreamAsnBinary(CNcbiIstream& in)
    : m_Input(in), m_LastTagState(eNoTagRead)
{
#if CHECK_STREAM_INTEGRITY
    m_CurrentPosition = 0;
    m_CurrentTagState = eTagStart;
    m_CurrentTagLimit = INT_MAX;
#endif
}

CObjectIStreamAsnBinary::~CObjectIStreamAsnBinary(void)
{
#if CHECK_STREAM_INTEGRITY
    if ( !m_Limits.empty() || m_CurrentTagState != eTagStart ) {
        ThrowError(eFormatError, "CObjectIStreamAsnBinary not finished");
    }
#endif
}

#if CHECK_STREAM_INTEGRITY
inline
void CObjectIStreamAsnBinary::StartTag(TByte code)
{
    m_Limits.push(m_CurrentTagLimit);
    m_CurrentTagCode = code;
    m_CurrentTagPosition = m_CurrentPosition;
    m_CurrentTagState = (code & 0x1f) == eLongTag? eTagValue: eLengthStart;
}

inline
void CObjectIStreamAsnBinary::EndTag(void)
{
    if ( m_Limits.empty() ) {
        ThrowError(eFormatError, "too many tag ends");
    }
    m_CurrentTagState = eTagStart;
    m_CurrentTagLimit = m_Limits.top();
    m_Limits.pop();
}

inline
void CObjectIStreamAsnBinary::SetTagLength(size_t length)
{
    size_t limit = m_CurrentPosition + 1 + length;
    if ( limit > m_CurrentTagLimit ) {
        ThrowError(eOverflow, "tag will overflow enclosing tag");
    }
    else
        m_CurrentTagLimit = limit;
    if ( m_CurrentTagCode & 0x20 ) // constructed
        m_CurrentTagState = eTagStart;
    else
        m_CurrentTagState = eData;
    if ( length == 0 )
        EndTag();
}
#endif

#if !CHECK_STREAM_INTEGRITY
inline
#endif
unsigned char CObjectIStreamAsnBinary::ReadByte(void)
{
    char c;
    m_Input.get(c);
    CheckIOError(m_Input);
#if CHECK_STREAM_INTEGRITY
    TByte byte = TByte(c);
    if ( m_CurrentPosition >= m_CurrentTagLimit ) {
        ThrowError(eOverflow, "tag size overflow");
    }
    switch ( m_CurrentTagState ) {
    case eTagStart:
        StartTag(byte);
        break;
    case eTagValue:
        if ( byte & 0x80 )
            m_CurrentTagState = eLengthStart;
        break;
    case eLengthStart:
        if ( byte == 0 ) {
            SetTagLength(0);
            if ( m_CurrentTagCode == 0 )
                EndTag();
        }
        else if ( byte == 0x80 ) {
            if ( !(m_CurrentTagCode & 0x20) ) {
                ThrowError(eFormatError,
                           "cannot use indefinite form for primitive tag");
            }
            m_CurrentTagState = eTagStart;
        }
        else if ( byte < 0x80 ) {
            SetTagLength(byte);
        }
        else {
            m_CurrentTagLengthSize = byte - 0x80;
            if ( m_CurrentTagLengthSize > sizeof(size_t) ) {
                ThrowError(eOverflow, "too big length");
            }
            m_CurrentTagState = eLengthValueFirst;
        }
        break;
    case eLengthValueFirst:
        if ( byte == 0 ) {
            ThrowError(eFormatError, "first byte of length is zero");
        }
        if ( --m_CurrentTagLengthSize == 0 ) {
            SetTagLength(byte);
		}
		else {
			m_CurrentTagLength = byte;
			m_CurrentTagState = eLengthValue;
		}
		break;
    case eLengthValue:
        m_CurrentTagLength = (m_CurrentTagLength << 8) | byte;
        if ( --m_CurrentTagLengthSize == 0 )
            SetTagLength(m_CurrentTagLength);
        break;
    case eData:
        if ( m_CurrentPosition + 1 == m_CurrentTagLimit )
            EndTag();
        break;
    }
    m_CurrentPosition++;
#endif
    return c;
}

inline
signed char CObjectIStreamAsnBinary::ReadSByte(void)
{
    return ReadByte();
}

#if !CHECK_STREAM_INTEGRITY
inline
#endif
void CObjectIStreamAsnBinary::ReadBytes(char* buffer, size_t count)
{
    if ( count == 0 )
        return;
#if CHECK_STREAM_INTEGRITY
    if ( m_CurrentTagState != eData ) {
        ThrowError(eIllegalCall, "ReadBytes only allowed in DATA");
    }
    if ( m_CurrentPosition + count > m_CurrentTagLimit ) {
        ThrowError(eIllegalCall, "tag DATA overflow");
    }
    if ( (m_CurrentPosition += count) == m_CurrentTagLimit ) {
        m_CurrentPosition--;
        EndTag();
        m_CurrentPosition++;
    }
#endif
    m_Input.read(buffer, count);
    CheckIOError(m_Input);
}

#if !CHECK_STREAM_INTEGRITY
inline
#endif
void CObjectIStreamAsnBinary::SkipBytes(size_t count)
{
    if ( count == 0 )
        return;
#if CHECK_STREAM_INTEGRITY
    if ( m_CurrentTagState != eData ) {
        ThrowError(eIllegalCall, "SkipBytes only allowed in DATA");
    }
    if ( m_CurrentPosition + count > m_CurrentTagLimit ) {
        ThrowError(eIllegalCall, "tag DATA overflow");
    }
    if ( (m_CurrentPosition += count) == m_CurrentTagLimit ) {
        m_CurrentTagLimit--;
        EndTag();
        m_CurrentPosition++;
    }
#endif
    m_Input.seekg(count, IOS_BASE::cur);
    CheckIOError(m_Input);
}

void CObjectIStreamAsnBinary::ExpectByte(TByte byte)
{
    if ( ReadByte() != byte ) {
        ThrowError(eFormatError, "expected " + NStr::IntToString(byte));
    }
}

ETag CObjectIStreamAsnBinary::ReadSysTag(void)
{
    switch ( m_LastTagState ) {
    case eNoTagRead:
        m_LastTagState = eSysTagRead;
        m_LastTagByte = ReadByte();
        break;
    case eSysTagRead:
        ThrowError(eIllegalCall, "double tag read");
    case eSysTagBack:
        m_LastTagState = eSysTagRead;
        break;
    }
    return ETag(m_LastTagByte & 0x1f);
}

void CObjectIStreamAsnBinary::BackSysTag(void)
{
    switch ( m_LastTagState ) {
    case eNoTagRead:
        ThrowError(eIllegalCall, "tag back without read");
    case eSysTagBack:
        ThrowError(eIllegalCall, "double tag back");
    case eSysTagRead:
        m_LastTagState = eSysTagBack;
        break;
    }
}

void CObjectIStreamAsnBinary::FlushSysTag(bool constructed)
{
    switch ( m_LastTagState ) {
    case eNoTagRead:
        ThrowError(eIllegalCall, "length read without tag");
    case eSysTagRead:
        break;
    case eSysTagBack:
        ThrowError(eIllegalCall, "length read with tag back");
    }
    if ( constructed && (m_LastTagByte & 0x20) == 0 ) {
        ThrowError(eFormatError,
                   "non indefinite length on constructed value");
    }
    if ( !constructed && (m_LastTagByte & 0x20) != 0 ) {
        ThrowError(eFormatError,
                   "indefinite length on primitive value");
    }
    m_LastTagState = eNoTagRead;
}

TTag CObjectIStreamAsnBinary::ReadTag(void)
{
    ETag sysTag = ReadSysTag();
    if ( sysTag != eLongTag )
        return sysTag;
    TTag tag = 0;
    TByte byte;
    do {
        if ( tag >= (1 << (sizeof(tag) * 8 - 1 - 7)) ) {
            ThrowError(eOverflow, "tag number is too big");
        }
        byte = ReadByte();
        tag = (tag << 7) | (byte & 0x7f);
    } while ( (byte & 0x80) == 0 );
    return tag;
}

inline
ETag CObjectIStreamAsnBinary::GetLastTag(void) const
{
    return ETag(m_LastTagByte & 0x1f);
}

inline
bool CObjectIStreamAsnBinary::LastTagWas(EClass c, bool constructed) const
{
    return m_LastTagState == eSysTagRead &&
        (m_LastTagByte >> 5) == ((c << 1) | (constructed? 1: 0));
}

inline
bool CObjectIStreamAsnBinary::LastTagWas(EClass c, bool constructed,
                                         ETag tag) const
{
    return m_LastTagState == eSysTagRead &&
        m_LastTagByte == ((c << 6) | (constructed? 0x20: 0) | tag);
}

ETag CObjectIStreamAsnBinary::ReadSysTag(EClass c, bool constructed)
{
    ETag tag = ReadSysTag();
    if ( tag == eLongTag ) {
        ThrowError(eFormatError, "unexpected long tag");
    }

    if ( !LastTagWas(c, constructed) ) {
        ThrowError(eFormatError,
                   "unexpected tag class/type: should be: " +
                   NStr::IntToString(c) +  (constructed? "C": "p"));
    }
    return tag;
}

string CObjectIStreamAsnBinary::ReadClassTag(void)
{
    string name;
    TByte c;
    while ( ((c = ReadByte()) & 0x80) == 0 ) {
        name += char(c);
    }
    return name + char(c & 0x7f);
}

TTag CObjectIStreamAsnBinary::ReadTag(EClass c, bool constructed)
{
    TTag tag = ReadTag();
    if ( !LastTagWas(c, constructed) ) {
        ThrowError(eFormatError,
                   "unexpected tag class/type: should be: " +
                   NStr::IntToString(c) +  (constructed? "C": "p"));
    }
    return tag;
}

void CObjectIStreamAsnBinary::ExpectSysTag(EClass c, bool constructed,
                                           ETag tag)
{
    if ( ReadSysTag(c, constructed) != tag ) {
        ThrowError(eFormatError,
                   "unexpected tag: " + NStr::IntToString(GetLastTag()) +
                   ", should be: " + NStr::IntToString(tag));
    }
}

inline
void CObjectIStreamAsnBinary::ExpectSysTag(ETag tag)
{
    ExpectSysTag(eUniversal, false, tag);
}

size_t CObjectIStreamAsnBinary::ReadShortLength(void)
{
    FlushSysTag();
    TByte byte = ReadByte();
    if ( byte >= 0x80 ) {
        ThrowError(eFormatError, "short length expected");
    }
    return byte;
}

size_t CObjectIStreamAsnBinary::ReadLength(bool allowIndefinite)
{
    FlushSysTag();
    TByte byte = ReadByte();
    if ( byte < 0x80 )
        return byte;
    size_t lengthLength = byte - 0x80;
    if ( lengthLength == 0 ) {
        if ( allowIndefinite )
            return size_t(-1);
        ThrowError(eFormatError, "unexpected indefinite length");
    }
    if ( lengthLength > sizeof(size_t) ) {
        ThrowError(eOverflow, "length overflow");
    }
    byte = ReadByte();
    if ( byte == 0 ) {
        ThrowError(eFormatError, "illegal length start");
    }
    if ( size_t(-1) < size_t(0) ) {
        if ( lengthLength == sizeof(size_t) && (byte & 0x80) != 0 ) {
            ThrowError(eOverflow, "length overflow");
        }
    }
    lengthLength--;
    size_t length = byte;
    while ( lengthLength-- > 0 )
        length = (length << 8) | ReadByte();
    return length;
}

void CObjectIStreamAsnBinary::ExpectShortLength(size_t length)
{
    if ( ReadShortLength() != length ) {
        ThrowError(eFormatError, "length expected");
    }
}

inline
void CObjectIStreamAsnBinary::ExpectIndefiniteLength(void)
{
    FlushSysTag(true);
    ExpectByte(0x80);
}

inline
void CObjectIStreamAsnBinary::ExpectEndOfContent(void)
{
    ExpectSysTag(eNone);
    ExpectShortLength(0);
}

template<typename T>
void ReadStdSigned(CObjectIStreamAsnBinary& in, T& data)
{
    size_t length = in.ReadShortLength();
    if ( length == 0 ) {
        in.ThrowError(in.eFormatError, "zero length of number");
    }
    T n;
    if ( length > sizeof(data) ) {
        // skip
        signed char c = in.ReadSByte();
        if ( c != 0 || c != -1 ) {
            in.ThrowError(in.eOverflow, "overflow error");
        }
        length--;
        while ( length-- > sizeof(data) )
            in.ExpectByte(c);
        length--;
        n = in.ReadSByte();
        if ( ((n ^ c) & 0x80) != 0 ) {
            in.ThrowError(in.eOverflow, "overflow error");
        }
    }
    else {
        length--;
        n = in.ReadSByte();
    }
    while ( length-- > 0 ) {
        n = (n << 8) | in.ReadByte();
    }
    data = n;
}

template<typename T>
void ReadStdUnsigned(CObjectIStreamAsnBinary& in, T& data)
{
    size_t length = in.ReadShortLength();
    if ( length == 0 ) {
        in.ThrowError(in.eFormatError, "zero length of number");
    }
    T n;
    if ( length > sizeof(data) ) {
        // skip
        while ( length-- > sizeof(data) )
            in.ExpectByte(0);
        length--;
        n = in.ReadByte();
        if ( (n & 0x80) != 0 ) {
            in.ThrowError(in.eOverflow, "overflow error");
        }
    }
    else if ( length == sizeof(data) ) {
        length--;
        n = in.ReadByte();
        if ( (n & 0x80) != 0 ) {
            in.ThrowError(in.eOverflow, "overflow error");
        }
    }
    else {
        n = 0;
    }
    while ( length-- > 0 ) {
        n = (n << 8) | in.ReadByte();
    }
    data = n;
}

bool CObjectIStreamAsnBinary::ReadBool(void)
{
    ExpectSysTag(eBoolean);
    ExpectShortLength(1);
    return ReadByte() != 0;
}

char CObjectIStreamAsnBinary::ReadChar(void)
{
    ExpectSysTag(eGeneralString);
    ExpectShortLength(1);
    return ReadByte();
}

int CObjectIStreamAsnBinary::ReadInt(void)
{
    ExpectSysTag(eInteger);
    int data;
    ReadStdSigned(*this, data);
    return data;
}

unsigned CObjectIStreamAsnBinary::ReadUInt(void)
{
    ExpectSysTag(eInteger);
    unsigned data;
    ReadStdUnsigned(*this, data);
    return data;
}

long CObjectIStreamAsnBinary::ReadLong(void)
{
#if LONG_MIN == INT_MIN && LONG_MAX == INT_MAX
    return ReadInt();
#else
    ExpectSysTag(eInteger);
    long data;
    ReadStdSigned(*this, data);
    return data;
#endif
}

unsigned long CObjectIStreamAsnBinary::ReadULong(void)
{
#if ULONG_MAX == UINT_MAX
    return ReadUInt();
#else
    ExpectSysTag(eInteger);
    unsigned long data;
    ReadStdUnsigned(*this, data);
    return data;
#endif
}

double CObjectIStreamAsnBinary::ReadDouble(void)
{
    ExpectSysTag(eReal);
    size_t length = ReadShortLength();
    if ( length < 2 ) {
        ThrowError(eFormatError, "too short REAL data");
    }
    if ( length > 32 ) {
        ThrowError(eFormatError, "too long REAL data");
    }

    ExpectByte(eDecimal);
    length--;
    char buffer[32];
    ReadBytes(buffer, length);
    buffer[length] = 0;
    char* endptr;
    double data = strtod(buffer, &endptr);
    if ( *endptr != 0 ) {
        ThrowError(eFormatError, "bad REAL data string");
    }
    return data;
}

void CObjectIStreamAsnBinary::ReadString(string& s)
{
    s.erase();
    ExpectSysTag(eVisibleString);
    size_t length = ReadLength();
    s.reserve(length);
    while ( length > 0 ) {
        char buffer[128];
        size_t count = length;
        if ( count > sizeof(buffer) )
            count = sizeof(buffer);
        ReadBytes(buffer, count);
        s.append(buffer, count);
        length -= count;
    }
}

void CObjectIStreamAsnBinary::ReadStringStore(string& s)
{
    s.erase();
    ExpectSysTag(eApplication, false, eStringStore);
    size_t length = ReadLength();
    s.reserve(length);
    while ( length > 0 ) {
        char buffer[128];
        size_t count = length;
        if ( count > sizeof(buffer) )
            count = sizeof(buffer);
        ReadBytes(buffer, count);
        s.append(buffer, count);
        length -= count;
    }
}

char* CObjectIStreamAsnBinary::ReadCString(void)
{
    ExpectSysTag(eVisibleString);
    size_t length = ReadLength();
	char* s = static_cast<char*>(malloc(length + 1));
	ReadBytes(s, length);
	s[length] = 0;
    return s;
}

void CObjectIStreamAsnBinary::VBegin(Block& block)
{
    ExpectSysTag(eUniversal, true, block.RandomOrder()? eSet: eSequence);
    ExpectIndefiniteLength();
}

bool CObjectIStreamAsnBinary::VNext(const Block& )
{
    if ( ReadSysTag() == eNone &&
         LastTagWas(eUniversal, false) ) {
        ExpectShortLength(0);
        return false;
    }
    else {
        BackSysTag();
        return true;
    }
}

void CObjectIStreamAsnBinary::StartMember(Member& m, const CMemberId& member)
{
    TTag tag = ReadTag(eContextSpecific, true);
    ExpectIndefiniteLength();
    if ( member.GetTag() != tag ) {
        ThrowError(eFormatError,
                   "expected " + member.ToString() +", found [" +
                   NStr::IntToString(tag) + "]");
    }
    SetIndex(m, 0, member);
}

void CObjectIStreamAsnBinary::StartMember(Member& m, const CMembers& members)
{
    TTag tag = ReadTag(eContextSpecific, true);
    ExpectIndefiniteLength();
    TMemberIndex index = members.FindMember(tag);
    if ( index < 0 ) {
        ThrowError(eFormatError,
                   "unexpected member: [" +
                   NStr::IntToString(tag) + "]");
    }
    SetIndex(m, index, members.GetMemberId(index));
}

void CObjectIStreamAsnBinary::StartMember(Member& m, LastMember& lastMember)
{
    TTag tag = ReadTag(eContextSpecific, true);
    ExpectIndefiniteLength();
    TMemberIndex index =
        lastMember.GetMembers().FindMember(tag, lastMember.GetIndex());
    if ( index < 0 ) {
        ThrowError(eFormatError,
                   "unexpected member: [" +
                   NStr::IntToString(tag) + "]");
    }
    SetIndex(lastMember, index);
    SetIndex(m, index, lastMember.GetMembers().GetMemberId(index));
}

void CObjectIStreamAsnBinary::EndMember(const Member& )
{
    ExpectEndOfContent();
}

void CObjectIStreamAsnBinary::Begin(ByteBlock& block)
{
	ExpectSysTag(eOctetString);
	SetBlockLength(block, ReadLength());
}

size_t CObjectIStreamAsnBinary::ReadBytes(const ByteBlock& ,
                                          char* dst, size_t length)
{
	ReadBytes(dst, length);
	return length;
}

void CObjectIStreamAsnBinary::ReadNull(void)
{
    ExpectSysTag(eNull);
    ExpectShortLength(0);
}

CObjectIStream::EPointerType CObjectIStreamAsnBinary::ReadPointerType(void)
{
    ETag tag = ReadSysTag();
    if ( LastTagWas(eUniversal, false) ) {
        switch ( tag ) {
        case eNull:
            ExpectShortLength(0);
            return eNullPointer;
        default:
            break;
        }
    }
    else if ( LastTagWas(eApplication, true) ) {
        switch ( tag ) {
        case eMemberReference:
            ExpectIndefiniteLength();
            return eMemberPointer;
        case eLongTag:
            return eOtherPointer;
        default:
            break;
        }
    }
    else if ( LastTagWas(eApplication, false) ) {
        switch ( tag) {
        case eObjectReference:
            return eObjectPointer;
        default:
            break;
        }
    }
    // by default: try this class
    BackSysTag();
    return eThisPointer;
}

pair<long, bool>
CObjectIStreamAsnBinary::ReadEnum(const CEnumeratedTypeValues& values)
{
    if ( values.IsInteger() ) {
        // allow any integer
        return make_pair(0l, false);
    }
    
    // enum element by value
    ExpectSysTag(eEnumerated);
    long value;
    ReadStdSigned(*this, value);
    values.FindName(value, false); // check value
    return make_pair(value, true);
}

CObjectIStreamAsnBinary::TMemberIndex
CObjectIStreamAsnBinary::ReadMemberSuffix(const CMembers& members)
{
    string member;
    ReadString(member);
    ExpectEndOfContent();
    TMemberIndex index = members.FindMember(member);
    if ( index < 0 ) {
        ThrowError(eFormatError,
                   "member not found: " + member);
    }
    return index;
}

CObjectIStream::TIndex CObjectIStreamAsnBinary::ReadObjectPointer(void)
{
    unsigned data;
    ReadStdUnsigned(*this, data);
    return data;
}

string CObjectIStreamAsnBinary::ReadOtherPointer(void)
{
    string className = ReadClassTag();
    ExpectIndefiniteLength();
    return className;
}

void CObjectIStreamAsnBinary::ReadOtherPointerEnd(void)
{
    ExpectEndOfContent();
}

bool CObjectIStreamAsnBinary::SkipRealValue(void)
{
    if ( ReadSysTag() == eLongTag ) {
        // long tag id
        while ( (ReadByte() & 0x80) == 0 )
            ;
    }

    if ( m_LastTagByte & 0x20 ) {
        // constructed
        ExpectIndefiniteLength();
        while ( SkipRealValue() )
            ;
    }
    else {
        size_t length = ReadLength(true);
        if ( length == size_t(-1) ) {
            // indefinite length
            while ( SkipRealValue() )
                ;
        }
        else if ( length == 0 )
            return m_LastTagByte != 0;
        else
            SkipBytes(length);
    }
    return true;
}

void CObjectIStreamAsnBinary::SkipValue()
{
    if ( !SkipRealValue() ) {
        ThrowError(eFormatError, "unexpected end of contents");
    }
}

#if HAVE_NCBI_C
unsigned CObjectIStreamAsnBinary::GetAsnFlags(void)
{
    return ASNIO_BIN;
}

void CObjectIStreamAsnBinary::AsnOpen(AsnIo& )
{
    if ( m_LastTagByte == eSysTagRead ) {
        ThrowError(eIllegalCall, "double tag read");
    }
}

size_t CObjectIStreamAsnBinary::AsnRead(AsnIo& , char* data, size_t )
{
    if ( m_LastTagState == eSysTagBack ) {
        m_LastTagByte = eNoTagRead;
        *data = m_LastTagByte;
        return 1;
    }
    *data = ReadByte();
    return 1;
}
#endif

END_NCBI_SCOPE
