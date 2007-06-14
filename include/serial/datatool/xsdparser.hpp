#ifndef XSDPARSER_HPP
#define XSDPARSER_HPP

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
*   XML Schema parser
*
* ===========================================================================
*/

#include <corelib/ncbiutil.hpp>
#include <serial/datatool/dtdparser.hpp>
#include <serial/datatool/xsdlexer.hpp>

BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
// XSDParser

class XSDParser : public DTDParser
{
public:
    XSDParser( XSDLexer& lexer);
    virtual ~XSDParser(void);

protected:
    virtual void BuildDocumentTree(CDataTypeModule& module);
    void Reset(void);
    TToken GetNextToken(void);

    bool IsAttribute(const char* att) const;
    bool IsValue(const char* value) const;
    bool DefineElementType(DTDElement& node);
    bool DefineAttributeType(DTDAttribute& att);

    void ParseHeader(void);
    void ParseInclude(void);
    void ParseImport(void);

    TToken GetRawAttributeSet(void);
    bool GetAttribute(const string& att);

    void SkipContent();

    DTDElement::EOccurrence ParseMinOccurs( DTDElement::EOccurrence occNow);
    DTDElement::EOccurrence ParseMaxOccurs( DTDElement::EOccurrence occNow);

    string ParseElementContent(DTDElement* owner, int emb);
    string ParseGroup(DTDElement* owner, int emb);
    void ParseContent(DTDElement& node, bool extended=false);
    void ParseDocumentation(void);
    void ParseContainer(DTDElement& node);

    void ParseComplexType(DTDElement& node);
    void ParseSimpleContent(DTDElement& node);
    void ParseGroupRef(DTDElement& node);
    void ParseExtension(DTDElement& node);
    void ParseRestriction(DTDElement& node);
    void ParseAttribute(DTDElement& node);
    
    void ParseAny(DTDElement& node);

    string ParseAttributeContent(void);
    void ParseContent(DTDAttribute& att);
    void ParseRestriction(DTDAttribute& att);
    void ParseEnumeration(DTDAttribute& att);

    static string CreateEmbeddedName(const string& name, int emb);
    static string CreateEntityId( const string& name, DTDEntity::EType type);
    void CreateTypeDefinition(DTDEntity::EType type);
    void ParseTypeDefinition(DTDEntity& ent);
    void ProcessNamedTypes(void);

    virtual void PushEntityLexer(const string& name);
    virtual bool PopEntityLexer(void);
    virtual AbstractLexer* CreateEntityLexer(
        CNcbiIstream& in, const string& name, bool autoDelete=true);

#if defined(NCBI_DTDPARSER_TRACE)
    virtual void PrintDocumentTree(void);
#endif

protected:
    string m_Raw;
    string m_Element;
    string m_ElementPrefix;
    string m_Attribute;
    string m_AttributePrefix;
    string m_Value;
    string m_ValuePrefix;

    map<string,string> m_RawAttributes;

    map<string,string> m_PrefixToNamespace;
    map<string,string> m_NamespaceToPrefix;
    map<string,DTDAttribute> m_MapAttribute;
    string m_TargetNamespace;
    bool m_ElementFormDefault;

private:
    stack< map<string,string> > m_StackPrefixToNamespace;
    stack< map<string,string> > m_StackNamespaceToPrefix;
    stack<string> m_StackTargetNamespace;
    stack<bool> m_StackElementFormDefault;
};

END_NCBI_SCOPE

#endif // XSDPARSER_HPP
