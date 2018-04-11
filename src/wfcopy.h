/********************************************************************

   wfcopy.h

   Include for WINFILE's File Copying Routines

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#define FIND_DIRS       0x0010

#define CNF_DIR_EXISTS      0x0001
#define CNF_ISDIRECTORY     0x0002

#define BUILD_TOPLEVEL      0
#define BUILD_RECURSING     1
#define BUILD_NORECURSE     2

#define OPER_MASK       0x0F00
#define OPER_MKDIR      0x0100
#define OPER_RMDIR      0x0200
#define OPER_DOFILE     0x0300
#define OPER_ERROR      0x0400

#define CCHPATHMAX      260
#define MAXDIRDEPTH     CCHPATHMAX/2

#define ATTR_ATTRIBS      0x200 /* Flag indicating we have file attributes */
#define ATTR_COPIED       0x400 /* we have copied this file */
#define ATTR_DELSRC       0x800 /* delete the source when done */

typedef struct _copyroot {
#ifdef FASTMOVE
   BOOL    fRecurse  : 1;
#endif
   BOOL    bFastMove : 1;
   WORD    cDepth;
   LPTSTR  pSource;
   LPTSTR  pRoot;
   TCHAR   cIsDiskThereCheck[26];
   TCHAR   sz[MAXPATHLEN];
   TCHAR   szDest[MAXPATHLEN];
   LFNDTA  rgDTA[MAXDIRDEPTH];
} COPYROOT, *PCOPYROOT;


DWORD FileMove(LPTSTR, LPTSTR, PBOOL, BOOL);
DWORD FileRemove(LPTSTR);
DWORD MKDir(LPTSTR, LPTSTR);
DWORD RMDir(LPTSTR);
BOOL WFSetAttr(LPTSTR lpFile, DWORD dwAttr);

VOID AppendToPath(LPTSTR,LPCTSTR);
UINT RemoveLast(LPTSTR pFile);
VOID Notify(HWND,WORD,LPTSTR,LPTSTR);

LPTSTR FindFileName(register LPTSTR pPath);

