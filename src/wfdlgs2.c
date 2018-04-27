/********************************************************************

   wfdlgs2.c

   More Windows File System Dialog procedures

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"
#include "wnetcaps.h"         // WNetGetCaps()
#include "commdlg.h"



BOOL (*lpfnGetFileVersionInfoW)(LPWSTR, DWORD, DWORD, LPVOID);
DWORD (*lpfnGetFileVersionInfoSizeW)(LPWSTR, LPDWORD);
BOOL (*lpfnVerQueryValueW)(const LPVOID, LPWSTR, LPVOID*, LPDWORD);
BOOL (*lpfnVerQueryValueIndexW)(const LPVOID, LPWSTR, INT, LPVOID*, LPVOID*, LPDWORD);

#define VERSION_DLL TEXT("version.dll")

// LATER: replace these with ordinal numbers

#define VERSION_GetFileVersionInfoW     "GetFileVersionInfoW"
#define VERSION_GetFileVersionInfoSizeW "GetFileVersionInfoSizeW"
#define VERSION_VerQueryValueW          "VerQueryValueW"
#define VERSION_VerQueryValueIndexW     "VerQueryValueIndexW"

#define GetFileVersionInfoW     (*lpfnGetFileVersionInfoW)
#define GetFileVersionInfoSizeW (*lpfnGetFileVersionInfoSizeW)
#define VerQueryValueW          (*lpfnVerQueryValueW)
#define VerQueryValueIndexW      (*lpfnVerQueryValueIndexW)

VOID CheckAttribsDlgButton(HWND hDlg, INT id, DWORD dwAttribs, DWORD dwAttribs3State, DWORD dwAttribsOn);
BOOL NoQuotes(LPTSTR szT);


// Return pointers to various bits of a path.
// ie where the dir name starts, where the filename starts and where the
// params are.
VOID
GetPathInfo(LPTSTR szTemp, LPTSTR *ppDir, LPTSTR *ppFile, LPTSTR *ppPar)
{
   // handle quoted things
   BOOL bInQuotes=FALSE;

   // strip leading spaces

   for (*ppDir = szTemp; **ppDir == CHAR_SPACE; (*ppDir)++)
      ;

   // locate the parameters

   // Use bInQuotes and add if clause
   for (*ppPar = *ppDir; **ppPar && (**ppPar != CHAR_SPACE || bInQuotes) ; (*ppPar)++)
      if ( CHAR_DQUOTE == **ppPar ) bInQuotes = !bInQuotes;

   // locate the start of the filename and the extension.

   for (*ppFile = *ppPar; *ppFile > *ppDir; --(*ppFile)) {
      if (((*ppFile)[-1] == CHAR_COLON) || ((*ppFile)[-1] == CHAR_BACKSLASH))
         break;
   }
}


//
// Strips off the path portion and replaces the first part of an 8-dot-3
// filename with an asterisk.
//

VOID
StarFilename(LPTSTR pszPath)
{
   LPTSTR p;
   TCHAR szTemp[MAXPATHLEN];

   // Remove any leading path information.
   StripPath(pszPath);

   lstrcpy(szTemp, pszPath);

   p=GetExtension(szTemp);

   if (*p) {
      pszPath[0] = CHAR_STAR;
      lstrcpy(pszPath+1, p-1);
   } else {
      lstrcpy(pszPath, szStarDotStar);
   }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SearchDlgProc() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
SearchDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  LPTSTR     p;
  MDICREATESTRUCT   MDICS;
  TCHAR szStart[MAXFILENAMELEN];

  UNREFERENCED_PARAMETER(lParam);

  switch (wMsg)
    {
      case WM_INITDIALOG:

          SendDlgItemMessage(hDlg, IDD_DIR, EM_LIMITTEXT, COUNTOF(SearchInfo.szSearch)-1, 0L);
          SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, COUNTOF(szStart)-1, 0L);

          GetSelectedDirectory(0, SearchInfo.szSearch);
          SetDlgItemText(hDlg, IDD_DIR, SearchInfo.szSearch);

          p = GetSelection(1, NULL);

          if (p) {
                GetNextFile(p, szStart, COUNTOF(szStart));
                StarFilename(szStart);
                SetDlgItemText(hDlg, IDD_NAME, szStart);
                LocalFree((HANDLE)p);
          }

          CheckDlgButton(hDlg, IDD_SEARCHALL, !SearchInfo.bDontSearchSubs);
		  CheckDlgButton(hDlg, IDD_INCLUDEDIRS, SearchInfo.bIncludeSubDirs);
          break;

      case WM_COMMAND:
          switch (GET_WM_COMMAND_ID(wParam, lParam)) {

              case IDD_HELP:
                  goto DoHelp;

              case IDCANCEL:
                  EndDialog(hDlg, FALSE);
                  break;

              case IDOK:

                  GetDlgItemText(hDlg, IDD_DIR, SearchInfo.szSearch, COUNTOF(SearchInfo.szSearch));
                  QualifyPath(SearchInfo.szSearch);

				  GetDlgItemText(hDlg, IDD_DATE, szStart, COUNTOF(szStart));
				  SearchInfo.ftSince.dwHighDateTime = SearchInfo.ftSince.dwLowDateTime = 0;
				  if (lstrlen(szStart) != 0)
				  {
					  DATE date;
					  SYSTEMTIME st;
					  FILETIME ftLocal;
					  HRESULT hr = VarDateFromStr(szStart, lcid, 0, &date);
					  BOOL b1 = VariantTimeToSystemTime(date, &st);
					  BOOL b2 = SystemTimeToFileTime(&st, &ftLocal);

					  // SearchInfo.ftSince is in UTC (as are FILETIME in files to which this will be compared)
					  BOOL b3 = LocalFileTimeToFileTime(&ftLocal, &SearchInfo.ftSince);
					  if (FAILED(hr) || !b1 || !b2 || !b3) {
						  MessageBeep(0);
						  break;
					  }
				  }

                  GetDlgItemText(hDlg, IDD_NAME, szStart, COUNTOF(szStart));

                  KillQuoteTrailSpace( szStart );

                  AppendToPath(SearchInfo.szSearch, szStart);

                  SearchInfo.bDontSearchSubs = !IsDlgButtonChecked(hDlg, IDD_SEARCHALL);
				  SearchInfo.bIncludeSubDirs = IsDlgButtonChecked(hDlg, IDD_INCLUDEDIRS);

                  EndDialog(hDlg, TRUE);

                  SearchInfo.iDirsRead = 0;
                  SearchInfo.iFileCount = 0;
                  SearchInfo.eStatus = SEARCH_NULL;
                  SearchInfo.bCancel = FALSE;

                  /* Is the search window already up? */
                  if (hwndSearch) {

                     // used to check ret val of change display then
                     // activate and non-iconize search window.
                     // now instead pass a flag

                     SendMessage(hwndSearch, FS_CHANGEDISPLAY, CD_PATH, (LPARAM)SearchInfo.szSearch);

                  } else {
					  BOOL bMaximized = FALSE;
					  HWND hwndMDIChild = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, (LPARAM)&bMaximized);

					  // cf. https://www.codeproject.com/articles/2077/creating-a-new-mdi-child-maximization-and-focus-is
					  if (bMaximized)
					  {
						  SendMessage(hwndMDIClient, WM_SETREDRAW, FALSE, 0);
					  }

                     //
                     // !! BUGBUG !!
                     //
                     // This is safe since szMessage = MAXPATHLEN*2+MAXSUGGESTLEN
                     // but it's not portable
                     //
                     LoadString(hAppInstance, IDS_SEARCHTITLE, szMessage,
                        COUNTOF(szMessage));

                     lstrcat(szMessage, SearchInfo.szSearch);

                     // Have the MDIClient create the MDI directory window.

                     MDICS.szClass = szSearchClass;
                     MDICS.hOwner = hAppInstance;
                     MDICS.szTitle = szMessage;

                     // Minimize

                     MDICS.style = bMaximized ? WS_MAXIMIZE : WS_MINIMIZE;
					 MDICS.x  = CW_USEDEFAULT;
                     MDICS.y  = 0;
                     MDICS.cx = CW_USEDEFAULT;
                     MDICS.cy = 0;

                     SendMessage(hwndMDIClient, WM_MDICREATE, 0L, (LPARAM)(LPMDICREATESTRUCT)&MDICS);

					 if (bMaximized)
					 {
						 // the WM_MDICREATE above creates the window maximized;
						 // here we re-activate the original maximized window

						 SendMessage(hwndMDIClient, WM_MDIACTIVATE, (WPARAM)hwndMDIChild, 0L);

						 SendMessage(hwndMDIClient, WM_SETREDRAW, TRUE, 0);
						 RedrawWindow(hwndMDIClient, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
					 }
                  }
                  break;

              default:
                  return(FALSE);
            }
          break;

       default:
          if (wMsg == wHelpMessage) {
DoHelp:
                WFHelp(hDlg);

                return TRUE;
          } else
                return FALSE;
     }
  return TRUE;
}


#define RUN_LENGTH      MAXPATHLEN

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RunDlgProc() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
RunDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  LPTSTR p,pDir,pFile,pPar;
  register DWORD ret;
  LPTSTR pDir2;
  TCHAR szTemp[MAXPATHLEN];
  TCHAR szTemp2[MAXPATHLEN];
  TCHAR sz3[RUN_LENGTH];

  UNREFERENCED_PARAMETER(lParam);

  switch (wMsg)
    {
      case WM_INITDIALOG:
      SetDlgDirectory(hDlg, NULL);
          SetWindowDirectory();          // and really set the DOS current directory

          SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, COUNTOF(szTemp)-1, 0L);

          p = GetSelection(1, NULL);

          if (p) {
              SetDlgItemText(hDlg, IDD_NAME, p);
              LocalFree((HANDLE)p);
          }
          break;

      case WM_COMMAND:
          switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
              case IDD_HELP:
                  goto DoHelp;

              case IDCANCEL:
                  EndDialog(hDlg, FALSE);
                  break;

              case IDOK:
                {
                  BOOL bLoadIt, bRunAs;

                  GetDlgItemText(hDlg, IDD_NAME, szTemp, COUNTOF(szTemp));
                  GetPathInfo(szTemp, &pDir, &pFile, &pPar);

                  // copy away parameters
                  lstrcpy(sz3,pPar);
                  *pPar = CHAR_NULL;    // strip the params from the program

                  // REVIEW HACK Hard code UNC style paths.
                  if (*pDir == CHAR_BACKSLASH && *(pDir+1) == CHAR_BACKSLASH) {
                     // This is a UNC style filename so NULLify directory.
                     pDir2 = NULL;
                  } else {
                     GetSelectedDirectory(0, szTemp2);
                     pDir2 = szTemp2;
                  }

                  bLoadIt = IsDlgButtonChecked(hDlg, IDD_LOAD);
				  bRunAs = IsDlgButtonChecked(hDlg, IDD_RUNAS);

                  // Stop SaveBits flickering by invalidating the SaveBitsStuff.
                  // You can't just hide the window because it messes up the
                  // the activation.

                  SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);

                  ret = ExecProgram(pDir, sz3, pDir2, bLoadIt, bRunAs);
                  if (ret) {
                     MyMessageBox(hDlg, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                     SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
                  } else
                      EndDialog(hDlg, TRUE);
                  break;
                }

              default:
                  return(FALSE);
            }
          break;

      default:
          if (wMsg == wHelpMessage || wMsg == wBrowseMessage) {
DoHelp:
                WFHelp(hDlg);

                return TRUE;
          } else
                return FALSE;
    }
  return TRUE;
}


VOID
EnableCopy(HWND hDlg, BOOL bCopy)
{
   HWND hwnd;

   // turn these off

   hwnd = GetDlgItem(hDlg, IDD_STATUS);
   if (hwnd) {
      EnableWindow(hwnd, !bCopy);
      ShowWindow(hwnd, !bCopy ? SW_SHOWNA : SW_HIDE);
   }

   hwnd = GetDlgItem(hDlg, IDD_NAME);
   if (hwnd) {
      EnableWindow(hwnd, !bCopy);
      ShowWindow(hwnd, !bCopy ? SW_SHOWNA : SW_HIDE);
   }
}

VOID
MessWithRenameDirPath(LPTSTR pszPath)
{
   TCHAR szPath[MAXPATHLEN];
   LPTSTR lpsz;

   // absolute path? don't tamper with it!

   // Also allow "\"f:\joe\me\""  ( ->   "f:\joe\me  )

   //
   // !! LATER !!
   //
   // Should we allow backslashes here also ?
   // CheckSlashes(pszPath); or add || clause.
   //

   lpsz = (CHAR_DQUOTE == pszPath[0]) ?
      pszPath+1 :
      pszPath;

   if (CHAR_COLON == lpsz[1] && CHAR_BACKSLASH == lpsz[2] ||
      lstrlen(pszPath) > (COUNTOF(szPath) - 4) )

      return;

   //
   // prepend "..\" to this non absolute path
   //
   lstrcpy(szPath, TEXT("..\\"));
   lstrcat(szPath, pszPath);
   lstrcpy(pszPath, szPath);
}


//--------------------------------------------------------------------------*/
//                                                                          */
//  SuperDlgProc() -                                                        */
//                                                                          */
//--------------------------------------------------------------------------*/

// This proc handles the Print, Move, Copy, Delete, and Rename functions.
// The calling routine (AppCommandProc()) sets 'dwSuperDlgMode' before
// calling DialogBox() to indicate which function is being used.

INT_PTR
SuperDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   UINT          len;
   LPTSTR        pszFrom;
   //
   // WFMoveCopyDrive tries to append \*.* to directories and
   // probably other nasty stuff.  2* for safety.
   //
   TCHAR  szTo[2*MAXPATHLEN];
   static BOOL   bTreeHasFocus;

JAPANBEGIN
   TCHAR         szStr[256];
JAPANEND

   static PCOPYINFO pCopyInfo;

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {
   case WM_INITDIALOG:
      {
         LPTSTR  p;
         HWND  hwndActive;

         pCopyInfo = NULL;

         SetDlgDirectory(hDlg, NULL);

         EnableCopy(hDlg, dwSuperDlgMode == IDM_COPY);

         hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
         bTreeHasFocus = (hwndActive != hwndSearch) &&
            (GetTreeFocus(hwndActive) == HasTreeWindow(hwndActive));

         switch (dwSuperDlgMode) {

         case IDM_COPY:

            p = GetSelection(0, NULL);

            LoadString(hAppInstance, IDS_COPY, szTitle, COUNTOF(szTitle));
            SetWindowText(hDlg, szTitle);

            if (bJAPAN) {

               //by yutakas 1992/10/29
               LoadString(hAppInstance, IDS_KK_COPYFROMSTR, szStr, COUNTOF(szStr));
               SetDlgItemText(hDlg, IDD_KK_TEXTFROM, szStr);
               LoadString(hAppInstance, IDS_KK_COPYTOSTR, szStr, COUNTOF(szStr));
               SetDlgItemText(hDlg, IDD_KK_TEXTTO, szStr);
            }

            break;
         case IDM_RENAME:

            LoadString(hAppInstance, IDS_RENAME, szTitle, COUNTOF(szTitle));
            SetWindowText(hDlg, szTitle);

            if (bJAPAN) {
               //by yutakas 1992/10/29
               LoadString(hAppInstance, IDS_KK_RENAMEFROMSTR, szStr, COUNTOF(szStr));
               SetDlgItemText(hDlg, IDD_KK_TEXTFROM, szStr);
               LoadString(hAppInstance, IDS_KK_RENAMETOSTR, szStr, COUNTOF(szStr));
               SetDlgItemText(hDlg, IDD_KK_TEXTTO, szStr);
            }

            // when renaming the current directory we cd up a level
            // (not really) and apply the appropriate adjustments

            if (bTreeHasFocus) {

               p = GetSelection(16, NULL);
               lstrcpy(szTo, p);
               StripFilespec(szTo);

               SetDlgDirectory(hDlg, szTo);  // make the user think this!
               StripPath(p);                 // name part of dir

               CheckEsc(p);
            } else {
               p = GetSelection(0, NULL);
            }

            break;

         default:

            p=GetSelection(0, NULL);
         }

         SetDlgItemText(hDlg, IDD_FROM, p);

         if ((dwSuperDlgMode == IDM_PRINT) || (dwSuperDlgMode == IDM_DELETE))
            wParam = IDD_FROM;
         else
         {
            TCHAR szDirs[MAXPATHLEN];
            LPTSTR rgszDirs[MAX_DRIVES];
            int drive, driveCur;
        	BOOL fFirst = TRUE;
            
            wParam = IDD_TO;
            if (dwSuperDlgMode == IDM_RENAME)
	            SetDlgItemText(hDlg, IDD_TO, p);

			driveCur = GetWindowLongPtr(hwndActive, GWL_TYPE);

			lstrcpy(szDirs, TEXT("Other: "));

   			GetAllDirectories(rgszDirs);

        	for (drive = 0; drive < MAX_DRIVES; drive++)
        	{
				if (drive != driveCur && rgszDirs[drive] != NULL)
				{
                    if (!fFirst)
                    {
                        wcsncat_s(szDirs, MAXPATHLEN, TEXT(";"), 1);
                    }
                    fFirst = FALSE;

                    // NOTE: this call may truncate the result that goes in szDirs,
                    // but due to the limited width of the dialog, we can't see it all anyway.
                    wcsncat_s(szDirs, MAXPATHLEN, rgszDirs[drive], _TRUNCATE);

	        		LocalFree(rgszDirs[drive]);
	        	}
        	}

	        SetDlgItemText(hDlg, IDD_DIRS, szDirs);
         }

         SendDlgItemMessage(hDlg, wParam, EM_LIMITTEXT, COUNTOF(szTo) - 1, 0L);
         LocalFree((HANDLE)p);
         break;
      }

   case WM_NCACTIVATE:
      if (IDM_RENAME == dwSuperDlgMode)
      {
		size_t ich1, ich2;
		LPWSTR pchDot;

		GetDlgItemText(hDlg, IDD_TO, szTo, COUNTOF(szTo));
		ich1 = 0;
		ich2 = wcslen(szTo);
		pchDot = wcsrchr(szTo, '.');
		if (pchDot != NULL)
			ich2 = pchDot - szTo;
		if (*szTo == '\"')
		{
			ich1 = 1;
			if (pchDot == NULL)
				ich2--;
		}
		SendDlgItemMessage(hDlg, IDD_TO, EM_SETSEL, ich1, ich2);
      }
      return FALSE;
      
   case FS_COPYDONE:

      //
      // Only cancel out if pCopyInfo == lParam
      // This indicates that the proper thread quit.
      //
      // wParam holds return value
      //

      if (lParam == (LPARAM)pCopyInfo) {
         SPC_SET_HITDISK(qFreeSpace);     // force status info refresh

         EndDialog(hDlg, wParam);
      }
      break;


   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {

      case IDD_HELP:
         goto DoHelp;

      case IDCANCEL:

         if (pCopyInfo)
            pCopyInfo->bUserAbort = TRUE;

SuperDlgExit:

         EndDialog(hDlg, 0);
         break;

      case IDOK:

         len = (UINT)(SendDlgItemMessage(hDlg, IDD_FROM, EM_LINELENGTH,
            (WPARAM)-1, 0L) + 1);

         //
         // make sure the pszFrom buffer is big enough to
         // add the "..\" stuff in MessWithRenameDirPath()
         //
         len += 4;

         pszFrom = (LPTSTR)LocalAlloc(LPTR, ByteCountOf(len));
         if (!pszFrom)
            goto SuperDlgExit;

         GetDlgItemText(hDlg, IDD_FROM, pszFrom, len);
         GetDlgItemText(hDlg, IDD_TO, szTo, COUNTOF(szTo));

         //
         // if dwSuperDlgMode is copy, rename, or move, do checkesc.
         // Only if no quotes in string!
         //
         if ( IDM_COPY == dwSuperDlgMode || IDM_MOVE == dwSuperDlgMode ||
            IDM_RENAME == dwSuperDlgMode) {

            if (NoQuotes(szTo))
               CheckEsc(szTo);
         }

         if (!szTo[0])
         {
             switch (dwSuperDlgMode)
             {
                 case IDM_RENAME:
                 case IDM_MOVE:
                 case IDM_COPY:
                 {
                     szTo[0] = CHAR_DOT;
                     szTo[1] = CHAR_NULL;
                     break;
                 }
             }
         }

         EnableCopy(hDlg, FALSE);

         hdlgProgress = hDlg;
         if (dwSuperDlgMode == IDM_PRINT) {
            WFPrint(pszFrom);

            LocalFree(pszFrom);
            goto SuperDlgExit;

		 } else {

            if (dwSuperDlgMode == IDM_RENAME && bTreeHasFocus) {
               MessWithRenameDirPath(pszFrom);
               MessWithRenameDirPath(szTo);
            }

            //
            // Setup pCopyInfo structure
            //
            // Note that everything must be malloc'd!!
            // (Freed by thread)
            //

            pCopyInfo = (PCOPYINFO) LocalAlloc(LPTR, sizeof(COPYINFO));

            if (!pCopyInfo) {

Error:
               FormatError(TRUE, szMessage, COUNTOF(szMessage), GetLastError());
               LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));

               MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

               LocalFree(pszFrom);
	           goto SuperDlgExit;
            }

            pCopyInfo->pFrom = pszFrom;
            pCopyInfo->pTo = (LPTSTR) LocalAlloc(LMEM_FIXED, sizeof(szTo));

            if (!pCopyInfo->pTo) {

               goto Error;
            }

            pCopyInfo->dwFunc =  dwSuperDlgMode-IDM_MOVE+1;
            pCopyInfo->bUserAbort = FALSE;

            lstrcpy(pCopyInfo->pTo, szTo);

            //
            // Move/Copy things.
            //
            // HACK: Compute the FUNC_ values from WFCOPY.H
            //
            if (WFMoveCopyDriver(pCopyInfo)) {

               LoadString(hAppInstance,
                          IDS_COPYERROR + pCopyInfo->dwFunc,
                          szTitle,
                          COUNTOF(szTitle));

               FormatError(TRUE, szMessage, COUNTOF(szMessage), GetLastError());

               MessageBox(hDlg, szMessage, szTitle, MB_ICONSTOP|MB_OK);

               EndDialog(hDlg, GetLastError());

            } else {

               //
               // Disable all but the cancel button on the notify dialog
               //
               DialogEnterFileStuff(hdlgProgress);
            }
         }
         break;

      default:
         return(FALSE);
      }
      break;

   default:
      if (wMsg == wHelpMessage) {
DoHelp:
         WFHelp(hDlg);

         return TRUE;
      } else {
         return FALSE;
      }
   }
   return TRUE;
}


VOID
CheckAttribsDlgButton(
   HWND hDlg,
   INT id,
   DWORD dwAttribs,
   DWORD dwAttribs3State,
   DWORD dwAttribsOn)
{
   INT i;

   if (dwAttribs3State & dwAttribs)
      i = 2;
   else if (dwAttribsOn & dwAttribs)
      i = 1;
   else
      i = 0;

   CheckDlgButton(hDlg, id, i);
}

// The following data structure associates a version stamp datum
// name (which is not localized) with a string ID.  This is so we
// can show translations of these names to the user.

struct vertbl {
    WCHAR *pszName;
    WORD idString;
};

//  Note that version stamp datum names are NEVER internationalized,
//  so the following literal strings are just fine.

#define MAX_VERNAMES 10
struct vertbl vernames[MAX_VERNAMES] = {
    { L"Comments",          IDS_VN_COMMENTS },
    { L"CompanyName",       IDS_VN_COMPANYNAME },
    { L"FileDescription",   IDS_VN_FILEDESCRIPTION },
    { L"InternalName",      IDS_VN_INTERNALNAME },
    { L"LegalTrademarks",   IDS_VN_LEGALTRADEMARKS },
    { L"OriginalFilename",  IDS_VN_ORIGINALFILENAME },
    { L"PrivateBuild",      IDS_VN_PRIVATEBUILD },
    { L"ProductName",       IDS_VN_PRODUCTNAME },
    { L"ProductVersion",    IDS_VN_PRODUCTVERSION },
    { L"SpecialBuild",      IDS_VN_SPECIALBUILD }
};

DWORD dwHandle;         // version subsystem handle
HANDLE hmemVersion=0;   // global handle for version data buffer
LPTSTR lpVersionBuffer; // pointer to version data
DWORD dwVersionSize;    // size of the version data
TCHAR szVersionKey[60]; // big enough for anything we need
LPWORD lpXlate;         // ptr to translations data
UINT cXlate;            // count of translations
LPWSTR pszXlate = NULL;
UINT cchXlateString;

#define LANGLEN          45    // characters per language

#define VER_KEY_END      25    // length of "\StringFileInfo\xxxx\yyyy" (chars)
#define VER_BLOCK_OFFSET 24    // to get block size (chars)

// (not localized)
TCHAR szFileVersion[]    = TEXT("FileVersion");
TCHAR szLegalCopyright[] = TEXT("LegalCopyright");

WCHAR wszFileVersion[] = L"FileVersion";
WCHAR wszLegalCopyright[] = L"LegalCopyright";

void FreeVersionInfo(void);


// Disables the version controls in a properties dialog.  Used for
// when the selection is a directory and also when a file has no
// version stamps.

VOID
DisableVersionCtls(HWND hDlg)
{
   EnableWindow(GetDlgItem(hDlg, IDD_VERSION_FRAME), FALSE);
   EnableWindow(GetDlgItem(hDlg, IDD_VERSION_KEY), FALSE);
   EnableWindow(GetDlgItem(hDlg, IDD_VERSION_VALUE), FALSE);
}


// Gets a particular datum about a file.  The file's version info
// should have already been loaded by GetVersionInfo.  If no datum
// by the specified name is available, NULL is returned.  The name
// specified should be just the name of the item itself;  it will
// be concatenated onto "\StringFileInfo\xxxxyyyy\" automatically.

// Version datum names are not localized, so it's OK to pass literals
// such as "FileVersion" to this function.

LPTSTR
GetVersionDatum(LPTSTR pszName)
{
   DWORD cbValue=0;
   LPTSTR lpValue;

   if (!hmemVersion)
      return NULL;

   lstrcpy(szVersionKey + VER_KEY_END, pszName);

   VerQueryValueW(lpVersionBuffer, szVersionKey, (LPVOID*)&lpValue, &cbValue);

   return (cbValue != 0) ? lpValue : NULL;
}

// Initialize version information for the properties dialog.  The
// above global variables are initialized by this function, and
// remain valid (for the specified file only) until FreeVersionInfo
// is called.

// Try in the following order
//
// 1. Current language winfile is running in.
// 2. English, 0409 codepage
// 3. English, 0000 codepage
// 4. First translation in resource
//    "\VarFileInfo\Translations" section

// GetVersionInfo returns LPTSTR if the version info was read OK,
// otherwise NULL.  If the return is NULL, the buffer may still
// have been allocated;  always call FreeVersionInfo to be safe.
//
// pszPath is modified by this call (pszName is appended).
//
// Note, Codepage is bogus, since everything is really in unicode.
// Note, Language is bogus, since FindResourceEx takes a language already...


LPTSTR
GetVersionInfo(PTSTR pszPath, PTSTR pszName)
{
   DWORD cbValue=0;
   DWORD cbValueTranslation=0;
   LPTSTR lpszValue=NULL;

   static bDLLFail = FALSE;

   if (!hVersion) {

      hVersion = LoadLibrary(VERSION_DLL);

      if (!hVersion) {
         bDLLFail = TRUE;
         return NULL;
      }

#define GET_PROC(x) \
   if (!(lpfn##x = (PVOID) GetProcAddress(hVersion,VERSION_##x))) {\
      bDLLFail = TRUE; \
      return NULL; }

      GET_PROC(GetFileVersionInfoW);
      GET_PROC(GetFileVersionInfoSizeW);
      GET_PROC(VerQueryValueW);
      GET_PROC(VerQueryValueIndexW);
   }

   if (bDLLFail)
      return NULL;

#undef GET_PROC

   //
   // Just in case, free old version buffer.
   //
   if (hmemVersion)
      FreeVersionInfo();

   AddBackslash(pszPath);

   // pszPath = fully qualified name
   lstrcat(pszPath, pszName);

   dwVersionSize = GetFileVersionInfoSizeW(pszPath, &dwHandle);

   if (dwVersionSize == 0L)
      // no version info
      return NULL;

   //
   // The value returned from GetFileVersionInfoSize is actually
   // a byte count.
   //
   hmemVersion = GlobalAlloc(GPTR, dwVersionSize);
   if (hmemVersion == NULL)
      // can't get memory for version info, blow out
      return NULL;

   lpVersionBuffer = GlobalLock(hmemVersion);

   //
   // If we fail, just return NULL. hmemVersion will be freed the next
   // time we do a version call.
   //
   if (!GetFileVersionInfoW(pszPath, dwHandle, dwVersionSize, lpVersionBuffer))
      return NULL;

   //
   // We must always get the translation since we want to display
   // all the languages anyway.
   //
   VerQueryValue(lpVersionBuffer, TEXT("\\VarFileInfo\\Translation"),
      (LPVOID*)&lpXlate, &cbValueTranslation);

   if (cbValueTranslation != 0) {

      //
      // We found some translations above; use the first one.
      //
      cXlate = cbValueTranslation / sizeof(DWORD);

      //
      // figure 45 LANGLEN chars per lang name
      //
      cchXlateString = cXlate * LANGLEN;
      pszXlate = (LPWSTR)LocalAlloc(LPTR, ByteCountOf(cchXlateString));

   } else {
      lpXlate = NULL;
   }

   //
   // First try the language we are currently in.
   //
   wsprintf(szVersionKey, TEXT("\\StringFileInfo\\%04X04B0\\"),
      LANGIDFROMLCID(lcid));

   lpszValue = GetVersionDatum(szFileVersion);

   if (lpszValue != NULL)
      return lpszValue;

   //
   // Now try the first translation
   //
   if (cbValueTranslation != 0) {

      wsprintf(szVersionKey, TEXT("\\StringFileInfo\\%04X%04X\\"),
         *lpXlate, *(lpXlate+1));

      //
      // a required field
      //
      lpszValue = GetVersionDatum(szFileVersion);

      if (lpszValue != NULL) {

         //
         // localized key found version data
         //
         return lpszValue;
      }
   }


   //
   // Now try the english, unicode
   //
   lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904B0\\"));
   lpszValue = GetVersionDatum(szFileVersion);

   if (lpszValue != NULL)
      return lpszValue;


   //
   // Try english with various code pages
   // (04E4) here
   //
   lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\040904E4\\"));
   lpszValue = GetVersionDatum(szFileVersion);

   if (lpszValue != NULL)
      return lpszValue;             // localized key found version data


   //
   // Try english with various code pages
   // (0000) here
   //
   lstrcpy(szVersionKey, TEXT("\\StringFileInfo\\04090000\\"));
   lpszValue = GetVersionDatum(szFileVersion);

   return lpszValue;
}

// Frees global version data about a file.  After this call, all
// GetVersionDatum calls will return NULL.  To avoid memory leaks,
// always call this before the main properties dialog exits.

VOID
FreeVersionInfo(VOID)
{
   lpVersionBuffer = NULL;
   dwHandle = 0L;
   if (hmemVersion) {
      GlobalUnlock(hmemVersion);
      GlobalFree(hmemVersion);
      hmemVersion = 0;
   }
   if (pszXlate) {
      LocalFree((HANDLE)pszXlate);
      pszXlate = NULL;
   }
}

// Looks for version information on the file.  If none is found,
// leaves the version number label as-is ("Not marked").  The
// specific datum displayed is "FileVersion" from the "StringFileInfo"
// block.

// Returns TRUE if the file has version stamp information.

BOOL
FillSimpleVersion(HWND hDlg, LPTSTR lpszValue)
{
   BOOL bRet = TRUE;

   if (lpszValue != NULL)
      SetDlgItemText(hDlg, IDD_VERSION, lpszValue);
   else {
      DisableVersionCtls(hDlg);
      bRet = FALSE;
   }

   lpszValue = GetVersionDatum(szLegalCopyright);

   if (lpszValue != NULL)
      SetDlgItemText(hDlg, IDD_COPYRIGHT, lpszValue);

   return bRet;
}

// Fills the version key listbox with all available keys in the
// StringFileInfo block, and sets the version value text to the
// value of the first item.


VOID
FillVersionList(HWND hDlg)
{
   LPTSTR lpszKey, lpszValue;

   DWORD cbValue;
   UINT i, j, cchOffset;
   INT idx;
   HWND hwndLB;

   hwndLB = GetDlgItem(hDlg, IDD_VERSION_KEY);

   szVersionKey[VER_KEY_END - 1] = CHAR_NULL;        // strip the backslash

   for (j=0; VerQueryValueIndexW(lpVersionBuffer,
                                szVersionKey,
                                j,
                                (LPVOID*)&lpszKey,
                                (LPVOID*)&lpszValue,
                                &cbValue);  j++) {

      if (!lstrcmp(lpszKey, szFileVersion) ||
          !lstrcmp(lpszKey, szLegalCopyright)) {

          continue;
      }

      for (i=0; i<MAX_VERNAMES; i++)
         if (!lstrcmp(vernames[i].pszName, lpszKey))
            break;

      if (i != MAX_VERNAMES && LoadString(hAppInstance, vernames[i].idString, szMessage, COUNTOF(szMessage)))
         lpszKey=szMessage;

      idx = (INT)SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)lpszKey);

      //
      // Only add if the value len isn't zero.
      // This is checked in the SendMessage 4th parm.
      //
      if (idx != LB_ERR)
         SendMessage(hwndLB, LB_SETITEMDATA, idx, (LPARAM)lpszValue);
   }

   //
   // Now look at the \VarFileInfo\Translations section and add an
   // item for the language(s) this file supports.
   //

   if (lpXlate == NULL || pszXlate == NULL)
      goto Done;

   if (!LoadString(hAppInstance, (cXlate == 1) ? IDS_VN_LANGUAGE : IDS_VN_LANGUAGES,
      szMessage, COUNTOF(szMessage)))

      goto Done;

   idx = SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szMessage);
   if (idx == LB_ERR)
      goto Done;

   pszXlate[0] = CHAR_NULL;
   cchOffset = 0;
   for (i=0; i<cXlate; i++) {
      if (cchOffset + 2 > cchXlateString)
         break;
      if (i != 0) {
         lstrcat(pszXlate, TEXT(", "));
         cchOffset += 2;
      }

      if (VerLanguageName(lpXlate[i*2],
                          pszXlate+cchOffset,
                          cchXlateString-cchOffset
                          ) > cchXlateString - cchOffset)

         break;

      cchOffset += lstrlen(pszXlate+cchOffset);
   }
   pszXlate[cchXlateString-1] = CHAR_NULL;

   SendMessage(hwndLB, LB_SETITEMDATA, idx, (LPARAM)pszXlate);

Done:

   SendMessage(hwndLB, LB_SETCURSEL, 0, 0L);
   PostMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDD_VERSION_KEY, NULL, LBN_SELCHANGE));
}


INT
InitPropertiesDialog(
   HWND hDlg)
{
   HWND hwndLB, hwndActive, hwndTree;
   LPXDTA lpxdta;
   DWORD dwAttribsOn, dwAttribs3State, dwAttribsLast;
   HWND hwndDir, hwnd, hwndView;
   WCHAR szName[MAXPATHLEN];
   WCHAR szPath[MAXPATHLEN];
   WCHAR szTemp[MAXPATHLEN + 20];
   WCHAR szBuf[MAXPATHLEN];
   WCHAR szNum[MAXPATHLEN];
   INT i, iMac, iCount, dyButton;
   RECT rc, rcT;
   DWORD dwAttrib;
   FILETIME ftLastWrite;
   LFNDTA lfndta;
   LPTSTR p;
   HFONT hFont;
   INT nType = 0;
   DWORD dwFlags;
   BOOL bFileCompression = FALSE;
   BOOL bFileEncryption = FALSE;

   LPTSTR lpszBuf;
   LARGE_INTEGER qSize, qCSize;

   LARGE_INTEGER_NULL(qSize);
   LARGE_INTEGER_NULL(qCSize);

   //
   // this is needed for relative findfirst calls below
   //
   SetWindowDirectory();

   hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
   hwndDir = HasDirWindow(hwndActive);
   hwndTree = HasTreeWindow(hwndActive);

   if (GetVolumeInformation(NULL, NULL, 0L, NULL, NULL, &dwFlags, NULL, 0L))
   {
      bFileCompression = ((dwFlags & FS_FILE_COMPRESSION) == FS_FILE_COMPRESSION);
      bFileEncryption = ((dwFlags & FS_FILE_ENCRYPTION) == FS_FILE_ENCRYPTION);
   }

   iCount = 0;
   dwAttribsOn = 0;                // all bits to check
   dwAttribs3State = 0;            // all bits to 3 state
   dwAttribsLast = 0xFFFF;         // previous bits

   if (hwndTree && hwndTree == GetTreeFocus(hwndActive)) {

      SendMessage(hwndActive, FS_GETDIRECTORY, COUNTOF(szPath), (LPARAM)szPath);
      StripBackslash(szPath);

      if (!WFFindFirst(&lfndta, szPath, ATTR_ALL | ATTR_DIR)) {
         LoadString(hAppInstance, IDS_ATTRIBERR, szMessage, COUNTOF(szMessage));
         FormatError(FALSE, szMessage, COUNTOF(szMessage), ERROR_FILE_NOT_FOUND);

         //
         // BUGBUG: szPath should be set to "Properties for %s"!
         //
         MessageBox(hwndFrame, szMessage, szPath, MB_OK | MB_ICONSTOP);
         EndDialog(hDlg, FALSE);
         return 0;
      }
      WFFindClose(&lfndta);

      dwAttribsOn = lfndta.fd.dwFileAttributes;
      ftLastWrite = lfndta.fd.ftLastWriteTime;

      lstrcpy(szName, szPath);

      goto FullPath;
   }

   if (hwndDir) {
      hwndLB = GetDlgItem(hwndDir, IDCW_LISTBOX);
      hwndView = hwndDir;
   } else {
      hwndLB = GetDlgItem(hwndActive, IDCW_LISTBOX);
      hwndView = hwndActive;
   }

   iMac = SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

   szPath[0] = CHAR_NULL;
   szName[0] = CHAR_NULL;

   for (i = 0; i < iMac; i++) {

      if (SendMessage(hwndLB, LB_GETSEL, i, 0L)) {

         //
         // get info from either dir or search window
         //
         SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&lpxdta);
         dwAttrib = lpxdta->dwAttrs;

         //
         // Check that this is not the .. entry
         //
         if (dwAttrib & ATTR_DIR && dwAttrib & ATTR_PARENT)
            continue;

         qSize.QuadPart = qSize.QuadPart + (lpxdta->qFileSize).QuadPart;

         if (!szName[0]) {

            ftLastWrite = lpxdta->ftLastWriteTime;
            lstrcpy(szName, MemGetFileName(lpxdta));
         }

         dwAttribsOn |= dwAttrib;

         if (dwAttribsLast == 0xFFFF) {

            //
            // save the previous bits for future compares
            //
            dwAttribsLast = dwAttrib;

         } else {

            //
            // remember all bits that don't compare to last bits
            //
            dwAttribs3State |= (dwAttrib ^ dwAttribsLast);
         }

         iCount++;
      }
   }

   GetDlgItemText(hDlg, IDD_TEXT1, szTemp, COUNTOF(szTemp));
   wsprintf(szBuf, szTemp, iCount);
   SetDlgItemText(hDlg, IDD_TEXT1, szBuf);

   GetDlgItemText(hDlg, IDD_TEXT2, szTemp, COUNTOF(szTemp));
   PutSize(&qSize, szNum);
   wsprintf(szBuf, szTemp, szNum);
   SetDlgItemText(hDlg, IDD_TEXT2, szBuf);

   if (iCount == 1)
   {
      if (hwndDir)
      {
         SendMessage(hwndDir, FS_GETDIRECTORY, COUNTOF(szPath), (LPARAM)szPath);
      }
      else
      {
         lstrcpy(szPath, szName);
FullPath:
         StripPath(szName);
         StripFilespec(szPath);
      }

      StripBackslash(szPath);

      GetWindowText(hDlg, szTitle, COUNTOF(szTitle));
      wsprintf(szTemp, szTitle, szName);
      SetWindowText(hDlg, szTemp);

      SetDlgItemText(hDlg, IDD_NAME, szName);
      SetDlgItemText(hDlg, IDD_DIR, szPath);

      if (dwAttribsOn & ATTR_DIR)
      {
         //
         //  Hide size, ratio, and version info.
         //
         if (LoadString(hAppInstance, IDS_DIRNAMELABEL, szTemp, COUNTOF(szTemp)))
            SetDlgItemText(hDlg, IDD_NAMELABEL, szTemp);

         ShowWindow(GetDlgItem(hDlg, IDD_SIZELABEL), SW_HIDE);
         ShowWindow(GetDlgItem(hDlg, IDD_SIZE), SW_HIDE);

         ShowWindow(GetDlgItem(hDlg, IDD_CSIZELABEL), SW_HIDE);
         ShowWindow(GetDlgItem(hDlg, IDD_CSIZE), SW_HIDE);

         ShowWindow(GetDlgItem(hDlg, IDD_CRATIOLABEL), SW_HIDE);
         ShowWindow(GetDlgItem(hDlg, IDD_CRATIO), SW_HIDE);
      }
      else
      {
         if ((bFileCompression) && (dwAttribsOn & ATTR_COMPRESSED))
         {
            qCSize.LowPart = GetCompressedFileSize(szName, &(qCSize.HighPart));
            PutSize(&qCSize, szNum);
            wsprintf(szTemp, szSBytes, szNum);
            SetDlgItemText(hDlg, IDD_CSIZE, szTemp);

            if (qSize.QuadPart != 0)
            {
               //
               //  Ratio = 100 - ((CompressSize * 100) / FileSize)
               //
               qCSize.QuadPart =
                   (qCSize.QuadPart * 100) /
                   qSize.QuadPart;

               if (qCSize.HighPart || (qCSize.LowPart > 100))
               {
                   //
                   //  Ratio = 100%
                   //
                   qCSize.LowPart = 100;
                   qCSize.HighPart = 0;
               }
               else
               {
                   qCSize.LowPart = 100 - qCSize.LowPart;
               }
            }
            else
            {
               //
               //  Ratio = 0%
               //
               qCSize.LowPart = 0;
               qCSize.HighPart = 0;
            }
            PutSize(&qCSize, szNum);
            wsprintf(szTemp, TEXT("%s%%"), szNum);
            SetDlgItemText(hDlg, IDD_CRATIO, szTemp);
         }
         else
         {
            ShowWindow(GetDlgItem(hDlg, IDD_CSIZELABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_CSIZE), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_CRATIOLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDD_CRATIO), SW_HIDE);
         }

         PostMessage(hDlg, FS_CHANGEDISPLAY, 0, 0L);

         // changes szPath
         lpszBuf = GetVersionInfo(szPath, szName);

         if (FillSimpleVersion(hDlg, lpszBuf))
            FillVersionList(hDlg);
      }

      if (!bFileCompression)
      {
         ShowWindow(GetDlgItem(hDlg, IDD_COMPRESSED), SW_HIDE);
      }

      if (!bFileEncryption)
      {
         ShowWindow(GetDlgItem(hDlg, IDD_ENCRYPTED), SW_HIDE);
      }

      PutSize(&qSize, szNum);
      wsprintf(szTemp, szSBytes, szNum);
      SetDlgItemText(hDlg, IDD_SIZE, szTemp);

      PutDate(&ftLastWrite, szTemp);
      lstrcat(szTemp, TEXT("  "));
      PutTime(&ftLastWrite, szTemp + lstrlen(szTemp));

      SetDlgItemText(hDlg, IDD_DATE, szTemp);
   }
   else
   {
      dwContext = IDH_GROUP_ATTRIBS;
      
      if (!bFileCompression)
      {
          ShowWindow(GetDlgItem(hDlg, IDD_COMPRESSED), SW_HIDE);
      }

      if (!bFileEncryption)
      {
          ShowWindow(GetDlgItem(hDlg, IDD_ENCRYPTED), SW_HIDE);
      }
   }

   //
   // add the network specific property buttons
   //
   if (WNetStat(NS_PROPERTYDLG)) {
      GetWindowRect(GetDlgItem(hDlg,IDOK), &rcT);
      GetWindowRect(GetDlgItem(hDlg,IDCANCEL), &rc);
      dyButton = rc.top - rcT.top;

      GetWindowRect(GetDlgItem(hDlg,IDD_HELP), &rc);
      ScreenToClient(hDlg,(LPPOINT)&rc.left);
      ScreenToClient(hDlg,(LPPOINT)&rc.right);

      p = GetSelection(4, NULL);
      if (p) {

         for (i = 0; i < 6; i++) {

            if (iCount > 1)
               nType = WNPS_MULT;
            else if (dwAttribsOn & ATTR_DIR)
               nType = WNPS_DIR;
            else
               nType = WNPS_FILE;

            if (WNetGetPropertyText((WORD)i, (WORD)nType, p, szTemp, 30, WNTYPE_FILE) != WN_SUCCESS)
               break;

            if (!szTemp[0])
               break;

            OffsetRect(&rc,0,dyButton);
            hwnd = CreateWindowEx(0L, TEXT("button"), szTemp,
               WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,
               rc.left, rc.top,
               rc.right - rc.left, rc.bottom-rc.top,
               hDlg, (HMENU)(i + IDD_NETWORKFIRST), hAppInstance, NULL);

            if (hwnd) {
               hFont = (HFONT)SendDlgItemMessage(hDlg, IDOK, WM_GETFONT, 0, 0L);
               SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            }
         }

         LocalFree((HANDLE)p);

         ClientToScreen(hDlg,(LPPOINT)&rc.left);
         ClientToScreen(hDlg,(LPPOINT)&rc.right);
         GetWindowRect(hDlg,&rcT);
         rc.bottom += dyButton;
         if (rcT.bottom <= rc.bottom) {
            SetWindowPos(hDlg,NULL,0,0,rcT.right-rcT.left,
            rc.bottom - rcT.top, SWP_NOMOVE|SWP_NOZORDER);
         }
      }
   }

   //
   // change those that don't need to be 3state to regular
   //
   if (ATTR_READONLY & dwAttribs3State)
   {
      SetWindowLongPtr( GetDlgItem(hDlg, IDD_READONLY),
                     GWL_STYLE,
                     WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_AUTO3STATE | WS_CHILD );
   }
   if (ATTR_HIDDEN & dwAttribs3State)
   {
      SetWindowLongPtr( GetDlgItem(hDlg, IDD_HIDDEN),
                     GWL_STYLE,
                     WS_VISIBLE | BS_AUTO3STATE | WS_CHILD);
   }
   if (ATTR_ARCHIVE & dwAttribs3State)
   {
      SetWindowLongPtr( GetDlgItem(hDlg, IDD_ARCHIVE),
                     GWL_STYLE,
                     WS_VISIBLE | BS_AUTO3STATE | WS_CHILD);
   }
   if (ATTR_SYSTEM & dwAttribs3State)
   {
      SetWindowLongPtr( GetDlgItem(hDlg, IDD_SYSTEM),
                     GWL_STYLE,
                     WS_VISIBLE | BS_AUTO3STATE | WS_CHILD);
   }
   if (ATTR_COMPRESSED & dwAttribs3State)
   {
      SetWindowLongPtr( GetDlgItem(hDlg, IDD_COMPRESSED),
                     GWL_STYLE,
                     WS_VISIBLE | BS_AUTO3STATE | WS_CHILD);
   }
   if (ATTR_ENCRYPTED & dwAttribs3State)
   {
      SetWindowLongPtr( GetDlgItem(hDlg, IDD_ENCRYPTED),
                     GWL_STYLE,
                     WS_VISIBLE | BS_AUTO3STATE | WS_CHILD | WS_DISABLED);
   }

   CheckAttribsDlgButton(hDlg, IDD_READONLY,   ATTR_READONLY, dwAttribs3State, dwAttribsOn);
   CheckAttribsDlgButton(hDlg, IDD_HIDDEN,     ATTR_HIDDEN, dwAttribs3State, dwAttribsOn);
   CheckAttribsDlgButton(hDlg, IDD_ARCHIVE,    ATTR_ARCHIVE, dwAttribs3State, dwAttribsOn);
   CheckAttribsDlgButton(hDlg, IDD_SYSTEM,     ATTR_SYSTEM, dwAttribs3State, dwAttribsOn);
   CheckAttribsDlgButton(hDlg, IDD_COMPRESSED, ATTR_COMPRESSED, dwAttribs3State, dwAttribsOn);
   CheckAttribsDlgButton(hDlg, IDD_ENCRYPTED,  ATTR_ENCRYPTED, dwAttribs3State, dwAttribsOn);

   return nType;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AttribsDlgProc() -                                                      */
/*                                                                          */
// assumes the active MDI child has a directory window
/*--------------------------------------------------------------------------*/

INT_PTR
AttribsDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   LPTSTR p, pSel;
   BOOL bRet;
   HCURSOR hCursor;
   DWORD dwAttribsNew, dwAttribs, dwChangeMask;
   UINT state;
   TCHAR szName[MAXPATHLEN];
   TCHAR szTemp[MAXPATHLEN];
   static INT nType;
   LPWSTR lpszValue;
   INT idx;

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {

   case WM_INITDIALOG:

      WAITNET();

      nType = InitPropertiesDialog(hDlg);
      break;

   case FS_CHANGEDISPLAY:
      {
         // Private message to make the version edit control and
         // listbox line up properly. Yes, a pain, but this is the
         // easiest way to do it...

         RECT rcVersionKey;
         RECT rcVersionValue;

         GetWindowRect(GetDlgItem(hDlg, IDD_VERSION_KEY), &rcVersionKey);
         ScreenToClient(hDlg, (POINT FAR *) &rcVersionKey.left);
         ScreenToClient(hDlg, (POINT FAR *) &rcVersionKey.right);

         GetWindowRect(GetDlgItem(hDlg, IDD_VERSION_VALUE), &rcVersionValue);
         ScreenToClient(hDlg, (POINT FAR *) &rcVersionValue.left);
         ScreenToClient(hDlg, (POINT FAR *) &rcVersionValue.right);

         SetWindowPos(GetDlgItem(hDlg, IDD_VERSION_VALUE),
         0,
         rcVersionValue.left, rcVersionKey.top,
         rcVersionValue.right - rcVersionValue.left,
         rcVersionKey.bottom-rcVersionKey.top,
         SWP_NOACTIVATE | SWP_NOZORDER);
      }

      break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
      case IDD_HELP:
         goto DoHelp;

      case IDD_NETWORKFIRST+0:
      case IDD_NETWORKFIRST+1:
      case IDD_NETWORKFIRST+2:
      case IDD_NETWORKFIRST+3:
      case IDD_NETWORKFIRST+4:
      case IDD_NETWORKFIRST+5:

         p = GetSelection(4|1|16, NULL);
         if (p) {

            WAITNET();

            if (WAITNET_LOADED) {
               WNetPropertyDialog(hDlg,
                  (WORD)(GET_WM_COMMAND_ID(wParam, lParam)-IDD_NETWORKFIRST),
                  (WORD)nType, p, WNTYPE_FILE);
            }

            LocalFree((HANDLE)p);
         }
         break;

      case IDD_VERSION_KEY:

         if (GET_WM_COMMAND_CMD(wParam,lParam) != LBN_SELCHANGE)
            break;
         idx = (INT)SendDlgItemMessage(hDlg, IDD_VERSION_KEY, LB_GETCURSEL, 0, 0L);
         lpszValue = (LPTSTR)SendDlgItemMessage(hDlg, IDD_VERSION_KEY, LB_GETITEMDATA, idx, 0L);

         SetDlgItemText(hDlg, IDD_VERSION_VALUE, lpszValue);
         break;

      case IDCANCEL:

         FreeVersionInfo();
         EndDialog(hDlg, FALSE);
         break;

      case IDOK:
      {
         BOOL bIgnoreAll = FALSE;

         bRet = TRUE;
         dwChangeMask = ATTR_READWRITE;
         dwAttribsNew = ATTR_READWRITE;

         if ((state = IsDlgButtonChecked(hDlg, IDD_READONLY)) < 2)
         {
            dwChangeMask |= ATTR_READONLY;
            if (state == 1)
            {
               dwAttribsNew |= ATTR_READONLY;
            }
         }

         if ((state = IsDlgButtonChecked(hDlg, IDD_HIDDEN)) < 2)
         {
            dwChangeMask |= ATTR_HIDDEN;
            if (state == 1)
            {
               dwAttribsNew |= ATTR_HIDDEN;
            }
         }

         if ((state = IsDlgButtonChecked(hDlg, IDD_ARCHIVE)) < 2)
         {
            dwChangeMask |= ATTR_ARCHIVE;
            if (state == 1)
            {
               dwAttribsNew |= ATTR_ARCHIVE;
            }
         }

         if ((state = IsDlgButtonChecked(hDlg, IDD_SYSTEM)) < 2)
         {
            dwChangeMask |= ATTR_SYSTEM;
            if (state == 1)
            {
               dwAttribsNew |= ATTR_SYSTEM;
            }
         }

         if ((state = IsDlgButtonChecked(hDlg, IDD_COMPRESSED)) < 2)
         {
            dwChangeMask |= ATTR_COMPRESSED;
            if (state == 1)
            {
               dwAttribsNew |= ATTR_COMPRESSED;
            }
         }

         //
         // Free old version buffer
         // (Ok to call even if no version info present.)
         //
         FreeVersionInfo();
         EndDialog(hDlg, bRet);

         pSel = GetSelection(0, NULL);

         if (!pSel)
            break;

         hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
         ShowCursor(TRUE);

         SendMessage(hwndFrame, FS_DISABLEFSC, 0, 0L);

         p = pSel;

         while (p = GetNextFile(p, szName, COUNTOF(szName))) {

            QualifyPath(szName);

            dwAttribs = GetFileAttributes(szName);

            if (dwAttribs == INVALID_FILE_ATTRIBUTES)
               goto AttributeError;
            else
               dwAttribs &= ~ATTR_DIR;

            //
            // Only try and change the attributes if we need to change
            // attributes.
            //
            if ((dwAttribs ^ dwAttribsNew) & dwChangeMask)
            {
               dwAttribs = (dwChangeMask & dwAttribsNew) | (~dwChangeMask & dwAttribs);

               //
               //  Process the IDD_COMPRESSED attribute setting and then
               //  set the attributes on the file or directory.
               //
               lstrcpy(szTemp, szName);
               if (!WFCheckCompress(hDlg, szTemp, dwAttribs, TRUE, &bIgnoreAll))
               {
                  //
                  //  WFCheckCompress will only be FALSE if the user
                  //  chooses to Abort the compression.
                  //
                  bRet = FALSE;
                  break;
               }
               if (WFSetAttr(szName, dwAttribs))
               {
AttributeError:
                  GetWindowText(hDlg, szTitle, COUNTOF(szTitle));
                  LoadString(hAppInstance, IDS_ATTRIBERR, szMessage, COUNTOF(szMessage));

                  FormatError(FALSE, szMessage, COUNTOF(szMessage), GetLastError());
                  MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);

                  bRet = FALSE;
                  break;
               }
            }

            //
            //  Clear all the FSC messages from the message queue.
            //
            wfYield();
         }

         SendMessage(hwndFrame, FS_ENABLEFSC, 0, 0L);

         ShowCursor(FALSE);
         SetCursor(hCursor);

         LocalFree((HANDLE)pSel);

         break;
      }
      default:
         return FALSE;
      }
      break;

   default:

      if (wMsg == wHelpMessage) {
DoHelp:
         WFHelp(hDlg);

         return TRUE;
      } else
         return FALSE;
   }
   return TRUE;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MakeDirDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
MakeDirDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   //
   // Must be at least MAXPATHLEN
   //
   TCHAR szPath[MAXPATHLEN*2];
   INT ret;

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {
   case WM_INITDIALOG:
      SetDlgDirectory(hDlg, NULL);
      SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, MAXPATHLEN-1, 0L);
      break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)){
      case IDD_HELP:
         goto DoHelp;

      case IDCANCEL:
         EndDialog(hDlg, FALSE);
         break;

      case IDOK:

         GetDlgItemText(hDlg, IDD_NAME, szPath, MAXPATHLEN);
         EndDialog(hDlg, TRUE);

         //
         // If "a b" typed in (no quotes, just a b) do we create two
         // directors, "a" and "b," or create just one: "a b."
         // For now, create just one.  (No need to call checkesc!)
         //
         // put it back in to handle quoted things.
         // Now, it _ignores_ extra files on the line.  We may wish to return
         // an error; that would be smart...
         //
         if (NoQuotes(szPath)) {
            CheckEsc(szPath);
         }

         GetNextFile(szPath, szPath, COUNTOF(szPath));

         QualifyPath(szPath);

         hdlgProgress = hDlg;

#ifdef NETCHECK
         if (NetCheck(szPath,WNDN_MKDIR) == WN_SUCCESS) {
#endif
            SendMessage(hwndFrame, FS_DISABLEFSC, 0, 0L);

            ret = WF_CreateDirectory(hDlg, szPath, NULL);

            if (ret && ret!=DE_OPCANCELLED) {
               // Handle error messages cleanly.
               // Special case ERROR_ALREADY_EXISTS

               if ( ERROR_ALREADY_EXISTS == ret ) {
                  ret = WFIsDir(szPath) ?
                     DE_MAKEDIREXISTS :
                     DE_DIREXISTSASFILE;
               }

               LoadString(hAppInstance, IDS_MAKEDIRERR, szMessage, COUNTOF(szMessage));
               FormatError(FALSE, szMessage, COUNTOF(szMessage), ret);

               GetWindowText(hDlg, szTitle, COUNTOF(szTitle));
               MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);
            }

            SendMessage(hwndFrame, FS_ENABLEFSC, 0, 0L);
#ifdef NETCHECK
         }
#endif

         break;

      default:
         return FALSE;
      }
      break;

   default:

      if (wMsg == wHelpMessage) {
DoHelp:
         WFHelp(hDlg);

         return TRUE;
      } else
         return FALSE;
   }
   return TRUE;
}


// Check if szT has quote in it.
// could use strchr...

BOOL
NoQuotes(LPTSTR szT)
{
   while (*szT) {
      if (CHAR_DQUOTE == *szT ) return FALSE;

      szT++;
   }

   return TRUE;
}

