#ifndef OBJTOOLS_FORMAT_ITEMS___DBSOURCE_ITEM__HPP
#define OBJTOOLS_FORMAT_ITEMS___DBSOURCE_ITEM__HPP

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
* Author: Mati Shomrat
*
* File Description:
*   
*/
#include <corelib/ncbistd.hpp>

#include <objtools/format/items/item_base.hpp>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


class CFFContext;
class IFormatter;


///////////////////////////////////////////////////////////////////////////
//
// DBSOURCE

class CDBSourceItem : public CFlatItem
{
public:
    typedef list<string> TDBSource;

    CDBSourceItem(CFFContext& ctx);
    void Format(IFormatter& formatter, IFlatTextOStream& text_os) const;
    
    const TDBSource& GetDBSource(void) const { return m_DBSource; }

private:
    void x_GatherInfo(CFFContext& ctx);
    void x_AddPIRBlock(CFFContext& ctx);
    void x_AddSPBlock(CFFContext& ctx);
    void x_AddPRFBlock(CFFContext& ctx);
    void x_AddPDBBlock(CFFContext& ctx);
    string x_FormatDBSourceID(const CSeq_id& id);

    // data
    TDBSource m_DBSource;
};


END_SCOPE(objects)
END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2003/12/17 19:46:04  shomrat
* Initial revision (adapted from flat lib)
*
*
* ===========================================================================
*/


#endif  /* OBJTOOLS_FORMAT_ITEMS___DBSOURCE_ITEM__HPP */
