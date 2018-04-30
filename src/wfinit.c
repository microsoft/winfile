/********************************************************************

   wfinit.c

   Windows File System Initialization Routines

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include <stdlib.h>

#include "winfile.h"
#include "lfn.h"
#include "wnetcaps.h"         // WNetGetCaps()

#include <ole2.h>
#include <shlobj.h>

typedef VOID (APIENTRY *FNPENAPP)(WORD, BOOL);

VOID (APIENTRY *lpfnRegisterPenApp)(WORD, BOOL);

BYTE chPenReg[] = "RegisterPenApp";    // used in GetProcAddress call

TCHAR szNTlanman[] = TEXT("ntlanman.dll");
TCHAR szHelv[] = TEXT("MS Shell Dlg");
/*
** 6/13/95 FloydR Note re: MS Gothic, MS Shell Dlg and System fonts for Japan.
** For 3.51J, the "MS Shell Dlg" font is linked to the "MS Gothic" Japanese
** TTF.  This means that all this stuff about using different fonts for
** Japan (and Korea/China) is gone.
*/

HBITMAP hbmSave;

DWORD   RGBToBGR(DWORD rgb);
VOID    BoilThatDustSpec(register WCHAR *pStart, BOOL bLoadIt);
VOID    DoRunEquals(PINT pnCmdShow);
VOID    GetSavedWindow(LPWSTR szBuf, PWINDOW pwin);
VOID    GetSettings(VOID);

#define MENU_STRING_SIZ 80
#define PROFILE_STRING_SIZ 300


INT
GetHeightFromPointsString(LPTSTR szPoints)
{
   HDC hdc;
   INT height;

   hdc = GetDC(NULL);
   height = MulDiv(-atoi(szPoints), GetDeviceCaps(hdc, LOGPIXELSY), 72);
   ReleaseDC(NULL, hdc);

   return height;
}


VOID
BiasMenu(HMENU hMenu, UINT Bias)
{
   UINT pos, id, count;
   HMENU hSubMenu;
   TCHAR szMenuString[MENU_STRING_SIZ];

   count = GetMenuItemCount(hMenu);

   if (count == (UINT) -1)
      return;

   for (pos = 0; pos < count; pos++) {

      id = GetMenuItemID(hMenu, pos);

      if (id  == (UINT) -1) {
         // must be a popup, recurse and update all ID's here
         if (hSubMenu = GetSubMenu(hMenu, pos))
            BiasMenu(hSubMenu, Bias);
      } else if (id) {
         // replace the item that was there with a new
         // one with the id adjusted

         GetMenuString(hMenu, pos, szMenuString, COUNTOF(szMenuString), MF_BYPOSITION);
         DeleteMenu(hMenu, pos, MF_BYPOSITION);
         InsertMenu(hMenu, pos, MF_BYPOSITION | MF_STRING, id + Bias, szMenuString);
      }
   }
}


VOID
InitExtensions()
{
   TCHAR szBuf[PROFILE_STRING_SIZ];
   TCHAR szPath[MAXPATHLEN];
   LPTSTR p;
   HANDLE hMod;
   FM_EXT_PROC fp;
   HMENU hMenu;
   INT iMax;
   HMENU hMenuFrame;
   HWND  hwndActive;
   INT iMenuOffset=0;
   BOOL bUnicode;

   hMenuFrame = GetMenu(hwndFrame);

   hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
   if (hwndActive && GetWindowLongPtr(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
      iMax = 1;
   else
      iMax = 0;

   GetPrivateProfileString(szAddons, NULL, szNULL, szBuf, COUNTOF(szBuf), szTheINIFile);

   for (p = szBuf; *p && iNumExtensions < MAX_EXTENSIONS; p += lstrlen(p) + 1) {

      GetPrivateProfileString(szAddons, p, szNULL, szPath, COUNTOF(szPath), szTheINIFile);

      hMod = LoadLibrary(szPath);

      if (hMod) {
         fp = (FM_EXT_PROC)GetProcAddress(hMod, FM_EXT_PROC_ENTRYW);
         if (fp) {
            bUnicode = TRUE;
         } else {
            fp = (FM_EXT_PROC)GetProcAddress(hMod, FM_EXT_PROC_ENTRYA);
            bUnicode = FALSE;
         }

         if (fp) {
            UINT bias;
            FMS_LOADA lsA;
            FMS_LOADW lsW;

            bias = ((IDM_EXTENSIONS + iNumExtensions + 1)*100);

            // We are now going to bias each menu, since extensions
            // don't know about each other and may clash IDM_xx if
            // we don't.

            // Our system is as follow:  IDMs 1000 - 6999 are reserved
            // for us (menus 0 - 5 inclusive).  Thus, IDMs
            // 7000 - 1199 is reserved for extensions.
            // This is why we added 1 in the above line to compute
            // bias: IDMs 0000-0999 are not used for menu 0.

            if (bUnicode)
               lsW.wMenuDelta = bias;
            else
               lsA.wMenuDelta = bias;

            if ((*fp)(hwndFrame, FMEVENT_LOAD, bUnicode ? (LPARAM)&lsW : (LPARAM)&lsA)) {

               if ((bUnicode ? lsW.dwSize : lsA.dwSize)
                  != (bUnicode ? sizeof(FMS_LOADW) : sizeof(FMS_LOADA)))
                  goto LoadFail;

               hMenu = bUnicode ? lsW.hMenu : lsA.hMenu;

               extensions[iNumExtensions].ExtProc = fp;
               extensions[iNumExtensions].Delta = bias;
               extensions[iNumExtensions].hModule = hMod;
               extensions[iNumExtensions].hMenu = hMenu;
               extensions[iNumExtensions].bUnicode = bUnicode;

               if (hMenu) {
                  BiasMenu(hMenu, bias);

                  if (bUnicode) {
                     InsertMenuW(hMenuFrame,
                        IDM_EXTENSIONS + iMenuOffset + iMax,
                        MF_BYPOSITION | MF_POPUP,
                        (UINT_PTR) hMenu, lsW.szMenuName);
                  } else {
                     InsertMenuA(hMenuFrame,
                        IDM_EXTENSIONS + iMenuOffset + iMax,
                        MF_BYPOSITION | MF_POPUP,
                        (UINT_PTR) hMenu, lsA.szMenuName);
                  }
                  iMenuOffset++;
               }

               iNumExtensions++;

            } else {
               goto LoadFail;
            }
         } else {
LoadFail:
            FreeLibrary(hMod);
         }
      }
   }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetSettings() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetSettings()
{
   TCHAR szTemp[128];
   INT size;
   INT weight;

   INT bfCharset;

   /* Get the flags out of the INI file. */
   bMinOnRun            = GetPrivateProfileInt(szSettings, szMinOnRun,            bMinOnRun,            szTheINIFile);
   bIndexOnLaunch       = GetPrivateProfileInt(szSettings, szIndexOnLaunch,       bIndexOnLaunch,       szTheINIFile);
   wTextAttribs         = (WORD)GetPrivateProfileInt(szSettings, szLowerCase,     wTextAttribs,         szTheINIFile);
   bStatusBar           = GetPrivateProfileInt(szSettings, szStatusBar,           bStatusBar,           szTheINIFile);
   bDisableVisualStyles = GetPrivateProfileInt(szSettings, szDisableVisualStyles, bDisableVisualStyles, szTheINIFile);

   bDriveBar       = GetPrivateProfileInt(szSettings, szDriveBar,      bDriveBar,      szTheINIFile);
   bToolbar        = GetPrivateProfileInt(szSettings, szToolbar,       bToolbar,       szTheINIFile);

   bNewWinOnConnect = GetPrivateProfileInt(szSettings, szNewWinOnNetConnect, bNewWinOnConnect, szTheINIFile);
   bConfirmDelete  = GetPrivateProfileInt(szSettings, szConfirmDelete, bConfirmDelete, szTheINIFile);
   bConfirmSubDel  = GetPrivateProfileInt(szSettings, szConfirmSubDel, bConfirmSubDel, szTheINIFile);
   bConfirmReplace = GetPrivateProfileInt(szSettings, szConfirmReplace,bConfirmReplace,szTheINIFile);
   bConfirmMouse   = GetPrivateProfileInt(szSettings, szConfirmMouse,  bConfirmMouse,  szTheINIFile);
   bConfirmFormat  = GetPrivateProfileInt(szSettings, szConfirmFormat, bConfirmFormat, szTheINIFile);
   bConfirmReadOnly= GetPrivateProfileInt(szSettings, szConfirmReadOnly, bConfirmReadOnly, szTheINIFile);
   uChangeNotifyTime= GetPrivateProfileInt(szSettings, szChangeNotifyTime, uChangeNotifyTime, szTheINIFile);
   bSaveSettings   = GetPrivateProfileInt(szSettings, szSaveSettings,  bSaveSettings, szTheINIFile);
   weight = GetPrivateProfileInt(szSettings, szFaceWeight, 400, szTheINIFile);

   GetPrivateProfileString(szSettings,
                           szSize,
                           bJAPAN ?
                              TEXT("14") :
                              TEXT("8"),
                           szTemp,
                           COUNTOF(szTemp),
                           szTheINIFile);

   size = GetHeightFromPointsString(szTemp);

   GetPrivateProfileString(szSettings,
                           szFace,
			   szHelv,
                           szTemp,
                           COUNTOF(szTemp),
                           szTheINIFile);


   bfCharset = bJAPAN ?
                  GetPrivateProfileInt(szSettings,
                                       szSaveCharset,
                                       SHIFTJIS_CHARSET,
                                       szTheINIFile) :
                  ANSI_CHARSET;

   hFont = CreateFont(size,
                      0, 0, 0, weight,
                      wTextAttribs & TA_ITALIC, 0, 0,
                      bfCharset,
                      OUT_DEFAULT_PRECIS,
                      CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY,
                      DEFAULT_PITCH | FF_SWISS,
                      szTemp);

}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetInternational() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetInternational()
{
   GetLocaleInfoW(lcid, LOCALE_STHOUSAND, (LPWSTR) szComma, COUNTOF(szComma));
   GetLocaleInfoW(lcid, LOCALE_SDECIMAL, (LPWSTR) szDecimal, COUNTOF(szDecimal));
}


INT
GetDriveOffset(register DRIVE drive)
{
   if (IsRemoteDrive(drive)) {

      if (aDriveInfo[drive].bRemembered)
         return dxDriveBitmap * 5;
      else
         return dxDriveBitmap * 4;
   }


   if (IsRemovableDrive(drive))
      return dxDriveBitmap * 1;

   if (IsRamDrive(drive))
      return dxDriveBitmap * 3;

   if (IsCDRomDrive(drive))
      return dxDriveBitmap * 0;

   return dxDriveBitmap * 2;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitMenus() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
InitMenus()
{
   HMENU hMenu;
   INT iMax;
   TCHAR szValue[MAXPATHLEN];
   HWND hwndActive;
   HMENU hMenuOptions;

   TCHAR szPathName[MAXPATHLEN];

   hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
   if (hwndActive && GetWindowLongPtr(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
      iMax = 1;
   else
      iMax = 0;

   GetPrivateProfileString(szSettings, szUndelete, szNULL, szValue, COUNTOF(szValue), szTheINIFile);

   if (szValue[0]) {

      // create explicit filename to avoid searching the path

      GetSystemDirectory(szPathName, COUNTOF(szPathName));
      AddBackslash(szPathName);
      lstrcat(szPathName, szValue);

      hModUndelete = LoadLibrary(szValue);

      if (hModUndelete) {
         lpfpUndelete = (FM_UNDELETE_PROC)GetProcAddress(hModUndelete, UNDELETE_ENTRYW);

         if (lpfpUndelete) {
            bUndeleteUnicode = TRUE;
         } else {
            lpfpUndelete = (FM_UNDELETE_PROC)GetProcAddress(hModUndelete, UNDELETE_ENTRYA);
            bUndeleteUnicode = FALSE;
         }

         if (lpfpUndelete) {
            hMenu = GetSubMenu(GetMenu(hwndFrame), IDM_FILE + iMax);
            LoadString(hAppInstance, IDS_UNDELETE, szValue, COUNTOF(szValue));
            InsertMenu(hMenu, 4, MF_BYPOSITION | MF_STRING, IDM_UNDELETE, szValue);
         }
      } else {
         FreeLibrary(hModUndelete);
         hModUndelete = NULL;
      }
   }

   //
   // use submenu because we are doing this by position
   //
   hMenu = GetSubMenu(GetMenu(hwndFrame), IDM_DISK + iMax);

   if (WNetStat(NS_CONNECTDLG)) {  // Network Connections...

      InsertMenu(hMenu, 4, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

      LoadString(hAppInstance, IDS_NEWWINONCONNECT, szValue, COUNTOF(szValue));
      hMenuOptions = GetSubMenu(GetMenu(hwndFrame), 4);
      InsertMenu(hMenuOptions, 8, MF_BYPOSITION | MF_STRING, IDM_NEWWINONCONNECT, szValue);

      LoadString(hAppInstance, IDS_CONNECT, szValue, COUNTOF(szValue));
      InsertMenu(hMenu, 5, MF_BYPOSITION | MF_STRING, IDM_CONNECT, szValue);

      LoadString(hAppInstance, IDS_DISCONNECT, szValue, COUNTOF(szValue));
      InsertMenu(hMenu, 6, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT, szValue);

   }

   // Shared Directories
   if (WNetStat(NS_SHAREDLG)) {

      InsertMenu(hMenu, 7, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

      LoadString(hAppInstance, IDS_SHAREAS, szValue, COUNTOF(szValue));
      InsertMenu(hMenu, 8, MF_BYPOSITION | MF_STRING, IDM_SHAREAS, szValue);

      LoadString(hAppInstance, IDS_STOPSHARE, szValue, COUNTOF(szValue));
      InsertMenu(hMenu, 9, MF_BYPOSITION | MF_STRING, IDM_STOPSHARE, szValue);

   }

   //
   // Init the Disk menu.
   //
   hMenu = GetMenu(hwndFrame);

   if (nFloppies == 0) {
      EnableMenuItem(hMenu, IDM_DISKCOPY, MF_BYCOMMAND | MF_GRAYED);
   }


   if (bStatusBar)
      CheckMenuItem(hMenu, IDM_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
   if (bMinOnRun)
      CheckMenuItem(hMenu, IDM_MINONRUN,  MF_BYCOMMAND | MF_CHECKED);
   if (bIndexOnLaunch)
      CheckMenuItem(hMenu, IDM_INDEXONLAUNCH, MF_BYCOMMAND | MF_CHECKED);

   if (bSaveSettings)
      CheckMenuItem(hMenu, IDM_SAVESETTINGS,  MF_BYCOMMAND | MF_CHECKED);

   if (bDriveBar)
      CheckMenuItem(hMenu, IDM_DRIVEBAR, MF_BYCOMMAND | MF_CHECKED);
   if (bToolbar)
      CheckMenuItem(hMenu, IDM_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);

   if (bNewWinOnConnect)
      CheckMenuItem(hMenu, IDM_NEWWINONCONNECT, MF_BYCOMMAND | MF_CHECKED);


   //
   // Init menus after the window/menu has been created
   //
   InitExtensions();

   InitToolbarButtons();

   //
   // Redraw the menu bar since it's already displayed
   //
   DrawMenuBar(hwndFrame);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  BoilThatDustSpec() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Parses the command line (if any) passed into WINFILE and exec's any tokens
 * it may contain.
 */

VOID
BoilThatDustSpec(register TCHAR *pStart, BOOL bLoadIt)
{
   register TCHAR *pEnd;
   DWORD         ret;
   BOOL          bFinished;

   if (*pStart == CHAR_NULL)
      return;

   bFinished = FALSE;
   while (!bFinished) {
      pEnd = pStart;
      while ((*pEnd) && (*pEnd != CHAR_SPACE) && (*pEnd != CHAR_COMMA))
         ++pEnd;

      if (*pEnd == CHAR_NULL)
         bFinished = TRUE;
      else
         *pEnd = CHAR_NULL;

      ret = ExecProgram(pStart, szNULL, NULL, bLoadIt, FALSE);
      if (ret)
         MyMessageBox(NULL, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

      pStart = pEnd+1;
   }
}


//
// BOOL LoadBitmaps()
//
// this routine loads DIB bitmaps, and "fixes up" their color tables
// so that we get the desired result for the device we are on.
//
// this routine requires:
//      the DIB is a 16 color DIB authored with the standard windows colors
//      bright green (FF 00 00) is converted to the background color!
//      light grey  (C0 C0 C0) is replaced with the button face color
//      dark grey   (80 80 80) is replaced with the button shadow color
//
// this means you can't have any of these colors in your bitmap
//

#define BACKGROUND      0x0000FF00      // bright green
#define BACKGROUNDSEL   0x00FF00FF      // bright red
#define BUTTONFACE      0x00C0C0C0      // bright grey
#define BUTTONSHADOW    0x00808080      // dark grey

DWORD
FlipColor(DWORD rgb)
{
   return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}


BOOL
LoadBitmaps(VOID)
{
   HDC                   hdc;
   HANDLE                h;
   DWORD FAR             *p;
   LPBYTE                lpBits;
   HANDLE                hRes;
   LPBITMAPINFOHEADER    lpBitmapInfo;
   INT                   numcolors;
   DWORD                 rgbSelected;
   DWORD                 rgbUnselected;
   UINT                  cbBitmapSize;
   LPBITMAPINFOHEADER    lpBitmapData;

   rgbSelected = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
   rgbUnselected = FlipColor(GetSysColor(COLOR_WINDOW));

   h = FindResource(hAppInstance, (LPTSTR) MAKEINTRESOURCE(BITMAPS), RT_BITMAP);
   hRes = LoadResource(hAppInstance, h);

   // Lock the bitmap data and make a copy of it for the mask and the bitmap.
   //
   cbBitmapSize = SizeofResource( hAppInstance, h );
   lpBitmapData  = (LPBITMAPINFOHEADER)( hRes );

   lpBitmapInfo = (LPBITMAPINFOHEADER) LocalAlloc(LMEM_FIXED, cbBitmapSize);

   if (!lpBitmapInfo) {
      return FALSE;
   }

   CopyMemory( (PBYTE)lpBitmapInfo, (PBYTE)lpBitmapData, cbBitmapSize );

   //
   // Get a pointer into the color table of the bitmaps, cache the number of
   // bits per pixel
   //
   p = (DWORD FAR *)((PBYTE)(lpBitmapInfo) + lpBitmapInfo->biSize);

   //
   // Search for the Solid Blue entry and replace it with the current
   // background RGB.
   //
   numcolors = 16;

   while (numcolors-- > 0) {
      if (*p == BACKGROUND)
         *p = rgbUnselected;
      else if (*p == BACKGROUNDSEL)
         *p = rgbSelected;
      else if (*p == BUTTONFACE)
         *p = FlipColor(GetSysColor(COLOR_BTNFACE));
      else if (*p == BUTTONSHADOW)
         *p = FlipColor(GetSysColor(COLOR_BTNSHADOW));

      p++;
   }

   //
   // Now create the DIB.
   //

   //
   // First skip over the header structure
   //
   lpBits = (LPBYTE)(lpBitmapInfo + 1);

   //
   //  Skip the color table entries, if any
   //
   lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

   //
   // Create a color bitmap compatible with the display device
   //
   hdc = GetDC(NULL);
   if (hdcMem = CreateCompatibleDC(hdc)) {

      if (hbmBitmaps = CreateDIBitmap(hdc, lpBitmapInfo, (DWORD)CBM_INIT, lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS))
         hbmSave = SelectObject(hdcMem, hbmBitmaps);

   }
   ReleaseDC(NULL, hdc);

#ifndef KKBUGFIX
//It is not necessary to free a resource loaded by using the LoadResource function.
   LocalUnlock(hRes);
   FreeResource(hRes);
#endif

   LocalFree(lpBitmapInfo);

   return TRUE;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     GetSavedWindow
//
// Synopsis:
//
// IN        szBuf  buffer to parse out all save win info (NULL = default)
// OUT       pwin   pwin filled with szBuf fields.
//
// Return:
//
//
// Assumes:
//
// Effects:
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////


VOID
GetSavedWindow(
   LPWSTR szBuf,
   PWINDOW pwin)
{
   PINT pint;
   INT count;

   //
   // defaults
   //
   // (CW_USEDEFAULT must be type casted since it is defined
   // as an int.
   //
   pwin->rc.right = pwin->rc.left = (LONG)CW_USEDEFAULT;
   pwin->pt.x = pwin->pt.y = pwin->rc.top = pwin->rc.bottom = 0;
   pwin->sw = SW_SHOWNORMAL;
   pwin->dwSort = IDD_NAME;
   pwin->dwView = VIEW_NAMEONLY;
   pwin->dwAttribs = ATTR_DEFAULT;
   pwin->nSplit = 0;

   pwin->szDir[0] = CHAR_NULL;

   if (!szBuf)
      return;

   //
   // BUGBUG: (LATER)
   // This assumes the members of the point structure are
   // the same size as INT (sizeof(LONG) == sizeof(INT) == sizeof(UINT))!
   //

   count = 0;
   pint = (PINT)&pwin->rc;         // start by filling the rect

   while (*szBuf && count < 11) {

      *pint++ = atoi(szBuf);  // advance to next field

      while (*szBuf && *szBuf != CHAR_COMMA)
         szBuf++;

      while (*szBuf && *szBuf == CHAR_COMMA)
         szBuf++;

      count++;
   }

   lstrcpy(pwin->szDir, szBuf);    // this is the directory
}


BOOL
CheckDirExists(
   LPWSTR szDir)
{
   BOOL bRet = FALSE;

   if (IsNetDrive(DRIVEID(szDir)) == 2) {

      CheckDrive(hwndFrame, DRIVEID(szDir), FUNC_SETDRIVE);
      return TRUE;
   }

   if (IsValidDisk(DRIVEID(szDir)))
      bRet = SetCurrentDirectory(szDir);

   return bRet;
}


BOOL
CreateSavedWindows()
{
   WCHAR buf[2*MAXPATHLEN+7*7], key[10];
   WINDOW win;

   //
   // since win.szDir is bigger.
   //
   WCHAR szDir[2*MAXPATHLEN];

   INT nDirNum;
   HWND hwnd;
   INT iNumTrees;

   //
   // make sure this thing exists so we don't hit drives that don't
   // exist any more
   //
   nDirNum = 1;
   iNumTrees = 0;

   do {
      wsprintf(key, szDirKeyFormat, nDirNum++);

      GetPrivateProfileString(szSettings, key, szNULL, buf, COUNTOF(buf), szTheINIFile);

      if (*buf) {

         GetSavedWindow(buf, &win);

         //
         // clean off some junk so we
         // can do this test
         //
         lstrcpy(szDir, win.szDir);
         StripFilespec(szDir);
         StripBackslash(szDir);

         if (!CheckDirExists(szDir))
            continue;

         dwNewView = win.dwView;
         dwNewSort = win.dwSort;
         dwNewAttribs = win.dwAttribs;

         hwnd = CreateTreeWindow(win.szDir,
                                 win.rc.left,
                                 win.rc.top,
                                 win.rc.right - win.rc.left,
                                 win.rc.bottom - win.rc.top,
                                 win.nSplit);

         if (!hwnd) {
            continue;
         }

         iNumTrees++;

         //
         // keep track of this for now...
         //
         if (IsIconic(hwnd))
             SetWindowPos(hwnd, NULL, win.pt.x, win.pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

         ShowWindow(hwnd, win.sw);
      }

   } while (*buf);

   //
   // if nothing was saved create a tree for the current drive
   //
   if (!iNumTrees) {

      lstrcpy(buf, szOriginalDirPath);
      AddBackslash(buf);
      lstrcat(buf, szStarDotStar);

      //
      // default to split window
      //
      hwnd = CreateTreeWindow(buf, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, -1);

      if (!hwnd)
         return FALSE;

      iNumTrees++;
   }

   return TRUE;
}


// void  GetTextStuff(HDC hdc)
//
// this computes all the globals that are dependent on the
// currently selected font
//
// in:
//      hdc     DC with font selected into it
//

VOID
GetTextStuff(HDC hdc)
{
   SIZE size;
   TEXTMETRIC tm;

   GetTextExtentPoint32(hdc, TEXT("W"), 1, &size);

   dxText = size.cx;
   dyText = size.cy;

   //
   // the old method of GetTextExtent("M") was not good enough for the
   // drive bar because in Arial 8 (the default font), a lowercase 'w'
   // is actually wider than an uppercase 'M', which causes drive W in
   // the drive bar to clip.  rather than play around with a couple of
   // sample letters and taking the max, or fudging the width, we just
   // get the metrics and use the real max character width.
   //
   // to avoid the tree window indents looking too wide, we use the old
   // 'M' extent everywhere besides the drive bar, though.
   //
   GetTextMetrics(hdc, &tm);

   //
   // these are all dependent on the text metrics
   //
   dxDrive = dxDriveBitmap + tm.tmMaxCharWidth + (4*dyBorderx2);
   dyDrive = max(dyDriveBitmap + (4*dyBorderx2), dyText);
   dyFileName = max(dyText, dyFolder);  //  + dyBorder;
}

UINT
FillDocType(
   PPDOCBUCKET ppDoc,
   LPCWSTR pszSection,
   LPCWSTR pszDefault)
{
   LPWSTR pszDocuments = NULL;
   LPWSTR p;
   LPWSTR p2;
   UINT uLen = 0;

   UINT uRetval = 0;

   do {

      uLen += 32;

      if (pszDocuments)
         LocalFree((HLOCAL)pszDocuments);

      pszDocuments = LocalAlloc(LMEM_FIXED, uLen*sizeof(WCHAR));

      if (!pszDocuments) {
         return 0;
      }

   } while (GetProfileString(szWindows,
                             pszSection,
                             pszDefault,
                             pszDocuments,
                             uLen) == (DWORD)uLen-2);

   //
   // Parse through string, searching for blanks
   //
   for (p=pszDocuments; *p; p++) {

      if (CHAR_SPACE == *p) {
         *p = CHAR_NULL;
      }
   }

   for(p2=pszDocuments; p2<p; p2++) {

      if (*p2) {
         if (DocInsert(ppDoc, p2, NULL) == 1)
            uRetval++;

         while(*p2 && p2!=p)
            p2++;
      }
   }

   LocalFree((HLOCAL)pszDocuments);

   return uRetval;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  InitFileManager() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
InitFileManager(
   HANDLE hInstance,
   LPWSTR lpCmdLine,
   INT nCmdShow)
{
   INT           i;
   HDC           hdcScreen;

   //
   // 2*MAXPATHLEN since may have huge filter.
   //
   WCHAR         szBuffer[2*MAXPATHLEN];

   HCURSOR       hcurArrow;
   WNDCLASS      wndClass;
   WINDOW        win;
   HWND          hwnd;
   HANDLE        hOld;
   RECT          rcT, rcS;
   WCHAR         szTemp[80];
   DWORD         Ignore;

   HANDLE        hThread;
   DWORD         dwRetval;

   hThread = GetCurrentThread();

   SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);

   InitializeCriticalSection(&CriticalSectionPath);

   // ProfStart();

   //
   // Preserve this instance's module handle
   //
   hAppInstance = hInstance;

   if (*lpCmdLine)
      nCmdShow = SW_SHOWMINNOACTIVE;

   // setup ini file location
   lstrcpy(szTheINIFile, szBaseINIFile);
   dwRetval = GetEnvironmentVariable(TEXT("APPDATA"), szBuffer, MAXPATHLEN);
   if (dwRetval > 0 && dwRetval <= (DWORD)(MAXPATHLEN - lstrlen(szRoamINIPath) - 1 - lstrlen(szBaseINIFile) - 1)) {
	   wsprintf(szTheINIFile, TEXT("%s%s"), szBuffer, szRoamINIPath);
	   if (CreateDirectory(szTheINIFile, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
		   wsprintf(szTheINIFile, TEXT("%s%s\\%s"), szBuffer, szRoamINIPath, szBaseINIFile);
	   }
	   else {
		   wsprintf(szTheINIFile, TEXT("%s\\%s"), szBuffer, szBaseINIFile);
	   }
   }

   // e.g., UILanguage=zh-CN; UI language defaults to OS set language or English if that language is not supported.
   GetPrivateProfileString(szSettings, szUILanguage, szNULL, szTemp, COUNTOF(szTemp), szTheINIFile);
   if (szTemp[0])
   {
       LCID lcidUI = LocaleNameToLCID(szTemp, 0);
       if (lcidUI != 0)
       {
           SetThreadUILanguage((LANGID)lcidUI);

           // update to current local used for dispaly
           SetThreadLocale(lcidUI);
       }
   }

   lcid = GetThreadLocale();

JAPANBEGIN
   bJapan = (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE);
JAPANEND

   //
   // Constructors for info system.
   // Must always do since these never fail and we don't test for
   // non-initialization during destruction (if FindWindow returns valid)
   //
   M_Info();

   M_Type();
   M_Space();
   M_NetCon();
   M_VolInfo();

   //
   // Bounce errors to us, not fs.
   //
   SetErrorMode(1);

   for (i=0; i<26;i++) {
      I_Space(i);
   }

	if (OleInitialize(0) != NOERROR)
		return FALSE;
	
   if (lpfnRegisterPenApp = (FNPENAPP)GetProcAddress((HANDLE)GetSystemMetrics(SM_PENWINDOWS), chPenReg))
      (*lpfnRegisterPenApp)(1, TRUE);


   //
   // Remember the current directory.
   //
   GetCurrentDirectory(COUNTOF(szOriginalDirPath), szOriginalDirPath);

   if (*lpCmdLine) {

      if (dwRetval = ExecProgram(lpCmdLine, pszNextComponent(lpCmdLine), NULL, FALSE, FALSE))
         MyMessageBox(NULL, IDS_EXECERRTITLE, dwRetval, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      else
         nCmdShow = SW_SHOWMINNOACTIVE;
   }

   //
   // Read WINFILE.INI and set the appropriate variables.
   //
   GetSettings();

   dyBorder = GetSystemMetrics(SM_CYBORDER);
   dyBorderx2 = dyBorder * 2;
   dxFrame = GetSystemMetrics(SM_CXFRAME) - dyBorder;

   dxDriveBitmap = DRIVES_WIDTH;
   dyDriveBitmap = DRIVES_HEIGHT;
   dxFolder = FILES_WIDTH;
   dyFolder = FILES_HEIGHT;

   LoadUxTheme();

   if (!LoadBitmaps())
      return FALSE;

   hicoTree = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(TREEICON));
   hicoTreeDir = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(TREEDIRICON));
   hicoDir = LoadIcon(hAppInstance, (LPTSTR) MAKEINTRESOURCE(DIRICON));

   chFirstDrive = CHAR_a;

   // now build the parameters based on the font we will be using

   hdcScreen = GetDC(NULL);

   hOld = SelectObject(hdcScreen, hFont);
   GetTextStuff(hdcScreen);
   if (hOld)
      SelectObject(hdcScreen, hOld);

   dxClickRect = max(GetSystemMetrics(SM_CXDOUBLECLK) / 2, 2 * dxText);
   dyClickRect = max(GetSystemMetrics(SM_CYDOUBLECLK) / 2, dyText);

   GetPrivateProfileString(szSettings,
                           szDriveListFace,
                           szHelv,
                           szTemp,
                           COUNTOF(szTemp),
                           szTheINIFile);

   hfontDriveList = CreateFont(GetHeightFromPointsString(TEXT("8")),
                               0, 0,
                               0, 400, 0,
                               0,
                               0,
                               bJAPAN ?
                                 SHIFTJIS_CHARSET :
                                 ANSI_CHARSET,
                               OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY,
                               VARIABLE_PITCH | FF_SWISS,
                               szTemp);

    // use the smaller font for Status bar so that messages fix in it.
   if( bJAPAN) {
       hFontStatus = CreateFont(GetHeightFromPointsString(TEXT("10")),
                               0, 0,
                               0, 400, 0,
                               0,
                               0,
                               SHIFTJIS_CHARSET,
                               OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY,
                               VARIABLE_PITCH,
                               szHelv );
    }
   ReleaseDC(NULL, hdcScreen);

   /* Load the accelerator table. */
   hAccel = LoadAccelerators(hInstance, (LPTSTR) MAKEINTRESOURCE(WFACCELTABLE));

   wHelpMessage = RegisterWindowMessage(TEXT("ShellHelp"));
   wBrowseMessage = RegisterWindowMessage(TEXT("commdlg_help"));

   hhkMsgFilter = SetWindowsHook(WH_MSGFILTER, (HOOKPROC)MessageFilter);

   hcurArrow = LoadCursor(NULL, IDC_ARROW);

   wndClass.lpszClassName  = szFrameClass;
   wndClass.style          = 0L;
   wndClass.lpfnWndProc    = FrameWndProc;
   wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = 0;
   wndClass.hInstance      = hInstance;
   wndClass.hIcon          = LoadIcon(hInstance, (LPTSTR) MAKEINTRESOURCE(APPICON));
   wndClass.hCursor        = hcurArrow;
   wndClass.hbrBackground  = (HBRUSH)(COLOR_APPWORKSPACE + 1); // COLOR_WINDOW+1;
   wndClass.lpszMenuName   = (LPTSTR) MAKEINTRESOURCE(FRAMEMENU);

   if (!RegisterClass(&wndClass)) {
      return FALSE;
   }

   wndClass.lpszClassName  = szTreeClass;

   wndClass.style          = 0;  //WS_CLIPCHILDREN;  //CS_VREDRAW | CS_HREDRAW;

   wndClass.lpfnWndProc    = TreeWndProc;
// wndClass.cbClsExtra     = 0;

   wndClass.cbWndExtra     = GWL_LASTFOCUS + sizeof(LONG_PTR);


// wndClass.hInstance      = hInstance;
   wndClass.hIcon          = NULL;
   wndClass.hCursor        = LoadCursor(hInstance, (LPTSTR) MAKEINTRESOURCE(SPLITCURSOR));
   wndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
   wndClass.lpszMenuName   = NULL;

   if (!RegisterClass(&wndClass)) {
      return FALSE;
   }

   wndClass.lpszClassName  = szDrivesClass;
   wndClass.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
   wndClass.lpfnWndProc    = DrivesWndProc;
// wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = sizeof(LONG_PTR) +// GWL_CURDRIVEIND
                             sizeof(LONG_PTR) +// GWL_CURDRIVEFOCUS
                             sizeof(LONG_PTR); // GWL_LPSTRVOLUME

// wndClass.hInstance      = hInstance;
// wndClass.hIcon          = NULL;
   wndClass.hCursor        = hcurArrow;
   wndClass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
// wndClass.lpszMenuName   = NULL;

   if (!RegisterClass(&wndClass)) {
      return FALSE;
   }

   wndClass.lpszClassName  = szTreeControlClass;
   wndClass.style          = CS_DBLCLKS;
   wndClass.lpfnWndProc    = TreeControlWndProc;
// wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = 2 * sizeof(LONG_PTR); // GWL_READLEVEL, GWL_XTREEMAX
// wndClass.hInstance      = hInstance;
// wndClass.hIcon          = NULL;
   wndClass.hCursor        = hcurArrow;
   wndClass.hbrBackground  = NULL;
// wndClass.lpszMenuName   = NULL;

   if (!RegisterClass(&wndClass)) {
      return FALSE;
   }

   wndClass.lpszClassName  = szDirClass;
   wndClass.style          = 0;  //CS_VREDRAW | CS_HREDRAW;
   wndClass.lpfnWndProc    = DirWndProc;
// wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = GWL_OLEDROP + sizeof(LONG_PTR);
// wndClass.hInstance      = hInstance;
   wndClass.hIcon          = NULL;
// wndClass.hCursor        = hcurArrow;
   wndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
// wndClass.lpszMenuName   = NULL;

   if (!RegisterClass(&wndClass)) {
      return FALSE;
   }

   wndClass.lpszClassName  = szSearchClass;
   wndClass.style          = 0L;
   wndClass.lpfnWndProc    = SearchWndProc;
// wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = GWL_LASTFOCUS + sizeof(LONG_PTR);

// wndClass.hInstance      = hInstance;
   wndClass.hIcon          = LoadIcon(hInstance, (LPTSTR) MAKEINTRESOURCE(DIRICON));
// wndClass.hCursor        = NULL;
   wndClass.hbrBackground  = NULL;
// wndClass.lpszMenuName   = NULL;

   if (!RegisterClass(&wndClass)) {
      return FALSE;
   }

#if 0
   wndClass.lpszClassName  = szListbBox;
   wndClass.style          = 0L;
   wndClass.lpfnWndProc    = DirListBoxWndProc;
// wndClass.cbClsExtra     = 0;
   wndClass.cbWndExtra     = sizeof(LONG_PTR);

// wndClass.hInstance      = hInstance;
   wndClass.hIcon          = LoadIcon(hInstance, (LPTSTR) MAKEINTRESOURCE(DIRICON));
// wndClass.hCursor        = NULL;
   wndClass.hbrBackground  = NULL;
// wndClass.lpszMenuName   = NULL;

   if ((atomDirListBox = RegisterClass(&wndClass)) == 0) {
      return FALSE;
   }
#endif

   if (!LoadString(hInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle))) {
      return FALSE;
   }

   GetPrivateProfileString(szSettings, szWindow, szNULL, szBuffer, COUNTOF(szBuffer), szTheINIFile);
   GetSavedWindow(szBuffer, &win);

   // Check that at least some portion of the window is visible;
   // set to screen size if not

   // OLD: SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rcT, 0);
   rcT.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
   rcT.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
   rcT.right = rcT.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
   rcT.bottom = rcT.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

   // right and bottom are width and height, so convert to coordinates

   // NOTE: in the cold startup case (no value in winfile.ini), the coordinates are
   // left: CW_USEDEFAULT, top: 0, right: CW_USEDEFAULT, bottom: 0.

   win.rc.right += win.rc.left;
   win.rc.bottom += win.rc.top;

   if (!IntersectRect(&rcS, &rcT, &win.rc))
   {
      // window off virtual screen or initial case; reset to defaults
       win.rc.right = win.rc.left = (LONG)CW_USEDEFAULT;
       win.rc.top = win.rc.bottom = 0;

       // compenstate as above so the conversion below still results in the defaults
       win.rc.right += win.rc.left;
       win.rc.bottom += win.rc.top;
   }

   // Now convert back again

   win.rc.right -= win.rc.left;
   win.rc.bottom -= win.rc.top;


   //
   // Check to see if another copy of winfile is running.  If
   // so, either bring it to the foreground, or restore it to
   // a window (from an icon).
   //
   {
      HWND hwndPrev;
      HWND hwnd;

      hwndPrev = NULL; // FindWindow (szFrameClass, NULL);

      if (hwndPrev != NULL) {
         //  For Win32, this will accomplish almost the same effect as the
         //  above code does for Win 3.0/3.1   [stevecat]

         hwnd = GetLastActivePopup(hwndPrev);

         if (IsIconic(hwndPrev)) {
            ShowWindow (hwndPrev, SW_RESTORE);
            SetForegroundWindow (hwndPrev);
         } else {
            SetForegroundWindow (hwnd);
         }

         return FALSE;
      }

   }


   if (!CreateWindowEx(0L, szFrameClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      win.rc.left, win.rc.top, win.rc.right, win.rc.bottom,
      NULL, NULL, hInstance, NULL)) {

      return FALSE;
   }

   // Now, we need to start the Change Notify thread, so that any changes
   // that happen will be notified.  This thread will create handles for
   // each viewed directory and wait on them.  If any of the handles are
   // awakened, then the appropriate directory will be sent a message to
   // refresh itself.  Another thread will also be created to watch for
   // network connections/disconnections.  This will be done via the
   // registry.  !! LATER !!

   InitializeWatchList();

   bUpdateRun = TRUE;
   //
   // Now create our worker thread and two events to communicate by
   //
   hEventUpdate        = CreateEvent(NULL, TRUE, FALSE, NULL);
   hEventUpdatePartial = CreateEvent(NULL, TRUE, FALSE, NULL);
   hEventNetLoad       = CreateEvent(NULL, TRUE, FALSE, NULL);
   hEventAcledit       = CreateEvent(NULL, TRUE, FALSE, NULL);

   hThreadUpdate = CreateThread( NULL,
      0L,
      (LPTHREAD_START_ROUTINE)UpdateInit,
      NULL,
      0L,
      &Ignore);

   if (!hEventUpdate ||
      !hEventUpdatePartial ||
      !hThreadUpdate ||
      !hEventNetLoad ||
      !hEventAcledit) {

      LoadFailMessage();

      bUpdateRun = FALSE;

      return FALSE;
   }

   SetThreadPriority(hThreadUpdate, THREAD_PRIORITY_BELOW_NORMAL);

   //
   // Reset simple drive info, and don't let anyone block
   //
   ResetDriveInfo();
   SetEvent(hEventUpdatePartial);

   nFloppies = 0;
   for (i=0; i < cDrives; i++) {
      if (IsRemovableDrive(rgiDrive[i])) {

         //
         // Avoid Phantom B: problems.
         //
         if ((nFloppies == 1) && (i > 1))
            nFloppies = 2;

         nFloppies++;
      }
   }

   //
   // Turn off redraw for hwndDriveList since we will display it later
   // (avoid flicker)
   //
   SendMessage(hwndDriveList,WM_SETREDRAW,0,0l);

   //
   // support forced min or max
   //
   if (nCmdShow == SW_SHOW || nCmdShow == SW_SHOWNORMAL &&
      win.sw != SW_SHOWMINIMIZED)

      nCmdShow = win.sw;

   ShowWindow(hwndFrame, nCmdShow);

   if (bDriveBar) {
      //
      // Update the drive bar
      //
      InvalidateRect(hwndDriveBar, NULL, TRUE);
   }
   UpdateWindow(hwndFrame);

   //
   // Party time...
   // Slow stuff here
   //

   LoadString(hInstance, IDS_DIRSREAD, szDirsRead, COUNTOF(szDirsRead));
   LoadString(hInstance, IDS_BYTES, szBytes, COUNTOF(szBytes));
   LoadString(hInstance, IDS_SBYTES, szSBytes, COUNTOF(szSBytes));

   //
   // Read the International constants out of WIN.INI.
   //
   GetInternational();

   if (ppProgBucket = DocConstruct()) {

      FillDocType(ppProgBucket, L"Programs", szDefPrograms);
   }

   BuildDocumentStringWorker();

   if (!InitDirRead()) {

      LoadFailMessage();
      return FALSE;
   }

   //
   // Now draw drive list box
   //
   FillToolbarDrives(0);
   SendMessage(hwndDriveList,WM_SETREDRAW,(WPARAM)TRUE,0l);
   InvalidateRect(hwndDriveList, NULL, TRUE);


   //
   // Init Menus fast stuff
   //
   // Init the Disk menu
   //
   InitMenus();


   if (!CreateSavedWindows()) {
      return FALSE;
   }

   ShowWindow(hwndMDIClient, SW_NORMAL);

   //
   // Kick start the background net read
   //
   // Note: We must avoid deadlock situations by kick starting the network
   // here.  There is a possible deadlock if you attempt a WAITNET() before
   // hEventUpdate is first set.  This can occur in the wfYield() step
   // of the background read below.
   //
   SetEvent(hEventUpdate);

   // now refresh all tree windows (start background tree read)
   //
   // since the tree reads happen in the background the user can
   // change the Z order by activating windows once the read
   // starts.  to avoid missing a window we must restart the
   // search through the MDI child list, checking to see if the
   // tree has been read yet (if there are any items in the
   // list box).  if it has not been read yet we start the read

   hwnd = GetWindow(hwndMDIClient, GW_CHILD);

   while (hwnd) {
      HWND hwndTree;

      if ((hwndTree = HasTreeWindow(hwnd)) &&
         (INT)SendMessage(GetDlgItem(hwndTree, IDCW_TREELISTBOX), LB_GETCOUNT, 0, 0L) == 0) {

         SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(FALSE, 0), 0L);
         hwnd = GetWindow(hwndMDIClient, GW_CHILD);
      } else {
         hwnd = GetWindow(hwnd, GW_HWNDNEXT);
      }
   }

   // ProfStop();

   // drive refresh taken out of update drive list, must be manually
   // placed here for screen init.  Do last.
   // Commented out (unnecessary)
   //    FillToolbarDrives(GetSelectedDrive());

   SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);

   if (bIndexOnLaunch)
   {
      StartBuildingDirectoryTrie();
   }

   return TRUE;
}


VOID
DeleteBitmaps()
{
   if (hdcMem) {

      SelectObject(hdcMem, hbmSave);

      if (hbmBitmaps)
         DeleteObject(hbmBitmaps);
      DeleteDC(hdcMem);
   }
}


//--------------------------------------------------------------------------
//
//  FreeFileManager() -
//
//--------------------------------------------------------------------------

VOID
FreeFileManager()
{
   if (hThreadUpdate && bUpdateRun) {
      UpdateWaitQuit();
      CloseHandle(hThreadUpdate);
   }

   DeleteCriticalSection(&CriticalSectionPath);

#define CLOSEHANDLE(handle) if (handle) CloseHandle(handle)

   CLOSEHANDLE(hEventNetLoad);
   CLOSEHANDLE(hEventAcledit);
   CLOSEHANDLE(hEventUpdate);
   CLOSEHANDLE(hEventUpdatePartial);

   DestroyWatchList();
   DestroyDirRead();

   D_Info();

   D_Type();
   D_Space();
   D_NetCon();
   D_VolInfo();

   DocDestruct(ppDocBucket);
   DocDestruct(ppProgBucket);

   if (lpfnRegisterPenApp)
      (*lpfnRegisterPenApp)(1, FALSE);

   DeleteBitmaps();

   if (hFont)
      DeleteObject(hFont);

   if (hfontDriveList)
      DeleteObject(hfontDriveList);

    // use the smaller font for Status bar so that messages fix in it.
    if( bJAPAN ) {
        if (hFontStatus)
            DeleteObject(hFontStatus);
    }

   //
   // Free the fmifs junk
   //
   if (hfmifsDll)
      FreeLibrary(hfmifsDll);

   if (hNTLanman)
      FreeLibrary(hNTLanman);

   if (hMPR)
      FreeLibrary(hMPR);

   if (hVersion)
      FreeLibrary(hVersion);

	OleUninitialize();

#undef CLOSEHANDLE
}


/////////////////////////////////////////////////////////////////////
//
// Name:     LoadFailMessage
//
// Synopsis: Puts up message on load failure.
//
// IN        VOID
//
// Return:   VOID
//
//
// Assumes:
//
// Effects:
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
LoadFailMessage(VOID)
{
   WCHAR szMessage[MAXMESSAGELEN];

   szMessage[0]=0;

   LoadString(hAppInstance,
      IDS_INITUPDATEFAIL,
      szMessage,
      COUNTOF(szMessage));

   FormatError(FALSE, szMessage, COUNTOF(szMessage), GetLastError());

   LoadString(hAppInstance,
      IDS_INITUPDATEFAILTITLE,
      szTitle,
      COUNTOF(szTitle));

   MessageBox(hwndFrame, szMessage, szTitle, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

   return;
}

/////////////////////////////////////////////////////////////////////
//
// Name:     LoadUxTheme
//
// Synopsis: Loads function SetWindowTheme dynamically
//
// IN:       VOID
//
// Return:   BOOL  T=Success, F=FAILURE
//
//
// Assumes:
//
// Effects:  hUxTheme, lpfnSetWindowTheme
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL
LoadUxTheme(VOID)
{
  UINT uErrorMode;

  //
  // Have we already loaded it?
  //
  if (hUxTheme)
    return TRUE;

  //
  // Let the system handle errors here
  //
  uErrorMode = SetErrorMode(0);
  hUxTheme = LoadLibrary(UXTHEME_DLL);
  SetErrorMode(uErrorMode);

  if (!hUxTheme)
    return FALSE;

#define GET_PROC(x) \
   if (!(lpfn##x = (PVOID) GetProcAddress(hUxTheme, UXTHEME_##x))) \
      return FALSE

  GET_PROC(SetWindowTheme);

#undef GET_PROC

  return TRUE;
}
