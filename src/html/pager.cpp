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
*   Common pager box
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.13  1999/06/04 13:38:46  golikov
* Items counter shown always
*
* Revision 1.12  1999/05/11 02:53:56  vakatov
* Moved CGI API from "corelib/" to "cgi/"
*
* Revision 1.11  1999/04/16 17:45:36  vakatov
* [MSVC++] Replace the <windef.h>'s min/max macros by the hand-made templates.
*
* Revision 1.10  1999/04/15 22:11:32  vakatov
* "min/max" --> "NcbiMin/Max"
*
* Revision 1.9  1999/04/15 19:48:24  vasilche
* Fixed several warnings detected by GCC
*
* Revision 1.8  1999/04/14 19:50:28  vasilche
* Fixed coredump. One more bug with map::const_iterator
*
* Revision 1.7  1999/04/14 17:29:01  vasilche
* Added parsing of CGI parameters from IMAGE input tag like "cmd.x=1&cmd.y=2"
* As a result special parameter is added with empty name: "=cmd"
*
* Revision 1.6  1999/04/06 19:33:41  vasilche
* Added defalut page size.
*
* Revision 1.5  1999/02/17 22:03:17  vasilche
* Assed AsnMemoryRead & AsnMemoryWrite.
* Pager now may return NULL for some components if it contains only one
* page.
*
* Revision 1.4  1999/01/21 21:13:00  vasilche
* Added/used descriptions for HTML submit/select/text.
* Fixed some bugs in paging.
*
* Revision 1.3  1999/01/21 16:18:06  sandomir
* minor changes due to NStr namespace to contain string utility functions
*
* Revision 1.2  1999/01/20 21:41:36  vasilche
* Fixed bug with lost current page.
*
* Revision 1.1  1999/01/19 21:17:42  vasilche
* Added CPager class
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>
#include <cgi/ncbicgi.hpp>
#include <html/pager.hpp>

BEGIN_NCBI_SCOPE

const string CPager::KParam_PageSize = "dispmax";
const string CPager::KParam_DisplayPage = "page";
const string CPager::KParam_Page = "page ";
const string CPager::KParam_PreviousPages = "previous pages";
const string CPager::KParam_NextPages = "next pages";

string CPagerView::sm_DefaultImagesDir = "/images/";

CPager::CPager(CCgiRequest& request, int pageBlockSize, int defaultPageSize)
    : m_PageSize(GetPageSize(request, defaultPageSize)),
      m_PageBlockSize(max(1, pageBlockSize)),
      m_PageChanged(false)
{
    TCgiEntries& entries = const_cast<TCgiEntries&>(request.GetEntries());

    // look in preprocessed IMAGE values with empty string key
    TCgiEntriesI i = entries.find(NcbiEmptyString);
    if ( i != entries.end() ) {
        if ( i->second == KParam_PreviousPages ) {
            // previous pages
            // round to previous page block
            m_PageChanged = true;
            int page = GetDisplayPage(request);
            m_DisplayPage = page - page % m_PageBlockSize - 1;
        }
        else if ( i->second == KParam_NextPages ) {
            // next pages
            // round to next page block
            m_PageChanged = true;
            int page = GetDisplayPage(request);
            m_DisplayPage = page - page % m_PageBlockSize + m_PageBlockSize;
        }
        else if ( NStr::StartsWith(i->second, KParam_Page) ) {
            // look for params like: "page 2"
            string page = i->second.substr(KParam_Page.size());
            try {
                m_DisplayPage = NStr::StringToInt(page) - 1;
                m_PageChanged = true;
            } catch (exception e) {
                _TRACE( "Exception in CQSearchCommand::GetDisplayRange: "
                        << e.what() );
                // ignore exception right now
                m_PageChanged = false;
            }
        }
        else {
            m_PageChanged = false;
        }
    }
    if ( !m_PageChanged ) {
        TCgiEntriesI page = entries.find(KParam_DisplayPage);
        if ( page != entries.end() ) {
            try {
                m_DisplayPage = NStr::StringToInt(page->second);
            } catch (exception e) {
                _TRACE( "Exception in CQSearchCommand::GetDisplayRange: "
                        << e.what() );
                m_DisplayPage = 0;
            }
        }
        else {
            m_DisplayPage = 0;
        }
    }

    if ( m_DisplayPage < 0 ) {
        _TRACE( "Negative page start in CQSearchCommand::GetDisplayRange: "
                << m_DisplayPage );
        m_DisplayPage = 0;
    }

    m_PageBlockStart = m_DisplayPage - m_DisplayPage % m_PageBlockSize;
}

bool CPager::IsPagerCommand(const CCgiRequest& request)
{
    TCgiEntries& entries = const_cast<TCgiEntries&>(request.GetEntries());

    // look in preprocessed IMAGE values with empty string key
    TCgiEntriesI i = entries.find(NcbiEmptyString);
    if ( i != entries.end() ) {
        if ( i->second == KParam_PreviousPages ) {
            // previous pages
            return true;
        }
        else if ( i->second == KParam_NextPages ) {
            // next pages
            return true;
        }
        else if ( NStr::StartsWith(i->second, KParam_Page) ) {
            // look for params like: "page 2"
            string page = i->second.substr(KParam_Page.size());
            try {
                NStr::StringToInt(page);
                return true;
            } catch (exception e) {
                _TRACE( "Exception in CQSearchCommand::GetDisplayRange: "
                        << e.what() );
                // ignore exception right now
            }
        }
    }

    return false;
}

int CPager::GetDisplayPage(CCgiRequest& request)
{
    const TCgiEntries& entries = request.GetEntries();
    TCgiEntriesCI entry = entries.find(KParam_DisplayPage);

    if ( entry != entries.end() ) {
        try {
            int displayPage = NStr::StringToInt(entry->second);
            if ( displayPage >= 0 )
                return displayPage;

            _TRACE( "Negative page start in CQSearchCommand::GetDisplayRange: " << displayPage );
        } catch (exception e) {
            _TRACE( "Exception in CQSearchCommand::GetPageSize " << e.what() );
        }
    }

    // use default page start
    return 0;
}

int CPager::GetPageSize(CCgiRequest& request, int defaultPageSize)
{
    const TCgiEntries& entries = request.GetEntries();
    TCgiEntriesCI entry = entries.find(KParam_DisplayPage);

    entry = entries.find(KParam_PageSize);
    if ( entry != entries.end() ) {
        try {
            int pageSize = NStr::StringToInt(entry->second);
            if ( pageSize > 0 )
                return pageSize;
            _TRACE( "Nonpositive page size in CQSearchCommand::GetDisplayRange: " << pageSize );
        } catch (exception e) {
            _TRACE( "Exception in CQSearchCommand::GetPageSize " << e.what() );
        }
    }
    // use default page size
    return defaultPageSize;
}

void CPager::SetItemCount(int itemCount)
{
    m_ItemCount = itemCount;
    if ( m_DisplayPage * m_PageSize >= itemCount ) {
        m_DisplayPage = 0;
    }
}

pair<int, int> CPager::GetRange(void) const
{
    int firstItem = m_DisplayPage * m_PageSize;
    return pair<int, int>(firstItem, min(firstItem + m_PageSize, m_ItemCount));
}

void CPager::CreateSubNodes(void)
{
    AppendChild(new CHTML_hidden(KParam_PageSize, m_PageSize));
    AppendChild(new CHTML_hidden(KParam_DisplayPage, m_DisplayPage));
}

CNCBINode* CPager::GetPageInfo(void) const
{
    if ( m_ItemCount <= m_PageSize )
        return 0;
    int lastPage = (m_ItemCount - 1) / m_PageSize;
    return new CHTMLText(
        NStr::IntToString(m_DisplayPage + 1) +
        " page of " + NStr::IntToString(lastPage + 1));
}

CNCBINode* CPager::GetItemInfo(void) const
{
    int firstItem = m_DisplayPage * m_PageSize + 1;
    int endItem = min((m_DisplayPage + 1) * m_PageSize, m_ItemCount);
    return new CHTMLText(
        NStr::IntToString(firstItem) + "-" + NStr::IntToString(endItem) +
        " items of " + NStr::IntToString(m_ItemCount));
}

CNCBINode* CPager::GetPagerView(void) const
{
    if ( m_ItemCount <= m_PageSize )
        return 0;
    return new CPagerView(*this);
}

// pager view

CPagerView::CPagerView(const CPager& pager)
    : m_ImagesDir(sm_DefaultImagesDir), m_Pager(pager)
{
}

void CPagerView::AddImageString(CNCBINode* node, int number,
                                const string& prefix, const string& suffix)
{
    string s = NStr::IntToString(number + 1);
    string name = CPager::KParam_Page + s;

    for ( size_t i = 0; i < s.size(); ++i ) {
        node->AppendChild(new CHTML_image(name,
                    m_ImagesDir + prefix + s[i] + suffix, 0));
    }
}

void CPagerView::AddInactiveImageString(CNCBINode* node, int number,
                                  const string& prefix, const string& suffix)
{
    string s = NStr::IntToString(number + 1);

    for ( size_t i = 0; i < s.size(); ++i ) {
        node->AppendChild(new CHTML_img(
                    m_ImagesDir + prefix + s[i] + suffix));
    }
}

void CPagerView::CreateSubNodes()
{
    int column = 0;
    int pageSize = m_Pager.m_PageSize;
    int blockSize = m_Pager.m_PageBlockSize;

    int currentPage = m_Pager.m_DisplayPage;
    int itemCount = m_Pager.m_ItemCount;

    int firstBlockPage = currentPage - currentPage % blockSize;
    int lastPage = max(0, (itemCount + pageSize - 1) / pageSize - 1);
    int lastBlockPage = min(firstBlockPage + blockSize - 1, lastPage);

    if ( firstBlockPage > 0 ) {
        InsertAt(0, column++, new CHTML_image(CPager::KParam_PreviousPages,
                                              m_ImagesDir + "prev.gif", 0));
    }

    for ( int i = firstBlockPage; i <= lastBlockPage ; ++i ) {
        if ( i == currentPage ) {
            // current link
            AddImageString(Cell(0, column++), i, "black_", ".gif");
        }
        else {
            // normal link
            AddImageString(Cell(0, column++), i, "", ".gif");
        }
    }

    if ( lastPage != lastBlockPage ) {
        InsertAt(0, column++, new CHTML_image(CPager::KParam_NextPages,
                                              m_ImagesDir + "next.gif", 0));
    }
}

END_NCBI_SCOPE
