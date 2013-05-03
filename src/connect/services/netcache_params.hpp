#ifndef CONN___NETCACHE_PARAMS__HPP
#define CONN___NETCACHE_PARAMS__HPP

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
 * Authors:  Dmitry Kazimirov
 *
 * File Description:
 *   NetCache API parameters (declarations).
 *
 */

/// @file netcache_params.hpp
/// NetCache client parameters.
///

#include <connect/services/netcache_api.hpp>

BEGIN_NCBI_SCOPE


/** @addtogroup NetCacheClient
 *
 * @{
 */

class CNetCacheAPIParameters
{
public:
    enum EDefinedParameter {
        eDP_TTL = 1 << 0,
        eDP_CachingMode = 1 << 1,
        eDP_MirroringMode = 1 << 2,
        eDP_ServerCheck = 1 << 3,
        eDP_ServerCheckHint = 1 << 4,
        eDP_Password = 1 << 5
    };
    typedef unsigned TDefinedParameters;

    // EVoid is only to avoid having a default constructor.
    CNetCacheAPIParameters(EVoid) :
        m_DefinedParameters(
            eDP_TTL |
            eDP_CachingMode |
            eDP_MirroringMode |
            eDP_Password),
        m_Defaults(NULL),
        m_TTL(0),
        m_CachingMode(CNetCacheAPI::eCaching_Disable),
        m_MirroringMode(CNetCacheAPI::eIfKeyMirrored),
        m_ServerCheck(eDefault),
        m_ServerCheckHint(true)
    {
    }

    CNetCacheAPIParameters(const CNetCacheAPIParameters* defaults);

    void LoadNamedParameters(const CNamedParameterList* optional);

    void SetTTL(unsigned blob_ttl);

    void SetCachingMode(CNetCacheAPI::ECachingMode caching_mode)
    {
        m_DefinedParameters |= eDP_CachingMode;
        m_CachingMode = caching_mode;
    }

    void SetMirroringMode(CNetCacheAPI::EMirroringMode mirroring_mode)
    {
        m_DefinedParameters |= eDP_MirroringMode;
        m_MirroringMode = mirroring_mode;
    }

    void SetServerCheck(ESwitch server_check)
    {
        m_DefinedParameters |= eDP_ServerCheck;
        m_ServerCheck = server_check;
    }

    void SetServerCheckHint(bool server_check_hint)
    {
        m_DefinedParameters |= eDP_ServerCheckHint;
        m_ServerCheckHint = server_check_hint;
    }

    void SetPassword(const string& password);

    unsigned GetTTL() const;
    CNetCacheAPI::ECachingMode GetCachingMode() const;
    CNetCacheAPI::EMirroringMode GetMirroringMode() const;
    bool GetServerCheck(ESwitch* server_check) const;
    bool GetServerCheckHint(bool* server_check_hint) const;
    std::string GetPassword() const;

private:
    TDefinedParameters m_DefinedParameters;
    const CNetCacheAPIParameters* m_Defaults;

    unsigned m_TTL;
    CNetCacheAPI::ECachingMode m_CachingMode;
    CNetCacheAPI::EMirroringMode m_MirroringMode;
    ESwitch m_ServerCheck;
    bool m_ServerCheckHint;
    std::string m_Password;
};

/* @} */


END_NCBI_SCOPE

#endif  /* CONN___NETCACHE_PARAMS__HPP */
