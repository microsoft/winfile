/********************************************************************

   wfdlgs3.c

   Windows File System Dialog procedures

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"
#include <shlobj.h>

#define LABEL_NTFS_MAX 32
#define LABEL_FAT_MAX  11
#define CCH_VERSION    40
#define CCH_DRIVE       3
#define CCH_DLG_TITLE  16

VOID FormatDrive( IN PVOID ThreadParameter );
VOID CopyDiskette( IN PVOID ThreadParameter );
VOID SwitchToSafeDrive(VOID);
VOID MDIClientSizeChange(HWND hwndActive, INT iFlags);

VOID CopyDiskEnd(VOID);
VOID FormatEnd(VOID);
VOID CancelDlgQuit(VOID);
VOID LockFormatDisk(INT iDrive1, INT iDrive2, DWORD dwMessage, DWORD dwCommand, BOOL bLock);

BOOL GetProductVersion(WORD * pwMajor, WORD * pwMinor, WORD * pwBuild, WORD * pwRevision);

DWORD ulTotalSpace, ulSpaceAvail;

typedef enum {
    FDC_FALSE,
    FDC_FALSE_Q,
    FDC_TRUE
} FDC_RET;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ChooseDriveDlgProc() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
ChooseDriveDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   TCHAR szDrive[5];

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {
   case WM_INITDIALOG:
      {
         INT   i;
         HWND  hwndLB;
         lstrcpy(szDrive, SZ_ACOLON);

         hwndLB = GetDlgItem(hDlg, IDD_DRIVE);

         switch (dwSuperDlgMode) {
         case IDM_DISKCOPY:
            {
               HWND hwndLB2;

               hwndLB2 = GetDlgItem(hDlg, IDD_DRIVE1);

               for (i = 0; i < cDrives; i++) {
                  if (IsRemovableDrive(rgiDrive[i])) {
                     DRIVESET(szDrive,rgiDrive[i]);
                     SendMessage(hwndLB, CB_ADDSTRING, 0, (LPARAM)szDrive);
                     SendMessage(hwndLB2, CB_ADDSTRING, 0, (LPARAM)szDrive);
                  }
               }

               SendMessage(hwndLB, CB_SETCURSEL, 0, 0L);
               SendMessage(hwndLB2, CB_SETCURSEL, 0, 0L);
            }
            break;
         }
         break;
      }

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
      case IDD_HELP:
         goto DoHelp;

      case IDOK:
         {
            TCHAR szTemp[128];

            if (dwSuperDlgMode == IDM_DISKCOPY) {

               if (bConfirmFormat) {
                  LoadString(hAppInstance, IDS_DISKCOPYCONFIRMTITLE, szTitle, COUNTOF(szTitle));
                  LoadString(hAppInstance, IDS_DISKCOPYCONFIRM, szMessage, COUNTOF(szMessage));
                  if (MessageBox(hDlg, szMessage, szTitle, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES)
                     break;
               }

               GetDlgItemText( hDlg, IDD_DRIVE1, szTemp, COUNTOF(szTemp) -1 );
               CancelInfo.Info.Copy.iSourceDrive = DRIVEID(szTemp);

               GetDlgItemText( hDlg, IDD_DRIVE, szTemp, COUNTOF(szTemp) -1 );
               CancelInfo.Info.Copy.iDestDrive = DRIVEID(szTemp);

               //
               // lock drives
               //
               LockFormatDisk(CancelInfo.Info.Copy.iSourceDrive,
                  CancelInfo.Info.Copy.iDestDrive,IDS_DRIVEBUSY_COPY,
                  IDM_FORMAT, TRUE);

               EndDialog(hDlg, TRUE);

               CreateDialog(hAppInstance, (LPTSTR) MAKEINTRESOURCE(CANCELDLG), hwndFrame, (DLGPROC) CancelDlgProc);
            } else {
               EndDialog(hDlg, TRUE);
            }
            break;
         }

      case IDCANCEL:
         EndDialog(hDlg, FALSE);
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


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DiskLabelDlgProc() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
DiskLabelDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   TCHAR szNewVol[MAXPATHLEN];
   LPTSTR lpszVol;
   TCHAR szDrive[] = SZ_ACOLON;
   DRIVE drive;
   INT i;

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {
   case WM_INITDIALOG:

      //
      // Get the current volume label
      //
      drive = GetSelectedDrive();

      DRIVESET(szDrive,drive);

      if (!IsTheDiskReallyThere(hDlg, szDrive, FUNC_LABEL, FALSE)) {
         EndDialog(hDlg, FALSE);
         break;
      }

      GetVolumeLabel(drive, &lpszVol, FALSE);

      //
      // Display the current volume label.
      //
      SetDlgItemText(hDlg, IDD_NAME, lpszVol);

      //
      // Set the limit.
      // LATER: figure a more generic way of doing this.
      //

      i = IsNTFSDrive(drive) ?
         LABEL_NTFS_MAX :
         LABEL_FAT_MAX;

      SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, i, 0L);
      break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
      case IDD_HELP:
         goto DoHelp;

      case IDCANCEL:
         EndDialog(hDlg, FALSE);
         break;

      case IDOK:
         {
            HWND hwnd;
            DWORD dwError;

            DRIVE drive;
            DRIVEIND driveInd;

            GetDlgItemText(hDlg, IDD_NAME, szNewVol, COUNTOF(szNewVol));

            if (!ChangeVolumeLabel(drive=GetSelectedDrive(), szNewVol)) {

               dwError = GetLastError();
               if ( ERROR_ACCESS_DENIED == dwError ) {
                  LoadString(hAppInstance, IDS_LABELACCESSDENIED, szMessage, COUNTOF(szMessage));
               } else {
                  LoadString(hAppInstance, IDS_LABELDISKERR, szMessage, COUNTOF(szMessage));
               }

               GetWindowText(hDlg, szTitle, COUNTOF(szTitle));
               MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
               EndDialog(hDlg, FALSE);
               break;
            }

            //
            // invalidate VolInfo[drive] to refresh off disk
            //
            I_VolInfo(drive);

            //
            // Scan through to fine Ind.
            //
            for ( driveInd=0; driveInd < cDrives; driveInd++ ) {
               if ( rgiDrive[driveInd] == drive ) {
                  SelectToolbarDrive( driveInd );
                  break;
               }
            }

            for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
               hwnd;
               hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

               // refresh windows on this drive

               if (GetSelectedDrive() == (INT)GetWindowLongPtr(hwnd, GWL_TYPE))
                  SendMessage(hwnd, FS_CHANGEDRIVES, 0, 0L);

            }
            EndDialog(hDlg, TRUE);
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


VOID
FormatDiskette(HWND hwnd, BOOL bModal)
{
    INT res = 0;
    DWORD dwSave;

    // in case current drive is on floppy

    SwitchToSafeDrive();

    dwSave = dwContext;
    dwContext = IDH_FORMAT;

    CancelInfo.bModal = bModal;

    res = DialogBox(hAppInstance, (LPTSTR) MAKEINTRESOURCE(FORMATDLG), hwnd, (DLGPROC) FormatDlgProc);

    dwContext = dwSave;
}


FDC_RET
FillDriveCapacity(HWND hDlg, INT nDrive, FMIFS_MEDIA_TYPE fmSelect, BOOL fDoPopup)
{
#if defined(DBCS)
   FMIFS_MEDIA_TYPE fmMedia[FmMediaEndOfData];  // Number of types in enumeration
#else
   FMIFS_MEDIA_TYPE fmMedia[12];  // Number of types in enumeration
#endif
   WCHAR wchDrive[4] = L"A:";
   DWORD MediaCount;
   INT index;
   UINT uiCount;
   TCHAR szTemp[32];

   INT iCurSel = 0;

   SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_RESETCONTENT, 0, 0L);

   wchDrive[0] += nDrive;

#if defined(DBCS)
   if (!(*lpfnQuerySupportedMedia)((LPWSTR)wchDrive, fmMedia, FmMediaEndOfData, (PDWORD)&MediaCount))
#else
   if (!(*lpfnQuerySupportedMedia)((LPWSTR)wchDrive, fmMedia, 12L, (PDWORD)&MediaCount))
#endif
   {
      return FDC_FALSE;
   }

   if (MediaCount == 1 && fmMedia[0] == FmMediaRemovable) {
       TCHAR szTmpStr[256];

       /*
        * We can't format this type of drive, tell the user to run WinDisk.Exe
        */
       if (fDoPopup) {
           LoadString(hAppInstance, IDS_CANTFORMAT, szTmpStr, COUNTOF(szTmpStr));
           wsprintf(szMessage, szTmpStr, wchDrive);

           LoadString(hAppInstance, IDS_CANTFORMATTITLE, szTmpStr,
                COUNTOF(szTmpStr));

           MessageBox(hDlg, szMessage, szTmpStr, MB_ICONEXCLAMATION | MB_OK);
       }
       return FDC_FALSE_Q;
   }

   for (index = 0, uiCount = 0; uiCount < MediaCount; uiCount++) {

      // If our selection == the media count, select it now!

      if (fmSelect == fmMedia[uiCount])
         iCurSel = index;

      switch(fmMedia[uiCount]) {
      case FmMediaUnknown:
      case FmMediaFixed:
      case FmMediaRemovable:       // Removable media other than floppy
         break;

      case FmMediaF5_1Pt2_512:     // 5.25", 1.2MB,  512 bytes/sector
         LoadString(hAppInstance, IDS_12MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF5_1Pt2_512,0));
         break;

      case FmMediaF5_360_512:      // 5.25", 360KB,  512 bytes/sector
#if defined(JAPAN) && defined(i386)
         if (! ISNECPC98(gdwMachineId)) {
#endif
         LoadString(hAppInstance, IDS_360KB, szTitle, COUNTOF(szTitle));
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF5_360_512,0));
#if defined(JAPAN) && defined(i386)
         }
#endif
         break;

      case FmMediaF5_320_512:      // 5.25", 320KB,  512 bytes/sector
      case FmMediaF5_320_1024:     // 5.25", 320KB,  1024 bytes/sector
      case FmMediaF5_180_512:      // 5.25", 180KB,  512 bytes/sector
      case FmMediaF5_160_512:      // 5.25", 160KB,  512 bytes/sector
         break;

      case FmMediaF3_2Pt88_512:    // 3.5",  2.88MB, 512 bytes/sector
         LoadString(hAppInstance, IDS_288MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_2Pt88_512,0));
         break;

      case FmMediaF3_1Pt44_512:    // 3.5",  1.44MB, 512 bytes/sector
         LoadString(hAppInstance, IDS_144MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_1Pt44_512,0));
         break;

      case FmMediaF3_720_512:      // 3.5",  720KB,  512 bytes/sector
         LoadString(hAppInstance, IDS_720KB, szTitle, COUNTOF(szTitle));
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_720_512,0));
         break;

      case FmMediaF3_20Pt8_512:
         LoadString(hAppInstance, IDS_2080MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_20Pt8_512,0));
         break;

#if defined(JAPAN) && defined(i386)
      //
      // FMR jul.21.1994 JY
      // add 5.25" 1.23MB media type
      //
      case FmMediaF5_1Pt23_1024:    // 5.25",  1.23MB, 1024 bytes/sector
         if (ISNECPC98(gdwMachineId))
             LoadString(hAppInstance, IDS_125MB, szTemp, COUNTOF(szTemp));
         else
             LoadString(hAppInstance, IDS_123MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF5_1Pt23_1024,0));
         break;

      //
      // add 3.5" 1.23MB media type
      //
      case FmMediaF3_1Pt23_1024:    // 3.5",  1.23MB, 1024 bytes/sector
         if (ISNECPC98(gdwMachineId))
             LoadString(hAppInstance, IDS_125MB, szTemp, COUNTOF(szTemp));
         else
             LoadString(hAppInstance, IDS_123MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_1Pt23_1024,0));
         break;

      //
      // add 3.5" 1.2MB media type
      //
      case FmMediaF3_1Pt2_512:     // 3.5", 1.2MB,  512 bytes/sector
         LoadString(hAppInstance, IDS_12MB, szTemp, COUNTOF(szTemp));
         wsprintf(szTitle, szTemp, szDecimal);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_1Pt2_512,0));
         break;

      //
      // add 5.25" 640KB media type
      //
      case FmMediaF5_640_512:      // 5.25", 640KB,  512 bytes/sector
         LoadString(hAppInstance, IDS_640KB, szTitle, COUNTOF(szTitle));
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF5_640_512,0));
         break;

      //
      // add 3.5" 640KB media type
      //
      case FmMediaF3_640_512:      // 3.5", 640KB,  512 bytes/sector
         LoadString(hAppInstance, IDS_640KB, szTitle, COUNTOF(szTitle));
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_640_512,0));
         break;

      //
      // FMR jul.21.1994 JY
      // add 5.25" 720KB media type
      //
      case FmMediaF5_720_512:      // 5.25",  720KB,  512 bytes/sector
         LoadString(hAppInstance, IDS_720KB, szTitle, COUNTOF(szTitle));
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF5_720_512,0));
         break;

/* ADD KBNES. NEC MEDIATYPE START */
      case FmMediaF8_256_128:      // 8"1s , 256KB,  128 bytes/sector
         break;

      case FmMediaF3_128Mb_512:    // 3.5" , 128MB,  512 bytes/sector  3.5"MO
         LoadString(hAppInstance, IDS_128MB, szTitle, COUNTOF(szTitle));
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_INSERTSTRING, index, (LPARAM)szTitle);
         SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETITEMDATA, index++, MAKELONG(FmMediaF3_128Mb_512,0));
         break;
/* ADD KBNES. NEC MEDIATYPE END */
#endif

      default:
         break;
      }
   }

   SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_SETCURSEL, iCurSel, 0L);

   return(FDC_TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FormatDlgProc() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
FormatDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   TCHAR szBuf[128];
   INT  i, count;
   static INT nLastDriveInd;
   static INT nItemIndex;

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {
   case WM_INITDIALOG:

      // make sure fmifs is loaded!

      if (!FmifsLoaded())
         EndDialog(hDlg,FALSE);

      CancelInfo.bCancel = FALSE;
      CancelInfo.dReason = IDS_FFERR;
      CancelInfo.fmifsSuccess = FALSE;
      CancelInfo.nPercentDrawn = 0;

      ulTotalSpace = ulSpaceAvail = 0L;

      // fill drives combo

      nLastDriveInd = -1;
      if (CancelInfo.Info.Format.fFlags & FF_PRELOAD)
      {
         // We are preloading, so go ahead and set quickformat and label
         // AND also density if it's available! (below)

         CheckDlgButton(hDlg, IDD_VERIFY,CancelInfo.Info.Format.fQuick);
         SetDlgItemText(hDlg, IDD_NAME, CancelInfo.Info.Format.szLabel);

         // attempt to read in new information
         // Set density combobox items as last time...
         if (FillDriveCapacity(hDlg,
             CancelInfo.Info.Format.iFormatDrive,
             CancelInfo.Info.Format.fmMediaType, TRUE) == FDC_TRUE)
         {
            /*
             *  Get the index of the drive selection to highlight.
             *  Only count removable drives.
             */
            for (i = 0; i < cDrives; i++)
            {
               if (IsRemovableDrive(rgiDrive[i]))
               {
                  nLastDriveInd++;
                  if (rgiDrive[i] == CancelInfo.Info.Format.iFormatDrive)
                     break;
               }
            }
         }
      }

      // Now that we've used the bool, clear it out.
      CancelInfo.Info.Format.fFlags &= ~FF_PRELOAD;

      LoadString(hAppInstance, IDS_DRIVETEMP, szBuf, COUNTOF(szBuf));

      count = 0;
      nItemIndex = -1;
      for (i = 0; i < cDrives; i++)
      {
         if (IsRemovableDrive(rgiDrive[i]))
         {
            nItemIndex++;

            wsprintf(szMessage, szBuf, (TCHAR)(CHAR_A+rgiDrive[i]), CHAR_SPACE);

            SendDlgItemMessage(hDlg, IDD_DRIVE, CB_INSERTSTRING, count, (LPARAM)szMessage);
            SendDlgItemMessage(hDlg, IDD_DRIVE, CB_SETITEMDATA, count++, MAKELONG(rgiDrive[i], 0));

            if ( (nLastDriveInd == -1) &&
                 (FillDriveCapacity( hDlg,
                                     rgiDrive[i],
                                     (DWORD) -1,
                                     FALSE ) == FDC_TRUE) )
            {
               /*
                *  Must keep track of the item index separately in case there
                *  is a non-removable drive between 2 removable drives.
                *  For example, if a:, b:, and d: are removable and c: is not.
                */
               nLastDriveInd = nItemIndex;
            }
         }
      }

      if (nLastDriveInd != -1)
      {
         /*
          *  Highlight the appropriate selection.
          */
         SendDlgItemMessage( hDlg,
                             IDD_DRIVE,
                             CB_SETCURSEL,
                             nLastDriveInd,
                             0L);
      }
      else
      {
         MyMessageBox(hwndFrame, IDS_WINFILE, IDS_QSUPMEDIA, MB_OK | MB_ICONEXCLAMATION);
         EndDialog(hDlg, FALSE);
      }

      SendDlgItemMessage(hDlg,
                         IDD_NAME,
                         EM_LIMITTEXT,
                         COUNTOF(CancelInfo.Info.Format.szLabel) - 2,
                         0L);

      break;

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {

      case IDD_HELP:
         goto DoHelp;

      case IDD_DRIVE:
         switch (GET_WM_COMMAND_CMD(wParam, lParam)) {

         case CBN_SELCHANGE: {
            FDC_RET fdcr;
            i = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETCURSEL, 0, 0L);
            i = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETITEMDATA, i, 0L);
            //fFormatFlags &= ~FF_CAPMASK;

            // If can't get info, switch back to old.

            if ((fdcr = FillDriveCapacity(hDlg, i, (DWORD) -1, TRUE)) !=
                   FDC_TRUE) {
               if (fdcr != FDC_FALSE_Q) {
                   // Popup message box
                   MyMessageBox(hwndFrame, IDS_WINFILE, IDS_QSUPMEDIA,
                        MB_OK | MB_ICONEXCLAMATION);
               }

               SendDlgItemMessage(hDlg, IDD_DRIVE, CB_SETCURSEL,
                    nLastDriveInd, 0L);

               FillDriveCapacity(hDlg, rgiDrive[nLastDriveInd], (DWORD) -1,
                    FALSE);
            }
            break;
         }

         default:
            break;
         }
         break;

      case IDCANCEL:

         EndDialog(hDlg, FALSE);
         break;

      case IDOK:

         CancelInfo.eCancelType = CANCEL_FORMAT;

         i = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETCURSEL, 0, 0L);
         CancelInfo.Info.Format.iFormatDrive = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, CB_GETITEMDATA, i, 0L);

         i = (INT)SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_GETCURSEL, 0, 0L);
         CancelInfo.Info.Format.fmMediaType = SendDlgItemMessage(hDlg, IDD_HIGHCAP, CB_GETITEMDATA, i, 0L);

         CancelInfo.Info.Format.fQuick = IsDlgButtonChecked(hDlg, IDD_VERIFY);

         GetDlgItemText(hDlg, IDD_NAME, CancelInfo.Info.Format.szLabel,
            COUNTOF(CancelInfo.Info.Format.szLabel));

         if (bConfirmFormat) {
            LoadString(hAppInstance, IDS_FORMATCONFIRMTITLE, szTitle, COUNTOF(szTitle));
            LoadString(hAppInstance, IDS_FORMATCONFIRM, szBuf, COUNTOF(szBuf));
            wsprintf(szMessage, szBuf, (TCHAR)(CHAR_A+CancelInfo.Info.Format.iFormatDrive));

            if (MessageBox(hDlg, szMessage, szTitle, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES)
               break;
         }

         //
         // Lock our drive and gray out formatdisk
         //
         LockFormatDisk(CancelInfo.Info.Format.iFormatDrive, -1, IDS_DRIVEBUSY_FORMAT, IDM_DISKCOPY, TRUE);

         EndDialog(hDlg,TRUE);

         // If we are formatting a disk because we tried to switch to a
         // different drive (e.g., mouse drag copy to unformatted disk)
         // use a modal box.

         if (CancelInfo.bModal) {
            DialogBox(hAppInstance, (LPTSTR) MAKEINTRESOURCE(CANCELDLG), hwndFrame, (DLGPROC) CancelDlgProc);
         } else {
            CreateDialog(hAppInstance, (LPTSTR) MAKEINTRESOURCE(CANCELDLG), hwndFrame, (DLGPROC) CancelDlgProc);
         }

         break;

      default:

         return FALSE;
      }

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

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  FormatSelectDlgProc() -  DialogProc callback function for FORMATSELECTDLG */
/*                                                                            */
/*----------------------------------------------------------------------------*/

INT_PTR
FormatSelectDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    HWND  hwndSelectDrive;
    INT   driveIndex;
    INT   comboxIndex;
    DRIVE drive;
    DWORD dwFormatResult;
    TCHAR szDrive[CCH_DRIVE] = { 0 };
    TCHAR szDlgTitle[CCH_DLG_TITLE] = { 0 };

    switch (wMsg)
    {
    case WM_INITDIALOG:
        {
            // Build the list of drives that can be selected for formatting.
            // Do not include remote drives or CD/DVD drives.
            szDrive[1] = ':';
            hwndSelectDrive = GetDlgItem(hDlg, IDD_SELECTDRIVE);
            if (hwndSelectDrive)
            {
                for (driveIndex = 0; driveIndex < cDrives; driveIndex++)
                {
                    drive = rgiDrive[driveIndex];
                    if (!IsRemoteDrive(drive) && !IsCDRomDrive(drive))
                    {
                        // Set the drive letter as the string and the drive index as the data.
                        DRIVESET(szDrive, drive);
                        comboxIndex = SendMessage(hwndSelectDrive, CB_ADDSTRING, 0, (LPARAM)szDrive);
                        SendMessage(hwndSelectDrive, CB_SETITEMDATA, comboxIndex, (LPARAM)drive);
                    }
                }

                SendMessage(hwndSelectDrive, CB_SETCURSEL, 0, 0);
            }

            return TRUE;
        }
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            {
                // Hide this dialog window while the SHFormatDrive dialog is displayed.
                // SHFormatDrive needs a parent window, and this dialog will serve as
                // that parent, even if it is hidden.
                ShowWindow(hDlg, SW_HIDE);

                // Retrieve the selected drive index and call SHFormatDrive with it.
                comboxIndex = (INT)SendDlgItemMessage(hDlg, IDD_SELECTDRIVE, CB_GETCURSEL, 0, 0);
                drive = (DRIVE)SendDlgItemMessage(hDlg, IDD_SELECTDRIVE, CB_GETITEMDATA, comboxIndex, 0);
                dwFormatResult = SHFormatDrive(hDlg, drive, SHFMT_ID_DEFAULT, 0);

                // If the format results in an error, show FORMATSELECTDLG again so
                // the user can select a different drive if needed, or cancel.
                // Otherwise, if the format was successful, just close FORMATSELECTDLG.
                if (dwFormatResult == SHFMT_ERROR || dwFormatResult == SHFMT_CANCEL || dwFormatResult == SHFMT_NOFORMAT)
                {
                    // SHFormatDrive sometimes sets the parent window title when it encounters an error.
                    // We don't want this; set the title back before we show the dialog.
                    LoadString(hAppInstance, IDS_FORMATSELECTDLGTITLE, szDlgTitle, CCH_DLG_TITLE);
                    SetWindowText(hDlg, szDlgTitle);
                    ShowWindow(hDlg, SW_SHOW);
                }
                else
                {
                    DestroyWindow(hDlg);
                    hwndFormatSelect = NULL;
                }

                return TRUE;
            }
        case IDCANCEL:
            DestroyWindow(hDlg);
            hwndFormatSelect = NULL;
            return TRUE;
        }
    }
    return FALSE;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AboutDlgProc() -  DialogProc callback function for ABOUTDLG             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
AboutDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    WORD wMajorVersion   = 0;
    WORD wMinorVersion   = 0;
    WORD wBuildNumber    = 0;
    WORD wRevisionNumber = 0;
    TCHAR szVersion[CCH_VERSION] = { 0 };

    switch (wMsg)
    {
    case WM_INITDIALOG:
        if (GetProductVersion(&wMajorVersion, &wMinorVersion, &wBuildNumber, &wRevisionNumber))
        {
            if (SUCCEEDED(StringCchPrintf(szVersion, CCH_VERSION, TEXT("Version %d.%d.%d.%d"),
                (int)wMajorVersion, (int)wMinorVersion, (int)wBuildNumber, (int)wRevisionNumber)))
            {
                SetDlgItemText(hDlg, IDD_VERTEXT, szVersion);
            }
        }
        return TRUE;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}

VOID
FormatDrive( IN PVOID ThreadParameter )
{
   WCHAR wszDrive[3];
   WCHAR wszFileSystem[4] = L"FAT";

   wszDrive[0] = (WCHAR)(CancelInfo.Info.Format.iFormatDrive + CHAR_A);
   wszDrive[1] = CHAR_COLON;
   wszDrive[2] = CHAR_NULL;

#define wszLabel CancelInfo.Info.Format.szLabel

   do {
      CancelInfo.Info.Format.fFlags &= ~FF_RETRY;

      (*lpfnFormat)(wszDrive,
         CancelInfo.Info.Format.fmMediaType,
         wszFileSystem,
         wszLabel,
         (BOOLEAN)(CancelInfo.Info.Format.fQuick ? TRUE : FALSE),
         (FMIFS_CALLBACK)&Callback_Function);
   } while (CancelInfo.Info.Format.fFlags & FF_RETRY);

   CancelDlgQuit();
}

VOID
CopyDiskette( IN PVOID ThreadParameter )
{
  BOOL fVerify = FALSE;

  WCHAR wszSrcDrive[3];
  WCHAR wszDestDrive[3];

  wszSrcDrive[0] = (WCHAR)(CancelInfo.Info.Copy.iSourceDrive + CHAR_A);
  wszSrcDrive[1] = CHAR_COLON;
  wszSrcDrive[2] = CHAR_NULL;

  wszDestDrive[0] = (WCHAR)(CancelInfo.Info.Copy.iDestDrive + CHAR_A);
  wszDestDrive[1] = CHAR_COLON;
  wszDestDrive[2] = CHAR_NULL;

  (*lpfnDiskCopy)(wszSrcDrive,
                  wszDestDrive,
                  (BOOLEAN)fVerify,
                  (FMIFS_CALLBACK)&Callback_Function);

   CancelDlgQuit();
}


BOOL
Callback_Function(FMIFS_PACKET_TYPE   PacketType,
   DWORD PacketLength,
   PVOID PacketData)
{
   // Quit if told to do so..

   if (CancelInfo.bCancel)
      return FALSE;

   switch (PacketType) {
   case FmIfsPercentCompleted:

      //
      // If we are copying and we just finished a destination format,
      // then set the window text back to the original message
      //
      if (CANCEL_COPY == CancelInfo.eCancelType &&
         CancelInfo.Info.Copy.bFormatDest) {

         CancelInfo.Info.Copy.bFormatDest = FALSE;
         SendMessage(hwndFrame, FS_CANCELCOPYFORMATDEST, 0, 0L);
      }

      PostMessage(hwndFrame, FS_CANCELUPDATE, ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted, 0L);

      break;
   case FmIfsFormatReport:
      ulTotalSpace = ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesTotalDiskSpace * 1024L;
      ulSpaceAvail = ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesAvailable * 1024L;
      break;
   case FmIfsInsertDisk:
      switch(((PFMIFS_INSERT_DISK_INFORMATION)PacketData)->DiskType) {
      case DISK_TYPE_GENERIC:
         CancelInfo.fuStyle = MB_OK | MB_ICONINFORMATION;
         SendMessage(hwndFrame, FS_CANCELMESSAGEBOX, IDS_COPYDISK, IDS_INSERTSRC);
         break;
      case DISK_TYPE_SOURCE:
         CancelInfo.fuStyle = MB_OK | MB_ICONINFORMATION;
         SendMessage(hwndFrame, FS_CANCELMESSAGEBOX, IDS_COPYDISK, IDS_INSERTSRC);
         break;
      case DISK_TYPE_TARGET:
         CancelInfo.fuStyle = MB_OK | MB_ICONINFORMATION;
         SendMessage(hwndFrame, FS_CANCELMESSAGEBOX, IDS_COPYDISK, IDS_INSERTDEST);
         break;
      case DISK_TYPE_SOURCE_AND_TARGET:
         CancelInfo.fuStyle = MB_OK | MB_ICONINFORMATION;
         SendMessage(hwndFrame, FS_CANCELMESSAGEBOX, IDS_COPYDISK, IDS_INSERTSRCDEST);
         break;
      }
      break;
   case FmIfsIncompatibleFileSystem:
      CancelInfo.dReason = IDS_FFERR_INCFS;
      break;
   case FmIfsFormattingDestination:
      CancelInfo.Info.Copy.bFormatDest = TRUE;
      SendMessage(hwndFrame, FS_CANCELCOPYFORMATDEST, 0, 0L);
      break;
   case FmIfsIncompatibleMedia:
      CancelInfo.fuStyle = MB_ICONHAND | MB_OK;
      SendMessage(hwndFrame, FS_CANCELMESSAGEBOX, IDS_COPYDISK, IDS_COPYSRCDESTINCOMPAT);
      break;
   case FmIfsAccessDenied:
      CancelInfo.dReason = IDS_FFERR_ACCESSDENIED;
      break;
   case FmIfsMediaWriteProtected:
      CancelInfo.dReason = IDS_FFERR_DISKWP;
      break;
   case FmIfsCantLock:
      CancelInfo.dReason = IDS_FFERR_CANTLOCK;
      break;
   case FmIfsBadLabel:
      CancelInfo.fuStyle = MB_ICONEXCLAMATION | MB_OK;
      SendMessage(hwndFrame, FS_CANCELMESSAGEBOX,
         IDS_COPYERROR + FUNC_LABEL, IDS_FFERR_BADLABEL);
      break;
   case FmIfsCantQuickFormat:

      // Can't quick format, ask if user wants to regular format:
      CancelInfo.fuStyle = MB_ICONEXCLAMATION | MB_YESNO;

      if (IDYES == SendMessage(hwndFrame, FS_CANCELMESSAGEBOX, IDS_FORMATERR,
         IDS_FORMATQUICKFAILURE)) {

         CancelInfo.Info.Format.fQuick = FALSE;
         CancelInfo.Info.Format.fFlags |= FF_RETRY;

      } else {

         //
         // Just fake a cancel
         //
         CancelInfo.fmifsSuccess = FALSE;
         CancelInfo.bCancel = TRUE;
      }

      break;
   case FmIfsIoError:
      switch(((PFMIFS_IO_ERROR_INFORMATION)PacketData)->DiskType) {
      case DISK_TYPE_GENERIC:
         CancelInfo.dReason = IDS_FFERR_GENIOERR;
         break;
      case DISK_TYPE_SOURCE:
         CancelInfo.dReason = IDS_FFERR_SRCIOERR;
         break;
      case DISK_TYPE_TARGET:
         CancelInfo.dReason = IDS_FFERR_DSTIOERR;
         break;
      case DISK_TYPE_SOURCE_AND_TARGET:
         CancelInfo.dReason = IDS_FFERR_SRCDSTIOERR;
         break;
      }
      break;
   case FmIfsFinished:
      CancelInfo.fmifsSuccess = ((PFMIFS_FINISHED_INFORMATION)PacketData)->Success;

      break;
   default:
      break;
   }
   return TRUE;
}

/*-------------------------  CancelDlgProc
 *
 *  DESCRIPTION:
 *    dialog procedure for the modeless dialog. two main purposes
 *    here:
 *
 *      1. if the user chooses CANCEL we set bCancel to TRUE
 *         which will end the PeekMessage background processing loop
 *
 *      2. handle the private FS_CANCELUPDATE message and draw
 *         a "gas gauge" indication of how FAR the background job
 *         has progressed
 *
 *  ARGUMENTS:
 *      stock dialog proc arguments
 *
 *  RETURN VALUE:
 *      stock dialog proc return value - BOOL
 *
 *  GLOBALS READ:
 *      none
 *
 *  GLOBALS WRITTEN:
 *      CancelInfo structure
 *
 *  MESSAGES:
 *      WM_COMMAND      - handle IDCANCEL by setting bCancel to TRUE
 *                        and calling DestroyWindow to end the dialog
 *
 *      WM_INITDIALOG   - set control text, get coordinates of gas gauge,
 *                        disable main window so we look modal
 *
 *      WM_PAINT        - draw the "gas gauge" control
 *
 *      FS_CANCELUPDATE - the percentage done has changed, so update
 *                        nPercentDrawn and force a repaint
 *
 *  NOTES:
 *
 *    The bCancel global variable is used to communicate
 *    with the main window. If the user chooses to cancel
 *    we set bCancel to TRUE.
 *
 *    When we get the private message FS_CANCELUPDATE
 *    we update the "gas gauge" control that indicates
 *    what percentage of the rectangles have been drawn
 *    so FAR. This shows that we can draw in the dialog
 *    as the looping operation progresses.  (FS_CANCELUPDATE is sent
 *    first to hwndFrame, which sets %completed then sends message to us.)
 *
 */

BOOL
CancelDlgProc(HWND hDlg,
   UINT message,
   WPARAM wParam,
   LPARAM lParam)
{
   static RECT rectGG;              // GasGauge rectangle
   DWORD Ignore;
   TCHAR szTemp[128];
   static BOOL bLastQuick;

   switch (message) {

   case WM_COMMAND:

      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
      case IDCANCEL:

         DestroyCancelWindow();
         CancelInfo.bCancel = TRUE;
         return TRUE;

      case IDD_HIDE:
         DestroyCancelWindow();
         return TRUE;
      }
      return TRUE;

   case WM_INITDIALOG:
      {
         CancelInfo.hCancelDlg = hDlg;
         bLastQuick = TRUE;

         switch(CancelInfo.eCancelType) {
         case CANCEL_FORMAT:

            //
            // Formatting disk requires that we release any notification
            // requests on this drive.
            //
            NotifyPause(CancelInfo.Info.Format.iFormatDrive, DRIVE_REMOVABLE);

            break;
         case CANCEL_COPY:

            //
            // Pause notifications on dest drive.
            //
            NotifyPause(CancelInfo.Info.Copy.iDestDrive, DRIVE_REMOVABLE);

            if (CancelInfo.Info.Copy.bFormatDest) {
               LoadString(hAppInstance, IDS_FORMATTINGDEST, szTemp, COUNTOF(szTemp));
            } else {
               LoadString(hAppInstance, IDS_COPYINGDISKTITLE, szTemp, COUNTOF(szTemp));
            }
            SetWindowText(hDlg, szTemp);

            break;
         default:
            break;
         }

         if (!CancelInfo.hThread) {
            switch (CancelInfo.eCancelType) {
            case CANCEL_FORMAT:
               CancelInfo.hThread = CreateThread( NULL,      // Security
                  0L,                                        // Stack Size
                  (LPTHREAD_START_ROUTINE)FormatDrive,
                  NULL,
                  0L,
                  &Ignore );
               break;
            case CANCEL_COPY:
              CancelInfo.hThread = CreateThread( NULL,      // Security
                  0L,                                        // Stack Size
                  (LPTHREAD_START_ROUTINE)CopyDiskette,
                  NULL,
                  0L,
                  &Ignore );
               break;
            default:
               break;
            }
         }

         // Get the coordinates of the gas gauge static control rectangle,
         // and convert them to dialog client area coordinates
         GetClientRect(GetDlgItem(hDlg, IDD_GASGAUGE), &rectGG);
         ClientToScreen(GetDlgItem(hDlg, IDD_GASGAUGE), (LPPOINT)&rectGG.left);
         ClientToScreen(GetDlgItem(hDlg, IDD_GASGAUGE), (LPPOINT)&rectGG.right);
         ScreenToClient(hDlg, (LPPOINT)&rectGG.left);
         ScreenToClient(hDlg, (LPPOINT)&rectGG.right);

         return TRUE;
      }

   case WM_PAINT:
       {
       HDC         hDC;
       PAINTSTRUCT ps;
       TCHAR       buffer[32];
       SIZE        size;
       INT         xText, yText;
       INT         nDivideRects;
       RECT        rectDone, rectLeftToDo;

       // The gas gauge is drawn by drawing a text string stating
       // what percentage of the job is done into the middle of
       // the gas gauge rectangle, and by separating that rectangle
       // into two parts: rectDone (the left part, filled in blue)
       // and rectLeftToDo(the right part, filled in white).
       // nDivideRects is the x coordinate that divides these two rects.
       //
       // The text in the blue rectangle is drawn white, and vice versa
       // This is easy to do with ExtTextOut()!

       hDC = BeginPaint(hDlg, &ps);

       //
       // If formatting quick, set this display
       //
       if (CancelInfo.Info.Format.fQuick &&
          CANCEL_FORMAT == CancelInfo.eCancelType) {

          LoadString(hAppInstance, IDS_QUICKFORMATTINGTITLE, buffer, COUNTOF(buffer));
          SendDlgItemMessage(hDlg, IDD_TEXT1, WM_SETTEXT, 0, (LPARAM)szNULL);

          bLastQuick = TRUE;

       } else {

          if (bLastQuick) {
             LoadString(hAppInstance, IDS_PERCENTCOMPLETE, buffer, COUNTOF(buffer));
             SendDlgItemMessage(hDlg, IDD_TEXT1, WM_SETTEXT, 0, (LPARAM)buffer);

             bLastQuick = FALSE;
          }

          wsprintf(buffer, SZ_PERCENTFORMAT, CancelInfo.nPercentDrawn);
       }

       GetTextExtentPoint32(hDC, buffer, lstrlen(buffer), &size);
       xText    = rectGG.left
                   + ((rectGG.right - rectGG.left) - size.cx) / 2;
       yText    = rectGG.top
                   + ((rectGG.bottom - rectGG.top) - size.cy) / 2;

       nDivideRects = ((rectGG.right - rectGG.left) * CancelInfo.nPercentDrawn) / 100;

       // Paint in the "done so FAR" rectangle of the gas
       // gauge with blue background and white text
       SetRect(&rectDone, rectGG.left, rectGG.top,
                                 rectGG.left + nDivideRects, rectGG.bottom);
       SetTextColor(hDC, RGB(255, 255, 255));
       SetBkColor(hDC, RGB(0, 0, 255));

       ExtTextOut(hDC, xText, yText, ETO_CLIPPED | ETO_OPAQUE,
                           &rectDone, buffer, lstrlen(buffer), NULL);

       // Paint in the "still left to do" rectangle of the gas
       // gauge with white background and blue text
       SetRect(&rectLeftToDo, rectGG.left+nDivideRects, rectGG.top,
                                 rectGG.right, rectGG.bottom);
       SetTextColor(hDC, RGB(0, 0, 255));
       SetBkColor(hDC, RGB(255, 255, 255));

       ExtTextOut(hDC, xText, yText, ETO_CLIPPED | ETO_OPAQUE,
                       &rectLeftToDo, buffer, lstrlen(buffer), NULL);

       EndPaint(hDlg, &ps);

       return TRUE;
       }

   case FS_CANCELUPDATE:

       InvalidateRect(hDlg, &rectGG, TRUE);
       UpdateWindow(hDlg);
       return TRUE;

   default:
       return FALSE;
   }
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ProgressDialogProc
//
// Synopsis: Modal dialog box for mouse move/copy progress
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

INT_PTR
ProgressDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   static PCOPYINFO pCopyInfo;
   TCHAR szTitle[MAXTITLELEN];

   switch (wMsg) {
   case WM_INITDIALOG:

      hdlgProgress = hDlg;
      pCopyInfo = (PCOPYINFO) lParam;

      // Set the destination directory in the dialog.
      // use IDD_TONAME 'cause IDD_TO gets disabled....

      // The dialog title defaults to "Moving..."
      if (pCopyInfo->dwFunc == FUNC_COPY) {

         if (bJAPAN) {
            // Use "Copying..." instead of "Moving..."
            SetDlgItemText(hdlgProgress, IDD_TOSTATUS, szNULL);
         }
         LoadString(hAppInstance,
                    IDS_COPYINGTITLE,
                    szTitle,
                    COUNTOF(szTitle));

         SetWindowText(hdlgProgress, szTitle);

      } else {

         SetDlgItemText(hdlgProgress, IDD_TOSTATUS, szNULL);
      }


      //
      // Move/Copy things.
      //

      if (WFMoveCopyDriver(pCopyInfo)) {

         //
         // Error message!!
         //

         EndDialog(hDlg, GetLastError());
      }
      break;

   case FS_COPYDONE:

      //
      // Only cancel out if pCopyInfo == lParam
      // This indicates that the proper thread quit.
      //
      // wParam holds return value
      //

      if (lParam == (LPARAM)pCopyInfo) {

         EndDialog(hDlg, wParam);
      }
      break;


   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {

      case IDCANCEL:

         pCopyInfo->bUserAbort = TRUE;

         //
         // What should be the return value??
         //
         EndDialog(hDlg, 0);

         break;

      default:
         return FALSE;
      }
      break;

   default:
      return FALSE;
   }
   return TRUE;
}


// update all the windows and things after drives have been connected
// or disconnected.

VOID
UpdateConnections(BOOL bUpdateDriveList)
{
   HWND hwnd, hwndNext, hwndDrive, hwndTree;
   INT i;
   DRIVE drive;
   HCURSOR hCursor;
   LPTSTR lpszVol;
   LPTSTR lpszOldVol;

   hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
   ShowCursor(TRUE);

   if (bUpdateDriveList) {
      UpdateDriveList();
   }

   // close all windows that have the current drive set to
   // the one we just disconnected

   for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = hwndNext) {

      hwndNext = GetWindow(hwnd, GW_HWNDNEXT);

      // ignore the titles and search window
      if (GetWindow(hwnd, GW_OWNER) || hwnd == hwndSearch)
         continue;

      drive = GetWindowLongPtr(hwnd, GWL_TYPE);

      //
      // IsValidDisk uses GetDriveType which was updated if
      // bUpdateDriveList == TRUE.
      //

      if (IsValidDisk(drive)) {

         //
         // Invalidate cache to get real one in case the user reconnected
         // d: from \\popcorn\public to \\rastaman\ntwin
         //
         // Previously used MDI window title to determine if the volume
         // has changed.  Now we will just check DriveInfo structure
         // (bypass status bits).
         //

         //
         // Now only do this for remote drives!
         //

         if (IsRemoteDrive(drive)) {

            R_NetCon(drive);

            if (!WFGetConnection(drive, &lpszVol, FALSE, ALTNAME_REG)) {
               lpszOldVol = (LPTSTR) GetWindowLongPtr(hwnd, GWL_VOLNAME);

               if (lpszOldVol && lpszVol) {

                  if (lstrcmpi(lpszVol, lpszOldVol)) {

                     //
                     // updatedrivelist/initdrivebitmaps called above;
                     // don't do here
                     //
                     RefreshWindow(hwnd, FALSE, TRUE);
                  }
               }
            }
         }

      } else {

         //
         // this drive has gone away
         //
         if (IsLastWindow()) {
            // disconnecting the last drive
            // set this guy to the first non floppy / cd rom

            for (i = 0; i < cDrives; i++) {
               if (!IsRemovableDrive(rgiDrive[i]) && !IsCDRomDrive(rgiDrive[i])) {
                  SendMessage(hwndDriveBar, FS_SETDRIVE, i, 0L);
                  break;
               }
            }
         } else if ((hwndTree = HasTreeWindow(hwnd)) &&
            GetWindowLongPtr(hwndTree, GWL_READLEVEL)) {

            //
            // abort tree walk
            //
            bCancelTree = TRUE;

         } else {
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
         }
      }
   }

   // why is this here?  Move it further, right redisplay if at all.
   // Reuse hwndDrive as the current window open!

   hwndDrive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

   i = (INT) GetWindowLongPtr(hwndDrive, GWL_TYPE);

   if (TYPE_SEARCH == i) {
      i = DRIVEID(SearchInfo.szSearch);
   }

   FillToolbarDrives(i);

   //
   // don't refresh, done in FillToolbarDrives
   //
   SwitchDriveSelection(hwndDrive,FALSE);

   //
   // Don't refresh toolbar... it doesn't change!
   //
   MDIClientSizeChange(NULL,DRIVEBAR_FLAG); /* update/resize drive bar */

   ShowCursor(FALSE);
   SetCursor(hCursor);

   //
   // Update disco btn/menu item
   //
   EnableDisconnectButton();

}

INT_PTR
DrivesDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   DRIVEIND driveInd;
   INT iSel;
   HWND hwndActive;

   UNREFERENCED_PARAMETER(lParam);

   switch (wMsg) {
   case WM_INITDIALOG:
      {
         INT nCurDrive;
         DRIVEIND nIndex;
         LPTSTR lpszVolShare;

         nCurDrive = GetSelectedDrive();
         nIndex = 0;

         for (driveInd=0; driveInd < cDrives; driveInd++) {

            BuildDriveLine(&lpszVolShare, driveInd, FALSE, ALTNAME_SHORT);

            if (nCurDrive == rgiDrive[driveInd])
               nIndex = driveInd;

            SendDlgItemMessage(hDlg, IDD_DRIVE, LB_ADDSTRING, 0, (LPARAM)lpszVolShare);
         }
         SendDlgItemMessage(hDlg, IDD_DRIVE, LB_SETCURSEL, nIndex, 0L);
         break;
      }

   case WM_COMMAND:

      switch (GET_WM_COMMAND_ID(wParam, lParam)) {
      case IDD_HELP:
         goto DoHelp;

      case IDD_DRIVE:
         if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK)
            break;

         // fall through
      case IDOK:
         iSel = (INT)SendDlgItemMessage(hDlg, IDD_DRIVE, LB_GETCURSEL, 0, 0L);
         EndDialog(hDlg, TRUE);

         hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

         if (hwndDriveBar) {

            //
            // If same drive, don't steal (lparam)
            //
            SendMessage(hwndDriveBar, FS_SETDRIVE, iSel, 1L);
         }
         break;

      case IDCANCEL:
         EndDialog(hDlg, FALSE);
         break;

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





/////////////////////////////////////////////////////////////////////
//
// Name:     CancelDlgQuit
//
// Synopsis: Quits the cancel modeless dialog (status for diskcopy/format)
//
// IN: VOID
//
// Return:   VOID
//
// Assumes:  Called from worker thread only; CancelInfo.hThread valid
//
// Effects:  Kills calling thread
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
CancelDlgQuit()
{
   //
   // Close thread if successful
   //

   if (CancelInfo.hThread) {
      CloseHandle(CancelInfo.hThread);
      CancelInfo.hThread = NULL;
   } else {
      //
      // BUGBUG
      //
   }

   //
   // At this point, when we call FS_CANCELEND,
   // the other thread thinks that this one has died since
   // CancelInfo.hThread is NULL.
   // This is exactly what we want, since we will very shortly
   // exit after the SendMessage.
   //
   SendMessage(hwndFrame, FS_CANCELEND,0,0L);

   ExitThread(0L);
}

VOID
CopyDiskEnd()
{

   //
   // Now resume notifications
   //
   NotifyResume(CancelInfo.Info.Copy.iDestDrive, DRIVE_REMOVABLE);

   LockFormatDisk(CancelInfo.Info.Copy.iSourceDrive,
      CancelInfo.Info.Copy.iDestDrive, 0, IDM_FORMAT, FALSE);

   // If not successful, and the user _didn't_ hit cancel, throw up
   // an error message (as implemented, hitting cancel does not return
   // a dialog box.

   if (!CancelInfo.fmifsSuccess && !CancelInfo.bCancel) {
      //
      // Get reason and display intelligent message.
      LoadString(hAppInstance, IDS_COPYDISKERR, szTitle, COUNTOF(szTitle));
      LoadString(hAppInstance, CancelInfo.dReason, szMessage, COUNTOF(szMessage));

      MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);
   }
}


VOID
FormatEnd()
{
   TCHAR szBuf[128];
   HWND hwnd, hwndActive;

   //
   // Now resume notifications
   //
   NotifyResume(CancelInfo.Info.Format.iFormatDrive, DRIVE_REMOVABLE);

   LockFormatDisk(CancelInfo.Info.Format.iFormatDrive, -1, 0, IDM_DISKCOPY, FALSE);

   // Update information only if successful
   // (What about if cancel?)

   if (CancelInfo.fmifsSuccess) {

      //
      // refresh all windows open on this drive
      //
      // Originally, we updated everything, but now just invalidate
      //

      I_VolInfo(CancelInfo.Info.Format.iFormatDrive);

      for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
         hwnd;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

         // refresh windows on this drive

         if (CancelInfo.Info.Format.iFormatDrive == (INT)GetWindowLongPtr(hwnd, GWL_TYPE))
            RefreshWindow(hwnd, FALSE, FALSE);

      }
      hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
      // update the drives windows

      // FS_CHANGEDRIVES (via RedoDriveWindows)
      // no longer calls UpdateDriveList or InitDriveBitmaps.
      // But that's good, since neither changes in a format!

      SendMessage(hwndActive, FS_CHANGEDRIVES, 0, 0L);

      // use fFlags and check ONLYONE, then clear it.

      LoadString(hAppInstance, IDS_FORMATCOMPLETE, szTitle, COUNTOF(szTitle));
      LoadString(hAppInstance, IDS_FORMATANOTHER, szMessage, COUNTOF(szMessage));

      wsprintf(szBuf, szMessage, ulTotalSpace, ulSpaceAvail);

      if ( !(CancelInfo.Info.Format.fFlags & FF_ONLYONE)   &&
         IDYES == MessageBox(hwndFrame, szBuf, szTitle, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2)) {


         CancelInfo.Info.Format.fFlags |= FF_PRELOAD;
         PostMessage(hwndFrame,WM_COMMAND,GET_WM_COMMAND_MPS(IDM_FORMAT,0,0L));
      }

      // now clear ONLYONE!
      CancelInfo.Info.Format.fFlags &= ~FF_ONLYONE;

   } else if (!CancelInfo.bCancel) {

      // Display reason only if user didn't hit cancel.

      // Get reason and display intelligent message.
      LoadString(hAppInstance, IDS_FORMATERR, szTitle, COUNTOF(szTitle));

      LoadString(hAppInstance, CancelInfo.dReason, szMessage, COUNTOF(szMessage));
      MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);

   }
}


// Locks the format disk(s) and grays out format/copy disk
// IN: 2 drives to block (-1 = ignore)
//     IDM_ that's us.
// OUT: VOID
// PRECOND: none
// POSTCOND: drives will be grayed out on drivebar and menu items disabled.

VOID
LockFormatDisk(DRIVE drive1, DRIVE drive2,
   DWORD dwMessage, DWORD dwCommand, BOOL bLock)
{
   HMENU hMenu;

   // Gray out menu item dwCommand
   hMenu = GetMenu(hwndFrame);

   // Special case for IDM_FORMAT, as it no longer invokes FormatDiskette,
   // it can be safely left enabled even when copying diskettes.
   // This change is made here rather than removing the calls to
   // LockFormatDisk with IDM_FORMAT since LockFormatDisk also
   // changes the state of aDriveInfo.
   if (dwCommand != IDM_FORMAT)
   {
       EnableMenuItem(hMenu, dwCommand,
           bLock ? MF_BYCOMMAND | MF_GRAYED : MF_BYCOMMAND | MF_ENABLED);
   }

   //
   // Fix up aDriveInfo
   //
   if (-1 != drive1) aDriveInfo[drive1].iBusy = bLock ? dwMessage : 0;
   if (-1 != drive2) aDriveInfo[drive2].iBusy = bLock ? dwMessage : 0;
}


VOID
DestroyCancelWindow()
{
   if (!CancelInfo.hCancelDlg)
      return;

   if (CancelInfo.bModal) {
      EndDialog(CancelInfo.hCancelDlg,0);
   } else {
      DestroyWindow(CancelInfo.hCancelDlg);
   }
   CancelInfo.hCancelDlg = NULL;
}

//
// GetProductVersion
// Gets the product version values for the current module
//
// Parameters:
//   pwMajor    - [OUT] A pointer to the major version number
//   pwMinor    - [OUT] A pointer to the minor version number
//   pwBuild    - [OUT] A pointer to the build number
//   pwRevision - [OUT] A pointer to the revision number
//   
// Returns TRUE if successful
//
BOOL GetProductVersion(WORD * pwMajor, WORD * pwMinor, WORD * pwBuild, WORD * pwRevision)
{
    BOOL               success = FALSE;
    TCHAR              szCurrentModulePath[MAX_PATH];
    DWORD              cchPath;
    DWORD              cbVerInfo;
    LPVOID             pFileVerInfo;
    UINT               uLen;
    VS_FIXEDFILEINFO * pFixedFileInfo;

    cchPath = GetModuleFileName(NULL, szCurrentModulePath, MAX_PATH);

    if (cchPath && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        cbVerInfo = GetFileVersionInfoSize(szCurrentModulePath, NULL);

        if (cbVerInfo)
        {
            pFileVerInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbVerInfo);

            if (pFileVerInfo)
            {
                if (GetFileVersionInfo(szCurrentModulePath, 0, cbVerInfo, pFileVerInfo))
                {
                    // Get the pointer to the VS_FIXEDFILEINFO structure
                    if (VerQueryValue(pFileVerInfo, TEXT("\\"), (LPVOID *)&pFixedFileInfo, &uLen))
                    {
                        if (pFixedFileInfo && uLen)
                        {
                            *pwMajor    = HIWORD(pFixedFileInfo->dwProductVersionMS);
                            *pwMinor    = LOWORD(pFixedFileInfo->dwProductVersionMS);
                            *pwBuild    = HIWORD(pFixedFileInfo->dwProductVersionLS);
                            *pwRevision = LOWORD(pFixedFileInfo->dwProductVersionLS);

                            success = TRUE;
                        }
                    }
                }

                HeapFree(GetProcessHeap(), 0, pFileVerInfo);
            }
        }

    }

    return success;
}
