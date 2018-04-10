/********************************************************************

   wfdrives.c

   window procs and other stuff for the drive bar

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#define PUBLIC           // avoid collision with shell.h
#include "winfile.h"
#include "treectl.h"
#include "lfn.h"
#include "wfcopy.h"
#include <commctrl.h>
#include <shlobj.h>

VOID RectDrive(INT nDrive, BOOL bFocusOn);
VOID InvalidateDrive(DRIVEIND driveInd);
INT  DriveFromPoint(HWND hwnd, POINT pt);
VOID DrawDrive(HDC hdc, INT x, INT y, DRIVEIND driveInd, BOOL bCurrent, BOOL bFocus);
INT  KeyToItem(HWND hWnd, WORD nDriveLetter);


/////////////////////////////////////////////////////////////////////
//
// Name:     NewTree
//
// Synopsis: Creates new split tree for given drive.  Inherits all
//           properties of current window.
//
// IN        drive    Drive number to create window for
// IN        hwndSrc  Base properties on this window
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
NewTree(
   DRIVE drive,
   HWND hwndSrc)
{
   HWND hwnd, hwndTree, hwndDir;
   WCHAR szDir[MAXPATHLEN * 2];
   INT dxSplit;
   BOOL bDir;
   LPWSTR pszSearchDir = NULL;
   LPWSTR psz;

   //
   // make sure the floppy/net drive is still valid
   //
   if (!CheckDrive(hwndSrc, drive, FUNC_SETDRIVE))
      return;

   pszSearchDir = (LPWSTR)SendMessage(hwndSrc,
	   FS_GETSELECTION,
	   1 | 4 | 16,
	   (LPARAM)&bDir);

   //
   // If no selection
   //
   if (!pszSearchDir || !pszSearchDir[0] || DRIVEID(pszSearchDir) != drive) {

	   //
	   // Update net con in case remote drive was swapped from console
	   //
	   if (IsRemoteDrive(drive)) {
		   R_NetCon(drive);
	   }

	   //
	   // Update volume label here too if removable
	   // This is broken since we may steal stale data from another window.
	   // But this is what winball does and there's no simpler solution.
	   //
	   if (IsRemovableDrive(drive)) {
		   R_VolInfo(drive);
	   }

	   // TODO: if directory in right pane, get that instead of directory in left pane

	   GetSelectedDirectory(drive + 1, szDir);
	   AddBackslash(szDir);
	   SendMessage(hwndSrc,
		   FS_GETFILESPEC,
		   MAXPATHLEN,
		   (LPARAM)(szDir + lstrlen(szDir)));
   }
   else {

	   lstrcpy(szDir, pszSearchDir);

	   if (!bDir) {

		   RemoveLast(szDir);

		   //
		   // pszInitialSel is a global used to pass initial state:
		   // currently selected item in the dir part of the window
		   //
		   // Freed by caller
		   //
		   psz = pszSearchDir + lstrlen(szDir) + 1;

		   pszInitialDirSel = (LPWSTR)LocalAlloc(LMEM_FIXED,
			   lstrlen(psz) * sizeof(*psz));
		   if (pszInitialDirSel)
			   lstrcpy(pszInitialDirSel, psz);
	   }

	   AddBackslash(szDir);
	   lstrcat(szDir, szStarDotStar);
   }

   if (hwndSrc == hwndSearch) {
      dxSplit = -1;
   }
   else {

	   hwndTree = HasTreeWindow(hwndSrc);
	   hwndDir = HasDirWindow(hwndSrc);

	   if (hwndTree && hwndDir)
		   dxSplit = GetSplit(hwndSrc);
	   else if (hwndDir)
		   dxSplit = 0;
	   else
		   dxSplit = 10000;
   }

   //
   // take all the attributes from the current window
   // (except the filespec, we may want to change this)
   //
   dwNewSort    = GetWindowLongPtr(hwndSrc, GWL_SORT);
   dwNewView    = GetWindowLongPtr(hwndSrc, GWL_VIEW);
   dwNewAttribs = GetWindowLongPtr(hwndSrc, GWL_ATTRIBS);

   hwnd = CreateTreeWindow(szDir, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, dxSplit);

   if (hwnd && (hwndTree = HasTreeWindow(hwnd)))
      SendMessage(hwndTree,
                  TC_SETDRIVE,
                  MAKELONG(MAKEWORD(FALSE, 0), TRUE),
                  0L);

   //
   // Cleanup
   //
   if (pszSearchDir)
      LocalFree((HLOCAL)pszSearchDir);
}

//
// assumes drive bar is visible.
//

VOID
GetDriveRect(DRIVEIND driveInd, PRECT prc)
{
   RECT rc;
   INT nDrivesPerRow;

   GetClientRect(hwndDriveBar, &rc);

   if (!dxDrive)           // avoid div by zero
      dxDrive++;

   nDrivesPerRow = rc.right / dxDrive;

   if (!nDrivesPerRow)     // avoid div by zero
      nDrivesPerRow++;

   prc->top = dyDrive * (driveInd / nDrivesPerRow);
   prc->bottom = prc->top + dyDrive;

   prc->left = dxDrive * (driveInd % nDrivesPerRow);
   prc->right = prc->left + dxDrive;
}

INT
DriveFromPoint(HWND hwnd, POINT pt)
{
   RECT rc, rcDrive;
   INT x, y;

   DRIVEIND driveInd;

   if (!bDriveBar || hwnd != hwndDriveBar)
      return -1;

   GetClientRect(hwndDriveBar, &rc);

   x = 0;
   y = 0;
   driveInd = 0;

   for (driveInd = 0; driveInd < cDrives; driveInd++) {
      rcDrive.left = x;
      rcDrive.right = x + dxDrive;
      rcDrive.top = y;
      rcDrive.bottom = y + dyDrive;
      InflateRect(&rcDrive, -dyBorder, -dyBorder);

      if (PtInRect(&rcDrive, pt))
         return driveInd;

      x += dxDrive;

      if (x + dxDrive > rc.right) {
         x = 0;
         y += dyDrive;
      }
   }

   return -1;      // no hit
}

VOID
InvalidateDrive(DRIVEIND driveInd)
{
   RECT rc;

   //
   // Get out early if the drivebar is not visible.
   //
   if (!bDriveBar)
      return;

   GetDriveRect(driveInd, &rc);
   InvalidateRect(hwndDriveBar, &rc, TRUE);
}


//
// void NEAR PASCAL RectDrive(DRIVEIND driveInd, BOOL bDraw)
//
// draw the highlight rect around the drive to indicate that it is
// the target of a drop action.
//
// in:
//      nDrive  the drive to draw the rect around
//      bDraw   if TRUE, draw a rect around this drive
//              FALSE, erase the rect (draw the default rect)
//

VOID
RectDrive(DRIVEIND driveInd, BOOL bDraw)
{
   RECT rc, rcDrive;
   HBRUSH hBrush;
   HDC hdc;

   GetDriveRect(driveInd, &rc);
   rcDrive = rc;
   InflateRect(&rc, -dyBorder, -dyBorder);

   if (bDraw) {

      hdc = GetDC(hwndDriveBar);

      if (hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))) {
         FrameRect(hdc, &rc, hBrush);
         DeleteObject(hBrush);
      }

      ReleaseDC(hwndDriveBar, hdc);

   } else {
      InvalidateRect(hwndDriveBar, &rcDrive, TRUE);
      UpdateWindow(hwndDriveBar);
   }
}

//
// void DrawDrive(HDC hdc, int x, int y, int nDrive, BOOL bCurrent, BOOL bFocus)
//
// paint the drive icons in the standard state, given the
// drive with the focus and the current selection
//
// in:
//      hdc             dc to draw to
//      x, y            position to start (dxDrive, dyDrive are the extents)
//      nDrive          the drive to paint
//      bCurrent        draw as the current drive (pushed in)
//      bFocus          draw with the focus
//

VOID
DrawDrive(HDC hdc, INT x, INT y, DRIVEIND driveInd, BOOL bCurrent, BOOL bFocus)
{
   RECT rc;
   TCHAR szTemp[2];
   DWORD rgb;
   DRIVE drive;

   drive = rgiDrive[driveInd];

   rc.left = x;
   rc.right = x + dxDrive;
   rc.top = y;
   rc.bottom = y + dyDrive;

   rgb = GetSysColor(COLOR_BTNTEXT);

   if (bCurrent) {
      HBRUSH hbr;

      if (hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT))) {
         if (bFocus) {
            rgb = GetSysColor(COLOR_HIGHLIGHTTEXT);
            FillRect(hdc, &rc, hbr);
         } else {
            InflateRect(&rc, -dyBorder, -dyBorder);
            FrameRect(hdc, &rc, hbr);
         }
         DeleteObject(hbr);
      }
   }

   if (bFocus)
      DrawFocusRect(hdc, &rc);

   szTemp[0] = (TCHAR)(chFirstDrive + rgiDrive[driveInd]);
   SetBkMode(hdc, TRANSPARENT);

   rgb = SetTextColor(hdc, rgb);
   TextOut(hdc, x + dxDriveBitmap+(dyBorder*6), y + (dyDrive - dyText) / 2, szTemp, 1);
   SetTextColor(hdc, rgb);

   BitBlt(hdc, x + 4*dyBorder, y + (dyDrive - dyDriveBitmap) / 2, dxDriveBitmap, dyDriveBitmap,
      hdcMem, aDriveInfo[drive].iOffset, 2 * dyFolder, SRCCOPY);
}


// check net/floppy drives for validity, sets the net drive bitmap
// when the thing is not available
//
// note: IsTheDiskReallyThere() has the side effect of setting the
// current drive to the new disk if it is successful

BOOL
CheckDrive(HWND hwnd, DRIVE drive, DWORD dwFunc)
{
   DWORD err;
   DRIVEIND driveInd;
   HCURSOR hCursor;
   WCHAR szDrive[] = SZ_ACOLON;

   // Put up the hourglass cursor since this
   // could take a long time

   hCursor = LoadCursor(NULL, IDC_WAIT);

   if (hCursor)
      hCursor = SetCursor(hCursor);
   ShowCursor(TRUE);

   DRIVESET(szDrive,drive);

   // find index for this drive

   driveInd = 0;
   while ((driveInd < cDrives) && (rgiDrive[driveInd] != drive))
       driveInd++;

   switch (IsNetDrive(drive)) {

   case 2:

      R_Type(drive);

      if (IsValidDisk(drive)) {

         R_NetCon(drive);

      } else {

         //
         // Not valid, so change it back to a remote connection
         //
         aDriveInfo[drive].uType = DRIVE_REMOTE;

         //
         // Wait for background wait net
         //
         WAITNET();

		 if (lpfnWNetRestoreSingleConnectionW != NULL)
		 	err = WNetRestoreSingleConnectionW(hwnd, szDrive, TRUE);
		 else
	        err = WNetRestoreConnectionW(hwnd, szDrive);

         if (err != WN_SUCCESS) {

            aDriveInfo[drive].iOffset = dxDriveBitmap * 5;
            InvalidateDrive(driveInd);

            if (hCursor)
               SetCursor(hCursor);
            ShowCursor(FALSE);

            return FALSE;
         }

         //
         // The safest thing to do is U_NetCon, since we've
         // just re-established our connection.  For quickness,
         // however, we'll just reset the return value of U_NetCon
         // to indicate that we're reconnected (and C_ close it).
         //
         // This is also safe since WNetRestoreConnection returns no
         // error and the drive name should not change since we last
         // read it.  -- UNLESS the user creates a new disconnected
         // remembered drive over the old one.  Difficult to do without
         // hitting the registry directly.
         //

         C_NetCon(drive, ERROR_SUCCESS);
      }

      aDriveInfo[drive].bRemembered = FALSE;

      // fall through...

   case 1:

      aDriveInfo[drive].iOffset = dxDriveBitmap * 4;
      InvalidateDrive(driveInd);
      break;

   default:
      break;
   }

   if (hCursor)
      SetCursor(hCursor);
   ShowCursor(FALSE);

   return IsTheDiskReallyThere(hwnd, szDrive, dwFunc, FALSE);
}


VOID
DrivesDropObject(HWND hWnd, LPDROPSTRUCT lpds)
{
    DRIVEIND driveInd;
    TCHAR szPath[MAXPATHLEN * 2];
    LPTSTR pFrom;
    BOOL bIconic;
    HWND hwndChild;

    hwndChild = hwndDropChild ? hwndDropChild :
    (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

    bIconic = IsIconic(hwndChild);

    if (bIconic) {
UseCurDir:
      SendMessage(hwndChild, FS_GETDIRECTORY, COUNTOF(szPath), (LPARAM)szPath);
    } else {

      driveInd = DriveFromPoint(lpds->hwndSink, lpds->ptDrop);

      if (driveInd < 0)
          goto UseCurDir;
      // this searches windows in the zorder then asks dos
      // if nothing is found...

      GetSelectedDirectory((WORD)(rgiDrive[driveInd] + 1), szPath);
    }
    AddBackslash(szPath);           // add spec part
    lstrcat(szPath, szStarDotStar);

    pFrom = (LPTSTR)lpds->dwData;

    CheckEsc(szPath);
    DMMoveCopyHelper(pFrom, szPath, fShowSourceBitmaps);

    if (!bIconic)
        RectDrive(driveInd, FALSE);
}


VOID
DrivesPaint(HWND hWnd, INT nDriveFocus, INT nDriveCurrent)
{
   RECT rc;
   INT nDrive;

   HDC hdc;
   PAINTSTRUCT ps;

   INT x, y;
   HANDLE hOld;
   INT cDriveRows, cDrivesPerRow;

   GetClientRect(hWnd, &rc);

   hdc = BeginPaint(hWnd, &ps);

   if (!rc.right) {

      EndPaint(hWnd, &ps);
      return;
   }

   hOld = SelectObject(hdc, hFont);

   cDrivesPerRow = rc.right / dxDrive;

   if (!cDrivesPerRow)
      cDrivesPerRow++;

   cDriveRows = ((cDrives-1) / cDrivesPerRow) + 1;

   x = 0;
   y = 0;
   for (nDrive = 0; nDrive < cDrives; nDrive++) {

      if (GetFocus() != hWnd)
         nDriveFocus = -1;

      DrawDrive(hdc, x, y, nDrive, nDriveCurrent == nDrive, nDriveFocus == nDrive);
      x += dxDrive;

      if (x + dxDrive > rc.right) {
         x = 0;
         y += dyDrive;
      }
   }

   if (hOld)
      SelectObject(hdc, hOld);

   EndPaint(hWnd, &ps);
}

//
// set the current window to a new drive
//

VOID
DrivesSetDrive(
   HWND hWnd,
   DRIVEIND driveInd,
   DRIVEIND driveIndCur,
   BOOL bDontSteal)
{
   WCHAR szPath[MAXPATHLEN * 2];

   HWND hwndChild;
   HWND hwndTree;
   HWND hwndDir;

   DRIVE drive;

   hwndChild = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   InvalidateRect(hWnd, NULL, TRUE);

   //
   // save the current directory on this drive for later so
   // we don't have to hit the drive to get the current directory
   // and other apps won't change this out from under us
   //

   GetSelectedDirectory(0, szPath);
   SaveDirectory(szPath);

   //
   // this also sets the current drive if successful
   //
   drive = rgiDrive[driveInd];

   I_NetCon(drive);
   I_VolInfo(drive);

   if (!CheckDrive(hWnd, drive, FUNC_SETDRIVE))
      return;

   //
   // cause current tree read to abort if already in progress
   //
   hwndTree = HasTreeWindow(hwndChild);


   if (hwndTree && GetWindowLongPtr(hwndTree, GWL_READLEVEL)) {

      //
      // bounce any clicks on a drive that is currently being read
      //

      if (driveInd != driveIndCur)
         bCancelTree = TRUE;
      return;
   }

   SelectToolbarDrive(driveInd);

   //
   // do again after in case a dialog cause the drive bar
   // to repaint
   //
   InvalidateRect(hWnd, NULL, TRUE);

   //
   // get this from our cache if possible
   //
   GetSelectedDirectory((drive + 1), szPath);

   //
   // set the drives window parameters and repaint
   //
   SetWindowLongPtr(hWnd, GWL_CURDRIVEIND, driveInd);
   SetWindowLongPtr(hWnd, GWL_CURDRIVEFOCUS, driveInd);

   // NOTE: similar to CreateDirWindow

   //
   // reset the dir first to allow tree to steal data
   // if szPath is not valid the TC_SETDRIVE will reinit
   // the files half (if there is no tree we have a problem)
   //
   if (hwndDir = HasDirWindow(hwndChild)) {

     AddBackslash(szPath);
     SendMessage(hwndDir, FS_GETFILESPEC, MAXFILENAMELEN, (LPARAM)(szPath + lstrlen(szPath)));

     SendMessage(hwndDir, FS_CHANGEDISPLAY,
        bDontSteal ? CD_PATH_FORCE | CD_DONTSTEAL : CD_PATH_FORCE,
        (LPARAM)szPath);

     StripFilespec(szPath);
   }

   //
   // do this before TC_SETDRIVE in case the tree read
   // is aborted and lFreeSpace gets set to -2L
   //
   // Was -1L, ignore new cache.
   //
   SPC_SET_HITDISK(qFreeSpace);   // force status info refresh

   //
   // tell the tree control to do it's thing
   //
   if (hwndTree) {
     SendMessage(hwndTree, TC_SETDRIVE,
        MAKEWORD(GetKeyState(VK_SHIFT) < 0, bDontSteal), (LPARAM)(szPath));
   } else {

      // at least resize things
      RECT rc;
      GetClientRect(hwndChild, &rc);
      ResizeWindows(hwndChild,(WORD)(rc.right+1),(WORD)(rc.bottom+1));
   }

   UpdateStatus(hwndChild);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DrivesWndProc() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LRESULT
DrivesWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  INT nDrive, nDriveCurrent, nDriveFocus;
  RECT rc;
  static INT nDriveDoubleClick = -1;
  static INT nDriveDragging = -1;
  HWND hwndChild;

  TCHAR szDir[MAXPATHLEN];

  hwndChild = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

  nDriveCurrent = GetWindowLongPtr(hWnd, GWL_CURDRIVEIND);
  nDriveFocus = GetWindowLongPtr(hWnd, GWL_CURDRIVEFOCUS);

  switch (wMsg) {
      case WM_CREATE:
          {
          INT i;

          // Find the current drive, set the drive bitmaps

          if (hwndChild == 0)
             nDrive = 0;
          else
             nDrive = GetWindowLongPtr(hwndChild, GWL_TYPE);




          for (i=0; i < cDrives; i++) {

              if (rgiDrive[i] == nDrive) {
                  SetWindowLongPtr(hWnd, GWL_CURDRIVEIND, i);
                  SetWindowLongPtr(hWnd, GWL_CURDRIVEFOCUS, i);
              }

          }
          break;
          }

      case WM_VKEYTOITEM:
          KeyToItem(hWnd, (WORD)wParam);
          return -2L;
          break;

      case WM_KEYDOWN:
          switch (wParam) {

          case VK_ESCAPE:
                bCancelTree = TRUE;
                break;

          case VK_F6:   // like excel
          case VK_TAB:
                {
                   HWND hwndTree, hwndDir, hwndSet, hwndNext;
                   BOOL bDir;
                   BOOL bChangeDisplay = FALSE;

                   GetTreeWindows(hwndChild, &hwndTree, &hwndDir);
                  
                   // Check to see if we can change to the directory window
                  
                   bDir = hwndDir != NULL;
                   if (bDir)
                   {
                      HWND hwndLB;

                      bChangeDisplay = GetWindowLongPtr(hwndDir, GWLP_USERDATA);

                      hwndLB = GetDlgItem (hwndDir, IDCW_LISTBOX);
                      if (hwndLB && !bChangeDisplay)
                      {
                         PVOID pv;
                         SendMessage (hwndLB, LB_GETTEXT, 0, (LPARAM) &pv);
                         bDir = pv != NULL;
                      }
                   }

                   if (GetKeyState(VK_SHIFT) < 0)
                   {
                      hwndTree = (!hwndTree) ? hWnd : hwndTree;

                      if (bDir)
                      {
                         hwndSet = hwndDir;
                         hwndNext = hwndTree;
                      }
                      else
                      {
                         hwndSet = hwndTree;
                      }
                   }
                   else
                   {
                      hwndSet = hwndTree ? hwndTree : (bDir ? hwndDir : hWnd);
                      hwndNext = hWnd;
                   }

                   SetFocus(hwndSet);
                   if ((hwndSet == hwndDir) && bChangeDisplay)
                   {
                       SetWindowLongPtr(hwndDir, GWL_NEXTHWND, (LPARAM)hwndNext);
                   }

                   break;
                }

          case VK_RETURN:               // same as double click
                NewTree(rgiDrive[nDriveFocus], hwndChild);
                break;

          case VK_SPACE:                // same as single click

                // lParam: if same drive, don't steal
                SendMessage(hWnd, FS_SETDRIVE, nDriveFocus, 1L);
                break;

          case VK_LEFT:
                nDrive = max(nDriveFocus-1, 0);
                break;

          case VK_RIGHT:
                nDrive = min(nDriveFocus+1, cDrives-1);
                break;
          }

          if ((wParam == VK_LEFT) || (wParam == VK_RIGHT)) {

                SetWindowLongPtr(hWnd, GWL_CURDRIVEFOCUS, nDrive);

                GetDriveRect(nDriveFocus, &rc);
                InvalidateRect(hWnd, &rc, TRUE);
                GetDriveRect(nDrive, &rc);
                InvalidateRect(hWnd, &rc, TRUE);
          } else if ((wParam >= CHAR_A) && (wParam <= CHAR_Z))
                KeyToItem(hWnd, (WORD)wParam);

          break;

      case FS_GETDRIVE:
          {
              POINT pt;

              POINTSTOPOINT(pt, lParam);
              nDrive = DriveFromPoint(hwndDriveBar, pt);

              if (nDrive < 0)
                  nDrive = nDriveCurrent;

              return rgiDrive[nDrive] + CHAR_A;
          }

      case WM_DRAGMOVE:
      {
         static BOOL fOldShowSourceBitmaps = 0;

         #define lpds ((LPDROPSTRUCT)lParam)

         nDrive = DriveFromPoint(lpds->hwndSink, lpds->ptDrop);

         // turn off?

// Handle if user hits control while dragging to drive

         if (nDrive == nDriveDragging && fOldShowSourceBitmaps != fShowSourceBitmaps) {
            fOldShowSourceBitmaps = fShowSourceBitmaps;
            RectDrive(nDrive, TRUE);
            nDriveDragging = -1;
         }

         if ((nDrive != nDriveDragging) && (nDriveDragging >= 0)) {

            RectDrive(nDriveDragging, FALSE);

            SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
               (LPARAM)szNULL);
            UpdateWindow(hwndStatus);

            nDriveDragging = -1;
         }

         // turn on?

         if ((nDrive >= 0) && (nDrive != nDriveDragging)) {
            RectDrive(nDrive, TRUE);
            nDriveDragging = nDrive;

            GetSelectedDirectory(rgiDrive[nDrive]+1, szDir);
         } else {
            if (nDrive != -1) {
               break;
            } else {
               // Blank space on end of drive bar is as if the user
               //  dropped onto the current drive.
               SendMessage(hwndChild, FS_GETDIRECTORY, COUNTOF(szDir), (LPARAM)szDir);
               StripBackslash(szDir);
            }
         }

         SetStatusText(SBT_NOBORDERS|255, SST_FORMAT|SST_RESOURCE,
            (LPTSTR)(DWORD)(fShowSourceBitmaps ? IDS_DRAG_COPYING : IDS_DRAG_MOVING),
            szDir);
         UpdateWindow(hwndStatus);

         break;
      }
      case WM_DRAGSELECT:
      #define lpds ((LPDROPSTRUCT)lParam)

          if (wParam) {

             SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
               (LPARAM)szNULL);
             SendMessage(hwndStatus, SB_SIMPLE, 1, 0L);

             UpdateWindow(hwndStatus);

             // entered, turn it on
             nDriveDragging = DriveFromPoint(lpds->hwndSink,lpds->ptDrop);
             if (nDriveDragging >= 0) {
                RectDrive(nDriveDragging, TRUE);
                GetSelectedDirectory(rgiDrive[nDriveDragging]+1, szDir);
             } else {
                SendMessage(hwndChild, FS_GETDIRECTORY, COUNTOF(szDir), (LPARAM)szDir);
                StripBackslash(szDir);
             }

         SetStatusText(SBT_NOBORDERS|255, SST_RESOURCE|SST_FORMAT,
            (LPTSTR)(DWORD)(fShowSourceBitmaps ? IDS_DRAG_COPYING : IDS_DRAG_MOVING),
            szDir);
         UpdateWindow(hwndStatus);

          } else {
             // leaving, turn it off

         SendMessage(hwndStatus, SB_SETTEXT, SBT_NOBORDERS|255,
            (LPARAM)szNULL);
         SendMessage(hwndStatus, SB_SIMPLE, 0, 0L);

         UpdateWindow(hwndStatus);
             if (nDriveDragging >= 0)
                RectDrive(nDriveDragging, FALSE);
      }

      break;

      case WM_QUERYDROPOBJECT:
          /* Validate the format. */
          #define lpds ((LPDROPSTRUCT)lParam)

          // if (DriveFromPoint(lpds->ptDrop) < 0)
          //    return FALSE;

          switch (lpds->wFmt) {
          case DOF_EXECUTABLE:
          case DOF_DIRECTORY:
          case DOF_MULTIPLE:
          case DOF_DOCUMENT:
                return(TRUE);
          default:
                return FALSE;
          }
          break;

      case WM_DROPOBJECT:
          // Turn off the status bar
          SendMessage(hwndStatus, SB_SIMPLE, 0, 0L);

          UpdateWindow(hwndStatus);

          DrivesDropObject(hWnd, (LPDROPSTRUCT)lParam);
          return TRUE;

      case WM_SETFOCUS:
          SetWindowLongPtr(hwndChild, GWL_LASTFOCUS, (LPARAM)hWnd);
          // fall through

      case WM_KILLFOCUS:

          InvalidateDrive(nDriveFocus);
          break;

      case WM_PAINT:
          DrivesPaint(hWnd, nDriveFocus, nDriveCurrent);
          break;

      // the following handlers deal with mouse actions on
      // the drive bar. they support the following:
      // 1) single click on a drive sets the window to that drive
      //    (on the upclick like regular buttons)
      // 2) double click on a drive creates a new window for
      //    that drive
      // 3) double click on empty area brings up
      //    the list of drive labels/share names
      //
      // since we see the up of the single click first we need to
      // wait the double click time to make sure we don't get the
      // double click. (we set a timer) since there is this
      // delay it is important to provide instant feedback
      // to the user by framing the drive when the mouse first hits it

      case WM_MDIACTIVATE:
          nDriveDoubleClick = -1; // invalidate any cross window drive actions

        break;


      case WM_TIMER:
        {
           HWND hwndTreeCtl;

           KillTimer(hWnd, wParam); // single shot timer

           if (nDriveDoubleClick >= 0) {
               // do the single click action

               // lParam!=0 => if same drive, don't steal!
               SendMessage(hWnd, FS_SETDRIVE, nDriveDoubleClick, 1L);
               nDriveDoubleClick = -1;
           }

           //
           //  Enable drive list combo box once the timer is finished
           //  if the readlevel is set to 0.
           //
           hwndTreeCtl = HasTreeWindow(hwndChild);
           if ( (!hwndTreeCtl) ||
                (GetWindowLongPtr(hwndTreeCtl, GWL_READLEVEL) == 0) )
           {
               EnableWindow(hwndDriveList, TRUE);
           }

           break;
        }

      case WM_LBUTTONDOWN:
        {
           POINT pt;

           POINTSTOPOINT(pt, lParam);
           SetCapture(hWnd);  // make sure we see the WM_LBUTTONUP
           nDriveDoubleClick = DriveFromPoint(hwndDriveBar, pt);

           // provide instant user feedback
           if (nDriveDoubleClick >= 0)
              RectDrive(nDriveDoubleClick, TRUE);
        }
        break;

      case WM_LBUTTONUP:
        {
            POINT pt;

            POINTSTOPOINT(pt, lParam);

            ReleaseCapture();
            nDrive = DriveFromPoint(hwndDriveBar, pt);

            if (nDriveDoubleClick >= 0) {
               InvalidateDrive(nDriveDoubleClick);

               if (hwndChild == hwndSearch)
                  break;

               if (nDrive == nDriveDoubleClick)
               {
                   //
                   //  Disable drive list combo box during the timer.
                   //
                   EnableWindow(hwndDriveList, FALSE);

                   SetTimer(hWnd, 1, GetDoubleClickTime(), NULL);
               }
           }
      }
          break;

      case WM_LBUTTONDBLCLK:
        {
            POINT pt;

            POINTSTOPOINT(pt, lParam);
            nDrive = DriveFromPoint(hwndDriveBar, pt);

            if (nDriveDoubleClick == nDrive) {
                nDriveDoubleClick = -1;

               // the double click is valid
               if (nDrive >= 0)
                    NewTree(rgiDrive[nDrive], hwndChild);
               else
                   PostMessage(hwndFrame,WM_COMMAND, GET_WM_COMMAND_MPS(IDM_DRIVESMORE, 0, 0));
           }


           // invalidate the single click action
           // ie. don't let the timer do a FS_SETDRIVE

            nDriveDoubleClick = -1;
      }
          break;

      case FS_SETDRIVE:
          // wParam     the drive index to set
          // lParam if Non-Zero, don't steal on same drive!

          DrivesSetDrive(hWnd, (DRIVEIND)wParam, nDriveCurrent,
            lParam && (wParam == (WPARAM)nDriveCurrent));

          break;


      default:
          return DefWindowProc(hWnd, wMsg, wParam, lParam);
  }

  return 0L;
}

/* Returns nDrive if found, else -1 */
INT
KeyToItem(HWND hWnd, WORD nDriveLetter)
{
    INT nDrive;

    if (nDriveLetter > CHAR_Z)
        nDriveLetter -= CHAR_a;
    else
        nDriveLetter -= CHAR_A;

    for (nDrive = 0; nDrive < cDrives; nDrive++) {
        if (rgiDrive[nDrive] == (int)nDriveLetter) {

           // If same drive, don't steal
           SendMessage(hWnd, FS_SETDRIVE, nDrive, 1L);
           return nDrive;
        }
    }
    return -1;
}
