#if defined(NCBIDIAG__HPP)  &&  !defined(NCBIDIAG__INL)
#define NCBIDIAG__INL

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
* Author:  Denis Vakatov
*
* File Description:
*   NCBI C++ diagnostic API
*
* --------------------------------------------------------------------------
* $Log$
* Revision 1.8  1998/12/30 21:52:17  vakatov
* Fixed for the new SunPro 5.0 beta compiler that does not allow friend
* templates and member(in-class) templates
*
* Revision 1.7  1998/11/05 00:00:42  vakatov
* Fix in CDiagBuffer::Reset() to avoid "!=" ambiguity when using
* new(templated) streams
*
* Revision 1.6  1998/11/04 23:46:36  vakatov
* Fixed the "ncbidbg/diag" header circular dependencies
*
* Revision 1.5  1998/11/03 22:28:33  vakatov
* Renamed Die/Post...Severity() to ...Level()
*
* Revision 1.4  1998/11/03 20:51:24  vakatov
* Adaptation for the SunPro compiler glitchs(see conf. #NO_INCLASS_TMPL)
*
* Revision 1.3  1998/10/30 20:08:25  vakatov
* Fixes to (first-time) compile and test-run on MSVS++
*
* Revision 1.2  1998/10/27 23:08:15  vakatov
* Finished with a draft redesign(from 1.1)
*
* Revision 1.1  1998/10/24 00:25:43  vakatov
* Initial revision(incomplete transition from "ncbimsg.inl")
*
* ==========================================================================
*/


/////////////////////////////////////////////////////////////////////////////
// WARNING -- all the beneath is for INTERNAL "ncbidiag" use only,
//            and any classes, typedefs and even "extern" functions and
//            variables declared in this file should not be used anywhere
//            but inside "ncbidiag.inl" and/or "ncbidiag.cpp"!!!
/////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////
// CDiagBuffer
// (can be accessed only by "CNcbiDiag" and created only by GetDiagBuffer())
//

class CDiagBuffer {
    friend CDiagBuffer& GetDiagBuffer(void);
#if !defined(NO_INCLASS_TMPL)
    friend class CNcbiDiag;
    friend CNcbiDiag& Reset(CNcbiDiag& diag);
    friend CNcbiDiag& Endm(CNcbiDiag& diag);
    friend EDiagSev SetDiagPostLevel(EDiagSev post_sev);
    friend EDiagSev SetDiagDieLevel(EDiagSev die_sev);
    friend void SetDiagHandler(FDiagHandler func, void* data,
                               FDiagCleanup cleanup);
private:
#else
public:
#endif

    const CNcbiDiag* m_Diag;    // present user
    CNcbiOstrstream  m_Stream;  // storage for the diagnostic message

    CDiagBuffer(void);

    //### This is temporary workaround to allow call the destructor of
    //### static instance of "CDiagBuffer" defined in GetDiagBuffer()
public:
    ~CDiagBuffer(void);

#if !defined(NO_INCLASS_TMPL)
private:
#endif
    //###

#if !defined(NO_INCLASS_TMPL)
    // formatted output
    template<class X> void Put(const CNcbiDiag& diag, const X& x) {
        if (diag.GetSeverity() < sm_PostSeverity)
            return;
        if (m_Diag != &diag) {
            if ( m_Stream.pcount() )
                Flush();
            m_Diag = &diag;
        }
        m_Stream << x;
    }
#endif  /* !NO_INCLASS_TMPL */

    void Flush  (void);
    void Reset  (const CNcbiDiag& diag);  // reset content of the diag. message
    void EndMess(const CNcbiDiag& diag);  // output current diag. message

    // flush & detach the current user
    void Detach(const CNcbiDiag* diag);

    // (NOTE:  these two dont need to be protected by mutexes because it is not
    //  critical -- while not having a mutex around would save us a little
    //  performance)
    static EDiagSev sm_PostSeverity;
    static EDiagSev sm_DieSeverity;

    // call the current diagnostics handler directly
    static void DiagHandler(EDiagSev    sev,
                            const char* message_buf,
                            size_t      message_len);

    // Symbolic name for the severity levels(used by CNcbiDiag::SeverityName)
    static const char* SeverityName[eDiag_Trace+1];

    // Application-wide diagnostic handler & Co.
    static FDiagHandler sm_HandlerFunc;
    static void*        sm_HandlerData;
    static FDiagCleanup sm_HandlerCleanup;
};



///////////////////////////////////////////////////////
//  CNcbiDiag::

inline CNcbiDiag::CNcbiDiag(EDiagSev sev, const char* message, bool flush)
 : m_Buffer(GetDiagBuffer())
{
    m_Severity = sev;
    if ( message )
        (*this) << message;
    if ( flush )
        (*this) << Endm;
}

inline CNcbiDiag::~CNcbiDiag(void) {
    m_Buffer.Detach(this);
}

inline EDiagSev CNcbiDiag::GetSeverity(void) const {
    return m_Severity;
}

#if defined(NO_INCLASS_TMPL)
template<class X> void Put(CNcbiDiag& diag, CDiagBuffer& dbuff, const X& x) {
    if (diag.GetSeverity() < dbuff.sm_PostSeverity)
        return;
    if (dbuff.m_Diag != &diag) {
        if ( dbuff.m_Stream.pcount() )
            dbuff.Flush();
        dbuff.m_Diag = &diag;
    }
    dbuff.m_Stream << x;
}

template<class X> CNcbiDiag& operator <<(CNcbiDiag& diag, const X& x) {
    Put(diag, diag.m_Buffer, x);
    return diag;
}
#endif /* NO_INCLASS_TMPL */

inline const char* CNcbiDiag::SeverityName(EDiagSev sev) {
    return CDiagBuffer::SeverityName[sev];
}


///////////////////////////////////////////////////////
//  CNcbiDiag:: manipulators

inline CNcbiDiag& Reset(CNcbiDiag& diag)  {
    diag.m_Buffer.Reset(diag);
    return diag;
}

inline CNcbiDiag& Endm(CNcbiDiag& diag)  {
    diag.m_Buffer.EndMess(diag);
    return diag;
}

inline CNcbiDiag& Info(CNcbiDiag& diag)  {
    diag << Endm;
    diag.m_Severity = eDiag_Info;
    return diag;
}
inline CNcbiDiag& Warning(CNcbiDiag& diag)  {
    diag << Endm;
    diag.m_Severity = eDiag_Warning;
    return diag;
}
inline CNcbiDiag& Error(CNcbiDiag& diag)  {
    diag << Endm;
    diag.m_Severity = eDiag_Error;
    return diag;
}
inline CNcbiDiag& Fatal(CNcbiDiag& diag)  {
    diag << Endm;
    diag.m_Severity = eDiag_Fatal;
    return diag;
}
inline CNcbiDiag& Trace(CNcbiDiag& diag)  {
    diag << Endm;
    diag.m_Severity = eDiag_Trace;
    return diag;
}



///////////////////////////////////////////////////////
//  CDiagBuffer::

inline CDiagBuffer::CDiagBuffer(void) {
    m_Diag = 0;
}

inline CDiagBuffer::~CDiagBuffer(void) {
    if (m_Diag  ||  m_Stream.pcount())
        abort();
}

inline void CDiagBuffer::Flush(void) {
    if ( !m_Diag )
        return;

    EDiagSev sev = m_Diag->GetSeverity();
    if ( m_Stream.pcount() ) {
        const char* message = m_Stream.str();
        m_Stream.rdbuf()->freeze(0);
        DiagHandler(sev, message, m_Stream.pcount());
        Reset(*m_Diag);
    }
    if (sev >= sm_DieSeverity  &&  sev != eDiag_Trace)
        abort();
}

inline void CDiagBuffer::Reset(const CNcbiDiag& diag) {
    if (&diag == m_Diag  &&
        m_Stream.rdbuf()->SEEKOFF(0, IOS_BASE::beg, IOS_BASE::out))
        abort();
}

inline void CDiagBuffer::EndMess(const CNcbiDiag& diag) {
    if (&diag == m_Diag)
        Flush();
}

inline void CDiagBuffer::Detach(const CNcbiDiag* diag) {
    if (diag == m_Diag) {
        Flush();
        m_Diag = 0;
    }
}


#endif /* def NCBIDIAG__HPP  &&  ndef NCBIDIAG__INL */
