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
 */

/// @file soap_body_.hpp
/// Data storage class.
///
/// This file was generated by application DATATOOL
/// using the following specifications:
/// 'soap_11.xsd'.
///
/// ATTENTION:
///   Don't edit or commit this file into CVS as this file will
///   be overridden (by DATATOOL) without warning!

#ifndef SOAP_BODY_BASE_HPP
#define SOAP_BODY_BASE_HPP

// standard includes
#include <serial/serialbase.hpp>

// generated includes
#include <list>


BEGIN_NCBI_SCOPE
// generated classes

/////////////////////////////////////////////////////////////////////////////
class CSoapBody_Base : public ncbi::CSerialObject
{
    typedef ncbi::CSerialObject Tparent;
public:
    // constructor
    CSoapBody_Base(void);
    // destructor
    virtual ~CSoapBody_Base(void);

    // type info
    DECLARE_INTERNAL_TYPE_INFO();

    // types
    typedef std::list< ncbi::CRef< ncbi::CAnyContentObject > > TAnyContent;

    // getters
    // setters

    /// optional
    /// typedef std::list< ncbi::CRef< ncbi::CAnyContentObject > > TAnyContent
    bool IsSetAnyContent(void) const;
    bool CanGetAnyContent(void) const;
    void ResetAnyContent(void);
    const TAnyContent& GetAnyContent(void) const;
    TAnyContent& SetAnyContent(void);

    /// Reset the whole object
    virtual void Reset(void);


private:
    // Prohibit copy constructor and assignment operator
    CSoapBody_Base(const CSoapBody_Base&);
    CSoapBody_Base& operator=(const CSoapBody_Base&);

    // data
    Uint4 m_set_State[1];
    TAnyContent m_AnyContent;
};






///////////////////////////////////////////////////////////
///////////////////// inline methods //////////////////////
///////////////////////////////////////////////////////////
inline
bool CSoapBody_Base::IsSetAnyContent(void) const
{
    return ((m_set_State[0] & 0x3) != 0);
}

inline
bool CSoapBody_Base::CanGetAnyContent(void) const
{
    return true;
}

inline
const CSoapBody_Base::TAnyContent& CSoapBody_Base::GetAnyContent(void) const
{
    return m_AnyContent;
}

inline
CSoapBody_Base::TAnyContent& CSoapBody_Base::SetAnyContent(void)
{
    m_set_State[0] |= 0x1;
    return m_AnyContent;
}

///////////////////////////////////////////////////////////
////////////////// end of inline methods //////////////////
///////////////////////////////////////////////////////////



END_NCBI_SCOPE



#endif // SOAP_BODY_BASE_HPP
