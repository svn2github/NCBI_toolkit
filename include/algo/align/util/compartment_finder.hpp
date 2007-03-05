#ifndef ALGO_ALIGN_UTIL_COMPARTMENT_FINDER__HPP
#define ALGO_ALIGN_UTIL_COMPARTMENT_FINDER__HPP

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
* Author:  Yuri Kapustin, Alexander Souvorov
*
* File Description:  CCompartmentFinder- identification of genomic compartments
*                   
* ===========================================================================
*/


#include <corelib/ncbi_limits.hpp>

#include <algo/align/nw/align_exception.hpp>
#include <algo/align/util/hit_filter.hpp>

#include <algorithm>
#include <numeric>
#include <vector>


BEGIN_NCBI_SCOPE


// Detect all compartments over a set of hits
template<class THit>
class CCompartmentFinder {

public:

    typedef CRef<THit>            THitRef;
    typedef vector<THitRef>       THitRefs;
    typedef typename THit::TCoord TCoord;

    // hits must be in plus strand
    CCompartmentFinder(typename THitRefs::const_iterator start,
                       typename THitRefs::const_iterator finish);

    // setters and getters
    void SetMaxIntron (TCoord intr_max) {
        m_intron_max = intr_max;
    }

    static TCoord s_GetDefaultMaxIntron(void) {
        return 1048575;
    }
    
    void SetPenalty(TCoord penalty) {
        m_penalty = penalty;
    }
        
    void SetMinMatches(TCoord min_matches) {
        m_MinSingletonMatches = m_MinMatches = min_matches;
    }
        
    void SetMinSingletonMatches(TCoord min_matches) {
        m_MinSingletonMatches = min_matches;
    }

    static TCoord GetDefaultPenalty(void) {
        return 500;
    }
    
    static TCoord GetDefaultMinCoverage(void) {
        return 500;
    }


    size_t Run(void); // do the compartment search

    // order compartments by lower subj coordinate
    void OrderCompartments(void);

    // single compartment representation
    class CCompartment {

    public:
        
        CCompartment(void) {
            m_coverage = 0;
            m_box[0] = m_box[2] = numeric_limits<TCoord>::max();
            m_box[1] = m_box[3] = 0;
        }

        const THitRefs& GetMembers(void) {
            return m_members;
        }
    
        void AddMember(const THitRef& hitref) {
            m_members.push_back(hitref);
        }

        void SetMembers(const THitRefs& hits) {
            m_members = hits;
        }
    
        void UpdateMinMax(void);

        bool GetStrand(void) const;

        const THitRef GetFirst(void) const {
            m_iter = 0;
            return GetNext();
        }
        
        const THitRef GetNext(void) const {
            if(m_iter < m_members.size()) {
                return m_members[m_iter++];
            }
            return THitRef(NULL);
        }
        
        const TCoord* GetBox(void) const {
            return m_box;
        }
        
        bool operator < (const CCompartment& rhs) {
            return m_coverage < rhs.m_coverage;
        }
        
        static bool s_PLowerSubj(const CCompartment& c1,
                                 const CCompartment& c2) {

            return c1.m_box[2] < c2.m_box[2];
        }
        
    protected:
        
        TCoord              m_coverage;
        THitRefs            m_members;
        TCoord              m_box[4];
        mutable size_t      m_iter;
    };

    // iterate through compartments
    CCompartment* GetFirst(void);
    CCompartment* GetNext(void);

private:

    TCoord                m_intron_max;    // max intron size
    TCoord                m_penalty;       // penalty per compartment
    TCoord                m_MinMatches;    // min approx matches to report
    TCoord                m_MinSingletonMatches; // min matches for singleton comps

    THitRefs              m_hitrefs;         // input hits
    vector<CCompartment>  m_compartments;    // final compartments
    int                   m_iter;            // GetFirst/Next index
    
    struct SHitStatus {
        
        enum EType {
            eNone, 
            eExtension,
            eOpening
        };
        
        EType  m_type;
        int    m_prev;
        double m_score;        

        SHitStatus(void): m_type(eNone), m_prev(-1), m_score(0) 
        {}
        SHitStatus(EType type, int prev, double score): 
            m_type(type), m_prev(prev), m_score(score) 
        {}
    };    
};


// Facilitate access to compartments over a hit set
template<class THit>
class CCompartmentAccessor
{
public:

    typedef CCompartmentFinder<THit> TCompartmentFinder;
    typedef typename TCompartmentFinder::THitRefs THitRefs;
    typedef typename TCompartmentFinder::TCoord   TCoord;

    // [start,finish) are assumed to share same query and subj
    CCompartmentAccessor(typename THitRefs::iterator start, 
                         typename THitRefs::iterator finish,
                         TCoord comp_penalty_bps,
                         TCoord min_matches,
                         TCoord min_singleton_matches =numeric_limits<TCoord>::max());
    
    bool GetFirst(THitRefs& compartment);
    bool GetNext(THitRefs& compartment);
    
    size_t GetCount(void) const {
        return m_pending.size();
    }
    
    void Get(size_t i, THitRefs& compartment) const {
        compartment = m_pending[i];
    }
    
    const TCoord* GetBox(size_t i) const {
        return &m_ranges.front() + i*4;
    }
    
    bool GetStrand(size_t i) const {
        return m_strands[i];
    }
        
private:
    
    vector<THitRefs>         m_pending;
    vector<TCoord>           m_ranges;
    vector<bool>             m_strands;
    size_t                   m_iter;
        
    void x_Copy2Pending(TCompartmentFinder& finder);
};



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


const double kPenaltyPerIntronBase = -1e-7; // a small penalty to prefer
                                            // more compact models among equal

const double kPenaltyPerIntronPos  = -1e-9; // a small penalty to favor uniform
                                            // selection among equivalent chains

template<class THit>
void CCompartmentFinder<THit>::CCompartment::UpdateMinMax() 
{
    typedef CHitFilter<THit> THitFilter;
    THitFilter::s_GetSpan(m_members, m_box);
}


template<class THit>
bool CCompartmentFinder<THit>::CCompartment::GetStrand() const {

    if(m_members.size()) {
        return m_members.front()->GetSubjStrand();
    }
    else {
        NCBI_THROW( CAlgoAlignException,
                    eInternal,
                    "Strand requested on an empty compartment" );
    }
}


template<class THit>
CCompartmentFinder<THit>::CCompartmentFinder(
    typename THitRefs::const_iterator start,
    typename THitRefs::const_iterator finish ):

    m_intron_max(s_GetDefaultMaxIntron()),
    m_penalty(GetDefaultPenalty()),
    m_MinMatches(1),
    m_MinSingletonMatches(1),
    m_iter(-1)
{
    m_hitrefs.resize(finish - start);
    copy(start, finish, m_hitrefs.begin());
}


// accumulate matches on query
template<class THit>
class CQueryMatchAccumulator:
    public binary_function<float, CRef<THit>, float>
{
public:

    CQueryMatchAccumulator(void):
        m_Finish(-1.0f)
    {}

    float operator() (float acm, CRef<THit> ph)
    {
        typename THit::TCoord qmin = ph->GetQueryMin(), 
            qmax = ph->GetQueryMax();
        if(qmin > m_Finish)
            return acm + ph->GetIdentity() * ((m_Finish = qmax) - qmin + 1);
        else {
            if(qmax > m_Finish) {
                float finish0 = m_Finish;
                return acm + ph->GetIdentity() * ((m_Finish = qmax) - finish0);
            }
            else {
                return acm;
            }
        }
    }

private:

    float m_Finish;
};


template<class THit>
double GetTotalMatches(
    const typename CCompartmentFinder<THit>::THitRefs& hitrefs0)
{
    typename CCompartmentFinder<THit>::THitRefs hitrefs (hitrefs0);   

    typedef CHitComparator<THit> THitComparator;
    THitComparator sorter (THitComparator::eQueryMin);
    stable_sort(hitrefs.begin(), hitrefs.end(), sorter);

    const float rv = accumulate(hitrefs.begin(), hitrefs.end(), 0.0f, 
                                CQueryMatchAccumulator<THit>());
    return rv;
}


template<class THit>
size_t CCompartmentFinder<THit>::Run()
{
    const double kMinusInf = -1e12;

    m_compartments.clear();
    const int dimhits = m_hitrefs.size();
    if(dimhits == 0) {
        return 0;
    }
    
    // sort the hits to make sure that each hit is placed after:
    // - hits from which to continue a compartment
    // - hits from which to open a new compartment

    typedef CHitComparator<THit> THitComparator;
    THitComparator sorter (THitComparator::eSubjMaxQueryMax);
    stable_sort(m_hitrefs.begin(), m_hitrefs.end(), sorter);
        
    // For every hit:
    // - evaluate its best extension potential
    // - evaluate its best potential to start a new compartment
    // - select the best variant
    // - update the hit status array
    
    typedef vector<SHitStatus> THitData;
    THitData hitstatus (dimhits);

    const TCoord subj_min_global (m_hitrefs.front()->GetSubjMin());

    for(int i = 0; i < dimhits; ++i) {
        
        const THitRef& h = m_hitrefs[i];
        const typename THit::TCoord hbox [4] = {
            h->GetQueryMin(),  h->GetQueryMax(),
            h->GetSubjMin(),   h->GetSubjMax()
        };

//#define CF_DBG_TRACE
#ifdef CF_DBG_TRACE
        cerr << endl << *h << endl;
#endif

        double best_ext_score = kMinusInf;
        int    i_best_ext = -1;
        double best_open_score = kMinusInf;
        int    i_best_open = -1;

        for(int j = i; j >= 0; --j) {
            
            THitRef phc;
            typename THit::TCoord phcbox[4];
            if(j != i) {

                phc = m_hitrefs[j];
                phcbox[0] = phc->GetQueryMin();
                phcbox[1] = phc->GetQueryMax();
                phcbox[2] = phc->GetSubjMin();
                phcbox[3] = phc->GetSubjMax();
#ifdef CF_DBG_TRACE
                cerr << '\t' << *phc << endl;
#endif
            }
#ifdef CF_DBG_TRACE
            else {
                cerr << "\t[Dummy]" << endl;
            }
#endif
            
            // check if good for extension
            {{
                typename THit::TCoord q0, s0; // possible continuation
                bool good = false;
                int subj_space;
                TCoord intron_start (0);
                if(i != j) {

                    if(phcbox[1] < hbox[1] && phcbox[0] < hbox[0]) {

                        if(hbox[0] <= phcbox[1] && 
                           hbox[2] + phcbox[1] - hbox[0] >= phcbox[3]) {

                            q0 = phcbox[1] + 1;
                            s0 = hbox[2] + phcbox[1] - hbox[0];
                        }
                        else if(phcbox[3] >= hbox[2]) {

                            s0 = phcbox[3] + 1;
                            q0 = hbox[1] - (hbox[3] - s0);
                        }
                        else {
                            
                            q0 = hbox[0];
                            s0 = hbox[2];
                        }
                        subj_space = s0 - phcbox[3] - 1;

                        const TCoord max_gap = 50; // max run of spaces
                                                   // inside an exon
                        good = subj_space <= int(m_intron_max)
                            && subj_space + max_gap >= q0 - phcbox[1] - 1;

                        if(good) {
                            intron_start = phcbox[3];
                        }
                    }
                }
                
                if(good) {
                    
                    const double identity = h->GetIdentity();
                    const double jscore = hitstatus[j].m_score;
                    const double intron_penalty = (subj_space > 0)?
                        (kPenaltyPerIntronPos * (intron_start - subj_min_global) 
                         + subj_space * kPenaltyPerIntronBase):
                        0.0;

                    const double ext_score = jscore +
                        identity * (hbox[1] - q0 + 1) + intron_penalty;

                    if(ext_score > best_ext_score) {
                        best_ext_score = ext_score;
                        i_best_ext = j;
                    }
#ifdef CF_DBG_TRACE
                    cerr << "\tGood for extension with score = " << ext_score << endl;
#endif
                }
            }}
            
            // check if good to open a new compartment
            if(i == j || hbox[2] > phcbox[3]) {

                const double identity = h->GetIdentity();
                const double jscore = (i == j)? 0: hitstatus[j].m_score;
                double score_open = jscore - m_penalty +
                    identity * (hbox[1] - hbox[0] + 1);
                if(score_open > best_open_score) {
                    best_open_score = score_open;
                    i_best_open = (i == j)? -1: j;
                }
#ifdef CF_DBG_TRACE
                cerr << "\tGood for opening with score = " << score_open << endl;
#endif
            }
        }
        
        typename SHitStatus::EType hit_type;
        int prev_hit;
        double best_score;
        if(best_ext_score > best_open_score) {

            hit_type = SHitStatus::eExtension;
            prev_hit = i_best_ext;
            best_score = best_ext_score;
        }
        else {

            hit_type = SHitStatus::eOpening;
            prev_hit = i_best_open;
            best_score = best_open_score;
        }
                
        hitstatus[i].m_type = hit_type;
        hitstatus[i].m_prev = prev_hit;
        hitstatus[i].m_score = best_score;

#ifdef CF_DBG_TRACE
        cerr << "Status = " << ((hit_type == SHitStatus::eOpening)? "Open": "Extend")
             << '\t' << best_score << endl;
        if(prev_hit == -1) {
            cerr << "[Dummy]" << endl;
        }
        else {
            cerr << *(m_hitrefs[prev_hit]) << endl;
        }
#endif
    }
    
#ifdef CF_DBG_TRACE
    cerr << "\n\n--- BACKTRACE ---\n";
#endif

    // *** backtrace ***
    // - find the chain with the highest score and trace it back
    int ibest = -1;
    double score_best = kMinusInf;
    for(int i = 0, n = hitstatus.size(); i < n; ++i) {
        if(score_best < hitstatus[i].m_score) {
            score_best = hitstatus[i].m_score;
            ibest = i;
        }
    }
    
    const double min_matches = m_MinSingletonMatches < m_MinMatches? 
        m_MinSingletonMatches: m_MinMatches;
    if(score_best + m_penalty >= min_matches) {

        int i = ibest;
        bool new_compartment = true;
        THitRefs hitrefs;
        while(i != -1) {

            if(new_compartment) {
                float mp (GetTotalMatches<THit>(hitrefs));
                if(mp >= m_MinMatches) {
                    // save the current compartment
                    m_compartments.push_back(CCompartment());
                    m_compartments.back().SetMembers(hitrefs);
                }
                hitrefs.resize(0);
                new_compartment = false;
            }
            hitrefs.push_back(m_hitrefs[i]);

#ifdef CF_DBG_TRACE
            cerr << *(m_hitrefs[i]) << endl;
#endif            
            
            if(hitstatus[i].m_type == SHitStatus::eOpening) {
                new_compartment = true;
            }
            i = hitstatus[i].m_prev;
        }

        float mp (GetTotalMatches<THit>(hitrefs));
        if(m_compartments.size() == 0 && mp >= m_MinSingletonMatches 
           || mp >= m_MinMatches) 
        {
            m_compartments.push_back(CCompartment());
            m_compartments.back().SetMembers(hitrefs);
        }
    }
    
    return m_compartments.size();
}


template<class THit>
typename CCompartmentFinder<THit>::CCompartment* 
CCompartmentFinder<THit>::GetFirst()
{
    if(m_compartments.size()) {
        m_iter = 0;
        return &m_compartments[m_iter++];
    }
    else {
        m_iter = -1;
        return 0;
    }
}

template<class THit>
void CCompartmentFinder<THit>::OrderCompartments(void) {
  
    for(size_t i = 0, dim = m_compartments.size(); i < dim; ++i) {
        m_compartments[i].UpdateMinMax();
    }
    
    stable_sort(m_compartments.begin(), m_compartments.end(), 
                CCompartmentFinder<THit>::CCompartment::s_PLowerSubj);
}

template<class THit>
typename CCompartmentFinder<THit>::CCompartment* 
CCompartmentFinder<THit>::GetNext()
{
    const size_t dim = m_compartments.size();
    if(m_iter == -1 || m_iter >= int(dim)) {
        m_iter = -1;
        return 0;
    }
    else {
        return &m_compartments[m_iter++];
    }
}


template<class THit>
CCompartmentAccessor<THit>::CCompartmentAccessor(
     typename THitRefs::iterator istart,
     typename THitRefs::iterator ifinish,
     TCoord comp_penalty,
     TCoord min_matches,
     TCoord min_singleton_matches)
{
    const TCoord kMax_TCoord = numeric_limits<TCoord>::max();

    const TCoord max_intron = CCompartmentFinder<THit>::s_GetDefaultMaxIntron();

    // separate strands for CompartmentFinder
    typename THitRefs::iterator ib = istart, ie = ifinish, ii = ib, 
        iplus_beg = ie;
    typedef CHitComparator<THit> THitComparator;
    THitComparator sorter (THitComparator::eSubjStrand);
    stable_sort(ib, ie, sorter);

    TCoord minus_subj_min = kMax_TCoord, minus_subj_max = 0;
    for(ii = ib; ii != ie; ++ii) {
        if((*ii)->GetSubjStrand()) {
            iplus_beg = ii;
            break;
        }
        else {
            if((*ii)->GetSubjMin() < minus_subj_min) {
                minus_subj_min = (*ii)->GetSubjMin();
            }
            if((*ii)->GetSubjMax() > minus_subj_max) {
                minus_subj_max = (*ii)->GetSubjMax();
            }
        }
    }
    
    // minus
    {{
        // flip
        for(ii = ib; ii != iplus_beg; ++ii) {

            const typename THit::TCoord s0 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMax();
            const typename THit::TCoord s1 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMin();
            (*ii)->SetSubjStart(s0);
            (*ii)->SetSubjStop(s1);
        }
        
        CCompartmentFinder<THit> finder (ib, iplus_beg);
        finder.SetPenalty(comp_penalty);
        finder.SetMinMatches(min_matches);
        finder.SetMinSingletonMatches(min_singleton_matches);
        finder.SetMaxIntron(max_intron);
        finder.Run();
        
        // un-flip
        for(ii = ib; ii != iplus_beg; ++ii) {

            const typename THit::TCoord s0 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMax();
            const typename THit::TCoord s1 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMin();
            (*ii)->SetSubjStart(s1);
            (*ii)->SetSubjStop(s0);
        }        
        x_Copy2Pending(finder);
    }}

    // plus
    {{
        CCompartmentFinder<THit> finder (iplus_beg, ie);
        finder.SetPenalty(comp_penalty);
        finder.SetMinMatches(min_matches);
        finder.SetMinSingletonMatches(min_singleton_matches);
        finder.SetMaxIntron(max_intron);
        finder.Run();
        x_Copy2Pending(finder);
    }}
}


template<class THit>
void CCompartmentAccessor<THit>::x_Copy2Pending(
    CCompartmentFinder<THit>& finder)
{
    finder.OrderCompartments();

    typedef typename CCompartmentFinder<THit>::THitRef THitRef;

    // copy to pending
    for(typename CCompartmentFinder<THit>::CCompartment* compartment =
            finder.GetFirst();  compartment; compartment = finder.GetNext()) {
        
        m_pending.push_back(THitRefs(0));
        THitRefs& vh = m_pending.back();
        
        for(THitRef ph = compartment->GetFirst(); ph; 
            ph = compartment->GetNext()) {
            
            vh.push_back(ph);
        }
        
        const TCoord* box = compartment->GetBox();
        m_ranges.push_back(box[0] - 1);
        m_ranges.push_back(box[1] - 1);
        m_ranges.push_back(box[2] - 1);
        m_ranges.push_back(box[3] - 1);
        
        m_strands.push_back(compartment->GetStrand());
    }
}


template<class THit>
bool CCompartmentAccessor<THit>::GetFirst(THitRefs& compartment) {

    m_iter = 0;
    return GetNext(compartment);
}


template<class THit>
bool CCompartmentAccessor<THit>::GetNext(THitRefs& compartment) {

    compartment.clear();
    if(m_iter < m_pending.size()) {
        compartment = m_pending[m_iter++];
        return true;
    }
    else {
        return false;
    }
}



END_NCBI_SCOPE


#endif
