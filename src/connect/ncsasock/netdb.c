/*
 * BSD-style socket emulation library for the Mac
 * Original author: Tom Milligan
 * Current author: Charlie Reiman - creiman@ncsa.uiuc.edu
 *
 * This source file is placed in the public domian.
 * Any resemblance to NCSA Telnet, living or dead, is purely coincidental.
 *
 *      National Center for Supercomputing Applications
 *      152 Computing Applications Building
 *      605 E. Springfield Ave.
 *      Champaign, IL  61820
 */

/*
 *	Internet name routines that every good unix program uses...
 *
 *		gethostbyname
 *		gethostbyaddr
 *      gethostid
 *		gethostname
 *		getdomainname
 *		inet_addr
 *		inet_ntoa
 *		getservbyname
 *		getprotobyname
 */
 
#ifdef USEDUMP
# pragma load "Socket.dump"
#else
# include <Stdio.h>
# include <Types.h>
# include <Resources.h>
# include <Errors.h>
# include <OSUtils.h>

# include <s_types.h>
# include <netdb.h>
# include <neti_in.h>
# include <s_socket.h>
# include <s_time.h>
# include <neterrno.h>

# include "sock_str.h"
# include "sock_int.h"
#endif

#include <Ctype.h>
#include <a_namesr.h>
#include <s_param.h>
#include <sock_ext.h>


extern SocketPtr sockets;
extern SpinFn spinroutine;
pascal void DNRDone(struct hostInfo *hostinfoPtr,Boolean *done);

int h_errno;

/*
 *   Gethostbyname and gethostbyaddr each return a pointer to an
 *   object with the following structure describing an Internet
 *   host referenced by name or by address, respectively. This
 *   structure contains the information obtained from the MacTCP
 *   name server.
 *
 *   struct    hostent 
 *   {
 *        char *h_name;
 *        char **h_aliases;
 *        int  h_addrtype;
 *        int  h_length;
 *        char **h_addr_list;
 *   };
 *   #define   h_addr  h_addr_list[0]
 *
 *   The members of this structure are:
 *
 *   h_name       Official name of the host.
 *
 *   h_aliases    A zero terminated array of alternate names for the host.
 *
 *   h_addrtype   The type of address being  returned; always AF_INET.
 *
 *   h_length     The length, in bytes, of the address.
 *
 *   h_addr_list  A zero terminated array of network addresses for the host.
 *
 *   Error return status from gethostbyname and gethostbyaddr  is
 *   indicated by return of a null pointer.  The external integer
 *   h_errno may then  be checked  to  see  whether  this  is  a
 *   temporary  failure  or  an  invalid  or  unknown  host.  The
 *   routine herror  can  be  used  to  print  an error  message
 *   describing the failure.  If its argument string is non-NULL,
 *   it is printed, followed by a colon and a space.   The  error
 *   message is printed with a trailing newline.
 *
 *   h_errno can have the following values:
 *
 *     HOST_NOT_FOUND  No such host is known.
 *
 *     TRY_AGAIN	This is usually a temporary error and
 *					means   that  the  local  server  did  not
 *					receive a response from  an  authoritative
 *					server.   A  retry at some later time may
 *					succeed.
 *
 *     NO_RECOVERY	Some unexpected server failure was encountered.
 *	 				This is a non-recoverable error.
 *
 *     NO_DATA		The requested name is valid but  does  not
 *					have   an IP  address;  this  is not  a
 *					temporary error. This means that the  name
 *					is known  to the name server but there is
 *					no address  associated  with  this  name.
 *					Another type of request to the name server
 *					using this domain name will result in  an
 *					answer;  for example, a mail-forwarder may
 *					be registered for this domain.
 *					(NOT GENERATED BY THIS IMPLEMENTATION)
 */

static struct hostInfo macHost;

#define MAXALIASES 0
static char *aliasPtrs[MAXALIASES+1] = {NULL};
static ip_addr *addrPtrs[NUM_ALT_ADDRS+1];

static struct hostent  unixHost = 
{
	macHost.cname,
	aliasPtrs,
	AF_INET,
	sizeof(ip_addr),
	(char **) addrPtrs
};

struct hostent *
gethostbyname(char *name)
{
	Boolean done;
	int i;
	OSErr rsult;
#ifdef __MACTCP__
	ResultUPP proc;
#endif
	
#if	NETDB_DEBUG >= 3
	dprintf("gethostbyname: '%s'\n",name);
#endif

	sock_init();

	for (i=0; i<NUM_ALT_ADDRS; i++)
		macHost.addr[i] = 0;
	done = false;
#ifdef __MACTCP__
  proc = NewResultProc (DNRDone);
  rsult = StrToAddr(name,&macHost,proc,(char *) &done);
  DisposeRoutineDescriptor (proc);
#else
  rsult = StrToAddr(name,&macHost,(ResultProcPtr) DNRDone,(char *) &done);
#endif
	if (rsult == cacheFault)
	{
#if NETDB_DEBUG >= 5
		dprintf("gethostbyname: spinning\n");
#endif
		SPINP(!done,SP_NAME,0L)

#if NETDB_DEBUG >= 5
		dprintf("gethostbyname: done spinning\n");
#endif
	}
	switch (macHost.rtnCode)
	{
		case noErr: break;
		
		case nameSyntaxErr:	h_errno = HOST_NOT_FOUND;	return(NULL);
		case cacheFault:	h_errno = NO_RECOVERY;		return(NULL);
		case noResultProc:	h_errno = NO_RECOVERY;		return(NULL);
		case noNameServer:	h_errno = HOST_NOT_FOUND;	return(NULL);
		case authNameErr:	h_errno = HOST_NOT_FOUND;	return(NULL);
		case noAnsErr:		h_errno = TRY_AGAIN;		return(NULL);
		case dnrErr:		h_errno = NO_RECOVERY;		return(NULL);
		case outOfMemory:	h_errno = TRY_AGAIN;		return(NULL);
		case notOpenErr:	h_errno = NO_RECOVERY;		return(NULL);
		default:			h_errno = NO_RECOVERY;		return(NULL);
	}
	
#if NETDB_DEBUG >= 5
	dprintf("gethostbyname: name '%s' addrs %08x %08x %08x %08x\n",
			macHost.cname,
			macHost.addr[0],macHost.addr[1],
			macHost.addr[2],macHost.addr[3]);
#endif

	/* was the 'name' an IP address? */
	if (macHost.cname[0] == 0)
	{
		h_errno = HOST_NOT_FOUND;
		return(NULL);
	}
	
	/* for some reason there is a dot at the end of the name */
	i = strlen(macHost.cname) - 1;
	if (macHost.cname[i] == '.')
		macHost.cname[i] = 0;
	
	for (i=0; i<NUM_ALT_ADDRS && macHost.addr[i]!=0; i++)
	{
		addrPtrs[i] = &macHost.addr[i];
	}
	addrPtrs[i] = NULL;
	
	return(&unixHost);
}

struct hostent *
gethostbyaddr(ip_addr *addrP,int len,int type)
{
#pragma unused(len)
#pragma unused(type)
	Boolean done;
	int i;
	OSErr rsult;
#ifdef __MACTCP__
	ResultUPP proc;
#endif
	
#if	NETDB_DEBUG >= 3
	dprintf("gethostbyaddr: %08x\n",*addrP);
#endif

	sock_init();
	
	for (i=0; i<NUM_ALT_ADDRS; i++)
		macHost.addr[i] = 0;
	done = false;
#ifdef __MACTCP__
  proc = NewResultProc (DNRDone);
  rsult = AddrToName(*addrP,&macHost,proc,(char *) &done);
  DisposeRoutineDescriptor (proc);
#else
  rsult = AddrToName(*addrP,&macHost,(ResultProcPtr) DNRDone,(char *) &done);
#endif
	if (rsult == cacheFault)
	{
#if NETDB_DEBUG >= 5
		dprintf("gethostbyaddr: spinning\n");
#endif

		SPINP(!done,SP_ADDR,0L)
#if NETDB_DEBUG >= 5
		dprintf("gethostbyaddr: done spinning\n");
#endif
	}
	switch (macHost.rtnCode)
	{
		case noErr: break;
		
		case cacheFault:	h_errno = NO_RECOVERY;		return(NULL);
		case noNameServer:	h_errno = HOST_NOT_FOUND;	return(NULL);
		case authNameErr:	h_errno = HOST_NOT_FOUND;	return(NULL);
		case noAnsErr:		h_errno = TRY_AGAIN;		return(NULL);
		case dnrErr:		h_errno = NO_RECOVERY;		return(NULL);
		case outOfMemory:	h_errno = TRY_AGAIN;		return(NULL);
		case notOpenErr:	h_errno = NO_RECOVERY;		return(NULL);
		default:			h_errno = NO_RECOVERY;		return(NULL);
	}
#if NETDB_DEBUG >= 5
	dprintf("gethostbyaddr: name '%s' addrs %08x %08x %08x %08x\n",
			macHost.cname,
			macHost.addr[0],macHost.addr[1],
			macHost.addr[2],macHost.addr[3]);
#endif
	/* for some reason there is a dot at the end of the name */
	i = strlen(macHost.cname) - 1;
	if (macHost.cname[i] == '.')
		macHost.cname[i] = 0;
	
	for (i=0; i<NUM_ALT_ADDRS; i++)
	{
		addrPtrs[i] = &macHost.addr[i];
	}
	addrPtrs[NUM_ALT_ADDRS] = NULL;
	
	return(&unixHost);
}

char *
inet_ntoa(ip_addr inaddr)
{
	sock_init();
	
	(void) AddrToStr(inaddr,macHost.cname);
	return(macHost.cname);
}

ip_addr 
inet_addr(char *address)
{
#ifndef JAE
OSErr retval;
#endif
	sock_init();
#ifndef JAE
	if ((retval = StrToAddr(address,&macHost,NULL,NULL)) != noErr)
#else


	if (StrToAddr(address,&macHost,NULL,NULL) != noErr)
#endif
		return((ip_addr)-1);
	
#if NETDB_DEBUG >= 5
	dprintf("inet_addr: name '%s' addr %08x\n",
			macHost.cname,macHost.addr[0]);
#endif

	/* was the 'address' really a name? */
	if (macHost.cname[0] != 0)
		return((ip_addr)-1);
	
	return(macHost.addr[0]);
}

/*
 * gethostid()
 *
 * Get Internet address. If the application can get by with just
 * this, it avoids the muss and fuss of DNR.
 */
 
unsigned long gethostid()
{
	ip_addr ipaddr;
	
	ipaddr = xIPAddr();
	
	return ((unsigned long) ipaddr);
}
 
/*
 * gethostname()
 *
 * Try to get my host name from DNR. If it fails, just return my
 * IP address as ASCII. This is non-standard, but it's a mac,
 * what do you want me to do?
 */

gethostname( machname, buflen)
	char *machname;
	long buflen;
{
	ip_addr ipaddr;
	struct	hostent *hp;
	
#if	NETDB_DEBUG >= 3
	dprintf("gethostname: \n");
#endif

	sock_init();		/* initialize the socket stuff. */
	ipaddr = xIPAddr();
	hp = gethostbyaddr( &ipaddr, sizeof(ip_addr), AF_INET);
	if( hp == NULL) 
		sprintf (machname, "%d.%d.%d.%d",ipaddr>>24,
								ipaddr>>16 & 0xff,
								ipaddr>>8 & 0xff,
								ipaddr & 0xff);
	else
		strncpy( machname, hp->h_name, buflen);
	
	machname[buflen-1] = 0;  /* extra safeguard */
	return(0);
}


/*
 *	getservbybname()
 *
 *	Real kludgy.  Should at least consult a resource file as the service
 *	database.
 */
typedef struct services {
	char		sv_name[12];
	short		sv_number;
	char		sv_protocol[5];
} services_t, *services_p;

static	struct services	slist[] = 
{ 
	{"echo", 7, "udp"},
	{"discard", 9, "udp"},
	{"time", 37, "udp"},
	{"domain", 53, "udp"},
	{"sunrpc", 111, "udp"},
	{"tftp", 69, "udp"},
	{"biff", 512, "udp"},
	{"who", 513, "udp"},
	{"talk", 517, "udp"},
	{"route", 520, "udp"},
	{"new-rwho", 550, "udp"},
	{"netstat", 15, "tcp"},
	{"ftp-data", 20, "tcp"},
	{"ftp", 21, "tcp"},
	{"telnet", 23, "tcp"},
	{"smtp", 25, "tcp"},
	{"time", 37, "tcp"},
	{"whois", 43, "tcp"},
	{"domain", 53, "tcp"},
	{"hostnames", 101, "tcp"},
	{"nntp", 119, "tcp"},
	{"finger", 79, "tcp"},
	{"uucp-path", 117, "tcp"},
	{"untp", 119, "tcp"},
	{"ntp", 123, "tcp"},
	{"exec", 512, "tcp"},
	{"login", 513, "tcp"},
	{"shell", 514, "tcp"},
	{"printer", 515, "tcp"},
	{"courier", 530, "tcp"},
	{"uucp", 540, "tcp"},
	{"", 0, "" }
};
					 
#define	MAX_SERVENT			10
static 	struct servent		servents[MAX_SERVENT];
static 	int					servent_count=0;

struct servent *
getservbyname (name, proto)
char		*name, *proto;
{
	int				i;
	struct	servent	*se;
	
	if (strcmp (proto, "udp") == 0 || strcmp (proto, "tcp") == 0) 
	{
		for (i=0; slist[i].sv_number != 0; i++)
			if (strcmp (slist[i].sv_name, name) == 0)
				if (strcmp (slist[i].sv_protocol, proto) == 0) 
				{
					se = &servents[servent_count];
					se->s_name = slist[i].sv_name;
					se->s_aliases = NULL;
					se->s_port = slist[i].sv_number;
					se->s_proto = slist[i].sv_protocol;
					servent_count = (servent_count +1) % MAX_SERVENT;
					return (se);
				}
		return (NULL);
	}
	else 
	{
		errno = errno_long = EPROTONOSUPPORT;
		return(NULL);
	}
}

static	char	tcp[] = "tcp";
static	char	udp[] = "udp";
#define	MAX_PROTOENT			10
static 	struct protoent		protoents[MAX_PROTOENT];
static 	int					protoent_count=0;
struct protoent *
getprotobyname (name)
char		*name;
{
	struct protoent *pe;
	
	pe = &protoents[protoent_count];
	if (strcmp (name, "udp") == 0) 
	{
		pe->p_name = udp;
		pe->p_proto = IPPROTO_UDP;
	}
	else if (strcmp (name, "tcp") == 0) 
	{
		pe->p_name = tcp;
		pe->p_proto = IPPROTO_TCP;
	}
	else 
	{	
		errno = errno_long = EPROTONOSUPPORT;
		return(NULL);
	}
	pe->p_aliases = NULL;
	protoent_count = (protoent_count +1) % MAX_PROTOENT;
	return (pe);
}

char *h_errlist[] = 
{
	"Error 0",
	"Unknown host",						/* 1 HOST_NOT_FOUND */
	"Host name lookup failure",			/* 2 TRY_AGAIN */
	"Unknown server error",				/* 3 NO_RECOVERY */
	"No address associated with name",	/* 4 NO_ADDRESS */
};

const int	h_nerr = { sizeof(h_errlist)/sizeof(h_errlist[0]) };

void herror(char *s)
	{
	fprintf(stderr,"%s: ",s);
	if (h_errno < h_nerr)
		fprintf(stderr,h_errlist[h_errno]);
	else
		fprintf(stderr,"error %d",h_errno);
	fprintf(stderr,"\n");
	}

char *herror_str(int theErr) {
	if (theErr > h_nerr )
		return NULL;
	else
		return h_errlist[theErr];
		}

#pragma segment SOCK_RESIDENT
pascal void DNRDone(hostinfoPtr,done)
	struct hostInfo *hostinfoPtr;
	Boolean *done;
{
#pragma unused(hostinfoPtr)
	*done = true;
}
