#ifndef CORELIB___PLUGIN_MANAGER__HPP
#define CORELIB___PLUGIN_MANAGER__HPP

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
 * Author:  Denis Vakatov, Anatoliy Kuznetsov
 *
 * File Description:  Plugin manager (using class factory paradigm)
 *
 */

/// @file plugin_manager.hpp
/// Plugin manager (using class factory paradigm).
///
/// Describe generic interface and provide basic functionality to advertise
/// and export a class factory. 
/// The class and class factory implementation code can be linked to
/// either statically (then, the class factory will need to be registered
/// explicitly by the user code) or dynamically (then, the DLL will be
/// searched for using plugin name, and the well-known DLL entry point
/// will be used to register the class factory, automagically).
/// 
/// - "class factory" -- An entity used to generate objects of the given class.
///                      One class factory can generate more than one version
///                      of the class.
/// 
/// - "interface"  -- Defines the implementation-independent API and expected
///                   behaviour of a class.
///                   Interface's name is provided by its class's factory,
///                   see IClassFactory::GetInterfaceName().
///                   Interfaces are versioned to track the compatibility.
/// 
/// - "driver"  -- A concrete implementation of the interface and its factory.
///                Each driver has its own name (do not confuse it with the
///                interface name!) and version.
/// 
/// - "host"    -- An entity (DLL or the EXEcutable itself) that contains
///                one or more drivers (or versions of the same driver),
///                which can implement one or more interfaces.
/// 
/// - "version" -- MAJOR (backward- and forward-incompatible changes in the
///                       interface and/or its expected behaviour);
///                MINOR (backward compatible changes in the driver code);
///                PATCH_LEVEL (100% compatible plugin or driver code changes).
/// 

#include <corelib/ncbimtx.hpp>
#include <corelib/version.hpp>


/** @addtogroup PluginMgr
 *
 * @{
 */


BEGIN_NCBI_SCOPE


/// CInterfaceVersion<> --
///
/// Current interface version.
///
/// It is just a boilerplate, to be hard-coded in the interface's header

template <class TClass>
class CInterfaceVersion
{
};

/*****  Example:

EMPTY_TEMPLATE
class CInterfaceVersion<CFooBar>
{
public:
    enum {
        eMajor = 1,  // absolute (in)compatibility
        eMinor = 2,  // backward (in)compatibility
        ePatch = 3   // patch level
    };
};

******/



/// IInterfaceInfo<> --
///
/// Interface properties -- name, version.

template <class TClass>
class IInterfaceInfo
{
public:
    // Name of the interface provided by the factory
    virtual string GetName(void) = 0;

    // Versions of the interface exported by the factory
    virtual list<const CVersionInfo&> GetVersions(void) = 0;

    virtual ~IInterfaceInfo(void) {}
};



/// IClassFactory<> --
///
/// Class factory for the given interface.
///
/// It is to be implemented in drivers and exported by hosts.

template <class TClass>
class IClassFactory : public IInterfaceInfo<TClass>
{
public:
    /// Create class instance
    ///
    /// @param version
    ///  Requested version (as understood by the caller).
    ///  By default it will be passed the version which is current from
    ///  the calling code's point of view.
    /// @return
    ///  NULL on any error (not found entry point, version mismatch, etc.)
    virtual TClass* CreateInstance(const CVersionInfo& version = CVersionInfo
                                   (CInterfaceVersion<TClass>::eMajor,
                                    CInterfaceVersion<TClass>::eMinor,
                                    CInterfaceVersion<TClass>::ePatchLevel))
        = 0;

    virtual ~IClassFactory(void) {}
};



/// CPluginManager<> --
///
/// To register (either directly, or via an "entry point") class factories
/// for the given interface.
///
/// Then, facilitate the process of instantiating the class given
/// the registered pool of drivers, and also taking into accont the driver name
/// and/or version as requested by the calling code.

template <class TClass>
class NCBI_XNCBI_EXPORT CPluginManager
{
public:
    typedef IClassFactory<TClass> TClassFactory;

    /// Create class instance
    /// @sa GetFactory()
    TClass* CreateInstance(const string&       driver  = kEmptyStr,
                           const CVersionInfo& version = CVersionInfo
                           (CInterfaceVersion<TClass>::eMajor,
                            CInterfaceVersion<TClass>::eMinor,
                            CInterfaceVersion<TClass>::ePatchLevel))
    {
        return GetFactory(name, version)->CreateInstance(version);
    }

    /// Get class factory
    ///
    /// If more than one driver is eligible, then try to pick the driver with
    /// the latest version.
    /// @param driver
    ///  Name of the driver. If passed empty, then -- any.
    /// @param version
    ///  Version of the interface which the class factory implemented by
    ///  the driver must be compatible with.
    /// @return
    ///  Never return NULL -- always throw exception on error.
    TClassFactory* GetFactory(const string&       driver  = kEmptyStr,
                              const CVersionInfo& version = CVersionInfo
                              (CInterfaceVersion<TClass>::eMajor,
                               CInterfaceVersion<TClass>::eMinor,
                               CInterfaceVersion<TClass>::ePatchLevel))
    {
        // ...TODO...
    }

    /// 
    class CFactoryInfo {
        TClassFactory* m_Factory;     //!< Class factory (can be NULL)
        string         m_DriverName;  //!< Driver name
        CVersionInfo*  m_Version;     //!< Driver version (

        SFactoryInfo(TClassFactory*      x_factory,
                     const string&       x_driver) :
        SFactoryInfo(const string&       x_driver,
                     const CVersionInfo& x_version) :
        factory(x_factory), driver(x_driver), version(x_version) {}
            
    };

    /// List of information
    typedef list<SFactoryInfo> TFactoryInfoList;

    /// Register factory in the manager.
    /// @param factory_info
    ///  
    void RegisterFactory(const SFactoryInfo& factory_info,
                         EOwnership          ownership = eTakeOwnership);


    /// Plugin info entry point -- modes of operation
    enum EEntryPointRequest {
        /// Add info about plugins to the end of list.
        ///
        /// "SFactoryInfo::factory" in the added info should be assigned NULL.
        eGetFactoryInfo,

        /// Scan info list for the [name,version] pairs exported
        /// by the given entry point.
        ///
        /// For each pair found, if its "SFactoryInfo::factory" is NULL,
        /// instantiate its class factory and assign it to the
        /// "SFactoryInfo::factory".
        eInstantiateFactory
    };

    /// Entry point to get plugin info, and (if requested) class factory.
    ///
    /// This function is usually (but not necessarily) called by
    /// RegisterWithEntryPoint().
    ///
    /// Usually, it's called twice -- the first time to get the info
    /// about the class factories exported by the entry point, and then
    /// to instantiate selected factories.
    ///
    /// Caller is responsible for the proper destruction (deallocation)
    /// of the instantiated factories.
    typedef void (*FNCBI_EntryPoint)(TFactoryInfoList&   info_list,
                                     EEntryPointRequest  method);

    /// Register all factories exported by the plugin entry point.
    /// @sa RegisterFactory()
    void RegisterWithEntryPoint(FNCBI_EntryPoint plugin_entry_point);

    // ctors
    CPluginManager(void);
    virtual ~CPluginManager();

protected:
    /// Protective mutex to syncronize the access to the plugin manager
    /// from different threads
    CFastMutex m_Mutex;

private:
    /// List of factories presently registered with the plugin manager.
    TFactoryInfoList m_FactoryInfo;      // NOTE: intentionally not multimap<> 
};




/// CDllResolver --
///
/// Class to resolve DLL by the plugin's class name, driver name, and version.
/// Also allows to filter for and get entry point for CPluginManager.

class NCBI_XNCBI_EXPORT CDllResolver
{
public:
    /// Return list of absolute DLL names matching the prugin's class name,
    /// driver name, and driver version.
    virtual string<list> Resolve
    (const string&       plugin,
     const string&       driver,
     const CVersionInfo& version = CVersionInfo::kAny);

    /// Adjustment of the DLL search path
    /// @sa SetDllSearchPath()
    enum ESetPath {
        eOverride,  //<! Override the existing DLL search path with "path"
        ePrepend,   //<! Add "path" to the head of the existing DLL search path
        eAppend     //<! Add "path" to the tail of the existing DLL search path
    };

    /// Change DLL search path.
    ///
    /// The default search paths are regular system-dependent DLL search paths.
    void SetDllSearchPath(const string& path, EModifyDllPath method);

protected:
    /// Compose a list of possible DLL names based on the plugin's class, name
    /// and version. 
    ///
    /// Legal return formats:
    ///  - Platform-independent form -- no path or dots in the DLL name, e.g.
    ///    "ncbi_plugin_dbapi_ftds_3_1_7".
    ///    In this case, the DLL will be searched for in the standard
    ///    DLL search paths, with automatic addition of any platform-specific
    ///    prefixes and suffixes.
    ///
    ///  - Platform-dependent form, with path search -- no path is specified
    ///    in the DLL name, e.g. "dbapi_plugin_dbapi_ftds.so.3.1.7".
    ///    In this case, the DLL will be searched for in the standard
    ///    DLL search paths "as is", without any automatic addition of
    ///    platform-specific prefixes and suffixes.
    ///
    ///  - Platform-dependent form, with fixed absolute path, e.g.
    ///    "/foo/bar/dir/dbapi_plugin_dbapi_ftds.so.3.1.7".
    ///
    /// Default DLL name list (depending of whether full or partial version
    /// and driver name are specified):
    ///  - "<GetDllPrefix()><plugin>_<driver>_<version>", e.g.
    ///     "ncbi_plugin_dbapi_ftds_3_1_7", "ncbi_plugin_dbapi_ftds_2"
    ///  - "<GetDllPrefix()><plugin>_<driver>.so.<version>"  (UNIX-only), e.g.
    ///     "ncbi_plugin_dbapi_ftds.so.3.1.7", "ncbi_plugin_dbapi_ftds.so.2"
    ///  - "<GetDllPrefix()><plugin>_<driver>", e.g. "ncbi_plugin_dbapi_ftds"
    ///  - "<GetDllPrefix()><plugin>", e.g. "ncbi_plugin_dbapi"
    virtual list<string> GetDllNames
    (const string&       plugin,
     const string&       driver  = kEmptyStr,
     const CVersionInfo& version = CVersionInfo::kAny);

    /// 
    ///
    /// Default:  "ncbi_plugin_"
    virtual string GetDllPrefix(void);

    // ctors
    CDllResolver(void);
    virtual ~CDllResolver();

private:
    /// List of dirs to scan in for the DLLs
    string m_DllPath;
};



/// CDllResolver_PluginManager<> --
///
/// Class to resolve DLL by the plugin's class name, driver name, and version.
/// Also allows to filter for and get entry point for CPluginManager.

template <class TClass, class TDllResolver = CDllResolver>
class NCBI_XNCBI_EXPORT CDllResolver_PluginManager : public TDllResolver
{
public:
    /// 
    



protected:
    // default:
    //  - "NCBI_EntryPoint_<TClassFactory::GetName()>_<name>"
    virtual string GetEntryPointName
    (const string&       name    = kEmptyStr,
     const CVersionInfo& version = CVersionInfo::kAny);

    // ctors
    CDllResolver_PluginManager(void) :
        CDllResolver() {}
    virtual ~CDllResolver_PluginManager() {}
};



END_NCBI_SCOPE


/* @} */


/*
 * ===========================================================================
 * $Log$
 * Revision 1.1  2003/10/28 00:12:23  vakatov
 * Initial revision
 *
 * Work-in-progress, totally unfinished.
 * Note: DLL resolution shall be split and partially moved to the NCBIDLL.
 *
 * ===========================================================================
 */

#endif  /* CORELIB___PLUGIN_MANAGER__HPP */
