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
* ===========================================================================*/

/*****************************************************************************

File name: blast_filter.h

Author: Ilya Dondoshansky

Contents: SEG filtering functions.

Detailed Contents: 

******************************************************************************
 * $Revision$
 * */
#ifndef __BLAST_SEG__
#define __BLAST_SEG__

#ifdef __cplusplus
extern "C" {
#endif

#include <blast_def.h>

#define AA20    2

#define LN20    2.9957322735539909

#define CHAR_SET 128

/*--------------------------------------------------------------(structs)---*/

typedef struct segm
  {
   int begin;
   int end;
   struct segm *next;
  } Seg, PNTR SegPtr;

typedef struct alpha
  {
   Int4 alphabet;
   Int4 alphasize;
   FloatHi lnalphasize;
   Int4Ptr alphaindex;
   BytePtr alphaflag;
   CharPtr alphachar;
  } Alpha, PNTR AlphaPtr;

typedef struct segparams
  {
   Int4 window;
   FloatHi locut;
   FloatHi hicut;
   Int4 period;
   Int4 hilenmin;
   Boolean overlaps;	/* merge overlapping pieces if TRUE. */
   Int4 maxtrim;
   Int4 maxbogus;
   AlphaPtr palpha;
  } SegParameters, PNTR SegParametersPtr;

typedef struct sequence
  {
   struct sequence PNTR parent;
   CharPtr seq;
   AlphaPtr palpha;
   Int4 start;
   Int4 length;
   Int4 bogus;
   Boolean punctuation;
   Int4 PNTR composition;
   Int4 PNTR state;
   FloatHi entropy;
  } Sequence, PNTR SequencePtr;

SegParametersPtr SegParametersNewAa (void);
void SegParametersFree(SegParametersPtr sparamsp);

Int2 SeqBufferSeg (Uint1Ptr sequence, Int4 length,
                    Int2 level, Int2 window, Int2 minwin, Int2 linker,
                    BlastSeqLocPtr PNTR seg_loc);

#ifdef __cplusplus
}
#endif
#endif /* !__BLAST_FILTER__ */
