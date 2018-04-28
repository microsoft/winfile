/********************************************************************

   wfdir.c

   Implements view for directories and search windows

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"
#include "numfmt.h"
#include <commctrl.h>

#include "wfdrop.h"

WCHAR   szAttr[]        = L"RHSACE";

typedef struct _SELINFO {
   LPWSTR pSel;
   BOOL bSelOnly;
   INT iTop;
   INT iLastSel;
   WCHAR szCaret[MAXFILENAMELEN];
   WCHAR szAnchor[MAXFILENAMELEN];
   WCHAR szTopIndex[MAXFILENAMELEN];
} SELINFO;


VOID RightTabbedTextOut(HDC hdc, INT x, INT y, LPWSTR pLine, WORD *pTabStops, INT x_offset, DWORD dwAlternateFileNameExtent);
LRESULT ChangeDisplay(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT CompareDTA(register LPXDTA lpItem1, register LPXDTA lpItem2, DWORD dwSort);
BOOL SetDirFocus(HWND hwndDir);
VOID DirGetAnchorFocus(register HWND hwndLB, LPXDTALINK lpStart, PSELINFO pSelInfo);
BOOL SetSelection(HWND hwndLB, LPXDTALINK lpStart, LPWSTR pszSel);
INT DirFindIndex(HWND hwndLB, LPXDTALINK lpStart, LPTSTR lpszFile);
VOID SortDirList(HWND hwndDir, LPXDTALINK lpStart, DWORD count, LPXDTA* lplpxdta);
VOID GetDirStatus(HWND hwnd, LPWSTR szMessage1, LPWSTR szMessage2);
VOID FreeSelInfo(PSELINFO pSelInfo);
BOOL SetSelInfo(HWND hwndLB, LPXDTALINK lpStart, PSELINFO pSelInfo);


VOID
DrawItem(
   HWND hwnd,
   DWORD dwViewOpts,
   LPDRAWITEMSTRUCT lpLBItem,
   BOOL bHasFocus)
{
   INT x, y, i;
   BOOL bDrawSelected;
   HWND hwndLB;
   RECT rc;
   DWORD rgbText, rgbBackground;
   WCHAR szBuf[MAXFILENAMELEN * 2];

   LPWSTR pszLine = szBuf;
   INT iError;

#define dyHeight dyFileName

   LPXDTA lpxdta  = (LPXDTA)lpLBItem->itemData;
   LPXDTALINK lpStart = (LPXDTALINK)GetWindowLongPtr(hwnd, GWL_HDTA);

   HDC hDC = lpLBItem->hDC;

   HWND hwndListParms = (HWND)GetWindowLongPtr(hwnd, GWL_LISTPARMS);
   BOOL bLower;

   //
   // Print out any errors
   //
   iError = (INT)GetWindowLongPtr(hwnd, GWL_IERROR);

   if (iError) {

      if (LoadString(hAppInstance,
                     iError,
                     szBuf,
                     COUNTOF(szBuf))) {

         WCHAR szError[MAXSUGGESTLEN];

         wsprintf(szError,
                  szBuf,
                  (WCHAR)SendMessage(hwnd, FS_GETDRIVE, 0, 0L));

         TextOut(hDC,
                 lpLBItem->rcItem.left,
                 lpLBItem->rcItem.top,
                 szError,
                 lstrlen(szError));
      }

      return;
   }

   hwndLB = lpLBItem->hwndItem;
   bDrawSelected = (lpLBItem->itemState & ODS_SELECTED);

   if (bHasFocus && bDrawSelected)
   {
      rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
      rgbBackground = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
   }
   else
   {
      //
      //  Set Text color of Compressed items to BLUE and Encrypted items
      //  to GREEN.
      //
      //  LATER:
      //  Should allow for User selection in the future.
      //
      if ((lpxdta) && (lpxdta->dwAttrs & ATTR_COMPRESSED))
      {
         rgbText = SetTextColor(hDC, RGB(0, 0, 255));
      }
      else if ((lpxdta) && (lpxdta->dwAttrs & ATTR_ENCRYPTED))
      {
         rgbText = SetTextColor(hDC, RGB(0, 192, 0));
      }
      else
      {
         rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
      }
      rgbBackground = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
   }

   if (lpLBItem->itemID == -1 || !lpStart || !lpxdta) {

      if (bHasFocus)
         goto FocusOnly;

      return;
   }

   if (lpLBItem->itemAction == ODA_FOCUS) {
      goto FocusOnly;
   }

   //
   // Draw the black/white background.
   //
   ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lpLBItem->rcItem, NULL, 0, NULL);

   x = lpLBItem->rcItem.left + 1;
   y = lpLBItem->rcItem.top + (dyHeight/2);


   bLower = ((wTextAttribs & TA_LOWERCASE) &&
            (lpxdta->dwAttrs & ATTR_LOWERCASE) ||
            wTextAttribs & TA_LOWERCASEALL);

   if (lpxdta->byBitmap == BM_IND_DIRUP) {

      szBuf[0] = CHAR_NULL;

   } else {

      if (bLower) {

         lstrcpy(szBuf, MemGetFileName(lpxdta));
         CharLower(szBuf);

      } else {

         //
         // We should copy lpxdta->cFileName into szBuf,
         // but will just shortcut by renaming pszLine.
         //
         pszLine = MemGetFileName(lpxdta);
      }
   }

   if (fShowSourceBitmaps || (hwndDragging != hwndLB) || !bDrawSelected) {

         HICON hIcon = DocGetIcon(lpxdta->pDocB);

         if (hIcon != NULL)
         {
             DrawIconEx(hDC,
                    x + dyBorder,
                    y-(dyFolder/2),
                    hIcon,
                    dxFolder,
                    dyFolder,
                    0,
                    NULL,
                    DI_NORMAL);
         }
         else
         {
             i = lpxdta->byBitmap;

             BitBlt(hDC,
                    x + dyBorder,
                    y-(dyFolder/2),
                    dxFolder,
                    dyFolder,
                    hdcMem,
                    i * dxFolder,
                    (bHasFocus && bDrawSelected) ? dyFolder : 0,
                    SRCCOPY);
         }
   }

   if (dwViewOpts & VIEW_EVERYTHING) {

      //
      // We want to display the entire line
      //

      // SetBkMode(hDC, TRANSPARENT);

      CreateLBLine(dwViewOpts, lpxdta, szBuf);

      if (bLower)
         CharLower(szBuf);

      x += dxFolder + dyBorderx2 + dyBorder;

      RightTabbedTextOut(hDC,
                         x,
                         y-(dyText/2),
                         szBuf,
                         (WORD *)GetWindowLongPtr(hwnd, GWL_TABARRAY),
                         x,
                         dwViewOpts & VIEW_DOSNAMES ?
                            MemLinkToHead(lpStart)->dwAlternateFileNameExtent :
                            0);

      // SetBkMode(hDC, OPAQUE);

   } else {

      ExtTextOut(hDC,
                 x + dxFolder + dyBorderx2 + dyBorder,
                 y-(dyText/2),
                 0,
                 NULL,
                 pszLine,
                 lstrlen(pszLine),
                 NULL);
   }


   if (lpLBItem->itemState & ODS_FOCUS) {

FocusOnly:

      //
      // toggles focus (XOR)
      //
      DrawFocusRect(hDC, &lpLBItem->rcItem);
   }

   //
   // Restore the normal drawing colors.
   //
   if (bDrawSelected) {

      if (!bHasFocus) {

         HBRUSH hbr;

         if (hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT))) {
            rc = lpLBItem->rcItem;
            rc.right = max(rc.right,
                           rc.left +
                           SendMessage(hwndLB, LB_GETHORIZONTALEXTENT, 0, 0)) -
                       dyBorder;
            rc.left += dyBorder;

            if (lpLBItem->itemID > 0 &&
               SendMessage(hwndLB, LB_GETSEL, lpLBItem->itemID - 1, 0L))

               rc.top -= dyBorder;

            FrameRect(hDC, &rc, hbr);
            DeleteObject(hbr);
         }
      }
   }

   SetTextColor(hDC, rgbText);
   SetBkColor(hDC, rgbBackground);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     CreateLBLine
//
// Synopsis: Creates a string with all details in a file
//
// Return:
//
// Assumes:
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
CreateLBLine(register DWORD dwLineFormat, LPXDTA lpxdta, LPWSTR szBuffer)
{
   register LPWSTR pch;
   DWORD dwAttr;

   pch = szBuffer;

   dwAttr = lpxdta->dwAttrs;

   if (BM_IND_DIRUP == lpxdta->byBitmap) {

      szBuffer[0] = CHAR_NULL;
      return;
   }

   //
   // Copy the file name.
   //
   lstrcpy(pch, MemGetFileName(lpxdta));

   pch += lstrlen(pch);

   if (dwLineFormat & VIEW_DOSNAMES) {

      *pch++ = CHAR_TAB;

      //
      // Copy the file name
      //
      lstrcpy(pch, MemGetAlternateFileName(lpxdta));

      //
      // Upper or Lowercase it as necessary
      //
      if (wTextAttribs & TA_LOWERCASE || wTextAttribs & TA_LOWERCASEALL)
         CharLower(pch);
      else
         CharUpper(pch);

      pch += lstrlen(pch);
   }

   //
   // Should we show the size?
   //
   if (dwLineFormat & VIEW_SIZE) {
      *pch++ = CHAR_TAB;
      if (dwAttr & ATTR_DIR)
      {
          if (dwAttr & ATTR_JUNCTION)
              lstrcpy(pch, TEXT("<JUNCTION>"));
          else if (dwAttr & ATTR_SYMBOLIC)
              lstrcpy(pch, TEXT("<SYMLINKD>"));
          else
              lstrcpy(pch, TEXT("<DIR>"));
        pch += lstrlen(pch);
      }
      else 
      {
         pch += PutSize(&lpxdta->qFileSize, pch);
      }
   }

   //
   // Should we show the date?
   //
   if (dwLineFormat & VIEW_DATE) {
      *pch++ = CHAR_TAB;
      pch += PutDate(&lpxdta->ftLastWriteTime, pch);
   }

   //
   // Should we show the time?
   //
   if (dwLineFormat & VIEW_TIME) {
      *pch++ = CHAR_TAB;
      pch += PutTime(&lpxdta->ftLastWriteTime, pch);
   }

   //
   // Should we show the attributes?
   //
   if (dwLineFormat & VIEW_FLAGS) {
      *pch++ = CHAR_TAB;
      pch += PutAttributes(dwAttr, pch);
   }

   *pch = CHAR_NULL;
}


LRESULT DirListBoxWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
    case WM_RBUTTONDOWN:
        MessageBeep(1);
        break;
    }
    return 0;
}

LRESULT
DirWndProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   INT iSel, i;
   HWND hwndLB;
   LPXDTA lpxdta;
   WCHAR szTemp[MAXPATHLEN * 2];
   HWND hwndParent = GetParent(hwnd);

   static HWND   hwndOwnerDraw = NULL;

#ifdef PROGMANHSCROLL
   //
   // If  the window is different and uMsg == WM_DRAWITEM   _OR_
   //     the window is the same and uMsg != WM_DRAWITEM
   // then we are drawing on a new window
   //
   if (hwndOwnerDraw != hwnd && uMsg == WM_DRAWITEM) {
      hwndOwnerDraw = NULL;

   }

   if (uMsg == WM_DRAWITEM && hwndOwnerDraw != hwnd) {

      IconHScroll(hwndParent, ((LPDRAWITEMSTRUCT)lParam)->hwndItem);
      hwndOwnerDraw = hwnd;
   }
#endif

   hwndLB = GetDlgItem(hwnd, IDCW_LISTBOX);

   switch (uMsg) {

   case FS_DIRREADDONE:
   {
      LPXDTALINK lpStart;
      PSELINFO pSelInfo;

      //
      // wParam => iError
      // lParam => lpxdta
      //
      SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);

      lpStart = DirReadDone(hwnd, (LPXDTALINK)lParam, (INT)wParam);

      if (lpStart) {

         //
         // Now set the selections
         //
         pSelInfo = (PSELINFO)GetWindowLongPtr(hwnd, GWL_SELINFO);

         SetSelInfo(hwndLB, lpStart, pSelInfo);

         FreeSelInfo(pSelInfo);
         SetWindowLongPtr(hwnd, GWL_SELINFO, 0L);
      }

      SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
      InvalidateRect(hwndLB, NULL, TRUE);

      SetDirFocus(hwnd);
      UpdateStatus(hwndParent);

      return (LRESULT)lpStart;
   }

   case FS_GETDIRECTORY:

      GetMDIWindowText(hwndParent, (LPWSTR)lParam, (INT)wParam);

      //
      // get the string
      //
      StripFilespec((LPWSTR)lParam); // Remove the trailing extension

      AddBackslash((LPWSTR)lParam);  // terminate with a backslash
      break;

   case FS_GETDRIVE:

      //
      // Returns the letter of the corresponding directory
      //
      return (LRESULT) CHAR_A + GetWindowLongPtr(hwndParent, GWL_TYPE);

   case FS_GETFILESPEC:

      //
      // returns the current filespec (from View.Include...).  this is
      // an uppercase ANSI string
      //
      GetMDIWindowText(hwndParent, (LPWSTR)lParam, (INT)wParam);
      StripPath((LPWSTR)lParam);

      break;

   case FS_SETSELECTION:
      //
      // wParam is the select(TRUE)/deselect(FALSE) param
      // lParam is the filespec to match against
      //
      SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
      DSSetSelection(hwndLB, wParam != 0, (LPWSTR)lParam, FALSE);

      SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
      InvalidateRect(hwndLB, NULL, TRUE);

      break;

   case FS_GETSELECTION:

      //
      // return = pszDir
      //
      return (LRESULT)DirGetSelection(hwnd,
                                   hwnd,
                                   hwndLB,
                                   (INT)wParam,
                                   (BOOL*)lParam,
                                   NULL);
   case FS_TESTEMPTY:
      {
         HWND hwndNext;

         SendMessage(hwndLB, LB_GETTEXT, 0, (LPARAM) &lpxdta);
         if (!lpxdta && GetFocus() == hwndLB)
         {
            hwndNext = (HWND)lParam;
            if (hwndNext && IsWindow(hwndNext))
            {
               SetFocus(hwndNext);
            }
            else
            {
                hwndNext = HasTreeWindow(hwndParent);
                if (!hwndNext && bDriveBar)
                {
                    hwndNext = hwndDriveBar;
                }
                SetFocus(hwndNext);
            }
         }

         break;
      }

   case WM_CREATE:
   case WM_FSC:
   case FS_CHANGEDISPLAY:

      return ChangeDisplay(hwnd, uMsg, wParam, lParam);

   case WM_DESTROY:
   {
      HANDLE hMem;
      HWND hwndTree;

      //
      // Remove from Change Notify Loop
      //
      ModifyWatchList(hwndParent, NULL, 0);

      if (hwndLB == GetFocus())
         if (hwndTree = HasTreeWindow(hwndParent))
            SetFocus(hwndTree);

      if (hMem = (HANDLE)GetWindowLongPtr(hwnd, GWL_TABARRAY))
         LocalFree(hMem);

      FreeSelInfo((PSELINFO)GetWindowLongPtr(hwnd, GWL_SELINFO));

      {
      IDropTarget *pDropTarget;
      
      pDropTarget = (IDropTarget *)GetWindowLongPtr(hwnd, GWL_OLEDROP);
      UnregisterDropWindow(hwnd, pDropTarget);
      }

      break;
   }
   case WM_CHARTOITEM:
   {
      UINT  i, j;
      WCHAR ch;
      UINT  cItems;
      LPWSTR szItem;
      WCHAR rgchMatch[MAXPATHLEN];
      SIZE_T cchMatch;

      if ((ch = LOWORD(wParam)) <= CHAR_SPACE || !GetWindowLongPtr(hwnd, GWL_HDTA))
         return(-1L);

      i = GET_WM_CHARTOITEM_POS(wParam, lParam);
      cItems = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

        // if more that one character to match, start at current position; else next position
      if (TypeAheadString(ch, rgchMatch))
          j = 0;
      else
          j = 1;

      for (; j < cItems; j++) {

         if (SendMessage(hwndLB, LB_GETTEXT, (i + j) % cItems, (LPARAM)&lpxdta)
            == LB_ERR) {

            return -2L;
         }

          szItem = (LPWSTR)MemGetFileName(lpxdta);
         if (szItem[0] == '\0')
                szItem = L"..";
         cchMatch = wcslen(rgchMatch);
         if (cchMatch > wcslen(szItem))
                cchMatch = wcslen(szItem);

         if (CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE, 
             rgchMatch, cchMatch, szItem, cchMatch) == 2)
            break;

      }

      if (j == cItems)
         return -2L;

      return((i + j) % cItems);
   }
   case WM_COMPAREITEM:

#define lpci ((LPCOMPAREITEMSTRUCT)lParam)

      return (LONG)CompareDTA((LPXDTA)lpci->itemData1,
                              (LPXDTA)lpci->itemData2,
                              GetWindowLongPtr(hwndParent, GWL_SORT));
#undef lpci

   case WM_NCDESTROY:

      DirReadDestroyWindow(hwnd);
      break;

   case WM_DRAGLOOP:

      // WM_DRAGLOOP is sent to the source window as the object is moved.
      //
      //    wParam: TRUE if the object is currently over a droppable sink
      //    lParam: LPDROPSTRUCT

      // based on current drop location scroll the sink up or down
      DSDragScrollSink((LPDROPSTRUCT)lParam);

      // DRAGLOOP is used to turn the source bitmaps on/off as we drag.
      DSDragLoop(hwndLB, wParam, (LPDROPSTRUCT)lParam);
      break;

   case WM_DRAGSELECT:

      // WM_DRAGSELECT is sent to a sink whenever an new object is dragged
      // inside of it.
      //
      //    wParam: TRUE if the sink is being entered, FALSE if it's being
      //            exited.
      //    lParam: LPDROPSTRUCT
      //

      // DRAGSELECT is used to turn our selection rectangle on or off.
      // Turn on/off status bar

      SendMessage(hwndStatus,
                  SB_SETTEXT,
                  SBT_NOBORDERS|255,
                  (LPARAM)szNULL);

      SendMessage(hwndStatus,
                  SB_SIMPLE,
                  (wParam ? 1 : 0),
                  0L);

      UpdateWindow(hwndStatus);

#define lpds ((LPDROPSTRUCT)lParam)

      iSelHighlight = lpds->dwControlData;
      DSRectItem(hwndLB, iSelHighlight, (BOOL)wParam, FALSE);
      break;

#undef lpds

   case WM_DRAGMOVE:

      {
         static BOOL fOldShowSourceBitmaps = 0;

         // WM_DRAGMOVE is sent to a sink as the object is being dragged
         // within it.
         //
         //    wParam: Unused
         //   lParam: LPDROPSTRUCT

         // DRAGMOVE is used to move our selection rectangle among sub-items.

#define lpds ((LPDROPSTRUCT)lParam)

         // Get the subitem we are over.
         iSel = lpds->dwControlData;

         // Is it a new one?

         if (iSel == iSelHighlight && fOldShowSourceBitmaps == fShowSourceBitmaps)
            break;

         fOldShowSourceBitmaps = fShowSourceBitmaps;

         // Yup, un-select the old item.
         DSRectItem(hwndLB, iSelHighlight, FALSE, FALSE);

         // Select the new one.
         iSelHighlight = iSel;
         DSRectItem(hwndLB, iSel, TRUE, FALSE);
         break;

#undef lpds
      }

   case WM_DRAWITEM:

      DrawItem(hwnd,
               GetWindowLongPtr(hwndParent, GWL_VIEW),
               (LPDRAWITEMSTRUCT)lParam,
               ((LPDRAWITEMSTRUCT)lParam)->hwndItem == GetFocus());
      break;

   case WM_DROPOBJECT:

      //
      // WM_DROPOBJECT is sent to a sink when the user releases an
      // acceptable object over it
      //
      //    wParam: TRUE if over the non-client area, FALSE if over the
      //            client area.
      //    lParam: LPDROPSTRUCT
      //
      if (DSDropObject(hwnd, hwndLB, (LPDROPSTRUCT)lParam, FALSE))
         return TRUE;

   case WM_LBTRACKPOINT:
      return DSTrackPoint(hwnd, hwndLB, wParam, lParam, FALSE);

   case WM_MEASUREITEM:

#define pLBMItem ((LPMEASUREITEMSTRUCT)lParam)

      pLBMItem->itemHeight =
            dyFileName;    // the same as in SetLBFont()
#undef pLBMItem
      break;

   case WM_QUERYDROPOBJECT:

      // lParam LPDROPSTRUCT
      //
      // return values:
      //    0       don't accept (use ghost buster)
      //    1       accept, use cursor from DragObject()
      //    hCursor accept, change to this cursor
      //

      //
      // Ensure that we are dropping on the client area of the listbox.
      //
#define lpds ((LPDROPSTRUCT)lParam)

      //
      // Ensure that we can accept the format.
      //
      switch (lpds->wFmt) {
      case DOF_EXECUTABLE:
      case DOF_DIRECTORY:
      case DOF_DOCUMENT:
      case DOF_MULTIPLE:
         if (lpds->hwndSink == hwnd)
            lpds->dwControlData = (DWORD)-1L;

          return TRUE;
      }
      return FALSE;
#undef lpds

   case WM_SETFOCUS:
      {
         UpdateStatus(hwndParent);

         SetWindowLongPtr(hwnd, GWL_NEXTHWND, 0L);
      }

      //
      // Fall through
      //
   case WM_LBUTTONDOWN:
      if (hwndLB != GetFocus())
         SetFocus(hwndLB);

      break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
      case LBN_DBLCLK:
         /* Double-click... Open the blasted thing. */
         SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_OPEN, 0, 0));
         break;

      case LBN_SELCHANGE:

         ExtSelItemsInvalidate();

         for (i = 0; i < iNumExtensions; i++) {
            (extensions[i].ExtProc)(hwndFrame, FMEVENT_SELCHANGE, 0L);
         }
         UpdateStatus(hwndParent);
         break;

      case LBN_SETFOCUS:

         //
         // Make sure there are files in this window.  If not, set
         // the focus to the tree or drives window.  Note:  This
         // message was caused by a mouse click and not an
         // accelerator, because these were handled in the window
         // routine that was losing the focus.
         //
         if (SetDirFocus(hwnd)) {
            SetWindowLongPtr(hwndParent, GWL_LASTFOCUS, (LPARAM)GET_WM_COMMAND_HWND(wParam, lParam));
            UpdateSelection(GET_WM_COMMAND_HWND(wParam, lParam));
         }
         UpdateStatus(hwndParent);
         break;

      case LBN_KILLFOCUS:
         SetWindowLongPtr(hwndParent, GWL_LASTFOCUS, 0L);
         UpdateSelection(GET_WM_COMMAND_HWND(wParam, lParam));
         SetWindowLongPtr(hwndParent,
                       GWL_LASTFOCUS,
                       (LPARAM)GET_WM_COMMAND_HWND(wParam, lParam));
         break;
      }
      break;

   case WM_CONTEXTMENU:
	   ActivateCommonContextMenu(hwnd, hwndLB, lParam);
	   break;

   case WM_VKEYTOITEM:
      switch (GET_WM_VKEYTOITEM_CODE(wParam, lParam)) {
      case VK_ESCAPE:
         bCancelTree = TRUE;
         TypeAheadString('\0', NULL);
         return -2L;

	  case 'A':			/* Ctrl-A */
		  if (GetKeyState(VK_CONTROL) >= 0)
			  break;
	  case 0xBF:        /* Ctrl-/ */
         TypeAheadString('\0', NULL);
         SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_SELALL, 0, 0));
         return -2;

      case 0xDC:        /* Ctrl-\ */
         TypeAheadString('\0', NULL);
         SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_DESELALL, 0, 0));
         return -2;

      case VK_F6:       // like excel
      case VK_TAB:
         {
            HWND hwndTree, hwndDrives;

            hwndTree = HasTreeWindow(hwndParent);

            hwndDrives = bDriveBar ? hwndDriveBar : (hwndTree ? hwndTree : hwnd);

            if (GetKeyState(VK_SHIFT) < 0)
               SetFocus(hwndTree ? hwndTree : hwndDrives);
            else
               SetFocus(hwndDrives);

            TypeAheadString('\0', NULL);
            break;
         }

      case VK_BACK:
         SendMessage(hwnd, FS_GETDIRECTORY, COUNTOF(szTemp), (LPARAM)szTemp);

         // are we already at the root?
         if (lstrlen(szTemp) <= 3)
            return -1;

         StripBackslash(szTemp);
         StripFilespec(szTemp);

         CreateDirWindow(szTemp, GetKeyState(VK_SHIFT) >= 0, hwndParent);
         TypeAheadString('\0', NULL);
         return -2;

      default:
         {
#if 0
          // check for Ctrl-[DRIVE LETTER] and pass on to drives
          // window

          if ((GetKeyState(VK_CONTROL) < 0) && hwndDriveBar) {
               return SendMessage(hwndDriveBar, uMsg, wParam, lParam);
          }
#endif
            break;
         }
      }
      return -1;

   case WM_SIZE:
      if (!IsIconic(hwndParent)) {
         INT iMax;

         MoveWindow(hwndLB, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);


         iMax = (INT)SendMessage(hwndLB, LB_GETCARETINDEX, 0, 0L);
         if (iMax >= 0) // scroll item into view
            /* SETCARETINDEX will scroll item into view */
            SendMessage(hwndLB, LB_SETCARETINDEX, iMax, 0L);
            //MakeItemVisible(iMax, hwndLB);

      }
      break;

   default:
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }

   return 0L;
}


// Handles WM_CREATE, WM_FSC and FS_CHANGEDISPLAY for DirectoryWndProc
// for WM_CREATE, wParam and lParam are ignored
// for WM_FSC, wParam is FSC_* and lParam depends on the function (but the cases handled here are limited)
// for FS_CHANGEDISPLAY, wParam is one of CD_* values; 
//	for CD_SORT, LOWORD(lParam) == sort value
//  for CD_VIEW, LOWORD(lParam) == view bits and HIWORD(lParam) == TRUE means always refresh
//  for CD_PATH and CD_PATH_FORCE, lParam is the new path; if NULL, use MDI window text

LRESULT
ChangeDisplay(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   //
   // Enough space for path and filter
   //
   WCHAR         szPath[2 * MAXFILENAMELEN];

   HCURSOR  hCursor;
   HWND     hwndLB, hwndT;
   DWORD ws;
   RECT  rc;
   WORD *pwTabs;
   BOOL bDontSteal = FALSE;
   HWND hwndListParms;
   LPXDTALINK lpStart;
   BOOL bDirFocus;
   BOOL bResetFocus= FALSE;

   PSELINFO pSelInfo;
   PSELINFO pSelInfoOld;
   LONG lRetval = 0;
   BOOL bCreateDTABlock = TRUE;

   pSelInfo = (PSELINFO)LocalAlloc(LMEM_FIXED, sizeof(SELINFO));

   if (!pSelInfo)
      return -1;

   pSelInfo->pSel = NULL;
   pSelInfo->bSelOnly = FALSE;

   hwndListParms = (HWND)GetWindowLongPtr(hwnd, GWL_LISTPARMS);
   hwndLB = GetDlgItem(hwnd, IDCW_LISTBOX);

   switch (uMsg) {

   case WM_FSC:

      //
      // If FSC is disabled and we are NOT refreshing, then defer.
      // The FSC_REFRESH was added because this is only sent by
      // EnableFSC and WM_DROPOBJECT.
      //
      if (cDisableFSC && wParam != FSC_REFRESH) {

         //
         // I need to be updated
         //
         SetWindowLongPtr(hwndListParms, GWL_FSCFLAG, TRUE);
         break;
      }
      wParam = CD_PATH;
      lParam = 0L;

      //
      // No need to clear out pending change notifications
      // here since they will be reset below when the new
      // listbox is filled.
      //

      /*** FALL THROUGH ***/

   case FS_CHANGEDISPLAY:

      if (GetWindowLongPtr(hwndListParms, GWL_FSCFLAG)) {
         if (wParam == CD_PATH) {
            wParam = CD_PATH_FORCE;
            bDontSteal = TRUE;
         }
      }

      SetWindowLongPtr(hwndListParms, GWL_FSCFLAG, FALSE);

      hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
      ShowCursor(TRUE);

      bResetFocus = (GetFocus() == hwndLB);

      lpStart = (LPXDTALINK)GetWindowLongPtr(hwnd, GWL_HDTA);

      //
      // parse wParam immediately since CD_DONTSTEAL added
      //
      bDontSteal = ((wParam & CD_DONTSTEAL) != 0);
      wParam &= ~CD_DONTSTEAL;

      switch (wParam) {
      case CD_SORT:

         //
         // change the sort order of the listbox
         //
         // we want to save the current selection and things here
         // and restore them once the listbox has been rebuilt
         //
         // But first, save a list of the selected items
         //
         // change 0 to 8 to get unqualified directory names
         //
         pSelInfo->pSel = DirGetSelection(hwnd,
                                          hwnd,
                                          hwndLB,
                                          8,
                                          NULL,
                                          &pSelInfo->iLastSel);

         pSelInfo->iTop = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);

         DirGetAnchorFocus(hwndLB, lpStart, pSelInfo);

         SetWindowLongPtr(hwndListParms, GWL_SORT, LOWORD(lParam));
         SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

         SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
         FillDirList(hwnd, lpStart);

         goto ResetSelection;

      case CD_VIEW:
      {
         DWORD dwCurView;

         //
         // change the view type (name only, vs full details)
         // Warning! Convoluted Code!  We want to destroy the
         // listbox only if we are going between Name Only view
         // and Details view.
         //
         dwNewView = LOWORD(lParam);
         dwCurView = GetWindowLongPtr(hwndListParms, GWL_VIEW);

         //
         // hiword lParam == TRUE means always refresh
         //
         if (dwNewView == dwCurView && !HIWORD(lParam))
            break;

         //
         // special case the long and partial view change
         // this doesn't require us to recreate the listbox
         //
         if ((VIEW_EVERYTHING & dwNewView) && (VIEW_EVERYTHING & dwCurView)) {

            SetWindowLongPtr(hwndListParms, GWL_VIEW, dwNewView);

            FixTabsAndThings(hwndLB,
                             (WORD *)GetWindowLongPtr(hwnd, GWL_TABARRAY),
                             GetMaxExtent(hwndLB, lpStart, FALSE),
                             GetMaxExtent(hwndLB, lpStart, TRUE),
                             dwNewView);

            InvalidateRect(hwndLB, NULL, TRUE);

            break;
         }

         //
         // Things are a changing radically.  Destroy the listbox.
         // But first, save a list of the selected items
         // Again 0 changed to 8
         //

         pSelInfo->pSel = DirGetSelection(hwnd,
                                          hwnd,
                                          hwndLB,
                                          8,
                                          NULL,
                                          &pSelInfo->iLastSel);

         pSelInfo->iTop = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);
         DirGetAnchorFocus(hwndLB, lpStart, pSelInfo);

         lstrcpy(pSelInfo->szTopIndex, pSelInfo->szCaret);

         bDirFocus = (HWND)GetWindowLongPtr(hwndListParms,
                                         GWL_LASTFOCUS) == hwndLB;

         DestroyWindow(hwndLB);

         if (bDirFocus)
            SetWindowLongPtr(hwndListParms, GWL_LASTFOCUS, 0L);

         //
         // Create a new one (preserving the Sort setting).
         //
         dwNewSort = GetWindowLongPtr(hwndListParms, GWL_SORT);
         dwNewAttribs = GetWindowLongPtr(hwndListParms, GWL_ATTRIBS);
         SetWindowLongPtr(hwndListParms, GWL_VIEW, dwNewView);

         bCreateDTABlock = FALSE;	// and szPath is NOT set

         goto CreateLB;
      }

      case CD_PATH | CD_ALLOWABORT:
      case CD_PATH:
      case CD_PATH_FORCE:

         //
         // bad stuff happens if we change the path
         // while we are reading the tree.  bounce this
         // in that case.  this causes the steal data
         // code in the tree to crash because we would
         // free the hDTA while it is being traversed
         // (very bad thing)
         //
         hwndT = HasTreeWindow(hwndListParms);
         if (hwndT && GetWindowLongPtr(hwndT, GWL_READLEVEL)) {
            SetWindowLongPtr(hwndListParms, GWL_FSCFLAG, TRUE);
            break;
         }

         //
         // change the path of the current directory window (basically
         // recreate the whole thing)
         //
         // if lParam == NULL this is a refresh, otherwise
         // check for short circuit case to avoid rereading
         // the directory
         //
         GetMDIWindowText(hwndListParms, szPath, COUNTOF(szPath));

         if (lParam) {

            //
            // get out early if this is a NOP
            //
            // lpStart added
            //

#ifdef DBCS
//fix kksuzuka: #2852
//DBCS dirname should not be compared ..
       if( (wParam != CD_PATH_FORCE) && lpStart ) {
      INT aLen = lstrlen(szPath);
      INT bLen = lstrlen((LPTSTR)lParam);

      if ( (WideCharToMultiByte(CP_ACP,0L,szPath,aLen,NULL,0L,NULL, NULL) == aLen) &&
           (WideCharToMultiByte(CP_ACP,0L,(LPTSTR)lParam,bLen,NULL,0L, NULL,NULL) == bLen ) ) {
         if( !lstrcmpi(szPath, (LPTSTR)lParam) )
             break;
          }
          else {
         if( !lstrcmp(szPath, (LPTSTR)lParam) )
             break;
          }
       }
#else
       if ((wParam != CD_PATH_FORCE) && lpStart &&
          !lstrcmpi(szPath, (LPTSTR)lParam)) {

          break;
       }
#endif

            lstrcpy(szPath, (LPTSTR)lParam);

            pSelInfo->iLastSel = -1;          // invalidate the last selection

            //
            // We are changing selections,
            // Free GWL_SELINFO.
            //
            FreeSelInfo((PSELINFO)GetWindowLongPtr(hwnd, GWL_SELINFO));

            SetWindowLongPtr(hwnd, GWL_SELINFO, 0L);

         } else {

            //
            // if this is a refresh save the current selection,
            // anchor stuff, etc.
            //

            //
            // If GWL_SELINFO is already set, then we are in the act
            // of refreshing already.  So don't try and read an empty
            // listbox, just use GWL_SELINFO
            //
            pSelInfoOld = (PSELINFO)GetWindowLongPtr(hwnd, GWL_SELINFO);

            if (!pSelInfoOld) {

               pSelInfo->pSel = DirGetSelection(hwnd,
                                                hwnd,
                                                hwndLB,
                                                8,
                                                NULL,
                                                &pSelInfo->iLastSel);

               pSelInfo->iTop = (INT)SendMessage(hwndLB,
                                                 LB_GETTOPINDEX,
                                                 0,
                                                 0L);

               DirGetAnchorFocus(hwndLB, lpStart, pSelInfo);

               pSelInfo->bSelOnly = FALSE;

            } else {

               //
               // We have old info we want to save.
               // Clear the current GWL_SELINFO
               // and replace the "new" pSelInfo with the
               // old one.
               //
               SetWindowLongPtr(hwnd, GWL_SELINFO, 0L);

               LocalFree(pSelInfo);
               pSelInfo = pSelInfoOld;
            }
         }

         //
         // Create a new one (preserving the Sort setting)
         //
         dwNewSort = GetWindowLongPtr(hwndListParms, GWL_SORT);
         dwNewView = GetWindowLongPtr(hwndListParms, GWL_VIEW);
         dwNewAttribs = GetWindowLongPtr(hwndListParms, GWL_ATTRIBS);

         SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
         SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

		 // bCreateDTABlock is TRUE and szPath is set

         goto CreateNewPath;
      }

      SetCursor(hCursor);
      ShowCursor(FALSE);
      break;

   case WM_CREATE:

      //
      // dwNewView, dwNewSort and dwNewAttribs define the viewing
      // parameters of the new window (GLOBALS)
      // the window text of the parent window defines the
      // filespec and the directory to open up
      //
      hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
      ShowCursor(TRUE);

      wParam = 0;           // don't allow abort in CreateDTABlock()
      lParam = 1L;          // allow DTA steal optimization

      bDontSteal = FALSE;
      hwndListParms = GetParent(hwnd);

      if ((pwTabs = (WORD *)LocalAlloc(LPTR,
                                       sizeof(WORD) * MAX_TAB_COLUMNS)) == NULL) {

         lRetval = -1L;
         goto Done;
      }

      SetWindowLongPtr(hwnd, GWL_LISTPARMS, (LPARAM)hwndListParms);
      SetWindowLongPtr(hwnd, GWL_TABARRAY, (LPARAM)pwTabs);
      SetWindowLongPtr(hwnd, GWL_SELINFO, 0L);

      {
      WF_IDropTarget *pDropTarget;
      
      RegisterDropWindow(hwnd, &pDropTarget);
      SetWindowLongPtr(hwnd, GWL_OLEDROP, (LPARAM)pDropTarget);
      }
      
      //
      // get the dir to open from our parent window text
      //
      GetMDIWindowText(hwndListParms, szPath, COUNTOF(szPath));

	  // bCreateDTABlock == TRUE and szPath just set

CreateLB:

      if ((dwNewView & VIEW_EVERYTHING) == VIEW_NAMEONLY)
         ws = WS_DIRSTYLE | LBS_MULTICOLUMN | WS_HSCROLL | WS_VISIBLE | WS_BORDER | LBS_DISABLENOSCROLL;
      else
         ws = WS_DIRSTYLE | WS_HSCROLL | WS_VSCROLL |  WS_VISIBLE | WS_BORDER | LBS_DISABLENOSCROLL;

      GetClientRect(hwnd, &rc);

      //
      // the border parameters are for the non initial create case
      // I don't know why
      //
      hwndLB = CreateWindowEx(0L,
                              szListbox, // atomDirListBox,
                              NULL,
                              ws,
                              dyBorder,
                              dyBorder,
                              rc.right - 2*dyBorder,
                              rc.bottom - 2*dyBorder,
                              hwnd,
                              (HMENU)IDCW_LISTBOX,
                              hAppInstance,
                              NULL);

      if (!hwndLB) {

         FreeDTA(hwnd);

         if (uMsg != WM_CREATE)
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);

         ShowCursor(FALSE);
         SetCursor(hCursor);

         lRetval = -1L;
         goto Done;
      }

      if (!dwNewAttribs)
         dwNewAttribs = ATTR_DEFAULT;

      //
      //  Set all the view/sort/include parameters if they haven't
      //  already been set.
      //
      //  NOTE: A value of 0 in GWL_SORT is invalid, so check this
      //        to see if the values have been initialized yet.
      //
      if (!GetWindowLongPtr(hwndListParms, GWL_SORT))
      {
         SetWindowLongPtr(hwndListParms, GWL_VIEW, dwNewView);
         SetWindowLongPtr(hwndListParms, GWL_SORT, dwNewSort);
         SetWindowLongPtr(hwndListParms, GWL_ATTRIBS, dwNewAttribs);
      }

      //
      // restore the last focus stuff if we are recreating here
      //
      if (!GetWindowLongPtr(hwndListParms, GWL_LASTFOCUS)) {
         SetWindowLongPtr(hwndListParms, GWL_LASTFOCUS, (LPARAM)hwndLB);
      }

  CreateNewPath:

	  if (bCreateDTABlock) {

         //
         // at this point szPath has the directory to read.  this
         // either came from the WM_CREATE case or the
         // FS_CHANGEDISPLAY (CD_PATH) directory reset
         //

		 CharUpperBuff(szPath, 1);     // make sure

		 SetWindowLongPtr(hwndListParms, GWL_TYPE, szPath[0] - TEXT('A'));

		 SetMDIWindowText(hwndListParms, szPath);

         lpStart = CreateDTABlock(hwnd,
                                  szPath,
                                  dwNewAttribs,
                                  (lParam == 0L) || bDontSteal);


         if (hwnd != hwndListParms)
         {
            //
            // For non search windows, reset notification
            //
            ModifyWatchList(hwndListParms,
                            NULL,
                            FILE_NOTIFY_CHANGE_FLAGS);

            if (lpStart)
            {
                GetMDIWindowText(hwndListParms, szPath, COUNTOF(szPath));
                StripFilespec(szPath);

                ModifyWatchList(hwndListParms,
                                szPath,
                                FILE_NOTIFY_CHANGE_FLAGS);
            }
         }

         //
         // We still need to indicate that space may be out of date.
         //
         // Moved to wfdirrd.c, right before SendMessage of FS_DIRREADDONE
         //      R_Space(DRIVEID(szPath));
         //
      }

      SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
      FillDirList(hwnd, lpStart);

      //
      // set the font and dimensions here
      //
      SetLBFont(hwnd,
                hwndLB,
                hFont,
                GetWindowLongPtr(hwndListParms, GWL_VIEW),
                lpStart);


      if (pszInitialDirSel) {

         //
         // ASSUME: pSelInfo->pSel needn't be freed.
         // This should be true since in WM_CREATE (the only time
         // which pszInitialDirSel is != NULL), pSel not initialized.
         //
         pSelInfo->pSel = pszInitialDirSel;
         pSelInfo->bSelOnly = TRUE;
         pszInitialDirSel = NULL;
      }

ResetSelection:

      if (lpStart) {

         SetSelInfo(hwndLB, lpStart, pSelInfo);

      } else {

         //
         // Wasn't used immediately; save it.
         //
         pSelInfoOld = (PSELINFO)GetWindowLongPtr(hwnd, GWL_SELINFO);

         if (pSelInfoOld != pSelInfo) {

            FreeSelInfo(pSelInfoOld);
            SetWindowLongPtr(hwnd, GWL_SELINFO, (LPARAM)pSelInfo);
         }
         pSelInfo = NULL;
      }

      if (bResetFocus)
         if (SetDirFocus(hwnd))
                SetFocus(hwnd);

      SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);

      InvalidateRect(hwndLB, NULL, TRUE);

      ShowCursor(FALSE);
      SetCursor(hCursor);
      break;
   }

Done:

   FreeSelInfo(pSelInfo);
   return lRetval;
}

VOID
FreeSelInfo(
   PSELINFO pSelInfo)
{
   if (pSelInfo) {

      if (pSelInfo->pSel)
         LocalFree((HLOCAL)pSelInfo->pSel);

      LocalFree((HLOCAL)pSelInfo);
   }
}



/////////////////////////////////////////////////////////////////////
//
// Name:     SetSelInfo
//
// Synopsis: Sets selection in dir based on pSelInfo
//
// hwndLB    list box to select
// lpStart   Directory Information  MUST BE VALID (non-NULL)
// pSelInfo  Selection info (if NULL, first item selected)
//
//
// Return:   BOOL, T=Used selection, F=selected first
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

BOOL
SetSelInfo(
   HWND hwndLB,
   LPXDTALINK lpStart,
   PSELINFO pSelInfo)
{
   INT iSel;
   INT iLBCount;
   LPXDTA lpxdta;
   INT iTop;

   iLBCount = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

   if (!pSelInfo) {
      goto SelectFirst;
   }

   iTop = pSelInfo->iTop;

   if (pSelInfo->pSel && pSelInfo->pSel[0]) {

      //
      // Give the selected item the focus rect and anchor pt.
      //
      if (SetSelection(hwndLB, lpStart, pSelInfo->pSel)) {

         if (pSelInfo->bSelOnly)
            return TRUE;

         iSel = DirFindIndex(hwndLB, lpStart, pSelInfo->szTopIndex);

         if (iSel == -1)
            iSel = 0;

         SendMessage(hwndLB, LB_SETTOPINDEX, iSel, 0L);

         iSel = DirFindIndex(hwndLB, lpStart, pSelInfo->szAnchor);

         if (iSel == -1)
            iSel = 0;

         SendMessage(hwndLB, LB_SETANCHORINDEX, iSel, 0L);

         iSel = DirFindIndex(hwndLB, lpStart, pSelInfo->szCaret);

         if (iSel == -1)
            iSel = 0;

         //
         // SETCARETINDEX will scroll item into view
         //
         SendMessage(hwndLB, LB_SETCARETINDEX, iSel, 0L);

         return TRUE;
      }

      //
      // If the latest top selection disappeared, then just select the
      // same index (but different file).
      //
      if (pSelInfo->iLastSel != -1 && (pSelInfo->iLastSel <= iLBCount)) {

         iSel = pSelInfo->iLastSel;

         //
         // check the case of the last item being deleted
         //
         if (iSel == iLBCount)
            iSel--;

         SendMessage(hwndLB, LB_SETSEL, TRUE, (DWORD)iSel);

      } else {
         goto SelectFirst;
      }

   } else {

SelectFirst:

      //
      // Select the first non-directory item
      //
      iSel = 0;
      while (iSel < iLBCount) {

         if (SendMessage(hwndLB,
                         LB_GETTEXT,
                         iSel,
                         (LPARAM)&lpxdta) == LB_ERR || !lpxdta) {
            break;
         }

         if (!(lpxdta->dwAttrs & ATTR_PARENT)) {
            iTop = iSel;
            break;
         }
         iSel++;
      }

      if (iSel == iLBCount)
         iSel = 0;
   }

   SendMessage(hwndLB, LB_SETTOPINDEX, iTop, 0L);

   //
   // and select this item if no tree window
   //
   if (!HasTreeWindow(GetParent(GetParent(hwndLB))))
      SendMessage(hwndLB, LB_SETSEL, TRUE, (DWORD)iSel);

   SendMessage(hwndLB, LB_SETANCHORINDEX, iSel, 0L);

   //
   // SETCARETINDEX will scroll item into view
   //
   SendMessage(hwndLB, LB_SETCARETINDEX, iSel, 0L);

   return FALSE;
}





/////////////////////////////////////////////////////////////////////
//
// Name:     GetPict
//
// Synopsis: Returns the number of consecutive chars of same kind.
//
// ch        Char to find
// pszStr    string to search
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

UINT
GetPict(
   WCHAR ch,
   LPCWSTR pszStr)
{
   register UINT  count;

   count = 0;
   while (ch == *pszStr++)
      count++;

   return count;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     CreateDate
//
// Synopsis: Converts a local time to a localized string
//
// IN        lpst       LPSYSTEMTIME  local system time
// INOUT     lpszOutStr --            string of date
//
// Return:   INT   length of date
//
// Assumes:  lpszOutStr is large enough for the date string.
//           Separator is one character long
//
// Effects:  lpszOutStr modified.
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

INT
CreateDate(
   LPSYSTEMTIME lpst,
   LPWSTR szOutStr)
{
    /*
     *  Need to subtract one from the return from GetDateFormatW
     *  to exclude the null terminator.
     */
    return ( GetDateFormatW( lcid,
                             DATE_SHORTDATE,
                             lpst,
                             NULL,
                             szOutStr,
                             MAXPATHLEN ) - 1 );
}



/////////////////////////////////////////////////////////////////////
//
// Name:     CreateTime
//
// Synopsis: Creates a localized time string for local time
//
// IN    lpst       LPSYSTEMTIME  local time
// INOUT lpszOutStr --            String
//
// Return:   INT  length of string
//
//
// Assumes:   lpszOutStr is big enough for all times
//
// Effects:   lpszOutStr modified.
//            Separator is 1 character
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

INT
CreateTime(
   LPSYSTEMTIME lpst,
   LPWSTR szOutStr)
{
    /*
     *  Need to subtract one from the return from GetTimeFormatW
     *  to exclude the null terminator.
     */
    return ( (bJapan) ? ( GetTimeFormatW( lcid,
                                          TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER,
                                          lpst,
                                          NULL,
                                          szOutStr,
                                          MAXPATHLEN ) - 1 )
                      : ( GetTimeFormatW( lcid,
                                          0,
                                          lpst,
                                          NULL,
                                          szOutStr,
                                          MAXPATHLEN ) - 1 ) );

}



/////////////////////////////////////////////////////////////////////
//
// Name:     PutSize
//
// Synopsis: puts large integer into string
//
// Return:
//
// Assumes:
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

INT
PutSize(
    PLARGE_INTEGER pqSize,
    LPWSTR szOutStr)
{
    INT Size;
    WCHAR szBuffer[MAXFILENAMELEN];
    NUMBERFMT NumFmt;

    /*
     *  Convert it into a string.
     */
    wsprintf(szBuffer, TEXT("%I64u"), pqSize->QuadPart);

    /*
     *  Format the string.
     */
    NumFmt.NumDigits = 0;
    NumFmt.LeadingZero = 0;
    NumFmt.Grouping = 3;
    NumFmt.lpDecimalSep = szDecimal;
    NumFmt.lpThousandSep = szComma;
    NumFmt.NegativeOrder = 1;

    if ( Size = GetNumberFormatW( GetUserDefaultLCID(),
                                  0,
                                  szBuffer,
                                  &NumFmt,
                                  szOutStr,
                                  MAXFILENAMELEN ) )
    {
        /*
         *  Return the size (without the null terminator).
         */
        return (Size - 1);
    }

    /*
     *  GetNumberFormat call failed, so just return the number string
     *  unformatted.
     */
    lstrcpy(szOutStr, szBuffer);
    return (wcslen(szOutStr));
}



/////////////////////////////////////////////////////////////////////
//
// Name:     PutDate
//
// Synopsis: Puts the date into a string
//
// Return:   Size of added string
//
// Assumes:
//
// Effects:
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////


INT
PutDate(
   LPFILETIME lpftDate,
   LPWSTR szStr)
{
   SYSTEMTIME st;
   FILETIME ftNew;

   FileTimeToLocalFileTime(lpftDate, &ftNew);
   FileTimeToSystemTime(&ftNew, &st);

   return CreateDate(&st, szStr);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     PutTime
//
// Synopsis:
//
// Return:
//
// Assumes:
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////


INT
PutTime(
   LPFILETIME lpftTime,
   LPWSTR szStr)
{
   SYSTEMTIME st;
   FILETIME ftNew;

   FileTimeToLocalFileTime(lpftTime, &ftNew);
   FileTimeToSystemTime(&ftNew, &st);

   return(CreateTime(&st, szStr));
}



/////////////////////////////////////////////////////////////////////
//
// Name:     PutAttributes
//
// Synopsis:
//
// Return:
//
// Assumes:
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

INT
PutAttributes(
   register DWORD dwAttribute,
   register LPWSTR pszStr)
{
   INT   cch = 0;

   if (dwAttribute & ATTR_READONLY)
   {
      *pszStr++ = szAttr[0];
      cch++;
   }

   if (dwAttribute & ATTR_HIDDEN)
   {
      *pszStr++ = szAttr[1];
      cch++;
   }

   if (dwAttribute & ATTR_SYSTEM)
   {
      *pszStr++ = szAttr[2];
      cch++;
   }

   if (dwAttribute & ATTR_ARCHIVE)
   {
      *pszStr++ = szAttr[3];
      cch++;
   }

   if (dwAttribute & ATTR_COMPRESSED)
   {
      *pszStr++ = szAttr[4];
      cch++;
   }

   if (dwAttribute & ATTR_ENCRYPTED)
   {
      *pszStr++ = szAttr[5];
      cch++;
   }

   *pszStr = CHAR_NULL;
   return(cch);
}





/////////////////////////////////////////////////////////////////////
//
// Name:     GetMaxExtent
//
// Synopsis: Compute the max ext of all files in this DTA block
//           and update the case to match (wTextAttribs & TA_LOWERCASE)
//
// Return:
//
// Assumes:
//
// Effects:
//
// Notes:
//
//        !! Called by UI and worker thread !!
//
/////////////////////////////////////////////////////////////////////

INT
GetMaxExtent(HWND hwndLB, LPXDTALINK lpLink, BOOL bNTFS)
{
   HDC hdc;
   DWORD dwItems;
   INT maxWidth = 0;
   SIZE size;
   HFONT hOld;
   WCHAR szPath[MAXPATHLEN];
   LPWSTR pszName;
   LPXDTA lpxdta;

   if (!lpLink)
      goto NoDTA;

   hdc = GetDC(hwndLB);
   hOld = SelectObject(hdc, hFont);

   for (dwItems = MemLinkToHead(lpLink)->dwEntries, lpxdta = MemFirst(lpLink);
        dwItems;
        dwItems--, lpxdta = MemNext(&lpLink, lpxdta))
   {
      if (bNTFS)
      {
         pszName = MemGetAlternateFileName(lpxdta);

         if (pszName[0])
         {
            lstrcpy(szPath, pszName);

            //
            // ALWAYS AnsiUpper/Lower based on TA_LOWERCASE
            // since this is a dos style name for ntfs.
            //
            if (wTextAttribs & TA_LOWERCASE ||
                wTextAttribs & TA_LOWERCASEALL)
            {
               CharLower(szPath);
            }
            else
            {
               CharUpper(szPath);
            }

            GetTextExtentPoint32(hdc, szPath, lstrlen(szPath), &size);

            maxWidth = max(size.cx, maxWidth);
         }
      }
      else
      {
         lstrcpy(szPath, MemGetFileName(lpxdta));

         //
         // set the case of the file names here!
         //
         if ( ( (lpxdta->dwAttrs & ATTR_LOWERCASE) &&
                (wTextAttribs & TA_LOWERCASE) ) ||
              (wTextAttribs & TA_LOWERCASEALL) )
         {
            CharLower(szPath);
         }

         GetTextExtentPoint32(hdc, szPath, lstrlen(szPath), &size);

         maxWidth = max(size.cx, maxWidth);
      }
   }

   if (hOld)
       SelectObject(hdc, hOld);

   ReleaseDC(hwndLB, hdc);

NoDTA:
   return maxWidth + 4; // pad it out
}




/////////////////////////////////////////////////////////////////////
//
// Name:     FixTabsAndThings
//
// Synopsis: Sets tabstops array for TabbedTextOut() calls
//
//
// Return:   Total extent of "File Details" view.
//           Used to set scroll extents
//
// Assumes:
//
// Effects:
//
//
// Notes:
//
//        !! Called by UI and worker thread !!
//
/////////////////////////////////////////////////////////////////////

INT
FixTabsAndThings(
   HWND hwndLB,
   WORD *pwTabs,
   INT iMaxWidthFileName,
   INT iMaxWidthNTFSFileName,
   DWORD dwViewOpts)
{
   INT i;
   HDC hdc;
   HFONT hOld;
   TCHAR szBuf[30];
   SIZE size;

   //
   // Max extent time/date
   //
   static SYSTEMTIME st = {
      9999,
      12,
      3,
      30,
      12,
      59,
      59,
      999
   };

   i = iMaxWidthFileName;  // the widest filename

   if (pwTabs == NULL)
      return i;

   hdc = GetDC(NULL);
   hOld = SelectObject(hdc, hFont);

   //
   //  Only if flag is set...
   //  Don't check if fIsNTFS
   //  Don't do if this is a search window
   //
   if (dwViewOpts & VIEW_DOSNAMES) {

      //
      // We have a 8.3 name to display
      //
      i += iMaxWidthNTFSFileName + dxText;
      *pwTabs++ = (WORD)i;  // Size
   }

   //
   // max size digits field  (allow for large integer - 2 dwords)
   //
   if (dwViewOpts & VIEW_SIZE)
   {
      GetTextExtentPoint32(hdc, TEXT("999,999,999,999"), 15, &size);
      i += size.cx + dxText;
      *pwTabs++ = (WORD)i;  // Size
   }

   if (dwViewOpts & VIEW_DATE) {

      //
      // Bypass UTC by using Create Date!
      //

      CreateDate(&st, szBuf);

      //
      // max date digits
      //
      GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);
      i += size.cx + dxText;
      *pwTabs++ = (WORD)i;  // Date
   }

   //
   // max time digits
   //
   if (dwViewOpts & VIEW_TIME) {

      CreateTime(&st, szBuf);

      GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);
      i += size.cx + dxText;
      *pwTabs++ = (WORD)i;  // Time
   }

   //
   // max attribute digits
   //
   if (dwViewOpts & VIEW_FLAGS) {
      PutAttributes(ATTR_ALL, szBuf);
      GetTextExtentPoint32(hdc, szBuf, lstrlen(szBuf), &size);
      i += size.cx + dxText;
      *pwTabs++ = (WORD)i;  // Attributes
   }

   if (hOld)
      SelectObject(hdc, hOld);

   ReleaseDC(NULL, hdc);

   SendMessage(hwndLB,
               LB_SETHORIZONTALEXTENT,
               i + dxFolder + 4 * dyBorderx2,
               0L);
   //
   // total extent
   //
   return i;
}





/////////////////////////////////////////////////////////////////////
//
// Name:     SetLBFont
//
// Synopsis: Sets font and adjust dimension parms for the new font
//
//    hwnd      hwnd of dir window
//    hwndLB    it's listbox
//    hNewFont  new font
//
// Return:
//
//
// Assumes:
//
//    alpxdtaSorted initialized and filled
//
//    dyFileName  GLOBAL; set based on new font height
//    GWL_VIEW    view definition
//    GWL_HDTA    data
//
// Effects:
//
//    Listbox tabs array
//       LB_SETCOLUMNWIDTH
//       or
//       LB_SETHORIZONTALEXTENT
//
// Notes: Must not have any dependency on GWL_HDTA state
//
//        !! Called by UI and worker thread !!
//
/////////////////////////////////////////////////////////////////////


VOID
SetLBFont(
   HWND hwnd,
   HWND hwndLB,
   HANDLE hNewFont,
   DWORD dwViewFlags,
   LPXDTALINK lpStart)
{
   INT dxMaxExtent;
   LPXDTAHEAD lpHead;


   SendMessage(hwndLB, WM_SETFONT, (WPARAM)hNewFont, MAKELPARAM(TRUE, 0));

   if (!lpStart)
      return;

   //
   // this is needed when changing the font. when creating
   // the return from WM_MEASUREITEM will set the cell height
   //
   SendMessage(hwndLB,
               LB_SETITEMHEIGHT,
               0,
               (LONG)dyFileName);

   lpHead = MemLinkToHead(lpStart);

   dxMaxExtent = GetMaxExtent(hwndLB, lpStart, FALSE);

   //
   // if we are in name only view we change the width
   //
   if ((VIEW_EVERYTHING & dwViewFlags) == VIEW_NAMEONLY) {

      SendMessage(hwndLB,
                  LB_SETCOLUMNWIDTH,
                  dxMaxExtent + dxFolder + dyBorderx2,
                  0L);

   } else {

      lpHead->dwAlternateFileNameExtent = GetMaxExtent(hwndLB,
                                                       lpStart,
                                                       TRUE);

      FixTabsAndThings(hwndLB,
                       (WORD *)GetWindowLongPtr(hwnd, GWL_TABARRAY),
                       dxMaxExtent,
                       lpHead->dwAlternateFileNameExtent,
                       dwViewFlags);
   }
}


INT
CharCountToTab(LPWSTR pszStr)
{
   LPWSTR pszTmp = pszStr;

   while (*pszStr && *pszStr != CHAR_TAB) {
      pszStr++;
   }

   return pszStr-pszTmp;
}


VOID
RightTabbedTextOut(
   HDC hdc,
   INT x, INT y,
   LPWSTR pLine,
   WORD *pTabStops,
   INT x_offset,
   DWORD dwAlternateFileNameExtent)
{
   INT len, cch;
   INT count = 0;
   SIZE size;

   len = lstrlen(pLine);

   cch = CharCountToTab(pLine);
   GetTextExtentPoint32(hdc, pLine, cch, &size);

   // first position is left aligned so bias initial x value
   x += size.cx;

   //
   // NOTE: on NTFS volumes, the second field is also left aligned.
   //
   while (len) {

      len -= cch + 1;

      ExtTextOut(hdc, x - size.cx, y, 0, NULL, pLine, cch, NULL);

      if (len <= 0)
         return;

      pLine += cch + 1;

      cch = CharCountToTab(pLine);
      GetTextExtentPoint32(hdc, pLine, cch, &size);

      x = *pTabStops++ + x_offset;

      if (++count == 1 && dwAlternateFileNameExtent)
         x += size.cx - dwAlternateFileNameExtent;
   }
}




/////////////////////////////////////////////////////////////////////
//
// Name:     FillDirList
//
// Synopsis: Fills the directory window listbox with entries from hDTA
//
// INOUT     hwnd    hwnd ListBox to add strings to
// INOUT     lpStart handle to DTA block to get files from
// IN        iError
//
// Return:   VOID
//
// Assumes:  hDTA->head.alpxdtaSorted is NULL _or_
//           holds valid previously malloc'd array of pointers
//
// Effects:  mallocs hDTA->head.alpxdtaSorted if it was NULL
//           In-place sorts alpxdtaSorted also
//
// Notes:    Can be called by either worker or UI thread, therefore
//           must be reentrant.
//
/////////////////////////////////////////////////////////////////////

VOID
FillDirList(
   HWND hwndDir,
   LPXDTALINK lpStart)
{
   register DWORD count;
   UINT   i;
   LPXDTAHEAD lpHead;
   INT iError;
   HWND hwndLB = GetDlgItem(hwndDir, IDCW_LISTBOX);

   //
   // Filling dir list; invalidate cache!
   //
   ExtSelItemsInvalidate();

   iError = (INT)GetWindowLongPtr(hwndDir, GWL_IERROR);

   lpHead = MemLinkToHead(lpStart);

   if (!lpStart || iError) {

Error:
      // token for no items
      SendMessage(hwndLB, LB_INSERTSTRING, 0, 0L);
      return;
   }

   count = lpHead->dwEntries;

   if (count == 0) {

      //
      // Now always set 0L since string is in lpxdta->byBitmap!
      // token for no items
      //
      goto Error;

   } else {

      if (!lpHead->alpxdtaSorted) {
         lpHead->alpxdtaSorted = (LPXDTA *)LocalAlloc(LMEM_FIXED,
            sizeof(LPXDTA) * count);
      }

      if (lpHead->alpxdtaSorted) {

         SortDirList(hwndDir, lpStart, count, lpHead->alpxdtaSorted);

         for (i = 0; i < count; i++) {
            SendMessage(hwndLB,
                        LB_INSERTSTRING,
                        (WPARAM)-1,
                        (LPARAM)lpHead->alpxdtaSorted[i]);
         }
      }
   }
}




/////////////////////////////////////////////////////////////////////
//
// Name:     CompareDTA
//
// Synopsis:
//
// Return:
//
// Assumes:
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

INT
CompareDTA(
   register LPXDTA lpItem1,
   register LPXDTA lpItem2,
   DWORD dwSort)
{
   register INT  ret;

   if (!lpItem1 || !lpItem2)
      return lpItem1 ? 1 : -1;

   if (lpItem1->dwAttrs & ATTR_PARENT) {
      ret = -1;
      goto CDDone;
   }

   if (lpItem2->dwAttrs & ATTR_PARENT) {
      ret = 1;
      goto CDDone;
   }

   if ((lpItem1->dwAttrs & ATTR_DIR) > (lpItem2->dwAttrs & ATTR_DIR)) {
      ret = -1;
      goto CDDone;
   } else if ((lpItem1->dwAttrs & ATTR_DIR) < (lpItem2->dwAttrs & ATTR_DIR)) {
      ret = 1;
      goto CDDone;
   }


   switch (dwSort) {
   case IDD_TYPE:
      {
         LPWSTR ptr1;
         LPWSTR ptr2;

         ptr1 = GetExtension(MemGetFileName(lpItem1));
         ptr2 = GetExtension(MemGetFileName(lpItem2));

         ret = lstrcmpi(ptr1, ptr2);

         if (ret == 0) {

            if (*ptr1) {
               *--ptr1 = CHAR_NULL;
            } else {
               ptr1 = NULL;
            }

            if (*ptr2) {
               *--ptr2 = CHAR_NULL;
            } else {
               ptr2 = NULL;
            }

            ret = lstrcmpi(MemGetFileName(lpItem1), MemGetFileName(lpItem2));

            if (ptr1) {
               *ptr1 = CHAR_DOT;
            }

            if (ptr2) {
               *ptr2 = CHAR_DOT;
            }
         }

         break;
      }

   case IDD_SIZE:

      if (lpItem1->qFileSize.HighPart == lpItem2->qFileSize.HighPart) {
         if (lpItem1->qFileSize.LowPart > lpItem2->qFileSize.LowPart)
            ret = -1;
         else if (lpItem1->qFileSize.LowPart < lpItem2->qFileSize.LowPart)
            ret = 1;
         else
            goto CompareNames;
      } else {
         if (lpItem1->qFileSize.HighPart > lpItem2->qFileSize.HighPart)
            ret = -1;
         else if (lpItem1->qFileSize.HighPart < lpItem2->qFileSize.HighPart)
            ret = 1;
         else
            goto CompareNames;
      }
      break;

   case IDD_DATE:
   case IDD_FDATE:
      {
         DWORD d1High, d1Low;
         DWORD d2High, d2Low;

         d1High = lpItem1->ftLastWriteTime.dwHighDateTime;
         d2High = lpItem2->ftLastWriteTime.dwHighDateTime;

         if (d1High > d2High) {
            ret = -1;
         }
         else if (d1High < d2High) {
            ret = 1;
         }
         else {
            d1Low = lpItem1->ftLastWriteTime.dwLowDateTime;
            d2Low = lpItem2->ftLastWriteTime.dwLowDateTime;

            if (d1Low > d2Low)
               ret = -1;
            else if (d1Low < d2Low)
               ret = 1;
            else
               goto CompareNames;

         }
         if (dwSort == IDD_FDATE)
             ret = -ret;
         break;
      }

   case IDD_NAME:

CompareNames:
      ret = lstrcmpi(MemGetFileName(lpItem1), MemGetFileName(lpItem2));
      break;
   }

CDDone:
  return ret;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     DirGetSelection()
//
// Synopsis:
//
// Takes a Listbox and returns a string containing the names of the selected
// files separated by spaces.
//
// iSelType == 0 return all files: dirs fully qualified, rest filespec only
// iSelType == 1 return only the first file (filter out the rest)
// iSelType == 2 test for LFN files in the selection, doesn't return string
// iSelType == 4 return fully qualified names
// iSelType == 8 return filespec (fully UNqualified; only last part)
// iSelType & 16 (bitfield) don't call CheckEsc!  Use with care!
// iSelType & 32 (bitfield) use short name when possible.
// iSelType & 64 (bitfield) check for compression attributes.
//
//
// Return:
//
//      if (iSelType == 2)
//              TRUE/FALSE if LFN is in the selection
//      else if (iSelType == 64)
//              1 - found uncompressed file
//              2 - found compressed file
//              3 - found both compressed and uncompressed file
//      else
//              pointer to the list of names (ANSI strings)
//              (must be freed by caller!)
//              *pfDir -> bool indicating directories are
//              contained in the selection (or that LFN names are present)
//
//
// Assumes:
//
// Effects:
//
//
// Notes: The caller must free the returned pointer!
//
/////////////////////////////////////////////////////////////////////

LPWSTR
DirGetSelection(
   HWND hwndDir,
   HWND hwndView,
   HWND hwndLB,
   INT  iSelType,
   BOOL *pfDir,
   PINT piLastSel)
{
   LPWSTR p = NULL, pT;
   INT i;
   INT cch;
   INT iMac;
   LPXDTA lpxdta;
   LPXDTALINK lpStart;
   LPXDTA* alpxdta;
   WCHAR szFile[MAXPATHLEN];
   WCHAR szPath[MAXPATHLEN];
   WCHAR szTemp[MAXPATHLEN];
   BOOL bDir;
   LPINT lpSelItems;
   BOOL bLFNTest;
   LPWSTR pszName;
   BOOL bCompressTest;
   UINT uiAttr = 0;


   if (hwndDir) {
      SendMessage(hwndDir,
                  FS_GETDIRECTORY,
                  COUNTOF(szPath),
                  (LPARAM)szPath);
   }

   if ((bLFNTest = (iSelType & 2)) && hwndDir) {

      //
      // determine if the directory itself is long...
      //
      lstrcpy(szTemp, szPath);

      StripBackslash(szTemp);

      if (IsLFN(szTemp))
         if (pfDir) {
            *pfDir = TRUE;
         }

      return (NULL);
   }

   bDir = FALSE;

   if (!bLFNTest) {
      cch = 1;

      //
      // +2 for checkesc safety
      //
      p = (LPTSTR)LocalAlloc(LPTR, ByteCountOf(cch + 2));
      if (!p)
         return (NULL);
      *p = CHAR_NULL;
   }

   lpStart = (LPXDTALINK)GetWindowLongPtr(hwndView, GWL_HDTA);
   if (!lpStart) {

Fail:
      if (p)
         LocalFree(p);

      return (NULL);
   }

   bCompressTest = (iSelType & 64);

   iMac = (INT) SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);
   if (iMac == 0)
   {
       if (p)
       {
           LocalFree(p);
           p = NULL;
       }
       goto GDSDone;
   }

   lpSelItems = (LPINT) LocalAlloc(LMEM_FIXED, sizeof(INT) * iMac);
   if (lpSelItems == NULL)
      goto Fail;

   alpxdta = MemLinkToHead(lpStart)->alpxdtaSorted;

   iMac = (INT)SendMessage(hwndLB,
                           LB_GETSELITEMS,
                           (WPARAM)iMac,
                           (LPARAM)lpSelItems);

   if (piLastSel) {
      if (iMac != -1)
         *piLastSel = lpSelItems[0];
      else
         *piLastSel = -1;
   }

   for (i = 0; i < iMac; i++) {

      //
      // !! LATER !!
      //
      // Sorting not implemented for search (yet), so
      // just read off of itemdata
      //
      if (!hwndDir) {

         lpxdta = (LPXDTA)SendMessage(hwndLB, LB_GETITEMDATA, lpSelItems[i], 0L);

      } else {

         lpxdta = alpxdta[lpSelItems[i]];
      }

      if (!lpxdta)
         break;

      if (iSelType & 32) {

         pszName = MemGetAlternateFileName(lpxdta);

         if (pszName[0]) {
            lstrcpy(szFile, pszName);
            goto UsedAltname;
         }
      }

      lstrcpy(szFile, MemGetFileName(lpxdta));

UsedAltname:

      //
      // is this a dir?
      //
      if (lpxdta->dwAttrs & ATTR_DIR) {

         if (hwndDir) {

            // reparse point; szFile is full path
            if (lpxdta->dwAttrs & (ATTR_JUNCTION | ATTR_SYMBOLIC))
            {
               if (iSelType & 8) 
               {
                  // if filename part, strip path
                  StripPath(szFile);
               }
            }
            //
            // parent dir?
            //
            else if (lpxdta->dwAttrs & ATTR_PARENT) {

               //
               // if we are getting a full selection don't
               // return the parent ".." entry (avoid deleting
               // and other nasty operations on the parent)
               //
               // was: if (!iSelType).  continue on 0,2,4  Don't cont on 1
               //
               if (!(iSelType & 1))      // Continue on iSel 2 or 4 or 8 or 0
                  continue;

               lstrcpy(szTemp, szPath);

               StripBackslash(szTemp);       // trim it down
               StripFilespec(szTemp);

               lstrcpy(szFile, szTemp);

            } else {

               if (!(iSelType & 8)) {

                  lstrcpy(szTemp, szPath);

                  lstrcat(szTemp, szFile);      // fully qualified
                  lstrcpy(szFile, szTemp);
               }
            }
         }

         if ((bCompressTest) && !(lpxdta->dwAttrs & ATTR_PARENT))
         {
             uiAttr = 3;
             goto GDSExit;
         }

         bDir = TRUE;
      }
      else if (bCompressTest)
      {
          uiAttr |= ((lpxdta->dwAttrs & ATTR_COMPRESSED) ? 2 : 1);
          if (uiAttr == 3)
          {
              goto GDSExit;
          }
      }

      if (iSelType & 4)
         QualifyPath(szFile);

      if (bLFNTest && lpxdta->dwAttrs & ATTR_LFN) {
         return ( (LPWSTR)TRUE );
      }

      if (!(iSelType & 16))
         CheckEsc(szFile);

      if (!bLFNTest) {
         cch += lstrlen(szFile) + 1;

         //
         // +2 for checkesc safety
         //
         pT = (LPWSTR)LocalReAlloc((HANDLE)p, ByteCountOf(cch + 2), LMEM_MOVEABLE | LMEM_ZEROINIT);
         if (!pT)
            goto GDSExit;
         p = pT;
         lstrcat(p, szFile);
      }

      if (iSelType & 1)
         goto GDSExit;

      if ((!bLFNTest) && ((i + 1) < iMac)) {
          if (p)
              lstrcat(p, szBlank);
      }
   }

GDSExit:
   LocalFree(lpSelItems);

GDSDone:
   if (bLFNTest) {
      if (pfDir) {
         *pfDir = FALSE;
      }
      return (NULL);
   }

   if (bCompressTest)
   {
       return ( (LPWSTR)uiAttr );
   }

   if (pfDir) {
      *pfDir = bDir;
   }
   return (p);
}




/////////////////////////////////////////////////////////////////////
//
// Name:     DirFindIndex
//
// Synopsis: Finds the index of szFile in the list box
//
// IN    hwndLB    list box to search for
// IN    hDTA      DTA to search match strings against
// IN    lpszFile  file to search for
//
// Return:  INT    index, (-1) = not found
//
// Assumes: hDTA->head.alpxdtaSorted is valid and matches listbox
//          structure
//
//          hDTA->head.dwEntries must be < INTMAX since there is
//          a conversion from dword to int.  blech.
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////


INT
DirFindIndex(HWND hwndLB, LPXDTALINK lpStart, LPWSTR lpszFile)
{
   register INT i;
   DWORD dwSel;
   LPXDTA lpxdta;

   if (!lpStart)
      return -1;

   dwSel = MemLinkToHead(lpStart)->dwEntries;

   for (i = 0; i < (INT) dwSel; i++) {

      if (SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&lpxdta) == LB_ERR)
         return -1;

      if (lpxdta && !lstrcmpi(lpszFile, MemGetFileName(lpxdta)))
         return i;
   }

   return -1;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     DirGetAnchorFocus
//
// Synopsis:
// Return:
// Assumes:
// Effects:
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
DirGetAnchorFocus(
   register HWND hwndLB,
   LPXDTALINK lpStart,
   PSELINFO pSelInfo)
{
   register INT iSel, iCount;
   LPXDTA   lpxdta;

   iSel = (INT)SendMessage(hwndLB, LB_GETANCHORINDEX, 0, 0L);
   iCount = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

   pSelInfo->szAnchor[0] = CHAR_NULL;
   pSelInfo->szCaret[0] = CHAR_NULL;
   pSelInfo->szTopIndex[0] = CHAR_NULL;

   if (!lpStart)
      return;

   if (iCount == 1) {
      SendMessage(hwndLB, LB_GETTEXT, (WPARAM)iSel, (LPARAM)&lpxdta);

      if (!lpxdta) {
         return;
      }
   }
   if (iSel >= 0 && iSel < iCount) {
      SendMessage(hwndLB, LB_GETTEXT, (WPARAM)iSel, (LPARAM)&lpxdta);

      lstrcpy(pSelInfo->szAnchor, MemGetFileName(lpxdta));
   }

   iSel = (INT)SendMessage(hwndLB, LB_GETCARETINDEX, 0, 0L);

   if (iSel >= 0 && iSel < iCount) {
      SendMessage(hwndLB, LB_GETTEXT, (WPARAM)iSel, (LPARAM)&lpxdta);
      lstrcpy(pSelInfo->szCaret, MemGetFileName(lpxdta));
   }

   iSel = SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);

   if (iSel >= 0 && iSel < iCount) {
      SendMessage(hwndLB, LB_GETTEXT, (WPARAM)iSel, (LPARAM)&lpxdta);
      lstrcpy(pSelInfo->szTopIndex, MemGetFileName(lpxdta));
   }
}




/////////////////////////////////////////////////////////////////////
//
// Name:     SetSelection
//
// Synopsis:
//
//
//
//
//
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

BOOL
SetSelection(
   HWND hwndLB,
   LPXDTALINK lpStart,
   LPWSTR pszSel)
{
   INT i;
   WCHAR szFile[MAXPATHLEN];
   BOOL bDidSomething = FALSE;

   while (pszSel = GetNextFile(pszSel, szFile, COUNTOF(szFile))) {

      i = DirFindIndex(hwndLB, lpStart, szFile);

      if (i != -1) {
         SendMessage(hwndLB, LB_SETSEL, TRUE, (DWORD)i);
         bDidSomething = TRUE;
      }
   }
   return bDidSomething;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     UpdateStatus
//
// Synopsis: Load the status buffers and repaint it
//
//
//
//
//
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
UpdateStatus(HWND hwnd)
{
   WCHAR szNumBuf1[40];
   WCHAR szNumBuf2[40];
   DRIVE drive;
   HWND hwndDir;

   if (!bStatusBar)
      return;

   if (hwnd != (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L))
      return;

   if (HasTreeWindow(hwnd)) {

      drive = GetWindowLongPtr(hwnd, GWL_TYPE);

      if (SPC_IS_NOTREE(qFreeSpace)) {

         SetStatusText(0,
                       SST_RESOURCE,
                       (LPWSTR) MAKEINTRESOURCE(IDS_TREEABORT));

      } else {

         //
         // cache free disk space for update status only
         // Now lFreeSpace == -3 means do a refresh (hit disk!)
         //
         if (SPC_REFRESH(qFreeSpace)) {

            if (SPC_IS_HITDISK(qFreeSpace)) {

               R_Space(drive);
            }

            U_Space(drive);

         }

         qFreeSpace=aDriveInfo[drive].qFreeSpace;
         qTotalSpace=aDriveInfo[drive].qTotalSpace;

         SetStatusText(0, SST_RESOURCE|SST_FORMAT,
            (LPWSTR) MAKEINTRESOURCE(IDS_DRIVEFREE),
            L'A' + drive,
            ShortSizeFormatInternal(szNumBuf1, qFreeSpace),
            ShortSizeFormatInternal(szNumBuf2, qTotalSpace));
      }
   } else
      SetStatusText(0, 0L, szNULL);


   hwndDir = HasDirWindow(hwnd);

   if (hwndDir)
   {
      GetDirStatus(hwndDir, szStatusTree, szStatusDir);
   }
   else
   {
      SetStatusText(1, 0L, szNULL);
   }
}


HWND
GetDirSelData(
   HWND hwnd,
   LARGE_INTEGER *pqSelSize,
   INT *piSelCount,
   LARGE_INTEGER* pqTotalSize,
   INT *piTotalCount,
   LPFILETIME *ppftLastWrite,
   BOOL *pisDir,
   BOOL *pisNet,
   LPWSTR pszName)
{
   INT i;
   LPXDTA lpxdta;
   LPXDTALINK lpStart;
   HWND hwndLB;
   INT iMac;
   LPXDTAHEAD lpHead;
   LPINT lpSelItems;

   *pszName = CHAR_NULL;

   if (!(hwndLB = GetDlgItem(hwnd, IDCW_LISTBOX))) {       // fast scroll
      return NULL;
   }

   LARGE_INTEGER_NULL(*pqSelSize);
   *piSelCount = 0;

   lpStart = (LPXDTALINK)GetWindowLongPtr(hwnd, GWL_HDTA);

   if (!lpStart) {

      LARGE_INTEGER_NULL(*pqTotalSize);
      *piTotalCount = 0;
      *pisDir = FALSE;
      *pisNet = FALSE;

      return NULL;
   }

   lpHead = MemLinkToHead(lpStart);

   *pqTotalSize = lpHead->qTotalSize;
   *piTotalCount = (INT)lpHead->dwTotalCount;

   iMac = (INT) SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);

   if (iMac == LB_ERR)
      return NULL;

   lpSelItems = (LPINT) LocalAlloc(LMEM_FIXED, sizeof(INT) * iMac);

   if (lpSelItems == NULL)
      return NULL;

   iMac = (INT)SendMessage(hwndLB, LB_GETSELITEMS, (WPARAM)iMac, (LPARAM)lpSelItems);

   for (i=0; i < iMac; i++) {

      SendMessage(hwndLB, LB_GETTEXT, lpSelItems[i], (LPARAM)(LPTSTR)&lpxdta);

      if (!lpxdta)
         break;

      if (lpxdta->dwAttrs & ATTR_PARENT)
         continue;

      (*piSelCount)++;


      (*pqSelSize).QuadPart = (*pqSelSize).QuadPart + (lpxdta->qFileSize).QuadPart;
      *ppftLastWrite = &(lpxdta->ftLastWriteTime);

      *pisDir = (lpxdta->dwAttrs & ATTR_DIR) != 0;
      *pisNet = lpxdta->byBitmap == BM_IND_CLOSEDFS;
      if (*pszName == CHAR_NULL)
         lstrcpy(pszName, MemGetFileName(lpxdta));
   }

   LocalFree(lpSelItems);

   return hwndLB;
}



VOID
GetDirStatus(
   HWND hwnd,
   LPWSTR szMessage1,
   LPWSTR szMessage2)
{
   INT iSelCount, iCount;
   LARGE_INTEGER qSelSize, qSize;
   WCHAR szNumBuf[40];
   HWND hwndLB;
   LPFILETIME lpftLastWrite;
   BOOL isDir, isNet;
   WCHAR szName[MAXPATHLEN];

   if (!GetWindowLongPtr(hwnd, GWL_HDTA) &&
      !GetWindowLongPtr(hwnd, GWL_IERROR)) {

      SetStatusText(1,
                    SST_RESOURCE,
                    (LPWSTR) MAKEINTRESOURCE(IDS_READING));
      return;
   }

   hwndLB = GetDirSelData(hwnd,
                          &qSelSize,
                          &iSelCount,
                          &qSize,
                          &iCount,
                          &lpftLastWrite,
                          &isDir,
                          &isNet,
                          szName);

   SetStatusText(1,
                 SST_RESOURCE|SST_FORMAT,
                 (LPWSTR) MAKEINTRESOURCE(IDS_STATUSMSG),
                 iCount,
                 ShortSizeFormatInternal(szNumBuf, qSize));

   if (hwndLB == (HWND)GetWindowLongPtr((HWND)GetWindowLongPtr(hwnd,
                                                         GWL_LISTPARMS),
                                     GWL_LASTFOCUS)) {

      SetStatusText(0,
                    SST_RESOURCE|SST_FORMAT,
                    (LPWSTR) MAKEINTRESOURCE(IDS_STATUSMSG2),
                    iSelCount,
                    ShortSizeFormatInternal(szNumBuf, qSelSize));

      if (iSelCount == 1) {
         LPWSTR pch;

         if (isDir) {
#ifdef TBCUSTSHOWSHARE
            if (isNet) {
               GetDirUNCName(szName, COUNTOF(szName), hwnd, szName);
               SetStatusText(2, NULL, szName);
            } else
               SetStatusText(2, SST_RESOURCE, (LPTSTR) MAKEINTRESOURCE(IDS_NOTSHARED));
#endif
         } else if (LoadString(hAppInstance, IDS_STATUSMSGSINGLE,
            szMessage, COUNTOF(szMessage))) {

            ShortSizeFormatInternal(szNumBuf, qSelSize);
            wsprintf(szName, szMessage, szNumBuf);

            pch = szName + lstrlen(szName);

            pch += PutDate(lpftLastWrite, pch);
            *(pch++) = CHAR_SPACE;
            pch += PutTime(lpftLastWrite, pch);
            *pch = CHAR_NULL;

            SetStatusText(0, 0L, szName);
         }
      } else {
#ifdef TBCUSTSHOWSHARE
         SetStatusText(2, SST_RESOURCE, (LPTSTR) MAKEINTRESOURCE(IDS_NOTSHARED));
#endif
         SetStatusText(0, SST_RESOURCE|SST_FORMAT, (LPTSTR) MAKEINTRESOURCE(IDS_STATUSMSG2),
            iSelCount, ShortSizeFormatInternal(szNumBuf, qSelSize));
      }
   }
}


// given a descendant of an MDI child (or an MDI child) return
// the MDI child in the descendant chain.  returns NULL if not
// found.


HWND
GetMDIChildFromDescendant(HWND hwnd)
{
   HWND hwndT;

   while (hwnd && ((hwndT = GetParent(hwnd)) != hwndMDIClient))
      hwnd = hwndT;

   return hwnd;
}



VOID
UpdateSelection(HWND hwndLB)
{
   INT iMac, i;
   RECT rc;
   LPINT lpSelItems;

   iMac = (INT) SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);
   lpSelItems = (LPINT) LocalAlloc(LMEM_FIXED, sizeof(INT) * iMac);

   if (lpSelItems == NULL)
      return;

   i = (INT)SendMessage(hwndLB,
                        LB_GETSELITEMS,
                        (WPARAM)iMac,
                        (LPARAM)lpSelItems);

   for (i=0; i < iMac; i++) {

      SendMessage(hwndLB, LB_GETITEMRECT, lpSelItems[i], (LPARAM)&rc);
      InvalidateRect(hwndLB, &rc, TRUE);
   }

   LocalFree((HLOCAL)lpSelItems);
}


VOID
SortDirList(
   HWND hwndDir,
   LPXDTALINK lpStart,
   DWORD count,
   LPXDTA* lplpxdta)
{
   INT i, j;
   DWORD dwSort;
   INT iMax, iMin, iMid;
   LPXDTA lpxdta;

   dwSort = GetWindowLongPtr((HWND)GetWindowLongPtr(hwndDir,
                                                   GWL_LISTPARMS),
                               GWL_SORT);

   lpxdta = MemFirst(lpStart);

   lplpxdta[0] = lpxdta;

   for (i = 1; i < (INT)count; i++) {

      //
      // advance to next
      //
      lpxdta = MemNext(&lpStart, lpxdta);

      //
      // Quick hack for NTFS/HPFS
      //
      // Since they sort already (excepting strange chars/localization)
      // do a quick check if it goes at the end of the list (name sort only).
      //

      if (IDD_NAME == dwSort && CompareDTA(lpxdta, lplpxdta[i-1], IDD_NAME) >= 0) {

         lplpxdta[i] = lpxdta;
         continue;
      }

      //
      // do a binary insert
      //
      iMin = 0;
      iMax = i-1;       // last index

      do {
         iMid = (iMax + iMin) / 2;
         if (CompareDTA(lpxdta, lplpxdta[iMid], dwSort) > 0)
            iMin = iMid + 1;
         else
            iMax = iMid - 1;

      } while (iMax > iMin);

      if (iMax < 0)
         iMax = 0;

      //
      // insert after this one
      //
      if (CompareDTA(lpxdta, lplpxdta[iMax], dwSort) > 0)
         iMax++;

      if (i != iMax) {
         for (j = i; j > iMax; j--)
            lplpxdta[j] = lplpxdta[j-1];
      }
      lplpxdta[iMax] = lpxdta;
   }
}



// Set the focus to whoever deserves it if not the directory window.
// Return whether focus needs to be set to directory window.

BOOL
SetDirFocus(HWND hwndDir)
{
   HWND hwndLB = GetDlgItem(hwndDir, IDCW_LISTBOX);

   HWND hwndFocus;
   HWND hwndTree;
   HWND hwndParent = GetParent(hwndDir);

   //
   // We want to bounce the focus if there is an error case in
   // the directory side (No files\access denied).
   //
   if (GetWindowLongPtr(hwndDir, GWL_IERROR)) {

      GetTreeWindows(hwndParent, &hwndTree, NULL);

      //
      // don't move if no tree, no drives (BUGBUG?)
      //
      if (!bDriveBar)
         return TRUE;

      if ((hwndFocus = GetTreeFocus(hwndParent)) == hwndDir)
         SetFocus(hwndTree ? hwndTree : hwndDriveBar);
      else
         SetFocus(hwndFocus);

      return FALSE;
   }
   else
      return TRUE;
}

