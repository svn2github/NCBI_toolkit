#ifndef CTOOLS___CTOOLS__H
#define CTOOLS___CTOOLS__H

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
 *   A bridge between C and C++ Toolkits (C->C++, for use in C only)
 *
 */


/** @addtogroup CToolsBridge
 *
 * @{
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
   

/* Reroute C Toolkit error posting into C++ Toolkit error posting facility.
 */
void SetupCToolkitErrPost(void);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


/* @} */


/*
 * --------------------------------------------------------------------------
 * $Log$
 * Revision 1.5  2005/07/06 19:43:08  lavr
 * Note the file is intended for use in pure C code
 *
 * Revision 1.4  2003/11/13 15:59:44  lavr
 * Guard macro changed; log moved to end
 *
 * Revision 1.3  2003/04/11 17:46:32  siyan
 * Added doxygen support
 *
 * Revision 1.2  2001/02/12 15:34:34  lavr
 * Extra semicolon removed
 *
 * Revision 1.1  2001/02/09 17:31:27  lavr
 * Initial revision
 *
 * ==========================================================================
 */

#endif  /* CTOOLS___CTOOLS__H */
