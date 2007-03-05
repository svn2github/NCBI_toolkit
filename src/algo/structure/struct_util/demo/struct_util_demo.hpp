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
*      Test/demo for utility alignment algorithms
*
* ===========================================================================
*/

#ifndef STRUCT_UTIL_DEMO__HPP
#define STRUCT_UTIL_DEMO__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiapp.hpp>


BEGIN_SCOPE(struct_util)

// class for standalone application
class SUApp : public ncbi::CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};

END_SCOPE(struct_util)

#endif // STRUCT_UTIL_DEMO__HPP
