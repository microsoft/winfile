/********************************************************************

   wfprint.c

   Windows Print Routines

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PrintFile() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD
PrintFile(HWND hwnd, LPTSTR szFile)
{
   DWORD          ret;
   INT           iCurCount;
   INT           i;
   HCURSOR       hCursor;
   WCHAR      szDir[MAXPATHLEN];

   ret = 0;

   hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
   iCurCount = ShowCursor(TRUE) - 1;

   //
   // This returns Ansi chars, and that's what ShellExecute expects
   //
   GetSelectedDirectory(0, szDir);

   //
   // open the object
   //
   SetErrorMode(0);
   ret = (DWORD)ShellExecute(hwnd, L"print", szFile, szNULL, szDir, SW_SHOWNORMAL);
   SetErrorMode(1);

   switch (ret) {
   case 0:
   case 8:
      ret = IDS_PRINTMEMORY;
      break;

   case 2:
      ret = IDS_FILENOTFOUNDMSG;
      break;

   case 3:
      ret = IDS_BADPATHMSG;
      break;

   case 5:
      ret = IDS_NOACCESSFILE;
      break;

   case SE_ERR_SHARE:
      ret = IDS_SHAREERROR;
      break;

   case SE_ERR_ASSOCINCOMPLETE:
      ret = IDS_ASSOCINCOMPLETE;
      break;

   case SE_ERR_DDETIMEOUT:
   case SE_ERR_DDEFAIL:
   case SE_ERR_DDEBUSY:
      ret = IDS_DDEFAIL;
      break;

   case SE_ERR_NOASSOC:
      ret = IDS_NOASSOCMSG;
      break;

   default:
      if (ret <= 32)
         goto EPExit;
      ret = 0;
   }

EPExit:
   i = ShowCursor(FALSE);

   //
   // Make sure that the cursor count is still balanced.
   //
   if (i != iCurCount)
      ShowCursor(TRUE);

   SetCursor(hCursor);

   return(ret);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WFPrint() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD
WFPrint(LPTSTR pSel)
{
  TCHAR szFile[MAXPATHLEN];
  TCHAR szTemp[MAXPATHLEN];
  DWORD ret;

  /* Turn off the print button. */
  if (hdlgProgress)
      EnableWindow(GetDlgItem(hdlgProgress, IDOK), FALSE);

  if (!(pSel = GetNextFile(pSel, szFile, COUNTOF(szFile))))
      return TRUE;

  /* See if there is more than one file to print.  Abort if so
   */
  if (pSel = GetNextFile(pSel, szTemp, COUNTOF(szTemp))) {
      MyMessageBox(hwndFrame, IDS_WINFILE, IDS_PRINTONLYONE, MB_OK | MB_ICONEXCLAMATION);
      return(FALSE);
  }

  if (hdlgProgress) {
      /* Display the name of the file being printed. */
      LoadString(hAppInstance, IDS_PRINTINGMSG, szTitle, COUNTOF(szTitle));
      wsprintf(szMessage, szTitle, szFile);
      SetDlgItemText(hdlgProgress, IDD_STATUS, szMessage);
  }

  ret = PrintFile(hdlgProgress ? hdlgProgress : hwndFrame, szFile);

  if (ret) {
     MyMessageBox(hwndFrame, IDS_PRINTERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION);
     return FALSE;
  }

  return TRUE;
}
