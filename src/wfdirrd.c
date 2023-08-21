/********************************************************************

   wfDirRd.c

   Background directory read support

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"

typedef enum {
   EDIRABORT_NULL        = 0,
   EDIRABORT_READREQUEST = 1,
   EDIRABORT_WINDOWCLOSE = 2,
} EDIRABORT;

typedef struct _EXT_LOCATION {
   HKEY hk;
   LPTSTR lpszNode;
} EXTLOCATION, *PEXTLOCATION;

EXTLOCATION aExtLocation[] = {
   { HKEY_CLASSES_ROOT, L"" },
#ifdef ASSOC
   { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Extensions" },
#endif
   { (HKEY)0, NULL }
};


HANDLE hEventDirRead;
HANDLE hThreadDirRead;
BOOL bDirReadRun;

BOOL bDirReadAbort;
BOOL bDirReadRebuildDocString;

CRITICAL_SECTION CriticalSectionDirRead;

//
// Prototypes
//
DWORD WINAPI DirReadServer(LPVOID lpvParm);
LPXDTALINK CreateDTABlockWorker(HWND hwnd, HWND hwndDir);
LPXDTALINK StealDTABlock(HWND hwndCur, LPWSTR pPath, DWORD dwAttribs);
BOOL IsNetDir(LPWSTR pPath, LPWSTR pName);
VOID DirReadAbort(HWND hwnd, LPXDTALINK lpStart, EDIRABORT eDirAbort);
LONG WFRegGetValueW(HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData);

BOOL
InitDirRead(VOID)
{
   DWORD dwIgnore;

   bDirReadRun = TRUE;
   InitializeCriticalSection(&CriticalSectionDirRead);

   hEventDirRead = CreateEvent(NULL, FALSE, FALSE, NULL);

   if (!hEventDirRead) {

Error:

      bDirReadRun = FALSE;
      DeleteCriticalSection(&CriticalSectionDirRead);

      return FALSE;
   }


   hThreadDirRead = CreateThread(NULL,
                                 0L,
                                 DirReadServer,
                                 NULL,
                                 0L,
                                 &dwIgnore);

   if (!hThreadDirRead) {

      CloseHandle(hEventDirRead);
      goto Error;
   }

   return TRUE;
}


VOID
DestroyDirRead(VOID)
{
   if (bDirReadRun) {

      bDirReadRun = FALSE;

      SetEvent(hEventDirRead);
      WaitForSingleObject(hThreadDirRead, INFINITE);

      CloseHandle(hEventDirRead);
      CloseHandle(hThreadDirRead);
      DeleteCriticalSection(&CriticalSectionDirRead);
   }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     CreateDTABlock
//
// Synopsis: Creates a DTA block for the directory window
//
// IN    hwnd        --    window of destination (?)
// IN    pPath       --    Path to search for files from
// IN    dwAttribs   --    files must satisfy these attributes
// IN    bDontSteal  --    TRUE = Always read off disk
//                   --    FALSE = Allow steal from other window
//
// Return:   LPXDTALINK Local memory block
//           NULL if not currently available
//
// Assumes:  pPath fully qualified with leading drive letter
//           Directory set in MDI title before calling
//
// Effects:  Updates GWL_HDTA{,ABORT}
//
// Notes:    The first DTA chunk has a head entry which holds
//           the number of item/size.  See header.
//
//           lpStart->head.alpxdtaSorted is set to NULL
//
//           dwTotal{Count,Size} handles "real" files only (no "..").
//           They are used for caching.  dwEntries holds the actual number
//           of listbox entries.
//
//           Called from Main thread.
//
/////////////////////////////////////////////////////////////////////

LPXDTALINK
CreateDTABlock(
   HWND hwnd,
   LPWSTR pPath,
   DWORD dwAttribs,
   BOOL bDontSteal)
{
   LPXDTALINK lpStart;
   MSG msg;

   SetWindowLongPtr(hwnd, GWL_IERROR, ERROR_SUCCESS);

   if (!bDontSteal && (lpStart = StealDTABlock(hwnd, pPath, dwAttribs))) {

      if (PeekMessage(&msg,
                      NULL,
                      WM_KEYDOWN,
                      WM_KEYDOWN,
                      PM_NOREMOVE)) {

         if (msg.wParam == VK_UP || msg.wParam == VK_DOWN) {

            MemDelete(lpStart);
            goto Abort;
         }
      }

      //
      // Abort the dir read, since we have stolen the correct thing.
      // (bAbort must be true to abort us, but the following call sets
      // lpStart to non-null, which prevents re-reading).
      //
      DirReadAbort(hwnd, lpStart, EDIRABORT_NULL);

      return lpStart;
   }

Abort:

   DirReadAbort(hwnd, NULL, EDIRABORT_READREQUEST);
   return NULL;
}


VOID
DirReadAbort(
   HWND hwnd,
   LPXDTALINK lpStart,
   EDIRABORT eDirAbort)
{
   //
   // This is the only code that issues the abort!
   //
   EnterCriticalSection(&CriticalSectionDirRead);

   FreeDTA(hwnd);

   SetWindowLongPtr(hwnd, GWL_HDTA, (LPARAM)lpStart);
   SetWindowLongPtr(hwnd, GWL_HDTAABORT, eDirAbort);
   bDirReadAbort = TRUE;

   SetEvent(hEventDirRead);

   LeaveCriticalSection(&CriticalSectionDirRead);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     StealDTABlock
//
// Synopsis: Steals a DTA block based on title bar directory
//
// hwnd      hwnd of target steal
// hwndDir   Dir to receive
// dwAttribs Attribs we are going to have
//
// Return:   Copied block or NULL
//
//
// Assumes:  If the title bar is set, the directory has been fully read.
//
//
// Effects:
//
//
// Notes:    Called from Main thread.
//
/////////////////////////////////////////////////////////////////////

LPXDTALINK
StealDTABlock(
   HWND hwndCur,
   LPWSTR pPath,
   DWORD dwAttribs)
{
   HWND hwndDir;
   HWND hwnd;
   WCHAR szPath[MAXPATHLEN];

   LPXDTALINK lpStart, lpStartCopy;
   INT iError;

   for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
      hwnd;
      hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

      if ((hwndDir = HasDirWindow(hwnd)) && (hwndDir != hwndCur)) {

         GetMDIWindowText(hwnd, szPath, COUNTOF(szPath));

         if ((dwAttribs == (DWORD)GetWindowLongPtr(hwnd, GWL_ATTRIBS)) &&
            !lstrcmpi(pPath, szPath) &&
            (lpStart = (LPXDTALINK)GetWindowLongPtr(hwndDir, GWL_HDTA))) {

            iError = (INT)GetWindowLongPtr(hwndDir, GWL_IERROR);
            if (!iError || IDS_NOFILES == iError) {

               lpStartCopy = MemClone(lpStart);

               return lpStartCopy;
            }
         }
      }
   }

   return NULL;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     FreeDTA
//
// Synopsis: Clears the DTA
//
// hwnd      Window to free (either hwndDir or hwndSearch)
//
// Return:   VOID
//
// Assumes:
//
// Effects:
//
//
// Notes:    !! Must not be called from worker thread !!
//           Directory windows assumed to be synchronized by
//           SendMessage to Main UI Thread.  (Search windows ok)
//
/////////////////////////////////////////////////////////////////////


VOID
FreeDTA(HWND hwnd)
{
   LPXDTALINK lpxdtaLink;

   lpxdtaLink = (LPXDTALINK)GetWindowLongPtr(hwnd, GWL_HDTA);

   SetWindowLongPtr(hwnd, GWL_HDTA, 0L);

   //
   // Only delete it if it's not in use.
   //
   if (lpxdtaLink) {

       if (MemLinkToHead(lpxdtaLink)->fdwStatus & LPXDTA_STATUS_READING) {

           MemLinkToHead(lpxdtaLink)->fdwStatus |= LPXDTA_STATUS_CLOSE;

       } else {

           MemDelete(lpxdtaLink);
       }
   }
}


VOID
DirReadDestroyWindow(HWND hwndDir)
{
   DirReadAbort(hwndDir, NULL, EDIRABORT_WINDOWCLOSE);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     DirReadDone
//
// Synopsis: Called by main thread (sync'd by SM)
//
// Return:   LRESULT
//
// Assumes:
//
// Effects:
//
//
// Notes:    Main Thread ONLY!
//
/////////////////////////////////////////////////////////////////////

LPXDTALINK
DirReadDone(
   HWND hwndDir,
   LPXDTALINK lpStart,
   INT iError)
{
   HWND hwndLB = GetDlgItem(hwndDir, IDCW_LISTBOX);
   HWND hwndParent = GetParent(hwndDir);
   WCHAR szPath[MAXPATHLEN];
   HWND hwndNext;


   EDIRABORT eDirAbort;

   eDirAbort = (EDIRABORT)GetWindowLongPtr(hwndDir, GWL_HDTAABORT);

   //
   // Last chance check for abort
   //
   if ((eDirAbort & (EDIRABORT_READREQUEST|EDIRABORT_WINDOWCLOSE)) ||
      GetWindowLongPtr(hwndDir, GWL_HDTA)) {

      //
      // We don't want it
      //
      return NULL;
   }

   GetMDIWindowText(hwndParent, szPath, COUNTOF(szPath));
   StripFilespec(szPath);

   ModifyWatchList(hwndParent,
                   szPath,
                   FILE_NOTIFY_CHANGE_FLAGS);

   SetWindowLongPtr(hwndDir, GWL_IERROR, iError);
   SetWindowLongPtr(hwndDir, GWL_HDTA, (LPARAM)lpStart);

   //
   // Remove the "reading" token
   //
   SendMessage(hwndLB, LB_DELETESTRING, 0, 0);

   FillDirList(hwndDir, lpStart);

   SetWindowLongPtr(hwndDir, GWLP_USERDATA, 0);

   hwndNext = (HWND)GetWindowLongPtr(hwndDir, GWL_NEXTHWND);
   if (hwndNext)
   {
       SendMessage(hwndDir, FS_TESTEMPTY, 0L, (LPARAM)hwndNext);
   }
   SetWindowLongPtr(hwndDir, GWL_NEXTHWND, 0L);

   //
   // Refresh display, but don't hit disk
   //
   SPC_SET_INVALID(qFreeSpace);

   return lpStart;
}


VOID
BuildDocumentString()
{
   bDirReadRebuildDocString = TRUE;
   SetEvent(hEventDirRead);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     BuildDocumentStringWorker
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
// Notes:    Runs in main thread, but sync'd with worker via SendMessage.
//
/////////////////////////////////////////////////////////////////////

VOID
BuildDocumentStringWorker()
{
   LPTSTR            p;
   INT               uLen;
   TCHAR             szT[EXTSIZ + 1];
   INT               i,j;
   LPTSTR            pszDocuments = NULL;
   HKEY hk;
   BOOL bCloseKey;

   DWORD dwStatus;

   //
   // Reinitialize the ppDocBucket struct
   //
   DocDestruct(ppDocBucket);
   ppDocBucket = DocConstruct();

   if (!ppDocBucket)
      goto Return;

   FillDocType(ppDocBucket, L"Documents", szNULL);

#ifdef NEC_98
    uLen = 96;      // 96 = 128 - 32.  Trying to match code added to wfinit.c
                    // in FE merge in Win95 source base.  ** for PC-7459 **
#else
    uLen = 0;
#endif

   do {

      uLen += 32;

      if (pszDocuments)
         LocalFree((HLOCAL) pszDocuments);

      pszDocuments = LocalAlloc(LMEM_FIXED, uLen*sizeof(*pszDocuments));

      if (!pszDocuments) {
         goto Return;
      }

   } while (GetProfileString(szExtensions,
                             NULL,
                             szNULL,
                             pszDocuments,
                             uLen) == (DWORD)uLen-2);

   //
   // Step through each of the keywords in 'szDocuments' changing NULLS into
   // spaces until a double-NULL is found.
   //
   p = pszDocuments;
   while (*p) {

      DocInsert(ppDocBucket, p, 0);

      //
      // Find the next NULL.
      //
      while (*p)
         p++;

      p++;
   }

   LocalFree((HLOCAL)pszDocuments);

   for(j=0; aExtLocation[j].lpszNode; j++) {

      if (aExtLocation[j].lpszNode[0]) {

         if (RegOpenKey(aExtLocation[j].hk,aExtLocation[j].lpszNode,&hk)) {
            continue;
         }
         bCloseKey = TRUE;

      } else {
         hk = aExtLocation[j].hk;
         bCloseKey = FALSE;
      }

      //
      // now enumerate the classes in the registration database and get all
      // those that are of the form *.ext
      //
      for (i=0, dwStatus = 0L; ERROR_NO_MORE_ITEMS!=dwStatus; i++) {
         DWORD cbClass, cbIconFile;
         TCHAR szClass[MAXPATHLEN];
         TCHAR szIconFile[MAXPATHLEN];

         dwStatus = RegEnumKey(hk, (DWORD)i, szT, COUNTOF(szT));

         if (dwStatus || szT[0] != '.') {

            //
            // either the class does not start with . or it has a greater
            // than 3 byte extension... skip it.
            //
            continue;
         }

         cbClass = sizeof(szClass);
         cbIconFile = 0;
         if (WFRegGetValueW(hk, szT, NULL, NULL, szClass, &cbClass) == ERROR_SUCCESS)
         {
            DWORD cbClass2;
            TCHAR szClass2[MAXPATHLEN];

            cbClass2 = sizeof(szClass2);
            lstrcat(szClass, L"\\CurVer");
            if (WFRegGetValueW(hk, szClass, NULL, NULL, szClass2, &cbClass2) == ERROR_SUCCESS)
            {
               lstrcpy(szClass, szClass2);
            }
            else
            {
               szClass[lstrlen(szClass)-7] = '\0';
            }

            cbIconFile = sizeof(szIconFile);
            lstrcat(szClass, L"\\DefaultIcon");
            if (WFRegGetValueW(hk, szClass, NULL, NULL, szIconFile, &cbIconFile) != ERROR_SUCCESS)
            {
               cbIconFile = 0;
            }
         }

         DocInsert(ppDocBucket, szT+1, cbIconFile != 0 ? szIconFile : NULL);
      }

      if (bCloseKey)
         RegCloseKey(hk);
   }

Return:

   return;
}

/********************************************************************

   The following code will be run in a separate thread!

********************************************************************/


DWORD
WINAPI
DirReadServer(
   LPVOID lpvParm)
{
   HWND hwnd;
   HWND hwndDir;
   BOOL bRead;

   while (bDirReadRun) {

      WaitForSingleObject(hEventDirRead, INFINITE);

Restart:
      //
      // Delete all extra pibs
      //
      if (!bDirReadRun)
         break;

      if (bDirReadRebuildDocString) {
         bDirReadRebuildDocString = FALSE;
         SendMessage(hwndFrame, FS_REBUILDDOCSTRING, 0, 0L);
      }

      //
      // bDirReadAbort means that we need to abort the current read
      // and rescan from the beginning.  Since we are here right now, we can
      // clear bDirReadAbort since we are about to rescan anyway.
      //
      bDirReadAbort = FALSE;

      //
      // Go through z order and update reads.
      //
      for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

         if (hwndDir = HasDirWindow(hwnd)) {

            //
            // Critical section since GWL_HDTA+HDTAABORT reads must
            // be atomic.
            //
            EnterCriticalSection(&CriticalSectionDirRead);

            bRead = !GetWindowLongPtr(hwndDir, GWL_HDTA) &&
                       EDIRABORT_READREQUEST ==
                          (EDIRABORT)GetWindowLongPtr(hwndDir, GWL_HDTAABORT);

            LeaveCriticalSection(&CriticalSectionDirRead);

            if (bRead) {
               CreateDTABlockWorker(hwnd, hwndDir);
               goto Restart;
            }
            SetWindowLongPtr(hwndDir, GWLP_USERDATA, 0);
         }
      }
   }

   return 0;
}


LPXDTALINK
CreateDTABlockWorker(
   HWND hwnd,
   HWND hwndDir)
{
   LPWSTR pName;
   PDOCBUCKET pDoc, pProgram;

   LFNDTA lfndta;

   LPXDTALINK lpLinkLast;
   LPXDTAHEAD lpHead;

   LPXDTA lpxdta;

   INT iBitmap;
   DRIVE drive;

   LPWSTR lpTemp;
   HWND hwndTree;

   BOOL bCasePreserved;
   BOOL bAbort;

   WCHAR szPath[MAXPATHLEN];
   WCHAR szLinkDest[MAXPATHLEN];

   DWORD dwAttribs;
   LPXDTALINK lpStart;

   INT iError = 0;

   lpStart = MemNew();

   if (!lpStart) {
      goto CDBMemoryErr;
   }

   //
   // Checking abort and reading current dir must be atomic,
   // since a directory change causes an abort.
   //
   EnterCriticalSection(&CriticalSectionDirRead);

   GetMDIWindowText(hwnd, szPath, COUNTOF(szPath));
   SetWindowLongPtr(hwndDir, GWL_HDTAABORT, EDIRABORT_NULL);

   LeaveCriticalSection(&CriticalSectionDirRead);

   dwAttribs = (DWORD)GetWindowLongPtr(hwnd, GWL_ATTRIBS);

   //
   // get the drive index assuming path is
   // fully qualified...
   //
   drive = DRIVEID(szPath);
   bCasePreserved = IsCasePreservedDrive(drive);

   lpHead = MemLinkToHead(lpStart);
   lpLinkLast = lpStart;

 RestartOverFindFirst:
   if (!WFFindFirst(&lfndta, szPath, dwAttribs & ATTR_ALL)) {

      //
      // Try again!  But first, see if the directory was invalid!
      //
      if (ERROR_PATH_NOT_FOUND == lfndta.err ||
         ERROR_INVALID_REPARSE_DATA == lfndta.err ||
         ERROR_SYMLINK_CLASS_DISABLED == lfndta.err ||
         ERROR_CANT_ACCESS_FILE == lfndta.err) {

         iError = IDS_BADPATHMSG;
         goto InvalidDirectory;
      }

      if (!IsTheDiskReallyThere(hwndDir, szPath, FUNC_EXPAND, FALSE)) {
         if (IsRemoteDrive(drive))
            iError = IDS_DRIVENOTAVAILABLE;
         goto CDBDiskGone;
      }

      //
      // break out of this loop if we were returned something
      // other than PATHNOTFOUND
      //
      if (!WFFindFirst(&lfndta, szPath, dwAttribs & ATTR_ALL)) {

         switch (lfndta.err) {
         case ERROR_PATH_NOT_FOUND:
         case ERROR_CANT_ACCESS_FILE:
         case ERROR_INVALID_REPARSE_DATA:
         case ERROR_SYMLINK_CLASS_DISABLED:

InvalidDirectory:

            //
            // LATER: fix this so that it comes up only
            // when not switching x: from \\x\x to \\y\y
            //
            // If the path is not found, but the disk is there, then go to
            // the root and try again; if already at the root, then exit
            //
            if (!(lpTemp=StrRChr(szPath, NULL, CHAR_BACKSLASH)) ||
               (lpTemp - szPath <= 2)) {

               goto CDBDiskGone;
            }

            if (hwndTree=HasTreeWindow(hwnd)) {

               // Check if it is a Reparse Point
               lpTemp[0] = '\0';
               DWORD attr = GetFileAttributes(szPath);
               lpTemp[0] = CHAR_BACKSLASH;
               if (attr & ATTR_REPARSE_POINT) {
                  // For dead Reparse Points just tell that the directory could not be read
                  break;
               } else {
               //
               // If we changed dirs, and there is a tree window, set the
               // dir to the root and collapse it
               // Note that lpTemp-szPath>2, szPath[3] is not in the file spec
               //
               szPath[3] = CHAR_NULL;
               SendMessage(hwndTree, TC_SETDIRECTORY, 0, (LPARAM)szPath);
               SendMessage(hwndTree, TC_COLLAPSELEVEL, 0, 0L);
Fail:
               MemDelete(lpStart);
               return NULL;
            }
            }

            lstrcpy(szPath+3, lpTemp+1);

            SendMessage(hwndDir,
                        FS_CHANGEDISPLAY,
                        CD_PATH | CD_ALLOWABORT,
                        (LPARAM)szPath);

            goto Fail;

         case ERROR_FILE_NOT_FOUND:
         case ERROR_NO_MORE_FILES:

            break;

         case ERROR_ACCESS_DENIED:
         {
            // Strip *.*
            WCHAR szTemp[2 * MAXPATHLEN];
            lstrcpy(szTemp, szPath);
            StripFilespec(szTemp);

            DWORD tag = DecodeReparsePoint(szTemp, szLinkDest, COUNTOF(szLinkDest));
            if (tag != IO_REPARSE_TAG_RESERVED_ZERO)
            {
               lstrcpy(szPath, szLinkDest);
               AppendToPath(szPath, szStarDotStar);
               goto RestartOverFindFirst;
            }
            else
            {
               iError = IDS_NOACCESSDIR;
            }
         }
         break;

         default:
            {
               WCHAR szText[MAXMESSAGELEN];
               WCHAR szTitle[MAXTITLELEN];

               iError = IDS_COPYERROR + FUNC_EXPAND;

               LoadString(hAppInstance,
                          IDS_COPYERROR+FUNC_EXPAND,
                          szTitle,
                          COUNTOF(szTitle));

               FormatError(TRUE, szText, COUNTOF(szText), lfndta.err);

               MessageBox(hwndDir,
                          szText,
                          szTitle,
                          MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);
            }
            break;
         }
      }
   }

   //
   // Find length of directory and trailing backslash.
   //
   if (!(lpTemp=StrRChr(szPath, NULL, CHAR_BACKSLASH)))
      goto CDBDiskGone;

   //
   // Always show .. if this is not the root directory.
   //
   if ((lpTemp - szPath > 3) && (dwAttribs & ATTR_DIR)) {

      //
      // Add a DTA to the list.
      //
      lpHead->dwEntries++;

      lpxdta = MemAdd(&lpLinkLast, 0, 0);

      if (!lpxdta)
         goto CDBMemoryErr;

      //
      // Fill the new DTA with a fudged ".." entry.
      //
      lpxdta->dwAttrs = ATTR_DIR | ATTR_PARENT;
      lpxdta->byBitmap = BM_IND_DIRUP;
      lpxdta->pDocB = NULL;


      MemGetFileName(lpxdta)[0] = CHAR_NULL;
      MemGetAlternateFileName(lpxdta)[0] = CHAR_NULL;

      //
      // Date time size name not set since they are ignored
      //
   }

   if (lfndta.err)
      goto CDBDiskGone;

   while (TRUE) {

      pName = lfndta.fd.cFileName;

      //
      // be safe, zero unused DOS dta bits
      //
      lfndta.fd.dwFileAttributes &= (ATTR_USED | ATTR_JUNCTION | ATTR_SYMBOLIC);

      //
      // filter unwanted stuff here based on current view settings
      //
      pDoc = NULL;
      pProgram = NULL;
      if (!(lfndta.fd.dwFileAttributes & ATTR_DIR)) {

         pProgram = IsProgramFile(pName);
         pDoc     = IsDocument(pName);

         //
         // filter programs and documents
         //
         if (!(dwAttribs & ATTR_PROGRAMS) && pProgram)
            goto CDBCont;
         if (!(dwAttribs & ATTR_DOCS) && pDoc)
            goto CDBCont;
         if (!(dwAttribs & ATTR_OTHER) && !(pProgram || pDoc))
            goto CDBCont;
      }
      else if (lfndta.fd.dwFileAttributes & ATTR_JUNCTION) {
         if (!(dwAttribs & ATTR_JUNCTION)) {
            goto CDBCont;
         }
      }

      //
      // figure out the bitmap type here
      //
      if (lfndta.fd.dwFileAttributes & ATTR_DIR) {

         //
         // ignore "."  and ".." directories
         //
         if (ISDOTDIR(pName)) {
            goto CDBCont;
         }

        // NOTE: Reparse points are directories

         if (IsNetDir(szPath,pName))
            iBitmap = BM_IND_CLOSEDFS;
         else
         {
            if (lfndta.fd.dwFileAttributes & (ATTR_SYMBOLIC | ATTR_JUNCTION))
               iBitmap = BM_IND_CLOSEREPARSE;
            else
            iBitmap = BM_IND_CLOSE;
         }
      } else if (lfndta.fd.dwFileAttributes & (ATTR_HIDDEN | ATTR_SYSTEM)) {
         iBitmap = BM_IND_RO;
      } else if (pProgram) {
         iBitmap = BM_IND_APP;
      } else if (pDoc) {
         iBitmap = BM_IND_DOC;
      } else {
         if (lfndta.fd.dwFileAttributes & (ATTR_SYMBOLIC | ATTR_JUNCTION))
            iBitmap = BM_IND_FILREPARSE;
         else
         iBitmap = BM_IND_FIL;
      }

      lpxdta = MemAdd(&lpLinkLast,
                      lstrlen(pName),
                      lstrlen(lfndta.fd.cAlternateFileName));

      if (!lpxdta)
         goto CDBMemoryErr;

      lpHead->dwEntries++;

      lpxdta->dwAttrs = lfndta.fd.dwFileAttributes;
      lpxdta->ftLastWriteTime = lfndta.fd.ftLastWriteTime;

      //
      // files > 2^63 will come out negative, so tough.
      // (WIN32_FIND_DATA.nFileSizeHigh is not signed, but
      // LARGE_INTEGER is)
      //
      lpxdta->qFileSize.LowPart = lfndta.fd.nFileSizeLow;
      lpxdta->qFileSize.HighPart = lfndta.fd.nFileSizeHigh;

      lpxdta->byBitmap = iBitmap;
      lpxdta->pDocB = pDoc;        // even if program, use extension list for icon to display

      if (IsLFN(pName)) {
         lpxdta->dwAttrs |= ATTR_LFN;
      }

      if (!bCasePreserved)
         lpxdta->dwAttrs |= ATTR_LOWERCASE;

      lstrcpy(MemGetFileName(lpxdta), pName);
      lstrcpy(MemGetAlternateFileName(lpxdta), lfndta.fd.cAlternateFileName);

      lpHead->dwTotalCount++;
      (lpHead->qTotalSize).QuadPart = (lpxdta->qFileSize).QuadPart +
                                      (lpHead->qTotalSize).QuadPart;

CDBCont:

      if (bDirReadRebuildDocString) {
Abort:
         WFFindClose(&lfndta);
         MemDelete(lpStart);

         return NULL;
      }


      if (bDirReadAbort) {

         EnterCriticalSection(&CriticalSectionDirRead);

         bAbort = ((GetWindowLongPtr(hwndDir,
                                  GWL_HDTAABORT) & (EDIRABORT_WINDOWCLOSE|
                                                    EDIRABORT_READREQUEST)) ||
                   GetWindowLongPtr(hwndDir, GWL_HDTA));

         LeaveCriticalSection(&CriticalSectionDirRead);

         if (bAbort)
            goto Abort;
      }

      if (!WFFindNext(&lfndta)) {
         break;
      }
   }

   WFFindClose(&lfndta);

CDBDiskGone:

   //
   // If no error, but no entries then no files
   //
   if (!iError && !lpHead->dwEntries)
      iError = IDS_NOFILES;

   goto Done;

CDBMemoryErr:

   WFFindClose(&lfndta);

   MyMessageBox(hwndFrame,
                IDS_OOMTITLE,
                IDS_OOMREADINGDIRMSG,
                MB_OK | MB_ICONEXCLAMATION);

   //
   // !! BUGBUG !!
   //
   // What should the error code be?
   //
   iError = 0;

Done:

   if (iError) {
      MemDelete(lpStart);
      lpStart = NULL;
   }

   SetLBFont(hwndDir,
             GetDlgItem(hwndDir, IDCW_LISTBOX),
             hFont,
             (DWORD)GetWindowLongPtr(hwnd, GWL_VIEW),
             lpStart);

   R_Space(drive);
   U_Space(drive);

   if (SendMessage(hwndDir,
                   FS_DIRREADDONE,
                   (WPARAM)iError,
                   (LPARAM)lpStart) != (LRESULT)lpStart) {

      MemDelete(lpStart);
      lpStart = NULL;
   }

   return lpStart;
}




/////////////////////////////////////////////////////////////////////
//
// Name:     IsNetDir
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
// Notes:    !! BUGBUG !! Check if this is reentrant !!
//
/////////////////////////////////////////////////////////////////////

BOOL
IsNetDir(LPWSTR pPath, LPWSTR pName)
{
   DWORD dwType;
   WCHAR szFullPath[2*MAXPATHLEN];

   DRIVE drive = DRIVEID(pPath);

   if (!WAITNET_TYPELOADED)
      return FALSE;

   lstrcpy(szFullPath, pPath);
   StripFilespec(szFullPath);

   AddBackslash(szFullPath);
   lstrcat(szFullPath, pName);

   //
   // Do a Net check, but only if we haven't failed before...
   // This logic is used to speed up share/not-share directory
   // information.  If it has failed before, never call it again
   // for this drive, since the fail is assumed always due to
   // insufficient privilege.
   //
   if (aDriveInfo[drive].bShareChkFail ||
       !(WNetGetDirectoryType(szFullPath, &dwType,
                              !aDriveInfo[drive].bShareChkTried) == WN_SUCCESS))
   {
      dwType = 0;

      //
      // Remind ourselves that we failed, so don't try again on this drive.
      //
      aDriveInfo[drive].bShareChkFail = TRUE;
   }

   aDriveInfo[drive].bShareChkTried = TRUE;
   return dwType;
}

// RegGetValue isn't available on Windows XP
LONG WFRegGetValueW(HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpValue, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)
{
    DWORD dwStatus;
    HKEY hkeySub;

    if ((dwStatus = RegOpenKey(hkey, lpSubKey, &hkeySub)) == ERROR_SUCCESS)
    {
        dwStatus = RegQueryValueEx(hkeySub, lpValue, NULL, pdwType, pvData, pcbData);

        RegCloseKey(hkeySub);
    }

    return dwStatus;
}
