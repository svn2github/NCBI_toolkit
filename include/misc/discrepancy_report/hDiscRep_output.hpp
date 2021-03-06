#ifndef _DiscOutput_HPP
#define _DiscOutput_HPP

/*  $Id$
 *===========================================================================
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
 *===========================================================================
 *
 * Author:  Jie Chen
 *
 * File Description:
 *   Output headfile for discrepancy report
 *
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiargs.hpp>

// Object Manager includes
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>

#include <serial/objistr.hpp>
#include <serial/objhook.hpp>
#include <serial/serial.hpp>

#include <misc/discrepancy_report/clickable_item.hpp>
#include <misc/xmlwrapp/xmlwrapp.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(DiscRepNmSpc)

class COutputConfig
{
  public:
    enum EMessageOptions {
       e_All = 0,
       e_Fatal
    };

    bool             use_flag;
    CNcbiOstream*    output_f;
    bool             summary_report;
    bool             add_output_tag;
    bool             add_extra_output_tag;
    bool             xml;
    EMessageOptions  msg_level;
};
 
struct s_fataltag {
    string setting_name;
    const char* description;
    const char* notag_description;
};

enum EOnCallerGrp {
    eMol = 0,
    eCitSub,
    eSource,
    eFeature,
    eSuspectText
};

typedef map <int, vector <unsigned> > UInt2UInts;
class NCBI_DISCREPANCY_REPORT_EXPORT CDiscRepOutput : public CObject
{
  public:
    ~CDiscRepOutput () {};

    void Export();
    void Export(vector <CRef <CClickableText> >& item_list);
    void Export(CClickableItem& c_item, const string& setting_name);
    void Export(vector <CRef <CClickableItem> >& c_item, const string& setting_name);

    static bool IsFatal(const CClickableItem& item, bool extra, bool multiple_nt);
    
  private:
    Str2Int                    m_OnCallerToolPriorities;
    Str2Str                    m_TestDesc;
    map <string, EOnCallerGrp> m_OnCallerToolGroups;

    void x_InitTestDesc();
    void x_SortReport(UInt2UInts& prt_ord);
    void x_Clear(UInt2UInts* prt_ord = 0);
    void x_WriteDiscRepSummary(xml::node& root, UInt2UInts& m_PrtOrd);
    static bool x_NeedsTag(const string& setting_name, const string& desc, 
                            const s_fataltag* tags, const unsigned& cnt);
    void x_AddListOutputTags();
    void x_WriteDiscRepDetails(vector <CRef < CClickableItem > > disc_rep_dt, 
                               xml::node& root,
                               UInt2UInts& prt_ord, 
                               bool use_flag, 
                               bool IsSubcategory = false);
    bool x_RmTagInDescp(string& str, const string& tag);
    void x_WriteDiscRepSubcategories(
                const vector <CRef <CClickableItem> >& subcategories, 
                xml::node& node,
                unsigned ident = 1);
    bool x_SubsHaveTags(CRef <CClickableItem> c_item);
    bool x_OkToExpand(CRef < CClickableItem > c_item);
    bool x_SuppressItemListForFeatureTypeForOutputFiles(const string& setting_name);
    void x_WriteDiscRepItems(CRef <CClickableItem> c_item, 
                                 const string& prefix, xml::node& node); 
    void x_StandardWriteDiscRepItems(COutputConfig& oc, 
                                     CRef <CClickableItem> c_item, 
                                     const string& prefix, 
                                     bool list_features_if_subcat,
                                     xml::node& node);
    string x_GetDesc4GItem(string desc);
    void x_OutputRepToGbenchItem(const CClickableItem& c_item,  
                                 CClickableText& item);
    void x_InitializeOnCallerToolPriorities();
    void x_InitializeOnCallerToolGroups();
    void x_OrderResult(UInt2UInts& ord2i_citem);
    void x_GroupResult(map <EOnCallerGrp, string>& grp_idx_str);
    void x_ReorderAndGroupOnCallerResults(UInt2UInts& ord2i_citem, 
                                       map <EOnCallerGrp, string>& grp_idx_str);
    string x_GetGrpName(EOnCallerGrp e_grp);
    CRef <CClickableItem> x_CollectSameGroupToGbench(UInt2UInts& ord2i_citem, 
                                             EOnCallerGrp e_grp, 
                                             const string& grp_idxes);
    void x_SendItemToGbench(CRef <CClickableItem> citem, 
                            vector <CRef <CClickableText> >& item_list);
    static bool x_IsItemTrnaInCDS(const CClickableItem& item);
    void x_AddFatalToItem(CClickableItem& item, COutputConfig& oc, bool more_than_one_nucleotide);
};

END_SCOPE(DiscRepNmSpc)
END_NCBI_SCOPE

#endif

