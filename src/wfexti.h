/********************************************************************

   wfexti.h

   Windows File Manager Extensions definitions

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#ifndef _INC_WFEXTI
#define _INC_WFEXTI

//------------------ private stuff ---------------------------  /* ;Internal */
                                                                /* ;Internal */
typedef struct _EXTENSION {                                     /* ;Internal */
        DWORD (APIENTRY *ExtProc)(HWND, WPARAM, LPARAM);        /* ;Internal */
        WORD     Delta;                                         /* ;Internal */
        HANDLE   hModule;                                       /* ;Internal */
        HMENU    hMenu;                                         /* ;Internal */
        DWORD    dwFlags;                                       /* ;Internal */
        HBITMAP  hbmButtons;                                    /* ;Internal */
        WORD     idBitmap;                                      /* ;Internal */
        BOOL     bUnicode;                                      /* ;Internal */
} EXTENSION;                                                    /* ;Internal */

// !! WARNING !!
// MAX_EXTENSIONS is assumed 5 in winfile
// Must be changed there LATER

#define MAX_EXTENSIONS 10                                       /* ;Internal */
extern EXTENSION extensions[MAX_EXTENSIONS];                    /* ;Internal */
                                                                /* ;Internal */
LRESULT ExtensionMsgProc(UINT wMsg, WPARAM wParam, LPARAM lpSel);    /* ;Internal */
VOID FreeExtensions(VOID);                                      /* ;Internal */

#endif /* _INC_WFEXTI */
