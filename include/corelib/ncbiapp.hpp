#ifndef CORELIB___NCBIAPP__HPP
#define CORELIB___NCBIAPP__HPP

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
 * Authors:  Denis Vakatov, Vsevolod Sandomirskiy
 *
 *
 */

/// @file ncbiapp.hpp
/// Defines the CNcbiApplication and CAppException classes for creating
/// NCBI applications.
///
/// The CNcbiApplication class defines the application framework and the high
/// high level behavior of an application, and the CAppException class is used
/// for the exceptions generated by CNcbiApplication.


#include <corelib/ncbistd.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/metareg.hpp>
#include <corelib/version.hpp>
#include <memory>


/// Avoid preprocessor name clash with the NCBI C Toolkit.
#if !defined(NCBI_OS_UNIX)  ||  defined(HAVE_NCBI_C)
#  if defined(GetArgs)
#    undef GetArgs
#  endif
#  define GetArgs GetArgs
#endif


/** @addtogroup AppFramework
 *
 * @{
 */


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
///
/// CAppException --
///
/// Define exceptions generated by CNcbiApplication.
///
/// CAppException inherits its basic functionality from CCoreException
/// and defines additional error codes for applications.

class NCBI_XNCBI_EXPORT CAppException : public CCoreException
{
public:
    /// Error types that an application can generate.
    ///
    /// These error conditions are checked for and caught inside AppMain().
    enum EErrCode {
        eUnsetArgs,     ///< Command-line argument description not found
        eSetupDiag,     ///< Application diagnostic stream setup failed
        eLoadConfig,    ///< Registry data failed to load from config file
        eSecond,        ///< Second instance of CNcbiApplication is prohibited
        eNoRegistry     ///< Registry file cannot be opened
    };

    /// Translate from the error code value to its string representation.
    virtual const char* GetErrCodeString(void) const;

    // Standard exception boilerplate code.
    NCBI_EXCEPTION_DEFAULT(CAppException, CCoreException);
};



///////////////////////////////////////////////////////
// CNcbiApplication
//


/////////////////////////////////////////////////////////////////////////////
///
/// CNcbiApplication --
///
/// Basic (abstract) NCBI application class.
///
/// Defines the high level behavior of an NCBI application.
/// A new application is written by deriving a class from the CNcbiApplication
/// and writing an implementation of the Run() and maybe some other (like
/// Init(), Exit(), etc.) methods.

class NCBI_XNCBI_EXPORT CNcbiApplication
{
public:
    /// Singleton method.
    ///
    /// Track the instance of CNcbiApplication, and throw an exception
    /// if an attempt is made to create another instance of the application.
    /// @return
    ///   Current application instance.
    static CNcbiApplication* Instance(void);

    /// Constructor.
    ///
    /// Register the application instance, and reset important
    /// application-specific settings to empty values that will
    /// be set later.
    CNcbiApplication(void);

    /// Destructor.
    ///
    /// Clean up the application settings, flush the diagnostic stream.
    virtual ~CNcbiApplication(void);

    /// Main function (entry point) for the NCBI application.
    ///
    /// You can specify where to write the diagnostics to (EAppDiagStream),
    /// and where to get the configuration file (LoadConfig()) to load
    /// to the application registry (accessible via GetConfig()).
    ///
    /// Throw exception if:
    ///  - not-only instance
    ///  - cannot load explicitly specified config.file
    ///  - SetupDiag() throws an exception
    ///
    /// If application name is not specified a default of "ncbi" is used.
    /// Certain flags such as -logfile, -conffile and -version are special so
    /// AppMain() processes them separately.
    /// @return
    ///   Exit code from Run(). Can also return non-zero value if application
    ///   threw an exception.
    /// @sa
    ///   LoadConfig(), Init(), Run(), Exit()
    int AppMain
    (int                argc,     ///< argc in a regular main(argc, argv, envp)
     const char* const* argv,     ///< argv in a regular main(argc, argv, envp)
     const char* const* envp = 0, ///< envp in a regular main(argc, argv, envp)
     EAppDiagStream     diag = eDS_Default,     ///< Specify diagnostic stream
     const char*        conf = NcbiEmptyCStr,   ///< Specify registry to load
     const string&      name = NcbiEmptyString  ///< Specify application name
     );

    /// Initialize the application.
    ///
    /// The default behavior of this is "do nothing". If you have special
    /// initialization logic that needs to be peformed, then you must override
    /// this method with your own logic.
    virtual void Init(void);

    /// Run the application.
    ///
    /// It is defined as a pure virtual method -- so you must(!) supply the
    /// Run() method to implement the application-specific logic.
    /// @return
    ///   Exit code.
    virtual int Run(void) = 0;

    /// Test run the application.
    ///
    /// It is only supposed to test if the Run() is possible,
    /// or makes sense: that is, test all preconditions etc.
    /// @return
    ///   Exit code.
    virtual int DryRun(void);

    /// Cleanup on application exit.
    ///
    /// Perform cleanup before exiting. The default behavior of this is
    /// "do nothing". If you have special cleanup logic that needs to be
    /// performed, then you must override this method with your own logic.
    virtual void Exit(void);

    /// Get the application's cached unprocessed command-line arguments.
    const CNcbiArguments& GetArguments(void) const;

    /// Get parsed command line arguments.
    ///
    /// Get command line arguments parsed according to the arg descriptions
    /// set by SetupArgDescriptions(). Throw exception if no descriptions
    /// have been set.
    /// @return
    ///   The CArgs object containing parsed cmd.-line arguments.
    /// @sa
    ///   SetupArgDescriptions().
    virtual const CArgs& GetArgs(void) const;

    /// Get the application's cached environment.
    const CNcbiEnvironment& GetEnvironment(void) const;

    /// Get a non-const copy of the application's cached environment.
    CNcbiEnvironment& SetEnvironment(void);

    /// Set a specified environment variable by name
    void SetEnvironment(const string& name, const string& value);

    /// Check if the config file has been loaded
    bool HasLoadedConfig(void) const;

    /// Get the application's cached configuration parameters.
    const CNcbiRegistry& GetConfig(void) const;
    CNcbiRegistry& GetConfig(void);

    /// Get the full path to the configuration file (if any) we ended
    /// up using.
    const string& GetConfigPath(void) const;

    /// Reload the configuration file.  By default, does nothing if
    /// the file has the same size and date as before.
    ///
    /// Note that this may lose other data stored in the registry!
    ///
    /// @param flags
    ///   Controls how aggressively to reload.
    /// @param reg_flags
    ///   Flags to use when parsing the registry; ignored if the registry
    ///   was already cached (as it should normally have been).
    /// @return
    ///   TRUE if a reload actually occurred.
    bool ReloadConfig(CMetaRegistry::TFlags flags
                      = CMetaRegistry::fReloadIfChanged,
                      IRegistry::TFlags reg_flags = 0);

    /// Flush the in-memory diagnostic stream (for "eDS_ToMemory" case only).
    ///
    /// In case of "eDS_ToMemory", the diagnostics is stored in
    /// the internal application memory buffer ("m_DiagStream").
    /// Call this function to dump all the diagnostics to stream "os" and
    /// purge the buffer.
    /// @param  os
    ///   Output stream to dump diagnostics to. If it is NULL, then
    ///   nothing will be written to it (but the buffer will still be purged).
    /// @param  close_diag
    ///   If "close_diag" is TRUE, then also destroy "m_DiagStream".
    /// @return
    ///   Total number of bytes actually written to "os".
    SIZE_TYPE FlushDiag(CNcbiOstream* os, bool close_diag = false);

    /// Get the application's "display" name.
    ///
    /// Get name of this application, suitable for displaying
    /// or for using as the base name for other files.
    /// Will be the 'name' argument of AppMain if given.
    /// Otherwise will be taken from the actual name of the application file
    /// or argv[0].
    const string& GetProgramDisplayName(void) const;

    /// Get the application's executable path.
    ///
    /// The path to executable file of this application. 
    /// Return empty string if not possible to determine this path.
    const string& GetProgramExecutablePath(EFollowLinks follow_links
                                           = eIgnoreLinks)
        const;

    /// Get the program version information.
    CVersionInfo GetVersion(void) const;
    
    /// Check if it is a test run.
    bool IsDryRun(void) const;

    /// Setup application specific diagnostic stream.
    ///
    /// Called from SetupDiag when it is passed the eDS_AppSpecific parameter.
    /// Currently, this calls SetupDiag(eDS_ToStderr) to setup diagonistic
    /// stream to the std error channel.
    /// @return
    ///   TRUE if successful, FALSE otherwise.
    /// @deprecated
    NCBI_DEPRECATED virtual bool SetupDiag_AppSpecific(void);

protected:
    /// Disable argument descriptions.
    ///
    /// Call with a bit flag set for those cmd.line args (std and/or user's)
    /// which you do not want to be parsed.
    /// Note that by default all cmd.line args are enabled.
    enum EDisableArgDesc {
        fDisableStdArgs     = 0x01   ///<  (-h, -logfile, -conffile, -version)
        // TODO: fDisableUserArgs    = 0x02   ///<  user-defined cmd.line args
    };
    typedef int TDisableArgDesc;  ///< Binary OR of "EDisableArgDesc"
    void DisableArgDescriptions(TDisableArgDesc disable = fDisableStdArgs);

    /// Which standard flag's descriptions should not be displayed in
    /// the usage message.
    ///
    /// Do not display descriptions of the standard flags such as the
    ///    -h, -logfile, -conffile, -version
    /// flags in the usage message. Note that you still can pass them in
    /// the command line.
    enum EHideStdArgs {
        fHideHelp     = 0x01,  ///< Hide help description
        fHideLogfile  = 0x02,  ///< Hide log file description
        fHideConffile = 0x04,  ///< Hide configuration file description
        fHideVersion  = 0x08,  ///< Hide version description
        fHideDryRun   = 0x10   ///< Hide dryrun description
    };
    typedef int THideStdArgs;  ///< Binary OR of "EHideStdArgs"

    /// Set the hide mask for the Hide Std Flags.
    void HideStdArgs(THideStdArgs hide_mask);

    /// Flags to adjust standard I/O streams' behaviour.
    enum EStdioSetup {
        fDefault_SyncWithStdio  = 0x01,
        ///< Use compiler-specific default as pertains to the synchronizing
        ///< of "C++" Cin/Cout/Cerr streams with their "C" counterparts.

        fDefault_CinBufferSize  = 0x02,
        ///< Use compiler-specific default of Cin buffer size.
        fBinaryCin   = 0x04,  ///< treat standard  input as binary
        fBinaryCout  = 0x08   ///< treat standard output as binary
    };
    typedef int TStdioSetupFlags;  ///< Binary OR of "EStdioSetup"

    /// Adjust the behavior of standard I/O streams.
    ///
    /// Unless this function is called, the behaviour of "C++" Cin/Cout/Cerr
    /// streams will be the same regardless of the compiler used.
    ///
    /// IMPLEMENTATION NOTE: Do not call this function more than once
    /// and from places other than App's constructor.
    void SetStdioFlags(TStdioSetupFlags stdio_flags);

    /// Set the version number for the program.
    ///
    /// If not set, a default of 0.0.0 (unknown) is used.
    void SetVersion(const CVersionInfo& version);

    /// Setup the command line argument descriptions.
    ///
    /// Call from the Init() method. The passed "arg_desc" will be owned
    /// by this class, and it will be deleted by ~CNcbiApplication(),
    /// or if SetupArgDescriptions() is called again.
    virtual void SetupArgDescriptions(CArgDescriptions* arg_desc);

    /// Get argument descriptions (set by SetupArgDescriptions)
    const CArgDescriptions* GetArgDescriptions(void) const;

    /// Setup the application diagnostic stream.
    /// @return
    ///   TRUE if successful,  FALSE otherwise.
    /// @deprecated
    NCBI_DEPRECATED bool SetupDiag(EAppDiagStream diag);

    /// Load settings from the configuration file to the registry.
    ///
    /// This method is called from inside AppMain() to load (add) registry
    /// settings from the configuration file specified as the "conf" arg
    /// passed to AppMain(). The "conf" argument has the following special
    /// meanings:
    ///  - NULL      -- dont even try to load registry from any file at all;
    ///  - non-empty -- if "conf" contains a path, then try to load from the
    ///                 conf.file of name "conf" (only!). Else - see NOTE.
    ///                 TIP: if the path is not fully qualified then:
    ///                      if it starts from "../" or "./" -- look starting
    ///                      from the current working dir.
    ///  - empty     -- compose conf.file name from the application name
    ///                 plus ".ini". If it does not match an existing
    ///                 file, then try to strip file extensions, e.g. for
    ///                 "my_app.cgi.exe" -- try subsequently:
    ///                   "my_app.cgi.exe.ini", "my_app.cgi.ini", "my_app.ini".
    ///
    /// NOTE:
    /// If "conf" arg is empty or non-empty, but without path, then
    /// the Toolkit will try to look for it in several potentially
    /// relevant directories, as described in <corelib/metareg.hpp>.
    ///
    /// Throw an exception if "conf" is non-empty, and cannot open file.
    /// Throw an exception if file exists, but contains invalid entries.
    /// @param reg
    ///   The loaded registry is returned via the reg parameter.
    /// @param conf
    ///   The configuration file to loaded the registry entries from.
    /// @param reg_flags
    ///   Flags for loading the registry
    /// @return
    ///   TRUE only if the file was non-NULL, found and successfully read.
    /// @sa
    ///   CMetaRegistry::GetDefaultSearchPath
    virtual bool LoadConfig(CNcbiRegistry& reg, const string* conf,
                            CNcbiRegistry::TFlags reg_flags);

    /// Load settings from the configuration file to the registry.
    ///
    /// CNcbiApplication::LoadConfig(reg, conf) just calls
    /// LoadConfig(reg, conf, IRegistry::fWithNcbirc).
    virtual bool LoadConfig(CNcbiRegistry& reg, const string* conf);

    /// Set program's display name.
    ///
    /// Set up application name suitable for display or as a basename for
    /// other files. It can also be set by the user when calling AppMain().
    void SetProgramDisplayName(const string& app_name);

    /// Find the application's executable file.
    ///
    /// Find the path and name of the executable file that this application is
    /// running from. Will be accessible by GetArguments().GetProgramName().
    /// @param argc
    ///   Standard argument count "argc".
    /// @param argv
    ///   Standard argument vector "argv".
    /// @param real_path
    ///   If non-NULL, will get the fully resolved path to the executable.
    /// @return
    ///   Name of application's executable file (may involve symlinks).
    string FindProgramExecutablePath(int argc, const char* const* argv,
                                     string* real_path = 0);

    /// Method to be called before application start.
    /// Can be used to set DiagContext properties to be printed
    /// in the application start message (e.g. host|host_ip_addr,
    /// client_ip and session_id for CGI applications).
    virtual void AppStart(void);

    /// Method to be called before application exit.
    /// Can be used to set DiagContext properties to be printed
    /// in the application stop message (exit_status, exit_signal,
    /// exit_code).
    virtual void AppStop(int exit_code);

    /// When to return a user-set exit code
    enum EExitMode {
        eNoExits,          ///< never (stick to existing logic)
        eExceptionalExits, ///< when an (uncaught) exception occurs
        eAllExits          ///< always (ignoring Run's return value)
    };
    /// Force the program to return a specific exit code later, either
    /// when it exits due to an exception or unconditionally.  In the
    /// latter case, Run's return value will be ignored.
    void SetExitCode(int exit_code, EExitMode when = eExceptionalExits);

private:
    /// Read standard NCBI application configuration settings.
    ///
    /// [NCBI]:   HeapSizeLimit, CpuTimeLimit
    /// [DEBUG]:  ABORT_ON_THROW, DIAG_POST_LEVEL, MessageFile
    /// @param reg
    ///   Registry to read from. If NULL, use the current registry setting.
    void x_HonorStandardSettings(IRegistry* reg = 0);

    /// Setup C++ standard I/O streams' behaviour.
    ///
    /// Called from AppMain() to do compiler-specific optimization
    /// for C++ I/O streams. For example, since SUN WorkShop STL stream
    /// library has significant performance loss when sync_with_stdio is
    /// TRUE (default), so we turn it off. Another, for GCC version greater
    /// than 3.00 we forcibly set cin stream buffer size to 4096 bytes -- which
    /// boosts the performance dramatically.
    void x_SetupStdio(void);

    static CNcbiApplication*   m_Instance;   ///< Current app. instance
    auto_ptr<CVersionInfo>     m_Version;    ///< Program version
    auto_ptr<CNcbiEnvironment> m_Environ;    ///< Cached application env.
    CRef<CNcbiRegistry>        m_Config;     ///< Guaranteed to be non-NULL
    auto_ptr<CNcbiOstream>     m_DiagStream; ///< Opt., aux., see eDS_ToMemory
    auto_ptr<CNcbiArguments>   m_Arguments;  ///< Command-line arguments
    auto_ptr<CArgDescriptions> m_ArgDesc;    ///< Cmd.-line arg descriptions
    auto_ptr<CArgs>            m_Args;       ///< Parsed cmd.-line args
    TDisableArgDesc            m_DisableArgDesc;  ///< Arg desc. disabled
    THideStdArgs               m_HideArgs;   ///< Std cmd.-line flags to hide
    TStdioSetupFlags           m_StdioFlags; ///< Std C++ I/O adjustments
    char*                      m_CinBuffer;  ///< Cin buffer if changed
    string                     m_ProgramDisplayName;  ///< Display name of app
    string                     m_ExePath;    ///< Program executable path
    string                     m_RealExePath; ///< Symlink-free executable path
    mutable string             m_LogFileName; ///< Log file name
    string                     m_ConfigPath;  ///< Path to .ini file used
    int                        m_ExitCode;    ///< Exit code to force
    EExitMode                  m_ExitCodeCond; ///< When to force it (if ever)
    bool                       m_DryRun;       ///< Dry run
};


/* @} */



/////////////////////////////////////////////////////////////////////////////
//  IMPLEMENTATION of INLINE functions
/////////////////////////////////////////////////////////////////////////////


inline const CNcbiArguments& CNcbiApplication::GetArguments(void) const
{
    return *m_Arguments;
}

inline const CNcbiEnvironment& CNcbiApplication::GetEnvironment(void) const
{
    return *m_Environ;
}

inline CNcbiEnvironment& CNcbiApplication::SetEnvironment(void)
{
    return *m_Environ;
}

inline const CNcbiRegistry& CNcbiApplication::GetConfig(void) const
{
    return *m_Config;
}

inline CNcbiRegistry& CNcbiApplication::GetConfig(void)
{
    return *m_Config;
}

inline const string& CNcbiApplication::GetConfigPath(void) const
{
    return m_ConfigPath;
}

inline bool CNcbiApplication::HasLoadedConfig(void) const
{
    return !m_ConfigPath.empty();
}

inline bool CNcbiApplication::ReloadConfig(CMetaRegistry::TFlags flags,
                                           IRegistry::TFlags reg_flags)
{
    return CMetaRegistry::Reload(GetConfigPath(), GetConfig(), flags,
                                 reg_flags);
}

inline const string& CNcbiApplication::GetProgramDisplayName(void) const
{
    return m_ProgramDisplayName;
}

inline const string&
CNcbiApplication::GetProgramExecutablePath(EFollowLinks follow_links) const
{
    return follow_links == eFollowLinks ? m_RealExePath : m_ExePath;
}


inline const CArgDescriptions* CNcbiApplication::GetArgDescriptions(void) const
{
    return m_ArgDesc.get();
}


inline bool CNcbiApplication::IsDryRun(void) const
{
    return m_DryRun;
}


END_NCBI_SCOPE

#endif  /* CORELIB___NCBIAPP__HPP */
