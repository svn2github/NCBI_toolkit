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
* Authors:  Paul Thiessen
*
* File Description:
*      Classes to hold sets of structure data
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.47  2001/02/02 20:17:33  thiessen
* can read in CDD with multi-structure but no struct. alignments
*
* Revision 1.46  2001/01/30 20:51:19  thiessen
* minor fixes
*
* Revision 1.45  2001/01/25 20:21:18  thiessen
* fix ostrstream memory leaks
*
* Revision 1.44  2001/01/18 19:37:28  thiessen
* save structure (re)alignments to asn output
*
* Revision 1.43  2001/01/12 01:34:18  thiessen
* network Biostruc load
*
* Revision 1.42  2000/12/29 19:23:40  thiessen
* save row order
*
* Revision 1.41  2000/12/22 19:26:40  thiessen
* write cdd output files
*
* Revision 1.40  2000/12/21 23:42:16  thiessen
* load structures from cdd's
*
* Revision 1.39  2000/12/20 23:47:48  thiessen
* load CDD's
*
* Revision 1.38  2000/12/15 15:51:47  thiessen
* show/hide system installed
*
* Revision 1.37  2000/12/01 19:35:57  thiessen
* better domain assignment; basic show/hide mechanism
*
* Revision 1.36  2000/11/30 15:49:40  thiessen
* add show/hide rows; unpack sec. struc. and domain features
*
* Revision 1.35  2000/11/13 18:06:53  thiessen
* working structure re-superpositioning
*
* Revision 1.34  2000/11/12 04:03:00  thiessen
* working file save including alignment edits
*
* Revision 1.33  2000/11/11 21:15:55  thiessen
* create Seq-annot from BlockMultipleAlignment
*
* Revision 1.32  2000/11/02 16:56:03  thiessen
* working editor undo; dynamic slave transforms
*
* Revision 1.31  2000/09/20 22:22:29  thiessen
* working conservation coloring; split and center unaligned justification
*
* Revision 1.30  2000/09/15 19:24:22  thiessen
* allow repeated structures w/o different local id
*
* Revision 1.29  2000/09/11 22:57:33  thiessen
* working highlighting
*
* Revision 1.28  2000/09/11 14:06:29  thiessen
* working alignment coloring
*
* Revision 1.27  2000/09/11 01:46:16  thiessen
* working messenger for sequence<->structure window communication
*
* Revision 1.26  2000/08/30 19:48:43  thiessen
* working sequence window
*
* Revision 1.25  2000/08/29 04:34:27  thiessen
* working alignment manager, IBM
*
* Revision 1.24  2000/08/28 23:47:19  thiessen
* functional denseg and dendiag alignment parsing
*
* Revision 1.23  2000/08/28 18:52:42  thiessen
* start unpacking alignments
*
* Revision 1.22  2000/08/27 18:52:22  thiessen
* extract sequence information
*
* Revision 1.21  2000/08/21 19:31:48  thiessen
* add style consistency checking
*
* Revision 1.20  2000/08/21 17:22:38  thiessen
* add primitive highlighting for testing
*
* Revision 1.19  2000/08/17 14:24:06  thiessen
* added working StyleManager
*
* Revision 1.18  2000/08/13 02:43:01  thiessen
* added helix and strand objects
*
* Revision 1.17  2000/08/07 14:13:16  thiessen
* added animation frames
*
* Revision 1.16  2000/08/07 00:21:18  thiessen
* add display list mechanism
*
* Revision 1.15  2000/08/04 22:49:04  thiessen
* add backbone atom classification and selection feedback mechanism
*
* Revision 1.14  2000/08/03 15:12:23  thiessen
* add skeleton of style and show/hide managers
*
* Revision 1.13  2000/07/27 13:30:51  thiessen
* remove 'using namespace ...' from all headers
*
* Revision 1.12  2000/07/18 16:50:11  thiessen
* more friendly rotation center setting
*
* Revision 1.11  2000/07/18 00:06:00  thiessen
* allow arbitrary rotation center
*
* Revision 1.10  2000/07/17 22:37:18  thiessen
* fix vector_math typo; correctly set initial view
*
* Revision 1.9  2000/07/17 04:20:50  thiessen
* now does correct structure alignment transformation
*
* Revision 1.8  2000/07/16 23:19:11  thiessen
* redo of drawing system
*
* Revision 1.7  2000/07/12 02:00:15  thiessen
* add basic wxWindows GUI
*
* Revision 1.6  2000/07/11 13:45:31  thiessen
* add modules to parse chemical graph; many improvements
*
* Revision 1.5  2000/07/01 15:43:50  thiessen
* major improvements to StructureBase functionality
*
* Revision 1.4  2000/06/29 16:46:07  thiessen
* use NCBI streams correctly
*
* Revision 1.3  2000/06/29 14:35:06  thiessen
* new atom_set files
*
* Revision 1.2  2000/06/28 13:07:55  thiessen
* store alt conf ensembles
*
* Revision 1.1  2000/06/27 20:09:40  thiessen
* initial checkin
*
* ===========================================================================
*/

#include <objects/ncbimime/Biostruc_seq.hpp>
#include <objects/ncbimime/Biostruc_seqs.hpp>
#include <objects/ncbimime/Biostruc_align.hpp>
#include <objects/ncbimime/Entrez_general.hpp>
#include <objects/mmdb1/Biostruc_id.hpp>
#include <objects/mmdb1/Mmdb_id.hpp>
#include <objects/mmdb1/Biostruc_descr.hpp>
#include <objects/mmdb2/Biostruc_model.hpp>
#include <objects/mmdb2/Model_type.hpp>
#include <objects/mmdb3/Biostruc_feature_set.hpp>
#include <objects/mmdb3/Biostruc_feature.hpp>
#include <objects/mmdb3/Biostruc_feature_id.hpp>
#include <objects/mmdb3/Chem_graph_alignment.hpp>
#include <objects/mmdb3/Transform.hpp>
#include <objects/mmdb3/Move.hpp>
#include <objects/mmdb3/Trans_matrix.hpp>
#include <objects/mmdb3/Rot_matrix.hpp>
#include <objects/mmdb3/Chem_graph_pntrs.hpp>
#include <objects/mmdb3/Residue_pntrs.hpp>
#include <objects/mmdb3/Residue_interval_pntr.hpp>

#include <corelib/ncbistre.hpp>
#include <connect/ncbi_util.h>
#include <connect/ncbi_conn_stream.hpp>
#include <connect/ncbi_core_cxx.hpp>
#include <serial/serial.hpp>
#include <serial/objistrasnb.hpp>

#include "cn3d/structure_set.hpp"
#include "cn3d/coord_set.hpp"
#include "cn3d/chemical_graph.hpp"
#include "cn3d/atom_set.hpp"
#include "cn3d/opengl_renderer.hpp"
#include "cn3d/show_hide_manager.hpp"
#include "cn3d/style_manager.hpp"
#include "cn3d/sequence_set.hpp"
#include "cn3d/alignment_set.hpp"
#include "cn3d/alignment_manager.hpp"
#include "cn3d/messenger.hpp"
#include "cn3d/cn3d_colors.hpp"
#include "cn3d/asn_reader.hpp"
#include "cn3d/block_multiple_alignment.hpp"

USING_NCBI_SCOPE;
USING_SCOPE(objects);


BEGIN_SCOPE(Cn3D)

static bool VerifyMatch(const Sequence *sequence, const StructureObject *object, const Molecule *molecule)
{
    if (molecule->residues.size() == sequence->Length()) {
        if (molecule->sequence) {
            ERR_POST(Error << "VerifyMatch() - confused by multiple sequences matching object "
                << object->pdbID << " moleculeID " << molecule->id);
            return false;
        }
        TESTMSG("matched sequence gi " << sequence->gi << " with object "
            << object->pdbID << " moleculeID " << molecule->id);
        return true;
    } else {
        ERR_POST(Error << "VerifyMatch() - length mismatch between sequence gi "
            << sequence->gi << " and matching molecule");
    }
    return false;
}

void StructureSet::MatchSequencesToMolecules(void)
{
    // crossmatch sequences with molecules - at most one molecule per sequence.
    // Match algorithm: for each molecule, check the sequence list for a matching
    // sequence that hasn't already been matched to a previous molecule.
    if (sequenceSet && objects.size() > 0) {

		SequenceSet::SequenceList::const_iterator s, se = sequenceSet->sequences.end();
        ObjectList::iterator o, oe = objects.end();
        int nMolecules, nSequenceMatches;

        for (o=objects.begin(); o!=oe; o++) {
            nMolecules = nSequenceMatches = 0;
            ChemicalGraph::MoleculeMap::const_iterator m, me = (*o)->graph->molecules.end();
            for (m=(*o)->graph->molecules.begin(); m!=me; m++) {
                if (!m->second->IsProtein() && !m->second->IsNucleotide()) continue;
                nMolecules++;

                for (s=sequenceSet->sequences.begin(); s!=se; s++) {
                    if ((*s)->molecule != NULL) continue; // skip already-matched sequences

                    if (SAME_SEQUENCE(m->second, *s)) {

                        if (VerifyMatch(*s, *o, m->second)) {
                            (const_cast<Molecule*>(m->second))->sequence = *s;
                            (const_cast<Sequence*>(*s))->molecule = m->second;
                            nSequenceMatches++;

                            // see if we can fill out gi/pdbID any further once we've verified the match
                            if (m->second->gi == Molecule::NOT_SET && (*s)->gi != Molecule::NOT_SET)
                                (const_cast<Molecule*>(m->second))->gi = (*s)->gi;
                            if (m->second->pdbID.size() == 0 && (*s)->pdbID.size() > 0) {
                                (const_cast<Molecule*>(m->second))->pdbID = (*s)->pdbID;
                                (const_cast<Molecule*>(m->second))->pdbChain = (*s)->pdbChain;
                            }
                            if ((*s)->gi == Molecule::NOT_SET && m->second->gi != Molecule::NOT_SET)
                                (const_cast<Sequence*>(*s))->gi = m->second->gi;
                            if ((*s)->pdbID.size() == 0 && m->second->pdbID.size() > 0) {
                                (const_cast<Sequence*>(*s))->pdbID = m->second->pdbID;
                                (const_cast<Sequence*>(*s))->pdbChain = m->second->pdbChain;
                            }

                            // if this is the master structure of a mutiple-structure alignment,
                            // then we know that this molecule's sequence must also be the
                            // master sequence for all sequence alignments
                            if (objects.size() > 1 && (*o)->IsMaster())
                                (const_cast<SequenceSet*>(sequenceSet))->master = *s;
                            break;
                        }
                    }
                }
            }

            // sanity check
            if (objects.size() == 1) {
                if (nSequenceMatches != nMolecules) { // must match all biopolymer molecules
                    ERR_POST(Error << "MatchSequencesToMolecules() - couldn't find sequence for "
                        "all biopolymers in " << (*o)->pdbID);
                    return;
                }
            } else { // multiple objects
                if (nSequenceMatches != 1) { // must match at exactly one biopolymer per object
                    ERR_POST(Error << "MatchSequencesToMolecules() - currently require exactly one "
                        "sequence per StructureObject (confused by " << (*o)->pdbID
                        << ", biopolymers: " << nMolecules << ", matches: " << nSequenceMatches << ' ');
                    return;
                }
            }
        }
    }
}

void StructureSet::Init(void)
{
    renderer = NULL;
    lastAtomName = OpenGLRenderer::NO_NAME;
    lastDisplayList = OpenGLRenderer::NO_LIST;
    sequenceSet = NULL;
    alignmentSet = NULL;
    alignmentManager = NULL;
    dataChanged = 0;
    colors = new Colors();
    nDomains = 0;
    parentSet = this;
    mimeData = NULL;
    cddData = NULL;
    showHideManager = new ShowHideManager();
    styleManager = new StyleManager();
    if (!showHideManager || !styleManager)
        ERR_POST(Fatal << "StructureSet::StructureSet() - out of memory");
    GlobalMessenger()->RemoveAllHighlights(false);
}

StructureSet::StructureSet(CNcbi_mime_asn1 *mime) :
    StructureBase(NULL), isMultipleStructure(mime->IsAlignstruc()), structureAlignments(NULL)
{
    Init();
    mimeData = mime;
    StructureObject *object;

    // create StructureObjects from (list of) biostruc
    if (mime->IsStrucseq()) {
        object = new StructureObject(this, mime->GetStrucseq().GetStructure(), true);
        objects.push_back(object);
        sequenceSet = new SequenceSet(this, mime->GetStrucseq().GetSequences());
        MatchSequencesToMolecules();
        alignmentManager = new AlignmentManager(sequenceSet, NULL);

    } else if (mime->IsStrucseqs()) {
        object = new StructureObject(this, mime->GetStrucseqs().GetStructure(), true);
        objects.push_back(object);
        sequenceSet = new SequenceSet(this, mime->GetStrucseqs().GetSequences());
        MatchSequencesToMolecules();
        alignmentSet = new AlignmentSet(this, mime->GetStrucseqs().GetSeqalign());
        alignmentManager = new AlignmentManager(sequenceSet, alignmentSet);
        styleManager->SetToAlignment(StyleSettings::eAligned);

    } else if (mime->IsAlignstruc()) {
        TESTMSG("Master:");
        object = new StructureObject(this, mime->GetAlignstruc().GetMaster(), true);
        objects.push_back(object);
        const CBiostruc_align::TSlaves& slaves = mime->GetAlignstruc().GetSlaves();
		CBiostruc_align::TSlaves::const_iterator i, e=slaves.end();
        for (i=slaves.begin(); i!=e; i++) {
            TESTMSG("Slave:");
            object = new StructureObject(this, i->GetObject(), false);
            if (!object->SetTransformToMaster(
                    mime->GetAlignstruc().GetAlignments(),
                    objects.front()->mmdbID))
                ERR_POST(Warning << "Can't get structure alignment for slave " << object->pdbID
                    << " with master " << objects.front()->pdbID
                    << ";\nwill likely require manual realignment");
            objects.push_back(object);
        }
        sequenceSet = new SequenceSet(this, mime->GetAlignstruc().GetSequences());
        MatchSequencesToMolecules();
        alignmentSet = new AlignmentSet(this, mime->GetAlignstruc().GetSeqalign());
        alignmentManager = new AlignmentManager(sequenceSet, alignmentSet);
        styleManager->SetToAlignment(StyleSettings::eAligned);
        structureAlignments = &(mime->GetAlignstruc().SetAlignments());

    } else if (mime->IsEntrez() && mime->GetEntrez().GetData().IsStructure()) {
        object = new StructureObject(this, mime->GetEntrez().GetData().GetStructure(), true);
        objects.push_back(object);

    } else {
        ERR_POST(Fatal << "Can't (yet) handle that Ncbi-mime-asn1 type");
    }

    VerifyFrameMap();
    showHideManager->ConstructShowHideArray(this);
}

static bool GetBiostrucByHTTP(int mmdbID, CBiostruc& biostruc, std::string& err)
{
    err.erase();
    bool okay = true;

    // set up registry field to set GET connection method for HTTP
    CNcbiRegistry* reg = new CNcbiRegistry;
//        reg->Set(DEF_CONN_REG_SECTION, REG_CONN_DEBUG_PRINTOUT, "TRUE");  // for debugging
    reg->Set(DEF_CONN_REG_SECTION, REG_CONN_DEBUG_PRINTOUT, "FALSE");
    reg->Set(DEF_CONN_REG_SECTION, REG_CONN_REQ_METHOD,     "GET");
    CORE_SetREG(REG_cxx2c(reg, true));

    try {
        // create HHTP stream from mmdbsrv URL
        CNcbiOstrstream args;
        args << "uid=" << mmdbID
            << "&form=6&db=t&save=Save&dopt=i"
            << "&Complexity=Virtual%20Bond%20Model"
            << '\0';
        std::string host = "www.ncbi.nlm.nih.gov", path = "/Structure/mmdb/mmdbsrv.cgi";
        TESTMSG("Trying to load Biostruc via " << host << path << '?' << args.str());
        CConn_HttpStream httpStream(host, path, args.str());
        delete args.str();

        // load Biostruc from this stream
        CObjectIStreamAsnBinary asnStream(httpStream);
        asnStream >> biostruc;

        // for debugging only - save stream to file
//        CNcbiOfstream ofstr("test-http-connector.val", IOS_BASE::out | IOS_BASE::binary);
//        while (!(httpStream.eof() || httpStream.fail() || httpStream.bad())) {
//            unsigned char ch;
//            httpStream >> ch;
//            ofstr << ch;
//        }
//        return false;

    } catch (exception& e) {
        err = e.what();
        okay = false;
    }

    CORE_SetREG(NULL);
    return okay;
}

StructureSet::StructureSet(CCdd *cdd, const char *dataDir) :
    StructureBase(NULL), isMultipleStructure(true)
{
    Init();
    cddData = cdd;

    // read sequences first; these contain links to MMDB id's
    sequenceSet = new SequenceSet(this, cdd->GetSequences());

    // create a list of MMDB ids to try to load
    vector < int > mmdbIDs;
    SequenceSet::SequenceList::const_iterator s, se = sequenceSet->sequences.end();
    for (s=sequenceSet->sequences.begin(); s!=se; s++)
        if ((*s)->mmdbLink != Sequence::NOT_SET) mmdbIDs.push_back((*s)->mmdbLink);

    // if more than one structure, the Biostruc-annot-set should contain structure alignments
    // for slaves->master - and thus also the identity of the master.
    static const int NOT_SET = -1;
    int masterMMDBID = NOT_SET;
    if (mmdbIDs.size() == 1) {
        masterMMDBID = mmdbIDs[0];
    } else if (mmdbIDs.size() > 1) {

        // try to get master MMDB ID from structure alignments if present
        if (cdd->IsSetFeatures()) {
            structureAlignments = &(cdd->SetFeatures());
            CBiostruc_annot_set::TId::const_iterator i, ie = cdd->GetFeatures().GetId().end();
            for (i=cdd->GetFeatures().GetId().begin(); i!=ie; i++) {
                if (i->GetObject().IsMmdb_id()) {
                    masterMMDBID = i->GetObject().GetMmdb_id().Get();
                    break;
                }
            }
        }

        // else assume the first sequence with a PDB and MMDB ID is the master
        else {
            SequenceSet::SequenceList::const_iterator s, se = sequenceSet->sequences.end();
            for (s=sequenceSet->sequences.begin(); s!=se; s++) {
                if ((*s)->pdbID.size() > 0 && (*s)->mmdbLink != Sequence::NOT_SET) {
                    ERR_POST(Error << "Warning: no structure alignments, "
                         << "so the first sequence with MMDB link ("
                         << (*s)->GetTitle() << ") is assumed to be the master structure");
                    masterMMDBID = (*s)->mmdbLink;
                    // create a new (empty) "features" area for the structure alignments
                    ClearStructureAlignments(masterMMDBID);
                    break;
                }
            }
        }

        if (masterMMDBID == NOT_SET) {
            ERR_POST(Error << "StructureSet::StructureSet() - "
                "can't determine master MMDB id from structure or sequence alignments. "
                "Structures not loaded.");
        }
    }

    // Once the master MMDB id is determined,
    // then we can load structures and slave structure alignments.
    if (masterMMDBID != NOT_SET) {
        for (int m=0; m<mmdbIDs.size(); m++) {

            // load Biostrucs from external files or network
            CBiostruc biostruc;
            bool gotBiostruc = false;

            // try local file first...
            std::string err;
            CNcbiOstrstream biostrucFile;
            biostrucFile << dataDir << mmdbIDs[m] << ".val" << '\0';
            TESTMSG("trying to read ncbi-backbone model from Biostruc in '" << biostrucFile.str() << "'");
            gotBiostruc = ReadASNFromFile(biostrucFile.str(), biostruc, true, err);
            if (!gotBiostruc) {
                ERR_POST(Warning << "Failed to read Biostruc from " << biostrucFile.str()
                    << "\nreason: " << err);
            }
            delete biostrucFile.str();

            // ... else try network
            if (!gotBiostruc) {
                gotBiostruc = GetBiostrucByHTTP(mmdbIDs[m], biostruc, err);
                if (!gotBiostruc) {
                    ERR_POST(Warning << "Failed to read Biostruc from network"
                        << "\nreason: " << err);
                }
            }

            if (gotBiostruc) {
                // create new StructureObject
                bool isMaster = (mmdbIDs[m] == masterMMDBID);
                StructureObject *object = new StructureObject(this, biostruc, isMaster, true);
                if (!isMaster) {
                    if (!object->SetTransformToMaster(cdd->GetFeatures(), masterMMDBID))
                        ERR_POST(Warning << "Can't get structure alignment for slave " << object->pdbID
                            << " with master " << objects.front()->pdbID
                            << ";\nwill likely require manual realignment");
                }
                objects.push_back(object);
            }
        }
    }

    MatchSequencesToMolecules();
    alignmentSet = new AlignmentSet(this, cdd->GetSeqannot());
    alignmentManager = new AlignmentManager(sequenceSet, alignmentSet);
    styleManager->SetToAlignment(StyleSettings::eAligned);
    VerifyFrameMap();
    showHideManager->ConstructShowHideArray(this);
}

StructureSet::~StructureSet(void)
{
    delete colors;
    delete showHideManager;
    delete styleManager;
    if (alignmentManager) delete alignmentManager;
    if (cddData) delete cddData;
    if (mimeData) delete mimeData;
}

void StructureSet::ClearStructureAlignments(int masterMMDBID)
{
    // create or empty the Biostruc-annot-set that will contain these alignments
    // in the asn data, erasing any structure alignments currently stored there
    if (structureAlignments) {
        structureAlignments->SetId().clear();
        structureAlignments->SetDescr().clear();
        structureAlignments->SetFeatures().clear();
    } else {
        structureAlignments = new CBiostruc_annot_set();
        // this should only happen if this is a Cdd, so plug in this new 'features' set
        if (cddData) {
            CRef < CBiostruc_annot_set > featRef(structureAlignments);
            cddData->SetFeatures(featRef);
        }
    }

    // set up the skeleton of the new Biostruc-annot-set
    // new Mmdb-id
    structureAlignments->SetId().resize(1);
    structureAlignments->SetId().front().Reset(new CBiostruc_id());
    CRef<CMmdb_id> mid(new CMmdb_id(masterMMDBID));
    structureAlignments->SetId().front().GetObject().SetMmdb_id(mid);
    // new Biostruc-feature-set
    CRef<CBiostruc_feature_set> featSet(new CBiostruc_feature_set());
    featSet.GetObject().SetId().Set(masterMMDBID);
    structureAlignments->SetFeatures().resize(1, featSet);

    // flag a change in data
    dataChanged |= eStructureAlignmentData;
}

void StructureSet::AddStructureAlignment(CBiostruc_feature *feature)
{
    CRef<CBiostruc_feature> featureRef(feature);
    structureAlignments->SetFeatures().front().GetObject().SetFeatures().resize(
        structureAlignments->SetFeatures().front().GetObject().GetFeatures().size() + 1, featureRef);
    dataChanged |= eStructureAlignmentData;
}

void StructureSet::ReplaceAlignmentSet(const AlignmentSet *newAlignmentSet)
{
    // update the AlignmentSet
    _RemoveChild(alignmentSet);
    delete alignmentSet;
    alignmentSet = newAlignmentSet;
    dataChanged |= eAlignmentData;

    // update the asn data
    SeqAnnotList *seqAnnots = NULL;
    if (mimeData) {
        if (mimeData->IsStrucseqs())
            seqAnnots = &(mimeData->GetStrucseqs().SetSeqalign());
        else if (mimeData->IsAlignstruc())
            seqAnnots = &(mimeData->GetAlignstruc().SetSeqalign());
    }

    else if (cddData) {
        seqAnnots = &(cddData->SetSeqannot());
    }

    if (seqAnnots) {
        seqAnnots->resize(alignmentSet->newAsnAlignmentData->size());
        SeqAnnotList::iterator o = seqAnnots->begin();
        SeqAnnotList::const_iterator n, ne = alignmentSet->newAsnAlignmentData->end();
        for (n=alignmentSet->newAsnAlignmentData->begin(); n!=ne; n++, o++)
            o->Reset(n->GetPointer());   // copy each Seq-annot CRef
    }
}

bool StructureSet::SaveASNData(const char *filename, bool doBinary)
{
    std::string err;
    bool writeOK = false;
    if (mimeData)
        writeOK = WriteASNToFile(filename, *mimeData, doBinary, err);
    else if (cddData)
        writeOK = WriteASNToFile(filename, *cddData, doBinary, err);
    if (writeOK) {
        dataChanged = 0;
    } else {
        ERR_POST(Error << "Write failed: " << err);
        return false;
    }
    return true;
}

// because the frame map (for each frame, a list of diplay lists) is complicated
// to create, this just verifies that all display lists occur exactly once
// in the map. Also, make sure that total # display lists in all frames adds up.
void StructureSet::VerifyFrameMap(void) const
{
    for (unsigned int l=OpenGLRenderer::FIRST_LIST; l<=lastDisplayList; l++) {
        bool found = false;
        for (unsigned int f=0; f<frameMap.size(); f++) {
            DisplayLists::const_iterator d, de=frameMap[f].end();
            for (d=frameMap[f].begin(); d!=de; d++) {
                if (*d == l) {
                    if (!found)
                        found = true;
                    else
                        ERR_POST(Fatal << "frameMap: repeated display list " << l);
                }
            }
        }
        if (!found)
            ERR_POST(Fatal << "display list " << l << " not in frameMap");
    }

    unsigned int nLists = 0;
    for (unsigned int f=0; f<frameMap.size(); f++) {
        DisplayLists::const_iterator d, de=frameMap[f].end();
        for (d=frameMap[f].begin(); d!=de; d++) nLists++;
    }
    if (nLists != lastDisplayList)
        ERR_POST(Fatal << "frameMap has too many display lists");
}

void StructureSet::SetCenter(const Vector *given)
{
    Vector siteSum;
    int nAtoms = 0;
    double dist;
    maxDistFromCenter = 0.0;

    // set new center if given one
    if (given) center = *given;

    // loop trough all atoms twice - once to get average center, then once to
    // find max distance from this center
    for (int i=0; i<2; i++) {
        if (given && i==0) continue; // skip center calculation if given one
        ObjectList::const_iterator o, oe=objects.end();
        for (o=objects.begin(); o!=oe; o++) {
            StructureObject::CoordSetList::const_iterator c, ce=(*o)->coordSets.end();
            for (c=(*o)->coordSets.begin(); c!=ce; c++) {
                AtomSet::AtomMap::const_iterator a, ae=(*c)->atomSet->atomMap.end();
                for (a=(*c)->atomSet->atomMap.begin(); a!=ae; a++) {
                    Vector site(a->second.front()->site);
                    if ((*o)->IsSlave() && (*o)->transformToMaster)
                        ApplyTransformation(&site, *((*o)->transformToMaster));
                    if (i==0) {
                        siteSum += site;
                        nAtoms++;
                    } else {
                        dist = (site - center).length();
                        if (dist > maxDistFromCenter)
                            maxDistFromCenter = dist;
                    }
                }
            }
        }
        if (i==0) center = siteSum / nAtoms;
    }
    TESTMSG("center: " << center << ", maxDistFromCenter " << maxDistFromCenter);
    rotationCenter = center;

    // set camera to center on the selected point
    if (renderer) renderer->ResetCamera();
}

bool StructureSet::Draw(const AtomSet *atomSet) const
{
    TESTMSG("drawing StructureSet");
    if (!styleManager->CheckStyleSettings(this)) return false;
    return true;
}

unsigned int StructureSet::CreateName(const Residue *residue, int atomID)
{
    lastAtomName++;
    nameMap[lastAtomName] = std::make_pair(residue, atomID);
    return lastAtomName;
}

bool StructureSet::GetAtomFromName(unsigned int name, const Residue **residue, int *atomID)
{
    NameMap::const_iterator i = nameMap.find(name);
    if (i == nameMap.end()) return false;
    *residue = i->second.first;
    *atomID = i->second.second;
	return true;
}

void StructureSet::SelectedAtom(unsigned int name)
{
    const Residue *residue;
    int atomID;

    if (name == OpenGLRenderer::NO_NAME || !GetAtomFromName(name, &residue, &atomID)) {
        ERR_POST(Info << "nothing selected");
        return;
    }

    // for now, if an atom is selected then use it as rotation center; use coordinate
    // from first CoordSet, default altConf
    const Molecule *molecule;
    if (!residue->GetParentOfType(&molecule)) return;
    const StructureObject *object;
    if (!molecule->GetParentOfType(&object)) return;

    // add highlight
    GlobalMessenger()->ToggleHighlightOnAnyResidue(molecule, residue->id);

    TESTMSG("rotating about " << object->pdbID
        << " molecule " << molecule->id << " residue " << residue->id << ", atom " << atomID);
    object->coordSets.front()->atomSet->SetActiveEnsemble(NULL);
    rotationCenter = object->coordSets.front()->atomSet->
        GetAtom(AtomPntr(molecule->id, residue->id, atomID)) ->site;
    if (object->IsSlave())
        ApplyTransformation(&rotationCenter, *(object->transformToMaster));
}


const int StructureObject::NO_MMDB_ID = -1;

StructureObject::StructureObject(StructureBase *parent,
    const CBiostruc& biostruc, bool master, bool doNCBIBackboneOnly) :
    StructureBase(parent), isMaster(master), mmdbID(NO_MMDB_ID), transformToMaster(NULL)
{
    // set numerical id simply based on # objects in parentSet
    id = parentSet->objects.size() + 1;

    // get MMDB id
    CBiostruc::TId::const_iterator j, je=biostruc.GetId().end();
    for (j=biostruc.GetId().begin(); j!=je; j++) {
        if (j->GetObject().IsMmdb_id()) {
            mmdbID = j->GetObject().GetMmdb_id().Get();
            break;
        }
    }
    TESTMSG("MMDB id " << mmdbID);

    // get PDB id
    if (biostruc.IsSetDescr()) {
        CBiostruc::TDescr::const_iterator k, ke=biostruc.GetDescr().end();
        for (k=biostruc.GetDescr().begin(); k!=ke; k++) {
            if (k->GetObject().IsName()) {
                pdbID = k->GetObject().GetName();
                break;
            }
        }
    }
    TESTMSG("PDB id " << pdbID);

    // get atom and feature spatial coordinates
    if (biostruc.IsSetModel()) {
        // iterate SEQUENCE OF Biostruc-model
        CBiostruc::TModel::const_iterator i, ie=biostruc.GetModel().end();
        for (i=biostruc.GetModel().begin(); i!=ie; i++) {

            // don't know how to deal with these...
            if (i->GetObject().GetType() == eModel_type_ncbi_vector ||
                i->GetObject().GetType() == eModel_type_other) continue;

            // special case, typically for loading CDD's, when we're only interested in a single model type
            if (doNCBIBackboneOnly && i->GetObject().GetType() != eModel_type_ncbi_backbone) continue;

            // otherwise, assume all models in this set are of same type
            if (i->GetObject().GetType() == eModel_type_ncbi_backbone)
                parentSet->isAlphaOnly = true;
            else
                parentSet->isAlphaOnly = false;

            // load each Biostruc-model into a CoordSet
            if (i->GetObject().IsSetModel_coordinates()) {
                CoordSet *coordSet =
                    new CoordSet(this, i->GetObject().GetModel_coordinates());
                coordSets.push_back(coordSet);
            }
        }
    }

    // get graph - must be done after atom coordinates are loaded, so we can
    // avoid storing graph nodes for atoms not present in the model
    graph = new ChemicalGraph(this, biostruc.GetChemical_graph(), biostruc.GetFeatures());
}

bool StructureObject::SetTransformToMaster(const CBiostruc_annot_set& annot, int masterMMDBID)
{
    CBiostruc_annot_set::TFeatures::const_iterator f1, f1e=annot.GetFeatures().end();
    for (f1=annot.GetFeatures().begin(); f1!=f1e; f1++) {
        CBiostruc_feature_set::TFeatures::const_iterator f2, f2e=f1->GetObject().GetFeatures().end();
        for (f2=f1->GetObject().GetFeatures().begin(); f2!=f2e; f2++) {

            // skip if already used
            if (f2->GetObject().IsSetId() &&
                    parentSet->usedFeatures.find(f2->GetObject().GetId().Get()) !=
                        parentSet->usedFeatures.end())
                continue;

            // look for alignment feature
            if (f2->GetObject().IsSetType() &&
				f2->GetObject().GetType() == CBiostruc_feature::eType_alignment &&
                f2->GetObject().IsSetLocation() &&
                f2->GetObject().GetLocation().IsAlignment()) {
                const CChem_graph_alignment& graphAlign =
					f2->GetObject().GetLocation().GetAlignment();

                // find transform alignment of this object with master
                if (graphAlign.GetDimension() == 2 &&
                    graphAlign.GetBiostruc_ids().size() == 2 &&
                    graphAlign.IsSetTransform() &&
                    graphAlign.GetTransform().size() == 1 &&
                    graphAlign.GetBiostruc_ids().front().GetObject().IsMmdb_id() &&
                    graphAlign.GetBiostruc_ids().front().GetObject().GetMmdb_id().Get() == masterMMDBID &&
                    graphAlign.GetBiostruc_ids().back().GetObject().IsMmdb_id() &&
                    graphAlign.GetBiostruc_ids().back().GetObject().GetMmdb_id().Get() == mmdbID) {

                    // mark feature as used
                    if (f2->GetObject().IsSetId())
                        parentSet->usedFeatures[f2->GetObject().GetId().Get()] = true;
                    TESTMSG("got transform for " << pdbID << "->master");

                    // unpack transform into matrix, moves in reverse order;
                    Matrix xform;
                    transformToMaster = new Matrix();
                    CTransform::TMoves::const_iterator
                        m, me=graphAlign.GetTransform().front().GetObject().GetMoves().end();
                    for (m=graphAlign.GetTransform().front().GetObject().GetMoves().begin(); m!=me; m++) {
                        Matrix xmat;
                        double scale;
                        if (m->GetObject().IsTranslate()) {
                            const CTrans_matrix& trans = m->GetObject().GetTranslate();
                            scale = 1.0 / trans.GetScale_factor();
                            SetTranslationMatrix(&xmat,
                                Vector(scale * trans.GetTran_1(),
                                       scale * trans.GetTran_2(),
                                       scale * trans.GetTran_3()));
                        } else { // rotate
                            const CRot_matrix& rot = m->GetObject().GetRotate();
                            scale = 1.0 / rot.GetScale_factor();
                            xmat.m[0]=scale*rot.GetRot_11(); xmat.m[1]=scale*rot.GetRot_21(); xmat.m[2]= scale*rot.GetRot_31(); xmat.m[3]=0;
                            xmat.m[4]=scale*rot.GetRot_12(); xmat.m[5]=scale*rot.GetRot_22(); xmat.m[6]= scale*rot.GetRot_32(); xmat.m[7]=0;
                            xmat.m[8]=scale*rot.GetRot_13(); xmat.m[9]=scale*rot.GetRot_23(); xmat.m[10]=scale*rot.GetRot_33(); xmat.m[11]=0;
                            xmat.m[12]=0;                    xmat.m[13]=0;                    xmat.m[14]=0;                     xmat.m[15]=1;
                        }
                        ComposeInto(transformToMaster, xmat, xform);
                        xform = *transformToMaster;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

void StructureObject::RealignStructure(int nCoords,
    const Vector * const *masterCoords, const Vector * const *slaveCoords,
    const double *weights, int slaveRow)
{
    Vector masterCOM, slaveCOM; // centers of mass for master, slave
    Matrix slaveRotation;       // rotation to align slave with master
    if (!transformToMaster) transformToMaster = new Matrix();

    // do the fit
    RigidBodyFit(nCoords, masterCoords, slaveCoords, weights, masterCOM, slaveCOM, slaveRotation);

    // apply the resulting transform elements from the fit to this object's transform Matrix
    Matrix single, combined;
    SetTranslationMatrix(&single, -slaveCOM);
    ComposeInto(transformToMaster, slaveRotation, single);
    combined = *transformToMaster;
    SetTranslationMatrix(&single, masterCOM);
    ComposeInto(transformToMaster, single, combined);

    const BlockMultipleAlignment *multiple = parentSet->alignmentManager->GetCurrentMultipleAlignment();

    // create a new Biostruc-feature that contains this alignment
    CBiostruc_feature *feature = new CBiostruc_feature();
    CRef<CBiostruc_feature_id> id(new CBiostruc_feature_id(mmdbID));
    feature->SetId(id);
    CNcbiOstrstream oss;
    oss << "Structure alignment of slave " << multiple->GetSequenceOfRow(slaveRow)->GetTitle()
        << " with master " << multiple->GetSequenceOfRow(0)->GetTitle()
        << ", as computed by Cn3D" << '\0';
    feature->SetName(std::string(oss.str()));
    delete oss.str();
    feature->SetType(CBiostruc_feature::eType_alignment);
    CRef<CBiostruc_feature::C_Location> location(new CBiostruc_feature::C_Location());
    feature->SetLocation(location);
    CRef<CChem_graph_alignment> graphAlignment(new CChem_graph_alignment());
    location.GetObject().SetAlignment(graphAlignment);

    // fill out the Chem-graph-alignment
    graphAlignment.GetObject().SetDimension(2);
    CRef<CMmdb_id>
        masterMID(new CMmdb_id(parentSet->objects.front()->mmdbID)),
        slaveMID(new CMmdb_id(mmdbID));
    CRef<CBiostruc_id>
        masterBID(new CBiostruc_id()),
        slaveBID(new CBiostruc_id());
    masterBID.GetObject().SetMmdb_id(masterMID);
    slaveBID.GetObject().SetMmdb_id(slaveMID);
    graphAlignment.GetObject().SetBiostruc_ids().resize(2);
    graphAlignment.GetObject().SetBiostruc_ids().front() = masterBID;
    graphAlignment.GetObject().SetBiostruc_ids().back() = slaveBID;
    graphAlignment.GetObject().SetAlignment().resize(2);

    // fill out sequence alignment intervals
    auto_ptr<BlockMultipleAlignment::UngappedAlignedBlockList> blocks(multiple->GetUngappedAlignedBlocks());
    if (blocks.get()) {
        CRef<CChem_graph_pntrs>
            masterCGPs(new CChem_graph_pntrs()),
            slaveCGPs(new CChem_graph_pntrs());
        graphAlignment.GetObject().SetAlignment().front() = masterCGPs;
        graphAlignment.GetObject().SetAlignment().back() = slaveCGPs;
        CRef<CResidue_pntrs>
            masterRPs(new CResidue_pntrs()),
            slaveRPs(new CResidue_pntrs());
        masterCGPs.GetObject().SetResidues(masterRPs);
        slaveCGPs.GetObject().SetResidues(slaveRPs);

        masterRPs.GetObject().SetInterval().resize(blocks->size());
        slaveRPs.GetObject().SetInterval().resize(blocks->size());
        BlockMultipleAlignment::UngappedAlignedBlockList::const_iterator b, be = blocks->end();
        CResidue_pntrs::TInterval::iterator
            mi = masterRPs.GetObject().SetInterval().begin(),
            si = slaveRPs.GetObject().SetInterval().begin();
        for (b=blocks->begin(); b!=be; b++, mi++, si++) {
            CResidue_interval_pntr
                *masterRIP = new CResidue_interval_pntr(),
                *slaveRIP = new CResidue_interval_pntr();
            mi->Reset(masterRIP);
            si->Reset(slaveRIP);

            masterRIP->SetMolecule_id().Set(multiple->GetSequenceOfRow(0)->molecule->id);
            slaveRIP->SetMolecule_id().Set(multiple->GetSequenceOfRow(slaveRow)->molecule->id);

            const Block::Range *range = (*b)->GetRangeOfRow(0);
            masterRIP->SetFrom().Set(range->from + 1); // +1 to convert seqLoc to residueID
            masterRIP->SetTo().Set(range->to + 1);

            range = (*b)->GetRangeOfRow(slaveRow);
            slaveRIP->SetFrom().Set(range->from + 1);
            slaveRIP->SetTo().Set(range->to + 1);
        }
    }

    // fill out structure alignment transform
    CTransform *xform = new CTransform();
    graphAlignment.GetObject().SetTransform().resize(1);
    graphAlignment.GetObject().SetTransform().front().Reset(xform);
    xform->SetId(1);
    xform->SetMoves().resize(3);
    CTransform::TMoves::iterator m = xform->SetMoves().begin();
    for (int i=0; i<3; i++, m++) {
        CMove *move = new CMove();
        m->Reset(move);
        static const int scaleFactor = 100000;
        if (i == 0) {   // translate slave so its COM is at origin
            CRef<CTrans_matrix> trans(new CTrans_matrix());
            move->SetTranslate(trans);
            trans.GetObject().SetScale_factor(scaleFactor);
            trans.GetObject().SetTran_1((int)(-(slaveCOM.x * scaleFactor)));
            trans.GetObject().SetTran_2((int)(-(slaveCOM.y * scaleFactor)));
            trans.GetObject().SetTran_3((int)(-(slaveCOM.z * scaleFactor)));
        } else if (i == 1) {
            CRef<CRot_matrix> rot(new CRot_matrix());
            move->SetRotate(rot);
            rot.GetObject().SetScale_factor(scaleFactor);
            rot.GetObject().SetRot_11((int)(slaveRotation[0] * scaleFactor));
            rot.GetObject().SetRot_12((int)(slaveRotation[4] * scaleFactor));
            rot.GetObject().SetRot_13((int)(slaveRotation[8] * scaleFactor));
            rot.GetObject().SetRot_21((int)(slaveRotation[1] * scaleFactor));
            rot.GetObject().SetRot_22((int)(slaveRotation[5] * scaleFactor));
            rot.GetObject().SetRot_23((int)(slaveRotation[9] * scaleFactor));
            rot.GetObject().SetRot_31((int)(slaveRotation[2] * scaleFactor));
            rot.GetObject().SetRot_32((int)(slaveRotation[6] * scaleFactor));
            rot.GetObject().SetRot_33((int)(slaveRotation[10] * scaleFactor));
        } else if (i == 2) {    // translate slave so its COM is at COM of master
            CRef<CTrans_matrix> trans(new CTrans_matrix());
            move->SetTranslate(trans);
            trans.GetObject().SetScale_factor(scaleFactor);
            trans.GetObject().SetTran_1((int)(masterCOM.x * scaleFactor));
            trans.GetObject().SetTran_2((int)(masterCOM.y * scaleFactor));
            trans.GetObject().SetTran_3((int)(masterCOM.z * scaleFactor));
        }
    }

    // store the new alignment in the Biostruc-annot-set
    parentSet->AddStructureAlignment(feature);
}

END_SCOPE(Cn3D)
