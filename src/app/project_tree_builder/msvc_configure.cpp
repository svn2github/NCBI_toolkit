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
 * Author:  Viatcheslav Gorelenkov
 *
 */

#include <ncbi_pch.hpp>
#include <app/project_tree_builder/msvc_configure.hpp>
#include <app/project_tree_builder/proj_builder_app.hpp>

#include <app/project_tree_builder/ptb_err_codes.hpp>

BEGIN_NCBI_SCOPE


CMsvcConfigure::CMsvcConfigure(void)
{
}


CMsvcConfigure::~CMsvcConfigure(void)
{
}


void s_ResetLibInstallKey(const string& dir, 
                          const string& lib)
{
    string key_file_name(lib);
    NStr::ToLower(key_file_name);
    key_file_name += ".installed";
    string key_file_path = CDirEntry::ConcatPath(dir, key_file_name);
    if ( CDirEntry(key_file_path).Exists() ) {
        CDirEntry(key_file_path).Remove();
    }
}


static void s_CreateThirdPartyLibsInstallMakefile
                                            (const CMsvcSite&   site, 
                                             const list<string> libs_to_install,
                                             const SConfigInfo& config,
                                             const CBuildType&  build_type)
{
    // Create makefile path
    string makefile_path = GetApp().GetProjectTreeInfo().m_Compilers;
    makefile_path = 
        CDirEntry::ConcatPath(makefile_path, 
                              GetApp().GetRegSettings().m_CompilersSubdir);

    makefile_path = CDirEntry::ConcatPath(makefile_path, build_type.GetTypeStr());
    makefile_path = CDirEntry::ConcatPath(makefile_path, config.GetConfigFullName());
    makefile_path = CDirEntry::ConcatPath(makefile_path, 
                                          "Makefile.third_party.mk");

    // Create dir if no such dir...
    string dir;
    CDirEntry::SplitPath(makefile_path, &dir);
    CDir makefile_dir(dir);
    if ( !makefile_dir.Exists() ) {
        CDir(dir).CreatePath();
    }

    CNcbiOfstream ofs(makefile_path.c_str(), 
                      IOS_BASE::out | IOS_BASE::trunc );
    if ( !ofs )
        NCBI_THROW(CProjBulderAppException, eFileCreation, makefile_path);

    
    ITERATE(list<string>, n, libs_to_install) {
        const string& lib = *n;
        SLibInfo lib_info;
        site.GetLibInfo(lib, config, &lib_info);
        if ( !lib_info.m_LibPath.empty() ) {
            string bin_dir = lib_info.m_LibPath;
            bin_dir = 
                CDirEntry::ConcatPath(bin_dir, 
                                      site.GetThirdPartyLibsBinSubDir());
            bin_dir = CDirEntry::NormalizePath(bin_dir);
            if ( CDirEntry(bin_dir).Exists() ) {
                //
                string key(lib);
                NStr::ToUpper(key);
                key += site.GetThirdPartyLibsBinPathSuffix();

                ofs << key << " = " << bin_dir << "\n";

                s_ResetLibInstallKey(dir, lib);
            } else {
                PTB_WARNING_EX(bin_dir, ePTB_PathNotFound,
                               lib << "|" << config.GetConfigFullName()
                               << " disabled, path not found");
            }
        } else {
            PTB_WARNING_EX(kEmptyStr, ePTB_PathNotFound,
                           lib << "|" << config.GetConfigFullName()
                           << ": no LIBPATH specified");
        }
    }
}


void CMsvcConfigure::Configure(CMsvcSite&         site, 
                               const list<SConfigInfo>& configs,
                               const string&            root_dir)
{
    _TRACE("*** Analyzing 3rd party libraries availability ***");
    
    InitializeFrom(site);
    site.ProcessMacros(configs);

    if (CMsvc7RegSettings::GetMsvcVersion() >= CMsvc7RegSettings::eMsvcNone) {
        return;
    }
    
    const CBuildType static_build(false);
    const CBuildType dll_build(true);
    ITERATE(list<SConfigInfo>, p, configs) {
        if (!p->m_VTuneAddon) {
            AnalyzeDefines( site, root_dir, *p, static_build);
        }
    }
    list<SConfigInfo> dlls;
    GetApp().GetDllsInfo().GetBuildConfigs(&dlls);
    ITERATE(list<SConfigInfo>, p, dlls) {
        if (!p->m_VTuneAddon) {
            AnalyzeDefines( site, root_dir, *p, dll_build);
        }
    }

    _TRACE("*** Creating Makefile.third_party.mk files ***");
    // Write makefile uses to install 3-rd party dlls
    list<string> third_party_to_install;
    site.GetThirdPartyLibsToInstall(&third_party_to_install);
    // For static buid
    ITERATE(list<SConfigInfo>, p, configs) {
        const SConfigInfo& config = *p;
        s_CreateThirdPartyLibsInstallMakefile(site, 
                                              third_party_to_install, 
                                              config, 
                                              static_build);
    }
    // For dll build
    list<SConfigInfo> dll_configs;
    GetApp().GetDllsInfo().GetBuildConfigs(&dll_configs);
    ITERATE(list<SConfigInfo>, p, dll_configs) {
        const SConfigInfo& config = *p;
        s_CreateThirdPartyLibsInstallMakefile(site, 
                                              third_party_to_install, 
                                              config, 
                                              dll_build);
    }
}


void CMsvcConfigure::InitializeFrom(const CMsvcSite& site)
{
    m_ConfigSite.clear();

    m_ConfigureDefinesPath = site.GetConfigureDefinesPath();
    if ( m_ConfigureDefinesPath.empty() ) {
        NCBI_THROW(CProjBulderAppException, 
           eConfigureDefinesPath,
           "Configure defines file name is not specified");
    }

    site.GetConfigureDefines(&m_ConfigureDefines);
    if( m_ConfigureDefines.empty() ) {
        PTB_ERROR(m_ConfigureDefinesPath,
                  "No configurable macro definitions specified.");
    } else {
        _TRACE("Configurable macro definitions: ");
        ITERATE(list<string>, p, m_ConfigureDefines) {
            _TRACE(*p);
        }
    }
}


bool CMsvcConfigure::ProcessDefine(const string& define, 
                                   const CMsvcSite& site, 
                                   const SConfigInfo& config) const
{
    if ( !site.IsDescribed(define) ) {
        PTB_ERROR_EX(kEmptyStr, ePTB_MacroUndefined,
                     define << ": Macro not defined");
        return false;
    }
    list<string> components;
    site.GetComponents(define, &components);
    ITERATE(list<string>, p, components) {
        const string& component = *p;
        SLibInfo lib_info;
        site.GetLibInfo(component, config, &lib_info);
        if ( !site.IsLibOk(lib_info) ) {
            if (!lib_info.IsEmpty()) {
                PTB_WARNING_EX("", ePTB_ConfigurationError,
                               component << "|" << config.GetConfigFullName()
                               << ": " << define << " not satisfied, disabled");
            }
            return false;
        }
    }
    return true;
}

void CMsvcConfigure::AnalyzeDefines(
    CMsvcSite& site, const string& root_dir,
    const SConfigInfo& config, const CBuildType&  build_type)
{
    string ncbi_conf_msvc_site_path = 
        CDirEntry::ConcatPath(root_dir, m_ConfigureDefinesPath);
    string dir, base, ext;
    CDirEntry::SplitPath(ncbi_conf_msvc_site_path, &dir, &base, &ext);
    string filename = CDirEntry::ConcatPath( dir, base) + "." + config.m_Name + ext;

    _TRACE("Configuration " << config.m_Name << ":");

    m_ConfigSite.clear();

    ITERATE(list<string>, p, m_ConfigureDefines) {
        const string& define = *p;
        if( ProcessDefine(define, site, config) ) {
            _TRACE("Macro definition Ok  " << define);
            m_ConfigSite[define] = '1';
        } else {
            PTB_WARNING_EX(kEmptyStr, ePTB_MacroUndefined,
                           "Macro definition not satisfied: " << define);
            m_ConfigSite[define] = '0';
        }
    }

    string candidate_path = filename + ".candidate";
    CDirEntry::SplitPath(filename, &dir);
    CDir(dir).CreatePath();
    WriteNcbiconfMsvcSite(candidate_path);
    if (PromoteIfDifferent(filename, candidate_path)) {
        PTB_WARNING_EX(filename, ePTB_FileModified,
                       "Configuration file modified");
    } else {
        PTB_INFO_EX(filename, ePTB_NoError,
                    "Configuration file unchanged");
    }
}

void CMsvcConfigure::WriteNcbiconfMsvcSite(const string& full_path) const
{
    CNcbiOfstream  ofs(full_path.c_str(), 
                       IOS_BASE::out | IOS_BASE::trunc );
    if ( !ofs )
	    NCBI_THROW(CProjBulderAppException, eFileCreation, full_path);

    ofs << endl;

    ofs <<"/* $" << "Id" << "$" << endl;
    ofs <<"* ===========================================================================" << endl;
    ofs <<"*" << endl;
    ofs <<"*                            PUBLIC DOMAIN NOTICE" << endl;
    ofs <<"*               National Center for Biotechnology Information" << endl;
    ofs <<"*" << endl;
    ofs <<"*  This software/database is a \"United States Government Work\" under the" << endl;
    ofs <<"*  terms of the United States Copyright Act.  It was written as part of" << endl;
    ofs <<"*  the author's official duties as a United States Government employee and" << endl;
    ofs <<"*  thus cannot be copyrighted.  This software/database is freely available" << endl;
    ofs <<"*  to the public for use. The National Library of Medicine and the U.S." << endl;
    ofs <<"*  Government have not placed any restriction on its use or reproduction." << endl;
    ofs <<"*" << endl;
    ofs <<"*  Although all reasonable efforts have been taken to ensure the accuracy" << endl;
    ofs <<"*  and reliability of the software and data, the NLM and the U.S." << endl;
    ofs <<"*  Government do not and cannot warrant the performance or results that" << endl;
    ofs <<"*  may be obtained by using this software or data. The NLM and the U.S." << endl;
    ofs <<"*  Government disclaim all warranties, express or implied, including" << endl;
    ofs <<"*  warranties of performance, merchantability or fitness for any particular" << endl;
    ofs <<"*  purpose." << endl;
    ofs <<"*" << endl;
    ofs <<"*  Please cite the author in any work or product based on this material." << endl;
    ofs <<"*" << endl;
    ofs <<"* ===========================================================================" << endl;
    ofs <<"*" << endl;
    ofs <<"* Author:  ......." << endl;
    ofs <<"*" << endl;
    ofs <<"* File Description:" << endl;
    ofs <<"*   ......." << endl;
    ofs <<"*" << endl;
    ofs <<"* ATTENTION:" << endl;
    ofs <<"*   Do not edit or commit this file into CVS as this file will" << endl;
    ofs <<"*   be overwritten (by PROJECT_TREE_BUILDER) without warning!" << endl;
    ofs <<"*/" << endl;
    ofs << endl;
    ofs << endl;


    ITERATE(TConfigSite, p, m_ConfigSite) {
        if (p->second == '1') {
            ofs << "#define " << p->first << " " << p->second << endl;
        } else {
            ofs << "/* #undef " << p->first << " */" << endl;
        }
    }
    ofs << endl;
}


END_NCBI_SCOPE
