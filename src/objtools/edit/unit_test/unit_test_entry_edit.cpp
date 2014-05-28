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
* Author:  Michael Kornbluh, based on template by Pavel Ivanov, NCBI
*
* File Description:
*   Unit test functions in seq_entry_edit.cpp
*
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>

#include "unit_test_entry_edit.hpp"

#include <connect/ncbi_pipe.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbifile.hpp>
#include <corelib/ncbi_system.hpp>
#include <objects/misc/sequence_macros.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objmgr/bioseq_ci.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/feat_ci.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objtools/edit/seq_entry_edit.hpp>
#include <map>
#include <objtools/unit_test_util/unit_test_util.hpp>

// This header must be included before all Boost.Test headers if there are any
#include <corelib/test_boost.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

// key is processing stage (e.g. "input", "output_expected", etc.)
typedef std::map<string, CFile> TMapTestFiles;

// key is name of test
// value is all the files for that test
typedef std::map<string, TMapTestFiles> TMapTestNameToTestFiles;

// key is function to test (e.g. "divvy")
// value is all the tests for that function
typedef std::map<string, TMapTestNameToTestFiles> TMapFunctionToVecOfTests;

namespace {

    TMapFunctionToVecOfTests s_mapFunctionToVecOfTests;
    CRef<CScope> s_pScope;

    // this loads the files it's given
    // into s_mapFunctionToVecOfTests
    class CFileSorter {
    public:
        CFileSorter(TMapFunctionToVecOfTests * out_pMapFunctionToVecOfTests) : m_pMapFunctionToVecOfTests(out_pMapFunctionToVecOfTests) { }

        void operator()(const CDirEntry & dir_entry) {
            if( ! dir_entry.IsFile() ) {
                return;
            }

            // skip files with unknown extensions
            static const char * arrAllowedExtensions[] = {
                ".asn"
            };
            bool bHasAllowedExtension = false;
            for( size_t ii = 0;
                 ii < sizeof(arrAllowedExtensions)/sizeof(arrAllowedExtensions[0]);
                 ++ii ) 
            {
                if( NStr::EndsWith(dir_entry.GetName(), arrAllowedExtensions[ii]) ){
                    bHasAllowedExtension = true;
                }
            }
            if( ! bHasAllowedExtension ) {
                return;
            }

            CFile file(dir_entry);

            // split the name
            vector<string> tokens;
            NStr::Tokenize(file.GetName(), ".", tokens);

            if( tokens.size() != 4u ) {
                throw std::runtime_error("initialization failed trying to tokenize this file: " + file.GetName());
            }
            
            const string & sFunction = tokens[0];
            const string & sTestName = tokens[1];
            const string & sStage    = tokens[2];
            const string & sSuffix   = tokens[3];
            if( sSuffix != "asn")  {
                cout << "Skipping file with unknown suffix: " << file.GetName() << endl;
                return;
            }

            const bool bInsertSuccessful =
                (*m_pMapFunctionToVecOfTests)[sFunction][sTestName].insert(
                    TMapTestFiles::value_type(sStage, file)).second;

            // this checks for duplicate file names
            if( ! bInsertSuccessful ) {
                throw std::runtime_error("initialization failed: duplicate file name: " + file.GetName() );
            }
        }

    private:
        TMapFunctionToVecOfTests * m_pMapFunctionToVecOfTests;
    };

    // sends the contents of the given file through the C preprocessor
    // and then parses the results as a CSeq_entry.
    // throws exception on error.
    CRef<CSeq_entry> s_ReadAndPreprocessEntry( const string & sFilename )
    {
        ifstream in_file(sFilename.c_str());
        BOOST_REQUIRE(in_file.good());
        CRef<CSeq_entry> pEntry( new CSeq_entry );
        in_file >> MSerial_AsnText >> *pEntry;
        return pEntry;
    }

    bool s_AreSeqEntriesEqualAndPrintIfNot(
        const CSeq_entry & entry1,
        const CSeq_entry & entry2 )
    {
        const bool bEqual = entry1.Equals(entry2);
        if( ! bEqual ) {
            // they're not equal, so print them
            cerr << "These entries should be equal but they aren't: " << endl;
            cerr << "Entry 1: " << MSerial_AsnText << entry1 << endl;
            cerr << "Entry 2: " << MSerial_AsnText << entry2 << endl;
        }
        return bEqual;
    }
}

NCBITEST_AUTO_INIT()
{
    s_pScope.Reset( new CScope(*CObjectManager::GetInstance()) );

    static const vector<string> kEmptyStringVec;

    // initialize the map of which files belong to which test
    CFileSorter file_sorter(&s_mapFunctionToVecOfTests);

    CDir test_cases_dir("./entry_edit_test_cases");
    FindFilesInDir(test_cases_dir,
                   kEmptyStringVec, kEmptyStringVec,
                   file_sorter,
                   (fFF_Default | fFF_Recursive ) );

    // print list of tests found
    cout << "List of tests found and their associated files: " << endl;
    ITERATE( TMapFunctionToVecOfTests, func_to_vec_of_tests_it,
             s_mapFunctionToVecOfTests )
    {
        cout << "FUNC: " << func_to_vec_of_tests_it->first << endl;
        ITERATE( TMapTestNameToTestFiles, test_name_to_files_it,
                 func_to_vec_of_tests_it->second )
        {
            cout << "\tTEST NAME: " << test_name_to_files_it->first << endl;
            ITERATE( TMapTestFiles, test_file_it, test_name_to_files_it->second ) {
                cout << "\t\tSTAGE: " << test_file_it->first << " (file path: " << test_file_it->second.GetPath() << ")" << endl;
            }
        }
    }

}

BOOST_AUTO_TEST_CASE(divvy)
{
    cout << "Testing FUNCTION: DivvyUpAlignments" << endl;
    // test the function DivvyUpAlignments

    TMapTestNameToTestFiles & mapOfTests = s_mapFunctionToVecOfTests["divvy"];

    BOOST_CHECK( ! mapOfTests.empty() );

    NON_CONST_ITERATE( TMapTestNameToTestFiles, test_it, mapOfTests ) {
        const string & sTestName = (test_it->first);
        cout << "Running TEST: " << sTestName << endl;

        TMapTestFiles & test_stage_map = (test_it->second);

        BOOST_REQUIRE( test_stage_map.size() == 2u );

        // pull out the stages
        const CFile & input_file = test_stage_map["input"];
        const CFile & output_expected_file = test_stage_map["output_expected"];
        BOOST_CHECK( input_file.Exists() && output_expected_file.Exists() );

        CRef<CSeq_entry> pInputEntry = s_ReadAndPreprocessEntry( input_file.GetPath() );
        CSeq_entry_Handle input_entry_h = s_pScope->AddTopLevelSeqEntry( *pInputEntry );

        edit::TVecOfSeqEntryHandles vecOfEntries;
        // all direct children are used; we do NOT
        // put the topmost seq-entry into the objmgr
        CSeq_entry_CI direct_child_ci( input_entry_h, CSeq_entry_CI::eNonRecursive );
        for( ; direct_child_ci; ++direct_child_ci ) {
            vecOfEntries.push_back( *direct_child_ci );
        }

        // do the actual function calling
        BOOST_CHECK_NO_THROW(edit::DivvyUpAlignments(vecOfEntries));

        CRef<CSeq_entry> pExpectedOutputEntry = s_ReadAndPreprocessEntry( output_expected_file.GetPath() );

        BOOST_CHECK( s_AreSeqEntriesEqualAndPrintIfNot(
            *pInputEntry, *pExpectedOutputEntry) );

        // to avoid mem leaks, now remove what we have
        BOOST_CHECK_NO_THROW(s_pScope->RemoveTopLevelSeqEntry( input_entry_h ) );
    }
}

BOOST_AUTO_TEST_CASE(SegregateSetsByBioseqList)
{
    cout << "Testing FUNCTION: SegregateSetsByBioseqList" << endl;

    TMapTestNameToTestFiles & mapOfTests = s_mapFunctionToVecOfTests["segregate"];

    BOOST_CHECK( ! mapOfTests.empty() );

    NON_CONST_ITERATE( TMapTestNameToTestFiles, test_it, mapOfTests ) {
        const string & sTestName = (test_it->first);
        cout << "Running TEST: " << sTestName << endl;

        TMapTestFiles & test_stage_map = (test_it->second);

        BOOST_REQUIRE( test_stage_map.size() == 2u );

        // pull out the stages
        const CFile & input_entry_file = test_stage_map["input_entry"];
        const CFile & output_expected_file = test_stage_map["output_expected"];

        CRef<CSeq_entry> pInputEntry = s_ReadAndPreprocessEntry( input_entry_file.GetPath() );
        CRef<CSeq_entry> pOutputExpectedEntry = s_ReadAndPreprocessEntry( output_expected_file.GetPath() );

        CSeq_entry_Handle entry_h = s_pScope->AddTopLevelSeqEntry(*pInputEntry);
        CSeq_entry_Handle expected_entry_h = s_pScope->AddTopLevelSeqEntry(*pOutputExpectedEntry);

        // load bioseq_handles with the bioseqs we want to segregate
        CScope::TBioseqHandles bioseq_handles;
        CBioseq_CI bioseq_ci( entry_h );
        for( ; bioseq_ci; ++bioseq_ci ) {
            // see if the bioseq has the user object that marks
            // it as one of the bioseqs to segregate
            CSeqdesc_CI user_desc_ci( *bioseq_ci, CSeqdesc::e_User );
            for( ; user_desc_ci; ++user_desc_ci ) {
                if( FIELD_EQUALS( user_desc_ci->GetUser(), Class,
                                  "objtools.edit.unit_test.segregate") )
                {
                    bioseq_handles.push_back( *bioseq_ci );
                    break;
                }
            }
        }

        BOOST_CHECK_NO_THROW(
            edit::SegregateSetsByBioseqList( entry_h, bioseq_handles ));

        // check if matches expected
        BOOST_CHECK( s_AreSeqEntriesEqualAndPrintIfNot(
             *entry_h.GetCompleteSeq_entry(),
             *expected_entry_h.GetCompleteSeq_entry()) );

        BOOST_CHECK_NO_THROW( s_pScope->RemoveTopLevelSeqEntry(entry_h) );
        BOOST_CHECK_NO_THROW( s_pScope->RemoveTopLevelSeqEntry(expected_entry_h) );
    }
}


void CheckLocalId(const CBioseq& seq, const string& expected)
{
    if (!seq.IsSetDescr()) {
        BOOST_CHECK_EQUAL("No descriptors", "Expected descriptors");
        return;
    }
    int num_user_descriptors_found = 0;
    ITERATE(CBioseq::TDescr::Tdata, it, seq.GetDescr().Get()) {
        if ((*it)->IsUser()) {
            const CUser_object& usr = (*it)->GetUser();
            BOOST_CHECK_EQUAL(usr.GetObjectType(), CUser_object::eObjectType_OriginalId);
            BOOST_CHECK_EQUAL(usr.GetData()[0]->GetLabel().GetStr(), "LocalId");
            BOOST_CHECK_EQUAL(usr.GetData()[0]->GetData().GetStr(), expected);
            num_user_descriptors_found++;;
        }
    }
    BOOST_CHECK_EQUAL(num_user_descriptors_found, 1);

}


BOOST_AUTO_TEST_CASE(FixCollidingIds)
{
    CRef<CSeq_entry> entry1 = unit_test_util::BuildGoodSeq();
    unit_test_util::SetDrosophila_melanogaster(entry1);
    CRef<CSeq_entry> entry2 = unit_test_util::BuildGoodSeq();
    unit_test_util::SetSebaea_microphylla(entry2);
    CRef<CSeq_entry> set_entry(new CSeq_entry());
    set_entry->SetSet().SetClass(CBioseq_set::eClass_phy_set);
    set_entry->SetSet().SetSeq_set().push_back(entry1);
    set_entry->SetSet().SetSeq_set().push_back(entry2);
    edit::AddLocalIdUserObjects(*set_entry);
    CheckLocalId(entry1->GetSeq(), "good");
    CheckLocalId(entry2->GetSeq(), "good");

    set_entry->ReassignConflictingIds();
    BOOST_CHECK_EQUAL(entry1->GetSeq().GetId().front()->GetLocal().GetStr(), "good");
    BOOST_CHECK_EQUAL(entry2->GetSeq().GetId().front()->GetLocal().GetStr(), "good_1");
    BOOST_CHECK_EQUAL(true, edit::HasRepairedIDs(*set_entry));
    CRef<CSeq_entry> entry3 = unit_test_util::BuildGoodSeq();
    set_entry->SetSet().SetSeq_set().push_back(entry3);
    edit::AddLocalIdUserObjects(*set_entry);
    CheckLocalId(entry1->GetSeq(), "good");
    CheckLocalId(entry2->GetSeq(), "good");
    CheckLocalId(entry3->GetSeq(), "good");
    set_entry->ReassignConflictingIds();
    BOOST_CHECK_EQUAL(entry1->GetSeq().GetId().front()->GetLocal().GetStr(), "good");
    BOOST_CHECK_EQUAL(entry2->GetSeq().GetId().front()->GetLocal().GetStr(), "good_1");
    BOOST_CHECK_EQUAL(entry3->GetSeq().GetId().front()->GetLocal().GetStr(), "good_2");
    BOOST_CHECK_EQUAL(true, edit::HasRepairedIDs(*set_entry));

    CRef<CSeq_entry> good_set = unit_test_util::BuildGoodEcoSet();
    BOOST_CHECK_EQUAL(false, edit::HasRepairedIDs(*good_set));
    edit::AddLocalIdUserObjects(*good_set);
    BOOST_CHECK_EQUAL(false, edit::HasRepairedIDs(*good_set));
    good_set->ReassignConflictingIds();
    BOOST_CHECK_EQUAL(false, edit::HasRepairedIDs(*good_set));

}


void s_CheckSeg(const CDelta_seq& ds, bool expect_gap, size_t expect_length)
{
    BOOST_CHECK_EQUAL(ds.IsLiteral(), true);
    BOOST_CHECK_EQUAL(ds.GetLiteral().GetSeq_data().IsGap(), expect_gap);
    BOOST_CHECK_EQUAL(ds.GetLiteral().GetLength(), expect_length);
}


CRef<CSeq_entry> MakeEntryForDeltaConversion(vector<string> segs)
{
    CRef<CSeq_entry> entry = unit_test_util::BuildGoodSeq();
    CSeq_inst& inst = entry->SetSeq().SetInst();
    string seq_str = "";
    ITERATE(vector<string>, it, segs) {
        seq_str += *it;
    }
    size_t orig_len = seq_str.length();
    inst.SetSeq_data().SetIupacna().Set(seq_str);
    inst.SetLength(orig_len);

    CRef<CSeq_id> id(new CSeq_id());
    id->Assign(*(entry->GetSeq().GetId().front()));
   
    CRef<CSeq_annot> annot(new CSeq_annot());
    entry->SetSeq().SetAnnot().push_back(annot);
    // first feature covers entire sequence
    CRef<CSeq_feat> f1(new CSeq_feat());
    f1->SetData().SetImp().SetKey("misc_feature");
    f1->SetLocation().SetInt().SetId().Assign(*id);
    f1->SetLocation().SetInt().SetFrom(0);
    f1->SetLocation().SetInt().SetTo(entry->GetSeq().GetLength() - 1);
    annot->SetData().SetFtable().push_back(f1);

    // second feature is coding region, code break is after gap of unknown length
    CRef<CSeq_feat> f2 (new CSeq_feat());
    CRef<CCode_break> brk(new CCode_break());
    brk->SetLoc().SetInt().SetId().Assign(*id);
    brk->SetLoc().SetInt().SetFrom(54);
    brk->SetLoc().SetInt().SetTo(56);
    f2->SetData().SetCdregion().SetCode_break().push_back(brk);
    f2->SetLocation().SetInt().SetId().Assign(*id);
    f2->SetLocation().SetInt().SetFrom(0);
    f2->SetLocation().SetInt().SetTo(entry->GetSeq().GetLength() - 1);
    annot->SetData().SetFtable().push_back(f2);

    // third feature is tRNA, code break is before gap of unknown length, mixed location
    CRef<CSeq_feat> f3 (new CSeq_feat());
    CSeq_interval& interval = f3->SetData().SetRna().SetExt().SetTRNA().SetAnticodon().SetInt();
    interval.SetFrom(6);
    interval.SetTo(8);
    interval.SetId().Assign(*id);
    f3->SetData().SetRna().SetType(CRNA_ref::eType_tRNA);

    //make mix location that does not include gaps
    size_t pos = 0;
    ITERATE(vector<string>, it, segs) {
        if (NStr::Find(*it, "N") == string::npos) {
            CRef<CSeq_loc> l1(new CSeq_loc());
            l1->SetInt().SetFrom(pos);
            l1->SetInt().SetTo(pos +  it->length() - 1);
            l1->SetInt().SetId().Assign(*id);
            f3->SetLocation().SetMix().Set().push_back(l1);
        }
        pos += it->length();
    }
    return entry;
}


void s_IntervalsMatchGaps(const CSeq_loc& loc, const CSeq_inst& inst)
{
    BOOST_CHECK_EQUAL(loc.Which(), CSeq_loc::e_Mix);
    BOOST_CHECK_EQUAL(inst.GetRepr(), CSeq_inst::eRepr_delta);
    if (loc.Which() != CSeq_loc::e_Mix || inst.GetRepr() != CSeq_inst::eRepr_delta) {
        return;
    }

    CSeq_loc_CI ci(loc);
    CSeq_ext::TDelta::Tdata::const_iterator ds_it = inst.GetExt().GetDelta().Get().begin();
    size_t pos = 0;
    while ((ds_it != inst.GetExt().GetDelta().Get().end()) && ci) {
        const CSeq_literal& lit = (*ds_it)->GetLiteral();
        if (lit.IsSetSeq_data() && !lit.GetSeq_data().IsGap()) {
            BOOST_CHECK_EQUAL(ci.GetRange().GetFrom(), pos);
            BOOST_CHECK_EQUAL(ci.GetRange().GetTo(), pos + lit.GetLength() - 1);
            ci++;
        }
        pos += lit.GetLength();
        ds_it++;
    }
}


BOOST_AUTO_TEST_CASE(Test_ConvertRawToDeltaByNs) 
{
    vector<string> segs;
    segs.push_back("AAAAAAAAAAAAAAAAAAAAAAAA");
    segs.push_back("NNNNNNNNNNNNNNN");
    segs.push_back("TTTTTTTT");
    segs.push_back("NNNNN");
    segs.push_back("TTTTTTTTTT");

    vector<size_t> lens;
    vector<bool> is_gap;
    ITERATE(vector<string>, it, segs) {
        if (NStr::Find(*it, "N") != string::npos) {
            is_gap.push_back(true);
            if ((*it).length() == 5) {
                lens.push_back(100);
            } else {
                lens.push_back((*it).length());
            }
        } else {
            is_gap.push_back(false);
            lens.push_back((*it).length());
        }        
    }

    CRef<CSeq_entry> entry = MakeEntryForDeltaConversion (segs);
    CSeq_inst& inst = entry->SetSeq().SetInst();
    size_t orig_len = inst.GetLength();

    // This should convert the first run of Ns (15) to a gap of known length
    // and the second run of Ns (5) to a gap of unknown length
    edit::ConvertRawToDeltaByNs(inst, 5, 5, 10, -1);
    edit::NormalizeUnknownLengthGaps(inst);
    BOOST_CHECK_EQUAL(inst.GetRepr(), CSeq_inst::eRepr_delta);
    BOOST_CHECK_EQUAL(inst.GetExt().GetDelta().Get().size(), 5);
    BOOST_CHECK_EQUAL(inst.GetLength(), orig_len - 5 + 100);
    CSeq_ext::TDelta::Tdata::const_iterator ds_it = inst.GetExt().GetDelta().Get().begin();
    vector<bool>::iterator is_gap_it = is_gap.begin();
    vector<size_t>::iterator len_it = lens.begin();
    while (ds_it != inst.GetExt().GetDelta().Get().end()) {
        s_CheckSeg(**ds_it, *is_gap_it, *len_it);
        ds_it++;
        is_gap_it++;
        len_it++;
    }


    edit::SetLinkageType(inst.SetExt(), CSeq_gap::eType_short_arm);
    ITERATE(CSeq_ext::TDelta::Tdata, it, inst.GetExt().GetDelta().Get()) {
        if ((*it)->GetLiteral().GetSeq_data().IsGap()) {
            BOOST_CHECK_EQUAL((*it)->GetLiteral().GetSeq_data().GetGap().GetType(), CSeq_gap::eType_short_arm);
        }
    }

    edit::SetLinkageType(inst.SetExt(), CSeq_gap::eType_scaffold);
    ITERATE(CSeq_ext::TDelta::Tdata, it, inst.GetExt().GetDelta().Get()) {
        if ((*it)->GetLiteral().GetSeq_data().IsGap()) {
            const CSeq_gap& gap = (*it)->GetLiteral().GetSeq_data().GetGap();
            BOOST_CHECK_EQUAL(gap.GetType(), CSeq_gap::eType_scaffold);
            BOOST_CHECK_EQUAL(gap.GetLinkage(), CSeq_gap::eLinkage_linked);
            BOOST_CHECK_EQUAL(gap.GetLinkage_evidence().front()->GetType(), CLinkage_evidence::eType_unspecified);
        }
    }

    edit::SetLinkageTypeLinkedRepeat(inst.SetExt(), CLinkage_evidence::eType_paired_ends);
    ITERATE(CSeq_ext::TDelta::Tdata, it, inst.GetExt().GetDelta().Get()) {
        if ((*it)->GetLiteral().GetSeq_data().IsGap()) {
            const CSeq_gap& gap = (*it)->GetLiteral().GetSeq_data().GetGap();
            BOOST_CHECK_EQUAL(gap.GetType(), CSeq_gap::eType_repeat);
            BOOST_CHECK_EQUAL(gap.GetLinkage(), CSeq_gap::eLinkage_linked);
            BOOST_CHECK_EQUAL(gap.GetLinkage_evidence().front()->GetType(), CLinkage_evidence::eType_paired_ends);
        }
    }

    edit::SetLinkageType(inst.SetExt(), CSeq_gap::eType_centromere);
    ITERATE(CSeq_ext::TDelta::Tdata, it, inst.GetExt().GetDelta().Get()) {
        if ((*it)->GetLiteral().GetSeq_data().IsGap()) {
            const CSeq_gap& gap = (*it)->GetLiteral().GetSeq_data().GetGap();
            BOOST_CHECK_EQUAL(gap.GetType(), CSeq_gap::eType_centromere);
            BOOST_CHECK_EQUAL(gap.IsSetLinkage(), false);
            BOOST_CHECK_EQUAL(gap.IsSetLinkage_evidence(), false);
        }        
    }


    entry = MakeEntryForDeltaConversion (segs);


    CRef<CObjectManager> objmgr = CObjectManager::GetInstance();
    CScope scope(*objmgr);
    scope.AddDefaults();
    CSeq_entry_Handle seh = scope.AddTopLevelSeqEntry(*entry);
    CBioseq_Handle bsh = seh.GetSeq();
    edit::ConvertRawToDeltaByNs(bsh, 5, 5, 10, -1);

    CFeat_CI fi(bsh);
    while (fi) {
        BOOST_CHECK_EQUAL(fi->GetLocation().GetInt().GetFrom(), 0);
        BOOST_CHECK_EQUAL(fi->GetLocation().GetInt().GetTo(), entry->GetSeq().GetInst().GetLength() - 1);
        if (fi->GetData().IsCdregion()) {
            const CSeq_interval& interval = fi->GetData().GetCdregion().GetCode_break().front()->GetLoc().GetInt();
            BOOST_CHECK_EQUAL(interval.GetFrom(), 149);
            BOOST_CHECK_EQUAL(interval.GetTo(), 151);
        } else if (fi->GetData().GetSubtype() == CSeqFeatData::eSubtype_tRNA) {
            s_IntervalsMatchGaps(fi->GetLocation(), entry->GetSeq().GetInst());
            const CSeq_interval& interval = fi->GetData().GetRna().GetExt().GetTRNA().GetAnticodon().GetInt();
            BOOST_CHECK_EQUAL(interval.GetFrom(), 6);
            BOOST_CHECK_EQUAL(interval.GetTo(), 8);
        }
        ++fi;
    }

}


END_SCOPE(objects)
END_NCBI_SCOPE
