#ifndef CONNECT___NCBI_HEAPMGR__H
#define CONNECT___NCBI_HEAPMGR__H

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
 * Author:  Anton Lavrentiev, Denis Vakatov
 *
 * File Description:
 *   Simple heap manager with a primitive garbage collection
 *
 */

#include <connect/connect_export.h>
#include <connect/ncbi_types.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Heap handle
 */
struct SHEAP_tag;
typedef struct SHEAP_tag* HEAP;


/* Header of a heap block
 */
typedef struct {
    unsigned int flag;  /* (short)flag == 0 if block is vacant              */
    TNCBI_Size   size;  /* size of the block (including the block header)   */
} SHEAP_Block;


/* Callback to expand the heap (a la 'realloc').
 * NOTE: the returned address must be aligned on a 'double' boundary!
 *
 *   old_base  |  new_size  |  Expected result
 * ------------+------------+--------------------------------------------------
 *   non-NULL  |     0      | Deallocate old_base and return 0
 *   non-NULL  |  non-zero  | Reallocate to the requested size, return new base
 *      0      |  non-zero  | Allocate (newly) and return base
 *      0      |     0      | Do nothing, return 0
 * ------------+------------+--------------------------------------------------
 * Note that reallocation can request either to expand or to shrink the
 * heap extent. When (re-)allocation fails, the callback should return 0.
 * When expected to return 0, this callback has to always do so.
 */
typedef char* (*FHEAP_Expand)
(char*      old_base,  /* current base of the heap to be expanded           */
 TNCBI_Size new_size   /* requested new heap size (zero to deallocate heap) */
 );


/* Create new heap.
 * NOTE: the initial heap base must be aligned on a 'double' boundary!
 */
extern NCBI_XCONNECT_EXPORT HEAP HEAP_Create
(char*        base,        /* initial heap base (use "expand" if NULL) */
 TNCBI_Size   size,        /* initial heap size                        */
 TNCBI_Size   chunk_size,  /* minimal increment size                   */
 FHEAP_Expand expand       /* NULL if not expandable                   */
 );


/* Attach to an already existing heap (in read-only mode).
 */
extern NCBI_XCONNECT_EXPORT HEAP HEAP_Attach
(char* base                /* base of the heap to attach to */
 );


/* Allocate a new block of memory in the heap.
 */
extern NCBI_XCONNECT_EXPORT SHEAP_Block* HEAP_Alloc
(HEAP       heap,          /* heap handle                       */
 TNCBI_Size size           /* data size of the block to contain */
 );


/* Deallocate block pointed by "block_ptr".
 */
extern NCBI_XCONNECT_EXPORT void HEAP_Free
(HEAP         heap,        /* heap handle         */
 SHEAP_Block* block_ptr    /* block to deallocate */
 );


/* Iterate through the heap blocks.
 * Return pointer to the block following block "prev_block".
 * Return NULL if "prev_block" is the last block of the heap.
 */
extern NCBI_XCONNECT_EXPORT SHEAP_Block* HEAP_Walk
(HEAP               heap,  /* heap handle                                  */
 const SHEAP_Block* prev   /* (if 0, then get the first block of the heap) */
 );


/* Make a snapshot of a given heap. Return a read-only heap
 * (like one after HEAP_Attach), which must be freed by a call to
 * either HEAP_Detach or HEAP_Destroy when no longer needed.
 * If a non-zero number provided (serial number) it is stored
 * in the heap descriptor (zero number is always changed into 1).
 */
extern NCBI_XCONNECT_EXPORT HEAP HEAP_CopySerial(HEAP orig, int serial);

#define HEAP_Copy(orig) HEAP_CopySerial(orig, 0)


/* Detach heap (previously attached by HEAP_Attach).
 */
extern NCBI_XCONNECT_EXPORT void HEAP_Detach(HEAP heap);


/* Destroy heap (previously created by HEAP_Create).
 */
extern NCBI_XCONNECT_EXPORT void HEAP_Destroy(HEAP heap);


/* Get base address of the heap
 */
extern NCBI_XCONNECT_EXPORT char* HEAP_Base(const HEAP heap);


/* Get the extent of the heap
 */
extern NCBI_XCONNECT_EXPORT size_t HEAP_Size(const HEAP heap);


/* Get non-zero serial number of the heap.
 * Return 0 if HEAP is passed as 0, or the heap is not a copy but original.
 */
extern NCBI_XCONNECT_EXPORT int HEAP_Serial(const HEAP heap);


#ifdef __cplusplus
} /* extern "C" */
#endif


/*
 * --------------------------------------------------------------------------
 * $Log$
 * Revision 6.13  2003/01/08 01:59:32  lavr
 * DLL-ize CONNECT library for MSVC (add NCBI_XCONNECT_EXPORT)
 *
 * Revision 6.12  2002/09/25 20:08:43  lavr
 * Added table to explain expand callback inputs and outputs
 *
 * Revision 6.11  2002/09/19 18:00:58  lavr
 * Header file guard macro changed; log moved to the end
 *
 * Revision 6.10  2002/04/13 06:33:22  lavr
 * +HEAP_Base(), +HEAP_Size(), +HEAP_Serial(), new HEAP_CopySerial()
 *
 * Revision 6.9  2001/07/03 20:23:46  lavr
 * Added function: HEAP_Copy()
 *
 * Revision 6.8  2001/06/19 20:16:19  lavr
 * Added #include <connect/ncbi_types.h>
 *
 * Revision 6.7  2001/06/19 19:09:35  lavr
 * Type change: size_t -> TNCBI_Size; time_t -> TNCBI_Time
 *
 * Revision 6.6  2000/10/05 21:25:45  lavr
 * ncbiconf.h removed
 *
 * Revision 6.5  2000/10/05 21:09:52  lavr
 * ncbiconf.h included
 *
 * Revision 6.4  2000/05/23 21:41:05  lavr
 * Alignment changed to 'double'
 *
 * Revision 6.3  2000/05/12 18:28:40  lavr
 * First working revision
 *
 * ==========================================================================
 */

#endif /* CONNECT___NCBI_HEAPMGR__H */
