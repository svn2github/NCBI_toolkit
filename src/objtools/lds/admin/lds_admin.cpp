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
 * Author: Anatoliy Kuznetsov
 *
 * File Description:  LDS database maintanance implementation.
 *
 */

#include <objtools/lds/lds_admin.hpp>
#include <objtools/lds/lds_files.hpp>
#include <objtools/lds/lds_object.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

CLDS_Management::CLDS_Management(CLDS_Database& db)
: m_lds_db(db)
{}


void CLDS_Management::SyncWithDir(const string& dir_name)
{
    CLDS_Set files_deleted;
    CLDS_Set files_updated;
    SLDS_TablesCollection& db = m_lds_db.GetTables();

    CLDS_File fl(db);
    fl.SyncWithDir(dir_name, &files_deleted, &files_updated);

    CLDS_Set objects_deleted;
    CLDS_Set annotations_deleted;

    CLDS_Object obj(db, m_lds_db.GetObjTypeMap());
    obj.DeleteCascadeFiles(files_deleted, &objects_deleted, &annotations_deleted);
    obj.UpdateCascadeFiles(files_updated);
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.1  2003/06/16 14:55:00  kuznets
 * lds splitted into "lds" and "lds_admin"
 *
 * ===========================================================================
*/
