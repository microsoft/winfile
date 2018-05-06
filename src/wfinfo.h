/********************************************************************

   wfinfo.h

   DriveInformation handler header:
   Handles caching and refreshing of drive information.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#ifndef _WFINFO_H
#define _WFINFO_H

#ifdef _GLOBALS
#define Extern
#define EQ(x) = x
#else
#define Extern extern
#define EQ(x)
#endif

//
// Set up altname stuff
//

#define MAX_ALTNAME 2

#define ALTNAME_MULTI 0
#define ALTNAME_SHORT 1
#define ALTNAME_REG   MAX_ALTNAME  // Request regular name


//
// Get RetVal
//

#define GETRETVAL(type, drive) \
   (aDriveInfo[drive].s##type.dwRetVal)


typedef struct _INFO_STATUS {
   BOOL bValid : 1;
   BOOL bRefresh : 1;
   DWORD dwRetVal;
} INFOSTATUS, *PINFOSTATUS;

#define STATUSNAME(name) INFOSTATUS s##name


#define R_REFRESH(type, drive) \
   aDriveInfo[drive].s##type.bRefresh = TRUE

#define I_INVALIDATE(type, drive) \
   aDriveInfo[drive].s##type.bValid = FALSE

#define C_CLOSE(type, drive, retval) \
   { \
   aDriveInfo[drive].s##type.bValid = TRUE; \
   aDriveInfo[drive].s##type.bRefresh = FALSE; \
   aDriveInfo[drive].s##type.dwRetVal = retval;\
   }

#define R_Type(drive)     R_REFRESH(Type, drive)
#define R_Space(drive)    R_REFRESH(Space, drive)
#define R_NetCon(drive)   R_REFRESH(NetCon, drive)
#define R_VolInfo(drive)  R_REFRESH(VolInfo, drive)


#define I_Type(drive)     I_INVALIDATE(Type, drive)
#define I_Space(drive)    I_INVALIDATE(Space, drive)
#define I_NetCon(drive)   I_INVALIDATE(NetCon, drive)
#define I_VolInfo(drive)  I_INVALIDATE(VolInfo, drive)


#define C_Type(drive,retval)     C_CLOSE(Type, drive, retval)
#define C_Space(drive,retval)    C_CLOSE(Space, drive, retval)
#define C_NetCon(drive,retval)   C_CLOSE(NetCon, drive, retval)
#define C_VolInfo(drive,retval)  C_CLOSE(VolInfo, drive, retval)



#define U_PROTO(type) VOID U_##type(DRIVE drive)

U_PROTO(Type);
U_PROTO(Space);
U_PROTO(NetCon);
U_PROTO(VolInfo);


//
// Define variables
//
#define V_Variable(type) Extern CRITICAL_SECTION CriticalSectionInfo##type

V_Variable(Type);
V_Variable(Space);
V_Variable(NetCon);
V_Variable(VolInfo);



//
// Define constructors
//

#define M_PROTO(type) VOID M_##type(DRIVE drive)
#define M_Make(type)  InitializeCriticalSection(&CriticalSectionInfo##type)

// Default Constructors or prototypes

VOID M_Info(VOID);

#define M_Type()     M_Make(Type)
#define M_Space()    M_Make(Space)
#define M_NetCon()   M_Make(NetCon)
#define M_VolInfo()  M_Make(VolInfo)


//
// Define destructors
//

#define D_PROTO(type) VOID D_##type(VOID)
#define D_Destroy(type) DeleteCriticalSection(&CriticalSectionInfo##type)

// Default Destructors or prototypes

VOID D_Info(VOID);

#define D_Type()     D_Destroy(Type)
#define D_Space()    D_Destroy(Space)
D_PROTO(NetCon);
#define D_VolInfo()  D_Destroy(VolInfo)


//
// Misc prototypes
//

INT NetCon_UpdateAltName(DRIVE drive, DWORD dwRetVal);


//
// Background update support
//

BOOL NetLoad(VOID);

DWORD WINAPI UpdateInit(PVOID ThreadParameter);
DWORD  WFGetConnection(DRIVE,LPTSTR*,BOOL,DWORD);
DWORD GetVolShare(DRIVE drive, LPTSTR* ppszVolShare, DWORD dwType);
VOID UpdateDriveListComplete(VOID);
VOID UpdateDriveList(VOID);
VOID ResetDriveInfo(VOID);

BOOL LoadComdlg(VOID);
VOID UpdateWaitQuit(VOID);
VOID WaitLoadEvent(BOOL bNet);


Extern BOOL   bUpdateRun;
Extern HANDLE hThreadUpdate;
Extern HANDLE hEventUpdate;
Extern HANDLE hEventUpdatePartial;


#endif // ndef _WFINFO_H
