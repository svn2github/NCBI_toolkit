#ifndef _DeBruijn_Graph_
#define _DeBruijn_Graph_

#include <iostream>
#include "Integer.hpp"
#include <bitset>
#include <unordered_map>
#include <unordered_set>
#include <algo/gnomon/gnomon_model.hpp>

USING_NCBI_SCOPE;
USING_SCOPE(gnomon);
USING_SCOPE(boost);
USING_SCOPE(std);

namespace DeBruijn {

    template <typename T1, typename T2, typename T3, typename T4> class CKmerCountTemplate {
    public:
        typedef variant<vector<pair<T1,size_t>>, vector<pair<T2,size_t>>, vector<pair<T3,size_t>>, vector<pair<T4,size_t>> > Type;

        CKmerCountTemplate(int kmer_len = 0) : m_kmer_len(kmer_len) {
            if(m_kmer_len > 0) {
                init_conainer();
            }
        }
        size_t Size() const {
            return apply_visitor(container_size(), m_container);
        }
        void Reserve(size_t rsrv) { apply_visitor(reserve(rsrv), m_container); }
        void Clear() { apply_visitor(clear(), m_container); }
        size_t Capacity() const {
            return apply_visitor(container_capacity(), m_container);
        }
        void PushBack(const TKmer& kmer, size_t count) { 
            if(m_kmer_len == 0) {
                cerr << "Can't insert in not initialized container" << endl;
                exit(1);
            }
            apply_visitor(push_back(kmer, count), m_container); 
        }
        void PushBackElementsFrom(const CKmerCountTemplate& other) { 
            if(m_kmer_len == 0) {
                cerr << "Can't insert in not initialized container" << endl;
                exit(1);
            }
            return apply_visitor(push_back_elements(), m_container, other.m_container); 
        }
        // 32 bit count; 8 bit branching; 1 bit 'visited'; 7 bit not used yet; 16 bit +/-
        void UpdateCount(size_t count, size_t index) { apply_visitor(update_count(count, index), m_container); }
        size_t GetCount(size_t index) const { return apply_visitor(get_count(index), m_container); }
        pair<TKmer,size_t> GetKmerCount(size_t index) const { return apply_visitor(get_kmer_count(index), m_container); }
        size_t Find(const TKmer& kmer) const { return apply_visitor(find_kmer(kmer), m_container); }
        int KmerLen() const { return m_kmer_len; }
        void Sort() { apply_visitor(container_sort(), m_container); }
        void SortAndExtractUniq(int min_count, CKmerCountTemplate& uniq) {
            uniq = CKmerCountTemplate(m_kmer_len); // init
            Sort();
            apply_visitor(extract_uniq(min_count), m_container, uniq.m_container);
        }
        void SortAndUniq(int min_count) {
            Sort();
            apply_visitor(uniq(min_count), m_container);
        }
        void MergeTwoSorted(const CKmerCountTemplate& other) {
            if(m_kmer_len != other.KmerLen()) {
                cerr << "Can't merge different kmers" << endl;
                exit(1);
            }
            apply_visitor(merge_sorted(), m_container, other.m_container);
        }
        void Swap(CKmerCountTemplate& other) {
            swap(m_kmer_len, other.m_kmer_len);
            apply_visitor(swap_with_other(), m_container, other.m_container);    
        }
        void Save(ostream& out) const { 
            out.write(reinterpret_cast<const char*>(&m_kmer_len), sizeof(m_kmer_len));
            apply_visitor(save(out), m_container);
        }
        void Load(istream& in) {
            in.read(reinterpret_cast<char*>(&m_kmer_len), sizeof(m_kmer_len));
            init_conainer();
            apply_visitor(load(in), m_container);
        }

    private:
        void init_conainer() {
            switch ((m_kmer_len+31)/32) {            
            case PREC_1: m_container = vector<pair<T1,size_t>>(); break;
            case PREC_2: m_container = vector<pair<T2,size_t>>(); break;
            case PREC_3: m_container = vector<pair<T3,size_t>>(); break;
            case PREC_4: m_container = vector<pair<T4,size_t>>(); break;
            default: throw runtime_error("Not supported kmer length");
            }
        }

        struct find_kmer : public static_visitor<size_t> { 
            find_kmer(const TKmer& k) : kmer(k) {}
            template <typename T> size_t operator()(const T& v) const { 
                typedef typename T::value_type pair_t;
                typedef typename pair_t::first_type large_t;
                auto it = lower_bound(v.begin(), v.end(), kmer.get<large_t>(), [](const pair_t& element, const large_t& target){ return element.first < target; });
                if(it == v.end() || it->first != kmer.get<large_t>())
                    return v.size();
                else
                    return it-v.begin();
            } 
            const TKmer& kmer;
        };
        struct reserve : public static_visitor<> { 
            reserve(size_t r) : rsrv(r) {}
            template <typename T> void operator() (T& v) const { v.reserve(rsrv); }
            size_t rsrv;
        };
        struct container_size : public static_visitor<size_t> { template <typename T> size_t operator()(const T& v) const { return v.size();} };
        struct container_capacity : public static_visitor<size_t> { template <typename T> size_t operator()(const T& v) const { return v.capacity();} };
        struct clear : public static_visitor<> { template <typename T> void operator()(const T& v) const { v.clear();} };
        struct push_back : public static_visitor<> { 
            push_back(const TKmer& k, size_t c) : kmer(k), count(c) {}
            template <typename T> void operator() (T& v) const {
                typedef typename  T::value_type::first_type large_t;
                v.push_back(make_pair(kmer.get<large_t>(), count)); 
            }
            const TKmer& kmer;
            size_t count;
        };
        struct push_back_elements : public static_visitor<> {
            template <typename T> void operator() (T& a, const T& b) const { a.insert(a.end(), b.begin(), b.end()); }
            template <typename T, typename U> void operator() (T& a, const U& b) const { cerr << "Can't copy from different type container" << endl; exit(1); }
        };
        struct merge_sorted : public static_visitor<> {
            template <typename T> void operator() (T& a, const T& b) const { 
                T merged;
                merged.reserve(a.size()+b.size());
                merge(a.begin(), a.end(), b.begin(), b.end(), back_inserter(merged));
                merged.swap(a); 
            }
            template <typename T, typename U> void operator() (T& a, const U& b) const { cerr << "This is not happening!" << endl; exit(1); }
        };
        struct update_count : public static_visitor<> {
            update_count(size_t c, size_t i) : count(c), index(i) {}
            template <typename T> void operator() (T& v) const { v[index].second = count; }  
            size_t count;
            size_t index;
        };
        struct get_count : public static_visitor<size_t> {
            get_count(size_t i) : index(i) {}
            template <typename T> size_t operator() (T& v) const { return v[index].second; }        
            size_t index;
        };
        struct get_kmer_count : public static_visitor<pair<TKmer,size_t>> {
            get_kmer_count(size_t i) : index(i) {}
            template <typename T> pair<TKmer,size_t> operator() (T& v) const { return make_pair(TKmer(v[index].first), v[index].second); }        
            size_t index;
        };
        struct container_sort : public static_visitor<> { template <typename T> void operator() (T& v) const { sort(v.begin(), v.end()); }};
        struct swap_with_other : public static_visitor<> { 
            template <typename T> void operator() (T& a, T& b) const { a.swap(b); }
            template <typename T, typename U> void operator() (T& a, U& b)  const { cerr << "Can't swap different type containers" << endl; exit(1); } 
        };
        struct uniq : public static_visitor<> {
            uniq(int mc) : min_count(mc) {}
            template <typename T> void operator() (T& v) const {
                typedef typename T::iterator iter_t;
                iter_t nextp = v.begin();
                for(iter_t ip = v.begin(); ip != v.end(); ) {
                    iter_t workp = ip;
                    while(++ip != v.end() && workp->first == ip->first)
                        workp->second += ip->second;   // accumulate all 8 bytes; we assume that count will not spill into higher half
                    if((uint32_t)workp->second >= min_count)
                        *nextp++ = *workp;
                }
                v.erase(nextp, v.end());
            }

            int min_count;
        };
        struct extract_uniq : public static_visitor<> {
            extract_uniq(int mc) : min_count(mc) {}
            template <typename T> void operator() (T& a, T& b) const {
                if(a.empty()) return;
                size_t num = 1;
                uint32_t count = a[0].second;  // count only 4 bytes!!!!!!
                for(size_t i = 1; i < a.size(); ++i) {
                    if(a[i-1].first < a[i].first) {
                        if(count >= min_count)
                            ++num;
                        count = a[i].second;
                    } else {
                        count += a[i].second;
                    }
                }
                if(count < min_count)
                    --num;
                b.reserve(num+1);
                b.push_back(a[0]);
                for(size_t i = 1; i < a.size(); ++i) {
                    if(b.back().first < a[i].first) {
                        if((uint32_t)b.back().second < min_count)
                            b.pop_back();
                        b.push_back(a[i]);
                    } else {
                        b.back().second += a[i].second;  // accumulate all 8 bytes; we assume that count will not spill into higher half
                    }
                }
                if((uint32_t)b.back().second < min_count)
                    b.pop_back();
            }
            template <typename T, typename U> void operator() (T& a, U& b) const { cerr << "This is not happening!" << endl; exit(1); }

            int min_count;
        };
        struct save : public static_visitor<> {
            save(ostream& out) : os(out) {}
            template <typename T> void operator() (T& v) const {
                size_t num = v.size();
                os.write(reinterpret_cast<const char*>(&num), sizeof num); 
                os.write(reinterpret_cast<const char*>(&v[0]), num*sizeof(v[0]));
            }
            ostream& os;
        };
        struct load : public static_visitor<> {
            load(istream& in) : is(in) {}
            template <typename T> void operator() (T& v) const {
                size_t num;
                is.read(reinterpret_cast<char*>(&num), sizeof num);
                v.resize(num);
                is.read(reinterpret_cast<char*>(&v[0]), num*sizeof(v[0]));
            }
            istream& is;
        };

        Type m_container;
        int m_kmer_len;
    };
    typedef CKmerCountTemplate <INTEGER_TYPES> TKmerCount;

    template <typename T1, typename T2, typename T3, typename T4> class CKmerMapTemplate {
    public:
        struct kmer_hash {
            template<typename T> 
            size_t operator() (const T& kmer) const { return kmer.oahash(); }
        };
        typedef variant<unordered_map<T1,size_t,kmer_hash>, unordered_map<T2,size_t,kmer_hash>, unordered_map<T3,size_t,kmer_hash>, unordered_map<T4,size_t,kmer_hash>> Type;
        CKmerMapTemplate(int kmer_len = 0) : m_kmer_len(kmer_len) {
            if(m_kmer_len > 0) {
                init_conainer();
            }
        }

        size_t Size() const {
            return apply_visitor(container_size(), m_container);
        }

        size_t& operator[] (const TKmer& kmer) { 
            return apply_visitor(mapper(kmer), m_container);
        }

        void DumpKmers(TKmerCount& kmer_count, int mincount) {
            kmer_count = TKmerCount(m_kmer_len);
            apply_visitor(dump_kmers(kmer_count,mincount), m_container);
        }

    private:
        void init_conainer() {
            switch ((m_kmer_len+31)/32) {            
            case PREC_1: m_container = unordered_map<T1,size_t,kmer_hash>(); break;
            case PREC_2: m_container = unordered_map<T2,size_t,kmer_hash>(); break;
            case PREC_3: m_container = unordered_map<T3,size_t,kmer_hash>(); break;
            case PREC_4: m_container = unordered_map<T4,size_t,kmer_hash>(); break;                
            default: throw runtime_error("Not supported kmer length");
            }
        }

        struct container_size : public static_visitor<size_t> { template <typename T> size_t operator()(const T& v) const { return v.size();} };

        struct mapper : public static_visitor<size_t&> { 
            mapper(const TKmer& k) : kmer(k) {}
            template <typename T> size_t& operator()(T& v) const { 
                typedef typename  T::key_type large_t;
                return v[kmer.get<large_t>()];
            } 
            const TKmer& kmer;
        };

        struct dump_kmers : public static_visitor<> { 
            dump_kmers(TKmerCount& kc, int mc) : kmer_count(kc), mincount(mc) {}
            template <typename T> void operator()(T& v) const { 
                size_t num = 0;
                for(auto& e : v) {
                    if(int(e.second) >= mincount)  // ignore 'upper' count
                        ++num;
                }
                kmer_count.Reserve(num);
                for(auto& e : v) {
                    if(int(e.second) >= mincount)
                        kmer_count.PushBack(TKmer(e.first), e.second); 
                }
            }

            TKmerCount& kmer_count;
            int mincount;
        };


        Type m_container;
        int m_kmer_len;
    };
    typedef CKmerMapTemplate <INTEGER_TYPES> TKmerMap;


    class CDBGraph {
    public:

        typedef size_t Node;
        struct Successor {
            Successor(const Node& node, char c) : m_node(node), m_nt(c) {}
            Node m_node;
            char m_nt;
        };

        CDBGraph(istream& in) {
            m_graph_kmers.Load(in);
            string max_kmer(m_graph_kmers.KmerLen(), bin2NT[3]);
            m_max_kmer = TKmer(max_kmer);
        }

        // 0 for not contained kmers    
        // positive even numbers for direct kmers   
        // positive odd numbers for reversed kmers  
        Node GetNode(const TKmer& kmer) const {
            TKmer rkmer = revcomp(kmer, KmerLen());
            if(kmer < rkmer) {
                size_t index = m_graph_kmers.Find(kmer);
                return (index == GraphSize() ? 0 : 2*(index+1));
            } else {
                size_t index = m_graph_kmers.Find(rkmer);
                return (index == GraphSize() ? 0 : 2*(index+1)+1);
            }
        }
        Node GetNode(const string& kmer_seq) const {
            if(kmer_seq.find_first_not_of("ACGT") != string::npos || (int)kmer_seq.size() != KmerLen())   // invalid kmer
                return 0;
            TKmer kmer(kmer_seq);
            return GetNode(kmer);
        }

        // for all access with Node there is NO check that node is in range !!!!!!!!
        int Abundance(const Node& node) const {
            if(node == 0)
                return 0;
            else
                return m_graph_kmers.GetKmerCount(node/2-1).second;  // atomatically clips branching information!
        }
        // 32 bit count; 8 bit branching; 1 bit 'visited'; 7 bit not used yet; 16 bit +/-
        double MinFraction(const Node& node) const {
            double plusf = PlusFraction(node);
            return min(plusf,1-plusf);
        }
        double PlusFraction(const Node& node) const {
            double plusf = double(m_graph_kmers.GetKmerCount(node/2-1).second >> 48)/numeric_limits<uint16_t>::max();
            if(node%2)
                plusf = 1-plusf;
            return plusf;
        }
        TKmer GetNodeKmer(const Node& node) const {
            if(node%2 == 0) 
                return m_graph_kmers.GetKmerCount(node/2-1).first;
            else
                return revcomp(m_graph_kmers.GetKmerCount(node/2-1).first, KmerLen());
        }
        string GetNodeSeq(const Node& node) const {
            return GetNodeKmer(node).toString(KmerLen());
        }

        void SetVisited(const Node& node) {
            m_graph_kmers.UpdateCount(m_graph_kmers.GetCount(node/2-1) | (uint64_t(1) << 40), node/2-1);
        }

        void ClearVisited(const Node& node) {
            m_graph_kmers.UpdateCount(m_graph_kmers.GetCount(node/2-1) & ~(uint64_t(1) << 40), node/2-1);
        }

        bool IsVisited(const Node& node) const {
            return (m_graph_kmers.GetCount(node/2-1) & (uint64_t(1) << 40));
        }
        
        vector<Successor> GetNodeSuccessors(const Node& node) const {
            vector<Successor> successors;
            if(!node)
                return successors;

            uint8_t branch_info = (m_graph_kmers.GetCount(node/2-1) >> 32);
            bitset<4> branches(node%2 ? (branch_info >> 4) : branch_info);
            if(branches.count()) {
                TKmer shifted_kmer = (GetNodeKmer(node) << 2) & m_max_kmer;
                for(int nt = 0; nt < 4; ++nt) {
                    if(branches[nt]) {
                        Node successor = GetNode(shifted_kmer + TKmer(KmerLen(), nt));
                        successors.push_back(Successor(successor, bin2NT[nt]));
                    }
                }
            }

            return successors;            
        }

        static Node ReverseComplement(Node node) {
            if(node != 0)
                node = (node%2 == 0 ? node+1 : node-1);

            return node;
        }

        int KmerLen() const { return m_graph_kmers.KmerLen(); }
        size_t GraphSize() const { return m_graph_kmers.Size(); }

    private:

        TKmerCount m_graph_kmers;     // only the minimal kmers are stored  
        TKmer m_max_kmer;             // contains 1 in all kmer_len bit positions    
    };

    class CReadHolder {
    public:
        CReadHolder() :  m_total_seq(0) {};
        void InsertRead(const string& read) {
            int shift = (m_total_seq*2)%64;
            for(int i = (int)read.size()-1; i >= 0; --i) {  // put backward for kmer compatibility
                if(shift == 0)
                    m_storage.push_back(0);
                m_storage.back() += ((find(bin2NT.begin(), bin2NT.end(),  read[i]) - bin2NT.begin()) << shift);
                shift = (shift+2)%64;
            }
            m_read_length.push_back(read.size());
            m_total_seq += read.size();
        }

        class kmer_iterator {
        public:
            kmer_iterator(int kmer_len, const CReadHolder& rholder, size_t position = 0, size_t position_in_read = 0, size_t read = 0) : m_kmer_len(kmer_len), m_readholder(rholder), m_position(position), m_position_in_read(position_in_read), m_read(read) {
                SkipShortReads();
            }

            TKmer operator*() const {
                TKmer kmer(m_kmer_len, 0);
                uint64_t* guts = (uint64_t*)kmer.getPointer();
                int precision = kmer.getSize()/64;
                
                size_t first_word = m_position/64;
                int shift = m_position%64;
                if(shift > 0) {    // first/last words are split
                    copy(m_readholder.m_storage.begin()+first_word+1, m_readholder.m_storage.begin()+first_word+1+precision, guts);
                    kmer = (kmer << 64-shift);  // make space for first partial word
                    *guts += m_readholder.m_storage[first_word] >> shift;
                } else {
                    copy(m_readholder.m_storage.begin()+first_word, m_readholder.m_storage.begin()+first_word+precision, guts);
                }
                
                int partial_part_bits = 2*(m_kmer_len%32);
                if(partial_part_bits > 0) {
                    uint64_t mask = (uint64_t(1) << partial_part_bits) - 1;
                    guts[precision-1] &= mask;
                }
                
                return kmer;
            }

            kmer_iterator& operator++() {
                if(m_position == 2*(m_readholder.m_total_seq-m_kmer_len)) {
                    m_position = 2*m_readholder.m_total_seq;
                    return *this;
                }

                m_position += 2;
                if(++m_position_in_read == m_readholder.m_read_length[m_read]-m_kmer_len+1) {
                    m_position += 2*(m_kmer_len-1);
                    ++m_read;
                    m_position_in_read = 0;
                    SkipShortReads();
                } 

                return *this;
            }
            friend bool operator==(kmer_iterator const& li, kmer_iterator const& ri) { return li.m_position == ri.m_position; }
            friend bool operator!=(kmer_iterator const& li, kmer_iterator const& ri) { return li.m_position != ri.m_position; }

        private:
            void SkipShortReads() {
                while(m_position < 2*m_readholder.m_total_seq && m_read < m_readholder.m_read_length.size() && m_readholder.m_read_length[m_read] < m_kmer_len)
                    m_position += 2*m_readholder.m_read_length[m_read++];               
            }
            int m_kmer_len;
            const CReadHolder& m_readholder;
            size_t m_position;      // BIT num in deque
            int m_position_in_read; // SYMBOL in read
            size_t m_read;          // read number
        };
        kmer_iterator kbegin(int kmer_len) const { return kmer_iterator(kmer_len, *this); }
        kmer_iterator kend() const { return kmer_iterator(0, *this, 2*m_total_seq, m_read_length.back(), m_read_length.size()); }

        class string_iterator {
        public:
            string_iterator(const CReadHolder& rholder, size_t position = 0, size_t read = 0) : m_readholder(rholder), m_position(position), m_read(read) {}

            string operator*() const {
                string read;
                int read_length = m_readholder.m_read_length[m_read];
                size_t position = m_position;
                for(int i = 0; i < read_length; ++i) {
                    read.push_back(bin2NT[(m_readholder.m_storage[position/64] >> position%64) & 3]);
                    position += 2;
                }
                reverse(read.begin(), read.end());
                return read;
            }
            string_iterator& operator++() {
                if(m_position == 2*m_readholder.m_total_seq)
                    return *this;
                m_position +=  2*m_readholder.m_read_length[m_read++]; 
                return *this;
            }
            friend bool operator==(string_iterator const& li, string_iterator const& ri) { return li.m_position == ri.m_position; }
            friend bool operator!=(string_iterator const& li, string_iterator const& ri) { return li.m_position != ri.m_position; }
        

        private:
            const CReadHolder& m_readholder;
            size_t m_position;
            size_t m_read;
        };
        string_iterator sbegin() const { return string_iterator(*this); }
        string_iterator send() const { return string_iterator(*this, 2*m_total_seq, m_read_length.size()); }


    private:
        deque<uint64_t> m_storage;
        deque<uint16_t> m_read_length;
        size_t m_total_seq;
    };

class CDBGraphDigger {
public:
    typedef list<string> TStrList;

    CDBGraphDigger(CDBGraph& graph, double fraction, int jump) : m_graph(graph), m_fraction(fraction), m_jump(jump) {}

private:
    typedef deque<CDBGraph::Successor> TBases;
    typedef tuple<TStrList::iterator,int> TContigEnd;
public:
    void ImproveContigs(TStrList& contigs, bool keep_jum_free_zone) {
        int kmer_len = m_graph.KmerLen();

        int SCAN_WINDOW = 50;  // 0 - no scan
        int JUMP_FREE_ZONE = PREC_4*32+SCAN_WINDOW;

        size_t kmers = 0;
        size_t kmers_in_graph = 0;
        //mark already used kmers
        for(string& contig : contigs) {
            CReadHolder rh;
            rh.InsertRead(contig);
            for(CReadHolder::kmer_iterator itk = rh.kbegin(kmer_len); itk != rh.kend(); ++itk) {
                ++kmers;
                CDBGraph::Node node = m_graph.GetNode(*itk);
                if(node) {
                    m_graph.SetVisited(node);
                    ++kmers_in_graph;
                }
            }
        }
        cerr << "Kmers in contigs: " << kmers << " Not in graph: " << kmers-kmers_in_graph << endl;

        {
            //create new seeds without jump 
            TStrList new_seeds;
            int jump = m_jump;
            m_jump = 0;
            for(size_t index = 0; index < m_graph.GraphSize(); ++index) {
                if(!m_graph.IsVisited(2*(index+1))) {                 
                    CDBGraph::Node node = 2*(index+1);
                    pair<string, double> contig = GetContigForKmer(node);
                    new_seeds.push_back(contig.first);                
                }
            }
            m_jump = jump;

            size_t num = 0;
            for(string& contig : new_seeds) {
                if((int)contig.size() >= JUMP_FREE_ZONE) {
                    contigs.push_back(contig);
                    ++num;
                } else {
                    CReadHolder rh;
                    rh.InsertRead(contig);
                    for(CReadHolder::kmer_iterator itk = rh.kbegin(kmer_len); itk != rh.kend(); ++itk) {
                        CDBGraph::Node node = m_graph.GetNode(*itk);
                        m_graph.ClearVisited(node);
                    }
                }
            }
            cerr << "New seeds: " << num << endl;
        }

        //create landing spots
        unordered_map<CDBGraph::Node, TContigEnd> landing_spots;
        landing_spots.reserve(2*(JUMP_FREE_ZONE-kmer_len+1)*contigs.size());
        typedef list<CDBGraph::Node> TNodeList;
        typedef pair<TStrList::iterator, TNodeList> TItL;
        list<TItL> landing_spots_for_contig;
        NON_CONST_ITERATE(TStrList, ic, contigs) {
            string& contig = *ic;
            int len = contig.size();
            landing_spots_for_contig.push_back(make_pair(ic,TNodeList()));
            TNodeList& contig_spots = landing_spots_for_contig.back().second;
            int scan_window = min(SCAN_WINDOW,(len-kmer_len)/2);
            
            CReadHolder lrh;
            string lchunk = contig.substr(0,kmer_len+scan_window);
            lrh.InsertRead(lchunk);
            int shift = scan_window+1;
            for(CReadHolder::kmer_iterator itk = lrh.kbegin(kmer_len); itk != lrh.kend(); ++itk) { // gives kmers in reverse order!
                CDBGraph::Node node = m_graph.GetNode(*itk);
                if(node) {
                    landing_spots[node] = TContigEnd(ic, shift);
                    contig_spots.push_back(node);
                }
                --shift;
            }
            CReadHolder rrh;
            string rchunk = contig.substr(contig.size()-kmer_len-scan_window, kmer_len+scan_window);
            ReverseComplement(rchunk.begin(), rchunk.end());
            rrh.InsertRead(rchunk);
            shift = scan_window+1;
            for(CReadHolder::kmer_iterator itk = rrh.kbegin(kmer_len); itk != rrh.kend(); ++itk) { // gives kmers in reverse order!
                CDBGraph::Node node = m_graph.GetNode(*itk);
                if(node) {
                    landing_spots[node] = TContigEnd(ic, -shift);
                    contig_spots.push_back(node);
                }
                --shift;
            }                    
        }

        TStrList new_contigs;
        while(!contigs.empty()) {
            TStrList::iterator ic = contigs.begin();
            new_contigs.push_back(*ic);
            string& contig = new_contigs.back();
            list<TStrList::iterator> included;
            included.push_back(contigs.begin());
            list<TItL>::iterator il = find_if(landing_spots_for_contig.begin(),landing_spots_for_contig.end(),[ic](const TItL& a){return a.first == ic;});
            for(CDBGraph::Node ls : il->second)
                landing_spots.erase(ls);
            landing_spots_for_contig.erase(il);

            for(int side = 0; side < 2; ++side) {
                while(true) {
                    int len = contig.size();
                    int scan_window = min(SCAN_WINDOW,(len-kmer_len)/2);
                    string extended_contig = contig;
                    bool connected = false;
                    for(int shift = 0; shift <= scan_window && !connected; ++shift) {
                        TKmer rkmer(contig.end()-kmer_len-shift, contig.end()-shift);
                        CDBGraph::Node rnode = m_graph.GetNode(rkmer);
                        if(!rnode)          // invalid kmer 
                            continue;
                        CDBGraphDigger::TBases step = ExtendToRightAndConnect(rnode, landing_spots, keep_jum_free_zone ? JUMP_FREE_ZONE : 0);
                        if(step.empty())
                            continue;
                        CDBGraph::Node last_node = step.back().m_node;
                        if(landing_spots.count(last_node)) {                                //connected
                            //update contig 
                            contig = contig.substr(0,len-shift);
                            for(auto& suc : step)
                                contig.push_back(suc.m_nt);
                            TContigEnd cend = landing_spots[last_node];
                            TStrList::iterator ir = get<0>(cend);
                            list<TItL>::iterator il = find_if(landing_spots_for_contig.begin(),landing_spots_for_contig.end(),[ir](const TItL& a){return a.first == ir;});
                            for(CDBGraph::Node ls : il->second)
                                landing_spots.erase(ls);
                            landing_spots_for_contig.erase(il);
                            string rcontig = *ir;
                            int landing_shift = get<1>(cend);
                            if(landing_shift < 0) {
                                ReverseComplement(rcontig.begin(), rcontig.end());
                                landing_shift = -landing_shift;
                            }
                            --landing_shift;;
                            contig += rcontig.substr(kmer_len+landing_shift);
                            included.push_back(ir);
                            connected = true;
                        } else if(contig.size()-shift+step.size() > extended_contig.size()) {  //extended
                            extended_contig = contig.substr(0,len-shift);
                            for(auto& suc : step)
                                extended_contig.push_back(suc.m_nt);                            
                        }                        
                    }
                    if(!connected) {
                        if(extended_contig.size() > contig.size())
                            contig = extended_contig;
                        break;
                    }
                }
                ReverseComplement(contig.begin(),contig.end());
            }            
            
            //clean-up
            for(TStrList::iterator ic : included) {
                contigs.erase(ic);            
            }
        }

        swap(contigs, new_contigs);        
    }

private:
    void FilterNeighbors(vector<CDBGraph::Successor>& successors, const CDBGraph::Node& previous) {
        if(m_graph.PlusFraction(previous) < 0.75 && successors.size() > 1) {
            bool has_minus = false;
            for(int j = 0; !has_minus && j < (int)successors.size(); ++j)
                has_minus = (m_graph.PlusFraction(successors[j].m_node) < 0.75);
            if(has_minus) {
                for(int j = 0; j < (int)successors.size(); ) {
                    double plusf = m_graph.PlusFraction(successors[j].m_node);
                    double minusf = 1.- plusf;
                    if(minusf < m_fraction*plusf)
                        successors.erase(successors.begin()+j);
                    else
                        ++j;
                }
            }
        }
    
        if(successors.size() > 1) {
            int abundance = 0;
            for(auto& suc : successors) {
                abundance += m_graph.Abundance(suc.m_node);
            }
            sort(successors.begin(), successors.end(), [&](const CDBGraph::Successor& a, const CDBGraph::Successor& b) {return m_graph.Abundance(a.m_node) > m_graph.Abundance(b.m_node);});
            for(int j = successors.size()-1; j > 0 && m_graph.Abundance(successors.back().m_node) <= m_fraction*abundance; --j)
                successors.pop_back();
        }
    }

    typedef tuple<TBases,size_t> TSequence;
    typedef list<TSequence> TSeqList;
    typedef unordered_map<CDBGraph::Node, TSeqList::iterator> TBranch;  // all 'leaves' will have the same length

    bool OneStepBranchExtend(TBranch& branch, TSeqList& sequences) {
        TBranch new_branch;
        for(auto& leaf : branch) {
            vector<CDBGraph::Successor> successors = m_graph.GetNodeSuccessors(leaf.first);
            FilterNeighbors(successors, leaf.first);
            if(successors.empty()) {
                sequences.erase(leaf.second);
                continue;
            }
            for(int i = successors.size()-1; i >= 0; --i) {
                TSeqList::iterator is = leaf.second;
                if(i > 0) {  // copy sequence if it is a fork           
                    sequences.push_front(*is);
                    is = sequences.begin();
                }
                TBases& bases = get<0>(*is);
                size_t& abundance = get<1>(*is);
                bases.push_back(successors[i]);
                CDBGraph::Node& node = successors[i].m_node;
                abundance += m_graph.Abundance(node);

                pair<TBranch::iterator, bool> rslt = new_branch.insert(make_pair(node, is));
                if(!rslt.second) {  // we alredy have this kmer - select larger abundance or fail (!!!!!! maybe more stringent criteria are needed)     
                    TSeqList::iterator& js = rslt.first->second;    // existing one 
                    if(abundance > get<1>(*js)) {                   // new is better    
                        sequences.erase(js);
                        js = is;
                    } else if(abundance < get<1>(*js)) {            // existing is better   
                        sequences.erase(is);                                        
                    } else {                                        // equal    
                        branch.clear();
                        return false;
                    }
                }
            }
        }

        swap(branch, new_branch);
        return true;
    }

    TBases JumpOver(vector<CDBGraph::Successor>& successors, int max_extent, int min_extent) {
        if(max_extent < m_graph.KmerLen())
            return TBases();

        TBranch extensions;
        TSeqList sequences; // storage
        for(auto& suc : successors) {
            sequences.push_front(TSequence(TBases(1,suc), m_graph.Abundance(suc.m_node)));
            extensions[suc.m_node] = sequences.begin();         
        }

        size_t MAX_BRANCH = 200;
        while(!extensions.empty() && extensions.size() < MAX_BRANCH) {
            int len = get<0>(*extensions.begin()->second).size();
            if(len == max_extent)
                break;

            if(!OneStepBranchExtend(extensions, sequences) || extensions.empty())  // ambiguos buble in a or can't extend
                return TBases();

            if(extensions.size() == 1 && len+1 >= min_extent)
                break;
        }

        if(extensions.size() == 1)
            return get<0>(*extensions.begin()->second);
        else
            return TBases();
    }

    template<typename T>  // T has to have fast count(elem)
    TBases ExtendToRightAndConnect(const CDBGraph::Node& initial_node, const T& landing_spots, int jump_free_zone) {
        CDBGraph::Node node = initial_node;
        TBases extension;
        vector<pair<int,int>> jumps;
        int kmer_len = m_graph.KmerLen();
        int max_extent = kmer_len-1+m_jump;
        m_graph.SetVisited(node);
        while(true) {
            vector<CDBGraph::Successor> successors = m_graph.GetNodeSuccessors(node);
            FilterNeighbors(successors, node);
            if(successors.empty())                     // no extensions
                break; 

            TBases step;
            if(successors.size() > 1) {                // test for bubble   
                step = JumpOver(successors, max_extent, 0);
            } else {                                   // simple extension  
                step.push_back(successors.front());
            }
            if(step.empty())                     // multiple extensions
                break;

            int step_size = step.size();
        
            CDBGraph::Node rev_node = CDBGraph::ReverseComplement(step.back().m_node);
            vector<CDBGraph::Successor> predecessors = m_graph.GetNodeSuccessors(rev_node);
            FilterNeighbors(predecessors, rev_node);
            if(predecessors.empty())                     // no extensions
                break; 

            TBases step_back;
            if(predecessors.size() > 1 || step_size > 1)
                step_back = JumpOver(predecessors, max_extent, step_size);
            else
                step_back.push_back(predecessors.front());

            int step_back_size = step_back.size();
            if(step_back_size < step_size)
                break;

            int extension_size = extension.size();
            bool good = true;
            for(int i = 0; i <= step_size-2 && good; ++i)
                good = (CDBGraph::ReverseComplement(step_back[i].m_node) == step[step_size-2-i].m_node);
            for(int i = step_size-1; i < min(step_back_size, extension_size-1+step_size) && good; ++i)
                good = (CDBGraph::ReverseComplement(step_back[i].m_node) == extension[extension_size-1+step_size-1-i].m_node);
            if(!good)
                break;
       
            int overshoot = step_back_size-step_size-extension_size;
            if(overshoot >= 0 && CDBGraph::ReverseComplement(step_back[step_back_size-1-overshoot].m_node) != initial_node) 
                break;

            if(overshoot > 0) { // overshoot
                CDBGraph::Node over_node = CDBGraph::ReverseComplement(step_back.back().m_node);
                vector<CDBGraph::Successor> oversuc = m_graph.GetNodeSuccessors(over_node);
                FilterNeighbors(oversuc, over_node);
                if(oversuc.empty())
                    break;
                TBases step_over;
                if(oversuc.size() > 1 || overshoot > 1)
                    step_over = JumpOver(oversuc, max_extent, overshoot);
                else
                    step_over.push_back(oversuc.front());

                if((int)step_over.size() < overshoot)
                    break;
                for(int i = 0; i < overshoot && good; ++i)
                    good = (CDBGraph::ReverseComplement(step_over[i].m_node) == step_back[step_back_size-2-i].m_node);
                if(!good)
                    break;
            }

            if(step_size > 1)  // jump
                jumps.push_back(make_pair(extension_size,extension_size+step_size-1));            

            bool repeat = false;
            for(auto& s : step) {
                extension.push_back(s);
                if(landing_spots.count(s.m_node)) {      // found connection
                    return extension;
                } else if(m_graph.IsVisited(s.m_node)) { // repeat 
                    extension.pop_back();
                    repeat = true;
                    break;
                }
                m_graph.SetVisited(s.m_node);
            }
            if(repeat)
                break;

            node = extension.back().m_node;
        }

        //jump free zone
        for( ; !jumps.empty(); jumps.pop_back()) {
            int ext = extension.size();
            int l = jumps.back().first;
            if(l >= ext)                          // not included
                continue;
            int r = min(ext-1,jumps.back().second);
            if(ext - jump_free_zone > r) {        // allowed
                break;
            } else {                              // trim
                for(int j = ext-1; j >= l; --j) {
                    m_graph.ClearVisited(extension[j].m_node);
                    extension.pop_back();                    
                }
            }
        }

        return extension;
    }

    //assembles the contig; changes the state of all used nodes to 'visited'
    pair<string, double> GetContigForKmer(const CDBGraph::Node& initial_node) {
        unordered_map<CDBGraph::Node, TContigEnd> landing_spots;
        TBases contigr = ExtendToRightAndConnect(initial_node, landing_spots, 0);
        TBases contigl = ExtendToRightAndConnect(CDBGraph::ReverseComplement(initial_node), landing_spots, 0);

        double abundance = 0;
        string contig;
        for(auto& base : contigl) {
            CDBGraph::Node rv = CDBGraph::ReverseComplement(base.m_node);
            abundance += m_graph.Abundance(rv);
            contig.push_back(base.m_nt);
        }
        ReverseComplement(contig.begin(), contig.end());
        abundance += m_graph.Abundance(initial_node);
        string kmer = m_graph.GetNodeSeq(initial_node);
        //        kmer = NStr::ToLower(kmer);
        contig += kmer;
        for(auto& base : contigr) {
            abundance += m_graph.Abundance(base.m_node);
            contig.push_back(base.m_nt);
        }

        return make_pair(contig, abundance);
    }

    CDBGraph& m_graph;
    double m_fraction;
    int m_jump;
};

}; // namespace
#endif /* _DeBruijn_Graph_ */