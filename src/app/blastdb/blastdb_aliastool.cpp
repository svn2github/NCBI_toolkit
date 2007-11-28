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
 * Author: Christiam Camacho
 *
 */

/** @file blastdb_aliastool.cpp
 * Command line tool to create BLAST database aliases and associated files. 
 */

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = 
    "$Id$";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <algo/blast/api/version.hpp>
#include <objtools/writers/writedb/writedb.hpp>
#include <objtools/writers/writedb/writedb_error.hpp>

#ifndef SKIP_DOXYGEN_PROCESSING
USING_NCBI_SCOPE;
USING_SCOPE(blast);
#endif

class CBlastDBAliasApp : public CNcbiApplication
{
public:
    CBlastDBAliasApp() {
        SetVersion(blast::Version);
    }
private:
    virtual void Init();
    virtual int Run();

    /// Converts gi files from binary to text format
    /// @return 0 on success
    int ConvertGiFile() const;

    /// Documentation for this program
    static const char * const DOCUMENTATION;

    /// Describes the modes of operation of this application
    enum EOperationMode {
        eCreateAlias,       ///< Create alias files
        eConvertGiFile      ///< Convert gi files from text to binary format
    };

    /// Determine what mode of operation is being used
    EOperationMode x_GetOperationMode() const {
        EOperationMode retval = eCreateAlias;
        if (GetArgs()["gi_file_in"].HasValue()) {
            retval = eConvertGiFile;
        }
        return retval;
    }
};

const char * const CBlastDBAliasApp::DOCUMENTATION = "\n\n"
"This application has two modes of operation:\n\n"
"1) Gi file conversion:\n"
"   Converts a text file containing GIs (one per line) to a more efficient\n"
"   binary format. This can be provided as an argument to the -gilist option\n"
"   of the BLAST search command line binaries or to the -gilist option of\n"
"   this program to create an alias file for a BLAST database (see below).\n\n"
"2) Alias file creation:\n"
"   Creates an alias for a BLAST database and a GI list which restricts this\n"
"   database. This is useful if one often searches a subset of a database\n"
"   (e.g., based on organism or a curated list). The alias file makes the\n"
"   search appear as if one were searching a regular BLAST database rather\n"
"   than the subset of one.\n";

void CBlastDBAliasApp::Init()
{
    HideStdArgs(fHideConffile | fHideDryRun);

    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(), 
                  "Application to create BLAST database aliases, version " 
                  + blast::Version.Print() + DOCUMENTATION);

    const string kOutput("out");
    string dflt("Default = input file name provided to -gi_file_in argument");
    dflt += " with the .bgl extension";

    const char* exclusions[]  = { "db", "dbtype", "title", "gilist", "out" };
    arg_desc->SetCurrentGroup("GI file conversion options");
    arg_desc->AddOptionalKey("gi_file_in", "input_file",
                     "Text file to convert, should contain one GI per line",
                     CArgDescriptions::eInputFile);
    for (size_t i = 0; i < sizeof(exclusions)/sizeof(*exclusions); i++) {
        arg_desc->SetDependency("gi_file_in", CArgDescriptions::eExcludes,
                                string(exclusions[i]));
    }
    arg_desc->AddOptionalKey("gi_file_out", "output_file",
                     "File name of converted GI file\n" + dflt,
                     CArgDescriptions::eOutputFile,
                     CArgDescriptions::fPreOpen | CArgDescriptions::fBinary);
    arg_desc->SetDependency("gi_file_out", CArgDescriptions::eRequires,
                            "gi_file_in");
    for (size_t i = 0; i < sizeof(exclusions)/sizeof(*exclusions); i++) {
        arg_desc->SetDependency("gi_file_out", CArgDescriptions::eExcludes,
                                string(exclusions[i]));
    }

    arg_desc->SetCurrentGroup("Alias file creation options");
    arg_desc->AddOptionalKey("db", "dbname", "BLAST database name", 
                             CArgDescriptions::eString);
    arg_desc->SetDependency("db", CArgDescriptions::eRequires, "gilist");
    arg_desc->SetDependency("db", CArgDescriptions::eRequires, kOutput);

    arg_desc->AddDefaultKey("dbtype", "molecule_type",
                            "Molecule type stored in BLAST database",
                            CArgDescriptions::eString, "guess");
    arg_desc->SetConstraint("dbtype", &(*new CArgAllow_Strings,
                                        "nucl", "prot", "guess"));

    arg_desc->AddOptionalKey("title", "database_title",
                     "Title for BLAST database\n"
                     "Default = name of BLAST database provided to -db"
                     " argument with the -gifile argument appended to it",
                     CArgDescriptions::eString);
    arg_desc->SetDependency("title", CArgDescriptions::eRequires, "db");
    arg_desc->SetDependency("title", CArgDescriptions::eRequires, "gilist");
    arg_desc->SetDependency("title", CArgDescriptions::eRequires, kOutput);

    arg_desc->AddOptionalKey("gilist", "input_file", 
                             "Text or binary gi file to restrict the BLAST "
                             "database provided in -db argument\n"
                             "If text format is provided, it will be converted "
                             "to binary",
                             CArgDescriptions::eInputFile);
    arg_desc->SetDependency("gilist", CArgDescriptions::eRequires, "db");
    arg_desc->SetDependency("gilist", CArgDescriptions::eRequires, kOutput);

    arg_desc->AddOptionalKey(kOutput, "database_name",
                             "Name of BLAST database alias to be created",
                             CArgDescriptions::eString);
    arg_desc->SetDependency(kOutput, CArgDescriptions::eRequires, "db");
    arg_desc->SetDependency(kOutput, CArgDescriptions::eRequires, "gilist");

    SetupArgDescriptions(arg_desc.release());
}

int CBlastDBAliasApp::ConvertGiFile() const
{
    const CArgs& args = GetArgs();
    CNcbiIstream& input = args["gi_file_in"].AsInputFile();
    CNcbiOstream& output = args["gi_file_out"].AsOutputFile();

    CBinaryListBuilder builder(CBinaryListBuilder::eGi);

    unsigned int line_ctr = 0;
    while (input) {
        string line;
        NcbiGetlineEOL(input, line);
        line_ctr++;
        if ( !line.empty() ) {
            try { builder.AppendId(NStr::StringToInt8(line)); }
            catch (const CStringException& e) {
                ERR_POST(Warning << "error in line " << line_ctr 
                         << ": " << e.GetMsg());
            }
        }
    }

    builder.Write(output);
    LOG_POST("Converted " << builder.Size() << " GIs");
    return 0;
}

int CBlastDBAliasApp::Run(void)
{
    int status = 0;

    try {

        if (x_GetOperationMode() == eConvertGiFile) {
            status = ConvertGiFile();
        } else {
            throw runtime_error("Unimplemented functionality");
        }

    } catch (const CException& exptn) {
        cerr << exptn.GetMsg() << endl;
        status = exptn.GetErrCode();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        status = -1;
    } catch (...) {
        cerr << "Unknown exception" << endl;
        status = -1;
    }
    return status;
}


#ifndef SKIP_DOXYGEN_PROCESSING
int main(int argc, const char* argv[] /*, const char* envp[]*/)
{
    return CBlastDBAliasApp().AppMain(argc, argv, 0, eDS_Default, 0);
}
#endif /* SKIP_DOXYGEN_PROCESSING */
