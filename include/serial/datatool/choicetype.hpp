#ifndef CHOICETYPE_HPP
#define CHOICETYPE_HPP

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
*   Type description of CHOICE type
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.2  2000/04/07 19:26:07  vasilche
* Added namespace support to datatool.
* By default with argument -oR datatool will generate objects in namespace
* NCBI_NS_NCBI::objects (aka ncbi::objects).
* Datatool's classes also moved to NCBI namespace.
*
* Revision 1.1  2000/02/01 21:46:15  vasilche
* Added CGeneratedChoiceTypeInfo for generated choice classes.
* Removed CMemberInfo subclasses.
* Added support for DEFAULT/OPTIONAL members.
* Changed class generation.
* Moved datatool headers to include/internal/serial/tool.
*
* Revision 1.2  1999/12/28 18:55:57  vasilche
* Reduced size of compiled object files:
* 1. avoid inline or implicit virtual methods (especially destructors).
* 2. avoid std::string's methods usage in inline methods.
* 3. avoid string literals ("xxx") in inline methods.
*
* Revision 1.1  1999/12/03 21:42:11  vasilche
* Fixed conflict of enums in choices.
*
* ===========================================================================
*/

#include <serial/tool/blocktype.hpp>
#include <serial/choice.hpp>
#include <serial/tool/statictype.hpp>

BEGIN_NCBI_SCOPE

class CChoiceDataType : public CDataMemberContainerType {
    typedef CDataMemberContainerType CParent;
public:
    void FixTypeTree(void) const;
    bool CheckValue(const CDataValue& value) const;

    CTypeInfo* CreateTypeInfo(void);
    AutoPtr<CTypeStrings> GenerateCode(void) const;
    AutoPtr<CTypeStrings> GetRefCType(void) const;
    AutoPtr<CTypeStrings> GetFullCType(void) const;
    const char* GetASNKeyword(void) const;
};

class CChoiceTypeInfoAnyType : public CChoiceTypeInfoBase
{
    typedef CChoiceTypeInfoBase CParent;
public:
    typedef AnyType TDataType;
    typedef struct {
        TMemberIndex index;
        TDataType data;
    } TObjectType;
    typedef CType<TObjectType> TType;

    CChoiceTypeInfoAnyType(const string& name);
    CChoiceTypeInfoAnyType(const char* name);
    ~CChoiceTypeInfoAnyType(void);

    // object getters:
    static TObjectType& Get(TObjectPtr object)
        {
            return TType::Get(object);
        }
    static const TObjectType& Get(TConstObjectPtr object)
        {
            return TType::Get(object);
        }

    size_t GetSize(void) const;
    virtual TObjectPtr Create(void) const;

protected:
    virtual TMemberIndex GetIndex(TConstObjectPtr object) const;
    virtual void SetIndex(TObjectPtr object, TMemberIndex index) const;
    virtual TObjectPtr x_GetData(TObjectPtr object, TMemberIndex index) const;
};

END_NCBI_SCOPE

#endif
