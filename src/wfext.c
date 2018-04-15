/********************************************************************

   wfExt.c

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

//----------------------------
//
//  Winfile Extension cache
//
//----------------------------

LPXDTA* lplpxdtaExtSelItems;
UINT    uExtSelItems                = (UINT)-1;
WCHAR   szExtSelDir[MAXPATHLEN];
WCHAR   szExtSelDirShort[MAXPATHLEN];


VOID
ExtSelItemsInvalidate()
{
   if (uExtSelItems != (UINT)-1) {

      if (lplpxdtaExtSelItems) {
         LocalFree(lplpxdtaExtSelItems);
      }

      uExtSelItems = (UINT)-1;
   }
}


LONG
GetExtSelection(
   HWND hwnd,
   UINT uItem,
   LPARAM lParam,
   BOOL bSearch,
   BOOL bGetCount,
   BOOL bLFNAware,
   BOOL bUnicode)
{
#define lpSelW ((LPFMS_GETFILESELW)lParam)
#define lpSelA ((LPFMS_GETFILESELA)lParam)

   LPXDTALINK lpStart;
   LPXDTA lpxdta = NULL;
   UINT uSel, i;
   HWND hwndLB;
   WCHAR szPath[COUNTOF(lpSelW->szName)];
   BOOL bUsedDefaultChar;
   HWND hwndView;
   LPINT  lpExtSelItems;
   LPWSTR pszAlternateFileName;
   LPWSTR pszFileName;

   hwndView = bSearch ?
      hwnd :
      HasDirWindow(hwnd);

   lpStart = (LPXDTALINK)GetWindowLongPtr(hwndView, GWL_HDTA);
   hwndLB = GetDlgItem(hwndView, IDCW_LISTBOX);

   if (uExtSelItems == (UINT)-1) {

      //
      // Cache invalid, refresh
      //
      uExtSelItems = (UINT) SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);

      lpExtSelItems = (LPINT) LocalAlloc(LMEM_FIXED, sizeof(INT) * uExtSelItems);

      if (lpExtSelItems == NULL) {
         uExtSelItems = (UINT)-1;
         return 0L;
      }

      lplpxdtaExtSelItems = (LPXDTA*) LocalAlloc(LMEM_FIXED,
                                                 sizeof(LPXDTA) * uExtSelItems);

      if (lplpxdtaExtSelItems == NULL) {

         LocalFree(lpExtSelItems);
         uExtSelItems = (UINT)-1;
         return 0L;
      }

      uExtSelItems = (UINT)SendMessage(hwndLB,
                                       LB_GETSELITEMS,
                                       (WPARAM)uExtSelItems,
                                       (LPARAM)lpExtSelItems);

      for (i=0, uSel=0; i < uExtSelItems; i++) {

         SendMessage(hwndLB,
                     LB_GETTEXT,
                     lpExtSelItems[i],
                     (LPARAM)&lplpxdtaExtSelItems[i]);
      }

      //
      // We can't cache this because there's files may be in many
      // different directories.
      //
      if (!bSearch) {
         SendMessage(hwnd,
                     FS_GETDIRECTORY,
                     COUNTOF(szExtSelDir),
                     (LPARAM)szExtSelDir);

	 GetShortPathName(szExtSelDir, szExtSelDirShort, COUNTOF(szExtSelDirShort));
      }

      LocalFree(lpExtSelItems);
   }


   for (i=0, uSel=0; i < uExtSelItems; i++) {

      lpxdta = lplpxdtaExtSelItems[i];

      if (!lpxdta || lpxdta->dwAttrs & ATTR_PARENT)
         continue;

      if ((!bLFNAware && (lpxdta->dwAttrs & ATTR_LFN)) &&
         !MemGetAlternateFileName(lpxdta)[0]) {

         continue;
      }

      if (!bGetCount && uItem == uSel) {
         break;
      }

      uSel++;
   }

   if (!lpxdta)
   {
       //
       //  Quit out if lpxdta is null.
       //
       return (0L);
   }

   //
   // We use uSel regardless; if we wanted item #4 but we ran out
   // at #2, give'm #2.  This adheres to the previous semantics.
   //
   if (bGetCount) {
      return (LONG)uSel;
   }

   pszAlternateFileName = MemGetAlternateFileName(lpxdta);

   if (bUnicode) {

      //
      // lpxdta setup up above
      //
      lpSelW->bAttr = (BYTE)lpxdta->dwAttrs;
      lpSelW->ftTime = lpxdta->ftLastWriteTime;
      lpSelW->dwSize = lpxdta->qFileSize.LowPart;

      pszFileName = lpSelW->szName;

   } else {

      lpSelA->bAttr = (BYTE)lpxdta->dwAttrs;
      lpSelA->ftTime = lpxdta->ftLastWriteTime;
      lpSelA->dwSize = lpxdta->qFileSize.LowPart;

      pszFileName = szPath;
   }

   if (bSearch) {

      lstrcpy(pszFileName, MemGetFileName(lpxdta));

      //
      // The search window can have any random directory for
      // each item, so don't use szExtSelDirShort caching
      //
      if (!bLFNAware) {

	 GetShortPathName(MemGetFileName(lpxdta), pszFileName, COUNTOF(lpSelW->szName));
      }

   } else {

      //
      // If we are LFNAware, then we need to get the dir.
      //
      if (bLFNAware) {

         lstrcpy(pszFileName, szExtSelDir);
         lstrcat(pszFileName, MemGetFileName(lpxdta));

      } else {

         //
         // Need short dir; use cache.
         //
         lstrcpy(pszFileName, szExtSelDirShort);
         lstrcat(pszFileName,
                 pszAlternateFileName[0] ?
                    pszAlternateFileName :
                    MemGetFileName(lpxdta));
      }

   }

   if (!bUnicode) {

      //
      // Not unicode, must do the thunking now
      // from our temp buffer to lpSelA
      //
      bUsedDefaultChar = FALSE;

      if (!WideCharToMultiByte(CP_ACP, 0, szPath, -1, lpSelA->szName,
         COUNTOF(lpSelA->szName), NULL, &bUsedDefaultChar)) {

            lpSelA->szName[0] = CHAR_NULL;
      }

      if (bUsedDefaultChar) {

         if (!WideCharToMultiByte(CP_ACP, 0, szPath, -1, lpSelA->szName,
            COUNTOF(lpSelA->szName), NULL, &bUsedDefaultChar)) {

            lpSelA->szName[0] = CHAR_NULL;
         }
      }
   }

   return (LONG)uSel;
}
#undef lpSelW
#undef lpSelA


LONG
GetDriveInfo(HWND hwnd, UINT uMsg, LPARAM lParam)
{
#define lpSelW ((LPFMS_GETDRIVEINFOW)lParam)
#define lpSelA ((LPFMS_GETDRIVEINFOA)lParam)

   WCHAR szPath[MAXPATHLEN];
   LPWSTR lpszVol;

   // this has to work for hwnd a tree or search window

   SendMessage(hwnd, FS_GETDIRECTORY, COUNTOF(szPath), (LPARAM)szPath);
   StripBackslash(szPath);

   if (FM_GETDRIVEINFOW == uMsg)
   {
      lpSelW->dwTotalSpace = qTotalSpace.LowPart;
      lpSelW->dwFreeSpace = qFreeSpace.LowPart;

      lstrcpy(lpSelW->szPath, szPath);

      if (ISUNCPATH(szPath))
      {
         lpSelW->szVolume[0] = CHAR_NULL;
      }
      else
      {
         GetVolumeLabel(DRIVEID(szPath), &lpszVol, FALSE);
         StrNCpy(lpSelW->szVolume, lpszVol, COUNTOF(lpSelW->szVolume)-1);
      }
   }
   else
   {
      lpSelA->dwTotalSpace = qTotalSpace.LowPart;
      lpSelA->dwFreeSpace = qFreeSpace.LowPart;

      if (!WideCharToMultiByte( CP_ACP,
                                0,
                                szPath,
                                -1,
                                lpSelA->szPath,
                                COUNTOF(lpSelA->szPath),
                                NULL,
                                NULL ))
      {
         lpSelA->szPath[0] = CHAR_NULL;
      }

      if (ISUNCPATH(szPath))
      {
         lpSelW->szVolume[0] = CHAR_NULL;
      }
      else
      {
         GetVolumeLabel(DRIVEID(szPath), &lpszVol, FALSE);

         if (!WideCharToMultiByte( CP_ACP,
                                   0,
                                   lpszVol,
                                   -1,
                                   lpSelA->szVolume,
                                   COUNTOF(lpSelA->szVolume),
                                   NULL,
                                   NULL ))
         {
            lpSelA->szVolume[0] = CHAR_NULL;
         }
         lpSelA->szVolume[COUNTOF(lpSelA->szVolume)-1] = CHAR_NULL;
      }
   }

   //
   // Force update
   //

   WAITNET();

   if (ISUNCPATH(szPath))
   {
       lpSelW->szShare[0] = CHAR_NULL;
       return (1L);
   }

   U_NetCon(DRIVEID(szPath));

   if (WFGetConnection(DRIVEID(szPath), &lpszVol, FALSE, ALTNAME_REG)) {
      lpSelW->szShare[0] = CHAR_NULL;
   } else {

      if (FM_GETDRIVEINFOW == uMsg) {

         StrNCpy(lpSelW->szShare, lpszVol, COUNTOF(lpSelW->szShare)-1);
         lpSelW->szShare[COUNTOF(lpSelW->szShare)-1] = CHAR_NULL;

      } else {

         if (!WideCharToMultiByte(CP_ACP, 0, lpszVol, -1,
            lpSelA->szShare, COUNTOF(lpSelA->szShare), NULL, NULL)) {

            lpSelA->szShare[0] = CHAR_NULL;
         }
         lpSelA->szShare[COUNTOF(lpSelA->szShare)-1] = CHAR_NULL;
      }
   }

   return (1L);
}
#undef lpSelW
#undef lpSelA


VOID
FreeExtensions()
{
   INT i;
   HMENU hMenuFrame;
   INT iMax;
   HWND hwndActive;

   FreeToolbarExtensions();


   hMenuFrame = GetMenu(hwndFrame);
   hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
   if (hwndActive && GetWindowLongPtr(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
      iMax = 1;
   else
      iMax = 0;

   for (i = 0; i < iNumExtensions; i++) {
      (extensions[i].ExtProc)(NULL, FMEVENT_UNLOAD, 0L);
      DeleteMenu(hMenuFrame, IDM_EXTENSIONS + iMax, MF_BYPOSITION);
      FreeLibrary((HANDLE)extensions[i].hModule);
   }
   iNumExtensions = 0;
}


LRESULT
ExtensionMsgProc(UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   HWND hwndActive;
   HWND hwndTree, hwndDir, hwndFocus;

   hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
   GetTreeWindows(hwndActive, &hwndTree, &hwndDir);

   switch (wMsg) {

   case FM_RELOAD_EXTENSIONS:
      SendMessage(hwndFrame, WM_CANCELMODE, 0, 0L);

      SaveRestoreToolbar(TRUE);
      SendMessage(hwndToolbar, WM_SETREDRAW, FALSE, 0L);

      FreeExtensions();

      InitExtensions();
      SaveRestoreToolbar(FALSE);

      SendMessage(hwndToolbar, WM_SETREDRAW, TRUE, 0L);
      InvalidateRect(hwndToolbar, NULL, TRUE);

      DrawMenuBar(hwndFrame);
      break;

   case FM_GETFOCUS:
      // wParam       unused
      // lParam       unused
      // return       window tyep with focus

      if (hwndActive == hwndSearch)
         return FMFOCUS_SEARCH;

      hwndFocus = GetTreeFocus(hwndActive);

      if (hwndFocus == hwndTree)
         return FMFOCUS_TREE;
      else if (hwndFocus == hwndDir)
         return FMFOCUS_DIR;
      else if (hwndFocus == hwndDriveBar)
         return FMFOCUS_DRIVES;
      break;

   case FM_GETDRIVEINFOA:
   case FM_GETDRIVEINFOW:

      // wParam       unused
      // lParam       LPFMS_GETDRIVEINFO structure to be filled in

      return GetDriveInfo(hwndActive, wMsg, lParam);
      break;

   case FM_REFRESH_WINDOWS:
      // wParam       0 refresh the current window
      //              non zero refresh all windows
      // lParam       unused


      //
      // Note: For all windows that it refreshes,
      // it requests a WNetGetDirectoryType flush cache.

      UpdateDriveList();

      if (wParam == 0) {
         RefreshWindow(hwndActive, FALSE, TRUE);
      } else {
         HWND hwndT, hwndNext;

         hwndT = GetWindow(hwndMDIClient, GW_CHILD);
         while (hwndT) {
            hwndNext = GetWindow(hwndT, GW_HWNDNEXT);

            if (!GetWindow(hwndT, GW_OWNER)) {

               RefreshWindow(hwndT, FALSE, TRUE);
            }
            hwndT = hwndNext;
         }
      }

      SPC_SET_HITDISK(qFreeSpace);
      UpdateStatus(hwndActive);
      break;

   case FM_GETSELCOUNT:
   case FM_GETSELCOUNTLFN:
      // wParam       unused
      // lParam       unused
      // return       # of selected items

   case FM_GETFILESELA:
   case FM_GETFILESELW:
   case FM_GETFILESELLFNA:
   case FM_GETFILESELLFNW:

      // wParam       index of selected item to get
      // lParam       LPFMS_GETFILESEL structure to be filled in

      if (hwndActive != hwndSearch && !hwndDir)
         return 0L;

      // note, this uses the fact that LFN messages are odd!

      return GetExtSelection(hwndActive, (UINT)wParam, lParam,
         hwndActive == hwndSearch, (wMsg & ~1) == FM_GETSELCOUNT,
         (BOOL)(wMsg & 1),
         (wMsg == FM_GETFILESELW || wMsg == FM_GETFILESELLFNW));
   }

   return 0;
}
