#ifndef OBJTOOLS_FORMAT___TEXT_OSTREAM_HPP
#define OBJTOOLS_FORMAT___TEXT_OSTREAM_HPP

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
* Author:  Aaron Ucko, NCBI
*          Mati Shomrat
*
* File Description:
*   Output streams interface for formatted items.
*
*/
#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>
#include <serial/serialbase.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


class IFlatItem;


/////////////////////////////////////////////////////////////////////////////
//
// TextOStream

class IFlatTextOStream : public CObject
{
public:
    virtual void AddParagraph(const list<string>&  text,
                              const CSerialObject* obj = 0) = 0;

    virtual void AddLine( const string& line,
                          const CSerialObject* obj = 0) = 0;
                          
    virtual ~IFlatTextOStream(void) {}
};


END_SCOPE(objects)
END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.3  2006/01/17 18:23:38  ludwigf
* ADDED: Declarations for a new AddLine() method.
*
* Revision 1.2  2005/04/14 14:42:37  shomrat
* IFlatTextOStream inherits from CObject
*
* Revision 1.1  2003/12/17 19:54:16  shomrat
* Initial revision (adapted from flat lib)
*
*
* ===========================================================================
*/


#endif  /* OBJTOOLS_FORMAT___TEXT_OSTREAM_HPP */
