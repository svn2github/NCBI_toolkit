#ifndef PROJECT_TREE_BUILDER__MSVC_PRJ_UTILS__HPP
#define PROJECT_TREE_BUILDER__MSVC_PRJ_UTILS__HPP

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

#include <app/project_tree_builder/ptb_registry.hpp>
#include <app/project_tree_builder/proj_item.hpp>
#include <corelib/ncbienv.hpp>
#if NCBI_COMPILER_MSVC
#   include <app/project_tree_builder/msvc71_project__.hpp>
#endif //NCBI_COMPILER_MSVC
#include <app/project_tree_builder/proj_item.hpp>
#include <set>

BEGIN_NCBI_SCOPE

#if NCBI_COMPILER_MSVC
USING_SCOPE(objects);

/// Creates CVisualStudioProject class instance from file.
///
/// @param file_path
///   Path to file load from.
/// @return
///   Created on heap CVisualStudioProject instance or NULL
///   if failed.
CVisualStudioProject * LoadFromXmlFile(const string& file_path);


/// Save CVisualStudioProject class instance to file.
///
/// @param file_path
///   Path to file project will be saved to.
/// @param project
///   Project to save.
void SaveToXmlFile  (const string&               file_path, 
                     const CVisualStudioProject& project);

/// Save CVisualStudioProject class instance to file only if no such file 
//  or contents of this file will be different from already present file.
///
/// @param file_path
///   Path to file project will be saved to.
/// @param project
///   Project to save.
void SaveIfNewer    (const string&               file_path, 
                     const CVisualStudioProject& project);
#endif //NCBI_COMPILER_MSVC

/// Consider promotion candidate to present 
bool PromoteIfDifferent(const string& present_path, 
                        const string& candidate_path);


/// Generate pseudo-GUID.
string GenerateSlnGUID(void);
string IdentifySlnGUID(const string& source_dir, const CProjKey& proj);

/// Get extension for source file without extension.
///
/// @param file_path
///   Source file full path withour extension.
/// @return
///   Extension of source file (".cpp" or ".c") 
///   if such file exist. Empty string string if there is no
///   such file.
string SourceFileExt(const string& file_path);


/////////////////////////////////////////////////////////////////////////////
///
/// SConfigInfo --
///
/// Abstraction of configuration informaion.
///
/// Configuration name, debug/release flag, runtime library 
/// 

struct SConfigInfo
{
    SConfigInfo(void);
    SConfigInfo(const string& name, 
                bool          debug, 
                const string& runtime_library);
    void DefineRtType();
    string GetConfigFullName(void) const;

    string m_Name;
    string m_RuntimeLibrary;
    bool   m_Debug;
    bool   m_VTuneAddon;
    enum {
        rtMultiThreaded = 0,
        rtMultiThreadedDebug = 1,
        rtMultiThreadedDLL = 2,
        rtMultiThreadedDebugDLL = 3,
        rtSingleThreaded = 4,
        rtSingleThreadedDebug = 5,
        rtUnknown = 6
    } m_rtType;
};

// Helper to load configs from ini files
void LoadConfigInfoByNames(const CNcbiRegistry& registry, 
                           const list<string>&  config_names, 
                           list<SConfigInfo>*   configs);


/////////////////////////////////////////////////////////////////////////////
///
/// SCustomBuildInfo --
///
/// Abstraction of custom build source file.
///
/// Information for custom buil source file 
/// (not *.c, *.cpp, *.midl, *.rc, etc.)
/// MSVC does not know how to buil this file and
/// we provide information how to do it.
/// 

struct SCustomBuildInfo
{
    string m_SourceFile; // absolut path!
    string m_CommandLine;
    string m_Description;
    string m_Outputs;
    string m_AdditionalDependencies;

    bool IsEmpty(void) const
    {
        return m_SourceFile.empty() || m_CommandLine.empty();
    }
    void Clear(void)
    {
        m_SourceFile.erase();
        m_CommandLine.erase();
        m_Description.erase();
        m_Outputs.erase();
        m_AdditionalDependencies.erase();
    }
};


/////////////////////////////////////////////////////////////////////////////
///
/// CMsvc7RegSettings --
///
/// Abstraction of [msvc7] section in app registry.
///
/// Settings for generation of msvc 7.10 projects
/// 

class CMsvc7RegSettings
{
public:
    enum EMsvcVersion {
        eMsvc710 = 0,
        eMsvc800,
        eMsvcNone
    };
    enum EMsvcPlatform {
        eMsvcWin32 = 0,
        eMsvcX64,
        eUnix
    };

    CMsvc7RegSettings(void);

    string            m_Version;
    list<SConfigInfo> m_ConfigInfo;
    string            m_CompilersSubdir;
    string            m_ProjectsSubdir;
    string            m_MakefilesExt;
    string            m_MetaMakefile;
    string            m_DllInfo;

    static EMsvcVersion    GetMsvcVersion(void)
    {
        return sm_MsvcVersion;
    }
    static const string&   GetMsvcVersionName(void)
    {
        return sm_MsvcVersionName;
    }
    static EMsvcPlatform   GetMsvcPlatform(void)
    {
        return sm_MsvcPlatform;
    }
    static const string&   GetMsvcPlatformName(void)
    {
        return sm_MsvcPlatformName;
    }
    static string          GetMsvcSection(void);

    string    GetProjectFileFormatVersion(void) const;
    string    GetSolutionFileFormatVersion(void) const;
private:
    static EMsvcVersion   sm_MsvcVersion;
    static EMsvcPlatform  sm_MsvcPlatform;
    static string sm_MsvcVersionName;
    static string sm_MsvcPlatformName;
    CMsvc7RegSettings(const CMsvc7RegSettings&);
    CMsvc7RegSettings& operator= (const CMsvc7RegSettings&);
};


/// Is abs_dir a parent of abs_parent_dir.
bool IsSubdir(const string& abs_parent_dir, const string& abs_dir);


/// Erase if predicate is true
template <class C, class P> 
void EraseIf(C& cont, const P& pred)
{
    for (typename C::iterator p = cont.begin(); p != cont.end(); )
    {
        if ( pred(*p) ) {
            typename C::iterator p_next = p;
	    ++p_next;
            cont.erase(p);
	    p = p_next;
        }
        else
            ++p;
    }
}


/// Get option fron registry from  
///     [<section>.debug.<ConfigName>] section for debug configuratios
///  or [<section>.release.<ConfigName>] for release configurations
///
/// if no such option then try      
///     [<section>.debug]
/// or  [<section>.release]
///
/// if no such option then finally try
///     [<section>]
///
string GetOpt(const CPtbRegistry& registry, 
              const string&        section, 
              const string&        opt, 
              const SConfigInfo&   config);

/// return <config>|Win32 as needed by MSVC compiler
string ConfigName(const string& config);



//-----------------------------------------------------------------------------

// Base interface class for all insertors
class IFilesToProjectInserter
{
public:
    virtual ~IFilesToProjectInserter(void)
    {
    }

    virtual void AddSourceFile (const string& rel_file_path) = 0;
    virtual void AddHeaderFile (const string& rel_file_path) = 0;
    virtual void AddInlineFile (const string& rel_file_path) = 0;

    virtual void Finalize      (void)                        = 0;
};


#if NCBI_COMPILER_MSVC
// Insert .cpp and .c files to filter and set PCH usage if necessary
class CSrcToFilterInserterWithPch
{
public:
    CSrcToFilterInserterWithPch(const string&            project_id,
                                const list<SConfigInfo>& configs,
                                const string&            project_dir);

    ~CSrcToFilterInserterWithPch(void);

    void operator() (CRef<CFilter>& filter, 
                     const string&  rel_source_file);

private:
    string            m_ProjectId;
    list<SConfigInfo> m_Configs;
    string            m_ProjectDir;

    typedef set<string> TPchHeaders;
    TPchHeaders m_PchHeaders;

    enum EUsePch {
        eNotUse = 0,
        eCreate = 1,
        eUse    = 3
    };
    typedef pair<EUsePch, string> TPch;

    TPch DefinePchUsage(const string&     project_dir,
                        const string&     rel_source_file);

    void InsertFile    (CRef<CFilter>&    filter, 
                        const string&     rel_source_file,
                        const string&     enable_cfg = kEmptyStr);

    // Prohibited to:
    CSrcToFilterInserterWithPch(void);
    CSrcToFilterInserterWithPch(const CSrcToFilterInserterWithPch&);
    CSrcToFilterInserterWithPch& operator=(const CSrcToFilterInserterWithPch&);
};

class CBasicProjectsFilesInserter : public IFilesToProjectInserter
{
public:
    CBasicProjectsFilesInserter(CVisualStudioProject*    vcproj,
                                const string&            project_id,
                                const list<SConfigInfo>& configs,
                                const string&            project_dir);

    virtual ~CBasicProjectsFilesInserter(void);

    // IFilesToProjectInserter implementation
    virtual void AddSourceFile (const string& rel_file_path);
    virtual void AddHeaderFile (const string& rel_file_path);
    virtual void AddInlineFile (const string& rel_file_path);

    virtual void Finalize      (void);

    struct SFiltersItem
    {
        SFiltersItem(void);
        SFiltersItem(const string& project_dir);

        CRef<CFilter> m_SourceFiles;
        CRef<CFilter> m_HeaderFiles;
        CRef<CFilter> m_HeaderFilesPrivate;
        CRef<CFilter> m_HeaderFilesImpl;
        CRef<CFilter> m_InlineFiles;
        
        string        m_ProjectDir;

        void Initilize(void);

        void AddSourceFile (CSrcToFilterInserterWithPch& inserter_w_pch,
                            const string&                rel_file_path);

        void AddHeaderFile (const string& rel_file_path);

        void AddInlineFile (const string& rel_file_path);

    };

private:
    CVisualStudioProject*       m_Vcproj;
    
    CSrcToFilterInserterWithPch m_SrcInserter;
    SFiltersItem                m_Filters;
    

    // Prohibited to:
    CBasicProjectsFilesInserter(void);
    CBasicProjectsFilesInserter(const CBasicProjectsFilesInserter&);
    CBasicProjectsFilesInserter& operator=(const CBasicProjectsFilesInserter&);
};

class CDllProjectFilesInserter : public IFilesToProjectInserter
{
public:
    CDllProjectFilesInserter(CVisualStudioProject*    vcproj,
                             const CProjKey           dll_project_key,
                             const list<SConfigInfo>& configs,
                             const string&            project_dir);

    virtual ~CDllProjectFilesInserter(void);

    // IFilesToProjectInserter implementation
    virtual void AddSourceFile (const string& rel_file_path);
    virtual void AddHeaderFile (const string& rel_file_path);
    virtual void AddInlineFile (const string& rel_file_path);

    virtual void Finalize      (void);

private:
    CVisualStudioProject*       m_Vcproj;
    CProjKey                    m_DllProjectKey;
    CSrcToFilterInserterWithPch m_SrcInserter;
    string                      m_ProjectDir;

    typedef CBasicProjectsFilesInserter::SFiltersItem TFiltersItem;
    TFiltersItem  m_PrivateFilters;
    CRef<CFilter> m_HostedLibrariesRootFilter;

    typedef map<CProjKey, TFiltersItem> THostedLibs;
    THostedLibs m_HostedLibs;

    // Prohibited to:
    CDllProjectFilesInserter(void);
    CDllProjectFilesInserter(const CDllProjectFilesInserter&);
    CDllProjectFilesInserter& operator=(const CDllProjectFilesInserter&);
};


/// Common function shared by 
/// CMsvcMasterProjectGenerator and CMsvcProjectGenerator
void AddCustomBuildFileToFilter(CRef<CFilter>&          filter, 
                                const list<SConfigInfo> configs,
                                const string&           project_dir,
                                const SCustomBuildInfo& build_info);


/// Fill-In MSVC 7.10 Utility project
void CreateUtilityProject(const string&            name, 
                          const list<SConfigInfo>& configs, 
                          CVisualStudioProject*    project);
#endif //NCBI_COMPILER_MSVC

/// Checks if 2 dirs has the same root
bool SameRootDirs(const string& dir1, const string& dir2);

/// Project naming schema
string CreateProjectName(const CProjKey& project_id);


/// Utility class for distinguish between static and dll builds
class CBuildType
{
public:
    CBuildType(bool dll_flag);

    enum EBuildType {
        eStatic,
        eDll
    };

    EBuildType GetType   (void) const;
    string     GetTypeStr(void) const;

private:
    EBuildType m_Type;
    
    //prohibited to:
    CBuildType(void);
    CBuildType(const CBuildType&);
    CBuildType& operator= (const CBuildType&);
};


/// Distribution if source files by lib projects
/// Uses in dll project to separate source files to groups by libs
class CDllSrcFilesDistr
{
public:
    CDllSrcFilesDistr(void);


    // Register .cpp .c files during DLL creation
    void RegisterSource  (const string&   src_file_path, 
                          const CProjKey& dll_project_id,
                          const CProjKey& lib_project_id);
    // Register .hpp .h files during DLL creation
    void RegisterHeader  (const string&   hrd_file_path, 
                          const CProjKey& dll_project_id,
                          const CProjKey& lib_project_id);
    // Register .inl    files during DLL creation
    void RegisterInline  (const string&   inl_file_path, 
                          const CProjKey& dll_project_id,
                          const CProjKey& lib_project_id);

    
    // Retrive original lib_id for .cpp .c file
    CProjKey GetSourceLib(const string&   src_file_path, 
                          const CProjKey& dll_project_id) const;
    // Retrive original lib_id for .cpp .c file
    CProjKey GetHeaderLib(const string&   hdr_file_path, 
                          const CProjKey& dll_project_id) const;
    // Retrive original lib_id for .inl file
    CProjKey GetInlineLib(const string&   inl_file_path, 
                          const CProjKey& dll_project_id) const;
private:

    typedef pair<string,    CProjKey> TDllSrcKey;
    typedef map<TDllSrcKey, CProjKey> TDistrMap;
    TDistrMap m_SourcesMap;
    TDistrMap m_HeadersMap;
    TDistrMap m_InlinesMap;

    //prohibited to
    CDllSrcFilesDistr(const CDllSrcFilesDistr&);
    CDllSrcFilesDistr& operator= (const CDllSrcFilesDistr&);
};


END_NCBI_SCOPE

#endif //PROJECT_TREE_BUILDER__MSVC_PRJ_UTILS__HPP
