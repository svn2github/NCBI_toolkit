/* $Copyright:
 *
 * Copyright � 1998-1999 by the Massachusetts Institute of Technology.
 * 
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Furthermore if you modify
 * this software you must label your software as modified software and not
 * distribute it in such a fashion that it might be confused with the
 * original MIT software. M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * Individual source code files are copyright MIT, Cygnus Support,
 * OpenVision, Oracle, Sun Soft, FundsXpress, and others.
 * 
 * Project Athena, Athena, Athena MUSE, Discuss, Hesiod, Kerberos, Moira,
 * and Zephyr are trademarks of the Massachusetts Institute of Technology
 * (MIT).  No commercial use of these trademarks may be made without prior
 * written permission of MIT.
 * 
 * "Commercial use" means use of a name in a product or other for-profit
 * manner.  It does NOT prevent a commercial firm from referring to the MIT
 * trademarks in order to convey information (although in doing so,
 * recognition of their trademark status should be given).
 * $
 */

/* 
 *
 * Idle.h -- Main external header file for the Idle library.
 *
 */

#ifndef _IDLELIB_
#define _IDLELIB_

#ifdef __cplusplus
extern "C" {
#endif

#include <Events.h>

/****************************/
/* API Structures and Types */
/****************************/

/* Callback API for Event handler proc for idle loop */
typedef CALLBACK_API (Boolean, IdleEventHandlerProcPtr) (EventRecord *theEvent, UInt32 refCon);

/* UPP for Idle Library event filter */
#if TARGET_API_MAC_CARBON
typedef IdleEventHandlerProcPtr                  IdleEventHandlerUPP;
#else
typedef STACK_UPP_TYPE (IdleEventHandlerProcPtr) IdleEventHandlerUPP;
#endif

/* Procinfo for Idle Library event filter */
enum {
	uppIdleEventHandlerProcInfo = kPascalStackBased |
		RESULT_SIZE (sizeof (Boolean)) |
		STACK_ROUTINE_PARAMETER (1, SIZE_CODE (sizeof (EventRecord *))) |
		STACK_ROUTINE_PARAMETER (2, SIZE_CODE (sizeof (UInt32)))
};

#define	NewIdleEventHandlerProc(userRoutine) 			\
	(IdleEventHandlerUPP) NewRoutineDescriptor((ProcPtr)(userRoutine), uppIdleEventHandlerProcInfo, GetCurrentArchitecture())


/***********************/
/* Function Prototypes */
/***********************/

/* IdleLib API calls */
OSStatus IdleAddEventHandler(IdleEventHandlerUPP eventHandlerUPP, UInt16 mask, UInt32 refCon);
OSStatus IdleRemoveEventHandler(IdleEventHandlerUPP eventHandlerUPP);

void IdleSetActive(IdleEventHandlerUPP eventHandlerUPP);
void IdleSetInactive(IdleEventHandlerUPP eventHandlerUPP);

void   IdleSetIdleFrequency(UInt32 idleFrequency);
UInt32 IdleGetIdleFrequency(void);

void   IdleSetEventSleepTime(UInt32 eventSleepTime);
UInt32 IdleGetEventSleepTime(void);

void    IdleSetThreaded(Boolean isThreaded);
Boolean IdleGetThreaded(void);

void    IdleSetShouldIdle(Boolean shouldIdle);
Boolean IdleGetShouldIdle(void);

OSStatus Idle(void);

#ifdef __cplusplus
}
#endif

#endif _IDLELIB_