#ifndef ALGO_BLAST_CORE___BLAST_PSI__H
#define ALGO_BLAST_CORE___BLAST_PSI__H

/* $Id$
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's offical duties as a United States Government employee and
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
 * Author:  Christiam Camacho
 *
 */

/** @file blast_psi.h
 * High level definitions and declarations for the PSI-BLAST API.
 */

#include <algo/blast/core/ncbi_std.h>
#include <algo/blast/core/blast_options.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Structure to describe the characteristics of a position in the multiple
 * sequence alignment data structure
 */
typedef struct PSIMsaCell {
    Uint1   letter;             /**< Preferred letter at this position */
    Boolean is_aligned;         /**< Is this letter being used? */
} PSIMsaCell;

/** Structure representing the dimensions of the multiple sequence alignment
 * data structure */
typedef struct PSIMsaDimensions {
    Uint4 query_length; /**< Length of the query */
    Uint4 num_seqs;     /**< Number of distinct sequences aligned with the 
                          query */
} PSIMsaDimensions;

/** Multiple sequence alignment (msa) data structure containing the raw data 
 * needed by the PSSM engine to create a PSSM. By convention, the first row of
 * the data field contains the query sequence */
typedef struct PSIMsa {
    PSIMsaDimensions*   dimensions; /**< dimensions of the msa */
    PSIMsaCell**        data;       /**< msa data */
} PSIMsa;

/** Allocates and initializes the multiple sequence alignment data structure
 * for use as input to the PSSM engine.
 * @param dimensions dimensions of multiple sequence alignment data structure
 * to allocate [in]
 * @return allocated PSIMsa structure or NULL if out of memory.
 */
PSIMsa*
PSIMsaNew(const PSIMsaDimensions* dimensions);

/** Deallocates the PSIMsa structure
 * @param msa multiple sequence alignment structure to deallocate [in]
 * @return NULL
 */
PSIMsa*
PSIMsaFree(PSIMsa* msa);

/** This is the main return value from the PSSM engine */
typedef struct PSIMatrix {
    Uint4   ncols;      /**< Number of columns in PSSM (query_length) */
    Uint4   nrows;      /**< Number of rows in PSSM (alphabet_size) */
    int**   pssm;       /**< Position-specific score matrix */
    double  lambda;     /**< Lambda Karlin-Altschul parameter */
    double  kappa;      /**< Kappa Karlin-Altschul parameter */
    double  h;          /**< H Karlin-Altschul parameter */
} PSIMatrix;

/** Allocates a new PSIMatrix structure 
 * @param query_length number of columns allocated for the PSSM [in]
 * @param alphabet_size number of rows allocated for the PSSM [in]
 * @return pointer to allocated PSIMatrix structure or NULL if out of memory
 */
PSIMatrix*
PSIMatrixNew(Uint4 query_length, Uint4 alphabet_size);

/** Deallocates the PSIMatrix structure passed in.
 * @param matrix structure to deallocate [in]
 * @return NULL
 */
PSIMatrix*
PSIMatrixFree(PSIMatrix* matrix);

/** Structure to allow requesting various diagnostics data to be collected by
 * PSSM engine */
typedef struct PSIDiagnosticsRequest {
    Boolean information_content;
    Boolean residue_frequencies;
    Boolean raw_residue_counts;
    Boolean sequence_weights;
    Boolean gapless_column_weights;
} PSIDiagnosticsRequest;

/** This structure contains the diagnostics information requested using the
 * PSIDiagnosticsRequest structure */
typedef struct PSIDiagnosticsResponse {
    double* information_content;           /**< position information content
                                             (dimensions->query_length 
                                             elements)*/
    double** residue_frequencies;          /**< PSSM's residue frequencies
                                             (Dimensions are
                                             dimensions->query_length by
                                             alphabet_size) */
    Uint4** raw_residue_counts;            /**< Raw residue counts at each
                                             position of the query (Dimensions
                                             are dimensions->query_length by
                                             alphabet_size) */
    double* sequence_weights;              /**< Normalized sequence weights
                                             (dimensions->num_seqs+1 elements)*/
    double* gapless_column_weights;        /**< Weights for columns without
                                             gaps (dimensions->query_length
                                             elements) */
    PSIMsaDimensions* dimensions;          /**< Dimensions for matrices */
    Uint4 alphabet_size;                   /**< Specifies length of alphabet */
} PSIDiagnosticsResponse;

/** Allocates a new PSI-BLAST diagnostics structure based on which fields of
 * the PSIDiagnosticsRequest structure are TRUE. Note: this is declared
 * here for consistency - this does not need to be called by client code of
 * this API, it is called in the PSICreatePssm* functions to allocate the 
 * diagnostics response structure.
 * @param msa_dimensions dimensions of the multiple alignment [in]
 * @param alphabet_size length of the alphabet [in]
 * @param request diagnostics to retrieve from PSSM engine [in]
 * @return pointer to allocated PSIDiagnosticsResponse or NULL if dimensions or
 * request are NULL
 */
PSIDiagnosticsResponse*
PSIDiagnosticsResponseNew(const PSIMsaDimensions* dimensions,
                          Uint4 alphabet_size, 
                          const PSIDiagnosticsRequest* request);

/** Deallocates the PSIDiagnosticsResponse structure passed in.
 * @param diags structure to deallocate [in]
 * @return NULL
 */
PSIDiagnosticsResponse*
PSIDiagnosticsResponseFree(PSIDiagnosticsResponse* diags);

/****************************************************************************/

/** Main entry point to core PSSM engine to calculate the PSSM.
 * @param msap multiple sequence alignment data structure [in]
 * @param options options to the PSSM engine [in]
 * @param sbp BLAST score block structure [in|out]
 * @param pssm PSSM and statistical information (the latter is also returned 
 * in the sbp->kbp_gap_psi[0]) 
 * @return 0 on success, else failure (FIXME)
 */
int
PSICreatePssm(const PSIMsa* msap,
              const PSIBlastOptions* options,
              BlastScoreBlk* sbp,
              PSIMatrix** pssm);

/** Main entry point to core PSSM engine which allows to request diagnostics
 * information.
 * @param msap multiple sequence alignment data structure [in]
 * @param options options to the PSSM engine [in]
 * @param sbp BLAST score block structure [in|out]
 * @param request diagnostics information request [in]
 * @param pssm PSSM and statistical information (the latter is also returned 
 * in the sbp->kbp_gap_psi[0]) 
 * @param diagnostics diagnostics information response, expects a pointer to an
 * uninitialized structure which will be populated with data requested in
 * requests [in]
 * @return 0 on success, else failure (FIXME)
 */
int
PSICreatePssmWithDiagnostics(const PSIMsa* msap,                    /* [in] */
                             const PSIBlastOptions* options,        /* [in] */
                             BlastScoreBlk* sbp,                    /* [in] */
                             const PSIDiagnosticsRequest* request,  /* [in] */
                             PSIMatrix** pssm,                      /* [out] */
                             PSIDiagnosticsResponse** diagnostics); /* [out] */
#ifdef __cplusplus
}
#endif

#endif /* !ALGO_BLAST_CORE__BLAST_PSI__H */

