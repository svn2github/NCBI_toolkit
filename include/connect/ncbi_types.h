#ifndef CONNECT___NCBI_TYPES__H
#define CONNECT___NCBI_TYPES__H

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
 * Author:  Anton Lavrentiev
 *
 * File Description:
 *   Special types for core library.
 *
 *********************************
 * Timeout:
 *    struct STimeout
 *
 * Switch:
 *    ESwitch         (on/off/default)
 *
 * Fixed-size size_t and time_t equivalents
 *    TNCBI_Size
 *    TNCBI_Time
 *       these two we need to use when mixing 32/64 bit programs
 *       which make simultaneous access to inter-process communication
 *       data areas, like shared memory segments
 *
 */

#include <stddef.h>


/** @addtogroup UtilityFunc
 *
 * @{
 */


#ifdef __cplusplus
extern "C" {
#endif


/* Timeout structure
 */
typedef struct {
    unsigned int sec;  /* seconds (truncated to the platf.-dep. max. limit) */
    unsigned int usec; /* microseconds (always truncated by mod. 1,000,000) */
} STimeout;

#ifdef __cplusplus
const STimeout *const kDefaultTimeout  = (const STimeout*)(-1);
const STimeout *const kInfiniteTimeout = (const STimeout*)( 0);
#else
#  define             kDefaultTimeout   ((const STimeout*)(-1))
#  define             kInfiniteTimeout  ((const STimeout*)( 0))
#endif /*__cplusplus*/


/* Aux. enum to set/unset/default various features
 */
typedef enum {
    eOff = 0,
    eOn,
    eDefault
} ESwitch;


/* Fixed size analogs of size_t and time_t (mainly for IPC)
 */
typedef unsigned int TNCBI_Size;
typedef unsigned int TNCBI_Time;


#ifdef __cplusplus
}  /* extern "C" */
#endif


/* @} */


/*
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.4  2003/08/25 14:36:26  lavr
 * +kDefaultTimeout, +kInfiniteTimeout
 *
 * Revision 6.3  2003/04/09 19:05:58  siyan
 * Added doxygen support
 *
 * Revision 6.2  2002/09/19 18:05:41  lavr
 * Header file guard macro changed; log moved to end
 *
 * Revision 6.1  2001/06/19 20:15:58  lavr
 * Initial revision
 *
 * ===========================================================================
 */

#endif /* CONNECT___NCBI_TYPES__H */
