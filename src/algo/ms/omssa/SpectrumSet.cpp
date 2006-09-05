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
 * Author:  Lewis Y. Geer
 *
 * File Description:
 *   Contains code for reading in spectrum data sets.
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the data definition file
 *   'omssa.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbi_limits.h>
#include <corelib/ncbistre.hpp>
#include <corelib/ncbistr.hpp>
#include <util/regexp.hpp>
#include <objects/omssa/MSSpectrum.hpp>


// generated includes
#include "SpectrumSet.hpp"

// added includes
#include "msms.hpp"
#include <math.h>

// generated classes
BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::


// getline that ignores extra eol char for windows
static CNcbiIstream& MyGetline(CNcbiIstream& is, string& str)
{
    // mac is \x0d
    // win is \x0d\x0a
    // unix is \x0a
    NcbiGetline(is, str, "\x0d\x0a");
    return is;
}


/////////////////////////////////////////////////////////////////////////////
//
//  CSpectrumSet::
//


///
/// wrapper for various file loaders
///

int CSpectrumSet::LoadFile(const EMSSpectrumFileType FileType, CNcbiIstream& DTA, int Max)
{
    switch (FileType) {
    case eMSSpectrumFileType_dta:
        return LoadDTA(DTA);
        break;
    case eMSSpectrumFileType_dtablank:
        return LoadMultBlankLineDTA(DTA, Max);
        break;
    case eMSSpectrumFileType_dtaxml:
        return LoadMultDTA(DTA, Max);
        break;
    case eMSSpectrumFileType_pkl:
        return LoadMultBlankLineDTA(DTA, Max, true);
        break;
    case eMSSpectrumFileType_mgf:
        return LoadMGF(DTA, Max);
        break;
    case eMSSpectrumFileType_asc:
    case eMSSpectrumFileType_pks:
    case eMSSpectrumFileType_sciex:
    case eMSSpectrumFileType_unknown:
    case eMSSpectrumFileType_oms:
    case eMSSpectrumFileType_omx:
    default:
        break;
    }
    return 1;  // not supported
}

///
/// load multiple dta's in xml-like format
///
int CSpectrumSet::LoadMultDTA(CNcbiIstream& DTA, int Max)
{   
    int iIndex(-1); // the spectrum index
    int Count(0);  // total number of spectra
    string Line;
    //    double dummy;
    bool GotOne(false);  // has a spectrum been read?
    try {
        //       DTA.exceptions(ios_base::failbit | ios_base::badbit);
        do {
            bool GotThisOne(false);  // was the most recent spectrum read?
            do {
                MyGetline(DTA, Line);
            } while (NStr::Compare(Line, 0, 4, "<dta") != 0 && DTA && !DTA.eof());
            if (!DTA || DTA.eof()) {
                if (GotOne) {
                    ERR_POST(Info << "LoadMultDTA: end of xml");
                    return 0;
                }
                else {
                    ERR_POST(Info << "LoadMultDTA: bad end of xml");
                    return 1;
                }
            }
//            GotOne = true;
            Count++;
            if (Max > 0 && Count > Max) {
                ERR_POST(Info << "LoadMultDTA: too many spectra in xml");
                return -1;  // too many
            }

            CRef <CMSSpectrum> MySpectrum(new CMSSpectrum);
            CRegexp RxpGetNum("\\sid\\s*=\\s*(\"(\\S+)\"|(\\S+)\b)");
            string Match;
            if ((Match = RxpGetNum.GetMatch(Line.c_str(), 0, 2)) != "" ||
                (Match = RxpGetNum.GetMatch(Line.c_str(), 0, 3)) != "") {
                MySpectrum->SetNumber(NStr::StringToInt(Match));
            } else {
                MySpectrum->SetNumber(iIndex);
                iIndex--;
            }

            CRegexp RxpGetName("\\sname\\s*=\\s*(\"(\\S+)\"|(\\S+)\b)");
            if ((Match = RxpGetName.GetMatch(Line.c_str(), 0, 2)) != "" ||
                (Match = RxpGetName.GetMatch(Line.c_str(), 0, 3)) != "") {
                MySpectrum->SetIds().push_back(NStr::PrintableString(Match));
            }
            {
                do {
                    MyGetline(DTA, Line);
                } while (Line.size() < 3 && !DTA.eof()); // skip blank lines
                
                CNcbiIstrstream istr(Line.c_str());
    
                if(!GetDTAHeader(istr, MySpectrum)) {
                    ERR_POST(Info << "LoadMultDTA: not able to get header");
                    return 1;
                }
            }
            MyGetline(DTA, Line);
            TInputPeaks InputPeaks;

            while (NStr::Compare(Line, 0, 5, "</dta") != 0) {
                CNcbiIstrstream istr(Line.c_str());
                if (!GetDTABody(istr, InputPeaks)) 
                    break;
                GotOne = true;
                GotThisOne = true;
                MyGetline(DTA, Line);
            } 
            if(GotThisOne) {
                Peaks2Spectrum(InputPeaks, MySpectrum);
                Set().push_back(MySpectrum);
            }
        } while (DTA && !DTA.eof());

    if (!GotOne) {
        ERR_POST(Info << "LoadMultDTA: didn't get one");
        return 1;
    }
        
    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultDTA: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultDTA: " );
        throw;
    }
    return 0;
}

///
/// load multiple dta's separated by a blank line
///
int CSpectrumSet::LoadMultBlankLineDTA(CNcbiIstream& DTA, int Max, bool isPKL)
{   
    int iIndex(0); // the spectrum index
    int Count(0);  // total number of spectra
    string Line;
    bool GotOne(false);  // has a spectrum been read?
    try {
//        DTA.exceptions(ios_base::failbit | ios_base::badbit);
        do {
            bool GotThisOne(false);  // was the most recent spectrum read?
            Count++;
            if (Max > 0 && Count > Max) 
                return -1;  // too many

            CRef <CMSSpectrum> MySpectrum(new CMSSpectrum);
            MySpectrum->SetNumber(iIndex);
            iIndex++;
            {
                do {
                    MyGetline(DTA, Line);
                } while (Line.size() < 3 && !DTA.eof()); // skip blank lines
                
                CNcbiIstrstream istr(Line.c_str());

                if (!GetDTAHeader(istr, MySpectrum, isPKL)) 
                    if(GotOne) 
                        break;  // probably the end of the file
                    else
                        return 1;
            }
            MyGetline(DTA, Line);

            if (!DTA || DTA.eof()) {
                if (GotOne) 
                    return 0;
                else 
                    return 1;
            }

            TInputPeaks InputPeaks;
            // loop while line is long (the > 1 deals with eol issues)
            while (Line.size() > 1) {
                CNcbiIstrstream istr(Line.c_str());
                if (!GetDTABody(istr, InputPeaks)) 
                    break;
                GotOne = true;
                GotThisOne = true;
                MyGetline(DTA, Line);
            } 
            if(GotThisOne) {
                Peaks2Spectrum(InputPeaks, MySpectrum);
                Set().push_back(MySpectrum);
            }
        } while (DTA && !DTA.eof());

        if (!GotOne) 
            return 1;

    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultBlankLineDTA: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMultBlankLineDTA: " );
        throw;
    }

    return 0;
}


///
///  Read in the header of a DTA file
///
bool CSpectrumSet::GetDTAHeader(CNcbiIstream& DTA, CRef <CMSSpectrum>& MySpectrum,
                                bool isPKL)
{
    double dummy(0.0L);
    double precursor(0.0L);

    if(!(DTA >> precursor) || precursor < 0) {
        return false;
    }

    // read in pkl intensity
    if(isPKL) {
        if(!(DTA >> dummy) || dummy < 0) {
            return false;
        }
    }

    // read in charge
    if(!(DTA >> dummy) || dummy < 0) {
        return false;
    }
    MySpectrum->SetCharge().push_back(static_cast <int> (dummy)); 

    // correct dta MH+ to true precursor
    if(!isPKL) {
        precursor = (precursor + (dummy - 1)*kProton ) / dummy;
    }
    MySpectrum->SetPrecursormz(MSSCALE2INT(precursor));

    return true;
}


//! Convert peak list to spectrum
/*!
    \param InputPeaks list of the input m/z and intensity values
    \param MySpectrum the spectrum to receive the scaled input
    \return success
*/

bool CSpectrumSet::Peaks2Spectrum(const TInputPeaks& InputPeaks, CRef <CMSSpectrum>& MySpectrum) const
{
    unsigned i;

    float MaxI(0.0);

    // find max peak
    for (i = 0; i < InputPeaks.size(); ++i) {
        if(InputPeaks[i].Intensity > MaxI) MaxI = InputPeaks[i].Intensity;
    }

    // set scale
    double Scale;
    if(MaxI > 0.0) Scale = 1000000000.0/MaxI;
    else Scale = 1.0;
    // normalize it to a power of 10
    Scale = pow(10.0, floor(log10(Scale)));
    MySpectrum->SetIscale(Scale);

    // loop thru
    for (i = 0; i < InputPeaks.size(); ++i) {
        // push back m/z
        MySpectrum->SetMz().push_back(InputPeaks[i].mz);
        // convert and push back intensity 
        MySpectrum->SetAbundance().push_back(static_cast <int> (InputPeaks[i].Intensity*Scale));
    }
    return true;
}

///
/// Read in the body of a dta file
///
bool CSpectrumSet::GetDTABody(CNcbiIstream& DTA, TInputPeaks& InputPeaks)
{
    float dummy(0.0);
    TInputPeak InputPeak;

    if(!(DTA >> dummy) || dummy < 0)
        return false;
    if (dummy > kMax_Int) dummy = MSSCALE2DBL(kMax_Int);
    InputPeak.mz = MSSCALE2INT(dummy);

    if(!(DTA >> dummy) || dummy < 0)
        return false;
    InputPeak.Intensity = dummy;

    InputPeaks.push_back(InputPeak);
    return true;
}


///
/// load in a single dta file
///
int CSpectrumSet::LoadDTA(CNcbiIstream& DTA)
{   
    CRef <CMSSpectrum> MySpectrum;
    bool GotOne(false);  // has a spectrum been read?
    string Line;

    try {
//        DTA.exceptions(ios_base::failbit | ios_base::badbit);

        MySpectrum = new CMSSpectrum;
        MySpectrum->SetNumber(1);

        {
            do {
                MyGetline(DTA, Line);
            } while (Line.size() < 3 && !DTA.eof()); // skip blank lines
            
            CNcbiIstrstream istr(Line.c_str());
            if(!GetDTAHeader(istr, MySpectrum)) return 1;
        }

        TInputPeaks InputPeaks;
        while (DTA) {
            MyGetline(DTA, Line);
            CNcbiIstrstream istr(Line.c_str());
            if (!GetDTABody(istr, InputPeaks)) break;
            GotOne = true;
        } 

        if(GotOne) {
            Peaks2Spectrum(InputPeaks, MySpectrum);
            Set().push_back(MySpectrum);
        }

    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadDTA: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadDTA: " );
        throw;
    }

    if (!GotOne) return 1;
    return 0;
}

///
/// load mgf
///

int CSpectrumSet::LoadMGF(CNcbiIstream& DTA, int Max)
{
    int iIndex(0); // the spectrum index
    int Count(0);  // total number of spectra
    int retval;
    bool GotOne(false);  // has a spectrum been read?
    try {
        
        do {
            CRef <CMSSpectrum> MySpectrum(new CMSSpectrum);
            Count++;
            if (Max > 0 && Count > Max) 
                return -1;  // too many

            MySpectrum->SetNumber(iIndex);
            iIndex++;
            retval = GetMGFBlock(DTA, MySpectrum);

            if (retval == 0 ) {
                GotOne = true;
            }
            else if (retval == 1) {
                return 1;  // something went wrong
            }
            // retval = -1 means no more records found
            // retval = -2 means empty block
            if (retval != -1 && retval != -2) Set().push_back(MySpectrum);

        } while (DTA && !DTA.eof() && retval != -1);

        if (!GotOne) 
            return 1;

    } catch (NCBI_NS_STD::exception& e) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMGF: " << e.what());
        throw;
    } catch (...) {
        ERR_POST(Info << "Exception in CSpectrumSet::LoadMGF: " );
        throw;
    }

    return 0;
}


///
/// Read in an ms/ms block in an mgf file
///
int CSpectrumSet::GetMGFBlock(CNcbiIstream& DTA, CRef <CMSSpectrum>& MySpectrum)
{
    string Line;
    bool GotMass(false);
    TInputPeaks InputPeaks;

    // find the start of the block
    do {
        MyGetline(DTA, Line);
        if(!DTA || DTA.eof()) 
            return -1;
    } while (NStr::CompareNocase(Line, 0, 10, "BEGIN IONS") != 0);

    // scan in headers
    do {
        MyGetline(DTA, Line);
        if(!DTA || DTA.eof()) 
            return 1;
        if (NStr::CompareNocase(Line, 0, 6, "TITLE=") == 0) {
            MySpectrum->SetIds().push_back(
                NStr::PrintableString(Line.substr(6, Line.size()-6)));
        }
        else if (NStr::CompareNocase(Line, 0, 8, "PEPMASS=") == 0) {
            // mandatory.
            GotMass = true;
            // create istrstream as some broken mgf generators insert whitespace separated cruft after mass
            string LastLine(Line.substr(8, Line.size()-8));
            CNcbiIstrstream istr(LastLine.c_str());
            double precursor;
            if(istr >> precursor) {
                MySpectrum->SetPrecursormz(MSSCALE2INT(precursor));
                MySpectrum->SetCharge().push_back(1);   // required in asn.1 (but shouldn't be)
                }
            else return 1;
        }
        // check for an empty scan
        else if(NStr::CompareNocase(Line, 0, 8, "END IONS") == 0) {
            return -2;
        }
        // keep looping while the first character is not numeric
    } while (Line.substr(0, 1).find_first_not_of("0123456789.-") == 0);

    if(!GotMass) 
        return 1;

    while (NStr::CompareNocase(Line, 0, 8, "END IONS") != 0) {
        if(!DTA || DTA.eof()) 
            return 1;
        CNcbiIstrstream istr(Line.c_str());
        // get rid of blank lines (technically not allowed, but they do occur)
        // size one allows for dos/mac end of line chars.
        if(Line.size() <= 1) 
            goto skipone;
        // skip comments (technically not allowed)
        if(Line.find_first_of("#;/!") == 0) 
            goto skipone;

        if(!GetDTABody(istr, InputPeaks)) return 1;
skipone:
        MyGetline(DTA, Line);
    } 
    Peaks2Spectrum(InputPeaks, MySpectrum);

    return 0;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/* Original file checksum: lines: 56, chars: 1753, CRC32: bdc55e21 */
