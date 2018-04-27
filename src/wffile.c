/********************************************************************

   wffile.c

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"
#include "treectl.h"


//
//  Constant Declarations.
//
#define PROGRESS_UPD_FILENAME            1
#define PROGRESS_UPD_DIRECTORY           2
#define PROGRESS_UPD_FILEANDDIR          3
#define PROGRESS_UPD_DIRCNT              4
#define PROGRESS_UPD_FILECNT             5
#define PROGRESS_UPD_COMPRESSEDSIZE      6
#define PROGRESS_UPD_FILESIZE            7
#define PROGRESS_UPD_PERCENTAGE          8
#define PROGRESS_UPD_FILENUMBERS         9
#define PROGRESS_UPD_FINAL              10


//
//  Return values for CompressErrMessageBox routine.
//
#define WF_RETRY_CREATE     1
#define WF_RETRY_DEVIO      2
//      IDABORT             3
//      IDIGNORE            5



//
//  Global variables to hold the User option information.
//
BOOL DoSubdirectories = FALSE;

BOOL bShowProgress    = FALSE;
HANDLE hDlgProgress   = NULL;

BOOL bCompressReEntry = FALSE;
BOOL bIgnoreAllErrors = FALSE;


//
//  Global variables to hold compression statistics.
//
LONGLONG TotalDirectoryCount        = 0;
LONGLONG TotalFileCount             = 0;
LONGLONG TotalCompressedFileCount   = 0;
LONGLONG TotalUncompressedFileCount = 0;

LARGE_INTEGER TotalFileSize;
LARGE_INTEGER TotalCompressedSize;

TCHAR  szGlobalFile[MAXPATHLEN];
TCHAR  szGlobalDir[MAXPATHLEN];

HDC   hDCdir = NULL;
DWORD dxdir;



//
//  Forward Declarations.
//
BOOL
WFDoCompress(
    HWND hDlg,
    LPTSTR DirectorySpec,
    LPTSTR FileSpec);

BOOL
WFDoUncompress(
    HWND hDlg,
    LPTSTR DirectorySpec,
    LPTSTR FileSpec);

VOID
RedrawAllTreeWindows(VOID);

int
CompressErrMessageBox(
    HWND hwndActive,
    LPTSTR szFile,
    PHANDLE phFile);

BOOL CALLBACK
CompressErrDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
OpenFileForCompress(
    PHANDLE phFile,
    LPTSTR szFile);




/////////////////////////////////////////////////////////////////////////////
//
//  MKDir
//
//  Creates the given directory.
//
/////////////////////////////////////////////////////////////////////////////

DWORD MKDir(
    LPTSTR pName,
    LPTSTR pSrc)
{
   DWORD dwErr = ERROR_SUCCESS;

   if ((pSrc && *pSrc) ?
         CreateDirectoryEx(pSrc, pName, NULL) :
         CreateDirectory(pName, NULL))
   {
      ChangeFileSystem(FSC_MKDIR, pName, NULL);
   }
   else
   {
      dwErr = GetLastError();
   }

   return (dwErr);
}


/////////////////////////////////////////////////////////////////////////////
//
//  RMDir
//
//  Removes the given directory.
//
/////////////////////////////////////////////////////////////////////////////

DWORD RMDir(
    LPTSTR pName)
{
   DWORD dwErr = 0;

   if (RemoveDirectory(pName))
   {
      ChangeFileSystem(FSC_RMDIR, pName, NULL);
   }
   else
   {
      dwErr = (WORD)GetLastError();
   }

   return (dwErr);
}


/////////////////////////////////////////////////////////////////////////////
//
//  WFSetAttr
//
//  Sets the file attributes.
//
/////////////////////////////////////////////////////////////////////////////

BOOL WFSetAttr(
    LPTSTR lpFile,
    DWORD dwAttr)
{
   BOOL bRet;

   //
   //  Compression attribute is handled separately -
   //  do not try to set it here.
   //
   dwAttr = dwAttr & ~(ATTR_COMPRESSED | ATTR_ENCRYPTED);

   bRet = SetFileAttributes(lpFile, dwAttr);

   if (bRet)
   {
      ChangeFileSystem(FSC_ATTRIBUTES, lpFile, NULL);
   }

   return ( (BOOL)!bRet );
}


//////////////////////////////////////////////////////////////////////////////
//
//  CentreWindow
//
//  Positions a window so that it is centered in its parent.
//
//////////////////////////////////////////////////////////////////////////////

VOID CentreWindow(
    HWND hwnd)
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    //
    //  Get window rect.
    //
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    //
    //  Get parent rect.
    //
    Style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0)
    {
        hwndParent = GetDesktopWindow();
    }
    else
    {
        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL)
        {
            hwndParent = GetDesktopWindow();
        }
    }
    GetWindowRect(hwndParent, &rectParent);

    dxParent = rectParent.right - rectParent.left;
    dyParent = rectParent.bottom - rectParent.top;

    //
    //  Centre the child in the parent.
    //
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    //
    //  Move the child into position.
    //
    SetWindowPos( hwnd,
                  NULL,
                  rect.left,
                  rect.top,
                  0,
                  0,
                  SWP_NOSIZE | SWP_NOZORDER );

    SetForegroundWindow(hwnd);
}


/////////////////////////////////////////////////////////////////////////////
//
//  wfProgressYield
//
//  Allow other messages including Dialog messages for Modeless dialog to be
//  processed while we are Compressing and Uncompressing files.  This message
//  loop is similar to "wfYield" in treectl.c except that it allows for the
//  processing of Modeless dialog messages also (specifically for the Progress
//  Dialogs).
//
//  Since the file/directory Compression/Uncompression is done on a single
//  thread (in order to keep it synchronous with the existing Set Attributes
//  processing) we need to provide a mechanism that will allow a user to
//  Cancel out of the operation and also allow window messages, like WM_PAINT,
//  to be processed by other Window Procedures.
//
/////////////////////////////////////////////////////////////////////////////

VOID wfProgressYield()
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!hDlgProgress || !IsDialogMessage(hDlgProgress, &msg))
        {
            if (!TranslateMDISysAccel(hwndMDIClient, &msg) &&
                (!hwndFrame || !TranslateAccelerator(hwndFrame, hAccel, &msg)))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//  DisplayUncompressProgress
//
//  Update the progress of uncompressing files.
//
//  This routine uses the global variables to update the Dialog box items
//  which display the progress through the uncompression process.  The global
//  variables are updated by individual routines.  An ordinal value is sent
//  to this routine which determines which dialog box item to update.
//
/////////////////////////////////////////////////////////////////////////////

VOID DisplayUncompressProgress(
    int iType)
{
    TCHAR szNum[30];

    if (!bShowProgress)
    {
        return;
    }

    switch (iType)
    {
        case ( PROGRESS_UPD_FILEANDDIR ) :
        case ( PROGRESS_UPD_FILENAME ) :
        {
            SetDlgItemText(hDlgProgress, IDD_UNCOMPRESS_FILE, szGlobalFile);
            if (iType != PROGRESS_UPD_FILEANDDIR)
            {
                break;
            }

            // else...fall through
        }
        case ( PROGRESS_UPD_DIRECTORY ) :
        {
            //
            //  Preprocess the directory name to shorten it to fit
            //  into our space.
            //
            CompactPath(hDCdir, szGlobalDir, dxdir);
            SetDlgItemText(hDlgProgress, IDD_UNCOMPRESS_DIR, szGlobalDir);

            break;
        }
        case ( PROGRESS_UPD_DIRCNT ) :
        {
            AddCommasInternal(szNum, (DWORD)TotalDirectoryCount);
            SetDlgItemText(hDlgProgress, IDD_UNCOMPRESS_TDIRS, szNum);

            break;
        }
        case ( PROGRESS_UPD_FILENUMBERS ) :
        case ( PROGRESS_UPD_FILECNT ) :
        {
            AddCommasInternal(szNum, (DWORD)TotalFileCount);
            SetDlgItemText(hDlgProgress, IDD_UNCOMPRESS_TFILES, szNum);

            break;
        }
    }

    wfProgressYield();
}


/////////////////////////////////////////////////////////////////////////////
//
//  UncompressProgDlg
//
//  Display progress messages to user based on progress in uncompressing
//  files and directories.
//
//  NOTE:  This is a modeless dialog and must be terminated with
//         DestroyWindow and NOT EndDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY UncompressProgDlg(
    HWND hDlg,
    UINT nMsg,
    DWORD wParam,
    LPARAM lParam)
{
    TCHAR szTemp[120];
    RECT  rect;

    switch (nMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            CentreWindow(hDlg);

            hDlgProgress = hDlg;

            //
            //  Clear Dialog items.
            //
            szTemp[0] = TEXT('\0');

            SetDlgItemText(hDlg, IDD_UNCOMPRESS_FILE, szTemp);
            SetDlgItemText(hDlg, IDD_UNCOMPRESS_DIR,  szTemp);
            SetDlgItemText(hDlg, IDD_UNCOMPRESS_TDIRS, szTemp);
            SetDlgItemText(hDlg, IDD_UNCOMPRESS_TFILES, szTemp);

            hDCdir = GetDC(GetDlgItem(hDlg, IDD_UNCOMPRESS_DIR));
            GetClientRect(GetDlgItem(hDlg, IDD_UNCOMPRESS_DIR), &rect);
            dxdir = rect.right;

            //
            // Set Dialog message text
            //
            EnableWindow(hDlg, TRUE);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                case ( IDCANCEL ) :
                {
                    if (hDCdir)
                    {
                        ReleaseDC(GetDlgItem(hDlg, IDD_UNCOMPRESS_DIR), hDCdir);
                        hDCdir = NULL;
                    }
                    DestroyWindow(hDlg);
                    hDlgProgress = NULL;
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  DisplayCompressProgress
//
//  Update the progress of compressing files.
//
//  This routine uses the global variables to update the Dialog box items
//  which display the progress through the compression process.  The global
//  variables are updated by individual routines.  An ordinal value is sent
//  to this routine which determines which dialog box item to update.
//
/////////////////////////////////////////////////////////////////////////////

void DisplayCompressProgress(
    int iType)
{
    TCHAR szTemp[120];
    TCHAR szNum[30];
    LARGE_INTEGER Percentage;


    if (!bShowProgress)
    {
        return;
    }

    switch (iType)
    {
        case ( PROGRESS_UPD_FILEANDDIR ) :
        case ( PROGRESS_UPD_FILENAME ) :
        {
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_FILE, szGlobalFile);
            if (iType != PROGRESS_UPD_FILEANDDIR)
            {
                break;
            }

            // else...fall through
        }
        case ( PROGRESS_UPD_DIRECTORY ) :
        {
            //
            //  Preprocess the directory name to shorten it to fit
            //  into our space.
            //
            CompactPath(hDCdir, szGlobalDir, dxdir);
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_DIR, szGlobalDir);

            break;
        }
        case ( PROGRESS_UPD_DIRCNT ) :
        {
            AddCommasInternal(szNum, (DWORD)TotalDirectoryCount);
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_TDIRS, szNum);

            break;
        }
        case ( PROGRESS_UPD_FILENUMBERS ) :
        case ( PROGRESS_UPD_FILECNT ) :
        {
            AddCommasInternal(szNum, (DWORD)TotalFileCount);
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_TFILES, szNum);
            if (iType != PROGRESS_UPD_FILENUMBERS)
            {
                break;
            }

            // else...fall through
        }
        case ( PROGRESS_UPD_COMPRESSEDSIZE ) :
        {
            PutSize(&TotalCompressedSize, szNum);
            wsprintf(szTemp, szSBytes, szNum);
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_CSIZE, szTemp);
            if (iType != PROGRESS_UPD_FILENUMBERS)
            {
                break;
            }

            // else...fall through
        }
        case ( PROGRESS_UPD_FILESIZE ) :
        {
            PutSize(&TotalFileSize, szNum);
            wsprintf(szTemp, szSBytes, szNum);
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_USIZE, szTemp);
            if (iType != PROGRESS_UPD_FILENUMBERS)
            {
                break;
            }

            // else...fall through
        }
        case ( PROGRESS_UPD_PERCENTAGE ) :
        {
            if (TotalFileSize.QuadPart != 0)
            {
                //
                //  Percentage = 100 - ((CompressSize * 100) / FileSize)
                //
                Percentage.QuadPart =
                    (TotalCompressedSize.QuadPart * 100) /
                    TotalFileSize.QuadPart;

                if (Percentage.HighPart || (Percentage.LowPart > 100))
                {
                    //
                    //  Percentage = 100%
                    //
                    Percentage.LowPart = 100;
                    Percentage.HighPart = 0;
                }
                else
                {
                    Percentage.LowPart = 100 - Percentage.LowPart;
                }
            }
            else
            {
                //
                //  Percentage = 0%
                //
                Percentage.LowPart = 0;
                Percentage.HighPart = 0;
            }
            PutSize(&Percentage, szNum);
            wsprintf(szTemp, TEXT("%s%%"), szNum);
            SetDlgItemText(hDlgProgress, IDD_COMPRESS_RATIO, szTemp);

            break;
        }
    }

    wfProgressYield();
}


/////////////////////////////////////////////////////////////////////////////
//
//  CompressProgDlg
//
//  Display progress messages to user based on progress in converting
//  font files to TrueType
//
//  NOTE:  This is a modeless dialog and must be terminated with DestroyWindow
//         and NOT EndDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY CompressProgDlg(
    HWND hDlg,
    UINT nMsg,
    DWORD wParam,
    LPARAM lParam)
{
    TCHAR szTemp[120];
    RECT  rect;

    switch (nMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            CentreWindow(hDlg);

            hDlgProgress = hDlg;

            //
            //  Clear Dialog items.
            //
            szTemp[0] = TEXT('\0');

            SetDlgItemText(hDlg, IDD_COMPRESS_FILE, szTemp);
            SetDlgItemText(hDlg, IDD_COMPRESS_DIR,  szTemp);
            SetDlgItemText(hDlg, IDD_COMPRESS_TDIRS, szTemp);
            SetDlgItemText(hDlg, IDD_COMPRESS_TFILES, szTemp);
            SetDlgItemText(hDlg, IDD_COMPRESS_CSIZE, szTemp);
            SetDlgItemText(hDlg, IDD_COMPRESS_USIZE, szTemp);
            SetDlgItemText(hDlg, IDD_COMPRESS_RATIO, szTemp);

            hDCdir = GetDC(GetDlgItem(hDlg, IDD_COMPRESS_DIR));
            GetClientRect(GetDlgItem(hDlg, IDD_COMPRESS_DIR), &rect);
            dxdir = rect.right;

            //
            // Set Dialog message text.
            //
            LoadString(hAppInstance, IDS_COMPRESSDIR, szTemp, COUNTOF(szTemp));
            EnableWindow(hDlg, TRUE);

            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                case ( IDCANCEL ) :
                {
                    if (hDCdir)
                    {
                        ReleaseDC(GetDlgItem(hDlg, IDD_COMPRESS_DIR), hDCdir);
                        hDCdir = NULL;
                    }
                    DestroyWindow(hDlg);
                    hDlgProgress = NULL;

                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  WFCheckCompress
//
//  Given a path and information determine if User wants to compress all
//  files and sub-dirs in directory, or just add attribute to file/dir.
//  Display progress and statistics during compression.
//
/////////////////////////////////////////////////////////////////////////////

BOOL WFCheckCompress(
    HWND hDlg,
    LPTSTR szNameSpec,
    DWORD dwNewAttrs,
    BOOL bPropertyDlg,
    BOOL *bIgnoreAll)
{
    DWORD   dwFlags, dwAttribs;
    TCHAR   szTitle[MAXTITLELEN];
    TCHAR   szTemp[MAXMESSAGELEN];
    TCHAR   szFilespec[MAXPATHLEN];
    BOOL    bCompressionAttrChange;
    BOOL    bIsDir;
    BOOL    bRet = TRUE;
    HCURSOR hCursor;


    //
    //  Make sure we're not in the middle of another compression operation.
    //  If so, put up an error box warning the user that they need to wait
    //  to do another compression operation.
    //
    if (bCompressReEntry)
    {
        LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));
        LoadString(hAppInstance, IDS_MULTICOMPRESSERR, szMessage, COUNTOF(szMessage));
        MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

        return (TRUE);
    }
    bCompressReEntry = TRUE;

    //
    //  Make sure the volume supports File Compression.
    //
    GetRootPath (szNameSpec, szTemp);

    if (GetVolumeInformation (szTemp, NULL, 0L, NULL, NULL, &dwFlags, NULL, 0L)
        && (!(dwFlags & FS_FILE_COMPRESSION)) )
    {
        //
        //  The volume does not support file compression, so just
        //  quit out.  Do not return FALSE, since that will not
        //  allow any other attributes to be changed.
        //
        bCompressReEntry = FALSE;
        return (TRUE);
    }

    //
    //  Show the hour glass cursor.
    //
    if (hCursor = LoadCursor(NULL, IDC_WAIT))
    {
        hCursor = SetCursor(hCursor);
    }
    ShowCursor(TRUE);

    //
    //  Get the file attributes.
    //
    dwAttribs = GetFileAttributes(szNameSpec);

    //
    //  Determine if ATTR_COMPRESSED is changing state.
    //
    bCompressionAttrChange = !( (dwAttribs & ATTR_COMPRESSED) ==
                                (dwNewAttrs & ATTR_COMPRESSED) );

    //
    //  Initialize progress global.
    //
    bShowProgress = FALSE;

    //
    //  Initialize ignore all errors global.
    //
    bIgnoreAllErrors = *bIgnoreAll;

    //
    //  If the Compression attribute changed or if we're dealing with
    //  a directory, perform action.
    //
    bIsDir = IsDirectory(szNameSpec);
    if (bCompressionAttrChange || (bIsDir && !bPropertyDlg))
    {
        //
        //  Reset globals before progress display.
        //
        TotalDirectoryCount        = 0;
        TotalFileCount             = 0;
        TotalCompressedFileCount   = 0;
        TotalUncompressedFileCount = 0;

        LARGE_INTEGER_NULL(TotalFileSize);
        LARGE_INTEGER_NULL(TotalCompressedSize);

        szGlobalFile[0] = '\0';
        szGlobalDir[0] = '\0';

        //
        //  See if the compressed attribute is set.
        //
        if (dwNewAttrs & ATTR_COMPRESSED)
        {
            if (bIsDir)
            {
                LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));
                LoadString(hAppInstance, IDS_COMPRESSDIR, szMessage, COUNTOF(szMessage));

                //
                //  Ask the user if we should compress all files and
                //  subdirs in this directory.
                //
                wsprintf(szTemp, szMessage, szNameSpec);

                switch (MessageBox(hDlg, szTemp, szTitle,
                          MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL))
                {
                    case ( IDYES ) :
                    {
                        lstrcpy(szFilespec, SZ_STAR);
                        DoSubdirectories = TRUE;
                        bShowProgress = TRUE;

                        break;
                    }
                    case ( IDCANCEL ) :
                    {
                        goto CancelCompress;
                    }
                    case ( IDNO ) :
                    default :
                    {
                        szFilespec[0] = TEXT('\0');
                        DoSubdirectories = FALSE;

                        break;
                    }
                }

                if (bShowProgress)
                {
                    hDlgProgress = CreateDialog(
                                       hAppInstance,
                                       MAKEINTRESOURCE(COMPRESSPROGDLG),
                                       hwndFrame,
                                       (DLGPROC)CompressProgDlg);

                    ShowWindow(hDlgProgress, SW_SHOW);
                }

                AddBackslash(szNameSpec);
                lstrcpy(szTemp, szNameSpec);

                bRet = WFDoCompress(hDlg, szNameSpec, szFilespec);

                //
                //  Set attribute on Directory if last call was successful.
                //
                if (bRet)
                {
                    szFilespec[0] = TEXT('\0');
                    DoSubdirectories = FALSE;
                    lstrcpy(szNameSpec, szTemp);
                    bRet = WFDoCompress(hDlg, szNameSpec, szFilespec);
                }

                if (bShowProgress && hDlgProgress)
                {
                    if (hDCdir)
                    {
                        ReleaseDC( GetDlgItem(hDlgProgress, IDD_COMPRESS_DIR),
                                   hDCdir );
                        hDCdir = NULL;
                    }
                    DestroyWindow(hDlgProgress);
                    hDlgProgress = NULL;
                }
            }
            else
            {
                //
                //  Compress single file.
                //
                DoSubdirectories = FALSE;

                lstrcpy(szFilespec, szNameSpec);

                StripPath(szFilespec);
                StripFilespec(szNameSpec);

                AddBackslash(szNameSpec);

                bRet = WFDoCompress(hDlg, szNameSpec, szFilespec);
             }
        }
        else
        {
            if (bIsDir)
            {
                LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));
                LoadString(hAppInstance, IDS_UNCOMPRESSDIR, szMessage, COUNTOF(szMessage));

                //
                //  Ask the user if we should uncompress all files and
                //  subdirs in this directory.
                //
                wsprintf(szTemp, szMessage, szNameSpec);

                switch (MessageBox(hDlg, szTemp, szTitle,
                          MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL))
                {
                    case ( IDYES ) :
                    {
                        lstrcpy(szFilespec, SZ_STAR);
                        DoSubdirectories = TRUE;
                        bShowProgress = TRUE;

                        break;
                    }
                    case ( IDCANCEL ) :
                    {
                        goto CancelCompress;
                    }
                    case ( IDNO ) :
                    default :
                    {
                        szFilespec[0] = TEXT('\0');
                        DoSubdirectories = FALSE;

                        break;
                    }
                }

                if (bShowProgress)
                {
                    hDlgProgress = CreateDialog(
                                       hAppInstance,
                                       MAKEINTRESOURCE(UNCOMPRESSPROGDLG),
                                       hwndFrame,
                                       (DLGPROC) UncompressProgDlg);

                    ShowWindow(hDlgProgress, SW_SHOW);
                }

                AddBackslash(szNameSpec);
                lstrcpy(szTemp, szNameSpec);

                bRet = WFDoUncompress(hDlg, szNameSpec, szFilespec);

                //
                //  Set attribute on Directory if last call was successful.
                //
                if (bRet)
                {
                    szFilespec[0] = TEXT('\0');
                    DoSubdirectories = FALSE;
                    lstrcpy(szNameSpec, szTemp);
                    bRet = WFDoUncompress(hDlg, szNameSpec, szFilespec);
                }

                if (bShowProgress && hDlgProgress)
                {
                    if (hDCdir)
                    {
                        ReleaseDC( GetDlgItem(hDlgProgress, IDD_UNCOMPRESS_DIR),
                                   hDCdir );
                        hDCdir = NULL;
                    }
                    DestroyWindow(hDlgProgress);
                    hDlgProgress = NULL;
                }
            }
            else
            {
                //
                //  Uncompress single file.
                //
                DoSubdirectories = FALSE;

                lstrcpy(szFilespec, szNameSpec);

                StripPath(szFilespec);
                StripFilespec(szNameSpec);

                AddBackslash(szNameSpec);

                bRet = WFDoUncompress(hDlg, szNameSpec, szFilespec);
            }
        }

        if (bIsDir)
        {
            //
            //  Need to redraw all of the tree list box windows
            //  so that they can be updated to show whether or not
            //  the directories are compressed.
            //
            RedrawAllTreeWindows();
        }
    }

CancelCompress:

    //
    //  Reset the cursor.
    //
    if (hCursor)
    {
        SetCursor(hCursor);
    }
    ShowCursor(FALSE);

    //
    //  Reset the globals.
    //
    *bIgnoreAll = bIgnoreAllErrors;
    bCompressReEntry = FALSE;

    //
    //  Return the appropriate value.
    //
    return (bRet);
}


/////////////////////////////////////////////////////////////////////////////
//
//  CompressFile
//
//  Compresses a single file.
//
/////////////////////////////////////////////////////////////////////////////

BOOL CompressFile(
    HANDLE Handle,
    LPTSTR FileSpec,
    PWIN32_FIND_DATA FindData)
{
    USHORT State;
    ULONG Length;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER CompressedSize;


    //
    //  Print out the file name and then do the Ioctl to compress the
    //  file.  When we are done we'll print the okay message.
    //
    lstrcpy(szGlobalFile, FindData->cFileName);
    DisplayCompressProgress(PROGRESS_UPD_FILENAME);

    State = 1;
    if (!DeviceIoControl( Handle,
                          FSCTL_SET_COMPRESSION,
                          &State,
                          sizeof(USHORT),
                          NULL,
                          0,
                          &Length,
                          FALSE ))
    {
        return (FALSE);
    }

    //
    //  Gather statistics.
    //
    FileSize.LowPart = FindData->nFileSizeLow;
    FileSize.HighPart = FindData->nFileSizeHigh;
    CompressedSize.LowPart = GetCompressedFileSize( FileSpec,
                                                    &(CompressedSize.HighPart) );

    //
    //  Increment the running total.
    //
    TotalFileSize.QuadPart += FileSize.QuadPart;
    TotalCompressedSize.QuadPart += CompressedSize.QuadPart;
    TotalFileCount++;

    DisplayCompressProgress(PROGRESS_UPD_FILENUMBERS);

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  WFDoCompress
//
//  Compresses a directory and its subdirectories (if necessary).
//
/////////////////////////////////////////////////////////////////////////////

BOOL WFDoCompress(
    HWND hDlg,
    LPTSTR DirectorySpec,
    LPTSTR FileSpec)
{
    LPTSTR DirectorySpecEnd;
    HANDLE FileHandle;
    USHORT State;
    ULONG  Length;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    int MBRet;


    //
    //  If the file spec is null, then set the compression bit for
    //  the directory spec and get out.
    //
    lstrcpy(szGlobalDir, DirectorySpec);
    DisplayCompressProgress(PROGRESS_UPD_DIRECTORY);

    if (lstrlen(FileSpec) == 0)
    {
DoCompressRetryCreate:

        if (!OpenFileForCompress(&FileHandle, DirectorySpec))
        {
            goto DoCompressError;
        }

DoCompressRetryDevIo:

        State = 1;
        if (!DeviceIoControl( FileHandle,
                              FSCTL_SET_COMPRESSION,
                              &State,
                              sizeof(USHORT),
                              NULL,
                              0,
                              &Length,
                              FALSE ))
        {
DoCompressError:

            if (!bIgnoreAllErrors)
            {
                MBRet = CompressErrMessageBox( hDlg,
                                               DirectorySpec,
                                               &FileHandle );
                if (MBRet == WF_RETRY_CREATE)
                {
                    goto DoCompressRetryCreate;
                }
                else if (MBRet == WF_RETRY_DEVIO)
                {
                    goto DoCompressRetryDevIo;
                }
                else if (MBRet == IDABORT)
                {
                    //
                    //  Return error.
                    //  File handle closed by CompressErrMessageBox.
                    //
                    return (FALSE);
                }
                //
                //  Else (MBRet == IDIGNORE)
                //  Continue on as if the error did not occur.
                //
            }
        }
        if (INVALID_HANDLE_VALUE != FileHandle)
        {
            CloseHandle(FileHandle);
            FileHandle = INVALID_HANDLE_VALUE;
        }

        TotalDirectoryCount++;
        TotalFileCount++;

        DisplayCompressProgress(PROGRESS_UPD_DIRCNT);
        DisplayCompressProgress(PROGRESS_UPD_FILECNT);

        ChangeFileSystem(FSC_ATTRIBUTES, DirectorySpec, NULL);

        return (TRUE);
    }

    //
    //  Get a pointer to the end of the directory spec, so that we can
    //  keep appending names to the end of it.
    //
    DirectorySpecEnd = DirectorySpec + lstrlen(DirectorySpec);

    //
    //  List the directory that is being compressed and display
    //  its current compress attribute.
    //
    TotalDirectoryCount++;
    DisplayCompressProgress(PROGRESS_UPD_DIRCNT);

    //
    //  For every file in the directory that matches the file spec,
    //  open the file and compress it.
    //
    //  Setup the template for findfirst/findnext.
    //
    lstrcpy(DirectorySpecEnd, FileSpec);

    if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            //
            //  Make sure the user hasn't hit cancel.
            //
            if (bShowProgress && !hDlgProgress)
            {
                break;
            }

            //
            //  Skip over the . and .. entries.
            //
            if ( !lstrcmp(FindData.cFileName, SZ_DOT) ||
                 !lstrcmp(FindData.cFileName, SZ_DOTDOT) )
            {
                continue;
            }
            else if ( (DirectorySpecEnd == (DirectorySpec + 3)) &&
                      !lstrcmpi(FindData.cFileName, SZ_NTLDR) )
            {
                //
                //  Do not allow \NTLDR to be compressed.
                //  Put up OK message box and then continue.
                //
                lstrcpy(DirectorySpecEnd, FindData.cFileName);
                LoadString(hAppInstance, IDS_NTLDRCOMPRESSERR, szTitle, COUNTOF(szTitle));
                wsprintf(szMessage, szTitle, DirectorySpec);

                LoadString(hAppInstance, IDS_WINFILE, szTitle, COUNTOF(szTitle));

                MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

                continue;
            }
            else
            {
                //
                //  Append the found file to the directory spec and
                //  open the file.
                //
                lstrcpy(DirectorySpecEnd, FindData.cFileName);

                if (GetFileAttributes(DirectorySpec) & ATTR_COMPRESSED)
                {
                    //
                    //  Already compressed, so just get the next file.
                    //
                    continue;
                }

CompressFileRetryCreate:

                if (!OpenFileForCompress(&FileHandle, DirectorySpec))
                {
                    goto CompressFileError;
                }

CompressFileRetryDevIo:

                //
                //  Compress the file.
                //
                if (!CompressFile(FileHandle, DirectorySpec, &FindData))
                {
CompressFileError:

                    if (!bIgnoreAllErrors)
                    {
                        MBRet = CompressErrMessageBox( hDlg,
                                                       DirectorySpec,
                                                       &FileHandle );
                        if (MBRet == WF_RETRY_CREATE)
                        {
                            goto CompressFileRetryCreate;
                        }
                        else if (MBRet == WF_RETRY_DEVIO)
                        {
                            goto CompressFileRetryDevIo;
                        }
                        else if (MBRet == IDABORT)
                        {
                            //
                            //  Return error.
                            //  File handle was closed by CompressErrMessageBox.
                            //
                            FindClose(FindHandle);
                            return (FALSE);
                        }
                        //
                        //  Else (MBRet == IDIGNORE)
                        //  Continue on as if the error did not occur.
                        //
                    }
                }
                if (INVALID_HANDLE_VALUE != FileHandle)
                {
                    //
                    // Close the file and get the next file.
                    //
                    CloseHandle(FileHandle);
                    FileHandle = INVALID_HANDLE_VALUE;
                }
            }

        } while (FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);

        ChangeFileSystem(FSC_ATTRIBUTES, DirectorySpec, NULL);
    }

    //
    //  If we are to do subdirectores, then look for every subdirectory
    //  and recursively call ourselves to list the subdirectory.
    //
    if (DoSubdirectories && hDlgProgress)
    {
        //
        //  Setup findfirst/findnext to search the entire directory.
        //
        lstrcpy(DirectorySpecEnd, SZ_STAR);

        if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //
                //  Skip over the . and .. entries, otherwise recurse.
                //
                if ( !lstrcmp(FindData.cFileName, SZ_DOT) ||
                     !lstrcmp(FindData.cFileName, SZ_DOTDOT) )
                {
                    continue;
                }
                else
                {
                    //
                    //  If the entry is for a directory, then tack
                    //  on the subdirectory name to the directory spec
                    //  and recurse.
                    //
                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        lstrcpy(DirectorySpecEnd, FindData.cFileName);
                        lstrcat(DirectorySpecEnd, SZ_BACKSLASH);

                        if (!WFDoCompress(hDlg, DirectorySpec, FileSpec))
                        {
                            FindClose(FindHandle);
                            return (FALSE);
                        }
                    }
                }

            } while (FindNextFile(FindHandle, &FindData));

            FindClose(FindHandle);
        }
    }

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  UncompressFile
//
//  Uncompresses a single file.
//
/////////////////////////////////////////////////////////////////////////////

BOOL UncompressFile(
    HANDLE Handle,
    PWIN32_FIND_DATA FindData)
{
    USHORT State;
    ULONG Length;


    //
    //  Print out the file name and then do the Ioctl to uncompress the
    //  file.  When we are done we'll print the okay message.
    //
    lstrcpy(szGlobalFile, FindData->cFileName);
    DisplayUncompressProgress(PROGRESS_UPD_FILENAME);

    State = 0;
    if (!DeviceIoControl( Handle,
                          FSCTL_SET_COMPRESSION,
                          &State,
                          sizeof(USHORT),
                          NULL,
                          0,
                          &Length,
                          FALSE ))
    {
        return (FALSE);
    }

    //
    //  Increment the running total.
    //
    TotalFileCount++;
    DisplayUncompressProgress(PROGRESS_UPD_FILENUMBERS);

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  WFDoUncompress
//
//  Uncompresses a directory and its subdirectories (if necessary).
//
/////////////////////////////////////////////////////////////////////////////

BOOL WFDoUncompress(
    HWND hDlg,
    LPTSTR DirectorySpec,
    LPTSTR FileSpec)
{
    LPTSTR DirectorySpecEnd;
    HANDLE FileHandle;
    USHORT State;
    ULONG  Length;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    int MBRet;


    //
    //  If the file spec is null, then clear the compression bit for
    //  the directory spec and get out.
    //
    lstrcpy(szGlobalDir, DirectorySpec);
    DisplayUncompressProgress(PROGRESS_UPD_DIRECTORY);

    if (lstrlen(FileSpec) == 0)
    {
DoUncompressRetryCreate:

        if (!OpenFileForCompress(&FileHandle, DirectorySpec))
        {
            goto DoUncompressError;
        }

DoUncompressRetryDevIo:

        State = 0;
        if (!DeviceIoControl( FileHandle,
                              FSCTL_SET_COMPRESSION,
                              &State,
                              sizeof(USHORT),
                              NULL,
                              0,
                              &Length,
                              FALSE ))
        {
DoUncompressError:

            if (!bIgnoreAllErrors)
            {
                MBRet = CompressErrMessageBox( hDlg,
                                               DirectorySpec,
                                               &FileHandle );
                if (MBRet == WF_RETRY_CREATE)
                {
                    goto DoUncompressRetryCreate;
                }
                else if (MBRet == WF_RETRY_DEVIO)
                {
                    goto DoUncompressRetryDevIo;
                }
                else if (MBRet == IDABORT)
                {
                    //
                    //  Return error.
                    //  File handle closed by CompressErrMessageBox.
                    //
                    return (FALSE);
                }
                //
                //  Else (MBRet == IDIGNORE)
                //  Continue on as if the error did not occur.
                //
            }
        }
        if (INVALID_HANDLE_VALUE != FileHandle)
        {
            CloseHandle(FileHandle);
            FileHandle = INVALID_HANDLE_VALUE;
        }

        TotalDirectoryCount++;
        TotalFileCount++;

        DisplayUncompressProgress(PROGRESS_UPD_DIRCNT);
        DisplayUncompressProgress(PROGRESS_UPD_FILECNT);

        ChangeFileSystem(FSC_ATTRIBUTES, DirectorySpec, NULL);

        return (TRUE);
    }

    //
    //  Get a pointer to the end of the directory spec, so that we can
    //  keep appending names to the end of it.
    //
    DirectorySpecEnd = DirectorySpec + lstrlen(DirectorySpec);

    TotalDirectoryCount++;
    DisplayUncompressProgress(PROGRESS_UPD_DIRCNT);

    //
    //  For every file in the directory that matches the file spec,
    //  open the file and uncompress it.
    //
    //  Setup the template for findfirst/findnext.
    //
    lstrcpy(DirectorySpecEnd, FileSpec);

    if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            //
            //  Make sure the user hasn't hit cancel.
            //
            if (bShowProgress && !hDlgProgress)
            {
                break;
            }

            //
            //  Skip over the . and .. entries.
            //
            if ( !lstrcmp(FindData.cFileName, SZ_DOT) ||
                 !lstrcmp(FindData.cFileName, SZ_DOTDOT) )
            {
                continue;
            }
            else
            {
                //
                //  Append the found file to the directory spec and
                //  open the file.
                //
                lstrcpy(DirectorySpecEnd, FindData.cFileName);

                if (!(GetFileAttributes(DirectorySpec) & ATTR_COMPRESSED))
                {
                    //
                    //  Already uncompressed, so get the next file.
                    //
                    continue;
                }

UncompressFileRetryCreate:

                if (!OpenFileForCompress(&FileHandle, DirectorySpec))
                {
                    goto UncompressFileError;
                }

UncompressFileRetryDevIo:

                //
                //  Uncompress the file.
                //
                if (!UncompressFile(FileHandle, &FindData))
                {
UncompressFileError:

                    if (!bIgnoreAllErrors)
                    {
                        MBRet = CompressErrMessageBox( hDlg,
                                                       DirectorySpec,
                                                       &FileHandle );
                        if (MBRet == WF_RETRY_CREATE)
                        {
                            goto UncompressFileRetryCreate;
                        }
                        else if (MBRet == WF_RETRY_DEVIO)
                        {
                            goto UncompressFileRetryDevIo;
                        }
                        else if (MBRet == IDABORT)
                        {
                            //
                            //  Return error.
                            //  File handle closed by CompressErrMessageBox.
                            //
                            FindClose(FindHandle);
                            return (FALSE);
                        }
                        //
                        //  Else (MBRet == IDIGNORE)
                        //  Continue on as if the error did not occur.
                        //
                    }
                }
                if (INVALID_HANDLE_VALUE != FileHandle)
                {
                    //
                    // Close the file and get the next file.
                    //
                    CloseHandle(FileHandle);
                    FileHandle = INVALID_HANDLE_VALUE;
                }
            }

        } while (FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);

        ChangeFileSystem(FSC_ATTRIBUTES, DirectorySpec, NULL);
    }

    //
    //  If we are to do subdirectores, then look for every subdirectory
    //  and recursively call ourselves to list the subdirectory.
    //
    if (DoSubdirectories && hDlgProgress)
    {
        //
        //  Setup findfirst/findnext to search the entire directory.
        //
        lstrcpy(DirectorySpecEnd, SZ_STAR);

        if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //
                //  Skip over the . and .. entries, otherwise recurse.
                //
                if ( !lstrcmp(FindData.cFileName, SZ_DOT) ||
                     !lstrcmp(FindData.cFileName, SZ_DOTDOT) )
                {
                    continue;
                }
                else
                {
                    //
                    //  If the entry is for a directory, then tack
                    //  on the subdirectory name to the directory spec
                    //  and recurse.
                    //
                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        lstrcpy(DirectorySpecEnd, FindData.cFileName);
                        lstrcat(DirectorySpecEnd, SZ_BACKSLASH);

                        if (!WFDoUncompress(hDlg, DirectorySpec, FileSpec))
                        {
                            FindClose(FindHandle);
                            return (FALSE);
                        }
                    }
                }

            } while (FindNextFile(FindHandle, &FindData));

            FindClose(FindHandle);
        }
    }

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  GetRootPath
//
//  Gets the root directory path name.  Take into account "Quoted" paths
//  because the first char is not the drive letter.
//
/////////////////////////////////////////////////////////////////////////////

BOOL GetRootPath(
    LPTSTR szPath,
    LPTSTR szReturn)
{
    if (!QualifyPath(szPath))
    {
        return (FALSE);
    }
    else
    {
        szReturn[0] = TEXT('\0');
    }

    //
    //  Return the correct drive letter by taking into account
    //  "quoted" pathname strings.
    //

    szReturn[0] = (szPath[0] == CHAR_DQUOTE) ? szPath[1] : szPath[0];
    szReturn[1] = TEXT(':');
    szReturn[2] = TEXT('\\');
    szReturn[3] = TEXT('\0');

    return (TRUE);
}


/////////////////////////////////////////////////////////////////////////////
//
//  RedrawAllTreeWindows
//
//  Loops through all windows and invalidates each tree list box so that
//  it will be redrawn.
//
/////////////////////////////////////////////////////////////////////////////

extern VOID GetTreePath(PDNODE pNode, register LPTSTR szDest);

VOID RedrawAllTreeWindows()
{
    HWND hwnd, hwndTree, hwndLB;
    int cItems, ctr;
    PDNODE pNode;
    DWORD dwAttribs;
    TCHAR szPathName[MAXPATHLEN * 2];


    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
       if (hwndTree = HasTreeWindow(hwnd))
       {
           hwndLB = GetDlgItem(hwndTree, IDCW_TREELISTBOX);

           //
           //  Search through all of the pNodes and update the
           //  attributes.
           //
           cItems = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);
           for (ctr = 0; ctr < cItems; ctr++)
           {
               SendMessage(hwndLB, LB_GETTEXT, ctr, (LPARAM)&pNode);

               //
               //  Set the attributes for this directory.
               //
               GetTreePath(pNode, szPathName);
               if ((dwAttribs = GetFileAttributes(szPathName)) != INVALID_FILE_ATTRIBUTES)
               {
                   pNode->dwAttribs = dwAttribs;
               }
           }

           InvalidateRect(hwndLB, NULL, FALSE);
       }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//  CompressErrMessageBox
//
//  Puts up the error message box when a file cannot be compressed or
//  uncompressed.  It also returns the user preference.
//
//  NOTE: The file handle is closed if the abort or ignore option is
//        chosen by the user.
//
/////////////////////////////////////////////////////////////////////////////

int CompressErrMessageBox(
    HWND hwndActive,
    LPTSTR szFile,
    PHANDLE phFile)
{
    int rc;


    //
    //  Put up the error message box - ABORT, RETRY, IGNORE, IGNORE ALL.
    //
    rc = DialogBoxParam( hAppInstance,
                         (LPTSTR) MAKEINTRESOURCE(COMPRESSERRDLG),
                         hwndFrame,
                         (DLGPROC)CompressErrDialogProc,
                         (LPARAM)szFile );

    //
    //  Return the user preference.
    //
    if (rc == IDRETRY)
    {
        if (*phFile == INVALID_HANDLE_VALUE)
        {
            return (WF_RETRY_CREATE);
        }
        else
        {
            return (WF_RETRY_DEVIO);
        }
    }
    else
    {
        //
        //  IDABORT or IDIGNORE
        //
        //  Close the file handle and return the message box result.
        //
        if (*phFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(*phFile);
            *phFile = INVALID_HANDLE_VALUE;
        }

        return (rc);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//  CompressErrDialogProc
//
//  Puts up a dialog to allow the user to Abort, Retry, Ignore, or
//  Ignore All when an error occurs during compression.
//
/////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK CompressErrDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    WORD Id = TRUE;

    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            //
            //  Set the dialog message text.
            //
            LoadString( hAppInstance,
                        IDS_COMPRESS_ATTRIB_ERR,
                        szTitle,
                        COUNTOF(szTitle) );
            wsprintf(szMessage, szTitle, (LPTSTR)lParam);
            SetDlgItemText(hDlg, IDD_TEXT1, szMessage);
            EnableWindow (hDlg, TRUE);

            break;
        }
        case ( WM_COMMAND ) :
        {
            Id = GET_WM_COMMAND_ID(wParam, lParam);
            switch (Id)
            {
                case ( IDD_IGNOREALL ) :
                {
                    bIgnoreAllErrors = TRUE;

                    //  fall through...
                }
                case ( IDABORT ) :
                case ( IDRETRY ) :
                case ( IDIGNORE ) :
                {
                    EndDialog(hDlg, Id);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }
    return (Id);
}


/////////////////////////////////////////////////////////////////////////////
//
//  OpenFileForCompress
//
//  Opens the file for compression.  It handles the case where a READONLY
//  file is trying to be compressed or uncompressed.  Since read only files
//  cannot be opened for WRITE_DATA, it temporarily resets the file to NOT
//  be READONLY in order to open the file, and then sets it back once the
//  file has been compressed.
//
/////////////////////////////////////////////////////////////////////////////

BOOL OpenFileForCompress(
    PHANDLE phFile,
    LPTSTR szFile)
{
    HANDLE hAttr;
    BY_HANDLE_FILE_INFORMATION fi;


    //
    //  Try to open the file - READ_DATA | WRITE_DATA.
    //
    if ((*phFile = CreateFile( szFile,
                               FILE_READ_DATA | FILE_WRITE_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL )) != INVALID_HANDLE_VALUE)
    {
        //
        //  Successfully opened the file.
        //
        return (TRUE);
    }

    if (GetLastError() != ERROR_ACCESS_DENIED)
    {
        return (FALSE);
    }

    //
    //  Try to open the file - READ_ATTRIBUTES | WRITE_ATTRIBUTES.
    //
    if ((hAttr = CreateFile( szFile,
                             FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL )) == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    //
    //  See if the READONLY attribute is set.
    //
    if ( (!GetFileInformationByHandle(hAttr, &fi)) ||
         (!(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) )
    {
        //
        //  If the file could not be open for some reason other than that
        //  the readonly attribute was set, then fail.
        //
        CloseHandle(hAttr);
        return (FALSE);
    }

    //
    //  Turn OFF the READONLY attribute.
    //
    fi.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(hAttr);
        return (FALSE);
    }

    //
    //  Try again to open the file - READ_DATA | WRITE_DATA.
    //
    *phFile = CreateFile( szFile,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL );

    //
    //  Close the file handle opened for READ_ATTRIBUTE | WRITE_ATTRIBUTE.
    //
    CloseHandle(hAttr);

    //
    //  Make sure the open succeeded.  If it still couldn't be opened with
    //  the readonly attribute turned off, then fail.
    //
    if (*phFile == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    //
    //  Turn the READONLY attribute back ON.
    //
    fi.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(*phFile);
        *phFile = INVALID_HANDLE_VALUE;
        return (FALSE);
    }

    //
    //  Return success.  A valid file handle is in *phFile.
    //
    return (TRUE);
}

