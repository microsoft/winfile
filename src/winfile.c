/********************************************************************

   winfile.c

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#define _GLOBALS
#include "winfile.h"
#include "lfn.h"
#include <commctrl.h>
#include <shlobj.h>

//
// Overall Window structure
//
// Frame Window (FrameWndProc(), global hwndFrame)
// Toolbar Window (?, global hwndToolbar)
//    Combo Box for drive list (n/a, global hwndDriveList)
// Drives bar (DrivesWndproc(), global hwndDriveBar)
// MDI Client (n/a, global hwndMDIClient)
//    Tree window (TreeWndProc(), <hwndActive> looked up)
//       Tree control on left (TreeControlWndProc(), hwndTree = HasTreeWindow(hwndActive))
//          Listbox (n/a, GetDlgItem(hwndTree, IDCW_TREELISTBOX))
//       Directory content list on right (DirWndProc(), hwndDir = HasDirWindow(hwndActive), GWL_LISTPARMS -> parent hwndActive)
//          Listbox (n/a, GetDlgItem(hwndDir, IDCW_LISTBOX))
//    Search results window (SearchWndProc(), global hwndSearch, GWL_LISTPARMS -> hwndSearch)
//       Listbox (n/a, GetDlgItem(hwndDir, IDCW_LISTBOX))
// Status window (n/a, hwndStatus)
//

//
// prototypes
//
BOOL EnablePropertiesMenu(HWND hwnd, LPWSTR pszSel);
BOOL bDialogMessage(PMSG pMsg);

extern BOOL bCompressReEntry;

INT APIENTRY
WinMain(
   HINSTANCE hInst,
   HINSTANCE hPrevInst,
   LPSTR pszCmdLineA,
   INT nCmdShow)
{
   MSG       msg;
   LPWSTR    pszCmdLine;

#ifdef HEAPCHECK
   HeapCheckInit();
#endif
   pszCmdLine = GetCommandLine();

#if defined(JAPAN) && defined(i386)
   RegGetMachineIdentifierValue( &gdwMachineId );
#endif // defined(JAPAN) && defined(i386)

   if (!InitFileManager(hInst, pszNextComponent(pszCmdLine), nCmdShow)) {
      FreeFileManager();
      return FALSE;
   }

   while (TRUE) {

      vWaitMessage();

      while (PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE)) {

         if (msg.message == WM_QUIT) {
            FreeFileManager();

#ifdef HEAPCHECK
            HeapCheckDump(0);
            HeapCheckDestroy();
#endif

            return (INT)msg.wParam;
         }

         //
         // since we use RETURN as an accelerator we have to manually
         // restore ourselves when we see VK_RETURN and we are minimized
         //

         if (msg.message == WM_SYSKEYDOWN &&
            msg.wParam == VK_RETURN &&
            IsIconic(hwndFrame)) {

            ShowWindow(hwndFrame, SW_NORMAL);

         } else {

            if (!bDialogMessage(&msg)) {

               if (!TranslateMDISysAccel(hwndMDIClient, &msg) &&
                  (!hwndFrame || !TranslateAccelerator(hwndFrame,
                                                       hAccel,
                                                       &msg))) {

                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
               }
            }
         }
      }
   }
}


VOID
ResizeControls(VOID)
{
   static INT nViews[] = {
      1, 0,                // placeholder for the main menu handle
      1, IDC_TOOLBAR,
      1, IDC_STATUS,
      0, 0                // signify the end of the list
   };

   RECT rc;
   INT cDrivesPerRow;
   INT dyDriveBar;

   //
   // These controls move and resize themselves
   //
   if (hwndStatus)
      SendMessage(hwndStatus, WM_SIZE, 0, 0L);

   //
   // This stuff is nec since bRepaint in MoveWindow seems
   // broken.  By invalidating, we don't scroll bad stuff.
   //
   if (bDriveBar) {
      InvalidateRect(hwndDriveBar,NULL, FALSE);
   }
   InvalidateRect(hwndMDIClient,NULL, FALSE);

   SendMessage(hwndToolbar, WM_SIZE, 0, 0L);

   GetEffectiveClientRect(hwndFrame, &rc, nViews);
   rc.right -= rc.left;

   cDrivesPerRow = rc.right / dxDrive;
   if (!cDrivesPerRow)
      cDrivesPerRow++;

   dyDriveBar = dyDrive*((cDrives+cDrivesPerRow-1)/cDrivesPerRow)
      + 2*dyBorder;

   rc.right += 2*dyBorder;

   MoveWindow(hwndDriveBar, rc.left-dyBorder, rc.top-dyBorder,
      rc.right, dyDriveBar, FALSE);


   if (bDriveBar)
      rc.top += dyDriveBar - dyBorder;

   MoveWindow(hwndMDIClient, rc.left-dyBorder, rc.top-dyBorder,
      rc.right, rc.bottom-rc.top+2*dyBorder-1, TRUE);
}


BOOL
InitPopupMenus(UINT uMenus, HMENU hMenu, HWND hwndActive)
{
   DWORD	 dwSort;
   DWORD     dwView;
   UINT      uMenuFlags, uCompFlags;
   HWND      hwndTree, hwndDir;
   DRIVE     drive;
   BOOL      bLFN;
   INT       i;
   DWORD     dwFlags;


   hwndTree = HasTreeWindow(hwndActive);
   hwndDir = HasDirWindow(hwndActive);
   dwSort = GetWindowLongPtr(hwndActive, GWL_SORT);
   dwView = GetWindowLongPtr(hwndActive, GWL_VIEW);

   uMenuFlags = MF_BYCOMMAND | MF_ENABLED;

   bLFN = FALSE;       // For now, ignore the case.

   if (uMenus & (1 << IDM_FILE)) {

      LPWSTR    pSel = NULL;
      BOOL      bDir = TRUE;
      IDataObject *pDataObj;

      //
      // In order to avoid deleting the tree control pNodes while
      // a read is in progress, the Move, Copy, Delete, Rename, and
      // and Create Directory menu items will be disabled.
      //
      // The reason this can happen is that the function ReadDirLevel
      // does a wfYield so that messages can be processed.  As a result,
      // if any operation that deletes the tree list occurs during the
      // yield, the ReadDirLevel function will be accessing freed memory
      // when it continues from the yield.
      //
      if ((hwndTree) && GetWindowLongPtr(hwndTree, GWL_READLEVEL))
      {
          uMenuFlags = MF_BYCOMMAND | MF_GRAYED;
      }
      else
      {
          uMenuFlags = MF_BYCOMMAND;
      }
      EnableMenuItem(hMenu, IDM_MOVE, uMenuFlags);
      EnableMenuItem(hMenu, IDM_COPY, uMenuFlags);
      EnableMenuItem(hMenu, IDM_DELETE, uMenuFlags);
      EnableMenuItem(hMenu, IDM_RENAME, uMenuFlags);
      EnableMenuItem(hMenu, IDM_MAKEDIR, uMenuFlags);

	  if (OleGetClipboard(&pDataObj) == S_OK)
	  {
	      UINT uPaste = uMenuFlags;
	  	  FORMATETC fmtetcDrop = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	  	  FORMATETC fmtetcLFN = { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	      unsigned short cp_format_descriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
		  unsigned short cp_format_contents = RegisterClipboardFormat(CFSTR_FILECONTENTS);

		  //Set up format structure for the descriptor and contents
		  FORMATETC descriptor_format = {cp_format_descriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		  FORMATETC contents_format = {cp_format_contents, NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};

  	  	  fmtetcLFN.cfFormat = RegisterClipboardFormat(TEXT("LongFileNameW"));

		  if (pDataObj->lpVtbl->QueryGetData(pDataObj, &fmtetcDrop) != S_OK &&
		  	  pDataObj->lpVtbl->QueryGetData(pDataObj, &fmtetcLFN) != S_OK && 
			  (pDataObj->lpVtbl->QueryGetData(pDataObj, &descriptor_format) != S_OK ||
			   pDataObj->lpVtbl->QueryGetData(pDataObj, &contents_format) != S_OK))
		  	 uPaste |= MF_GRAYED;
		  	 
		  EnableMenuItem(hMenu, IDM_PASTE, uPaste);

		  pDataObj->lpVtbl->Release(pDataObj);
      }

      if (!hwndDir)
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

      EnableMenuItem(hMenu, IDM_SELALL,   uMenuFlags);
      EnableMenuItem(hMenu, IDM_DESELALL, uMenuFlags);

      if (hwndActive == hwndSearch || hwndDir)
         uMenuFlags = MF_BYCOMMAND;
      else
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

      EnableMenuItem(hMenu, IDM_SELECT, uMenuFlags);

      pSel = (LPTSTR)SendMessage(hwndActive, FS_GETSELECTION, 5, (LPARAM)&bDir);

      //
      // can't print a dir
      //
      uMenuFlags = bDir
         ? MF_BYCOMMAND | MF_DISABLED | MF_GRAYED
         : MF_BYCOMMAND | MF_ENABLED;

      EnableMenuItem(hMenu, IDM_PRINT, uMenuFlags);

      //
      // See if we can enable the Properties... menu
      //
      if (EnablePropertiesMenu(hwndActive, pSel))
         uMenuFlags = MF_BYCOMMAND;
      else
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;
      EnableMenuItem (hMenu, IDM_ATTRIBS, uMenuFlags);

      //
      // Check for Compress/Uncompress menu item enabling.
      //
      uCompFlags = MF_BYCOMMAND | MF_GRAYED;          // Compress...
      uMenuFlags = MF_BYCOMMAND | MF_GRAYED;          // Uncompress...

      if (pSel && !bCompressReEntry)
      {
         WCHAR     szDrv[6];      //  Enough to hold root pathname

         //
         //  Path is already fully qualified by passing in bit 4 to the
         //  FS_GETSELECTION call, so simply null terminate the string
         //  after the root path.  However, GetRootPath() will take care
         //  quoted paths.
         //

         // get a buffer big enough for a quoted path
         TCHAR szTemp[MAX_PATH + 2 * sizeof(TCHAR)];

         lstrcpy( szTemp, pSel);

         GetRootPath (szTemp, szDrv);

         //
         //  See if the volume allows file compression.
         //
         if (GetVolumeInformation(szDrv, NULL, 0L, NULL, NULL, &dwFlags, NULL, 0L) &&
             (dwFlags & FS_FILE_COMPRESSION) )
         {
            if (hwndTree == GetTreeFocus(hwndActive))
            {
               //
               //  Always enable the compress and decompress menu items
               //  when the tree control side is highlighted, since it
               //  only contains directories.
               //
               uCompFlags = MF_BYCOMMAND | MF_ENABLED;     // Compress...
               uMenuFlags = MF_BYCOMMAND | MF_ENABLED;     // Uncompress...
            }
            else
            {
               switch ((UINT)SendMessage(hwndActive, FS_GETSELECTION, 64, 0L))
               {
                  case ( 3 ) :
                  {
                     //
                     //  Enable both the compress and decompress menu items.
                     //
                     uCompFlags = MF_BYCOMMAND | MF_ENABLED;

                     // FALL THROUGH...
                  }
                  case ( 2 ) :
                  {
                     //
                     //  Enable the uncompress menu item and disable the
                     //  compress menu item.
                     //
                     uMenuFlags = MF_BYCOMMAND | MF_ENABLED;
                     break;
                  }
                  case ( 1 ) :
                  {
                     //
                     //  Enable the compress menu item and disable the
                     //  uncompress menu item.
                     //
                     uCompFlags = MF_BYCOMMAND | MF_ENABLED;
                     break;
                  }
                  case ( 0 ) :
                  default :
                  {
                     //
                     //  Disable both the compress and decompress menu items.
                     //
                     break;
                  }
               }
            }
         }

         LocalFree((HLOCAL)pSel);
      }

      EnableMenuItem(hMenu, IDM_COMPRESS, uCompFlags);
      EnableMenuItem(hMenu, IDM_UNCOMPRESS, uMenuFlags);

      if (uMenus & (1<<IDM_DISK)) {

         // be sure not to allow disconnect while any trees
         // are still being read (iReadLevel != 0)

         if (bConnectable) {

            uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

            if (!iReadLevel) {
               for (i=0; i < cDrives; i++) {
                  drive = rgiDrive[i];

                  if ( !IsCDRomDrive(drive) && IsRemoteDrive(drive) ) {

                     uMenuFlags = MF_BYCOMMAND | MF_ENABLED;
                     break;
                  }
               }
            }
            EnableMenuItem(hMenu, IDM_DISCONNECT, uMenuFlags);

         } else {
            if (iReadLevel)
               EnableMenuItem(hMenu, IDM_CONNECTIONS, MF_BYCOMMAND | MF_GRAYED);
            else
               EnableMenuItem(hMenu, IDM_CONNECTIONS, MF_BYCOMMAND | MF_ENABLED);
         }
      }
   }

   if (uMenus & (1<<IDM_TREE)) {

      if (!hwndTree || iReadLevel)
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

      EnableMenuItem(hMenu, IDM_EXPONE,     uMenuFlags);
      EnableMenuItem(hMenu, IDM_EXPSUB,     uMenuFlags);
      EnableMenuItem(hMenu, IDM_EXPALL,     uMenuFlags);
      EnableMenuItem(hMenu, IDM_COLLAPSE,   uMenuFlags);
      EnableMenuItem(hMenu, IDM_ADDPLUSES,  uMenuFlags);

      if (hwndTree)
         CheckMenuItem(hMenu, IDM_ADDPLUSES, GetWindowLongPtr(hwndActive, GWL_VIEW) & VIEW_PLUSES ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);

   }

   if (uMenus & (1<<IDM_VIEW)) {

      if (hwndActive == hwndSearch || IsIconic(hwndActive))
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;
      else {
         CheckMenuItem(hMenu, IDM_BOTH, hwndTree && hwndDir ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
         CheckMenuItem(hMenu, IDM_DIRONLY, !hwndTree && hwndDir ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
         CheckMenuItem(hMenu, IDM_TREEONLY, hwndTree && !hwndDir ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      }

      EnableMenuItem(hMenu, IDM_BOTH,      uMenuFlags);
      EnableMenuItem(hMenu, IDM_TREEONLY,  uMenuFlags);
      EnableMenuItem(hMenu, IDM_DIRONLY,   uMenuFlags);
      EnableMenuItem(hMenu, IDM_SPLIT,     uMenuFlags);

      dwView &= VIEW_EVERYTHING;

      CheckMenuItem(hMenu, IDM_VNAME,   (dwView == VIEW_NAMEONLY) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      CheckMenuItem(hMenu, IDM_VDETAILS,(dwView == VIEW_EVERYTHING) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      CheckMenuItem(hMenu, IDM_VOTHER,  (dwView != VIEW_NAMEONLY && dwView != VIEW_EVERYTHING) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);

      CheckMenuItem(hMenu, IDM_BYNAME, (dwSort == IDD_NAME) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      CheckMenuItem(hMenu, IDM_BYTYPE, (dwSort == IDD_TYPE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      CheckMenuItem(hMenu, IDM_BYSIZE, (dwSort == IDD_SIZE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      CheckMenuItem(hMenu, IDM_BYDATE, (dwSort == IDD_DATE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      CheckMenuItem(hMenu, IDM_BYFDATE,(dwSort == IDD_FDATE) ? MF_CHECKED | MF_BYCOMMAND : MF_UNCHECKED | MF_BYCOMMAND);
      
      if (hwndActive == hwndSearch || hwndDir)
         uMenuFlags = MF_BYCOMMAND | MF_ENABLED;
      else
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

      EnableMenuItem(hMenu, IDM_VNAME,    uMenuFlags);
      EnableMenuItem(hMenu, IDM_VDETAILS, uMenuFlags);
      EnableMenuItem(hMenu, IDM_VOTHER,   uMenuFlags);

      if (hwndDir)
         uMenuFlags = MF_BYCOMMAND | MF_ENABLED;
      else
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

      EnableMenuItem(hMenu, IDM_BYNAME, uMenuFlags);
      EnableMenuItem(hMenu, IDM_BYTYPE, uMenuFlags);
      EnableMenuItem(hMenu, IDM_BYSIZE, uMenuFlags);
      EnableMenuItem(hMenu, IDM_BYDATE, uMenuFlags);
      EnableMenuItem(hMenu, IDM_BYFDATE, uMenuFlags);

      if (IsIconic(hwndActive))
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;
      EnableMenuItem(hMenu, IDM_VINCLUDE, uMenuFlags);
   }

   if (uMenus & (1<<IDM_OPTIONS)) {

      if (iReadLevel)
         uMenuFlags = MF_BYCOMMAND | MF_GRAYED;

      EnableMenuItem(hMenu, IDM_ADDPLUSES, uMenuFlags);
      EnableMenuItem(hMenu, IDM_EXPANDTREE, uMenuFlags);

      uMenuFlags = bToolbar ? MF_BYCOMMAND|MF_ENABLED : MF_BYCOMMAND|MF_GRAYED;
      EnableMenuItem(hMenu, IDM_TOOLBARCUST, uMenuFlags);
   }

   return TRUE;
}


LRESULT
FrameWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   RECT     rc;
   HMENU    hMenu2;

   DRIVEIND driveInd;
   BOOL     bRedoDriveBar;

   switch (wMsg) {
   case WM_TIMER:

      //
      // this came from a FSC that wasn't generated by us
      //
      bFSCTimerSet = FALSE;
      KillTimer(hwnd, 1);

      //
      // Fall through to FS_ENABLEFSC
      //

   case FS_ENABLEFSC:
   {
      HWND hwndTree;

      if (--cDisableFSC)
         break;

      for (hwndTree = GetWindow(hwndMDIClient, GW_CHILD);
         hwndTree;
         hwndTree = GetWindow(hwndTree, GW_HWNDNEXT)) {

         //
         // a tree or search window
         //
         if (!GetWindow(hwndTree, GW_OWNER) &&
            GetWindowLongPtr(hwndTree,GWL_FSCFLAG)) {

            SendMessage(hwndTree, WM_FSC, FSC_REFRESH, 0L);
         }
      }
      break;
   }
   case FS_DISABLEFSC:

      cDisableFSC++;
      break;

   case FS_REBUILDDOCSTRING:

      BuildDocumentStringWorker();
      break;

   case FS_UPDATEDRIVETYPECOMPLETE:
      //
      // wParam = new cDrives
      //

      //
      // See if we need to update the drivebar.
      // If wParam == cDrives and rgiDrive hasn't changed, then
      // we don't need to refresh.
      //

      bRedoDriveBar = TRUE;

      if (cDrives == (INT)wParam) {

         for (driveInd=0; driveInd < cDrives; driveInd++) {

            if (rgiDriveReal[0][driveInd] != rgiDriveReal[1][driveInd])
               break;
         }

         bRedoDriveBar = (driveInd != cDrives);
      }

      cDrives = (INT)wParam;
      iUpdateReal ^= 1;

      //
      // Update drivelist cb based on new rgiDrive[] if nec.
      //

      if (bRedoDriveBar) {
         RedoDriveWindows(NULL);
      }

      //
      // Now update the DisconnectButton
      //

      EnableDisconnectButton();

      break;

   case FS_UPDATEDRIVELISTCOMPLETE:

      UpdateDriveListComplete();

      break;

   case FS_CANCELCOPYFORMATDEST:

      if (CancelInfo.hCancelDlg) {

         TCHAR szTemp[128];

         if (CancelInfo.Info.Copy.bFormatDest) {
            LoadString(hAppInstance, IDS_FORMATTINGDEST, szTemp, COUNTOF(szTemp));
         } else {
            LoadString(hAppInstance, IDS_COPYINGDISKTITLE, szTemp, COUNTOF(szTemp));
         }

         SetWindowText(CancelInfo.hCancelDlg, szTemp);
      }
      break;

   case FS_CANCELMESSAGEBOX:
      {
         TCHAR szMessage[MAXMESSAGELEN];
         TCHAR szTitle[MAXTITLELEN];
         HWND hwndT;

         LoadString(hAppInstance, (UINT)wParam, szTitle, COUNTOF(szTitle));
         LoadString(hAppInstance, (UINT)lParam, szMessage, COUNTOF(szMessage));

         hwndT = CancelInfo.hCancelDlg ?
            CancelInfo.hCancelDlg :
            hwndFrame;

         return MessageBox(hwndT, szMessage, szTitle, CancelInfo.fuStyle);
      }

   case FS_CANCELUPDATE:

      // wParam = % completed

      if (CancelInfo.hCancelDlg) {
         CancelInfo.nPercentDrawn = (INT)wParam;
         SendMessage(CancelInfo.hCancelDlg, FS_CANCELUPDATE, 0, 0L);
      }

      break;

   case FS_SEARCHUPDATE:

      // wParam = iDirsRead
      // lParam = iFileCount

      if (SearchInfo.hSearchDlg) {
         TCHAR szTemp[20];

         wsprintf(szTemp, SZ_PERCENTD,wParam);

         SendDlgItemMessage(SearchInfo.hSearchDlg,IDD_TIME,
            WM_SETTEXT,0,(LPARAM)szTemp);

         wsprintf(szTemp, SZ_PERCENTD,lParam);

         SendDlgItemMessage(SearchInfo.hSearchDlg,IDD_FOUND,
            WM_SETTEXT,0,(LPARAM)szTemp);

         UpdateWindow(SearchInfo.hSearchDlg);
      }

      //
      // If search window is active, update the status bar
      // (since set by same thread, no problem of preemption:
      // preemption only at message start/end)
      //
      if (SearchInfo.bUpdateStatus) {
         UpdateSearchStatus(SearchInfo.hwndLB, (INT) lParam);
      }

      break;

   case FS_CANCELEND:

      DestroyCancelWindow();

      switch (CancelInfo.eCancelType) {
      case CANCEL_FORMAT:
         FormatEnd();
         break;
      case CANCEL_COPY:
         CopyDiskEnd();
         break;
      default:
         break;
      }
      return 0L;

   case FS_SEARCHEND:

      //
      // The thread is now dead for our purposes
      //
      SearchInfo.hThread = NULL;

      //
      // Dismiss modless dialog box then inform user if nec.
      //
      if (SearchInfo.hSearchDlg) {
         DestroyWindow(SearchInfo.hSearchDlg);
         SearchInfo.hSearchDlg = NULL;
      }

      SearchEnd();

      return 0L;

   case FS_SEARCHLINEINSERT:
      {
         INT iRetVal;

         // wParam = &iFileCount
         // lParam = lpxdta

         ExtSelItemsInvalidate();

         iRetVal = (INT)SendMessage(SearchInfo.hwndLB,
                                    LB_ADDSTRING,
                                    0,
                                    lParam);

         if (iRetVal >= 0) {

            (*(INT *)wParam) ++;
         }
      }

      break;

   case WM_CREATE:
      {
         CLIENTCREATESTRUCT    ccs;

         // Store the Frame's hwnd.
         hwndFrame = hwnd;

         // Fill up this array with some popup menus.

         hMenu2 = GetMenu(hwnd);
         dwMenuIDs[3] = (DWORD)GetSubMenu(hMenu2, IDM_EXTENSIONS);
         dwMenuIDs[5] = (DWORD)GetSubMenu(hMenu2, IDM_EXTENSIONS+1);

         // the extensions haven't been loaded yet so the window
         // menu is in the position of the first extensions menu

         ccs.hWindowMenu = (HWND)dwMenuIDs[3];
         ccs.idFirstChild = IDM_CHILDSTART;

         // create the MDI client at approximate size to make sure
         // "run minimized" works

         GetClientRect(hwndFrame, &rc);

         hwndMDIClient = CreateWindow(TEXT("MDIClient"), NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | WS_BORDER,
            0, 0, rc.right, rc.bottom,
            hwnd, (HMENU)1, hAppInstance, (LPVOID)&ccs);

         if (!hwndMDIClient) {
            return -1L;
         }

         // make new drives window

         hwndDriveBar = CreateWindow(szDrivesClass, NULL,
            bDriveBar ? WS_CHILD|WS_BORDER|WS_VISIBLE|WS_CLIPSIBLINGS :
               WS_CHILD|WS_BORDER|WS_CLIPSIBLINGS,
            0, 0, 0, 0, hwndFrame, 0, hAppInstance, NULL);

         if (!hwndDriveBar)
            return -1L;

         // make toolbar window

         CreateFMToolbar();
         if (!hwndToolbar)
            return -1L;

         hwndStatus = CreateStatusWindow(
            bStatusBar ? WS_CHILD|WS_BORDER|WS_VISIBLE|WS_CLIPSIBLINGS :
            WS_CHILD|WS_BORDER|WS_CLIPSIBLINGS,
            szNULL, hwndFrame, IDC_STATUS);

		 if (hwndStatus) {
			 HDC hDC;
			 INT nParts[3];
			 INT nInch;

			 hDC = GetDC(NULL);
			 nInch = GetDeviceCaps(hDC, LOGPIXELSX);
			 ReleaseDC(NULL, hDC);

			 // use the smaller font for Status bar so that messages fix in it.
			 if (bJAPAN)
			 {
				 nParts[0] = nInch * 8 / 4 + (nInch * 7 / 8);
				 nParts[1] = nParts[0] + nInch * 11 / 4 + nInch * 7 / 8;
				 nParts[2] = -1;

				 SendMessage(hwndStatus, SB_SETPARTS, 3, (LPARAM)(LPINT)nParts);

				 SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFontStatus, 0L);
			 }
			 else
			 {
				 nParts[0] = nInch * 9 / 4 + (nInch * 7 / 8);
				 nParts[1] = nParts[0] + nInch * 5 / 2 + nInch * 7 / 8;
				 nParts[2] = -1;

				 SendMessage(hwndStatus, SB_SETPARTS, 3, (LPARAM)(LPINT)nParts);
			 }
		 }
         break;
      }

   case WM_INITMENUPOPUP:
      {
         HWND      hwndActive;
         UINT      uMenu;
         INT       index;
         BOOL      bMaxed;

         hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
         if (hwndActive && GetWindowLongPtr(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
            bMaxed = 1;
         else
            bMaxed = 0;

         uMenu = (UINT)LOWORD(lParam) - bMaxed;

         if (uMenu == IDM_SECURITY) {

            if (lpfnAcledit)
               (*lpfnAcledit)(hwndFrame, FMEVENT_INITMENU, (LPARAM)(HMENU)wParam);

         } else if ((uMenu >= IDM_EXTENSIONS) && (uMenu < ((UINT)iNumExtensions + IDM_EXTENSIONS))) {

            index = uMenu - IDM_EXTENSIONS;
            (extensions[index].ExtProc)(hwndFrame, FMEVENT_INITMENU, (LPARAM)(HMENU)wParam);

         } else {

            InitPopupMenus(1<<uMenu, (HMENU)wParam, hwndActive);
         }

         break;
      }

   case WM_DESTROY:

      if (!WinHelp(hwndFrame, szWinfileHelp, HELP_QUIT, 0L)) {
         MyMessageBox(hwndFrame, IDS_WINFILE, IDS_WINHELPERR, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
      }
      hwndFrame = NULL;
      PostQuitMessage(0);
      DestroyWindow(hwndDriveBar);
      break;

   case WM_SIZE:
      if (wParam != SIZEICONIC) {

         // uses new resize!
         ResizeControls();
      }
      break;

   case FS_FSCREQUEST:

      if (cDisableFSC == 0 || bFSCTimerSet) {
         if (bFSCTimerSet)
            KillTimer(hwndFrame, 1);                // reset the timer

         if (SetTimer(hwndFrame, 1, uChangeNotifyTime, NULL)) {

            bFSCTimerSet = TRUE;
            if (cDisableFSC == 0)                   // only disable once
               SendMessage(hwndFrame, FS_DISABLEFSC, 0, 0L);
         }
      }

      break;

   case WM_FSC:

      ChangeFileSystem((WORD)wParam, (LPTSTR)lParam, NULL);
      break;

   case WM_SYSCOLORCHANGE:
   case WM_WININICHANGE:
      if (!lParam || !lstrcmpi((LPTSTR)lParam, szInternational)) {
         HWND hwnd, hwndT;
         DWORD dwFlags;

         GetInternational();

         for (hwnd = GetWindow(hwndMDIClient,GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd,GW_HWNDNEXT)) {

            if (!GetWindow(hwnd, GW_OWNER)) {

               dwFlags = GetWindowLongPtr(hwnd, GWL_VIEW) &
                               (VIEW_EVERYTHING | VIEW_PLUSES);

               if (hwndT = HasDirWindow(hwnd)) {
                  SendMessage(hwndT, FS_CHANGEDISPLAY, CD_VIEW,
                     MAKELONG(dwFlags, TRUE));
               } else if (hwnd == hwndSearch) {
                  SetWindowLongPtr(hwnd, GWL_VIEW, dwFlags);
                  SendMessage(hwndSearch, FS_CHANGEDISPLAY, CD_VIEW, 0L);
               }
            }
         }
      }

      // win.ini section [colors]
      if (!lParam || !lstrcmpi((LPTSTR)lParam, TEXT("colors"))) {

         DeleteBitmaps();
         LoadBitmaps();

         // we need to recreate the drives windows to change
         // the bitmaps

         RedoDriveWindows(NULL);
      }

#if 0
      //
      // Check if the user's environment variables have changed, if so
      // regenerate the environment, so that new apps started from
      // taskman will have the latest environment.
      //
      if (lParam && (!lstrcmpi((LPTSTR)lParam, TEXT("Environment")))) {
         PVOID pEnv;

         RegenerateUserEnvironment(&pEnv, TRUE);
      }
#endif
      break;

   case FM_GETFOCUS:
   case FM_GETDRIVEINFOA:
   case FM_GETDRIVEINFOW:
   case FM_GETSELCOUNT:
   case FM_GETSELCOUNTLFN:
   case FM_GETFILESELA:
   case FM_GETFILESELW:
   case FM_GETFILESELLFNA:
   case FM_GETFILESELLFNW:
   case FM_REFRESH_WINDOWS:
   case FM_RELOAD_EXTENSIONS:
      return ExtensionMsgProc(wMsg, wParam, lParam);
      break;

   case WM_SETFOCUS:
	   UpdateMoveStatus(ReadMoveStatus());
	   goto DoDefault;

   case WM_MENUSELECT:
      {
         UINT uTemp;


         if (GET_WM_MENUSELECT_HMENU(wParam, lParam)) {

            // Save the menu the user selected
            uMenuID = GET_WM_MENUSELECT_CMD(wParam, lParam);
            uMenuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
            hMenu = GET_WM_MENUSELECT_HMENU(wParam, lParam);
            if (uMenuID >= IDM_CHILDSTART && uMenuID < IDM_HELPINDEX)
               uMenuID = IDM_CHILDSTART;


            // Handle child/frame sys menu decision

            //
            // If maximized, and the 0th menu is set,
            // then it must be the MDI child system menu.
            //
            bMDIFrameSysMenu = (hMenu == GetSystemMenu(hwndFrame, FALSE));

         }

         DriveListMessage(wMsg, wParam, lParam, &uTemp);
      }

      break;

   case WM_SYSCOMMAND:
      if (GetFocus() == hwndDriveList)
         SendMessage(hwndDriveList, CB_SHOWDROPDOWN, FALSE, 0L);
      return DefFrameProc(hwnd, hwndMDIClient, wMsg, wParam, lParam);
      break;

   case WM_ENDSESSION:

      if (wParam) {
         // Yeah, I know I shouldn't have to save this, but I don't
         // trust anybody

         BOOL bSaveExit = bExitWindows;
         bExitWindows = FALSE;

         // Simulate an exit command to clean up, but don't display
         // the "are you sure you want to exit", since somebody should
         // have already taken care of that, and hitting Cancel has no
         // effect anyway.

         AppCommandProc(IDM_EXIT);
         bExitWindows = bSaveExit;
      }
      break;

   case WM_NOTIFY:
      {
         UINT uRetVal;
         DriveListMessage(wMsg, wParam, lParam, &uRetVal);

         return uRetVal;
      }


   case WM_DRAWITEM:
   case WM_MEASUREITEM:
      {
         UINT uRetVal;
         DriveListMessage(wMsg, wParam, lParam, &uRetVal);

         if (uRetVal) break;

         goto DoDefault;
      }
   case WM_CLOSE:

      wParam = IDM_EXIT;

      /*** FALL THROUGH to WM_COMMAND ***/

   case WM_COMMAND:
      {
         UINT uRetVal;
         DWORD dwTemp;

         dwTemp = DriveListMessage(wMsg, wParam, lParam, &uRetVal);
         if (uRetVal)
            return(dwTemp);

      }

      if (AppCommandProc(GET_WM_COMMAND_ID(wParam, lParam)))
         break;
      if (GET_WM_COMMAND_ID(wParam, lParam) == IDM_EXIT) {

            FreeExtensions();
            if (hModUndelete >= (HANDLE)32)
               FreeLibrary(hModUndelete);

            DestroyWindow(hwnd);
            break;
         }
          /*** FALL THROUGH ***/

   default:

DoDefault:

      if (wMsg == wHelpMessage) {

         if (GET_WM_COMMAND_ID(wParam, lParam) == MSGF_MENU) {

            // Get outta menu mode if help for a menu item

            if (uMenuID && hMenu) {
               UINT m = uMenuID;       // save
               HMENU hM = hMenu;
               UINT mf = uMenuFlags;

               SendMessage(hwnd, WM_CANCELMODE, 0, 0L);

               uMenuID   = m;          // restore
               hMenu = hM;
               uMenuFlags = mf;
            }

            if (!(uMenuFlags & MF_POPUP)) {

               //
               // According to winhelp: GetSystemMenu, uMenuID >= 0x7000
               // means system menu items!
               //
               // This should not be nec since MF_SYSMENU is set!
               //
               if (uMenuFlags & MF_SYSMENU || uMenuID >= 0xf000) {

                  dwContext = bMDIFrameSysMenu ?
                     IDH_SYSMENU :
                     IDH_SYSMENUCHILD;

               } else {

                  INT iExt;

                  iExt = uMenuID / 100 - 1;

                  if (IDM_SECURITY == iExt) {

                     WAITACLEDIT();

                     if (lpfnAcledit) {
                        (*lpfnAcledit)(hwndFrame, FMEVENT_HELPMENUITEM, uMenuID % 100);
                     }

                     return 0L;
                  }

                  iExt -= IDM_EXTENSIONS;

                  if (0 <= iExt && iExt < iNumExtensions) {

                     extensions[iExt].ExtProc(hwndFrame,
                        FMEVENT_HELPMENUITEM, uMenuID % 100);

                     return 0L;
                  }

                  dwContext = uMenuID + IDH_HELPFIRST;
               }

               WFHelp(hwnd);
            }

         }
         else if (GET_WM_COMMAND_ID(wParam, lParam) == MSGF_DIALOGBOX) {

            // context range for message boxes

            if ((dwContext >= IDH_MBFIRST && dwContext <= IDH_MBLAST) ||
                (dwContext == IDH_CTBAR))
            {
               WFHelp(hwnd);
            }
            else
            {
               // let dialog box deal with it
               PostMessage(GetRealParent((HWND)lParam), wHelpMessage, 0, 0L);
            }
         }

      }
      else {
         return DefFrameProc(hwnd, hwndMDIClient, wMsg, wParam, lParam);
      }
   }

   return 0L;
}


INT
MessageFilter(INT nCode, WPARAM wParam, LPMSG lpMsg)
{
   if (nCode < 0)
      goto DefHook;

   if (nCode == MSGF_MENU) {

      if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
         // Window of menu we want help for is in loword of lParam.

         PostMessage(hwndFrame, wHelpMessage, MSGF_MENU, (LPARAM)lpMsg->hwnd);
         return 1;
      }

   }
   else if (nCode == MSGF_DIALOGBOX) {

      if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F1) {
         // Dialog box we want help for is in loword of lParam

         PostMessage(hwndFrame, wHelpMessage, MSGF_DIALOGBOX, (LPARAM)lpMsg->hwnd);
         return 1;
      }

   } else

DefHook:
      return (INT)DefHookProc(nCode, wParam, (LPARAM)lpMsg, &hhkMsgFilter);

  return 0;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     EnablePropertiesMenu
//
// Synopsis: Check if we enable the Properties... menu item
//           Disable if:
//
//           1. The rootdir is selected
//           2. _ONLY_ the .. dir is sel
//           3. Nothing is selected in the window with focus
//
// IN    hwndActive   Current active window, has listbox in LASTFOCUS
// IN    pSel         Current sel
//
// Return:   TRUE if Properties... should be enabled.
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
EnablePropertiesMenu(
   HWND hwndActive,
   LPWSTR pSel)
{
   LPXDTALINK lpStart;
   LRESULT dwHighlight;  // Number of highlighted entries in listbox
   LPXDTA lpxdta;    // Pointer to listbox DTA data
   BOOL bRet;        // Return value
   HWND hwndLB;
   HWND hwndDir;
   HWND hwndTree;
   HWND hwndParent;

   bRet = FALSE;

   //
   // Can't get properties on root directory
   //
   // Also quit if pSel == NULL (File selected before any window created)
   //
   if (!pSel || (lstrlen (pSel) == 3 && pSel[2] == CHAR_BACKSLASH))
      return (FALSE);

   hwndLB = (HWND)GetWindowLongPtr(hwndActive, GWL_LASTFOCUS);

   if (!hwndLB)
      return (FALSE);

   dwHighlight = SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);

   //
   // This is OK since the search window can never contain the root
   //
   if (hwndActive == hwndSearch)
      return (dwHighlight >= 1);

   hwndTree = HasTreeWindow(hwndActive);
   hwndDir = HasDirWindow(hwndActive);
   hwndParent = GetParent(hwndLB);

   if (hwndParent == hwndDir) {

      //
      // Lock down DTA data
      //
      if (!(lpStart = (LPXDTALINK)GetWindowLongPtr(GetParent(hwndLB), GWL_HDTA)))
         return (FALSE);

      if (dwHighlight <= 0)
         goto ReturnFalse;

      if (dwHighlight > 1)
         goto ReturnTrue;

      //
      // If exactly one element is highlighted, make sure it is not ..
      //
      if (!(BOOL) SendMessage(hwndLB, LB_GETSEL, 0, 0L))
         goto ReturnTrue;

      //
      // Get the DTA index.
      //
      if (SendMessage(hwndLB,
                      LB_GETTEXT,
                      0,
                      (LPARAM)&lpxdta) == LB_ERR || !lpxdta) {
         goto ReturnFalse;
      }

      if ((lpxdta->dwAttrs & ATTR_DIR) &&
         (lpxdta->dwAttrs & ATTR_PARENT))
         goto ReturnFalse;

ReturnTrue:

      bRet = TRUE;

ReturnFalse:

      return (bRet);
   }

   //
   // If this is the tree window and we are not in the middle of ReadDirLevel
   // and it is not the root, then it is OK to change properties.
   //
   if (hwndParent == hwndTree) {
      if (SendMessage(hwndLB, LB_GETCURSEL, 0, 0L) > 0L &&
         !GetWindowLongPtr(hwndTree, GWL_READLEVEL))

      return(TRUE);
   }

   return(FALSE);
}

BOOL
bDialogMessage(PMSG pMsg)
{
   if ((CancelInfo.hCancelDlg &&
      !CancelInfo.bModal &&
      IsDialogMessage(CancelInfo.hCancelDlg, pMsg)) ||

      (hwndFormatSelect &&
      IsDialogMessage(hwndFormatSelect, pMsg)) ||

      (SearchInfo.hSearchDlg &&
      IsDialogMessage(SearchInfo.hSearchDlg, pMsg)))

      return TRUE;

   return FALSE;
}

