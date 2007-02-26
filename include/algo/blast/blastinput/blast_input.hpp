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
 * Author:  Jason Papadopoulos
 *
 */

/** @file algo/blast/blastinput/blast_input.hpp
 * Interface for converting sources of sequence data into
 * blast sequence input
 */

#ifndef ALGO_BLAST_BLASTINPUT___BLAST_INPUT__HPP
#define ALGO_BLAST_BLASTINPUT___BLAST_INPUT__HPP

#include <corelib/ncbistd.hpp>
#include <algo/blast/api/sseqloc.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

/// Class that centralizes the configuration data for
/// sequences to be converted
///
class NCBI_XBLAST_EXPORT CBlastInputConfig {

public:

    /// Constructor
    /// @param strand All SeqLoc types will have this strand assigned;
    ///             If set to 'other', the strand will be set to 'unknown'
    ///             for protein sequences and 'both' for nucleotide [in]
    /// @param lowercase If true, lowercase mask locations are generated
    ///                 for all input sequences [in]
    /// @param believe_defline If true, all sequences ID's are parsed;
    ///                 otherwise all sequences receive a local ID set
    ///                 to a monotonically increasing count value [in]
    /// @param range Range restriction for all sequences (default means no
    //                  restriction) [in]
    CBlastInputConfig(objects::ENa_strand strand = objects::eNa_strand_other,
                     bool lowercase = false,
                     bool believe_defline = false,
                     TSeqRange range = TSeqRange());

    /// Destructor
    ///
    ~CBlastInputConfig() {}

    /// Set the strand to a specified value
    /// @param strand The strand value
    ///
    void SetStrand(objects::ENa_strand strand) { m_Strand = strand; }

    /// Retrieve the current strand value
    /// @return the strand
    objects::ENa_strand GetStrand() const { return m_Strand; }

    /// Turn lowercase masking on/off
    /// @param mask boolean to toggle lowercase masking
    ///
    void SetLowercaseMask(bool mask) { m_LowerCaseMask = mask; }

    /// Retrieve lowercase mask status
    /// @return boolean to toggle lowercase masking
    ///
    bool GetLowercaseMask() const { return m_LowerCaseMask; }

    /// Turn parsing of sequence IDs on/off
    /// @param believe boolean to toggle parsing of seq IDs
    ///
    void SetBelieveDeflines(bool believe) { m_BelieveDeflines = believe; }

    /// Retrieve current sequence ID parsing status
    /// @return boolean to toggle parsing of seq IDs
    ///
    bool GetBelieveDeflines() const { return m_BelieveDeflines; }

    /// Set range for all sequences
    /// @param r range to use [in]
    void SetRange(const TSeqRange& r) { m_Range = r; }
    TSeqRange& SetRange(void) { return m_Range; }

    /// Get range for all sequences
    /// @return range specified for all sequences
    TSeqRange GetRange() const { return m_Range; }

private:
    objects::ENa_strand m_Strand;  ///< strand to assign to sequences
    bool m_LowerCaseMask;          ///< whether to save lowercase mask locs
    bool m_BelieveDeflines;        ///< whether to parse sequence IDs
    TSeqRange m_Range;             ///< sequence range
};


/// Base class representing a source of biological sequences
///
class NCBI_XBLAST_EXPORT CBlastInputSource : public CObject
{
public:
    /// Constructor
    /// @param objmgr Object Manager instance
    ///
    CBlastInputSource(objects::CObjectManager& objmgr);

    /// Retrieve the scope used by the sequences in this object
    CRef<objects::CScope> GetScope() { return m_Scope; }

    /// Destructor
    ///
    virtual ~CBlastInputSource() {}

    /// Retrieve a single sequence (in an SSeqLoc container)
    ///
    virtual SSeqLoc GetNextSSeqLoc() = 0;

    /// Retrieve a single sequence (in a CBlastSearchQuery container)
    ///
    virtual CRef<CBlastSearchQuery> GetNextSequence() = 0;

    /// Signal whether there are any unread sequence left
    /// @return true if no unread sequences remaining
    ///
    virtual bool End() = 0;

protected:
    objects::CObjectManager& m_ObjMgr;  ///< object manager instance
    CRef<objects::CScope> m_Scope;      ///< scope instance (local to object)
};


/// Generalized converter from an abstract source of
/// biological sequence data to collections of blast input
///
class NCBI_XBLAST_EXPORT CBlastInput : public CObject
{
public:
    /// Constructor
    /// @param source Pointer to abstract source of sequences
    /// @param batch_size A hint specifying how many letters should
    ///               be in a batch of converted sequences
    ///
    CBlastInput(CBlastInputSource *source, int batch_size = kMax_Int)
        : m_Source(source), m_BatchSize(batch_size) {}

    /// Destructor
    ///
    ~CBlastInput() {}

    /// Read and convert all the sequences from the source
    /// @return The converted sequences
    ///
    TSeqLocVector GetAllSeqLocs();

    /// Read and convert all the sequences from the source
    /// @return The converted sequences
    ///
    CRef<CBlastQueryVector> GetAllSeqs();

    /// Read and convert the next batch of sequences
    /// @return The next batch of sequence. The size of the batch is
    ///        either all remaining sequences, or the size of sufficiently
    ///        many whole sequences whose combined size exceeds m_BatchSize,
    ///        whichever is smaller
    ///
    TSeqLocVector GetNextSeqLocBatch();

    /// Read and convert the next batch of sequences
    /// @return The next batch of sequence. The size of the batch is
    ///        either all remaining sequences, or the size of sufficiently
    ///        many whole sequences whose combined size exceeds m_BatchSize,
    ///        whichever is smaller
    ///
    CRef<CBlastQueryVector> GetNextSeqBatch();

    /// Set the target size of a batch of sequences
    /// @param batch_size The desired total size of all the 
    ///                 sequences in future batches
    ///                  
    void SetBatchSize(TSeqPos batch_size) { m_BatchSize = batch_size; }

    /// Retrieve the target size of a batch of sequences
    /// @return The current batch size
    ///                  
    TSeqPos GetBatchSize() const { return m_BatchSize; }

private:
    CBlastInputSource *m_Source;  ///< pointer to source of sequences
    TSeqPos m_BatchSize;          ///< total size of one block of sequences
};


END_SCOPE(blast)
END_NCBI_SCOPE

#endif  /* ALGO_BLAST_BLASTINPUT___BLAST_INPUT__HPP */

/*---------------------------------------------------------------------
 * $Log$
 * Revision 1.5  2006/10/03 19:42:14  ivanov
 * Added NCBI_XBLAST_EXPORT export specifier.
 * MSVC: blastinput added to ncbi_algo.dll.
 *
 * Revision 1.4  2006/10/03 19:12:24  ivanov
 * Remove NCBI_BLAST_EXPORT from CBlastInput declaration,
 * because on MSVC blastinput builds as static lib, not dll.
 *
 * Revision 1.3  2006/09/26 21:45:38  papadopo
 * add to blast scope; add CVS log
 *
 *-------------------------------------------------------------------*/
