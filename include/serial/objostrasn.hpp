#ifndef OBJOSTRASN__HPP
#define OBJOSTRASN__HPP

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
* Revision 1.19  2000/01/10 20:12:37  vasilche
* Fixed duplicate argument names.
* Fixed conflict between template and variable name.
*
* Revision 1.18  2000/01/10 19:46:32  vasilche
* Fixed encoding/decoding of REAL type.
* Fixed encoding/decoding of StringStore.
* Fixed encoding/decoding of NULL type.
* Fixed error reporting.
* Reduced object map (only classes).
*
* Revision 1.17  1999/09/24 18:19:14  vasilche
* Removed dependency on NCBI toolkit.
*
* Revision 1.16  1999/09/23 18:56:53  vasilche
* Fixed bugs with overloaded methods in objistr*.hpp & objostr*.hpp
*
* Revision 1.15  1999/09/22 20:11:50  vasilche
* Modified for compilation on IRIX native c++ compiler.
*
* Revision 1.14  1999/08/13 15:53:44  vasilche
* C++ analog of asntool: datatool
*
* Revision 1.13  1999/07/22 17:33:44  vasilche
* Unified reading/writing of objects in all three formats.
*
* Revision 1.12  1999/07/21 14:19:59  vasilche
* Added serialization of bool.
*
* Revision 1.11  1999/07/19 15:50:18  vasilche
* Added interface to old ASN.1 routines.
* Added naming of key/value in STL map.
*
* Revision 1.10  1999/07/14 18:58:04  vasilche
* Fixed ASN.1 types/field naming.
*
* Revision 1.9  1999/07/09 16:32:54  vasilche
* Added OCTET STRING write/read.
*
* Revision 1.8  1999/07/07 21:15:03  vasilche
* Cleaned processing of string types (string, char*, const char*).
*
* Revision 1.7  1999/07/02 21:31:47  vasilche
* Implemented reading from ASN.1 binary format.
*
* Revision 1.6  1999/07/01 17:55:20  vasilche
* Implemented ASN.1 binary write.
*
* Revision 1.5  1999/06/30 16:04:32  vasilche
* Added support for old ASN.1 structures.
*
* Revision 1.4  1999/06/24 14:44:41  vasilche
* Added binary ASN.1 output.
*
* Revision 1.3  1999/06/17 20:42:02  vasilche
* Fixed storing/loading of pointers.
*
* Revision 1.2  1999/06/16 20:35:24  vasilche
* Cleaned processing of blocks of data.
* Added input from ASN.1 text format.
*
* Revision 1.1  1999/06/15 16:20:04  vasilche
* Added ASN.1 object output stream.
*
* Revision 1.2  1999/06/10 21:06:40  vasilche
* Working binary output and almost working binary input.
*
* Revision 1.1  1999/06/07 19:30:18  vasilche
* More bug fixes
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>
#include <serial/objostr.hpp>

BEGIN_NCBI_SCOPE

class CObjectOStreamAsn : public CObjectOStream
{
public:
    typedef unsigned char TByte;
    typedef map<string, TIndex> TStrings;

    CObjectOStreamAsn(CNcbiOstream& out);
    virtual ~CObjectOStreamAsn(void);

    virtual void WriteTypeName(const string& name);
    virtual bool WriteEnum(const CEnumeratedTypeValues& values, long value);

protected:
    virtual void WriteBool(bool data);
    virtual void WriteChar(char data);
    virtual void WriteInt(int data);
    virtual void WriteUInt(unsigned data);
    virtual void WriteLong(long data);
    virtual void WriteULong(unsigned long data);
    virtual void WriteDouble(double data);
    virtual void WriteString(const string& str);
    virtual void WriteCString(const char* str);

#if HAVE_NCBI_C
    virtual unsigned GetAsnFlags(void);
    virtual void AsnOpen(AsnIo& asn);
    virtual void AsnWrite(AsnIo& asn, const char* data, size_t length);
#endif

    virtual void WriteMemberSuffix(const CMemberId& id);
    virtual void WriteNullPointer(void);
    virtual void WriteObjectReference(TIndex index);
    virtual void WriteOther(TConstObjectPtr object, TTypeInfo typeInfo);
    void WriteId(const string& str);

    void WriteNull(void);
    void WriteEscapedChar(char c);

    virtual void VBegin(Block& block);
    virtual void VNext(const Block& block);
    virtual void VEnd(const Block& block);
    virtual void StartMember(Member& member, const CMemberId& id);
    virtual void EndMember(const Member& member);
	virtual void Begin(const ByteBlock& block);
	virtual void WriteBytes(const ByteBlock& block,
                            const char* bytes, size_t length);
	virtual void End(const ByteBlock& block);

    void WriteNewLine(void);

private:
    CNcbiOstream& m_Output;

    int m_Ident;
};

//#include <serial/objostrasn.inl>

END_NCBI_SCOPE

#endif  /* OBJOSTRASN__HPP */
