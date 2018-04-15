/********************************************************************

    WFSearch.c - Windows File Manager Search code

    Copyright(c) Microsoft Corporation.All rights reserved.
    Licensed under the MIT License.

    Strategy for single multithreaded search:

    3 ways to terminate search
      1. Search completes normally
      2. User hits cancel on search progress dlg (search dlg)
      3. User closes mdi search window (search window) while searching

    bCancel indicates quit searching (cases 2,3)
          2-> SEARCH_CANCEL
          3-> SEARCH_MDICLOSE

    Case 1:  W: SendMessage FS_SEARCHEND to hwndFrame
             M: hThread = NULL               < FS_SEARCHEND >
                if search dlg up
                   kill search dlg
                   hSearchDlg = NULL
             M: invalidate hwndLB            < SearchEnd >
                if SEARCH_ERROR
                   message, quit
                if no matches
                   message, quit
                show normal window
             W: Worker thread dies

    Case 2:  M: bCancel = TRUE
                SEARCH_CANCEL
             W: handle like Case 1.

    Case 3:  M: bCancel = TRUE
                SEARCH_MDICLOSE
                hwndSearch = NULL
             M: ClearSearchLB()               < WM_DESTROY >
             M: Listbox cleared               < ClearSearchLB >
                MDI search window destroyed

             << asynchronous >>

             W: SendMessage FS_SEARCHEND

             << Now synchronous >>

             M: hThread = NULL
                destroy search dlg
             M: ClearSearchLB()               < SearchEnd >
             M: free memory                   < ClearSearchLB >

    All race conditions are eliminated by using SendMessages to
    the main thread or by "don't care" situations.

    Other race conditions:

       M:               W:
                        Search completes
       Hide/Cancel hit
                        FS_SEARCHEND sent

       No problem, since Hide/Cancel normally force a search completion.
       Now it just happens sooner.


       M:               W:
       MDI search close
                        Search completes

       MDI Search close must complete before FS_SEARCHEND can process.
       (Since it is SendMessage'd by W:)

    Freeing associated memory.  This is done by whoever leaves last.
    If the search window is closed first, the worker thread cleans up.  If
    the search completes and then the mdi search window is closed, the
    memory is freed by the mdi search window.

********************************************************************/

#include "winfile.h"
#include "lfn.h"

#include <commctrl.h>

INT maxExt;
INT iDirsRead;
DWORD dwLastUpdateTime;
INT maxExtLast;


VOID UpdateIfDirty(HWND hWnd);
INT  FillSearchLB(HWND hwndLB, LPWSTR szSearchFileSpec, BOOL bRecurse, BOOL bIncludeSubdirs);
INT  SearchList(
   HWND hwndLB,
   LPWSTR szPath,
   LPWSTR szFileSpec,
   BOOL bRecurse,
   BOOL bIncludeSubdirs,
   LPXDTALINK* plpStart,
   INT iFileCount,
   BOOL bRoot);
VOID ClearSearchLB(BOOL bWorkerCall);
INT SearchDrive();

#define SEARCH_FILE_WIDTH_DEFAULT 50




/////////////////////////////////////////////////////////////////////
//
// Name:     SearchList
//
// Synopsis: Recursive search
//
//
// Return:   INT, # of files found
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

INT
SearchList(
   HWND hwndLB,
   LPWSTR szPath,
   LPWSTR szFileSpec,
   BOOL bRecurse,
   BOOL bIncludeSubdirs,
   LPXDTALINK* plpStart,
   INT iFileCount,
   BOOL bRoot)
{
   INT iRetVal;
   SIZE size;
   BOOL bFound;
   LPWSTR pszNewPath;
   LPWSTR pszNextFile;
   LFNDTA lfndta;
   LPXDTA lpxdta;

   HDC hdc;
   HANDLE hOld;
   DWORD dwTimeNow;

   BOOL bLowercase;
   WCHAR szTemp[MAXPATHLEN];

   BOOL bLFN;
   DWORD dwAttrs;
   INT iBitmap;

   //
   // hack: setup ATTR_LOWERCASE if a letter'd (NON-unc) drive
   // LATER: do GetVolumeInfo for UNC too!
   //

   bLowercase = (wTextAttribs & TA_LOWERCASEALL) ||
      ((wTextAttribs & TA_LOWERCASE) && !SearchInfo.bCasePreserved);

   dwTimeNow = GetTickCount();

   if (dwTimeNow > dwLastUpdateTime+1000) {
      dwLastUpdateTime = dwTimeNow;

      SearchInfo.iDirsRead = iDirsRead;
      SearchInfo.iFileCount = iFileCount;

      PostMessage(hwndFrame, FS_SEARCHUPDATE, iDirsRead, iFileCount);
   }

   iDirsRead++;

   if (!*plpStart) {

      *plpStart = MemNew();

      if (!*plpStart) {
MemoryError:
         SearchInfo.dwError = ERROR_NOT_ENOUGH_MEMORY;
         SearchInfo.eStatus = SEARCH_ERROR;
         return iFileCount;
      }

      //
      // Never shows altname
      //
      MemLinkToHead(*plpStart)->dwAlternateFileNameExtent = 0;

      SetWindowLongPtr(GetParent(hwndLB), GWL_HDTA, (LPARAM)*plpStart);
      SearchInfo.lpStart = *plpStart;
   }

   //
   // allocate the buffer for this level
   //
   pszNewPath = (LPWSTR)LocalAlloc(LPTR, ByteCountOf(lstrlen(szPath) + MAXFILENAMELEN + 2));

   if (!pszNewPath) {
      goto MemoryError;
   }

   lstrcpy(pszNewPath, szPath);
   AddBackslash(pszNewPath);

   pszNextFile = pszNewPath + lstrlen(pszNewPath);
   lstrcpy(pszNextFile, szFileSpec);

   bFound = WFFindFirst(&lfndta, pszNewPath, ATTR_ALL);

   hdc = GetDC(hwndLB);
   hOld = SelectObject(hdc, hFont);

   //
   // Ignore file not found errors AND access denied errors
   // AND PATH_NOT_FOUND when not in the root
   //

   if (!bFound && ERROR_FILE_NOT_FOUND != lfndta.err &&
      (bRoot ||
         ERROR_ACCESS_DENIED != lfndta.err &&
         ERROR_PATH_NOT_FOUND != lfndta.err &&
         ERROR_INVALID_NAME != lfndta.err)) {

      SearchInfo.eStatus = SEARCH_ERROR;
      SearchInfo.dwError = lfndta.err;
      bRecurse = FALSE;

      goto SearchCleanup;
   }

   while (bFound) {

      //
      // allow escape to exit
      //
      if (SearchInfo.bCancel) {

         bRecurse = FALSE;
         break;
      }

	  // default ftSince is 0 and so normally this will be true
	  bFound = CompareFileTime(&SearchInfo.ftSince, &lfndta.fd.ftLastWriteTime) < 0;

	  // if we otherwise match, but shouldn't include directories in the output, skip
	  if (bFound && !bIncludeSubdirs && (lfndta.fd.dwFileAttributes & ATTR_DIR) != 0)
	  {
		  bFound = FALSE;
	  }

      //
      // Make sure this matches date (if specified) and is not a "." or ".." directory 
      //
      if (bFound && !ISDOTDIR(lfndta.fd.cFileName)) {

         lstrcpy(pszNextFile, lfndta.fd.cFileName);

         // Warning: was OemToChar(pszNewPath, szMessage);
         // but taken out since no translation necessary.
         // Here on out _was_ using szMessage
         // (Multithreaded=> szMessage usage BAD)

         bLFN = IsLFN(lfndta.fd.cFileName);

         if (bLowercase) {
            lstrcpy(szTemp, pszNewPath);
            CharLower(szTemp);

            GetTextExtentPoint32(hdc, szTemp, lstrlen(szTemp), &size);
         } else {
            GetTextExtentPoint32(hdc, pszNewPath, lstrlen(pszNewPath), &size);
         }

         maxExt = max(size.cx,maxExt);

         lpxdta = MemAdd(plpStart, lstrlen(pszNewPath), 0);

         if (!lpxdta) {

            bRecurse = FALSE;       // simulate an abort
            SearchInfo.dwError = ERROR_NOT_ENOUGH_MEMORY;
            SearchInfo.eStatus = SEARCH_ERROR;

            break;
         }

         dwAttrs = lpxdta->dwAttrs = lfndta.fd.dwFileAttributes;
         lpxdta->ftLastWriteTime = lfndta.fd.ftLastWriteTime;
         lpxdta->qFileSize.LowPart = lfndta.fd.nFileSizeLow;
         lpxdta->qFileSize.HighPart = lfndta.fd.nFileSizeHigh;

         lstrcpy(MemGetFileName(lpxdta), pszNewPath);
         MemGetAlternateFileName(lpxdta)[0] = CHAR_NULL;

         if (bLFN)
            lpxdta->dwAttrs |= ATTR_LFN;

         if (!SearchInfo.bCasePreserved)
            lpxdta->dwAttrs |= ATTR_LOWERCASE;

         if (dwAttrs & ATTR_DIR)
            iBitmap = BM_IND_CLOSE;
         else if (dwAttrs & (ATTR_HIDDEN | ATTR_SYSTEM))
            iBitmap = BM_IND_RO;
         else if (IsProgramFile(lfndta.fd.cFileName))
            iBitmap = BM_IND_APP;
         else if (IsDocument(lfndta.fd.cFileName))
            iBitmap = BM_IND_DOC;
         else
            iBitmap = BM_IND_FIL;

         lpxdta->byBitmap = iBitmap;
		 lpxdta->pDocB = NULL;

         SendMessage(hwndFrame,
                     FS_SEARCHLINEINSERT,
                     (WPARAM)&iFileCount,
                     (LPARAM)lpxdta);

      }

      //
      // Search for more files in the current directory
      //
      bFound = WFFindNext(&lfndta);
   }

SearchCleanup:

   WFFindClose(&lfndta);

   if (hOld)
      SelectObject(hdc, hOld);
   ReleaseDC(hwndLB, hdc);

   if (!bRecurse)
      goto SearchEnd;

   //
   // Now see if there are any subdirectories here
   //
   lstrcpy(pszNextFile, szStarDotStar);

   bFound = WFFindFirst(&lfndta, pszNewPath, ATTR_DIR | ATTR_HS);

   while (bFound) {

      //
      // allow escape to exit
      //
      if (SearchInfo.bCancel) {

         bRecurse = FALSE;
         break;
      }

      //
      // Make sure this is not a "." or ".." directory.
      //
      if (!ISDOTDIR(lfndta.fd.cFileName) &&
         (lfndta.fd.dwFileAttributes & ATTR_DIR)) {

         //
         // Yes, search and add files in this directory
         //
         lstrcpy(pszNextFile, lfndta.fd.cFileName);

         //
         // Add all files in this subdirectory.
         //
         iRetVal = SearchList(hwndLB,
                              pszNewPath,
                              szFileSpec,
                              bRecurse,
							  bIncludeSubdirs,
                              plpStart,
                              iFileCount,
                              FALSE);

         iFileCount = iRetVal;

         if (SEARCH_ERROR == SearchInfo.eStatus) {
            break;
         }

      }
      bFound = WFFindNext(&lfndta);
   }

   WFFindClose(&lfndta);

SearchEnd:


   //
   // Save the number of files in the xdtahead structure.
   //
   MemLinkToHead(SearchInfo.lpStart)->dwEntries = iFileCount;

   LocalFree((HANDLE)pszNewPath);
   return iFileCount;
}


VOID
FixUpFileSpec(
   LPWSTR szFileSpec)
{
  WCHAR szTemp[MAXPATHLEN+1];
  register LPWSTR p;

  if (*szFileSpec == CHAR_DOT) {
    lstrcpy(szTemp, SZ_STAR);
    lstrcat(szTemp, szFileSpec);
    lstrcpy(szFileSpec, szTemp);
  }


  //
  // HACK:  If there isn't a dot and the last char is a *, append ".*"
  //
  p = szFileSpec;
  while ((*p) && (*p != CHAR_DOT))
      ++p;

  if ((!*p) && (p != szFileSpec)) {
     --p;

     if (*p == CHAR_STAR)
        lstrcat(p, SZ_DOTSTAR);
  }
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FillSearchLB() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*  This parses the given string for Drive, PathName, FileSpecs and
 *  calls SearchList() with proper parameters;
 *
 *  hwndLB           : List box where files are to be displayed;
 *  szSearchFileSpec : ANSI path to search
 *  bSubDirOnly      : TRUE, if only subdirectories are to be searched;
 */

INT
FillSearchLB(HWND hwndLB, LPWSTR szSearchFileSpec, BOOL bRecurse, BOOL bIncludeSubdirs)
{
   INT iRet;
   INT iFileCount;
   WCHAR szFileSpec[MAXPATHLEN+1];
   WCHAR szPathName[MAXPATHLEN+1];
   WCHAR szWildCard[20];
   LPWCH lpszCurrentFileSpecStart;
   LPWCH lpszCurrentFileSpecEnd;
   LPXDTALINK lpStart = NULL;

   //
   // Get the file specification part of the string.
   //
   lstrcpy(szFileSpec, szSearchFileSpec);
   lstrcpy(szPathName, szSearchFileSpec);

   StripPath(szFileSpec);
   StripFilespec(szPathName);

   lpszCurrentFileSpecEnd = szFileSpec;

   maxExt = 0;

   iDirsRead = 1;
   dwLastUpdateTime = 0;
   iRet = 0;
   iFileCount = 0;

   //
   // This loop runs for each subspec in the search string
   //
   while (*lpszCurrentFileSpecEnd) {
	  lpszCurrentFileSpecStart = lpszCurrentFileSpecEnd;

	  // Find the next separator or the end of the string
	  while ((*lpszCurrentFileSpecEnd) && (*lpszCurrentFileSpecEnd) != ';')
		 lpszCurrentFileSpecEnd++;

	  // If a separator is reached, add the string terminator and move the
	  // end after the terminator for the next cycle
	  if (*lpszCurrentFileSpecEnd == ';') {
		  *lpszCurrentFileSpecEnd = TEXT('\0');
		  lpszCurrentFileSpecEnd++;
	  }

      // copy the wild card to a temporary buffer sine FixUpFileSpec modifies the buffer
      wcsncpy_s(szWildCard, COUNTOF(szWildCard), lpszCurrentFileSpecStart, _TRUNCATE);

	  FixUpFileSpec(szWildCard);

	  iRet = SearchList(hwndLB,
		  szPathName,
		  szWildCard,
		  bRecurse,
		  bIncludeSubdirs,
		  &lpStart,
		  iFileCount,
		  TRUE);

      iFileCount = iRet;
   }

   //
   // Only SetSel if none set already
   //
   if (LB_ERR == SendMessage(hwndLB, LB_GETCURSEL, 0, 0L))
      SendMessage(hwndLB, LB_SETSEL, TRUE, 0L);

   return(iRet);
}


VOID
GetSearchPath(HWND hWnd, LPWSTR pszPath)
{
   LPWSTR p;

   WCHAR szTemp[MAXPATHLEN+32];

   // the search window doesn't have a current directory
   GetWindowText(hWnd, szTemp, COUNTOF(szTemp));

   // the window text looks like "Search Results: C:\FOO\BAR\*.*"

   p = szTemp;
   while (*p && *p != CHAR_COLON) // find the :
      ++p;

   p += 2;                 // skip the ": "

   lstrcpy(pszPath, p);
}

//--------------------------------------------------------------------------
//
//  UpdateSearchStatus() -
//
//--------------------------------------------------------------------------

VOID
UpdateSearchStatus(HWND hwndLB, INT nCount)
{
   SetStatusText(0, SST_FORMAT|SST_RESOURCE, (LPWSTR) MAKEINTRESOURCE(IDS_SEARCHMSG), nCount);

   if (SearchInfo.hThread)
      SetStatusText(1, SST_RESOURCE, (LPWSTR) MAKEINTRESOURCE(IDS_SEARCHING));
   else
      SetStatusText(1, 0, szNULL);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     SearchWndProc
//
// Synopsis:
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


LRESULT
SearchWndProc(
   register HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   INT  iSel;
   HWND hwndLB;
   WCHAR szPath[MAXPATHLEN];
   WCHAR szTitle[128];
   WCHAR szMessage[MAXMESSAGELEN];

   hwndLB = GetDlgItem(hwnd, IDCW_LISTBOX);

   switch (uMsg) {

   case WM_COMPAREITEM:
#define lpcis ((LPCOMPAREITEMSTRUCT)lParam)
#define lpItem1 ((LPXDTA)(lpcis->itemData1))
#define lpItem2 ((LPXDTA)(lpcis->itemData2))

	   // simple name sort if no date; otherwise sort by date (newest on top)
	   if (SearchInfo.ftSince.dwHighDateTime == 0 && SearchInfo.ftSince.dwLowDateTime == 0)
		   return lstrcmpi(MemGetFileName(lpItem1), MemGetFileName(lpItem2));
	   else
		   return CompareFileTime(&lpItem2->ftLastWriteTime, &lpItem1->ftLastWriteTime);

#undef lpcis
#undef lpItem1
#undef lpItem2

   case WM_CLOSE:

      //
      // This should be ok since we can't pre-empt ourselves
      //
      if (SearchInfo.hThread) {
         SearchInfo.bCancel = TRUE;
         SearchInfo.eStatus = SEARCH_MDICLOSE;
      }
      hwndSearch = NULL;

      SendMessage(hwndMDIClient, WM_MDIDESTROY, (WPARAM) hwnd, 0L);

      return 0;

   case FS_GETDRIVE:

      //
      // Returns the letter of the corresponding directory
      //
      // !! BUGBUG !!
      //
      // returns error on UNC
      //
      SendMessage(hwnd, FS_GETDIRECTORY, COUNTOF(szPath), (LPARAM)szPath);
      return (LRESULT) CHAR_A + DRIVEID(szPath);

   case FS_GETDIRECTORY:

      GetSearchPath(hwnd, szPath);

      StripFilespec(szPath);        // remove the filespec
      AddBackslash(szPath);         // to be the same as DirWndProc
      lstrcpy((LPWSTR)lParam, szPath);
      break;

   case FS_GETFILESPEC:

      // the search window doesn't have a current directory
      GetSearchPath(hwnd, szPath);
      StripPath(szPath);                    // remove the path (leave the filespec)
      lstrcpy((LPWSTR)lParam, szPath);
      break;

   case FS_SETSELECTION:
      // wParam is the select(TRUE)/deselect(FALSE) param
      // lParam is the filespec to match against

      SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
      DSSetSelection(hwndLB, wParam != 0, (LPWSTR)lParam, TRUE);
      SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
      InvalidateRect(hwndLB, NULL, TRUE);
      break;

   case FS_GETSELECTION:
      return (LRESULT)DirGetSelection(NULL,
                                   hwnd,
                                   hwndLB,
                                   wParam,
                                   (BOOL *)lParam,
                                   NULL);
      break;

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

         //
         // update status bar
         // and inform SearchList to update the status bar
         //
         UpdateSearchStatus(hwndLB,(INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L));
         SearchInfo.bUpdateStatus = TRUE;

         //
         // if we are dirty, ask if we should update
         //
         UpdateIfDirty(hwnd);
      } else {

         //
         // Don't update status
         //
         SearchInfo.bUpdateStatus = FALSE;
      }
      break;

   case WM_FSC:

      //
      // if the search window is not active or FSCs are disabled
      // don't prompt now, wait till we get the end FSC or are
      // activated (above in WM_ACTIVATE)
      //
      if (cDisableFSC ||
         (hwnd != (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L)) ||
         (GetActiveWindow() != hwndFrame)) {

         SetWindowLongPtr(hwnd, GWL_FSCFLAG, TRUE);
         break;
      }

      SetWindowLongPtr(hwnd, GWL_FSCFLAG, FALSE);
      SendMessage(hwnd, FS_CHANGEDISPLAY, CD_SEARCHUPDATE, 0L);

      break;

   case FS_CHANGEDISPLAY:

      //
      // For safety, clear the CD_DONTSTEAL flag, although
      // it should never be set here (set by DrivesSetDrive)
      //
      wParam &= ~CD_DONTSTEAL;

      if (wParam == CD_VIEW || wParam == CD_SEARCHFONT) {
         dwNewView = GetWindowLongPtr(hwnd, GWL_VIEW);

         //
         // in case font changed, update maxExt
         //
         if (CD_SEARCHFONT == wParam) {

            //
            // Update dwEntries.. bogus since duplicated in iFileCount,
            // but quick fix.
            //
            MemLinkToHead(SearchInfo.lpStart)->dwEntries =
               (DWORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

            maxExt = GetMaxExtent(hwndLB, SearchInfo.lpStart, FALSE);
         }

         FixTabsAndThings(hwndLB,
                          (WORD *)GetWindowLongPtr(hwnd, GWL_TABARRAY),
                          maxExt + dxText,
                          0,
                          dwNewView);

         InvalidateRect(hwndLB, NULL, TRUE);

      } else {

         // Allow only 1 thread to be created!
         if (SearchInfo.hThread)
            return 0L;

         if (wParam == CD_SEARCHUPDATE) {
            LoadString(hAppInstance, IDS_SEARCHTITLE, szTitle, COUNTOF(szTitle));
            LoadString(hAppInstance, IDS_SEARCHREFRESH, szMessage, COUNTOF(szMessage));
			int msg = MessageBox(hwnd, szMessage, szTitle, MB_ABORTRETRYIGNORE | MB_ICONQUESTION);

			if (msg == IDABORT)
			{
				HWND hwndNext = GetWindow(hwndSearch, GW_HWNDNEXT);
				SendMessage(hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndNext, 0);
				SendMessage(hwndSearch, WM_CLOSE, 0, 0L);
			}
			if (msg != IDRETRY)
			{
				break;
			}
         }

         //
         // Now clear the listbox
         //
         ClearSearchLB(FALSE);

         //
         // is this a refresh?
         //

         if (!lParam) {
            GetSearchPath(hwnd, szPath);
         } else {
            lstrcpy(szPath, (LPWSTR)lParam);   // explicit re-search
         }

         LoadString(hAppInstance, IDS_SEARCHTITLE, szMessage, COUNTOF(szMessage));
         lstrcat(szMessage, szPath);
         SetWindowText(hwnd, szMessage);

         //
         // Init, just like SearchDlgProc
         //

         SearchInfo.iDirsRead = 0;
         SearchInfo.iFileCount = 0;
         SearchInfo.eStatus = SEARCH_NULL;
         SearchInfo.bCancel = FALSE;

         // Create our dialog!  (modeless)
         CreateDialog(hAppInstance, (LPWSTR) MAKEINTRESOURCE(SEARCHPROGDLG), hwndFrame, (DLGPROC) SearchProgDlgProc);

      }  // ELSE from wParam == CD_VIEW

      break;

   case WM_SETFOCUS:

      SetFocus(hwndLB);
      UpdateIfDirty(hwnd);
      return SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

   case WM_COMMAND:

      //
      // Was this a double-click?
      //
      switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
      case LBN_DBLCLK:

         SendMessage(hwndFrame,
                     WM_COMMAND,
                     GET_WM_COMMAND_MPS(IDM_OPEN, 0, 0));
         break;

      case LBN_SELCHANGE:
      {
         INT i;

         ExtSelItemsInvalidate();

         for (i = 0; i < iNumExtensions; i++) {
            (extensions[i].ExtProc)(hwndFrame, FMEVENT_SELCHANGE, 0L);
         }
         break;
      }
      }
      break;

   case WM_DESTROY:
   {
      HANDLE hMem;

      ClearSearchLB(FALSE);
      SearchInfo.hwndLB = NULL;

      if (hMem = (HANDLE)GetWindowLongPtr(hwnd, GWL_TABARRAY))
         LocalFree(hMem);

      break;
   }
   case WM_CREATE:
   {
      // globals used:
      //    SearchInfo.szSearch        path to start search at
      //    SearchInfo.bDontSearchSubs tells us not to do a recursive search

      RECT      rc;
      WORD      *pwTabs;

      GetClientRect(hwnd, &rc);
      hwndLB = CreateWindowEx(0L, szListbox, NULL,
         WS_CHILD | WS_BORDER | LBS_SORT | LBS_NOTIFY |
         LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL |
         LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT |
         WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
         -1, -1, rc.right+2, rc.bottom+2,
         hwnd, (HMENU)IDCW_LISTBOX,
         hAppInstance, NULL);

      if (!hwndLB)
         return -1L;

      if ((pwTabs = (WORD *)LocalAlloc(LPTR,sizeof(WORD) * MAX_TAB_COLUMNS)) == NULL)
         return -1L;

      hwndSearch = hwnd;
      SetWindowLongPtr(hwnd, GWL_TYPE,   TYPE_SEARCH);
      SetWindowLongPtr(hwnd, GWL_VIEW,   dwNewView);
      SetWindowLongPtr(hwnd, GWL_SORT,   IDD_NAME);
      SetWindowLongPtr(hwnd, GWL_ATTRIBS,ATTR_DEFAULT);
      SetWindowLongPtr(hwnd, GWL_FSCFLAG,   FALSE);
      SetWindowLongPtr(hwnd, GWL_HDTA, 0L);
      SetWindowLongPtr(hwnd, GWL_TABARRAY, (LPARAM)pwTabs);
      SetWindowLongPtr(hwnd, GWL_LASTFOCUS, (LPARAM)hwndLB);

      SetWindowLongPtr(hwnd, GWL_LISTPARMS, (LPARAM)hwnd);
      SetWindowLongPtr(hwnd, GWL_IERROR, 0);

      //
      // Fill the listbox
      //
      SendMessage(hwndLB, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

      SearchInfo.hwndLB = hwndLB;

      //
      // Create our dialog!  (modeless)
      //
      CreateDialog(hAppInstance, (LPWSTR) MAKEINTRESOURCE(SEARCHPROGDLG), hwndFrame, (DLGPROC) SearchProgDlgProc);

      break;
   }

   case WM_DRAGLOOP:

      //
      // WM_DRAGLOOP is sent to the source window as the object is moved.
      //
      //    wParam: TRUE if the object is currently over a droppable sink
      //    lParam: LPDROPSTRUCT
      //

      // based on current drop location scroll the sink up or down
      DSDragScrollSink((LPDROPSTRUCT)lParam);

      //
      // DRAGLOOP is used to turn the source bitmaps on/off as we drag.
      //

      DSDragLoop(hwndLB, wParam, (LPDROPSTRUCT)lParam);
      break;

   case WM_CONTEXTMENU:
	   ActivateCommonContextMenu(hwnd, hwndLB, lParam);
	   break;

   case WM_DRAGSELECT:

      //
      // WM_DRAGSELECT is sent to a sink whenever an new object is dragged
      // inside of it.
      //
      //    wParam: TRUE if the sink is being entered, FALSE if it's being
      //            exited.
      //    lParam: LPDROPSTRUCT
      //

      //
      // DRAGSELECT is used to turn our selection rectangle on or off.
      //
#define lpds ((LPDROPSTRUCT)lParam)

      //
      // Turn on/off status bar
      //

      SendMessage(hwndStatus,
                  SB_SETTEXT,
                  SBT_NOBORDERS|255,
                  (LPARAM)szNULL);

      SendMessage(hwndStatus, SB_SIMPLE, (wParam ? 1 : 0), 0L);
      UpdateWindow(hwndStatus);

      iSelHighlight = lpds->dwControlData;
      DSRectItem(hwndLB, iSelHighlight, (BOOL)wParam, TRUE);
      break;

   case WM_DRAGMOVE:

      //
      // WM_DRAGMOVE is sent to a sink as the object is being dragged
      // within it.
      //
      //   wParam: Unused
      //   lParam: LPDROPSTRUCT
      //

      //
      // DRAGMOVE is used to move our selection rectangle among sub-items.
      //

#define lpds ((LPDROPSTRUCT)lParam)

      //
      // Get the subitem we are over.
      //
      iSel = lpds->dwControlData;

      //
      // Is it a new one?
      //
      if (iSel == iSelHighlight)
         break;

      //
      // Yup, un-select the old item.
      //
      DSRectItem(hwndLB, iSelHighlight, FALSE, TRUE);

      //
      // Select the new one.
      iSelHighlight = iSel;
      DSRectItem(hwndLB, iSel, TRUE, TRUE);
      break;

   case WM_DRAWITEM:
   {
      LPDRAWITEMSTRUCT lpLBItem;
      PWORD pwTabs;
      DWORD dwView = GetWindowLongPtr(hwnd, GWL_VIEW);

      lpLBItem = (LPDRAWITEMSTRUCT)lParam;
      iSel = lpLBItem->itemID;

      if (iSel < 0)
         break;

      if (maxExt > maxExtLast) {

         pwTabs = (WORD *)GetWindowLongPtr(hwndSearch, GWL_TABARRAY);

         //
         // Need to update tabs
         //
         FixTabsAndThings(SearchInfo.hwndLB,
                          pwTabs,
                          maxExt+dxText,
                          0,
                          dwView);

         maxExtLast = maxExt;

         //
         // If there is > 1 column (i.e., wider filenames shift over
         // size/date/time/flags column), then we must invalidate the
         // entire rect to realign everything.
         //
         // Note that we explicitly ignore VIEW_DOSNAMES, since this
         // is not displayed in the search window.
         //
         if (dwView & ~VIEW_DOSNAMES)
            InvalidateRect(SearchInfo.hwndLB, NULL, TRUE);
      }

      DrawItem(hwnd,
               GetWindowLongPtr(hwnd, GWL_VIEW),
               (LPDRAWITEMSTRUCT)lParam,
               TRUE);
      break;
   }

   case WM_DROPOBJECT:

      return DSDropObject(hwnd, hwndLB, (LPDROPSTRUCT)lParam, TRUE);

   case WM_LBTRACKPOINT:
      return DSTrackPoint(hwnd, hwndLB, wParam, lParam, TRUE);

   case WM_MEASUREITEM:
#define pLBMItem ((LPMEASUREITEMSTRUCT)lParam)

      pLBMItem->itemHeight = dyFileName;
      break;
#undef pLBItem
   case WM_QUERYDROPOBJECT:

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

   case WM_SIZE:
      if (wParam != SIZEICONIC) {
         MoveWindow(GetDlgItem(hwnd, IDCW_LISTBOX),
                    -1, -1,
                    LOWORD(lParam)+2,
                    HIWORD(lParam)+2,
                    TRUE);
      }

      /*** FALL THROUGH ***/

   default:
      return(DefMDIChildProc(hwnd, uMsg, wParam, lParam));
   }
   return(0L);
}


VOID
UpdateIfDirty(HWND hwnd)
{
   if (GetWindowLongPtr(hwnd, GWL_FSCFLAG)) {
      //
      // I am clean
      //
      SetWindowLongPtr(hwnd, GWL_FSCFLAG, FALSE);
      SendMessage(hwnd, FS_CHANGEDISPLAY, CD_SEARCHUPDATE, 0L);
   }
}


VOID
LockSearchFile()
{
   EnableMenuItem( GetMenu(hwndFrame), IDM_SEARCH, MF_BYCOMMAND | MF_GRAYED );
}

VOID
UnlockSearchFile()
{
   EnableMenuItem( GetMenu(hwndFrame), IDM_SEARCH, MF_BYCOMMAND | MF_ENABLED );
}


LRESULT CALLBACK
SearchProgDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   DWORD dwIgnore;
   WCHAR szTemp[MAXPATHLEN];
   INT i;
   DRIVE drive;

   switch (uMsg) {
   case WM_INITDIALOG:

      SearchInfo.hSearchDlg = hDlg;

      //
      // Initialize display
      //
      SendMessage(hwndFrame, FS_SEARCHUPDATE,
         SearchInfo.iDirsRead, SearchInfo.iFileCount);

      lstrcpy(szTemp,SearchInfo.szSearch);
      StripPath(szTemp);
      SetDlgItemText(hDlg, IDD_NAME, szTemp);

	  if (SearchInfo.ftSince.dwHighDateTime == 0 && SearchInfo.ftSince.dwLowDateTime == 0) {
		  SetDlgItemText(hDlg, IDD_DATE, TEXT("n/a"));
	  }
	  else {
		  SYSTEMTIME st;
		  FILETIME ftLocal;
		  FileTimeToLocalFileTime(&SearchInfo.ftSince, &ftLocal);
		  FileTimeToSystemTime(&ftLocal, &st);
		  if (st.wHour == 0 && st.wMinute == 0)
			  wsprintf(szTemp, TEXT("%4d-%2d-%2d"), st.wYear, st.wMonth, st.wDay);
		  else
			  wsprintf(szTemp, TEXT("%4d-%2d-%2d %02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
		  SetDlgItemText(hDlg, IDD_DATE, szTemp);
	  }

      lstrcpy(szTemp,SearchInfo.szSearch);
      StripFilespec(szTemp);
      SetDlgItemPath(hDlg, IDD_PATH, szTemp);


      // Speedy switch to search window (change drive)
      // And also check if bCasePreserved.

      if (CHAR_COLON == SearchInfo.szSearch[1]) {
         drive = DRIVEID(SearchInfo.szSearch);

         SearchInfo.bCasePreserved =  IsCasePreservedDrive(drive);

         for (i=0; i<cDrives; i++) {
            if (drive == rgiDrive[i]) {
               break;
            }
         }

         // don't do this if drive doesn't exist

         if (i != cDrives) {

            SetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEIND, i);
            SetWindowLongPtr(hwndDriveBar, GWL_CURDRIVEFOCUS, i);

            UpdateStatus(hwndSearch);

            SelectToolbarDrive(i);
            InvalidateRect(hwndDriveBar, NULL, TRUE);
            UpdateWindow(hwndDriveBar);
         }
      } else {

         //
         // LATER: handle UNC case
         //
         // Should actually hit network to see if case preserving
         //
         SearchInfo.bCasePreserved =  FALSE;
      }

      //
      // Add thread here!
      //
      if (!SearchInfo.hThread) {
         SearchInfo.hThread = CreateThread( NULL,        // Security
            0L,                                          // Stack Size
            (LPTHREAD_START_ROUTINE)SearchDrive,
            NULL,
            0L,
            &dwIgnore );
      }

      return TRUE;
   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
      case IDCANCEL:
         SearchInfo.bCancel = TRUE;
         SearchInfo.eStatus = SEARCH_CANCEL;
         return TRUE;
      case IDD_HIDE:
         DestroyWindow(SearchInfo.hSearchDlg);
         SearchInfo.hSearchDlg = NULL;

         return TRUE;
      }
      return TRUE;

   default:
      return 0;
   }
   return 0;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     SearchEnd
//
// Synopsis: Handles display window of search results/brings up hwndSearch
//
// IN VOID
//
// Return:   VOID
//
//
// Assumes:  At this point, either hwndSearch & hwndLB are valid,
//           or eStatus == SEARCH_MDICLOSE.
//
//           This is safe since eStatus is set to SEARCH_MDICLOSE before
//           hwnd{Search,LB} are destroyed.
//
//           Must be called by main thread.
//
// Effects:
//
//
// Notes:    This call is atomic with respect to all SendMessages in
//           the main UI thread.  (This is called only by FS_SEARCHEND
//           in FrameWndProc)  We can safely ignore certain race conditions:
//
//           hwndSearch
//
/////////////////////////////////////////////////////////////////////

VOID
SearchEnd(VOID)
{
   HWND hwndMDIChild;

   if (SEARCH_MDICLOSE == SearchInfo.eStatus) {
      //
      // Free up the data structure
      // Actually called by main thread, but on behalf of worker thread.
      //
      ClearSearchLB(TRUE);
   } else {
      InvalidateRect(SearchInfo.hwndLB, NULL, TRUE);
   }

   if (SEARCH_ERROR == SearchInfo.eStatus) {

      LoadString(hAppInstance, IDS_SEARCHTITLE, szTitle, COUNTOF(szTitle));

      FormatError(TRUE, szMessage, COUNTOF(szMessage), SearchInfo.dwError);
      MessageBox(hwndFrame, szMessage, szTitle, MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);

      if (0 == SearchInfo.iRet)
         goto CloseWindow;

   } else if (0 == SearchInfo.iRet && SEARCH_MDICLOSE != SearchInfo.eStatus) {

      LoadString(hAppInstance, IDS_SEARCHTITLE, szTitle, COUNTOF(szTitle));
      LoadString(hAppInstance, IDS_SEARCHNOMATCHES, szMessage, COUNTOF(szMessage));
      MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);

CloseWindow:

      ShowWindow(hwndSearch, SW_HIDE);
      PostMessage(hwndSearch, WM_CLOSE, 0, 0L);

      return;

   }

   if (SEARCH_MDICLOSE != SearchInfo.eStatus) {

      hwndMDIChild = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

      if (hwndMDIChild) {
         ShowWindow(hwndSearch,
            GetWindowLongPtr(hwndMDIChild, GWL_STYLE) & WS_MAXIMIZE ?
               SW_SHOWMAXIMIZED :
               SW_SHOWNORMAL);
      }

      SendMessage(hwndMDIClient, WM_MDIACTIVATE, (WPARAM) hwndSearch, 0L);

	  // for some reason when the results list is short and the window already maximized,
	  // the focus doesn't get set on the results window (specifically the LB); do so no.
	  PostMessage(hwndSearch, WM_SETFOCUS, 0, 0L);
	  
	  UpdateSearchStatus(SearchInfo.hwndLB,
         (INT)SendMessage(SearchInfo.hwndLB, LB_GETCOUNT, 0, 0L));
   }
}

INT
SearchDrive()
{
   maxExtLast = SEARCH_FILE_WIDTH_DEFAULT;

   FixTabsAndThings(SearchInfo.hwndLB,
                    (WORD *)GetWindowLongPtr(hwndSearch, GWL_TABARRAY),
                    maxExtLast,
                    0,
                    GetWindowLongPtr(hwndSearch, GWL_VIEW));

   SearchInfo.iRet = FillSearchLB(SearchInfo.hwndLB,
                                  SearchInfo.szSearch,
                                  !SearchInfo.bDontSearchSubs,
								  SearchInfo.bIncludeSubDirs);

   if (SearchInfo.hThread) {
      CloseHandle(SearchInfo.hThread);
   } else {
      //
      // Error case?
      //
   }

   SendMessage(hwndFrame, FS_SEARCHEND,0,0L);

   ExitThread(0L);
   return 0;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     ClearSearchLB
//
// Synopsis: Clears out memory associated with search LB/kills LB.
//
// INC  bWorkerCall  --  T/F if called by worker thread
//
// Return:   VOID
//
//
// Assumes:  * hThread non-NULL if worker thread up
//             (if hThread=NULL, memory already freed)
//           * SearchInfo.pBrickSearch not changing
//
//           If called by main thread, hwndLB is valid.
//
// Effects:  SearchInfo.pBrickSearch freed
//           hwndLB cleared, if it exists.
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////


VOID
ClearSearchLB(BOOL bWorkerCall)
{
   //
   // When to free up:
   //
   // 1. Worker thread is dead (called by main thread)   OR
   // 2. Search window is gone (called by worker)
   //

   if (!SearchInfo.hThread || bWorkerCall) {

      MemDelete(SearchInfo.lpStart);
   }

   if (!bWorkerCall) {

      ExtSelItemsInvalidate();
      SendMessage(SearchInfo.hwndLB, LB_RESETCONTENT, 0, 0);
   }
}

