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
 *   using specifications from the ASN data definition file
 *   'twebenv.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 1.1  2001/07/27 18:33:42  grichenk
 * Initial revision
 *
 *
 * ===========================================================================
 */

#ifndef DB_ENV_HPP
#define DB_ENV_HPP


// generated includes
#include <Db_Env_.hpp>

// generated classes

class CDb_Env : public CDb_Env_Base
{
    typedef CDb_Env_Base Tparent;
public:
    // constructor
    CDb_Env(void);
    // destructor
    ~CDb_Env(void);

private:
    // Prohibit copy constructor and assignment operator
    CDb_Env(const CDb_Env& value);
    CDb_Env& operator=(const CDb_Env& value);

};



/////////////////// CDb_Env inline methods

// constructor
inline
CDb_Env::CDb_Env(void)
{
}


/////////////////// end of CDb_Env inline methods



#endif // DB_ENV_HPP
/* Original file checksum: lines: 82, chars: 2166, CRC32: 5fc2cd4c */
