/********************************************************************

   tbar.c

   Windows File System Toolbar support routines

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

#define DRIVELIST_BORDER        3
#define MINIDRIVE_MARGIN        4

// The height of the drivelist combo box is defined
// as the height of a character + DRIVELIST_BORDER
// LATER: create a specific global for this to avoid
// redundant computations.

// ASSUMES: height of combobox-padding > size of bitmap!


#define MAXDESCLEN              128
#define HIDEIT                  0x8000
#define Static

static HWND hwndExtensions = NULL;

static DWORD dwSaveHelpContext; // saves dwContext in tbcustomize dialog

static UINT uExtraCommands[] =
  {
    IDM_CONNECT,
    IDM_DISCONNECT,
    IDM_CONNECTIONS,
    IDM_SHAREAS,
    IDM_STOPSHARE,
    IDM_SHAREAS,

    IDM_UNDELETE,
    IDM_NEWWINONCONNECT,
    IDM_PERMISSIONS,
    IDM_COMPRESS,
    IDM_UNCOMPRESS,
  } ;
#define NUMEXTRACOMMANDS  (sizeof(uExtraCommands)/sizeof(uExtraCommands[0]))

/* Note that the idsHelp field is used internally to determine if the
 * button is "available" or not.
 */
static TBBUTTON tbButtons[] = {
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  { 0, IDM_CONNECTIONS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 1, IDM_DISCONNECT , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  { 2, IDM_SHAREAS    , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 3, IDM_STOPSHARE  , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  { 4, IDM_VNAME      , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 5, IDM_VDETAILS   , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  { 6, IDM_BYNAME     , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 7, IDM_BYTYPE     , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 8, IDM_BYSIZE     , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 9, IDM_BYDATE     , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  {10, IDM_NEWWINDOW  , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  {11, IDM_COPY       , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  {12, IDM_MOVE       , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  {13, IDM_DELETE     , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
  { 0, 0              , TBSTATE_ENABLED, TBSTYLE_SEP   , 0 },
  {27, IDM_PERMISSIONS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
};

#define ICONNECTIONS 1  /* Index of the Connections button */

#define TBAR_BITMAP_COUNT 14  /* number of std toolbar bitmaps */
#define TBAR_BUTTON_COUNT (sizeof(tbButtons)/sizeof(tbButtons[0]))

static struct {
  int idM;
  int idB;
} sAllButtons[] = {
  IDM_MOVE,             12,
  IDM_COPY,             11,
  IDM_DELETE,           13,
  IDM_RENAME,           14,
  IDM_ATTRIBS,          15,
  IDM_PRINT,            16,
  IDM_MAKEDIR,          17,
  IDM_SEARCH,           18,
  IDM_SELECT,           19,

  IDM_CONNECT,          0,
  IDM_DISCONNECT,       1,
  IDM_CONNECTIONS,      0,
  IDM_SHAREAS,          2,
  IDM_STOPSHARE,        3,

  IDM_VNAME,            4,
  IDM_VDETAILS,         5,
  IDM_VOTHER,           20,
  IDM_VINCLUDE,         21,     /* this is out of order, but looks better */
  IDM_BYNAME,           6,
  IDM_BYTYPE,           7,
  IDM_BYSIZE,           8,
  IDM_BYDATE,           9,

  IDM_FONT,             22,

  IDM_NEWWINDOW,        10,
  IDM_CASCADE,          23,
  IDM_TILEHORIZONTALLY, 24,
  IDM_TILE,             26,

  IDM_PERMISSIONS,      27,

  IDM_HELPINDEX,        25,

  IDM_COMPRESS,         28,
  IDM_UNCOMPRESS,       29,

} ;

/* Actually, EXTRA_BITMAPS is an upper bound on the number of bitmaps, since
 * some bitmaps may be repeated.
 */
#define TBAR_ALL_BUTTONS        (sizeof(sAllButtons)/sizeof(sAllButtons[0]))
#define TBAR_EXTRA_BITMAPS      16

static int iSel = -1;

Static VOID
ExtensionName(int i, LPTSTR szName)
{
  TCHAR szFullName[256];
  LPTSTR lpName;

  *szName = TEXT('\0');

  if ((UINT)i<(UINT)iNumExtensions
   && GetModuleFileName(extensions[i].hModule, szFullName,
   COUNTOF(szFullName)) && (lpName=StrRChr (szFullName, NULL, TEXT('\\'))))
  StrNCpy(szName, lpName+1, 15);
}


VOID
EnableStopShareButton(void)
{
   return;

//  SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_STOPSHARE,
// WNetGetShareCount(WNTYPE_DRIVE));
}


Static VOID
EnableDisconnectButton(void)
{
   INT i;

   for (i=0; i<cDrives; i++)
      if (!IsCDRomDrive(rgiDrive[i]) && IsRemoteDrive(rgiDrive[i]))
         break;

   SendMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_DISCONNECT, i<cDrives);

   EnableMenuItem(GetMenu(hwndFrame), IDM_DISCONNECT, i<cDrives ?
      MF_BYCOMMAND | MF_ENABLED :
      MF_BYCOMMAND | MF_GRAYED );

}


VOID
CheckTBButton(DWORD idCommand)
{
  UINT i, begin, end;

  /* Make sure to "pop-up" any other buttons in the group.
   */
  if ((UINT)(idCommand-IDM_VNAME) <= IDM_VOTHER-IDM_VNAME)
    {
      begin = IDM_VNAME;
      end = IDM_VOTHER;
    }
  else if ((UINT)(idCommand-IDM_BYNAME) <= IDM_BYFDATE-IDM_BYNAME)
    {
      begin = IDM_BYNAME;
      end = IDM_BYDATE;
      // NOTE: no toolbar button for IDM_BYFDATE yet
    }
  else
    {
      SendMessage(hwndToolbar, TB_CHECKBUTTON, idCommand, 1L);
      return;
    }

  for (i=begin; i<=end; ++i)
      SendMessage(hwndToolbar, TB_CHECKBUTTON, i, i==idCommand);
}


//    Enable/disable and check/uncheck toolbar buttons based on the
//    state of the active child window.

VOID
EnableCheckTBButtons(HWND hwndActive)
{
   DWORD dwSort;
   BOOL fEnable;
   INT  iButton;

   // If the active window is the search window, clear the selection
   // in the drive list.

   if (hwndActive == hwndSearch) {

      SwitchDriveSelection(hwndSearch, TRUE);
      UpdateStatus(hwndSearch);
   }

   // Check or uncheck the sort-by and view-details buttons based
   // on the settings for the active window.

   switch (GetWindowLongPtr(hwndActive, GWL_VIEW) & VIEW_EVERYTHING) {
   case VIEW_NAMEONLY:
      CheckTBButton(IDM_VNAME);
      break;

   case VIEW_EVERYTHING:
      CheckTBButton(IDM_VDETAILS);
      break;

   default:
      CheckTBButton(IDM_VOTHER);
      break;
   }

   // Now do the sort-by buttons.  While we're at it, disable them all
   // if the active window is a search window or lacks a directory pane,
   // else enable them all.

   dwSort = GetWindowLongPtr(hwndActive, GWL_SORT) - IDD_NAME + IDM_BYNAME;

   fEnable = ((int)GetWindowLongPtr(hwndActive, GWL_TYPE) >= 0 &&
      HasDirWindow(hwndActive));

   CheckTBButton(dwSort);
   for (iButton=IDM_BYNAME; iButton<=IDM_BYDATE; ++iButton) {
      SendMessage(hwndToolbar, TB_ENABLEBUTTON, iButton, fEnable);
   }

   UpdateWindow(hwndToolbar);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     BuildDriveLine
//
// Synopsis: Builds a drive line for display A:{\} <label>
//
// OUT    ppszTemp        --    pointer to string to get drive line
// IN     driveInd        --    Drive index of driveline to build
// IN     fGetFloppyLabel BOOL  determines whether floppy disk is hit
// IN     dwFlags         --    Drive line flags passed to GetVolShare
//            ALTNAME_MULTI    : \n and \t added
//            ALTNAME_SHORT    : single line
//            ALTNAME_REG      : regular
//
// Return:   VOID
//
//
// Assumes:
//
// Effects:
//
//
// Notes:   Non re-entrant!  This is due to static szDriveSlash!
//          (NOT multithread safe)
//
/////////////////////////////////////////////////////////////////////


VOID
BuildDriveLine(LPTSTR* ppszTemp, DRIVEIND driveInd,
   BOOL fGetFloppyLabel, DWORD dwType)
{
   static TCHAR szDrive[64];
   DRIVE drive;
   LPTSTR p;
   DWORD dwError;

   drive = rgiDrive[driveInd];

   //
   // If !fGetFloppyLabel, but our VolInfo is valid,
   // we might as well just pick it up.
   //

   if (fGetFloppyLabel || (!IsRemovableDrive(drive) && !IsCDRomDrive(drive)) ||
      (aDriveInfo[drive].sVolInfo.bValid && !aDriveInfo[drive].sVolInfo.bRefresh)) {

      if (dwError = GetVolShare(rgiDrive[driveInd], ppszTemp, dwType)) {

         if (DE_REGNAME == dwError) {

            goto UseRegName;
         }

         goto Failed;

      } else {

         //
         // If regular name, do copy
         //
         if (ALTNAME_REG == dwType) {

UseRegName:

            p = *ppszTemp;

            *ppszTemp = szDrive;
            StrNCpy(szDrive+3, p, COUNTOF(szDrive)-4);

         } else {

            //
            // Assume header is valid!
            //
            (*ppszTemp) -=3;
         }
      }

   } else {

Failed:

      *ppszTemp = szDrive;

      //
      // Delimit
      //
      (*ppszTemp)[3]=CHAR_NULL;
   }

   DRIVESET(*ppszTemp,rgiDrive[driveInd]);

   (*ppszTemp)[1] = CHAR_COLON;
   (*ppszTemp)[2] = CHAR_SPACE;
}


VOID
RefreshToolbarDrive(DRIVEIND iDriveInd)
{
   INT iSel;
   DRIVE drive;

   iSel = (INT)SendMessage(hwndDriveList, CB_GETCURSEL, 0, 0L);

   SendMessage(hwndDriveList, CB_DELETESTRING, iDriveInd, 0L);

   drive = rgiDrive[iDriveInd];
   //
   // For floppy drives, when we refresh, we should pickup the
   // drive label.
   //

   if (IsRemovableDrive(drive) || IsCDRomDrive(drive))
      U_VolInfo(drive);

   //
   // We must tell ourselves which drive we are
   // working on.
   //
   SendMessage(hwndDriveList, CB_INSERTSTRING,iDriveInd, (LPARAM)drive);

   if (iSel!=-1) {

      SendMessage(hwndDriveList, CB_SETCURSEL, iSel, 0L);
   }
}


VOID
SelectToolbarDrive(DRIVEIND DriveInd)
{

   //
   // Turn off\on redrawing.
   //

   SendMessage(hwndDriveList, WM_SETREDRAW, (WPARAM)FALSE, 0L);
   RefreshToolbarDrive(DriveInd);

   SendMessage(hwndDriveList, WM_SETREDRAW, (WPARAM)TRUE, 0L);
   SendMessage(hwndDriveList, CB_SETCURSEL, DriveInd, 0L);

   //
   // Move focus of drivebar
   //
   SetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEIND, DriveInd);
}



// value iDrive = drive to highlight added.

VOID
FillToolbarDrives(DRIVE drive)
{
   INT i;

   if (hwndDriveList == NULL)
      return;

   //
   // Disable redraw
   //
   SendMessage(hwndDriveList,WM_SETREDRAW,(WPARAM)FALSE,0l);

   SendMessage(hwndDriveList, CB_RESETCONTENT, 0, 0L);

   for (i=0; i<cDrives; i++) {
      SendMessage(hwndDriveList, CB_INSERTSTRING, i, (LPARAM)szNULL);

      // change from i==0 to i==drive to eliminate garbage in cb
      if (rgiDrive[i]==drive) {

         SendMessage(hwndDriveList, CB_SETCURSEL, i, 0L);
      }
   }

   // Enable redraw; moved down
   SendMessage(hwndDriveList,WM_SETREDRAW,(WPARAM)TRUE,0l);

// Caller must do separately.
//  EnableDisconnectButton();
}


Static VOID
PaintDriveLine(DRAWITEMSTRUCT FAR *lpdis)
{
   HDC hdc = lpdis->hDC;
   LPTSTR lpszText;
   TCHAR* pchTab;
   RECT rc = lpdis->rcItem;
   DRIVE drive;
   INT dxTabstop=MINIDRIVE_WIDTH;
   HBRUSH hbrFill;
   HFONT hfontOld;
   DWORD clrBackground;
   RECT rc2;

   //
   // Check rectangle: if > 1 line and dwLines > 1 then use multiline
   // else use abbreviated
   //

   //
   // If no itemID, quit
   //
   if ((UINT)-1 == lpdis->itemID || lpdis->itemID >= (UINT)cDrives)
      return;

   drive = rgiDrive[lpdis->itemID];

   //
   // Total Hack: if rc.left == 0, assume in dropdown, not edit!
   //

   //
   // TRUE or FALSE?  When do we fetch the drive label?
   //
   if (!rc.left) {
      BuildDriveLine(&lpszText, lpdis->itemID, FALSE, ALTNAME_MULTI);
   } else {
      BuildDriveLine(&lpszText, lpdis->itemID, FALSE, ALTNAME_SHORT);

      for (pchTab = lpszText; *pchTab && *pchTab != CHAR_TAB; pchTab++)
         ;

      if (*pchTab)
         *(pchTab++) = (TCHAR) 0;
   }

   if (lpdis->itemAction != ODA_FOCUS) {
      clrBackground = GetSysColor((lpdis->itemState & ODS_SELECTED) ?
         COLOR_HIGHLIGHT : COLOR_WINDOW);

      hbrFill = CreateSolidBrush(clrBackground);

      FillRect(hdc, &rc, hbrFill);

      DeleteObject(hbrFill);

      //
      // No error checking necessary since BuildDriveLine
      // will at worst return the blank-o A:
      //
#if 0
      if (lpszText == NULL || lpszText == (LPTSTR)-1 ||
         lpdis->itemID == (UINT) -1)
         return;
#endif

//    OffsetRect(&rc, -rc.left, -rc.top);

      hfontOld = SelectObject(hdc, hfontDriveList);

      SetBkColor(hdc, clrBackground);
      SetTextColor(hdc, GetSysColor((lpdis->itemState & ODS_SELECTED) ?
         COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));


      rc2.left = rc.left + MINIDRIVE_WIDTH + 2*MINIDRIVE_MARGIN;
      rc2.top = rc.top + DRIVELIST_BORDER / 2;
      rc2.right = rc.right;
      rc2.bottom = rc.bottom;

      DrawText(hdc, lpszText, -1, &rc2, DT_LEFT|DT_EXPANDTABS|DT_NOPREFIX);


      SelectObject(hdc, hfontOld);

      BitBlt(hdc, rc.left + MINIDRIVE_MARGIN,
         rc.top + (dyDriveItem+DRIVELIST_BORDER - MINIDRIVE_HEIGHT) / 2,
         MINIDRIVE_WIDTH, MINIDRIVE_HEIGHT,
         hdcMem, aDriveInfo[drive].iOffset, 2 * dyFolder + dyDriveBitmap,
         SRCCOPY);
   }

   if (lpdis->itemAction == ODA_FOCUS ||
      (lpdis->itemState & ODS_FOCUS))

      DrawFocusRect(hdc, &rc);
}


Static VOID
ResetToolbar(void)
{
   INT nItem;
   INT i, idCommand;
   HMENU hMenu;
   UINT state;

   HWND hwndActive;

   // Remove from back to front as a speed optimization

   for (nItem=(INT)SendMessage(hwndToolbar, TB_BUTTONCOUNT, 0, 0L)-1;
      nItem>=0; --nItem)

      SendMessage(hwndToolbar, TB_DELETEBUTTON, nItem, 0L);

   // Add the default list of buttons

   SendMessage(hwndToolbar, TB_ADDBUTTONS, TBAR_BUTTON_COUNT,
      (LPARAM)(LPTBBUTTON)tbButtons);

   // Add the extensions back in

   if (hwndExtensions) {
      INT nExtButtons;
      TBBUTTON tbButton;

      nExtButtons = (INT)SendMessage(hwndExtensions, TB_BUTTONCOUNT, 0, 0L);
      for (nItem=0; nItem<nExtButtons; ++nItem) {
         SendMessage(hwndExtensions, TB_GETBUTTON, nItem,
            (LPARAM)(LPTBBUTTON)&tbButton);
         SendMessage(hwndToolbar, TB_ADDBUTTONS, 1,
            (LPARAM)(LPTBBUTTON)&tbButton);
      }
   }

   // Set the states correctly

   hMenu = GetMenu(hwndFrame);

   hwndActive = (HWND) SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   if (hwndActive && InitPopupMenus(0xffff, hMenu, hwndActive)) {
      for (i=0; i<TBAR_BUTTON_COUNT; ++i) {
         if (tbButtons[i].fsStyle == TBSTYLE_SEP)
            continue;

         idCommand = tbButtons[i].idCommand;
         state = GetMenuState(hMenu, idCommand, MF_BYCOMMAND);

         SendMessage(hwndToolbar, TB_CHECKBUTTON, idCommand, state&MF_CHECKED);
         SendMessage(hwndToolbar, TB_ENABLEBUTTON, idCommand,
            !(state&(MF_DISABLED|MF_GRAYED)));
      }

      for (i=0; i<TBAR_ALL_BUTTONS; ++i) {
         idCommand = sAllButtons[i].idM;
         state = GetMenuState(hMenu, idCommand, MF_BYCOMMAND);
         SendMessage(hwndToolbar, TB_CHECKBUTTON, idCommand, state&MF_CHECKED);
         SendMessage(hwndToolbar, TB_ENABLEBUTTON, idCommand,
            !(state&(MF_DISABLED|MF_GRAYED)));
      }
   } else {
      EnableStopShareButton();
   }
}


Static VOID
LoadDesc(UINT uID, LPTSTR lpDesc)
{
   HMENU hMenu;
   UINT uMenu;
   TCHAR szFormat[20];
   TCHAR szMenu[20];
   TCHAR szItem[MAXDESCLEN-COUNTOF(szMenu)];
   LPTSTR lpIn;

   HWND hwndActive;

   hMenu = GetMenu(hwndFrame);

   uMenu = uID/100 - 1;
   if (uMenu > IDM_EXTENSIONS)
      uMenu -= MAX_EXTENSIONS - iNumExtensions;

   // Add 1 if MDI is maximized.

   hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   if (hwndActive && GetWindowLongPtr(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
      uMenu++;

   GetMenuString(hMenu, uMenu, szMenu, COUNTOF(szMenu), MF_BYPOSITION);
   if (GetMenuString(hMenu, uID, szItem, COUNTOF(szItem), MF_BYCOMMAND) <= 0) {

      int i;

      for (i=0; ; ++i) {
         if (i >= NUMEXTRACOMMANDS)
            return;
         if (uExtraCommands[i] == uID)
            break;
      }

      LoadString(hAppInstance, MS_EXTRA+i, szItem, COUNTOF(szItem));
   }

   LoadString(hAppInstance, IDS_MENUANDITEM, szFormat, COUNTOF(szFormat));
   wsprintf(lpDesc, szFormat, szMenu, szItem);

   // Remove the ampersands

   for (lpIn=lpDesc; ; ++lpIn, ++lpDesc) {
      TCHAR cTemp;

      cTemp = *lpIn;
      if (cTemp == TEXT('&'))
         cTemp = *(++lpIn);
      if (cTemp == TEXT('\t'))
         cTemp = TEXT('\0');

      *lpDesc = cTemp;
      if (cTemp == TEXT('\0'))
         break;
   }
}


Static BOOL
GetAdjustInfo(LPTBNOTIFY lpTBNotify)
{
   LPTBBUTTON lpButton = &lpTBNotify->tbButton;
   FMS_HELPSTRING tbl;
   int iExt;
   int j = lpTBNotify->iItem;


   if ((UINT)j < TBAR_ALL_BUTTONS) {

      *lpButton = tbButtons[TBAR_BUTTON_COUNT-1];
      lpButton->iBitmap = sAllButtons[j].idB & ~HIDEIT;
      lpButton->fsState = (sAllButtons[j].idB & HIDEIT)
         ? TBSTATE_HIDDEN : TBSTATE_ENABLED;
      lpButton->idCommand = sAllButtons[j].idM;

LoadDescription:
      lpTBNotify->pszText[0] = TEXT('\0');
      if (!(lpButton->fsStyle & TBSTYLE_SEP))
         LoadDesc(lpButton->idCommand, lpTBNotify->pszText);

UnlockAndReturn:
      return(TRUE);
   }

   j -= TBAR_ALL_BUTTONS;
   if (hwndExtensions && SendMessage(hwndExtensions, TB_GETBUTTON, j,
      (LPARAM)lpButton)) {

      if (lpButton->fsStyle & TBSTYLE_SEP)
         goto LoadDescription;

      iExt = lpButton->idCommand/100 - IDM_EXTENSIONS - 1;
      if ((UINT)iExt < (UINT)iNumExtensions) {
         tbl.idCommand = lpButton->idCommand % 100;
         tbl.hMenu = extensions[iExt].hMenu;
         tbl.szHelp[0] = TEXT('\0');

         extensions[iExt].ExtProc(hwndFrame, FMEVENT_HELPSTRING,
             (LPARAM)(LPFMS_HELPSTRING)&tbl);

         if (extensions[iExt].bUnicode == FALSE) {
            CHAR   szAnsi[MAXDESCLEN];

            memcpy (szAnsi, tbl.szHelp, COUNTOF(szAnsi));
            MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szAnsi, COUNTOF(szAnsi),
                                 tbl.szHelp, COUNTOF(tbl.szHelp));
         }

         StrNCpy(lpTBNotify->pszText, tbl.szHelp, MAXDESCLEN - 1);

         goto UnlockAndReturn;
      }
   }

   return FALSE;
}


DWORD
DriveListMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, UINT* puiRetVal)
{
   UINT uID;
   FMS_HELPSTRING tbl;
   INT iExt;
   DRIVE drive;
   HMENU hMenu;

   switch (uMsg) {
   case WM_MENUSELECT:

#define uItem   GET_WM_MENUSELECT_CMD(wParam, lParam)
#define fuFlags GET_WM_MENUSELECT_FLAGS(wParam, lParam)
#define hMenu2  GET_WM_MENUSELECT_HMENU(wParam, lParam)


      //
      // If menugoes away, reset sb.
      //

      if ( (WORD)-1 == (WORD)fuFlags && NULL == hMenu2 ) {
         SendMessage(hwndStatus, SB_SIMPLE, 0, 0L);
         return 0;
      }

      if (fuFlags&MF_POPUP) {

         hMenu = GetSubMenu(hMenu2, uItem);

         for (iExt=iNumExtensions-1; iExt>=0; --iExt) {
            if (hMenu == extensions[iExt].hMenu) {

               tbl.idCommand = -1;
ExtensionHelp:
               tbl.hMenu = extensions[iExt].hMenu;
               tbl.szHelp[0] = TEXT('\0');

               extensions[iExt].ExtProc(hwndFrame, FMEVENT_HELPSTRING,
                   (LPARAM)(LPFMS_HELPSTRING)&tbl);

               if (extensions[iExt].bUnicode == FALSE)
               {
                  CHAR   szAnsi[MAXDESCLEN];

                  memcpy (szAnsi, tbl.szHelp, COUNTOF(szAnsi));
                  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szAnsi, COUNTOF(szAnsi),
                                       tbl.szHelp, COUNTOF(tbl.szHelp));
               }

               SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
                  (LPARAM)tbl.szHelp);

               SendMessage(hwndStatus, SB_SIMPLE, 1, 0L);

               UpdateWindow(hwndStatus);

               *puiRetVal = TRUE;
               return(0);
            }
         }

NormalHelp:
         MenuHelp((WORD)uMsg, wParam, lParam, GetMenu(hwndFrame),
            hAppInstance, hwndStatus, (LPUINT)dwMenuIDs);
      } else {
         uID = uItem;

         iExt = uID/100 - IDM_EXTENSIONS -1;

         if ((UINT)iExt < MAX_EXTENSIONS) {
            tbl.idCommand = uID % 100;
            goto ExtensionHelp;
         } else {
            goto NormalHelp;
         }
      }
      break;

#undef uItem
#undef fuFlags
#undef hMenu

   case WM_DRAWITEM:
      PaintDriveLine((DRAWITEMSTRUCT FAR *)lParam);
      break;

   case WM_MEASUREITEM:

#define pMeasureItem ((MEASUREITEMSTRUCT FAR *)lParam)

      //
      // If just checking edit control (-1) or it's not a network drive,
      // then just return one line default.
      //
      if (-1 == pMeasureItem->itemID ||
         !IsRemoteDrive(rgiDrive[pMeasureItem->itemID])) {

         pMeasureItem->itemHeight = dyDriveItem + DRIVELIST_BORDER;
         pMeasureItem->itemWidth = dxDriveList;
         break;
      }

      drive = rgiDrive[pMeasureItem->itemID];

      //
      // Don't update net con here.  If it's ready it's fine; if
      // it's not, then it will be updated later.
      //
      // U_NetCon(drive);
      //

      pMeasureItem->itemHeight = dyDriveItem *
         aDriveInfo[drive].dwLines[ALTNAME_MULTI] + DRIVELIST_BORDER;

#undef pMeasureItem

      break;

   case WM_NOTIFY:
       {
       LPNMHDR lpnmhdr;
       LPTBNOTIFY lpTBNotify;

       lpnmhdr = (LPNMHDR) lParam;
       lpTBNotify = (LPTBNOTIFY) lParam;

       switch (wParam)
          {
          case IDC_TOOLBAR:
              switch (lpnmhdr->code)
                 {
                 case TBN_CUSTHELP:
                    {
                       DWORD dwSave;

                       dwSave = dwContext;
                       dwContext = IDH_CTBAR;
                       WFHelp(lpnmhdr->hwndFrom);
                       dwContext = dwSave;
                       break;
                    }

                 case TBN_BEGINDRAG:
                    uID = lpTBNotify->iItem;
                    break;

                 case TBN_QUERYDELETE:
                 case TBN_QUERYINSERT:
                    // Do not allow addition of buttons before the first
                    // separator (this is the drives list)
                    if (lpTBNotify->iItem == 0)
                        *puiRetVal = FALSE;
                    else
                        *puiRetVal = TRUE;
                    return(lpTBNotify->iItem);

                 case TBN_GETBUTTONINFO:

                    *puiRetVal = GetAdjustInfo(lpTBNotify);
                    return(*puiRetVal);

                 case TBN_RESET:
                    ResetToolbar();
                    // fall through
                 case TBN_TOOLBARCHANGE:
                    SaveRestoreToolbar(TRUE);
                    break;

                 case TBN_BEGINADJUST:

                    dwSaveHelpContext = dwContext;
                    dwContext = IDH_CTBAR;
                    break;

                 case TBN_ENDADJUST:

                    dwContext = dwSaveHelpContext;
                    break;

                 case TBN_ENDDRAG:
                 default:
                    break;
                 }

                 *puiRetVal = TRUE;
                 return (0);
          }


       if (lpnmhdr->code == TTN_NEEDTEXT) {
           int iExt;
           UINT idString;
           LPTOOLTIPTEXT lpTT = (LPTOOLTIPTEXT) lParam;
           FMS_HELPSTRING tbl;


           iExt = lpTT->hdr.idFrom/100 - IDM_EXTENSIONS - 1;

           if (hwndExtensions && ((UINT)iExt < (UINT)iNumExtensions)) {
               tbl.idCommand = lpTT->hdr.idFrom % 100;
               tbl.hMenu = extensions[iExt].hMenu;
               tbl.szHelp[0] = TEXT('\0');

               extensions[iExt].ExtProc(hwndFrame, FMEVENT_HELPSTRING,
                   (LPARAM)(LPFMS_HELPSTRING)&tbl);

               if (extensions[iExt].bUnicode == FALSE) {
                  CHAR   szAnsi[MAXDESCLEN];

                  memcpy (szAnsi, tbl.szHelp, COUNTOF(szAnsi));
                  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szAnsi, COUNTOF(szAnsi),
                                       tbl.szHelp, COUNTOF(tbl.szHelp));
               }

               StrNCpy(lpTT->szText, tbl.szHelp, MAXDESCLEN - 1);

           } else {
               idString = lpTT->hdr.idFrom + MH_MYITEMS;

               if (lpTT->hdr.idFrom == IDM_CONNECTIONS) {
                   idString = IDM_CONNECT + MH_MYITEMS;
               }

               if (!LoadString (hAppInstance, idString, lpTT->szText, 80)) {
                   lpTT->szText[0] = TEXT('\0');
               }
           }
           *puiRetVal = TRUE;
           return (0);
       }

       *puiRetVal = FALSE;
       return (0);

       }
       break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam,lParam)) {

      case IDM_OPEN:
      case IDM_ESCAPE:
         if (GetFocus() != hwndDriveList) {
            *puiRetVal=FALSE;
            return(0);
         }

         if (GET_WM_COMMAND_ID(wParam,lParam) == IDM_ESCAPE)
            SendMessage(hwndDriveList, CB_SETCURSEL, iSel, 0L);
         SendMessage(hwndDriveList, CB_SHOWDROPDOWN, FALSE, 0L);
         break;

      case IDC_DRIVES:
         switch (GET_WM_COMMAND_CMD(wParam,lParam)) {
         case CBN_SETFOCUS:
            iSel = (int)SendMessage(hwndDriveList, CB_GETCURSEL, 0, 0L);
            break;

         case CBN_CLOSEUP:
            {
               HWND hwndActive;
               INT iNewSel;

               hwndActive = (HWND)SendMessage(hwndMDIClient,
                  WM_MDIGETACTIVE, 0, 0L);

               SetFocus(hwndActive);
               if (GetFocus() == hwndDriveList)
                  SetFocus(hwndMDIClient);

               hwndActive = (HWND)SendMessage(hwndMDIClient,
                  WM_MDIGETACTIVE, 0, 0L);
               if (hwndActive == hwndSearch) {
                  SendMessage(hwndDriveList, CB_SETCURSEL, iSel, 0L);
                  break;
               }

               iNewSel = (int)SendMessage(hwndDriveList, CB_GETCURSEL, 0, 0L);
               if (iNewSel != iSel) {
                  if (!CheckDrive(hwndFrame, rgiDrive[iNewSel], FUNC_SETDRIVE)) {
                     SendMessage(hwndDriveList, CB_SETCURSEL, iSel,0L);
                     break;
                  }
                  SendMessage(hwndDriveBar, FS_SETDRIVE, iNewSel, 0L);

               } else {
;//                  SendMessage(hwndDriveBar, FS_SETDRIVE, iNewSel, 1L);
               }

               break;
            }

         default:
            break;
         }
         break;

      default:
         *puiRetVal = FALSE;
         return(0);
      }
      break;

   default:
      *puiRetVal = FALSE;
      return(0);
   }

   *puiRetVal = TRUE;
   return(0);
}


VOID
CreateFMToolbar(void)
{
   RECT rc;
   INT xStart;
   HDC hDC;
   HFONT hOldFont;
   TEXTMETRIC TextMetric;
   TBADDBITMAP tbAddBitmap;

   hDC = GetDC(NULL);

   xStart = dxButtonSep;

   hOldFont = SelectObject(hDC, hfontDriveList);

   //
   // Set cchDriveListMax
   // (-2 since we need space for "X: ")
   //

   //
   // !! LATER !!
   // Error checking for GetTextMetrics return value
   //
   GetTextMetrics(hDC, &TextMetric);

   cchDriveListMax =
       (INT)( (dxDriveList - MINIDRIVE_WIDTH - 2 * MINIDRIVE_MARGIN) /
              (TextMetric.tmAveCharWidth * 3 / 2) - 2 );

   dyDriveItem = TextMetric.tmHeight;

   if (hOldFont)
      SelectObject(hDC, hOldFont);

   ReleaseDC(NULL, hDC);

   // There should be another ButtonSep here, except the toolbar code
   // automatically puts one in before the first item.

   tbButtons[0].iBitmap = xStart + dxDriveList;

   // We'll start out by adding no buttons; that will be done in
   // InitToolbarButtons

   hwndToolbar = CreateToolbarEx(hwndFrame,
      WS_CHILD|WS_BORDER|CCS_ADJUSTABLE|WS_CLIPSIBLINGS|TBSTYLE_TOOLTIPS|
      (bToolbar ? WS_VISIBLE : 0),
      IDC_TOOLBAR, TBAR_BITMAP_COUNT, hAppInstance, IDB_TOOLBAR,
      tbButtons, 0, 0,0,0,0, sizeof(TBBUTTON));

   if (!hwndToolbar)
      return;

   if (bDisableVisualStyles)
     SetWindowTheme(hwndToolbar, pwszInvalidTheme, pwszInvalidTheme);

   SendMessage (hwndToolbar, TB_SETINDENT, 8, 0);

   tbAddBitmap.hInst = hAppInstance;
   tbAddBitmap.nID   = IDB_EXTRATOOLS;

   SendMessage(hwndToolbar, TB_ADDBITMAP, TBAR_EXTRA_BITMAPS,
               (LPARAM) &tbAddBitmap);

   GetClientRect(hwndToolbar, &rc);
   dyToolbar = rc.bottom;

   hwndDriveList = CreateWindow(TEXT("combobox"), NULL,
      WS_BORDER | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWVARIABLE | WS_VSCROLL,
      xStart, 0, dxDriveList, dxDriveList,
      hwndToolbar, (HMENU)IDC_DRIVES, hAppInstance, NULL);

   if (!hwndDriveList) {
      DestroyWindow(hwndToolbar);
      hwndToolbar = NULL;
      return;
   }

   if (bDisableVisualStyles)
     SetWindowTheme(hwndDriveList, pwszInvalidTheme, pwszInvalidTheme);

   SendMessage(hwndDriveList, CB_SETEXTENDEDUI, 0, 0L);
   SendMessage(hwndDriveList, WM_SETFONT, (WPARAM)hfontDriveList, MAKELPARAM(TRUE, 0));

   GetWindowRect(hwndDriveList, &rc);
   rc.bottom -= rc.top;

   MoveWindow(hwndDriveList, xStart, (dyToolbar - rc.bottom)/2,
      dxDriveList, dxDriveList, TRUE);

   ShowWindow(hwndDriveList, SW_SHOW);

   //
   // Done right after UpdateDriveList in wfinit.c
   //
//   FillToolbarDrives(0);
}


DWORD
ShareCountStub(DWORD iType)
{
  /* This is to reference the variable */
  iType;
  return 1;
}


VOID
InitToolbarButtons(VOID)
{
   INT i;
   HMENU hMenu;
   BOOL bLastSep;

   hMenu = GetMenu(hwndFrame);

   //
   // HACK: Don't show both Connections and Connect/Disconnect in the
   // Customize toolbar dialog.
   //
   if (GetMenuState(hMenu, IDM_CONNECTIONS, MF_BYCOMMAND) == (UINT)-1)
      tbButtons[ICONNECTIONS].idCommand = IDM_CONNECT;

   for (i=1, bLastSep=TRUE; i<TBAR_BUTTON_COUNT; ++i) {
      if (tbButtons[i].fsStyle & TBSTYLE_SEP) {
         if (bLastSep)
            tbButtons[i].fsState = TBSTATE_HIDDEN;
         bLastSep = TRUE;

      } else {

         if (GetMenuState(hMenu, tbButtons[i].idCommand, MF_BYCOMMAND)
            == (UINT)-1)

            tbButtons[i].fsState = TBSTATE_HIDDEN;
         else
            bLastSep = FALSE;
      }
   }

   for (i=0; i<TBAR_ALL_BUTTONS; ++i) {

      //
      // Set the top bit to indicate that the button should be hidden
      //
      if (GetMenuState(hMenu, sAllButtons[i].idM, MF_BYCOMMAND) == (UINT)-1)
         sAllButtons[i].idB |= HIDEIT;
   }

   SaveRestoreToolbar(FALSE);

   //
   // Now that we have API entrypoints, set the initial states of the
   // disconnect and stop-sharing buttons appropriately.
   //

   EnableDisconnectButton();
   EnableStopShareButton();
}


BOOL
InitToolbarExtension(INT iExt)
{
   TBBUTTON extButton;
   FMS_TOOLBARLOAD tbl;
   LPEXT_BUTTON lpButton;
   INT i, iStart, iBitmap;
   BOOL fSepLast;
   TBADDBITMAP tbAddBitmap;

   tbl.dwSize = sizeof(tbl);
   tbl.lpButtons = NULL;
   tbl.cButtons = 0;
   tbl.idBitmap = 0;
   tbl.hBitmap = NULL;


   if (!extensions[iExt].ExtProc(hwndFrame, FMEVENT_TOOLBARLOAD,
       (LPARAM)(LPFMS_TOOLBARLOAD)&tbl))

      return FALSE;

   if (tbl.dwSize != sizeof(tbl)) {

      if (!(0x10 == tbl.dwSize && tbl.idBitmap))

         return FALSE;
   }

   if (!tbl.cButtons || !tbl.lpButtons || (!tbl.idBitmap && !tbl.hBitmap))
      return FALSE;

   // We add all extension buttons to a "dummy" toolbar

   if (hwndExtensions) {
      // If the last "button" is not a separator, then add one.  If it is, and
      // there are no extensions yet, then "include" it in the extensions.

      i = (INT)SendMessage(hwndToolbar, TB_BUTTONCOUNT, 0, 0L);
      SendMessage(hwndToolbar, TB_GETBUTTON, i-1,
         (LPARAM)(LPTBBUTTON)&extButton);
      if (!(extButton.fsStyle & TBSTYLE_SEP))
         goto AddSep;
   } else {
      hwndExtensions = CreateToolbarEx(hwndFrame, WS_CHILD,
         IDC_EXTENSIONS, 0, hAppInstance, IDB_TOOLBAR, tbButtons, 0,
         0,0,0,0,sizeof(TBBUTTON));

      if (!hwndExtensions)
         return FALSE;

AddSep:
      extButton.iBitmap = 0;
      extButton.idCommand = 0;
      extButton.fsState = 0;
      extButton.fsStyle = TBSTYLE_SEP;

      SendMessage(hwndExtensions, TB_INSERTBUTTON, (WORD)-1,
         (LPARAM)(LPTBBUTTON)&extButton);
   }

   // Notice we add the bitmaps to hwndToolbar, not hwndExtensions, because
   // it is hwndToolbar that may actually paint the buttons.

   if (tbl.idBitmap) {
      tbAddBitmap.hInst = extensions[iExt].hModule;
      tbAddBitmap.nID   = (UINT_PTR)tbl.idBitmap;
      iStart = (INT)SendMessage(hwndToolbar, TB_ADDBITMAP, tbl.cButtons,
                               (LPARAM) &tbAddBitmap);
   } else {
      tbAddBitmap.hInst = 0;
      tbAddBitmap.nID   = (UINT_PTR)tbl.hBitmap;
      iStart = (INT)SendMessage(hwndToolbar, TB_ADDBITMAP, tbl.cButtons,
                               (LPARAM) &tbAddBitmap);
   }

   // Add all of his buttons.

   for (fSepLast=TRUE, i=tbl.cButtons, iBitmap=0, lpButton=tbl.lpButtons;
      i>0; --i, ++lpButton) {

      if (lpButton->fsStyle & TBSTYLE_SEP) {
         if (fSepLast)
            continue;

         extButton.iBitmap = 0;
         fSepLast = TRUE;
      } else {
         extButton.iBitmap = iBitmap + iStart;
         ++iBitmap;
         fSepLast = FALSE;
      }

      extButton.fsStyle   = (BYTE)lpButton->fsStyle;
      extButton.idCommand = lpButton->idCommand + extensions[iExt].Delta;
      extButton.fsState   = TBSTATE_ENABLED;
      extButton.dwData    = 0;
      extButton.iString   = 0;

      SendMessage(hwndExtensions, TB_INSERTBUTTON, (WORD)-1,
         (LPARAM)(LPTBBUTTON)&extButton);
   }

   return TRUE;
}


VOID
FreeToolbarExtensions(VOID)
{
   if (hwndExtensions)
      DestroyWindow(hwndExtensions);
   hwndExtensions = NULL;
}


VOID
SaveRestoreToolbar(BOOL bSave)
{
   static TCHAR  szSubKey[] = TEXT("Software\\Microsoft\\File Manager\\Settings");
   static TCHAR  szValueName [] = TEXT("ToolbarWindow");

   TCHAR szNames[20*MAX_EXTENSIONS];
   TBSAVEPARAMS tbSave;

   if (bSave) {
      INT i;
      LPTSTR pName;

      // Write out a comma separated list of the current extensions

      for (i=0, pName=szNames; i<iNumExtensions; ++i) {
         ExtensionName(i, pName);
         pName += lstrlen(pName);
         *pName++ = TEXT(',');
      }

      *pName = TEXT('\0');
      WritePrivateProfileString(szSettings, szAddons, szNames, szTheINIFile);

      // Remove the beginning space

      SendMessage(hwndToolbar, TB_DELETEBUTTON, 0, 0L);

      // Save the state

      tbSave.hkr = HKEY_CURRENT_USER;
      tbSave.pszSubKey = szSubKey;
      tbSave.pszValueName = szValueName;
      SendMessage(hwndToolbar, TB_SAVERESTORE, 1, (LPARAM)&tbSave);

      // Add the beginning space back in.

      SendMessage(hwndToolbar, TB_INSERTBUTTON, 0,
         (LPARAM)(LPTBBUTTON)tbButtons);
   } else {
      INT i, iExt, nExtButtons;
      BOOL bRestored;
      TBBUTTON tbButton;
      LPTSTR pName, pEnd;

      // Only load the buttons for the extensions that were the same as
      // the last time the state was saved.

      GetPrivateProfileString(szSettings, szAddons, szNULL, szNames,
         COUNTOF(szNames), szTheINIFile);

      for (iExt=0, pName=szNames; iExt<iNumExtensions; ++iExt, pName=pEnd) {
         TCHAR szName[20];

         pEnd = StrChr(pName, TEXT(','));
         if (!pEnd)
            break;
         *pEnd++ = TEXT('\0');

         ExtensionName(iExt, szName);
         if (lstrcmpi(szName, pName))
            break;

         InitToolbarExtension(iExt);
      }

      // Save the number of buttons currently loaded

      if (hwndExtensions)
         nExtButtons = (int)SendMessage(hwndExtensions, TB_BUTTONCOUNT, 0, 0L);
      else
         nExtButtons = 0;

      // Now actually set the toolbar buttons and load the rest of the
      // extension buttons.

      tbSave.hkr = HKEY_CURRENT_USER;
      tbSave.pszSubKey = szSubKey;
      tbSave.pszValueName = szValueName;
      bRestored = (BOOL)SendMessage(hwndToolbar, TB_SAVERESTORE, 0, (LPARAM)&tbSave);


      for ( ; iExt<iNumExtensions; ++iExt)
         InitToolbarExtension(iExt);

      if (bRestored) {
         INT idGood, idBad, nItem;
         HMENU hMenu;

         // Change CONNECTIONS to CONNECT (or vice versa) if necessary

         idGood = tbButtons[ICONNECTIONS].idCommand;
         idBad = IDM_CONNECT+IDM_CONNECTIONS-idGood;
         nItem = (int)SendMessage(hwndToolbar, TB_COMMANDTOINDEX, idBad, 0L);

         hMenu = GetMenu(hwndFrame);
         if(GetMenuState(hMenu, idGood, MF_BYCOMMAND)!=(UINT)-1 && nItem>=0) {
            SendMessage(hwndToolbar, TB_DELETEBUTTON, nItem, 0L);
            SendMessage(hwndToolbar, TB_INSERTBUTTON, nItem,
               (LPARAM)(LPTBBUTTON)(&tbButtons[ICONNECTIONS]));
         }

         // Add in the beginning separator and the "new" extensions

         SendMessage(hwndToolbar, TB_INSERTBUTTON, 0,
            (LPARAM)(LPTBBUTTON)tbButtons);

         // Add in any extensions that are new

         if (hwndExtensions) {
            i = nExtButtons;
            nExtButtons = (int)SendMessage(hwndExtensions, TB_BUTTONCOUNT,
            0, 0L) - nExtButtons;
            for ( ; nExtButtons>0; ++i, --nExtButtons) {
               SendMessage(hwndExtensions, TB_GETBUTTON, i,
                  (LPARAM)(LPTBBUTTON)&tbButton);
               SendMessage(hwndToolbar, TB_ADDBUTTONS, 1,
                  (LPARAM)(LPTBBUTTON)&tbButton);
            }
         }

      } else
         ResetToolbar();

   }
}
