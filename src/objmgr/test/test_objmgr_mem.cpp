#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbitime.hpp>

#include <objects/objmgr/object_manager.hpp>
#include <objects/objmgr/scope.hpp>
#include <objects/objmgr/bioseq_handle.hpp>
#include <objects/objmgr/seq_vector.hpp>
#include <objects/objmgr/desc_ci.hpp>
#include <objects/objmgr/feat_ci.hpp>
#include <objects/objmgr/align_ci.hpp>
#include <objects/objmgr/gbloader.hpp>
#include <objects/objmgr/reader_id1.hpp>

#include <objects/seqset/Seq_entry.hpp>
#include <objects/seqset/Bioseq_set.hpp>
#include <objects/seq/Bioseq.hpp>
#include <objects/seq/Seq_descr.hpp>
#include <objects/seq/NCBI2na.hpp>

#include <serial/object.hpp>
#include <serial/objistr.hpp>
#include <serial/objostr.hpp>
#include <serial/objcopy.hpp>
#include <serial/objectinfo.hpp>
#include <serial/iterator.hpp>
#include <serial/objectiter.hpp>
#include <serial/serial.hpp>

#include <memory>
#include <unistd.h>

USING_NCBI_SCOPE;
using namespace objects;

class CMemTestApp : public CNcbiApplication
{
public:
    virtual void Init(void);
    virtual int  Run (void);
};


void CMemTestApp::Init(void)
{
    // Prepare command line descriptions
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->AddOptionalKey("gi", "SeqEntryID",
                             "GI id of the Seq-Entry to fetch",
                             CArgDescriptions::eInteger);
    arg_desc->AddOptionalKey("count", "Count", "Repeat count",
                             CArgDescriptions::eInteger);
    arg_desc->AddOptionalKey("file", "File", "File with Seq-entry",
                             CArgDescriptions::eInputFile);
    arg_desc->AddFlag("objmgr", "Add entries from file to object manager");
    arg_desc->AddFlag("iterate", "Run CTypeConstIterator<CSeq_feat>");

    string prog_description = "memtest";
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              prog_description, false);

    SetupArgDescriptions(arg_desc.release());
}


int CMemTestApp::Run(void)
{
    const CArgs& args = GetArgs();
    int gi = args["gi"]? args["gi"].AsInteger(): -1;
    string file = args["file"]? args["file"].AsString(): string();
    int repeat_count = args["count"]?args["count"].AsInteger():100;
    bool add_to_objmgr = args["objmgr"];
    bool run_iter = args["iterate"];

    vector< CRef<CSeq_entry> > entries;
    if ( file.size() ) {
        ifstream ifs(file.c_str());
        auto_ptr<CObjectIStream> is(CObjectIStream::Open(eSerial_AsnText,
                                                         ifs));
        const CClassTypeInfo *seqSetInfo =
            (const CClassTypeInfo*)CBioseq_set::GetTypeInfo();
        is->SkipFileHeader(seqSetInfo);
        is->BeginClass(seqSetInfo);
        while (TMemberIndex i = is->BeginClassMember(seqSetInfo)) {
            const CMemberInfo &mi = *seqSetInfo->GetMemberInfo(i);
            const string &miId = mi.GetId().GetName();
            if (miId.compare("seq-set") == 0) {
                int count = 0;
                for (CIStreamContainerIterator m(*is, mi.GetTypeInfo());
                     m; ++m) {
                    CRef<CSeq_entry> entry(new CSeq_entry);
                    {
                        NcbiCout << "Reading Seq-entry: " << &*entry << NcbiEndl;
                        m >> *entry;
                    }
                    if ( ++count <= repeat_count ) {
                        if ( add_to_objmgr ) {
                            CRef<CObjectManager> objMgr(new CObjectManager);
                            CRef<CScope> scope(new CScope(*objMgr));
                            scope->AddTopLevelSeqEntry(*entry);
                            if ( run_iter ) {
                                for ( CTypeConstIterator<CSeq_feat> it=ConstBegin(*entry);
                                      it; ++it ) {
                                }
                            }
                        }
                        else {
                            if ( run_iter ) {
                                for ( CTypeConstIterator<CSeq_feat> it=ConstBegin(*entry);
                                      it; ++it ) {
                                }
                            }
                        }
                    }

                    if ( entry->ReferencedOnlyOnce() ) {
                        NcbiCout << "Unreferenced: " << &*entry << NcbiEndl;
                    }
                    else {
                        NcbiCout << "Still referenced: " << &*entry << NcbiEndl;
                        entries.push_back(entry);
                    }
                }
            }
            else {
                is->SkipObject(mi.GetTypeInfo());
            }
        }
    }
    else if ( gi > 0 ) {
        for ( int count = 0; count < repeat_count; ++count ) {
            typedef CNCBI2na TObject;
            typedef map<const CObject*, int> TCounterMap;
            TCounterMap cnt;
            {
                CRef<CObjectManager> objMgr(new CObjectManager);
                objMgr->RegisterDataLoader(*new CGBDataLoader("ID"),
                                           CObjectManager::eDefault);
                CScope scope(*objMgr);
                scope.AddDefaults();
                CSeq_id id;
                id.SetGi(gi);
                CBioseq_Handle bh = scope.GetBioseqHandle(id);
                const CSeq_entry& entry = bh.GetTopLevelSeqEntry();
                const CBioseq& seq = entry.GetSeq();
                {
                    const CObject* obj = &entry;
                    int c = reinterpret_cast<const int*>(obj)[1];
                    cnt[obj] = c;
                    NcbiCout << "Entry at " << obj << " have counter " << c << NcbiEndl;
                }
                if ( run_iter ) {
                    for ( CTypeConstIterator<TObject> it=ConstBegin(bh.GetTopLevelSeqEntry());
                          it; ++it ) {
                        const CObject* obj = &*it;
                        int c = reinterpret_cast<const int*>(obj)[1];
                        cnt[obj] = c;
                        NcbiCout << "Object at " << obj << " have counter " << c << NcbiEndl;
                    }
                    for ( CTypeConstIterator<TObject> it=ConstBegin(bh.GetTopLevelSeqEntry());
                          it; ++it ) {
                        const CObject* obj = &*it;
                        int c = reinterpret_cast<const int*>(obj)[1];
                        if ( cnt[obj] != c ) {
                            NcbiCout << "Object at " << obj << " changed counter, was " << cnt[obj] << " now " << c << NcbiEndl;
                            cnt[obj] = c;
                        }
                    }
                    for ( CTypeConstIterator<TObject> it=ConstBegin(bh.GetTopLevelSeqEntry());
                          it; ++it ) {
                        const CObject* obj = &*it;
                        int c = reinterpret_cast<const int*>(obj)[1];
                        if ( cnt[obj] != c ) {
                            NcbiCout << "Object at " << obj << " changed counter, was " << cnt[obj] << " now " << c << NcbiEndl;
                            cnt[obj] = c;
                        }
                    }
                }
                {
                    const CObject* obj = &entry;
                    int c = reinterpret_cast<const int*>(obj)[1];
                    NcbiCout << "Entry at " << obj << " last counter " << c << NcbiEndl;
                }
            }
            ITERATE ( TCounterMap, it, cnt ) {
                const CObject* obj = it->first;
                int c = reinterpret_cast<const int*>(obj)[1];
                NcbiCout << "Object at " << obj << " last counter " << c << NcbiEndl;
            }
        }
    }
    NON_CONST_ITERATE ( vector< CRef<CSeq_entry> >, i, entries ) {
        CRef<CSeq_entry> entry = *i;
        i->Reset();
        if ( entry->ReferencedOnlyOnce() ) {
            NcbiCout << "Unreferenced: " << &*entry << NcbiEndl;
        }
        else {
            NcbiCout << "Still referenced: " << &*entry << NcbiEndl;
        }
    }
    return 0;
}

int main(int argc, const char* argv[])
{
    return CMemTestApp().AppMain(argc, argv, 0, eDS_Default, 0);
}
