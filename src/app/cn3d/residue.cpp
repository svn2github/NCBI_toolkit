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
*      Classes to hold residues
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.8  2000/08/04 22:49:03  thiessen
* add backbone atom classification and selection feedback mechanism
*
* Revision 1.7  2000/08/03 15:12:23  thiessen
* add skeleton of style and show/hide managers
*
* Revision 1.6  2000/07/27 13:30:51  thiessen
* remove 'using namespace ...' from all headers
*
* Revision 1.5  2000/07/17 22:37:18  thiessen
* fix vector_math typo; correctly set initial view
*
* Revision 1.4  2000/07/17 11:59:16  thiessen
* fix nucleotide virtual bonds
*
* Revision 1.3  2000/07/17 04:20:50  thiessen
* now does correct structure alignment transformation
*
* Revision 1.2  2000/07/16 23:19:11  thiessen
* redo of drawing system
*
* Revision 1.1  2000/07/11 13:45:31  thiessen
* add modules to parse chemical graph; many improvements
*
* ===========================================================================
*/

#include <objects/mmdb1/Residue_graph_pntr.hpp>
#include <objects/mmdb1/Biost_resid_graph_set_pntr.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/mmdb1/Biomol_descr.hpp>
#include <objects/mmdb1/Intra_residue_bond.hpp>
#include <objects/mmdb1/Atom.hpp>

#include "cn3d/residue.hpp"
#include "cn3d/bond.hpp"
#include "cn3d/structure_set.hpp"
#include "cn3d/coord_set.hpp"
#include "cn3d/atom_set.hpp"
#include "cn3d/molecule.hpp"
#include "cn3d/periodic_table.hpp"
#include "cn3d/opengl_renderer.hpp"
#include "cn3d/show_hide_manager.hpp"

USING_NCBI_SCOPE;
USING_SCOPE(objects);

BEGIN_SCOPE(Cn3D)

const char Residue::NO_CODE = '?';
const int Residue::NO_ALPHA_ID = -1;

static Residue::eAtomClassification ClassifyAtom(const Residue *residue, const CAtom& atom)
{
    if (!atom.IsSetIupac_code()) return Residue::eUnknownAtom;
    std::string code = atom.GetIupac_code().front();
    CAtom::EElement element = atom.GetElement();

    if (residue->IsAminoAcid()) {

        // amino acid partial backbone
        if (
            (element==CAtom::eElement_c && (code==" C  " || code==" CA ")) ||
            (element==CAtom::eElement_n && code==" N  ")
           ) 
            return Residue::ePartialBackboneAtom;

        // amino acid complete backbone (all backbone that's not part of "partial")
        // including both GLY alpha hydrogens and terminal COOH and NH3+
        else if (
            (element==CAtom::eElement_o &&
                (code==" O  " || code==" OXT")) ||
            (element==CAtom::eElement_h && 
                (code==" H  " || code==" HA " || code=="1HA " || code=="2HA " || 
                 code==" HXT" || code=="1H  " || code=="2H  " || code=="3H  "))
            )
            return Residue::eCompleteBackboneAtom;

        // anything else is side chain
        else
            return Residue::eSideChainAtom;

    } else if (residue->IsNucleotide()) {

        // nucleic acid partial backbone
        if (
            (element==CAtom::eElement_p && code==" P  ") ||
            (element==CAtom::eElement_c && (code==" C5*" || code==" C4*" || code==" C3*")) ||
            (element==CAtom::eElement_o && (code==" O5*" || code==" O3*")) ||
            (element==CAtom::eElement_h && (code==" H3T" || code==" H5T"))
           ) 
            return Residue::ePartialBackboneAtom;

        // nucleic acid complete backbone (all backbone that's not part of "partial")
        else if (
            (element==CAtom::eElement_o &&
                (code==" O1P" || code==" O2P" || code==" O4*" || code==" O2*")) ||
            (element==CAtom::eElement_c && (code==" C2*" || code==" C1*")) ||
            (element==CAtom::eElement_h && 
                (code=="1H5*" || code=="2H5*" || code==" H4*" || code==" H3*" || 
                 code==" H2*" || code==" H1*" || code==" H1P" || code==" H2P" || 
                 code==" HO2" || code=="1H2*" || code=="2H2*"))
           )
            return Residue::eCompleteBackboneAtom;

        // anything else is side chain
        else
            return Residue::eSideChainAtom;

    }

    return Residue::eUnknownAtom;
}

Residue::Residue(StructureBase *parent,
    const CResidue& residue, int moleculeID,
    const ResidueGraphList& standardDictionary,
    const ResidueGraphList& localDictionary) :
    StructureBase(parent), code(NO_CODE), alphaID(NO_ALPHA_ID), type(eOther)
{
    // get ID
    id = residue.GetId().Get();

    // get CResidue_graph*
    // standard (of correct type) or local dictionary?
    const ResidueGraphList *dictionary;
    int graphID;
    if (residue.GetResidue_graph().IsStandard() &&
        residue.GetResidue_graph().GetStandard().GetBiostruc_residue_graph_set_id().IsOther_database() &&
        residue.GetResidue_graph().GetStandard().GetBiostruc_residue_graph_set_id().GetOther_database().GetDb() == "Standard residue dictionary" &&
        residue.GetResidue_graph().GetStandard().GetBiostruc_residue_graph_set_id().GetOther_database().GetTag().IsId() &&
        residue.GetResidue_graph().GetStandard().GetBiostruc_residue_graph_set_id().GetOther_database().GetTag().GetId() == 1) {
        dictionary = &standardDictionary;
        graphID = residue.GetResidue_graph().GetStandard().GetResidue_graph_id().Get();
    } else if (residue.GetResidue_graph().IsLocal()) {
        dictionary = &localDictionary;
        graphID = residue.GetResidue_graph().GetLocal().Get();
    } else
        ERR_POST(Fatal << "confused by Molecule #?, Residue #" << id << "; can't find appropriate dictionary");

    // look up appropriate Residue_graph
    CResidue_graph *residueGraph = NULL;
    ResidueGraphList::const_iterator i, ie=dictionary->end();
    for (i=dictionary->begin(); i!=ie; i++) {
        if (i->GetObject().GetId().Get() == graphID) {
            residueGraph = i->GetPointer();
            break;
        }
    }
    if (!residueGraph) 
        ERR_POST(Fatal << "confused by Molecule #?, Residue #" << id 
            << "; can't find Residue-graph ID #" << graphID);

    // get iupac-code if present - assume it's the first character of the first VisibleString
    if (residueGraph->IsSetIupac_code())
        code = residueGraph->GetIupac_code().front()[0];

    // get name if present
    if (residueGraph->IsSetDescr()) {
        CResidue_graph::TDescr::const_iterator j, je = residueGraph->GetDescr().end();
        for (j=residueGraph->GetDescr().begin(); j!=je; j++) {
            if (j->GetObject().IsName()) {
                name = j->GetObject().GetName();
                break;
            }
        }
    }

    // get StructureObject* parent
    const StructureObject *object;
    if (!GetParentOfType(&object) || object->coordSets.size() == 0) {
        ERR_POST("Residue() : parent doesn't have any CoordSets");
        return;
    }

    // set residue type and name of alpha atom to store ID of
    const char *alphaCode = NULL;
    CAtom::EElement alphaElement;
    if (residueGraph->IsSetResidue_type()) {
        type = static_cast<eType>(residueGraph->GetResidue_type());
        if (IsAminoAcid()) {
            alphaCode = " CA ";
            alphaElement = CAtom::eElement_c;
        } else if (IsNucleotide()) {
            alphaCode = " P  ";
            alphaElement = CAtom::eElement_p;
        }
    }

    // get atom info
    CResidue_graph::TAtoms::const_iterator a, ae = residueGraph->GetAtoms().end();
    for (a=residueGraph->GetAtoms().begin(); a!=ae; a++) {
        const CAtom& atom = a->GetObject();
        int atomID = atom.GetId().Get();

        AtomPntr ap(moleculeID, id, atomID);

        // first see if this atom is present each CoordSet; if not, don't
        // bother storing info. This forces an intersection on CoordSets - e.g.,
        // from a multi-model NMR structure, only those atoms present in *all*
        // models will be displayed.
        StructureObject::CoordSetList::const_iterator c, ce=object->coordSets.end();
        for (c=object->coordSets.begin(); c!=ce; c++) {
            if (!((*c)->atomSet->GetAtom(ap, true, true))) break;
        }
        if (c != ce) continue;

        // store alphaID
        if (alphaCode && atom.IsSetIupac_code() && atom.GetIupac_code().front()==alphaCode &&
            atom.GetElement()==alphaElement)
            alphaID = atomID;

        AtomInfo *info = new AtomInfo;
        // get name if present
        if (atom.IsSetName()) info->name = atom.GetName();
        // get code if present - just use first one of the SEQUENCE
        if (atom.IsSetIupac_code())
            info->code = atom.GetIupac_code().front();
        // get atomic number, assuming it's the integer value of the enumerated type
        info->atomicNumber = static_cast<int>(atom.GetElement());
        // get ionizable status
        if (atom.IsSetIonizable_proton() &&
            atom.GetIonizable_proton() == CAtom::eIonizable_proton_true)
            info->isIonizableProton = true;
        else
            info->isIonizableProton = false;
        // classify atom
        info->classification = ClassifyAtom(this, atom);
        // assign (unique) name
        info->glName = parentSet->CreateName(this, atomID);

        // add info to map
        if (atomInfos.find(atom.GetId().Get()) != atomInfos.end())
            ERR_POST(Fatal << "Residue #" << id << ": confused by multiple atom IDs " << atom.GetId().Get());
        atomInfos[atomID] = info;
    }

    // get bonds
    CResidue_graph::TBonds::const_iterator b, be = residueGraph->GetBonds().end();
    for (b=residueGraph->GetBonds().begin(); b!=be; b++) {
        int order = b->GetObject().IsSetBond_order() ? 
                b->GetObject().GetBond_order() : Bond::eUnknown;
        const Bond *bond = MakeBond(this,
            moleculeID, id, b->GetObject().GetAtom_id_1().Get(),
            moleculeID, id, b->GetObject().GetAtom_id_2().Get(),
            order);
        if (bond) bonds.push_back(bond);
    }
}

Residue::~Residue(void)
{
    AtomInfoMap::iterator a, ae = atomInfos.end();
    for (a=atomInfos.begin(); a!=ae; a++) delete a->second;
}

// draw atom spheres here
bool Residue::Draw(const AtomSet *atomSet) const
{
    if (!atomSet) {
        ERR_POST(Error << "Residue::Draw(data) - NULL AtomSet*");
        return false;
    }

    // get Molecule parent
    const Molecule *molecule;
    if (!GetParentOfType(&molecule)) {
        ERR_POST(Error << "Residue::Draw(data) - can't get parent Molecule");
        return false;
    }

    // verify presense of OpenGLRenderer
    if (!parentSet->renderer) {
        ERR_POST(Warning << "Residue::Draw() - no renderer");
        return false;
    }

    // get object parent
    const StructureObject *object;
    if (!GetParentOfType(&object)) {
        ERR_POST(Error << "Residue::Draw() - can't get StructureObject parent");
        return false;
    }

    bool overlayEnsembles = parentSet->showHideManager->OverlayConfEnsembles();
    AtomStyle atomStyle;

    // iterate atoms; key is atomID
    AtomInfoMap::const_iterator a, ae = atomInfos.end();
    for (a=atomInfos.begin(); a!=ae; a++) {

        AtomPntr ap(molecule->id, id, a->first);

        // get Atom* for appropriate altConf
        const Atom *atom = atomSet->GetAtom(ap, overlayEnsembles);
        if (!atom) continue; // skip atom if no altConf

        // get Style
        if (!parentSet->styleManager->GetAtomStyle(object, ap, &atomStyle))
            return false;

        // draw the atom
        if (atomStyle.style != StyleManager::eNotDisplayed)
            parentSet->renderer->DrawAtom(atom->site, atomStyle);
    }

    return true;
}

END_SCOPE(Cn3D)

