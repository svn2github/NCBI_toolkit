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
 *   using the following specifications:
 *   'omssa.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/omssa/MSSearchSettings.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CMSSearchSettings::~CMSSearchSettings(void)
{
}


///
/// Validate Search Settings
/// returns 0 if OK, 1 if not
/// Error contains explanation
///
int CMSSearchSettings::Validate(std::list<std::string> & Error) const
{
    int retval(0);


    if(CanGetSearchtype()) {
	if(GetSearchtype() < 0 || GetSearchtype() > 1 ) {
	    Error.push_back("Invalid search type");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Search type missing");
	retval = 1;
    }
	

    if(CanGetPeptol()) {
	if( GetPeptol() < 0) {
	    Error.push_back("Precursor mass tolerance less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Precursor mass tolerance missing");
	retval = 1;
    }


    if(CanGetMsmstol()) {
	if(GetMsmstol() < 0) {
	    Error.push_back("Product mass tolerance less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Product mass tolerance missing");
	retval = 1;
    }


    if(CanGetCutlo()) {
	if(GetCutlo() < 0 || GetCutlo() > 1) {
	    Error.push_back("Low cut value out of range");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Low cut value missing");
	retval = 1;
    }


    if(CanGetCuthi()) {
	if(GetCuthi() < 0 || GetCuthi() > 1) {
	    Error.push_back("Hi cut value out of range");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Hi cut value missing");
	retval = 1;
    }


    if(CanGetCuthi() && CanGetCutlo()) {
	if(GetCutlo() > GetCuthi()) {
	    Error.push_back("Lo cut exceeds Hi cut value");
	    retval = 1;
	}
	else if (CanGetCutinc() && GetCutinc() <= 0) {
	    Error.push_back("Cut increment too small");
	    retval = 1;
	}
    }

    if(CanGetCutinc()) {
	if(GetCutinc() < 0) {
	    Error.push_back("Cut increment less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Cut increment missing");
	retval = 1;
    }


    if(CanGetSinglewin()) {
	if(GetSinglewin() < 0) {
	    Error.push_back("Single win size less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Single win size missing");
	retval = 1;
    }


    if(CanGetDoublewin()) {
	if(GetDoublewin() < 0) {
	    Error.push_back("Double win size less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Double win size missing");
	retval = 1;
    }


    if(CanGetSinglenum()) {
	if(GetSinglenum() < 0) {
	    Error.push_back("Single num less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Single num missing");
	retval = 1;
    }


    if(CanGetDoublenum()) {
	if(GetDoublenum() < 0) {
	    Error.push_back("Double num less than 0");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Double num missing");
	retval = 1;
    }


    if(CanGetFixed()) {
	TFixed::const_iterator i;
	for(i = GetFixed().begin(); i != GetFixed().end(); i++)
	    if( *i < 0 ) {
		Error.push_back("Unknown fixed mod");
		retval = 1;
	    }
    }


    if(CanGetVariable()) {
	TVariable::const_iterator i;
	for(i = GetVariable().begin(); i != GetVariable().end(); i++)
	    if( *i < 0 ) {
		Error.push_back("Unknown variable mod");
		retval = 1;
	    }
    }


    if(CanGetEnzyme()) {
	if(GetEnzyme() < 0) {
	    Error.push_back("Unknown enzyme");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Enzyme missing");
	retval = 1;
    }


    if(CanGetMissedcleave()) {
	if(GetMissedcleave() < 0) {
	    Error.push_back("Invalid missed cleavage value");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Missed cleavage value missing");
	retval = 1;
    }


    if(CanGetHitlistlen()) {
	if(GetHitlistlen() < 1) {
	    Error.push_back("Invalid hit list length");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Hit list length missing");
	retval = 1;
    }


    if(CanGetDb()) {
	if(GetDb().empty()) {
	    Error.push_back("Empty BLAST library name");
	    retval = 1;
	}
    }
    else {
	Error.push_back("BLAST library name missing");
	retval = 1;
    }


    if(CanGetTophitnum()) {
	if(GetTophitnum() < 1) {
	    Error.push_back("Less than one top hit needed");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Missing top hit value");
	retval = 1;
    }


    if(CanGetMinhit()) {
	if(GetMinhit() < 1) {
	    Error.push_back("Less than one match need for hit");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Missing minimum number of matches");
	retval = 1;
    }


    if(CanGetMinspectra()) {
	if(GetMinspectra() < 2) {
	    Error.push_back("Less than two peaks required in input spectra");
	    retval = 1;
	}
    }
    else {
	Error.push_back("Missing minimum peaks value for valid spectra");
	retval = 1;
    }


    if(CanGetScale()) {
	if(GetScale() < 1) {
	    Error.push_back("m/z scaling value less than one");
	    retval = 1;
	}
    }
    else {
	Error.push_back("missing m/z scaling value");
	retval = 1;
    }

    // optional arguments

    if(IsSetTaxids()) {
	TTaxids::const_iterator i;
	for(i = GetTaxids().begin(); i != GetTaxids().end(); i++)
	    if( *i < 0 ) {
		Error.push_back("taxid < 0");
		retval = 1;
	    }
    }

    if(IsSetChargehandling()) {
	if( GetChargehandling() < 0 || GetChargehandling() > 1 ) {
	    Error.push_back("unknown chargehandling option");
	    retval = 1;
	}
    }
    
    return retval;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2004/06/08 19:46:20  lewisg
* input validation, additional user settable parameters
*
*
* ===========================================================================
*/
/* Original file checksum: lines: 64, chars: 1885, CRC32: ffa5fdc2 */
