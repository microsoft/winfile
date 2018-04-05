/********************************************************************

   mpr.h

   Standard MPR Header File for NT-WIN32

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include <lmcons.h>
#ifndef _MPR_H_INCLUDED
#define _MPR_H_INCLUDED

//For restoring connection stuff. Add by congpay.
// const used by connect.c
#define SHOW_CONNECTION     (WM_USER+200)
#define DO_PASSWORD_DIALOG  (WM_USER + 201)
#define DO_ERROR_DIALOG     (WM_USER + 202)

// types used by connect.c

typedef struct _CONNECTION_INFO *LPCONNECTION_INFO;

// The following two structures are used by two threads in mpr.dll and
// mprui.dll to share data.

typedef struct _PARAMETERS {
    HWND    hDlg;
    HANDLE  hDlgCreated;                //Initialized in WNetRestoreConnection
    HANDLE  hDlgFailed;                 //Initialized in WNetRestoreConnection
    HANDLE  hDonePassword;              //Initialized in WNetRestoreConnection
    TCHAR * pchResource;                //ShowReconnectDialog, DoRestoreConnection
    TCHAR * pchUserName;                //For DoPasswordDialog
    TCHAR   passwordBuffer[UNLEN+1];    //Used by WNetRestoreThisConnection
    BOOL    fSuccess;                   //For the DoPasswordDialog
    BOOL    fDidCancel;                 //For the DoPasswordDialog
    BOOL    fDownLevel;                 //FALSE if error==ERROR_LOGON_FAILURE
    BOOL    fTerminateThread;           //TRUE if we want the second thread to be end
    DWORD   status;                     //return value from DoRestoreConnection
    DWORD   numSubKeys;                 //Initialized in WNetRestoreConnection
    DWORD   RegMaxWait;
    LPCONNECTION_INFO ConnectArray;     //Initialized in WNetRestoreConnection
    }PARAMETERS;

#ifdef __cplusplus
extern "C" {
#endif

//function load from mprui.dll.

DWORD DoPasswordDialog(
    HWND          hwndOwner,
    TCHAR *       pchResource,
    TCHAR *       pchUserName,
    TCHAR *       pchPasswordReturnBuffer,
    DWORD         cbPasswordReturnBuffer, // bytes!
    BOOL *        pfDidCancel,
    BOOL          fDownLevel
    );

DWORD DoProfileErrorDialog(
    HWND          hwndOwner,
    const TCHAR * pchDevice,
    const TCHAR * pchResource,
    const TCHAR * pchProvider,
    DWORD         dwError,
    BOOL          fAllowCancel, // ask whether to stop reconnecting devices
                                //  this time?
    BOOL *        pfDidCancel,  // stop reconnecting devices this time?
                                //  active iff fAllowCancel
    BOOL *        pfDisconnect, // do not reconnect this device in future?
    BOOL *        pfHideErrors  // stop displaying error dialogs this time?
                                //  active iff fAllowCancel
    );

DWORD ShowReconnectDialog (
    HWND          hwndParent,
    PARAMETERS *  Params);

//
// Return codes from WNetRestoreConnection
//
#define WN_CONTINUE     0x00000BB9

DWORD APIENTRY
RestoreConnectionA0(
     HWND    hWnd,
     LPSTR  lpDevice
    );

DWORD APIENTRY
WNetClearConnections(
     HWND    hWnd
     ) ;

//
// Authentication Provider (Credential Management) Functions
//

DWORD APIENTRY
WNetLogonNotify(
    LPCWSTR             lpPrimaryAuthenticator,
    PLUID               lpLogonId,
    LPCWSTR             lpAuthentInfoType,
    LPVOID              lpAuthentInfo,
    LPCWSTR             lpPreviousAuthentInfoType,
    LPVOID              lpPreviousAuthentInfo,
    LPWSTR              lpStationName,
    LPVOID              StationHandle,
    LPWSTR              *lpLogonScripts
    );

DWORD APIENTRY
WNetPasswordChangeNotify(
    LPCWSTR             lpPrimaryAuthenticator,
    LPCWSTR             lpAuthentInfoType,
    LPVOID              lpAuthentInfo,
    LPCWSTR             lpPreviousAuthentInfoType,
    LPVOID              lpPreviousAuthentInfo,
    LPWSTR              lpStationName,
    LPVOID              StationHandle,
    DWORD               dwChangeInfo
    );

//
// Directory functions
//
DWORD
WNetGetDirectoryTypeA (
    LPSTR   lpName,
    LPDWORD lpType,
    BOOL    bFlushCache
    );

DWORD
WNetGetDirectoryTypeW (
    LPTSTR  lpName,
    LPINT   lpType,
    BOOL    bFlushCache
    );

#ifdef UNICODE
#define WNetGetDirectoryType   WNetGetDirectoryTypeW
#else
#define WNetGetDirectoryType   WNetGetDirectoryTypeA
#endif  // UNICODE


DWORD
WNetDirectoryNotifyA (
    HWND    hwnd,
    LPSTR   lpDir,
    DWORD   dwOper
    );

DWORD
WNetDirectoryNotifyW (
    HWND    hwnd,
    LPTSTR  lpDir,
    DWORD   dwOper
    );

#ifdef UNICODE
#define WNetDirectoryNotify   WNetDirectoryNotifyW
#else
#define WNetDirectoryNotify   WNetDirectoryNotifyA
#endif  // UNICODE


DWORD
WNetPropertyDialogA (
    HWND  hwndParent,
    DWORD iButton,
    DWORD nPropSel,
    LPSTR lpszName,
    DWORD nType
    );

DWORD
WNetPropertyDialogW (
    HWND  hwndParent,
    DWORD iButton,
    DWORD nPropSel,
    LPTSTR lpszName,
    DWORD nType
    );

#ifdef UNICODE
#define WNetPropertyDialog    WNetPropertyDialogW
#else
#define WNetPropertyDialog    WNetPropertyDialogA
#endif  // UNICODE


DWORD
WNetGetPropertyTextA (
    DWORD iButton,
    DWORD nPropSel,
    LPSTR lpszName,
    LPSTR lpszButtonName,
    DWORD nButtonNameLength,
    DWORD nType
    );

DWORD
WNetGetPropertyTextW (
    DWORD  iButton,
    DWORD  nPropSel,
    LPTSTR lpszName,
    LPTSTR lpszButtonName,
    DWORD  nButtonNameLength,
    DWORD  nType
    );

#ifdef UNICODE
#define WNetGetPropertyText   WNetGetPropertyTextW
#else
#define WNetGetPropertyText   WNetGetPropertyTextA
#endif  // UNICODE

typedef struct _WNET_CONNECTINFOA
{
    LPSTR lpRemoteName ;
    LPSTR lpProvider ;
} WNET_CONNECTIONINFOA, *LPWNET_CONNECTIONINFOA ;

typedef struct _WNET_CONNECTINFOW
{
    LPWSTR lpRemoteName ;
    LPWSTR lpProvider ;
} WNET_CONNECTIONINFOW, *LPWNET_CONNECTIONINFOW ;

#ifdef UNICODE
#define WNET_CONNECTIONINFO   WNET_CONNECTIONINFOW
#define LPWNET_CONNECTIONINFO LPWNET_CONNECTIONINFOW
#else
#define WNET_CONNECTIONINFO WNET_CONNECTIONINFOA
#define LPWNET_CONNECTIONINFO LPWNET_CONNECTIONINFOA
#endif  // UNICODE

//
//  Used in conjunction with WNET_CONNECTIONINFO, gets the net provider name
//  in addition to the remote name for this connection.
//
DWORD
WNetGetConnection2A(
    LPSTR   lpLocalName,
    LPVOID  lpBuffer,       // Contains WNET_CONNECTIONINFOA struct
    LPDWORD lpBufferSize    // In bytes!
    ) ;

DWORD
WNetGetConnection2W(
    LPWSTR  lpLocalName,
    LPVOID  lpBuffer,       // Contains WNET_CONNECTIONINFOW struct
    LPDWORD lpBufferSize    // In bytes!
    ) ;

#ifdef UNICODE
#define WNetGetConnection2 WNetGetConnection2W
#else
#define WNetGetConnection2 WNetGetConnection2A
#endif  // UNICODE

//
// Versions of the dialog with the ability to supply help.
// These are not in Win32 because we do not want to force
// nor encourage apps to have to document the way these
// dialogs work, since they are liable to change.
//
DWORD WNetConnectionDialog2 (
    HWND    hwndParent,
    DWORD   dwType,
    WCHAR  *lpHelpFile,
    DWORD   nHelpContext
    );

DWORD WNetDisconnectDialog2 (
    HWND    hwndParent,
    DWORD   dwType,
    WCHAR  *lpHelpFile,
    DWORD   nHelpContext
    );


//
// Browse dialog
//

// Type of the callback routine used by the browse dialog to validate
// the path input by the user
typedef BOOL (*PFUNC_VALIDATION_CALLBACK)( LPWSTR pszName );

//  WNetBrowseDialog and WNetBrowsePrinterDialog
//  NOTE: WNetBrowsePrintDialog =
//        WNetBrowseDialog with dwType RESOURCETYPE_PRINT
//
/*******************************************************************

    NAME:       WNetBrowseDialog, WNetBrowsePrinterDialog

    SYNOPSIS:   Presents a dialog to the user from which the user can
                browse the network for disk or print shares.

    ENTRY:      hwndParent  -  Parent window handle
                dwType      -  ( Only in WNetBrowseDialog )
                   RESOURCETYPE_DISK or RESOURCETYPE_PRINT
                lpszName    -  The path name typed by the user. It will be
                               undefined if the user hits the CANCEL button.
                cchBufSize  -  The buffer size of the lpszName in characters
                lpszHelpFile-  The helpfile to use when the user hits F1.
                nHelpContext-  The helpcontext to use for the helpfile above
                pfuncValidation - Callback method to validate the path typed
                   by the user. If NULL, no validation will
                               be done.

    RETURNS:    WN_CANCEL when the user cancels the dialog. NO_ERROR
                on success, standard ERROR_* error code otherwise

    NOTES:      This is a UNICODE only API.

    HISTORY:
        Yi-HsinS    22-Nov-1992    Created

********************************************************************/

DWORD WNetBrowseDialog(
    HWND    hwndParent,
    DWORD   dwType,
    WCHAR  *lpszName,
    DWORD   cchBufSize,
    WCHAR  *lpszHelpFile,
    DWORD   nHelpContext,
    PFUNC_VALIDATION_CALLBACK pfuncValidation );

DWORD WNetBrowsePrinterDialog(
    HWND    hwndParent,
    WCHAR  *lpszName,
    DWORD   cchBufSize,
    WCHAR  *lpszHelpFile,
    DWORD   nHelpContext,
    PFUNC_VALIDATION_CALLBACK pfuncValidation );

//
// stuff in user, not driver, for shell apps ;Internal
//
DWORD APIENTRY WNetErrorText(DWORD,LPTSTR,DWORD); // ;Internal

//
// used by MPRUI.DLL to determine if a provider supports
// NpSearchDialog() and obtain to a pointer to it.
//
FARPROC WNetGetSearchDialog(LPWSTR lpProvider) ;

//
// used by MPRUI.DLL to determine if a provider supports
// NPFormatNetworkName() and obtain a pointer to it.
//
FARPROC WNetGetFormatNameProc(LPWSTR lpProvider) ;

//
// used by MPRUI.DLL to determine if a provider supports
// WNNC_ENUM_GLOBAL
//
BOOL WNetSupportGlobalEnum(LPWSTR lpProvider) ;

//
// used by ACLEDIT.DLL to get provider-specific permission editor
//

DWORD WNetFMXGetPermCaps( LPWSTR lpDriveName ) ;
DWORD WNetFMXEditPerm( LPWSTR lpDriveName, HWND hwndFMX, DWORD nDialogType );
DWORD WNetFMXGetPermHelp( LPWSTR  lpDriveName,
                          DWORD   nDialogType,
                          BOOL    fDirectory,
                          LPVOID  lpFileNameBuffer,
                          LPDWORD lpBufferSize,
                          LPDWORD lpnHelpContext );

//
// sections and keys used for persistent connections
//

#define WNNC_DLG_DISCONNECT     0x0008
#define WNNC_DLG_CONNECT        0x0004

#ifdef UNICODE

#define MPR_MRU_FILE_SECTION        L"NET_Files"
#define MPR_MRU_PRINT_SECTION       L"NET_Printers"
#define MPR_MRU_ORDER_KEY       L"Order"

#define MPR_NETWORK_SECTION     L"Network"
#define MPR_SAVECONNECTION_KEY      L"SaveConnections"
#define MPR_RESTORECONNECTION_KEY   L"RestoreConnections"
#define MPR_EXPANDLOGONDOMAIN_KEY       L"ExpandLogonDomain"

#define MPR_YES_VALUE           L"yes"
#define MPR_NO_VALUE            L"no"

#else

#define MPR_MRU_FILE_SECTION        "NET_Files"
#define MPR_MRU_PRINT_SECTION       "NET_Printers"
#define MPR_MRU_ORDER_KEY       "Order"

#define MPR_NETWORK_SECTION     "Network"
#define MPR_SAVECONNECTION_KEY      "SaveConnections"
#define MPR_RESTORECONNECTION_KEY   "RestoreConnections"
#define MPR_EXPANDLOGONDOMAIN_KEY       "ExpandLogonDomain"

#define MPR_YES_VALUE           "yes"
#define MPR_NO_VALUE            "no"

#endif  // UNICODE


//
// Internal NP interface used to help the NTLM provider remember
// whether a persistent connection is a DFS connection or not
//

DWORD APIENTRY
NPGetReconnectFlags (
       IN  LPWSTR   lpLocalName,
       OUT LPBYTE   lpPersistFlags
    );
typedef DWORD (*PF_NPGetReconnectFlags) (
       LPWSTR   lpLocalName,
       LPBYTE   lpPersistFlags
    );

// This macro operates on the dwFlags parameter of NPAddConnection3
#define CONNECT_PROVIDER_FLAGS(dwFlags)   ((BYTE) (((dwFlags) & 0xFF000000) >> 24))

#ifdef __cplusplus
}
#endif

#endif // _MPR_H_INCLUDED
