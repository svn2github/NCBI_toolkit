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
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'genome_collection.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/genomecoll/GC_Assembly.hpp>
#include <objects/genomecoll/GC_AssemblyUnit.hpp>
#include <objects/genomecoll/GC_AssemblySet.hpp>
#include <objects/genomecoll/GC_AssemblyDesc.hpp>
#include <objects/genomecoll/GC_Replicon.hpp>
#include <objects/genomecoll/GC_Sequence.hpp>
#include <objects/genomecoll/GC_TaggedSequences.hpp>

#include <objects/seq/Seq_descr.hpp>
#include <objects/seq/Seqdesc.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/general/Object_id.hpp>
#include <objects/seqfeat/BioSource.hpp>
#include <objects/seqfeat/Org_ref.hpp>

#include <serial/serial.hpp>
#include <serial/iterator.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// constructor
CGC_Assembly::CGC_Assembly(void)
{
}


// destructor
CGC_Assembly::~CGC_Assembly(void)
{
}


int CGC_Assembly::GetReleaseId() const
{
    int release_id = 0;
    CGC_AssemblyUnit::TId ids;
    if (IsAssembly_set()) {
        ITERATE (CGC_AssemblySet::TId, id_it, GetAssembly_set().GetId()) {

            if ((*id_it)->GetDb() == "GenColl"  &&
                (*id_it)->GetTag().IsId()) {
                release_id = (*id_it)->GetTag().GetId();
                break;
            }
        }
    } else if (IsUnit()) {
        ITERATE (CGC_AssemblyUnit::TId, id_it, GetUnit().GetId()) {
            if ((*id_it)->GetDb() == "GenColl"  &&
                (*id_it)->GetTag().IsId()) {
                release_id = (*id_it)->GetTag().GetId();
                break;
            }
        }
    } else {
        NCBI_THROW(CException, eUnknown,
                   "unhandled GC-Assembly choice");
    }
    return release_id;
}


string CGC_Assembly::GetAccession() const
{
    string accession;
    CGC_AssemblyUnit::TId ids;
    if (IsAssembly_set()) {
        ITERATE (CGC_AssemblySet::TId, id_it, GetAssembly_set().GetId()) {
            if ((*id_it)->GetDb() == "GenColl"  &&
                (*id_it)->GetTag().IsStr()) {
                accession = (*id_it)->GetTag().GetStr();
                break;
            }
        }
    } else if (IsUnit()) {
        ITERATE (CGC_AssemblyUnit::TId, id_it, GetUnit().GetId()) {
            if ((*id_it)->GetDb() == "GenColl"  &&
                (*id_it)->GetTag().IsStr()) {
                accession = (*id_it)->GetTag().GetStr();
                break;
            }
        }
    } else {
        NCBI_THROW(CException, eUnknown,
                   "unhandled GC-Assembly choice");
    }
    return accession;
}


const CGC_AssemblyDesc& CGC_Assembly::GetDesc() const
{
    CConstRef<CGC_AssemblyDesc> desc;
    if (IsAssembly_set()) {
        return GetAssembly_set().GetDesc();
    } else if (IsUnit()) {
        return GetUnit().GetDesc();
    } else {
        NCBI_THROW(CException, eUnknown,
                   "assembly is neither unit not set");
    }
}


string CGC_Assembly::GetName() const
{
    CConstRef<CGC_AssemblyDesc> desc;
    if (IsAssembly_set()) {
        desc.Reset(&GetAssembly_set().GetDesc());
    } else if (IsUnit()) {
        desc.Reset(&GetUnit().GetDesc());
    }

    if (desc && desc->CanGetName()) {
        return desc->GetName();
    }

    return kEmptyStr;
}


int CGC_Assembly::GetTaxId() const
{
    CConstRef<CGC_AssemblyDesc> desc;
    if (IsAssembly_set()) {
        desc.Reset(&GetAssembly_set().GetDesc());
    } else if (IsUnit()) {
        desc.Reset(&GetUnit().GetDesc());
    }

    int tax_id = 0;
    if (desc  &&  desc->IsSetDescr()) {
        ITERATE (CGC_AssemblyDesc::TDescr::Tdata, it, desc->GetDescr().Get()) {
            if ((*it)->IsSource()) {
                tax_id = (*it)->GetSource().GetOrg().GetTaxId();
                break;
            }
        }
    }
    return tax_id;
}


bool CGC_Assembly::IsRefSeq() const
{
    CConstRef<CGC_AssemblyDesc> desc;
    if (IsAssembly_set()) {
        desc.Reset(&GetAssembly_set().GetDesc());
    } else if (IsUnit()) {
        desc.Reset(&GetUnit().GetDesc());
    }

    if (desc  &&  desc->IsSetRelease_type()) {
        return (desc->GetRelease_type() ==  CGC_AssemblyDesc::eRelease_type_refseq);
    }
    return false;
}


bool CGC_Assembly::IsGenBank() const
{
    CConstRef<CGC_AssemblyDesc> desc;
    if (IsAssembly_set()) {
        desc.Reset(&GetAssembly_set().GetDesc());
    } else if (IsUnit()) {
        desc.Reset(&GetUnit().GetDesc());
    }

    if (desc  &&  desc->IsSetRelease_type()) {
        return (desc->GetRelease_type() ==  CGC_AssemblyDesc::eRelease_type_genbank);
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////

/// Retrieve a list of all assembly units contained in this assembly
CGC_Assembly::TAssemblyUnits CGC_Assembly::GetAssemblyUnits() const
{
    TAssemblyUnits units;
    if (IsUnit()) {
        units.push_back(CConstRef<CGC_AssemblyUnit>(&GetUnit()));
    } else {
        TAssemblyUnits tmp =
            GetAssembly_set().GetPrimary_assembly().GetAssemblyUnits();
        units.insert(units.end(), tmp.begin(), tmp.end());
        if (GetAssembly_set().IsSetMore_assemblies()) {
            ITERATE (CGC_AssemblySet::TMore_assemblies, it,
                     GetAssembly_set().GetMore_assemblies()) {
                tmp = (**it).GetAssemblyUnits();
                units.insert(units.end(), tmp.begin(), tmp.end());
            }
        }
    }

    return units;
}


/////////////////////////////////////////////////////////////////////////////

CGC_Assembly::TFullAssemblies CGC_Assembly::GetFullAssemblies() const
{
    TFullAssemblies assms;

    if (IsAssembly_set()) {
        const CGC_AssemblySet& set = GetAssembly_set();
        switch (set.GetSet_type()) {
        case CGC_AssemblySet::eSet_type_assembly_set:
            /// each sub-assembly is its own entity and acts as its own root
            assms.push_back
                (CConstRef<CGC_Assembly>(&set.GetPrimary_assembly()));
            if (set.IsSetMore_assemblies()) {
                ITERATE (CGC_AssemblySet::TMore_assemblies, it,
                         set.GetMore_assemblies()) {
                    assms.push_back(*it);
                }
            }
            break;

        case CGC_AssemblySet::eSet_type_full_assembly:
            assms.push_back
                (CConstRef<CGC_Assembly>(this));
            break;

        default:
            break;
        }
    } else {
        TAssemblyUnits units = GetAssemblyUnits();
        set< CConstRef<CGC_Assembly> > tmp;
        ITERATE (TAssemblyUnits, it, units) {
            CConstRef<CGC_Assembly> assm = (*it)->GetFullAssembly();
            if (tmp.insert(assm).second) {
                assms.push_back(assm);
            }
        }
    }

    return assms;
}


/////////////////////////////////////////////////////////////////////////////

CConstRef<CGC_Sequence> CGC_Assembly::Find(const CSeq_id_Handle& id) const
{
    if (m_SequenceMap.empty()) {
        const_cast<CGC_Assembly&>(*this).CreateIndex();
    }
    TSequenceIndex::const_iterator it = m_SequenceMap.find(id);
    if (it == m_SequenceMap.end()  ||  it->second.size() == 0) {
        return CConstRef<CGC_Sequence>();
    }
    if (it->second.size() > 1) {
        NCBI_THROW(CException, eUnknown,
                   "multiple sequences found in assembly: " +
                   id.GetSeqId()->AsFastaString());
    }
    return it->second.front();
}

void CGC_Assembly::Find(const CSeq_id_Handle& id,
                        TSequenceList& sequences) const
{
    if (m_SequenceMap.empty()) {
        const_cast<CGC_Assembly&>(*this).CreateIndex();
    }
    sequences.clear();
    TSequenceIndex::const_iterator it = m_SequenceMap.find(id);
    if (it != m_SequenceMap.end()) {
        sequences = it->second;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CGC_Assembly::PreWrite() const
{
}

void CGC_Assembly::PostRead()
{
    CreateHierarchy();
}


void CGC_Assembly::CreateHierarchy()
{
    //LOG_POST(Error << "CGC_Assembly::CreateHierarchy()");

    ///
    /// generate the up-links as needed
    ///
    if (IsUnit()) {
        x_Index(*this);
    }
    else if (IsAssembly_set()) {
        CGC_AssemblySet& set = SetAssembly_set();
        switch (set.GetSet_type()) {
        case CGC_AssemblySet::eSet_type_assembly_set:
            /// each sub-assembly is its own entity and acts as its own root
            set.SetPrimary_assembly().CreateHierarchy();
            if (set.IsSetMore_assemblies()) {
                NON_CONST_ITERATE (CGC_AssemblySet::TMore_assemblies, it,
                                   set.SetMore_assemblies()) {
                    (*it)->CreateHierarchy();
                }
            }
            break;

        case CGC_AssemblySet::eSet_type_full_assembly:
            /// we are the root
            set.SetPrimary_assembly().x_Index(*this);
            if (set.IsSetMore_assemblies()) {
                NON_CONST_ITERATE (CGC_AssemblySet::TMore_assemblies, it,
                                   set.SetMore_assemblies()) {
                    (*it)->x_Index(*this);
                }
            }
            break;

        default:
            NCBI_THROW(CException, eUnknown,
                       "unknown assembly set type");
        }
    }
}


//////////////////////////////////////////////////////////////////////////////

void CGC_Assembly::CreateIndex()
{
    if (m_SequenceMap.empty()) {
        CMutexGuard LOCK(m_Mutex);
        if (m_SequenceMap.empty()) {
            CTypeConstIterator<CGC_Sequence> seq_it(*this);
            for ( ;  seq_it;  ++seq_it) {
                const CGC_Sequence& this_seq = *seq_it;
                CConstRef<CGC_Replicon> repl = this_seq.GetReplicon();

                /// bizarre pattern: the sequence is a single placed sequence
                /// with itself as the only scaffold.  if this is the case,
                /// don't index the scaffold
                if (repl  &&
                    repl->GetSequence().IsSingle()  &&
                    &repl->GetSequence().GetSingle() != &this_seq) {
                    const CGC_Sequence& repl_seq =
                        repl->GetSequence().GetSingle();
                    if (repl_seq.IsSetSequences()  &&
                        repl_seq.GetSequences().size() == 1  &&
                        repl_seq.GetSequences().front()->GetState() == CGC_TaggedSequences::eState_placed  &&
                        repl_seq.GetSequences().front()->GetSeqs().size() == 1  &&
                        repl_seq.GetSequences().front()->GetSeqs().front() == &this_seq  &&
                        repl->GetSequence().GetSingle().GetSeq_id()
                        .Match(this_seq.GetSeq_id())) {
                        continue;
                    }
                }

                m_SequenceMap[CSeq_id_Handle::GetHandle(seq_it->GetSeq_id())]
                    .push_back(CConstRef<CGC_Sequence>(&*seq_it));
            }
        }
    }
}


void CGC_Assembly::x_Index(CGC_Assembly& root)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_Assembly& root)");
    if (IsUnit()) {
        SetUnit().m_Assembly = &root;
        if (GetUnit().IsSetMols()) {
            NON_CONST_ITERATE (CGC_AssemblyUnit::TMols, it,
                               SetUnit().SetMols()) {
                x_Index(root, **it);
                x_Index(SetUnit(), **it);
            }
        }

        if (GetUnit().IsSetOther_sequences()) {
            NON_CONST_ITERATE (CGC_AssemblyUnit::TOther_sequences, it,
                               SetUnit().SetOther_sequences()) {
                NON_CONST_ITERATE (CGC_TaggedSequences::TSeqs, i,
                                   (*it)->SetSeqs()) {
                    x_Index(root, **i);
                    x_Index(SetUnit(), **i);
                    x_Index(**i, (*it)->GetState());
                }
            }
        }
    }
    else if (IsAssembly_set()) {
        CGC_AssemblySet& set = SetAssembly_set();
        set.SetPrimary_assembly().x_Index(root);
        if (set.IsSetMore_assemblies()) {
            NON_CONST_ITERATE (CGC_AssemblySet::TMore_assemblies, it,
                               set.SetMore_assemblies()) {
                (*it)->x_Index(root);
            }
        }
    }

    ///
    /// we also maintain a map of objects for fast look-ups
    ///
    m_SequenceMap.clear();
    CTypeConstIterator<CGC_Sequence> seq_it(*this);
    for ( ;  seq_it;  ++seq_it) {
        m_SequenceMap[CSeq_id_Handle::GetHandle(seq_it->GetSeq_id())]
            .push_back(CConstRef<CGC_Sequence>(&*seq_it));
    }
}


void CGC_Assembly::x_Index(CGC_Assembly& assm, CGC_Replicon& replicon)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_Assembly& assm, CGC_Replicon& replicon)");
    replicon.m_Assembly = &assm;

    if (replicon.GetSequence().IsSingle()) {
        CGC_Sequence& seq = replicon.SetSequence().SetSingle();
        x_Index(assm, seq);
    } else {
        NON_CONST_ITERATE (CGC_Replicon::TSequence::TSet, it,
                           replicon.SetSequence().SetSet()) {
            CGC_Sequence& seq = **it;
            x_Index(assm, seq);
        }
    }
}


void CGC_Assembly::x_Index(CGC_Assembly& assm, CGC_Sequence& seq)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_Assembly& assm, CGC_Sequence& seq)");
    seq.m_Assembly = &assm;
    if (seq.IsSetSequences()) {
        NON_CONST_ITERATE (CGC_Sequence::TSequences, it, seq.SetSequences()) {
            NON_CONST_ITERATE (CGC_TaggedSequences::TSeqs, i,
                               (*it)->SetSeqs()) {
                x_Index(assm, **i);
            }
        }
    }
}


void CGC_Assembly::x_Index(CGC_AssemblyUnit& unit, CGC_Replicon& replicon)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_AssemblyUnit& unit, CGC_Replicon& replicon)");
    replicon.m_AssemblyUnit = &unit;

    if (replicon.GetSequence().IsSingle()) {
        CGC_Sequence& seq = replicon.SetSequence().SetSingle();
        seq.m_ParentRel = CGC_TaggedSequences::eState_placed;

        x_Index(unit,     seq);
        x_Index(replicon, seq);
    } else {
        NON_CONST_ITERATE (CGC_Replicon::TSequence::TSet, it,
                           replicon.SetSequence().SetSet()) {
            CGC_Sequence& seq = **it;
            seq.m_ParentRel = CGC_TaggedSequences::eState_placed;

            x_Index(unit,     seq);
            x_Index(replicon, seq);
        }
    }
}


void CGC_Assembly::x_Index(CGC_AssemblyUnit& unit, CGC_Sequence& seq)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_AssemblyUnit& unit, CGC_Sequence& seq)");
    seq.m_AssemblyUnit = &unit;
    if (seq.IsSetSequences()) {
        NON_CONST_ITERATE (CGC_Sequence::TSequences, it, seq.SetSequences()) {
            NON_CONST_ITERATE (CGC_TaggedSequences::TSeqs, i,
                               (*it)->SetSeqs()) {
                x_Index(unit, **i);
                x_Index(seq, **i, (*it)->GetState());
            }
        }
    }
}


void CGC_Assembly::x_Index(CGC_Replicon& replicon, CGC_Sequence& seq)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_Replicon& replicon, CGC_Sequence& seq)");
    seq.m_Replicon = &replicon;
    if (seq.IsSetSequences()) {
        NON_CONST_ITERATE (CGC_Sequence::TSequences, it, seq.SetSequences()) {
            NON_CONST_ITERATE (CGC_TaggedSequences::TSeqs, i,
                               (*it)->SetSeqs()) {
                x_Index(replicon, **i);
            }
        }
    }
}


void CGC_Assembly::x_Index(CGC_Sequence& parent, CGC_Sequence& seq,
                           CGC_TaggedSequences::TState relation)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_Sequence& parent, CGC_Sequence& seq, CGC_TaggedSequences::TState relation)");
    seq.m_ParentSequence = &parent;
    seq.m_ParentRel = relation;
    if (seq.IsSetSequences()) {
        NON_CONST_ITERATE (CGC_Sequence::TSequences, it, seq.SetSequences()) {
            NON_CONST_ITERATE (CGC_TaggedSequences::TSeqs, i,
                               (*it)->SetSeqs()) {
                x_Index(seq, **i, (*it)->GetState());
            }
        }
    }
}

void CGC_Assembly::x_Index(CGC_Sequence& seq,
                           CGC_TaggedSequences::TState relation)
{
    //LOG_POST(Error << "CGC_Assembly::x_Index(CGC_Sequence& seq, CGC_TaggedSequences::TState relation)");
    seq.m_ParentSequence = NULL;
    seq.m_ParentRel = relation;
    if (seq.IsSetSequences()) {
        NON_CONST_ITERATE (CGC_Sequence::TSequences, it, seq.SetSequences()) {
            NON_CONST_ITERATE (CGC_TaggedSequences::TSeqs, i,
                               (*it)->SetSeqs()) {
                x_Index(seq, **i, (*it)->GetState());
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
///
/// Molecule Extraction Routines
///

static void s_Extract(const CGC_Assembly& assm,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset);

static void s_Extract(const CGC_AssemblyUnit& unit,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset);

static void s_Extract(const CGC_AssemblySet& set,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset);

static void s_Extract(const CGC_Replicon& repl,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset);

static void s_Extract(const CGC_Sequence& seq,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset);

static void s_Extract(const CGC_TaggedSequences& seq,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset);



static void s_Extract(const CGC_TaggedSequences& seq,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset)
{
    switch (subset) {
    case CGC_Assembly::eChromosome:
        NCBI_THROW(CException, eUnknown,
                   "s_Extract(): don't extract chromosomes this way");
        break;

    case CGC_Assembly::eScaffold:
        /// by definition, we are called on a replicon
        /// we therefore skip this sequence and go one level deeper
        ITERATE (CGC_TaggedSequences::TSeqs, i, seq.GetSeqs()) {
            molecules.push_back(*i);
        }
        break;

    case CGC_Assembly::eTopLevel:
        // add ourselves
        ITERATE (CGC_TaggedSequences::TSeqs, i, seq.GetSeqs()) {
            molecules.push_back(*i);
        }
        break;

    case CGC_Assembly::eComponent:
        /// by definition, we are called on a replicon
        /// we therefore skip this sequence and go one level deeper
        ITERATE (CGC_TaggedSequences::TSeqs, i, seq.GetSeqs()) {
            s_Extract(**i, molecules, subset);
        }
        break;

    default:
        break;
    }
}


static void s_Extract(const CGC_Sequence& seq,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset)
{
    switch (subset) {
    case CGC_Assembly::eChromosome:
        {{
             bool has_placed = false;
             if (seq.IsSetSequences()) {
                 ITERATE (CGC_Sequence::TSequences, i, seq.GetSequences()) {
                     if ((*i)->GetState() == CGC_TaggedSequences::eState_placed) {
                         has_placed = true;
                         break;
                     }
                 }
             }
             else {
                 // assume that the replicon is real...
                 has_placed = true;
             }
             if (has_placed) {
                 molecules.push_back(CConstRef<CGC_Sequence>(&seq));
             }
         }}
        break;

    case CGC_Assembly::eScaffold:
        /// skip this sequence and go one level deeper
        if (seq.IsSetSequences()) {

            // complex rules here, in lieu of explicit mark-up
            set<CSeq_id_Handle> syns;
            if (seq.IsSetSeq_id_synonyms()) {
                ITERATE (CGC_Sequence::TSeq_id_synonyms, i,
                         seq.GetSeq_id_synonyms()) {
                    CTypeConstIterator<CSeq_id> id_it(**i);
                    for ( ;  id_it;  ++id_it) {
                        syns.insert(CSeq_id_Handle::GetHandle(*id_it));
                    }
                }
            }

            ITERATE (CGC_Sequence::TSequences, it, seq.GetSequences()) {
                switch ((*it)->GetState()) {
                case CGC_TaggedSequences::eState_placed:
                    {{
                         bool is_syn = false;
                         ITERATE (CGC_TaggedSequences::TSeqs, i,
                                  (*it)->GetSeqs()) {
                             // this sequence likely should be a scaffold
                             // corner case: avoid adding a sequence that is
                             // explicitly a synonym of the parent we still do
                             // see some cases in which a sequence is reported
                             // as being composed of a placed sequence that is
                             // itself
                             //
                             // note that it is fine to include the self
                             // reference if it is *NOT* a synonym...
                             CSeq_id_Handle idh =
                                 CSeq_id_Handle::GetHandle((*i)->GetSeq_id());
                             if (syns.find(idh) != syns.end()) {
                                 is_syn = true;
                                 break;
                             }
                         }
                         if (is_syn) {
                             molecules.push_back(CConstRef<CGC_Sequence>(&seq));
                         }
                         else {
                             ITERATE (CGC_TaggedSequences::TSeqs, i,
                                      (*it)->GetSeqs()) {
                                 // assumed to be scaffold
                                 molecules.push_back(*i);
                             }
                         }
                     }}
                    break;

                default:
                    ITERATE (CGC_TaggedSequences::TSeqs, i, (*it)->GetSeqs()) {
                        // assumed to be scaffold
                        molecules.push_back(*i);
                    }
                    break;
                }
            }
        }
        else {
            // only one level to consider; therefore, it's a scaffold
            molecules.push_back(CConstRef<CGC_Sequence>(&seq));
        }
        break;

    case CGC_Assembly::eComponent:
        /// migrate all the way to leaves
        if (seq.IsSetSequences()) {
            ITERATE (CGC_Sequence::TSequences, it, seq.GetSequences()) {
                ITERATE (CGC_TaggedSequences::TSeqs, i, (*it)->GetSeqs()) {
                    s_Extract(**i, molecules, subset);
                }
            }
        } else {
            molecules.push_back(CConstRef<CGC_Sequence>(&seq));
        }
        break;

    case CGC_Assembly::eTopLevel:
        {{
             // the current sequence is top-level iff there are placed
             // sequences (i.e., chromosomes and top-level scaffolds)
             bool has_placed = false;
             if (seq.IsSetSequences()) {
                 ITERATE (CGC_Sequence::TSequences, i, seq.GetSequences()) {
                     if ((*i)->GetState() == CGC_TaggedSequences::eState_placed) {
                         has_placed = true;
                         break;
                     }
                 }
             }
             else {
                 // assume that the replicon is real...
                 has_placed = true;
             }
             if (has_placed) {
                 molecules.push_back(CConstRef<CGC_Sequence>(&seq));
             }
             // the order here is explicit
             // we check and iterate again, excluding placed sequences
             if (seq.IsSetSequences()) {
                 ITERATE (CGC_Sequence::TSequences, it, seq.GetSequences()) {
                     switch ((*it)->GetState()) {
                     case CGC_TaggedSequences::eState_placed:
                         continue;

                     default:
                         break;
                     }
                     s_Extract(**it, molecules, subset);
                 }
             }
         }}
        break;

    default:
        break;
    }
}


static void s_Extract(const CGC_Replicon& repl,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset)
{
    // replicons are chromosomes;
    // we report the replicon as real iff it contains a set of placed
    // sequences
    if (repl.GetSequence().IsSingle()) {
        s_Extract(repl.GetSequence().GetSingle(), molecules, subset);
    } else {
        // replicon is a set; 
        ITERATE (CGC_Replicon::TSequence::TSet, i,
                 repl.GetSequence().GetSet()) {
            s_Extract(**i, molecules, subset);
        }
    }
}

static bool s_RoleFitsSubset(int role, CGC_Assembly::ESubset subset)
{
    switch (subset) {
    case CGC_Assembly::eChromosome:
        return role == eGC_SequenceRole_chromosome;

    case CGC_Assembly::eScaffold:
        return role == eGC_SequenceRole_scaffold;

    case CGC_Assembly::eComponent:
        return role == eGC_SequenceRole_component;

    case CGC_Assembly::eTopLevel:
        return role == eGC_SequenceRole_top_level;

    default:
        NCBI_THROW(CException, eUnknown,
                   "Unexpected subset in call to CGC_Assembly::GetMolecules()");
    }
}

static void s_Extract(const CGC_AssemblyUnit& unit,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset)
{
    CTypeConstIterator<CGC_Sequence> sequence_it(unit);
    if (sequence_it->IsSetRoles()) {
        for ( ;  sequence_it;  ++sequence_it) {
            /// Include this sequence if it has the correct role, or if
            /// all sequences are requested
            bool fits_role = false;
            if (subset == CGC_Assembly::eAll) {
                fits_role = true;
            } else {
                ITERATE (CGC_Sequence::TRoles, it, sequence_it->GetRoles()) {
                    if (s_RoleFitsSubset(*it, subset)) {
                        fits_role = true;
                        break;
                    }
                }
            }
            if (fits_role) {
                molecules.push_back(CConstRef<CGC_Sequence>(&*sequence_it));
            }
        }
        return;
    }
    
    /// Data does not contain sequence roles, so we need to use the older,
    /// recursive method for getting the right sequences
    switch (subset) {
    case CGC_Assembly::eChromosome:
        if (unit.GetClass() != CGC_AssemblyUnit::eClass_alt_loci  &&
            unit.GetClass() != CGC_AssemblyUnit::eClass_assembly_patch  &&
            unit.IsSetMols()) {
            ITERATE (CGC_AssemblyUnit::TMols, it, unit.GetMols()) {
                s_Extract(**it, molecules, subset);
            }
        }
        break;

    case CGC_Assembly::eScaffold:
    case CGC_Assembly::eComponent:
        if (unit.IsSetMols()) {
            ITERATE (CGC_AssemblyUnit::TMols, it, unit.GetMols()) {
                s_Extract(**it, molecules, subset);
            }
        }
        if (unit.IsSetOther_sequences()) {
            ITERATE (CGC_AssemblyUnit::TOther_sequences, it,
                     unit.GetOther_sequences()) {
                s_Extract(**it, molecules, subset);
            }
        }
        break;

    case CGC_Assembly::eTopLevel:
        if (unit.IsSetMols()) {
            CGC_Assembly::ESubset tmp = subset;
            if (unit.GetClass() == CGC_AssemblyUnit::eClass_alt_loci  ||
                unit.GetClass() == CGC_AssemblyUnit::eClass_assembly_patch) {
                tmp = CGC_Assembly::eScaffold;
            }
            ITERATE (CGC_AssemblyUnit::TMols, it, unit.GetMols()) {
                s_Extract(**it, molecules, tmp);
            }
        }
        if (unit.IsSetOther_sequences()) {
            ITERATE (CGC_AssemblyUnit::TOther_sequences, it,
                     unit.GetOther_sequences()) {
                s_Extract(**it, molecules, subset);
            }
        }
        break;

    case CGC_Assembly::eAll:
        {{
             CTypeConstIterator<CGC_Sequence> seq_it(unit);
             for ( ;  seq_it;  ++seq_it) {
                 molecules.push_back(CConstRef<CGC_Sequence>(&*seq_it));
             }
         }}
        break;

    default:
        break;
    }
}


static void s_Extract(const CGC_AssemblySet& set,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset)
{
    s_Extract(set.GetPrimary_assembly(), molecules, subset);
    if (set.IsSetMore_assemblies()) {
        ITERATE (CGC_AssemblySet::TMore_assemblies, it,
                 set.GetMore_assemblies()) {
            s_Extract(**it, molecules, subset);
        }
    }
}


static void s_Extract(const CGC_Assembly& assm,
                      list< CConstRef<CGC_Sequence> >& molecules,
                      CGC_Assembly::ESubset subset)
{
    if (assm.IsUnit()) {
        s_Extract(assm.GetUnit(), molecules, subset);
    } else {
        s_Extract(assm.GetAssembly_set(), molecules, subset);
    }
}


void CGC_Assembly::GetMolecules(list< CConstRef<CGC_Sequence> >& molecules,
                                ESubset subset) const
{
    s_Extract(*this, molecules, subset);
}



END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1758, CRC32: 382c4e0c */
