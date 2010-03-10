#ifndef OBJISTRJSON__HPP
#define OBJISTRJSON__HPP

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
* Author: Andrei Gourianov
*
* File Description:
*   Decode data object using JSON format
*/

#include <corelib/ncbistd.hpp>
#include <serial/objistr.hpp>
//#include <stack>


/** @addtogroup ObjStreamSupport
 *
 * @{
 */


BEGIN_NCBI_SCOPE

/////////////////////////////////////////////////////////////////////////////
///
/// CObjectIStreamJson --
///
/// Decode serial data object using JSON format
class NCBI_XSERIAL_EXPORT CObjectIStreamJson : public CObjectIStream
{
public:
    CObjectIStreamJson(void);
    ~CObjectIStreamJson(void);

    /// Get current stream position as string.
    /// Useful for diagnostic and information messages.
    ///
    /// @return
    ///   string
    virtual string GetPosition(void) const;

    /// Set default encoding of 'string' objects
    /// If data encoding is different, string will be converted to
    /// this encoding
    ///
    /// @param enc
    ///   Encoding
    void SetDefaultStringEncoding(EEncoding enc);

    /// Get default encoding of 'string' objects
    ///
    /// @return
    ///   Encoding
    EEncoding GetDefaultStringEncoding(void) const;

    virtual string ReadFileHeader(void);

protected:

    virtual void EndOfRead(void);

    virtual bool ReadBool(void);
    virtual void SkipBool(void);

    virtual char ReadChar(void);
    virtual void SkipChar(void);

    virtual Int8 ReadInt8(void);
    virtual Uint8 ReadUint8(void);

    virtual void SkipSNumber(void);
    virtual void SkipUNumber(void);

    virtual double ReadDouble(void);

    virtual void SkipFNumber(void);

    virtual void ReadString(string& s,
                            EStringType type = eStringTypeVisible);
    virtual void SkipString(EStringType type = eStringTypeVisible);

    virtual void ReadNull(void);
    virtual void SkipNull(void);

    virtual void ReadAnyContentObject(CAnyContentObject& obj);
    virtual void SkipAnyContentObject(void);

    virtual void ReadBitString(CBitString& obj);
    virtual void SkipBitString(void);

    virtual void SkipByteBlock(void);

    virtual TEnumValueType ReadEnum(const CEnumeratedTypeValues& values);

    // container
    virtual void BeginContainer(const CContainerTypeInfo* containerType);
    virtual void EndContainer(void);
    virtual bool BeginContainerElement(TTypeInfo elementType);
    virtual void EndContainerElement(void);

    // class
    virtual void BeginClass(const CClassTypeInfo* classInfo);
    virtual void EndClass(void);

    virtual TMemberIndex BeginClassMember(const CClassTypeInfo* classType);
    virtual TMemberIndex BeginClassMember(const CClassTypeInfo* classType,
                                          TMemberIndex pos);
    virtual void EndClassMember(void);
    virtual void UndoClassMember(void);

    // choice
    virtual void BeginChoice(const CChoiceTypeInfo* choiceType);
    virtual void EndChoice(void);
    virtual TMemberIndex BeginChoiceVariant(const CChoiceTypeInfo* choiceType);
    virtual void EndChoiceVariant(void);

    // byte block
    virtual void BeginBytes(ByteBlock& block);
    int GetHexChar(void);
    int GetBase64Char(void);
    virtual size_t ReadBytes(ByteBlock& block, char* buffer, size_t count);
    virtual void EndBytes(const ByteBlock& block);

    // char block
    virtual void BeginChars(CharBlock& block);
    virtual size_t ReadChars(CharBlock& block, char* buffer, size_t count);
    virtual void EndChars(const CharBlock& block);

    virtual EPointerType ReadPointerType(void);
    virtual TObjectIndex ReadObjectPointer(void);
    virtual string ReadOtherPointer(void);

private:

    char GetChar(void);
    char PeekChar(void);
    char GetChar(bool skipWhiteSpace);
    char PeekChar(bool skipWhiteSpace);

    void SkipEndOfLine(char c);
    char SkipWhiteSpace(void);
    char SkipWhiteSpaceAndGetChar(void);

    bool GetChar(char c, bool skipWhiteSpace = false);
    void Expect(char c, bool skipWhiteSpace = false);

    int ReadEscapedChar(bool* encoded=0);
    char ReadEncodedChar(EStringType type = eStringTypeVisible, bool* encoded=0);
    TUnicodeSymbol ReadUtf8Char(char c);
    string x_ReadString(EStringType type = eStringTypeVisible);
    string x_ReadData(EStringType type = eStringTypeVisible);
    string ReadKey(void);
    string ReadValue(EStringType type = eStringTypeVisible);

    void StartBlock(char expect);
    void EndBlock(char expect);
    bool NextElement(void);

    TMemberIndex FindDeep(const CItemsInfo& items, const CTempString& name, bool& deep) const;

    bool m_BlockStart;
    bool m_ExpectValue;
    char m_Closing;
    EEncoding m_StringEncoding;
    string m_LastTag;
    string m_RejectedTag;
};

/* @} */

END_NCBI_SCOPE

#endif
