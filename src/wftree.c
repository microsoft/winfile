/********************************************************************

   wftree.c

   Windows File System Tree Window Proc Routines

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"

#include <commctrl.h>

HICON
GetTreeIcon(HWND hWnd);

HICON
GetTreeIcon(HWND hWnd)
{
   HWND hwndTree, hwndDir;

   hwndTree = HasTreeWindow(hWnd);
   hwndDir = HasDirWindow(hWnd);

   if (hwndTree && hwndDir)
      return hicoTreeDir;
   else if (hwndTree)
      return hicoTree;
   else
      return hicoDir;
}



VOID
GetTreeWindows( HWND hwnd,
   PHWND phwndTree,
   PHWND phwndDir)
{
   if (phwndTree) {
      *phwndTree = GetDlgItem(hwnd, IDCW_TREECONTROL);
   }
   if (phwndDir) {
      *phwndDir  = GetDlgItem(hwnd, IDCW_DIR);
   }
}


// returns hwndTree, hwndDir or hwndDrives depending on the focus tracking
// for the window.  if none is found we return NULL

HWND
GetTreeFocus(HWND hwndTree)
{
   HWND hwnd, hwndLast = NULL;

   if (bDriveBar && GetFocus() == hwndDriveBar)
      return hwndDriveBar;

   hwndLast = hwnd = (HWND)GetWindowLongPtr(hwndTree, GWL_LASTFOCUS);

   while (hwnd && hwnd != hwndTree) {
      hwndLast = hwnd;
      hwnd = GetParent(hwnd);
   }

   return hwndLast;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CompactPath() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
CompactPath(HDC hDC, LPTSTR lpszPath, DWORD dx)
{
   register INT  len;
   SIZE          sizeF, sizeT;
   LPTSTR        lpEnd;          /* end of the unfixed string */
   LPTSTR        lpFixed;        /* start of text that we always display */
   BOOL          bEllipsesIn;
   TCHAR         szTemp[MAXPATHLEN];
   DWORD         dxEllipses;

   //
   // Does it already fit?
   //
   GetTextExtentPoint32(hDC, lpszPath, lstrlen(lpszPath), &sizeF);
   if (sizeF.cx <= (INT)dx)
      return(TRUE);

   //
   // Search backwards for the '\', and man, it better be there!
   //
   lpFixed = lpszPath + lstrlen(lpszPath) - 1;
   while (*lpFixed != CHAR_BACKSLASH)
      lpFixed--;

   // Save this guy to prevent overlap.
   lstrcpy(szTemp, lpFixed);

   lpEnd = lpFixed;
   bEllipsesIn = FALSE;

   GetTextExtentPoint32(hDC, szEllipses, 3, &sizeF);
   dxEllipses = sizeF.cx;

   GetTextExtentPoint32(hDC, lpFixed, lstrlen(lpFixed), &sizeF);

   while (TRUE) {
      GetTextExtentPoint32(hDC, lpszPath, lpEnd - lpszPath, &sizeT);
      len = sizeF.cx + sizeT.cx;

      if (bEllipsesIn)
         len += dxEllipses;

      if (len <= (INT)dx)
         break;

      bEllipsesIn = TRUE;

      if (lpEnd <= lpszPath) {

         // Things didn't fit.

         lstrcpy(lpszPath, szEllipses);
         lstrcat(lpszPath, szTemp);
         return(FALSE);
      }

      // Step back a character.
      --lpEnd;
   }

   if (bEllipsesIn) {
      lstrcpy(lpEnd, szEllipses);
      lstrcat(lpEnd, szTemp);
   }

   return(TRUE);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ResizeSplit
//
// Synopsis: Creates/resizes kids of MDI child for give path or
//           resizes (perhaps creating/destroying) kids based on dxSplit
//
// IN        hwnd    Window to change
// IN        dxSplit
//
//
// Return:   TRUE = success
//           FALSE = fail (out of mem, can't create win, unresizable tree)
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
ResizeSplit(HWND hwnd, INT dxSplit)
{
   RECT rc;
   HWND hwndTree, hwndDir, hwndLB;

   GetTreeWindows(hwnd, &hwndTree, &hwndDir);

   if (hwndTree && GetWindowLongPtr(hwndTree, GWL_READLEVEL))
      return FALSE;

   GetClientRect(hwnd, &rc);

   if (dxSplit > dxDriveBitmap * 2) {

      if (!hwndTree) {        // make new tree window

         hwndTree = CreateWindowEx(0L,
                                   szTreeControlClass,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                                   0, 0, 0, 0,
                                   hwnd,
                                   (HMENU)IDCW_TREECONTROL,
                                   hAppInstance, NULL);

         if (!hwndTree)
            return FALSE;

         //
         // only reset this if the dir window already
         // exists, that is we are creating the tree
         // by splitting open a dir window
         //
         if (hwndDir)
            SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(FALSE, 0), 0L);
      }
   } else {

      //
      // In this conditional, always set dxSplit=0.
      //
      if (hwndTree) {          // we are closing the tree window

         //
         // If the directory window is empty, then set the focus to the
         // drives window.
         //
         if (hwndDir) {
            hwndLB = GetDlgItem (hwndDir,IDCW_LISTBOX);
            if (hwndLB) {
               PVOID pv;
               SendMessage (hwndLB,LB_GETTEXT,0,(LPARAM)(LPTSTR) &pv);
               if (!pv)
                  SetFocus(hwndDriveBar);
            }
         }
         DestroyWindow(hwndTree);
      }
      dxSplit = 0;
   }

   if ((rc.right - dxSplit) > dxDriveBitmap * 2) {

      if (!hwndDir) {
         hwndDir = CreateWindowEx(0L,
                                  szDirClass,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
                                  0, 0, 0, 0,
                                  hwnd,
                                  (HMENU)IDCW_DIR,
                                  hAppInstance,
                                  NULL);
         if (!hwndDir)
            return FALSE;

      } else {

         //
         // Must invalidate: if viewing extreme left, paint residue left.
         //
         InvalidateRect(hwndDir, NULL, TRUE);
      }
   } else {

      //
      // Again, always set dxSplit
      //
      if (hwndDir) {
         DestroyWindow(hwndDir);
      }
      dxSplit = rc.right;
   }


   SetWindowLongPtr(hwnd, GWL_SPLIT, dxSplit);

   SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)GetTreeIcon(hwnd));

   UpdateStatus(hwnd);
   EnableCheckTBButtons(hwnd);

   return TRUE;
}


VOID
SwitchDriveSelection(HWND hwndChild, BOOL bSelectToolbarDrive)
{
   DRIVE drive;
   DRIVEIND i, driveIndOld, driveIndOldFocus;
   RECT rc;

   drive = GetWindowLongPtr(hwndChild, GWL_TYPE);

   if (TYPE_SEARCH == drive) {

      drive = (DRIVE)SendMessage(hwndSearch, FS_GETDRIVE, 0, 0L) - CHAR_A;
   }


   driveIndOld      = GetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEIND);
   driveIndOldFocus = GetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEFOCUS);

   for (i=0; i < cDrives; i++) {
      if (rgiDrive[i] == drive) {
         SetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEIND, i);
         SetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEFOCUS, i);

         break;
      }
   }

   if (i == cDrives)
      return;     // didn't find drive;  it must have been invalidated
                  // other code (such as tree walk) will handle

   if (bDriveBar) {

      //
      // Optimization: don't invalidate whole drive bar, just the
      // two drives that change.
      //
      // was: InvalidateRect(hwndDriveBar, NULL, TRUE);
      //

      GetDriveRect(i, &rc);
      InvalidateRect(hwndDriveBar, &rc, TRUE);

      GetDriveRect(driveIndOld, &rc);
      InvalidateRect(hwndDriveBar, &rc, TRUE);

      GetDriveRect(driveIndOldFocus, &rc);
      InvalidateRect(hwndDriveBar, &rc, TRUE);

      UpdateWindow(hwndDriveBar);
   }

   // made optional to prevent extra refreshing.
   if (bSelectToolbarDrive)
      SelectToolbarDrive(i);
}


//--------------------------------------------------------------------------*/
//                                                                          */
//  TreeWndProc() -                                                         */
//                                                                          */
//--------------------------------------------------------------------------*/

// WndProc for the MDI child window containing the drives, volume, and
// directory tree child windows.

LRESULT
TreeWndProc(
   register HWND hwnd,
   UINT uMsg,
   register WPARAM wParam,
   LPARAM lParam)
{
   HWND hwndTree, hwndDir, hwndFocus;
   TCHAR szDir[2*MAXPATHLEN];

   RECT rc;
   HDC hdc;

   switch (uMsg) {

   case WM_MENUSELECT:

      if (GET_WM_MENUSELECT_HMENU(wParam, lParam)) {

         //
         // Save the menu the user selected
         //
         uMenuID = GET_WM_MENUSELECT_CMD(wParam, lParam);
         uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
         hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);

         bMDIFrameSysMenu = FALSE;
      }

      MenuHelp((WORD)uMsg, wParam, lParam, GetMenu(hwndFrame),
         hAppInstance, hwndStatus, (LPDWORD)dwMenuIDs);
      break;

   case WM_FSC:

      if (hwndDir = HasDirWindow(hwnd))
         SendMessage(hwndDir, uMsg, wParam, lParam);

      break;

   case FS_NOTIFYRESUME:

      if (hwndDir = HasDirWindow(hwnd)) {
         SendMessage(hwnd, FS_GETDIRECTORY, COUNTOF(szDir), (LPARAM)szDir);

         ModifyWatchList(hwnd, szDir, FILE_NOTIFY_CHANGE_FLAGS);
      }
      break;

   case FS_CHANGEDRIVES:
      {
         GetMDIWindowText(hwnd, szDir, COUNTOF(szDir));
         SetMDIWindowText(hwnd, szDir);  /* refresh label in title */
         RedoDriveWindows((HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L));

         break;
      }

      case FS_GETSELECTION:
        {
#define pfDir            (BOOL *)lParam
          LPTSTR p;

          GetTreeWindows(hwnd, &hwndTree, &hwndDir);
          hwndFocus = GetTreeFocus(hwnd);

          if (hwndFocus == hwndDir || !hwndTree) {
              return SendMessage(hwndDir, FS_GETSELECTION, wParam, lParam);
          } else {

             //
             // +2 for checkesc safety
             //
             p = (LPTSTR)LocalAlloc(LPTR, ByteCountOf(MAXPATHLEN + 2));
             if (p) {
                SendMessage(hwnd, FS_GETDIRECTORY, MAXPATHLEN, (LPARAM)(LPTSTR)p);
                StripBackslash(p);

                if (!(wParam & 16)) CheckEsc(p);

                if (wParam == 2) {      // BUG ??? wParam should be fMostRecentOnly
                   if (pfDir) {
                      *pfDir = IsLFN(p);
                   }
                   LocalFree((HANDLE)p);
                   return (LRESULT)p;
                }
             }
             if (pfDir) {
                *pfDir = TRUE;
             }
             return (LRESULT)p;
          }
#undef pfDir
        }

      case FS_GETDIRECTORY:

          // wParam is the length of the string pointed to by lParam
          // returns in lParam ANSI directory string with
          // a trailing backslash.  if you want to do a SetCurrentDirector()
          // you must first StripBackslash() the thing!

          GetMDIWindowText(hwnd, (LPTSTR)lParam, (INT)wParam);        // get the string
          StripFilespec((LPTSTR)lParam);        // Remove the trailing extension
          AddBackslash((LPTSTR)lParam);        // terminate with a backslash
          break;


      case FS_GETFILESPEC:

          // WARNING: Requires ((LPTSTR)lparam)[MAXPATHLEN] space!
          // (TreeControlWndProc WM_COMMAND LBN_SELCHANGE broke this!)

          // returns the current filespec (from View.Include...).  this is
          // an uppercase ANSI string

          GetMDIWindowText(hwnd, (LPTSTR)lParam, (INT)wParam);
          StripPath((LPTSTR)lParam);
          break;

          // redirect these messages to the drive icons to get the same
          // result as dropping on the active drive.
          // this is especially useful when we are minimized

      case WM_DRAGSELECT:
      case WM_QUERYDROPOBJECT:
      case WM_DROPOBJECT:

         // This code is really messed up since there is only one drive bar.
         // Fix the case for Iconic windows but the uniconic state is weird

#define lpds   ((LPDROPSTRUCT)lParam)

         if (IsIconic(hwnd) || hwnd != (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L)) {
            if (hwndDir = HasDirWindow(hwnd)) {
               lpds->dwControlData = (DWORD) -1;
               return SendMessage(hwndDir, uMsg, wParam, lParam);
            } else {
               break;
            }
         }
#undef lpds

         if (hwndDriveBar) {

            LRESULT lRet;

            hwndDropChild = hwnd;
            lRet = SendMessage(hwndDriveBar, uMsg, wParam, lParam);
            hwndDropChild = NULL;
            return lRet;
         }

         if (hwndDir = HasDirWindow(hwnd)) {
            return SendMessage(hwndDir, uMsg, wParam, lParam);
         }

         break;

      case FS_GETDRIVE:

          GetTreeWindows(hwnd, &hwndTree, &hwndDir);

          if (hwndTree)
             return SendMessage(hwndTree, uMsg, wParam, lParam);
          else
             return SendMessage(hwndDir, uMsg, wParam, lParam);

          break;

      case WM_CREATE:
         {
#define lpcs ((LPCREATESTRUCT)lParam)
#define lpmdics ((LPMDICREATESTRUCT)(lpcs->lpCreateParams))

            INT dxSplit;
            DRIVE drive;
            WCHAR szPath[2 * MAXFILENAMELEN];

            //
            // lpcs->lpszName is the path we are opening the
            // window for (has extension stuff "*.*")
            //
            drive = DRIVEID(lpcs->lpszName);

            SetWindowLongPtr(hwnd, GWL_TYPE, drive);

            //
            // if dxSplit is negative we split in the middle
            //
            dxSplit = (INT)lpmdics->lParam;

            //
            // if dxSplit is negative we split in the middle
            //
            if (dxSplit < 0)
               dxSplit = lpcs->cx / 2;

            SetWindowLongPtr(hwnd, GWL_NOTIFYPAUSE, 0L);
            SetWindowLongPtr(hwnd, GWL_SPLIT, dxSplit);
            SetWindowLongPtr(hwnd, GWL_LASTFOCUS, 0L);
            SetWindowLongPtr(hwnd, GWL_FSCFLAG, FALSE);

            SetWindowLongPtr(hwnd, GWL_VOLNAME, 0L);
            SetWindowLongPtr(hwnd, GWL_PATHLEN, 0L);

            if (!ResizeSplit(hwnd, dxSplit))
               return -1;

            GetTreeWindows(hwnd, &hwndTree, &hwndDir);
            SetWindowLongPtr(hwnd, GWL_LASTFOCUS, (LPARAM)(hwndTree ? hwndTree : hwndDir));

            iNumWindows++;

            GetMDIWindowText(hwnd, szPath, COUNTOF(szPath));
            SetMDIWindowText(hwnd, szPath);

            break;
#undef lpcs
#undef lpmdics
         }

      case WM_CLOSE:

         if (hwndTree = HasTreeWindow(hwnd)) {

            //
            // don't close if we are reading the tree
            //
            if (GetWindowLongPtr(hwndTree, GWL_READLEVEL))
               break;
         }

         //
         // don't allow the last MDI child to be closed!
         //

         if (!IsLastWindow()) {
            iNumWindows--;

            //
            // don't leave current dir on floppies
            //
            GetSystemDirectory(szDir, COUNTOF(szDir));
            SetCurrentDirectory(szDir);

            goto DEF_MDI_PROC;      // this will close this window
         }

         break;

      case WM_DESTROY:
      {
         LPWSTR lpszVolName;

         //
         // Free GWL_VOLNAME
         //
         lpszVolName = (LPTSTR)GetWindowLongPtr(hwnd, GWL_VOLNAME);

         if (lpszVolName)
            LocalFree(lpszVolName);

         break;
      }
      case WM_MDIACTIVATE:

         //
         // Invalidate the cache since we are switching windows
         //
         ExtSelItemsInvalidate();

         //
         // we are receiving the activation
         //
         if (GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wParam, lParam)) {

            EnableCheckTBButtons(hwnd);

            SPC_SET_INVALID(qFreeSpace);

            UpdateStatus(hwnd);

            hwndFocus = (HWND)GetWindowLongPtr(hwnd, GWL_LASTFOCUS);
            SetFocus(hwndFocus);

            SwitchDriveSelection(hwnd, TRUE);
         }
         else if (hwndDriveBar)
            SendMessage(hwndDriveBar, uMsg, wParam, lParam);
         break;

      case WM_SETFOCUS:

          hwndFocus = (HWND)GetWindowLongPtr(hwnd, GWL_LASTFOCUS);
          SetFocus(hwndFocus);
          break;

      case WM_INITMENUPOPUP:
          if (HIWORD(lParam)) {
             EnableMenuItem((HMENU)wParam, SC_CLOSE,
               IsLastWindow() ? MF_BYCOMMAND | MF_DISABLED | MF_GRAYED :
               MF_ENABLED);
          }
          break;

      case WM_SYSCOMMAND:

          if (wParam != SC_SPLIT)
                goto DEF_MDI_PROC;

          GetClientRect(hwnd, &rc);

          lParam = MAKELONG(rc.right / 2, 0);

          // fall through

      case WM_LBUTTONDOWN:
          {
          MSG msg;
          INT x, y, dx, dy;

          if (IsIconic(hwnd))
             break;

          y = 0;

          x = LOWORD(lParam);

          GetClientRect(hwnd, &rc);

          dx = 4;
          dy = rc.bottom - y;   // the height of the client less the drives window

          hdc = GetDC(hwnd);

          // split bar loop

          PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);

          SetCapture(hwnd);

          while (GetMessage(&msg, NULL, 0, 0)) {

             if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN ||
                (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)) {

                if (msg.message == WM_LBUTTONUP || msg.message == WM_LBUTTONDOWN)
                   break;

                if (msg.message == WM_KEYDOWN) {

                   if (msg.wParam == VK_LEFT) {
                      msg.message = WM_MOUSEMOVE;
                      msg.pt.x -= 2;
                   } else if (msg.wParam == VK_RIGHT) {
                      msg.message = WM_MOUSEMOVE;
                      msg.pt.x += 2;
                   } else if (msg.wParam == VK_RETURN ||
                   msg.wParam == VK_ESCAPE) {
                      break;
                   }

                   SetCursorPos(msg.pt.x, msg.pt.y);
                }

                if (msg.message == WM_MOUSEMOVE) {

                   // erase old

                   PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                   ScreenToClient(hwnd, &msg.pt);
                   x = msg.pt.x;

                   // put down new

                   PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                }
             } else {
                DispatchMessage(&msg);
             }
          }
          ReleaseCapture();

          // erase old

          PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
          ReleaseDC(hwnd, hdc);

          if (msg.wParam != VK_ESCAPE) {

             if (ResizeSplit(hwnd, x))
                SendMessage(hwnd, WM_SIZE, SIZENOMDICRAP, MAKELONG(rc.right, rc.bottom));
          }

          break;
          }

      case WM_QUERYDRAGICON:
          return (LRESULT)GetTreeIcon(hwnd);
          break;

      case WM_ERASEBKGND:

         if (IsIconic(hwnd)) {
            // this paints the background of the icon properly, doing
            // brush alignment and other nasty stuff

            DefWindowProc(hwnd, WM_ICONERASEBKGND, wParam, 0L);
         } else {
            //
            // Don't bother since we will be redrawing everything
            // over it anyway.
            //
         }
         break;

      case WM_PAINT:
          {
             PAINTSTRUCT ps;
             HBRUSH hbr;

             hdc = BeginPaint(hwnd, &ps);

             if (IsIconic(hwnd)) {
                DrawIcon(hdc, 0, 0, GetTreeIcon(hwnd));
             } else {
                GetClientRect(hwnd, &rc);
                rc.left = GetSplit(hwnd);

                if (rc.left >= rc.right)
                        rc.left = 0;

                rc.right = rc.left + dxFrame;

                // draw the black pane handle

                rc.top = rc.bottom - GetSystemMetrics(SM_CYHSCROLL);
                FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));

                if (hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW))) {

                   rc.bottom = rc.top;
                   rc.top = 0;

                   FillRect(hdc, &rc, hbr);

                   DeleteObject(hbr);
                }
             }
             EndPaint(hwnd, &ps);
             break;
         }


      case WM_SIZE:

        if (wParam != SIZEICONIC) {

           //
           // Must invalidate rect to clear up the hwndDir display
           // when the view is single column (partial/full details)
           // and it's scrolled to the extreme left.  (Either resizing
           // larger or maximizing)
           //
           // To avoid nasty double flicker, we turn off the redraw
           // flag for the list box (hwndDir is set to IDCW_LISTBOX).
           // Then we redraw then repaint.
           //
           if (hwndDir = HasDirWindow(hwnd)) {

              hwndDir = GetDlgItem(hwndDir, IDCW_LISTBOX);
              SendMessage(hwndDir, WM_SETREDRAW, FALSE, 0L);
           }

           ResizeWindows(hwnd,LOWORD(lParam),HIWORD(lParam));

           //
           // When hwndDir is non-NULL, it holds the IDCW_LISTBOX set
           // above.  Now set redraw true and repaint.
           //
           if (hwndDir) {

              SendMessage(hwndDir, WM_SETREDRAW, TRUE, 0L);

              InvalidateRect(hwndDir, NULL, TRUE);
              UpdateWindow(hwndDir);
           }

        } else if (IsLastWindow()) {

           //
           // A bug in MDI sometimes causes us to get the input focus
           // even though we're minimized.  Prevent that...
           //
           SetFocus(GetParent(hwnd));
        }

        //
        // if wParam is SIZENOMDICRAP this WM_SIZE was generated by us.
        // don't let this through to the DefMDIChildProc().
        // that might change the min/max state, (show parameter)
        //

        if (wParam == SIZENOMDICRAP)
           break;

        //
        // FALL THROUGH
        //

      default:

DEF_MDI_PROC:

          return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
    }

  return 0L;
}


VOID
ResizeWindows(HWND hwndParent,INT dxWindow, INT dyWindow)
{
   INT y, dy, split;

   HWND hwndTree, hwndDir;
   RECT rc;

   GetTreeWindows(hwndParent, &hwndTree, &hwndDir);

   y = -dyBorder;

   split = GetSplit(hwndParent);

   //
   // user has been fixed to do this right
   //
   dy = dyWindow - y + dyBorder;

   if (hwndTree) {
      if (!hwndDir)
         MoveWindow(hwndTree, dxFrame, y, dxWindow - dxFrame + dyBorder, dy, TRUE);
      else
         MoveWindow(hwndTree, -dyBorder, y, split + dyBorder, dy, TRUE);
   }

   if (hwndDir) {
      if (!hwndTree)
         MoveWindow(hwndDir, dxFrame, y, dxWindow - dxFrame + dyBorder, dy, TRUE);
      else
         MoveWindow(hwndDir, split + dxFrame, y,
      dxWindow - split - dxFrame + dyBorder, dy, TRUE);
   }

   //
   // since this window is not CS_HREDRAW | CS_VREDRAW we need to force
   // the repaint of the split
   //
   rc.top = y;
   rc.bottom = y + dy;

   //
   // yutakas
   // When split moved, Tree Window couldn't redraw.
   //
   if ((hwndTree) && (!(rc.left = GetScrollPos(GetDlgItem(hwndTree,
                                                          IDCW_TREELISTBOX),
                                               SB_HORZ) ?
                                               0 :
                                               split))) {

       rc.right = split;
       InvalidateRect(hwndParent, &rc, FALSE);
   }

   rc.left = split;
   rc.right = split + dxFrame;

   InvalidateRect(hwndParent, &rc, TRUE);
   UpdateWindow(hwndParent);
}

