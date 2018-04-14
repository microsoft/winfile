/********************************************************************

   wfassoc.c

   Windows File Manager association code

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"
#include <commdlg.h>


#define DDETYPECOMBOBOXSIZ 20
#define DDETYPEMAX  (sizeof(aDDEType) / sizeof(aDDEType[0]))

#define STRINGSIZ  MAX_PATH   // at least >= {DESCSIZ, IDENTSIZ}
#define DESCSIZ    MAX_PATH
#define COMMANDSIZ MAX_PATH
#define DDESIZ     MAX_PATH

#define FILETYPEBLOCK MAX_PATH


#if 0
#define odf(x,y) {TCHAR szT[100]; wsprintf(szT,x,y); OutputDebugString(szT);}
#else
#undef OutputDebugString
#define OutputDebugString(x)
#define odf(x,y)
#endif

// structures

typedef struct _DDE_INFO {
   BOOL  bUsesDDE;
   TCHAR  szCommand[COMMANDSIZ];   // doesn't really belong here..
   TCHAR  szDDEMesg[DDESIZ];
   TCHAR  szDDEApp[DDESIZ];
   TCHAR  szDDENotRun[DDESIZ];
   TCHAR  szDDETopic[DDESIZ];
} DDEINFO, *PDDEINFO;

typedef struct _DDE_TYPE {
   UINT  uiComboBox;              // String in the combo box "action!"
   LPTSTR lpszRegistry;           // Registry entry under "shell"
} DDETYPE, *PDDETYPE;

// Allow the Action drop list to be extensible.
// To add new action types, just add new lines to this list.

DDETYPE aDDEType[] = {
   { IDS_ASSOC_OPEN,    TEXT("open") },
   { IDS_ASSOC_PRINT,   TEXT("print") }
};


typedef struct _FILETYPE *PFILETYPE;
typedef struct _EXT *PEXT;


//
// Normal state of lpszBuf:   "FMA000_FileType\0Nice name\0(blah.exe %1"
//                                              ^           ^       ^
//                                        uDesc/       uExe/   uExeSpace

typedef struct _FILETYPE {
   PFILETYPE next;
   UINT   uDesc;
   UINT   uExe;
   UINT   uExeSpace;
   UINT   cchBufSiz;
   LPTSTR lpszBuf;                      // Ident then desc then exe
   PEXT  pExt;
} FILETYPE;


typedef struct _EXT {
   PEXT next;
   PEXT pftNext;
   BOOL bAdd    : 1;
   BOOL bDelete : 1;
   PFILETYPE pFileType;
   PFILETYPE pftOrig;
   TCHAR szExt[EXTSIZ+1];   // +1 for dot
   TCHAR szIdent[1];        // Variable length; must be last
                            // This is ONLY USED BY ClassesRead
                            // Do NOT rely on this being stable;
} EXT;


// Must be defined AFTER aDDEType[]!

typedef struct _ASSOCIATE_FILE_DLG_INFO {
   BOOL       bRefresh  : 1;
   BOOL       bExtFocus : 1;
   BOOL       bReadOnly : 1;
   BOOL       bChange   : 1;
   BOOL       bOKEnable : 1;
   UINT       mode;                      // IDD_NEW, IDD_CONFIG, IDD_COMMAND
   PFILETYPE  pFileType;
   INT        iAction;
   HWND       hDlg;
   INT        iClassList;                // HACK: == LB_ERR for winini editing
   DDEINFO    DDEInfo[DDETYPEMAX];
   TCHAR      szExt[EXTSIZ+2];           // for dots (ALWAYS has prefix dot)
} ASSOCIATEFILEDLGINFO, *PASSOCIATEFILEDLGINFO;

typedef union _VFILETYPEEXT {
   PFILETYPE pFileType;
   PEXT pExt;
   VOID* vBoth;
} VFILETYPEEXT, *PVFILETYPEEXT;

// globals

TCHAR szShellOpenCommand[] = TEXT("\\shell\\open\\command");

TCHAR szShell[]   = TEXT("\\shell\\");
TCHAR szCommand[] = TEXT("\\command");
TCHAR szDDEExec[] = TEXT("\\ddeexec");
TCHAR szApp[]     = TEXT("\\application");
TCHAR szTopic[]   = TEXT("\\topic");
TCHAR szIFExec[]  = TEXT("\\ifexec");

TCHAR szDDEDefaultTopic[] = TEXT("System");

// These two strings must match exactly in length
TCHAR szFileManPrefix[] = TEXT("FMA000_");
TCHAR szFileManPrefixGen[] = TEXT("FMA%03x_");
#define MAX_PREFIX 0xfff

TCHAR szDotEXE[] = TEXT(".exe");
TCHAR szSpacePercentOne[] = TEXT(" %1");
TCHAR szNone[32];

PFILETYPE pFileTypeBase = NULL;
PEXT pExtBase = NULL;

// Prototypes

INT APIENTRY AssociateFileDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
BOOL AssociateDlgInit(HWND hDlg, LPTSTR lpszExt, INT iSel);
BOOL AssociateFileDlgExtAdd(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
BOOL AssociateFileDlgExtDelete(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
BOOL AssociateFileDlgCommand(HWND hDlg, WPARAM wParam, LPARAM lParam, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
DWORD AssociateFileWrite(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);

BOOL RegLoad(VOID);
VOID RegUnload(VOID);
BOOL ClassesRead(HKEY hKey, LPTSTR lpszSubKey, PFILETYPE* ppFileTypeBase, PEXT* ppExtBase);
VOID ClassesFree(BOOL bFileType);
VOID ActionUpdate(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
VOID ActionDlgRead(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
VOID UpdateOKEnable(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
VOID UpdateSelectionExt(HWND hDlg, BOOL bForce);

VOID  DDEUpdate(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, INT iAction);
DWORD DDERead(PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, INT i);
DWORD DDEWrite(PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, INT i);
VOID  DDEDlgRead(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, INT iAction);

INT ClassListFileTypeAdd(HWND hDlg, PFILETYPE pFileType);
DWORD CommandWrite(HWND hDlg, LPTSTR lpszExt, LPTSTR lpszCommand);
DWORD RegNodeDelete(HKEY hk, LPTSTR lpszKey);
DWORD RegExtAddHelper(HKEY hk, LPTSTR lpszExt, PFILETYPE pFileType);
VOID FileAssociateErrorCheck(HWND hwnd, UINT idsTitle, UINT idsText, DWORD dwError);

BOOL  FileTypeGrow(PFILETYPE pFileType, UINT uNewSize);
DWORD FileTypeRead(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo);
DWORD FileTypeWrite(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, HKEY hk, LPTSTR lpszKey);
DWORD FileTypeAddString(PFILETYPE pFileType, LPTSTR lpstr, PUINT pcchOffset);
BOOL  FileTypeDupIdentCheck(HWND hDlg, UINT uIDD_FOCUS, LPTSTR lpszIdent);
VOID  FileTypeFree(PFILETYPE pFileType);

DWORD RegExtAdd(HWND hDlg, HKEY hk, PEXT pExt, PFILETYPE pFileType);
DWORD RegExtDelete(HWND hDlg, HKEY hk, PEXT pExt);

VOID ExtClean(LPTSTR lpszExt);
BOOL ExtLinkToFileType(PEXT pExt, LPTSTR lpszIdent);
BOOL ExtDupCheck(LPTSTR lpszExt, PEXT pExt);
PEXT BaseExtFind(LPTSTR lpszExt);
VOID ExtLink(PEXT pExt, PFILETYPE pFileType);
VOID ExtDelink(PEXT pExt);
VOID ExtFree(PEXT pExt);

LPTSTR StrChrQuote(LPTSTR lpszString, TCHAR c);
LPTSTR GenerateFriendlyName(LPTSTR lpszCommand);

// endproto


// since LoadString() only reads up to a null we have to mark
// special characters where we want nulls then convert them
// after loading.

VOID
FixupNulls(LPTSTR p)
{
   LPTSTR pT;

   while (*p) {
      if (*p == CHAR_HASH) {
         pT = p;
         p = CharNext(p);
         *pT = CHAR_NULL;
      }
      else
         p = CharNext(p);
   }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     ValidateClass
//
// Synopsis: Updates the Delete/Config buttons and updates edit text
//
// IN:       hDlg   -- First dialog handle
//
// Return:   VOID
//
//
// Assumes:  pFileType set up properly:
//           This is done in ClassListFileTypeAdd
//
// Effects:  Delete/Config buttons
//           Sets the edit text above it to the executable name.
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
ValidateClass(HWND hDlg)
{
   INT i;
   PFILETYPE pFileType;

   //
   // If (none) is selected, we can't config or delete.
   //
   i = (INT)SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_GETCURSEL, 0, 0L);

   if (-1 == i) {
      SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_SETCURSEL, 0, 0L);
      i=0;
   }

   EnableWindow(GetDlgItem(hDlg, IDD_CONFIG), i);
   EnableWindow(GetDlgItem(hDlg, IDD_DELETE), i);

   if (i) {

      pFileType = (PFILETYPE) SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_GETITEMDATA, i, 0L);

      //
      // Put command string there
      //
      SendDlgItemMessage(hDlg, IDD_COMMAND, WM_SETTEXT, 0,
         (LPARAM) &pFileType->lpszBuf[pFileType->uDesc]);

   } else {

      SendDlgItemMessage(hDlg, IDD_COMMAND, WM_SETTEXT, 0, (LPARAM)szNone);
   }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     UpdateSelectionExt
//
// Synopsis:
//
// Given an extension (with or without a dot) set the list box selection
//
// IN        hDlg   --  Dlg to modify
// INC       bForce --  base selection on ext selection, not cb edit text
//
//
// Return:   VOID
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
UpdateSelectionExt(HWND hDlg, BOOL bForce)
{
   TCHAR szExt[EXTSIZ+1];
   PEXT pExt;
   INT i;
   TCHAR c, c2;
   PTCHAR p;
   PFILETYPE pFileType;

   TCHAR szTemp[MAX_PATH];

   //
   // bForce is used when we base the ext on GETCURSEL rather than
   // the text in the edit field.
   //
   if (bForce) {
      i = SendDlgItemMessage(hDlg, IDD_EXT, CB_GETCURSEL, 0, 0L);
      SendDlgItemMessage(hDlg, IDD_EXT, CB_GETLBTEXT, i, (LPARAM)szExt);
   } else {

      //
      // Get the current extension
      //
      GetDlgItemText(hDlg, IDD_EXT, szExt, COUNTOF(szExt));
   }

   //
   // Search for present association
   //
   pExt = BaseExtFind(szExt);

   //
   // Add only if not deleted and has associated filetype
   //
   if (pExt && pExt->pFileType && !pExt->bDelete) {

      pFileType=pExt->pFileType;

      //
      // Munge data structure for string... ugly
      //
      p = &pFileType->lpszBuf[pFileType->uExeSpace];

      c = *p;
      c2 = *(p+1);

      *p = CHAR_CLOSEPAREN;
      *(p+1) = CHAR_NULL;

      pFileType->lpszBuf[pFileType->uExe-2] = CHAR_SPACE;

      // Found one, set the selection!
      SendDlgItemMessage(hDlg,
         IDD_CLASSLIST,
         LB_SELECTSTRING,
         (WPARAM)-1,
         (LPARAM)&pExt->pFileType->lpszBuf[pExt->pFileType->uDesc]);

      pFileType->lpszBuf[pFileType->uExe-2] = CHAR_NULL;

      *p = c;
      *(p+1) = c2;

   } else {

      if (GetProfileString(szExtensions, szExt+1, szNULL, szTemp, COUNTOF(szTemp))) {

         //
         // Remove the "^." bulloney.
         //
         p = szTemp;
         while ((*p) && (CHAR_CARET != *p) && (CHAR_PERCENT != *p))
            p++;

         *p = CHAR_NULL;

         p--;

         if (CHAR_SPACE == *p)
            *p = CHAR_NULL;

         SetDlgItemText(hDlg, IDD_COMMAND, szTemp);

         //
         // Set clear the selection.
         SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_SETCURSEL, (WPARAM)-1, 0L);

         EnableWindow(GetDlgItem(hDlg, IDD_CONFIG), TRUE);
         EnableWindow(GetDlgItem(hDlg, IDD_DELETE), FALSE);

         return;
      }

      // Not found: only do selecting if not already at 0

      if (0 != SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_GETCURSEL, 0, 0L)) {
         SendDlgItemMessage(hDlg, IDD_CLASSLIST,LB_SETCURSEL, 0, 0L);
      }
   }

   // Now turn on/off class buttons (config/del)
   ValidateClass(hDlg);
}


//--------------------------------------------------------------------------
//
//  AssociateDlgProc() -
//
//  GWL_USERDATA = do we need to rebuild document string?
//--------------------------------------------------------------------------


INT_PTR
AssociateDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   TCHAR szTemp[STRINGSIZ];
   PFILETYPE pFileType, pft2;
   INT i;
   DWORD dwError;
   PEXT pExt, pExtNext;

   // Make sure szTemp is initialized.
   szTemp[0] = CHAR_NULL;

   switch (wMsg) {
   case WM_INITDIALOG:
      {
         LPTSTR p;
         register LPTSTR pSave;
         INT iItem;

         // Turn off refresh flag (GWL_USERDATA)
         SetWindowLongPtr(hDlg, GWLP_USERDATA, 0L);

         // Make 'p' point to the file's extension.
         pSave = GetSelection(1, NULL);
         if (pSave) {
            p = GetExtension(pSave);

            //
            // hack fix: check for " or > 4 chars then add \0.
            // Better fix: GetExtension should axe \" , but this
            // would be bad if the callee expects the \" .  So for
            // now just axe it here since we immediately delete it.
            //  Also, 4 is a magic number and should be #defined.
            //
            for (iItem=0;iItem<EXTSIZ-1 && p[iItem] && p[iItem]!=CHAR_DQUOTE ;iItem++)
               ;

            p[iItem] = CHAR_NULL;

            if (!IsProgramFile(pSave)) {
               lstrcpy(szTemp,p);
            } else {
               szTemp[0] = CHAR_NULL;
            }
            LocalFree((HLOCAL)pSave);
         }

         SendDlgItemMessage(hDlg, IDD_EXT, CB_LIMITTEXT, EXTSIZ-1, 0L);

         //
         // Save space for ".exe"" %1"
         //
         SendDlgItemMessage(hDlg, IDD_COMMAND, EM_LIMITTEXT,
            COMMANDSIZ-1-(COUNTOF(szDotEXE)-1)-(COUNTOF(szSpacePercentOne)-1),
            0L);

         if (!AssociateDlgInit(hDlg, szTemp, -1)) {
            EndDialog(hDlg, FALSE);
            return TRUE;
         }

         break;
      }

   case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam)) {

#if 0
      case IDD_COMMAND:

         //
         // The winball compatibility "sorta a file type definer" control
         //

         if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {

            //
            // Search for matching pFileType
            //

//          while();;xxxxx

         }
         break;
#endif
      case IDD_HELP:
         goto DoHelp;

      case IDD_EXT:
         {
            BOOL bForce = FALSE;

            if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_EDITCHANGE ||
               (bForce = (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE))) {

               UpdateSelectionExt(hDlg, bForce);
            }
            break;
         }

      case IDD_CLASSLIST:

         if (LBN_SELCHANGE == GET_WM_COMMAND_CMD(wParam, lParam)) {
            ValidateClass(hDlg);
            break;
         }

         // If not double click or (none) selected, break

         if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK ||
            !SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_GETCURSEL, 0, 0L)) {
            break;
         }

         // !! WARNING !!
         //
         // Non-portable usage of wParam BUGBUG (LATER)

         wParam = IDD_CONFIG;
         //
         // Fall though to config.
         //
      case IDD_NEW:
      case IDD_CONFIG:
         {
            DWORD dwSave = dwContext;

            // Allocate space for dialog info

            ASSOCIATEFILEDLGINFO AssociateFileDlgInfo;

            // Init stuff
            AssociateFileDlgInfo.bRefresh = FALSE;
            AssociateFileDlgInfo.bReadOnly = FALSE;
            //
            // No need to set AssociateFileDlgInfo.bChange = FALSE;
            // since this is done in WM_INITDIALOG.
            //
            AssociateFileDlgInfo.hDlg = hDlg;

            // copy extension
            GetDlgItemText(hDlg,
               IDD_EXT,
               AssociateFileDlgInfo.szExt,
               COUNTOF(AssociateFileDlgInfo.szExt));

            ExtClean(AssociateFileDlgInfo.szExt);

            AssociateFileDlgInfo.iClassList =
               (INT) SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_GETCURSEL,0,0L);

            if (IDD_NEW == GET_WM_COMMAND_ID(wParam, lParam)) {

               // Set the help context to us
               dwContext = IDH_DLG_ASSOCIATEFILEDLG;

               AssociateFileDlgInfo.mode = IDD_NEW;
DoConfigWinIni:

               AssociateFileDlgInfo.pFileType =
                  (PFILETYPE) LocalAlloc(LPTR, sizeof(FILETYPE));

               if (!AssociateFileDlgInfo.pFileType) {
                  FileAssociateErrorCheck(hDlg, IDS_EXTTITLE,
                     0L, GetLastError());

                  break;
               }

            } else {
               AssociateFileDlgInfo.mode = IDD_CONFIG;

               // Set the help context to us
               dwContext = IDH_DLG_ASSOCIATEFILEDLGCONFIG;

               //
               // If we are configing a winini ext, fake it
               // by doing a new
               //

               if (LB_ERR == AssociateFileDlgInfo.iClassList) {

                  AssociateFileDlgInfo.mode = IDD_COMMAND;
                  goto DoConfigWinIni;
               }

               AssociateFileDlgInfo.pFileType =
                  (PFILETYPE) SendDlgItemMessage(hDlg, IDD_CLASSLIST,
                     LB_GETITEMDATA, AssociateFileDlgInfo.iClassList, 0L);
            }

            DialogBoxParam(hAppInstance,
               (LPTSTR) MAKEINTRESOURCE(ASSOCIATEFILEDLG),
               hDlg, (DLGPROC) AssociateFileDlgProc,
               (LPARAM) &AssociateFileDlgInfo);

            ValidateClass(hDlg);

            // Restore help context
            dwContext = dwSave;

            // If prev dialog requests build doc refresh, set our flag

            if (AssociateFileDlgInfo.bRefresh)
               SetWindowLongPtr(hDlg, GWLP_USERDATA, 1L);

            //
            // Instead of clearing IDD_EXT, go ahead and leave
            // it.  Just be sure to validate IDD_CLASSLIST
            // based on ext.
            //
            UpdateSelectionExt(hDlg, FALSE);

            break;
         }
      case IDD_DELETE:

         //
         // Find which item to remove
         //
         i = SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_GETCURSEL, 0, 0L);

         //
         // Should never be 0...  If so, we're in trouble since
         // the delete key should have been disabled.
         //
         if (!i || LB_ERR == i)
            break;

         pFileType = (PFILETYPE) SendDlgItemMessage(hDlg, IDD_CLASSLIST,
            LB_GETITEMDATA, i , 0L);

         //
         // Verify with dialog
         //
         {
            TCHAR szText[MAXERRORLEN];
            TCHAR szTitle[MAXTITLELEN];
            TCHAR szTemp[MAXERRORLEN];

            LoadString(hAppInstance, IDS_FILETYPEDELCONFIRMTITLE, szTitle,
               COUNTOF(szTitle));

            LoadString(hAppInstance, IDS_FILETYPEDELCONFIRMTEXT, szTemp,
               COUNTOF(szTemp));

            wsprintf(szText,szTemp,&pFileType->lpszBuf[pFileType->uDesc]);

            if (IDYES != MessageBox(hDlg, szText, szTitle,
               MB_TASKMODAL|MB_YESNO|MB_ICONEXCLAMATION)) {

               break;
            }
         }

         //
         // Delete the actual file type....
         // If the filetype is deleted successfully but the user can't
         // delete an extension, we'll have a left over extension.
         // This could occur if the admin incorrectly sets up security
         // on HKEY_CLASSES_ROOT: filetype is deleteable, but ext is not.
         //
         // LATER: better error checking; delete filetype node only after
         // deleting all exts without error
         //
         dwError = RegNodeDelete(HKEY_CLASSES_ROOT, pFileType->lpszBuf);

         //
         // Flush it!
         //
         RegFlushKey(HKEY_CLASSES_ROOT);

         FileAssociateErrorCheck(hDlg, IDS_EXTTITLE,
            IDS_FILETYPEDELERROR, dwError);

         LoadString(hAppInstance, IDS_CLOSE, szTitle, COUNTOF(szTitle));
         SetDlgItemText(hDlg, IDCANCEL, szTitle);

         if (ERROR_SUCCESS != dwError)
            break;

         if (pFileType->pExt) {
            //
            // Only set refresh if we deleted one of them.
            //
            SetWindowLongPtr(hDlg, GWLP_USERDATA, 1L);
         }

         //
         // Now go through all .exts and delete them too.
         //
         for (pExt = pFileType->pExt; pExt; pExt=pExtNext) {

            pExtNext = pExt->pftNext;

            //
            // No longer associated with a filetype
            //
            RegExtDelete(hDlg, HKEY_CLASSES_ROOT, pExt);
         }

         //
         // Done deleting everything, now free pFileType after
         // delinking
         //
         if (pFileType == pFileTypeBase) {
            pFileTypeBase = pFileType->next;
         } else {
            for (pft2 = pFileTypeBase; pft2->next != pFileType; pft2=pft2->next)
               ;

            pft2->next = pFileType->next;
         }

         FileTypeFree(pFileType);

         //
         // Set text of IDD_EXT to szNULL so that the user doesn't
         // hit OK and change the extension
         // Alternatively, we could have a new button called "associate"
         //
         SetDlgItemText(hDlg, IDD_EXT, szNULL);

         SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_DELETESTRING, i, 0L);

         //
         // Set it to the next thing
         //
         dwError = SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_SETCURSEL, i, 0L);
         if (LB_ERR == dwError)
            i--;


         //
         // i Must always be > 0 (since deleting (none) is disallowed)
         //
         SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_SETCURSEL, i, 0L);
         ValidateClass(hDlg);

         break;

      case IDOK:
         {
            PEXT pExt;
            WCHAR szExt[EXTSIZ+1];
            WCHAR szCommand[COMMANDSIZ];

            GetDlgItemText(hDlg, IDD_EXT, szExt, COUNTOF(szExt));
            GetDlgItemText(hDlg, IDD_COMMAND, szCommand, COUNTOF(szCommand));

            //
            // Need to clean up szExt by adding .!
            //

            ExtClean(szExt);

            if (!szExt[1])
               goto Cancel;

            //
            // Now make sure it isn't a program extension
            //

            if (IsProgramFile(szExt)) {

               LoadString(hAppInstance, IDS_NOEXEASSOC, szCommand, COUNTOF(szCommand));

               wsprintf(szMessage, szCommand, szExt);
               GetWindowText(hDlg, szTitle, COUNTOF(szTitle));

               MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
               SetDlgItemText(hDlg, IDD_EXT, szNULL);

               break;
            }


            pExt = BaseExtFind(szExt);

            //
            // Check for delete case
            //
            // ALSO: if szCommand is szNULL ("")
            //
            if (!lstrcmpi(szCommand, szNone) || !szCommand[0]) {

               //
               // Axe it in winini
               //
               WriteProfileString(szExtensions, szExt+1, NULL);

               // If !pExt then it was never associated!
               if (pExt) {
                  FileAssociateErrorCheck(hDlg, IDS_EXTTITLE,
                     IDS_EXTDELERROR, RegExtDelete(hDlg, HKEY_CLASSES_ROOT,pExt));

               } else {
                  RegNodeDelete(HKEY_CLASSES_ROOT, szExt);
               }

               // Request refresh
               SetWindowLongPtr(hDlg, GWLP_USERDATA, 1L);

               goto Done;
            }

            //
            // There are two cases here:
            //
            //  1. Add to exe that the user typed in
            //  2. Add to preexisting selection
            //

            //
            // Strategy: Scan through all pFileTypes, using
            // lstrcmpi (case insensitive).  Don't clean up name
            // at all; leave heading/trailing spaces
            //

            for (pFileType=pFileTypeBase; pFileType; pFileType=pFileType->next) {

               if (!lstrcmpi(&pFileType->lpszBuf[pFileType->uDesc], szCommand))
                  break;
            }

            if (pFileType) {

               FileAssociateErrorCheck(hDlg,
                  IDS_EXTTITLE,
                  IDS_EXTADDERROR,
                  RegExtAddHelper(HKEY_CLASSES_ROOT, szExt, pFileType));

            } else {

               //
               // make sure it has an extension
               //
               if (*GetExtension(szCommand) == 0) {

                  lstrcat(szTemp, L".exe");

               } else {

                  if (!IsProgramFile(szCommand)) {

                     LoadString(hAppInstance,
                                IDS_ASSOCNOTEXE,
                                szTemp,
                                COUNTOF(szTemp));

                     wsprintf(szMessage, szTemp, szCommand);
                     GetWindowText(hDlg, szTitle, COUNTOF(szTitle));

                     MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                     SetDlgItemText(hDlg, IDD_COMMAND, szNULL);

                     break;
                  }
               }

               //
               // Create one.
               //
               if (DE_RETRY == CommandWrite(hDlg, szExt, szCommand))
                  break;
            }

            //
            // Request refresh
            //
            SetWindowLongPtr(hDlg, GWLP_USERDATA, 1L);

            // Flush it!
Done:
            RegFlushKey(HKEY_CLASSES_ROOT);

         }
         // FALL THROUGH

      case IDCANCEL:
Cancel:
         {
            HWND hwndNext, hwndT;

            //
            // If refresh request, then do it.
            //
            if (GetWindowLongPtr(hDlg, GWLP_USERDATA)) {

               BuildDocumentString();

               // Update all of the Directory Windows in order to see
               // the effect of the new extensions.

               hwndT = GetWindow(hwndMDIClient, GW_CHILD);
               while (hwndT) {
                  hwndNext = GetWindow(hwndT, GW_HWNDNEXT);
                  if (!GetWindow(hwndT, GW_OWNER))
                     SendMessage(hwndT, WM_FSC, FSC_REFRESH, 0L);
                  hwndT = hwndNext;
               }
            }

            // Free up class list
            RegUnload();

            EndDialog(hDlg, TRUE);
            break;
         }

      case IDD_BROWSE:
         {
            OPENFILENAME ofn;
            DWORD dwSave = dwContext;

            TCHAR szFile[MAX_PATH + 2];

            LPTSTR p;

            dwContext = IDH_ASSOC_BROWSE;

            LoadString(hAppInstance, IDS_PROGRAMS, szTemp, COUNTOF(szTemp));
            FixupNulls(szTemp);
            LoadString(hAppInstance, IDS_ASSOCIATE, szTitle, COUNTOF(szTitle));

            szFile[1] = CHAR_NULL;

            ofn.lStructSize          = sizeof(ofn);
            ofn.hwndOwner            = hDlg;
            ofn.hInstance            = NULL;
            ofn.lpstrFilter          = szTemp;
            ofn.lpstrCustomFilter    = NULL;
            ofn.nFilterIndex         = 1;
            ofn.lpstrFile            = szFile + 1;
            ofn.lpstrFileTitle       = NULL;
            ofn.nMaxFile             = COUNTOF(szFile)-2;
            ofn.lpstrInitialDir      = NULL;
            ofn.lpstrTitle           = szTitle;
            ofn.Flags                = OFN_SHOWHELP | OFN_HIDEREADONLY;
            ofn.lpfnHook             = NULL;
            ofn.lpstrDefExt          = NULL;

            if (!LoadComdlg())
               return TRUE;

            if ((*lpfnGetOpenFileNameW)(&ofn)) {

               if (StrChr(szFile+1, CHAR_SPACE)) {

                  szFile[0] = CHAR_DQUOTE;
                  lstrcat(szFile, SZ_DQUOTE);

                  p = szFile;
               } else {

                  p = szFile+1;
               }

               SetDlgItemText(hDlg, IDD_COMMAND, p);
            }

            dwContext = dwSave;
         }
         break;

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
   return(TRUE);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     AssociateFileDlgProc
//
// Synopsis:
//
// Return:   BOOL    TRUE  always
//
// Assumes:
//
// Effects:
//
//
// Notes:    pFileTypeBase prepended with new item if IDD_NEW
//
/////////////////////////////////////////////////////////////////////


INT
AssociateFileDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   INT i;
   DWORD dwError;

   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo;
   PTCHAR p;

   pAssociateFileDlgInfo = (PASSOCIATEFILEDLGINFO)GetWindowLongPtr(hDlg, GWLP_USERDATA);

   switch (wMsg) {
   case WM_INITDIALOG:
      {
         TCHAR szComboBoxString[DDETYPECOMBOBOXSIZ];

         // Set limit on # TCHARs on everything

         SendDlgItemMessage(hDlg, IDD_EXT, EM_LIMITTEXT,
            EXTSIZ-1, 0L);

         //
         // Save 3 characters of szCommand for " %1"
         //
         SendDlgItemMessage(hDlg, IDD_COMMAND, EM_LIMITTEXT,
            COUNTOF(pAssociateFileDlgInfo->DDEInfo[0].szCommand)-1-
            (COUNTOF(szDotEXE)-1),
            0L);

         SendDlgItemMessage(hDlg, IDD_DDEMESGTEXT, EM_LIMITTEXT,
            COUNTOF(pAssociateFileDlgInfo->DDEInfo[0].szDDEMesg)-1, 0L);

         SendDlgItemMessage(hDlg, IDD_DDEAPP, EM_LIMITTEXT,
            COUNTOF(pAssociateFileDlgInfo->DDEInfo[0].szDDEApp)-1, 0L);

         SendDlgItemMessage(hDlg, IDD_DDENOTRUN, EM_LIMITTEXT,
            COUNTOF(pAssociateFileDlgInfo->DDEInfo[0].szDDENotRun)-1, 0L);

         SendDlgItemMessage(hDlg, IDD_DDETOPIC, EM_LIMITTEXT,
            COUNTOF(pAssociateFileDlgInfo->DDEInfo[0].szDDETopic)-1, 0L);

         //
         // Initialize pAssociateFileDlgInfo;
         //
         pAssociateFileDlgInfo = (PASSOCIATEFILEDLGINFO) lParam;
         SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pAssociateFileDlgInfo);

         //
         // Set up combo box
         // Use aDDEType for entries to put in
         //
         SendDlgItemMessage(hDlg, IDD_ACTION, CB_RESETCONTENT, 0, 0L);

         for (i=0;i<DDETYPEMAX;i++) {

            //
            // Load in the string from the structure
            // Store as resource for localization
            //
            LoadString(hAppInstance, aDDEType[i].uiComboBox,
               szComboBoxString, COUNTOF(szComboBoxString));

            SendDlgItemMessage(hDlg, IDD_ACTION, CB_ADDSTRING,
               0, (LPARAM) szComboBoxString);

            //
            // Set the default item to the first thing
            //
            if (!i)
               SendDlgItemMessage(hDlg, IDD_ACTION, CB_SETCURSEL, 0, 0L);
         }

         dwError = FileTypeRead(hDlg, pAssociateFileDlgInfo);

         if (ERROR_SUCCESS != dwError) {

            FileAssociateErrorCheck(hDlg, IDS_EXTTITLE,
               IDS_FILETYPEREADERROR, dwError);
         }

         //
         // put in extension from prev dialog
         //
         SetDlgItemText(hDlg, IDD_EXT, &pAssociateFileDlgInfo->szExt[1]);

         //
         // HACK: for editing winini, go ahead and add the extension
         //
         if (IDD_COMMAND == pAssociateFileDlgInfo->mode) {

            AssociateFileDlgExtAdd(hDlg, pAssociateFileDlgInfo);

            GetProfileString(szExtensions, &pAssociateFileDlgInfo->szExt[1],
               szNULL, szTitle, COUNTOF(szTitle)-COUNTOF(szSpacePercentOne));

            p = StrChrQuote(szTitle, CHAR_SPACE);
            if (p)
               *p = CHAR_NULL;

            lstrcat(szTitle, szSpacePercentOne);
            SetDlgItemText(hDlg, IDD_COMMAND, szTitle);

            //
            // Setup the default name
            //
            p = GenerateFriendlyName(szTitle);
            SetDlgItemText(hDlg, IDD_DESC, p);

            // UpdateOKEnable(hDlg, pAssociateFileDlgInfo);
            pAssociateFileDlgInfo->bChange = TRUE;

         } else {
            pAssociateFileDlgInfo->bChange = FALSE;
         }

         pAssociateFileDlgInfo->bOKEnable = TRUE;

         UpdateOKEnable(hDlg, pAssociateFileDlgInfo);

         //
         // Send notification of change
         //
         SendMessage(hDlg, WM_COMMAND,
            GET_WM_COMMAND_MPS(IDD_EXT, GetDlgItem(hDlg, IDD_EXT), EN_CHANGE));

         break;
      }
   case WM_COMMAND:

      return AssociateFileDlgCommand(hDlg, wParam, lParam, pAssociateFileDlgInfo);

   default:
      if (wMsg == wHelpMessage || wMsg == wBrowseMessage) {
         WFHelp(hDlg);

         return TRUE;
      } else {
         return FALSE;
      }
   }
   return(TRUE);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     AssociateFileDlgCommand
//
// Synopsis: Handles WM_COMMAND for AssociateFileDlg
//
// IN  hDlg
// IN  wParam
// IN  lParam
// IN  pAssociateFileDlgInfo
//
//
// Return:   BOOL T = handled, F = nothandled
//
// Assumes:
//
// Effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL
AssociateFileDlgCommand(HWND hDlg,
   WPARAM wParam,
   LPARAM lParam,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
   INT i;
   TCHAR szExt[EXTSIZ];
   BOOL bChange;

   switch (GET_WM_COMMAND_ID(wParam, lParam)) {

   case IDD_EXTLIST:
      switch(GET_WM_COMMAND_CMD(wParam, lParam)) {
      case LBN_SELCHANGE:

         EnableWindow(GetDlgItem(hDlg, IDD_DELETE), TRUE);
         EnableWindow(GetDlgItem(hDlg, IDD_ADD), FALSE);

         //
         // Now set the edit text to the current selection
         //
         i = (INT) SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_GETCURSEL, 0, 0L);

         SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_GETTEXT, i, (LPARAM)szExt);
         SendDlgItemMessage(hDlg, IDD_EXT, WM_SETTEXT, 0, (LPARAM)szExt);

         break;
      default:
         break;
      }
      break;

   case IDD_HELP:
      WFHelp(hDlg);
      break;

   case IDD_EXT:
      {
         PEXT pExt;
         PEXT pExtNext;
         BOOL bForceOff = FALSE;
         HWND hwndAdd;
         HWND hwndDelete;
         HWND hwnd;

         switch(GET_WM_COMMAND_CMD(wParam, lParam)) {
         case EN_CHANGE:

            GetDlgItemText(hDlg, IDD_EXT, szExt, COUNTOF(szExt));

            ExtClean(szExt);

            //
            // If no match, turn everything off!
            //
            if (!szExt[0])
            {
               pExt = NULL;
               bForceOff = TRUE;
            }
            else
            {
               //
               // Now scan through to see if there is a match
               //
               for (pExt = pAssociateFileDlgInfo->pFileType->pExt;
                    pExt;
                    pExt = pExtNext)
               {
                  pExtNext = pExt->pftNext;

                  if (!pExt->bDelete && !lstrcmpi(szExt, pExt->szExt))
                  {
                     //
                     // Found one, Highlight!
                     //
                     i = SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_FINDSTRINGEXACT,
                        (WPARAM)-1, (LPARAM) &szExt[1]);

                     SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_SETCURSEL, i, 0L);
                     break;
                  }
               }

               if (!pExt)
               {
                  //
                  // No match, so deselect
                  //
                  SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_SETCURSEL, (WPARAM) -1, 0L);
               }
            }

            hwnd = GetFocus();

            EnableWindow(hwndDelete = GetDlgItem(hDlg, IDD_DELETE),
                         pExt ? TRUE: FALSE);

            EnableWindow(hwndAdd = GetDlgItem(hDlg, IDD_ADD),
                         bForceOff || pExt ? FALSE: TRUE);

            //
            // If we are disabling a button that has focus,
            // we must reassign focus to the other button
            //
            // (Can't get into the state where neither button is
            // enabled since this would require no text on the combobox
            // line, which can't happen since one of the add/del buttons
            // was enabled a second ago
            //

            if ((hwnd == hwndDelete && !pExt) || (hwnd == hwndAdd && pExt))
            {
               SendMessage(hwnd, BM_SETSTYLE,
                  MAKELONG(BS_PUSHBUTTON, 0), MAKELPARAM(FALSE, 0));

               hwnd = GetDlgItem(hDlg, IDOK);

               SendMessage(hwnd, BM_SETSTYLE,
                  MAKELONG(BS_DEFPUSHBUTTON, 0), MAKELPARAM(TRUE, 0));

               SetFocus(hwnd);
            }

            break;

         case EN_SETFOCUS:

            pAssociateFileDlgInfo->bExtFocus = TRUE;
            break;

         case EN_KILLFOCUS:

            pAssociateFileDlgInfo->bExtFocus = FALSE;
            break;

         default:
            break;
         }
      }

      break;
   case IDD_ADD:

      AssociateFileDlgExtAdd(hDlg, pAssociateFileDlgInfo);

      goto AddDelUpdate;

      break;
   case IDD_DELETE:

      AssociateFileDlgExtDelete(hDlg, pAssociateFileDlgInfo);

AddDelUpdate:

      // Send notification of change

      SendMessage(hDlg, WM_COMMAND,
         GET_WM_COMMAND_MPS(IDD_EXT, GetDlgItem(hDlg, IDD_EXT), EN_CHANGE));

      if (pAssociateFileDlgInfo->bExtFocus)
         SendDlgItemMessage(hDlg, IDD_EXT, EM_SETSEL, 0L, MAKELPARAM(0,-1));

      break;

   case IDD_ACTION:

      //
      // If changing selection, update.
      //
      if (CBN_SELCHANGE == GET_WM_COMMAND_CMD(wParam, lParam))
      {
         //
         // We must save bChange since any changes due to Action updates
         // aren't really changes by the user.
         //
         bChange = pAssociateFileDlgInfo->bChange;

         ActionDlgRead(hDlg, pAssociateFileDlgInfo);
         ActionUpdate(hDlg, pAssociateFileDlgInfo);

         pAssociateFileDlgInfo->bChange = bChange;
      }

      break;

   case IDD_DDE:

      if (BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam))
      {
         i = pAssociateFileDlgInfo->iAction =
            SendDlgItemMessage(hDlg, IDD_ACTION, CB_GETCURSEL, 0, 0L);

         DDEDlgRead(hDlg, pAssociateFileDlgInfo, i);
         pAssociateFileDlgInfo->DDEInfo[i].bUsesDDE =
            SendDlgItemMessage(hDlg, IDD_DDE, BM_GETCHECK, 0, 0L) != BST_UNCHECKED;

         DDEUpdate(hDlg, pAssociateFileDlgInfo,i);

         //
         // Modified, must update!
         //
         pAssociateFileDlgInfo->bChange = TRUE;
      }
      break;

   case IDCANCEL:

      //
      // If IDD_NEW or IDD_COMMAND, then free it (!IDD_CONFIG)
      //
      if (IDD_CONFIG != pAssociateFileDlgInfo->mode)
      {
         FileTypeFree(pAssociateFileDlgInfo->pFileType);
      }

Reload:
      //
      // Reload it!
      //
      RegUnload();

      //
      // We don't check errors here; put up what we can.
      //
      AssociateDlgInit(pAssociateFileDlgInfo->hDlg,
         pAssociateFileDlgInfo->szExt, pAssociateFileDlgInfo->iClassList);

      EndDialog(hDlg, TRUE);
      break;

   case IDOK:

      if (!pAssociateFileDlgInfo->bOKEnable)
      {
         MyMessageBox(hDlg, IDS_EXTTITLE, IDS_FILETYPECOMMANDNULLTEXT,
            MB_TASKMODAL|MB_OK|MB_ICONEXCLAMATION);

         SetFocus(GetDlgItem(hDlg, IDD_COMMAND));

         break;
      }

      ActionDlgRead(hDlg, pAssociateFileDlgInfo);
      i = AssociateFileWrite(hDlg, pAssociateFileDlgInfo);

      switch (i) {
      case DE_RETRY:
         break;

      case ERROR_SUCCESS:
         {
            SendDlgItemMessage(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST,
               WM_SETREDRAW, (WPARAM)FALSE, 0L);

            //
            // Delete the (None) entry at the beginning.
            //
            SendDlgItemMessage(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST, LB_DELETESTRING, 0, 0L);

            if (IDD_CONFIG != pAssociateFileDlgInfo->mode)
            {
               //
               // Add item..
               // Update pAssociateFileDlgInfo->iClassList for selection below
               //
               pAssociateFileDlgInfo->iClassList =
                  ClassListFileTypeAdd(pAssociateFileDlgInfo->hDlg, pFileTypeBase);
            }
            else
            {
               //
               // Now delete the old selection and reinsert
               //
               SendDlgItemMessage(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST,
                  LB_DELETESTRING, pAssociateFileDlgInfo->iClassList-1, 0L);

               ClassListFileTypeAdd(pAssociateFileDlgInfo->hDlg,
                  pAssociateFileDlgInfo->pFileType);
            }

            //
            // Add the (None) entry at the beginning.
            //
            SendDlgItemMessage(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST, LB_INSERTSTRING,0,(LPARAM)szNone);

            //
            // Select the item
            //
            SendDlgItemMessage(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST,
               LB_SETCURSEL, pAssociateFileDlgInfo->iClassList, 0L);

            SendDlgItemMessage(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST,
               WM_SETREDRAW, (WPARAM)TRUE, 0L);

            InvalidateRect(GetDlgItem(pAssociateFileDlgInfo->hDlg, IDD_CLASSLIST),
               NULL, TRUE);

            //
            // Notify previous dialog it needs to rebuild doc string
            //
            pAssociateFileDlgInfo->bRefresh = TRUE;

            // At this point, we change the previous dialog's "cancel"
            // button to Close.

            LoadString(hAppInstance, IDS_CLOSE, szTitle, COUNTOF(szTitle));

            SetDlgItemText(pAssociateFileDlgInfo->hDlg, IDCANCEL, szTitle);

            EndDialog(hDlg, TRUE);
            break;
         }
      default:

         //
         // Oops, couldn't write, simulate escape: discard all
         //
         goto Reload;
      }
      break;

   case IDD_BROWSE:
      {
         OPENFILENAME ofn;
         DWORD dwSave = dwContext;
         LPTSTR p;

         TCHAR szFile[MAX_PATH + 2];
         TCHAR szTemp2[MAX_PATH];

         dwContext = IDH_ASSOC_BROWSE;

         LoadString(hAppInstance, IDS_PROGRAMS, szTemp2, COUNTOF(szTemp2));
         FixupNulls(szTemp2);
         LoadString(hAppInstance, IDS_ASSOCIATE, szTitle, COUNTOF(szTitle));

         szFile[1] = 0L;

         ofn.lStructSize          = sizeof(ofn);
         ofn.hwndOwner            = hDlg;
         ofn.hInstance            = NULL;
         ofn.lpstrFilter          = szTemp2;
         ofn.lpstrCustomFilter    = NULL;
         ofn.nFilterIndex         = 1;
         ofn.lpstrFile            = szFile+1;
         ofn.lpstrFileTitle       = NULL;
         ofn.nMaxFile             = COUNTOF(szFile)-2;
         ofn.lpstrInitialDir      = NULL;
         ofn.lpstrTitle           = szTitle;
         ofn.Flags                = OFN_SHOWHELP | OFN_HIDEREADONLY;
         ofn.lpfnHook             = NULL;
         ofn.lpstrDefExt          = NULL;

         if (!LoadComdlg())
            return TRUE;

         if ((*lpfnGetOpenFileNameW)(&ofn))
         {
            if (StrChr(szFile+1, CHAR_SPACE))
            {
               szFile[0] = CHAR_DQUOTE;
               lstrcat(szFile, SZ_DQUOTE);

               p = szFile;
            }
            else
            {
               p = szFile+1;
            }

            SetDlgItemText(hDlg, IDD_COMMAND, p);
         }

         dwContext = dwSave;
      }
      break;

   case IDD_COMMAND:

      UpdateOKEnable(hDlg, pAssociateFileDlgInfo);

      //
      // Fall through
      //
   case IDD_DESC:
   case IDD_DDEMESG:
   case IDD_DDENOTRUN:
   case IDD_DDEAPP:
   case IDD_DDETOPIC:

      if (EN_CHANGE == GET_WM_COMMAND_CMD(wParam, lParam))
      {
         pAssociateFileDlgInfo->bChange = TRUE;
      }
      break;

   default:
      return FALSE;
   }

   return TRUE;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     UpdateOKEnable
//
// Synopsis: Updates the enablement of the OK button based commands
//
// INOUTC    hDlg                    --  dlg to read IDD_COMMAND/set IDOK
// INOUTC    pAssociateFileDlgInfo   --  info to check/report
//
// Return:   VOID
//
//
// Assumes:
//
// Effects:  IDOK button enablement, bOKEnable
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
UpdateOKEnable(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
   BOOL bOKEnable;
   INT i;

   //
   // The code here handles enablement of the OK button.
   // We want to disable this button when the command strings
   // for _all_ actions is NULL.
   //
   // Note that the command string for the current action isn't in
   // szCommand, but rather in the edit control.  That's why we skip
   // it and set bChange to the IDD_COMMAND len next.
   //
   bOKEnable=SendDlgItemMessage(hDlg, IDD_COMMAND, WM_GETTEXTLENGTH, 0, 0L)!=0;

   for (i=0; i<DDETYPEMAX; i++) {

      //
      // Don't let strings that the user is about to overwrite in
      // IDD_COMMAND count as a non-null command.
      //
      if (i == pAssociateFileDlgInfo->iAction)
         continue;

      if (pAssociateFileDlgInfo->DDEInfo[i].szCommand[0]) {
         bOKEnable = TRUE;
         break;
      }
   }

   if (bOKEnable != pAssociateFileDlgInfo->bOKEnable) {

      EnableWindow(GetDlgItem(hDlg, IDOK), bOKEnable);
      pAssociateFileDlgInfo->bOKEnable = bOKEnable;
   }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     RegLoad
//
// Synopsis: Reads the registry for extensions and file types
//           (Friendly name and key only).
//
// Return:   BOOL      TRUE  = success
//                     FALSE = fail     FileType, Ext INVALID
//
// Assumes:  !! Exclusive access to pExt->szIdent !!
//           ClassesRead sets this field, RegLoad reads it,
//           then the filed is INVALID !!
//
//           This is done because we must read everything in before we
//           try to link up.
//
// Effects:  Fills in the pExtBase and pFileTypeBase globals
//
//
// Notes:    RegUnload must be called to free space.
//
// First, we scan HKEY_CLASSES_ROOT (user/mach) for filetypes
// then extensions, then [extensions] in win.ini
//
/////////////////////////////////////////////////////////////////////

BOOL
RegLoad(VOID)
{
   PEXT pExt, pExtPrev = NULL;
   PEXT pExtNext;

   if (!ClassesRead(HKEY_CLASSES_ROOT,szNULL, &pFileTypeBase, &pExtBase))
      return FALSE;

   //
   // Traverse through all exts and initialize them
   // since ClassesRead must wait for all pFileTypes
   // to be read in before scanning.
   //

   for (pExt = pExtBase; pExt; pExt = pExtNext)
   {
      //
      // Get the next pointer
      //
      pExtNext = pExt->next;

      if (!ExtLinkToFileType(pExt, pExt->szIdent))
      {
         if (pExtPrev)
         {
            pExtPrev->next = pExtNext;
         }
         else
         {
            pExtBase = pExtNext;
         }

         ExtFree(pExt);
      }
      else
      {
         pExtPrev = pExt;
      }
   }

   return TRUE;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ClassesRead
//
// Synopsis: Appends FileType and Ext globals with registry info
//
// IN        hKey           HKEY     Key to use
// IN        lpszSubKey     LPTSTR    Name of subkey to open
//                                   = NULL for local machine else user path
// INOUT     ppFileType Base --       Last used base, * = NULL = first
// INOUT     ppExtBase       --       Last used base, * = NULL = first
//
// Return:   BOOL     = TRUE for success
//                    = FALSE for fail
//
// Assumes:  pExtBase and pFileTypeBase initialized (Valid or NULL)
//
// Effects:  pExtBase and pFileTypeBase (updated)
//           pFileTypeBase initialized if NULL
//           pExtBase initialized if NULL
//           if pFileType, classes inserted into IDD_CLASSLIST
//
// Notes:    Much cooler to do in C++
//
/////////////////////////////////////////////////////////////////////

BOOL
ClassesRead(HKEY hKey,
   LPTSTR lpszSubKey,
   PFILETYPE *ppFileTypeBase,
   PEXT *ppExtBase)
{
   HKEY hk;
   DWORD dwNameSiz;
   LONG  lNameSiz;
   UINT  uNameSiz;
   FILETIME ftLastWrite;
   DWORD dwError1, dwError2;
   PFILETYPE pFileTypePrev, pFileType;
   PEXT pExtPrev, pExt;
   BOOL retval;
   INT iKey;
   TCHAR szIdent[DESCSIZ+COUNTOF(szFileManPrefix)];
   TCHAR szExt[EXTSIZ+1];
   BOOL bFileType;

   // Open the requested location

   if (ERROR_SUCCESS != RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_ALL_ACCESS, &hk)) {
      return FALSE;
   }

   //
   // iKey is the index we use to
   // traverse through the keys
   //

   iKey = 0;

   pFileTypePrev = *ppFileTypeBase;
   pExtPrev = *ppExtBase;

   while (TRUE) {

      //
      // Begin enumerating all keys.  In one pass, we check
      // both filetypes and extensions.  We can't match
      // exts->filetypes until all filetypes are read in.
      //

      //
      // Leave one character off for the NULL
      // These must be BYTES, not CHARACTERS!

      dwNameSiz = sizeof(szIdent)-sizeof(*szIdent);

      // NOTE!  The returned length of the copied string
      // does not include the null terminator!
      // (contrast with RegQueryValue!)

      dwError1 = RegEnumKeyEx(hk,
         iKey,
         szIdent,
         &dwNameSiz,
         NULL, NULL, NULL, &ftLastWrite);

      switch (dwError1) {

      case ERROR_NO_MORE_ITEMS:

         //
         // This is the only case where we return true
         //

         retval = TRUE;
         goto ProcExit;

      case ERROR_MORE_DATA:

         // We can't handle this error!
         // Just truncate!

         szIdent[COUNTOF(szIdent)-1] = CHAR_NULL;

      case ERROR_SUCCESS:

         // Add zero delimiter
         szIdent[dwNameSiz] = CHAR_NULL;

         break;

      default:
         odf(TEXT("Error #%d\n"),dwError1);

         retval = FALSE;
         bFileType = FALSE;

         goto RestoreProcExit;
      }

      // Now add to pFileTypeBase or pExtBase

      if (bFileType = (CHAR_DOT != szIdent[0])) {

         //
         // It's a file type
         //

         pFileType = (PFILETYPE) LocalAlloc(LPTR, sizeof(FILETYPE));

         // Should put up dialog box error then
         // gracefully quit, keeping read in entries.

         if (!pFileType) {

            retval = FALSE;
            goto RestoreProcExit;
         }

         pFileType->uDesc = 0;
         if (FileTypeAddString(pFileType, szIdent, &pFileType->uDesc)) {

            retval = FALSE;
            goto RestoreProcExit;
         }

      } else {

         // truncate if necessary
         szIdent[COUNTOF(szExt)-1] = CHAR_NULL;
         lstrcpy(szExt,szIdent);
      }

      // It is valid, read in nice description/identifier

      lNameSiz = sizeof(szIdent);

      // Note!  RegQueryValue's return length does include
      // the null terminator!

      dwError2 = (DWORD) RegQueryValue(hk,
         bFileType ?
            pFileType->lpszBuf:
            szExt,
         szIdent,
         &lNameSiz);

      //
      // Divide by size of char to get character count
      // (not used)
      //
      // lNameSiz /= sizeof(*szIdent);

      switch (dwError2) {

      case ERROR_SUCCESS:
         break;

      case ERROR_FILE_NOT_FOUND:

         odf(TEXT("File not found: %s\n"),bFileType ? pFileType->lpszBuf:szExt);

         if (bFileType) {
            szIdent[0] = CHAR_NULL;
            dwError2 = ERROR_SUCCESS;
         } else {

            iKey++;
            continue;
         }
         break;

      case ERROR_MORE_DATA:

         // truncate
         szIdent[COUNTOF(szIdent)-1] = CHAR_NULL;
         dwError2 = ERROR_SUCCESS;
         break;

      default:

         odf(TEXT("2 Error #%d\n"),dwError2);

         if (bFileType) {
            szIdent[0] = CHAR_NULL;
            dwError2 = ERROR_SUCCESS;
         } else {

            iKey++;
            continue;
         }
//
//         retval = FALSE;
//         goto RestoreProcExit;
//
      }

      // Now read in \\shell\\open\\command
      // or Link up ext to filetype

      if (bFileType) {

         TCHAR szTemp[DESCSIZ+COUNTOF(szShellOpenCommand)];
         TCHAR szCommand[COMMANDSIZ];

         //
         // Copy in nice name
         //

         pFileType->uExe = pFileType->uDesc;
         if (FileTypeAddString(pFileType, szIdent, &pFileType->uExe)) {

            retval = FALSE;
            goto RestoreProcExit;
         }


         //
         // Create the new shell open command key
         //
         lstrcpy(szTemp, pFileType->lpszBuf);
         lstrcat(szTemp, szShellOpenCommand);

         lNameSiz = sizeof(szCommand);

         //
         // Get the exe
         //
         dwError2 = (DWORD) RegQueryValue(hk,
            szTemp,
            szCommand,
            &lNameSiz);

         //
         // Divide by char size to get cch.  (not used)
         //
         // lNameSiz /= sizeof(*szCommand);

         switch (dwError2) {
         case ERROR_SUCCESS:
            break;

         case ERROR_FILE_NOT_FOUND:

            //
            // Bogus entry; continue.
            //

            FileTypeFree(pFileType);

            iKey++;
            continue;

         case ERROR_MORE_DATA:

            // truncate

            szCommand[COUNTOF(szCommand)-1] = CHAR_NULL;
            dwError2 = ERROR_SUCCESS;

            break;

         default:
            odf(TEXT("2a Error #%d\n"),dwError2);

            //
            // Bogus entry; continue.
            //

            FileTypeFree(pFileType);

            iKey++;
            continue;
//
//            retval = FALSE;
//            goto RestoreProcExit;
//
         }

         //
         // Jam in the '(' and make the previous character a space
         //

         pFileType->lpszBuf[pFileType->uExe-1] = CHAR_SPACE;
         pFileType->lpszBuf[pFileType->uExe++] = CHAR_OPENPAREN;

         uNameSiz = pFileType->uExe;
         FileTypeAddString(pFileType, szCommand, &uNameSiz);
      }
      else
      {
         //
         // Check for duplicate extensions
         //
         if (ExtDupCheck(szExt, pExtPrev))
         {
            iKey++;
            continue;
         }

         // Put ident into pExt

         pExt = (PEXT) LocalAlloc(LPTR, sizeof(EXT) + ByteCountOf(lstrlen(szIdent)));
         if (!pExt)
         {
            retval = FALSE;
            goto RestoreProcExit;
         }

         lstrcpy(pExt->szExt, szExt);
         lstrcpy(pExt->szIdent, szIdent);
      }

      // Don't call ExtLinkToFileType!
      // Go through linked list only when everything read

      // Now either link up or initialize

      if (bFileType) {

         //
         // If prev exists, then we have already linked so just
         // append.  Otherwise, it's the first one.
         //
         if (pFileTypePrev)
         {
            pFileTypePrev->next = pFileType;
         }
         else
         {
            *ppFileTypeBase = pFileType;
         }

         pFileTypePrev = pFileType;
      }
      else
      {
         if (pExtPrev)
         {
            pExtPrev->next = pExt;
         }
         else
         {
            *ppExtBase = pExt;
         }

         pExtPrev = pExt;
      }

      iKey++;
   }

ProcExit:
   RegCloseKey(hk);
   return retval;

RestoreProcExit:

   // Memory error; restore and get out
   // Free as much as we can
   if (bFileType && pFileType) {

      FileTypeFree(pFileType);
   }
   goto ProcExit;
}





/////////////////////////////////////////////////////////////////////
//
// Name:     Classes Free
//
// Synopsis: Frees malloc'd info in pFileTypeBase
//
// IN  VOID
//
// Return:   VOID
//
// Assumes:
//
// Effects:  pFileTypeBase freed
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
ClassesFree(BOOL bFileType)
{
   VFILETYPEEXT vNext;
   VFILETYPEEXT vCur;

   if (bFileType) {
      vCur.pFileType = pFileTypeBase;
   } else {
      vCur.pExt = pExtBase;
   }

   while (vCur.vBoth) {

      if (bFileType) {
         vNext.pFileType = vCur.pFileType->next;
      } else {
         vNext.pExt = vCur.pExt->next;
      }

      if (bFileType) {
         FileTypeFree(vCur.pFileType);
      } else {
         ExtFree(vCur.pExt);
      }

      // Should be ok til next Alloc, but if multithreaded...
      vCur.vBoth=vNext.vBoth;
   }

   if (bFileType) {
      pFileTypeBase = NULL;
   } else {
      pExtBase = NULL;
   }
}



/////////////////////////////////////////////////////////////////////
//
// Name:     RegUnload
//
// Synopsis: Cleans up after RegLoad
//
// IN        VOID
//
// Return:   VOID
//
// Assumes:  RegLoad at some point before
//
// Effects:  Frees pExtBase and pFileTypeBase
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////


VOID
RegUnload(VOID)
{
   ClassesFree(TRUE);
   ClassesFree(FALSE);
}




/////////////////////////////////////////////////////////////////////
//
// Name:      FileTypeRead
//
// Synopsis:  Sets file dialog with info
//
// IN    hDlg                  HWND     Dialog box to fill
// INOUT pFileAssociateDlgInfo --       Identifier to read (e.g., "Word Doc")
//                                      NULL = Clear out fields
// IN uiShowSection)           UINT     DDESection
//
// Return:                     DWORD     Error code (ERROR_SUCCESS = 0l = ok)
//
// Assumes:
//
// Effects:  Sets all fields in the hDlg and updates
//           Modifies _ALL_ pFileAssociateDlgInfo.DDEInfo fields.
//
// Notes:  Does not do a CB_SETCURSEL
//
/////////////////////////////////////////////////////////////////////

DWORD
FileTypeRead(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
   UINT i;
   PEXT pExt;
   PEXT pExtNext;
   PFILETYPE pFileType = pAssociateFileDlgInfo->pFileType;
   DWORD dwError;
   HKEY hk;

   TCHAR szTemp[STRINGSIZ];

   // If we are in IDD_NEW mode, just return success
   // since we don't read anything for new ones.
   // We must clear out the lists

   pAssociateFileDlgInfo->iAction = 0;

   // Clear out DDEInfo
   for (i=0; i < DDETYPEMAX; i++) {
      pAssociateFileDlgInfo->DDEInfo[i].szCommand[0] = CHAR_NULL;
      pAssociateFileDlgInfo->DDEInfo[i].bUsesDDE = 0;
      pAssociateFileDlgInfo->DDEInfo[i].szDDEMesg[0] = CHAR_NULL;
      pAssociateFileDlgInfo->DDEInfo[i].szDDEApp[0] = CHAR_NULL;
      pAssociateFileDlgInfo->DDEInfo[i].szDDENotRun[0] = CHAR_NULL;
      pAssociateFileDlgInfo->DDEInfo[i].szDDETopic[0] = CHAR_NULL;
   }

   if (IDD_CONFIG != pAssociateFileDlgInfo->mode) {

      if (IDD_NEW == pAssociateFileDlgInfo->mode) {

         //
         // Set the dialog window title
         //
         if (LoadString(hAppInstance, IDS_NEWFILETYPETITLE, szTemp, COUNTOF(szTemp))) {
            SetWindowText(hDlg, szTemp);
         }
      }

      dwError = ERROR_SUCCESS;

   } else {

      // Put in the description!
      SetDlgItemText(hDlg, IDD_DESC, &pFileType->lpszBuf[pFileType->uDesc]);

      // Put up the open executable by default!
      SetDlgItemText(hDlg, IDD_COMMAND, &pFileType->lpszBuf[pFileType->uExe]);

      // Put in the extensions!

      for (pExt = pFileType->pExt; pExt; pExt = pExtNext)
      {
         pExtNext = pExt->pftNext;

         if (!pExt->bDelete)
         {
            CharLower(&pExt->szExt[1]);
            i = SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_ADDSTRING, 0, (LPARAM)&pExt->szExt[1]);

            SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_SETITEMDATA, i, (LPARAM)pExt);
         }
      }

      //
      // Certain file types will be read only
      // If so, we want to disable all the editing fields
      // To check this, we do a RegOpenKeyEx with KEY_WRITE privilege
      //

      if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT,
         pFileType->lpszBuf, 0L, KEY_WRITE, &hk)) {

         RegCloseKey(hk);

      } else {
         pAssociateFileDlgInfo->bReadOnly = TRUE;

         //
         // Disable EVERYTHING (well, almost)
         //

#define DISABLE(x) EnableWindow(GetDlgItem(hDlg, x), FALSE)

         DISABLE( IDD_DESCTEXT );
         DISABLE( IDD_COMMANDTEXT );
         DISABLE( IDD_BROWSE );
         DISABLE( IDD_DDEMESGTEXT );
         DISABLE( IDD_DDEAPPTEXT );
         DISABLE( IDD_DDENOTRUNTEXT );
         DISABLE( IDD_DDETOPICTEXT );
         DISABLE( IDD_DDEOPTIONALTEXT );

         DISABLE( IDD_DESC );
         DISABLE( IDD_COMMAND );
         DISABLE( IDD_SEARCHALL );
         DISABLE( IDD_DDE );
         DISABLE( IDD_DDEMESG );
         DISABLE( IDD_DDEAPP );
         DISABLE( IDD_DDENOTRUN );
         DISABLE( IDD_DDETOPIC );

#undef DISABLE
      }


      // Read in DDE Stuff.

      for (i=0; i < DDETYPEMAX; i++) {
         // Read in everything
         if (ERROR_SUCCESS != (dwError = DDERead(pAssociateFileDlgInfo, i)))
            break;
      }
   }

   ActionUpdate(hDlg, pAssociateFileDlgInfo);
   return dwError;
}





/////////////////////////////////////////////////////////////////////
//
// Name:     FileTypeWrite
//
// Synopsis: Writes out a file type
//
// IN      hDlg                  HWND   Dialog to read from
// IN      pAssociateFileDlgInfo --     Target to write out
// IN      hk                    HKEY   HKey to write to
// IN      lpszKey               LPTSTR  Beginning root key
//
//
// Return:   DWORD   ERROR_SUCCESS = success
//
//
// Assumes:  pAssociateFileDlgInfo setup
//
// Effects:  Writes to registry: NO exts written
//
//           IF IDD_NEW, does one of the following:
//              * Links up pFileType up to pFileType base: success
//                -- deleted when pFileTypeBase freed
//              * returns DE_RETRY, pFileType floating
//                -- return to original state.
//              * Error; returns !(==DE_RETRY || ==ERROR_SUCCESS)
//                -- pFileType FREED!
//
//           This is necessary to avoid memory leaks.
//
// Notes:    Generates identifier if none exists
//           Returns DE_RETRY if need to re-edit fields
//           Does not flushkey
//
//           If IDD_NEW, links up pFileType
//
//
/////////////////////////////////////////////////////////////////////

DWORD
FileTypeWrite(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo,
   HKEY hk,
   LPTSTR lpszKey)
{
   INT i;
   PFILETYPE pFileType = pAssociateFileDlgInfo->pFileType;
   DWORD dwError;
   LPTSTR p, p2;
   UINT uOffset;
   TCHAR szDesc[DESCSIZ];
   BOOL bSpace;
   BOOL bNotSpace;

   //
   // If read only, the user couldn't have changed anything,
   // so don't bother writing
   //

   if (pAssociateFileDlgInfo->bReadOnly)
      return ERROR_SUCCESS;

   if (!pAssociateFileDlgInfo->bChange)
      return ERROR_SUCCESS;

   //
   // First check if an identifier exists.  If it doesn't,
   // generate one.
   //

   if (IDD_CONFIG != pAssociateFileDlgInfo->mode) {

      //
      // Algo for generating ident begins here.
      // Just use the Desc for now...
      //

      GetDlgItemText(hDlg, IDD_DESC, szDesc, COUNTOF(szDesc));

      pFileType->uDesc = 0;

      //
      // Create prefix
      //
      wsprintf(szFileManPrefix, szFileManPrefixGen, 0);

      p=StrChrQuote(szFileManPrefix, CHAR_SPACE);

      if (p)
         *p = CHAR_NULL;

      if (dwError = FileTypeAddString(pFileType,
         szFileManPrefix, &pFileType->uDesc)) {
         goto Error;
      }

      //
      // Safe since szFileManPrefix is non-null.
      //

      pFileType->uDesc--;

      if (dwError = FileTypeAddString(pFileType, szDesc, &pFileType->uDesc))
         goto Error;

      //
      // Algo ends here
      // Now make sure that the ident is unique
      //

      if (FileTypeDupIdentCheck(hDlg, IDD_DESC, pFileType->lpszBuf))
         return DE_RETRY;
   }

   //
   // Get new desc
   //
   i = GetWindowTextLength(GetDlgItem(hDlg, IDD_DESC));

   // Make sure that it's non-NULL.
   if (!i) {

      //
      // Warn user to use non-NULL desc
      //

      MyMessageBox(hDlg, IDS_EXTTITLE, IDS_FILETYPENULLDESCERROR,
         MB_TASKMODAL|MB_OK|MB_ICONEXCLAMATION);

      //
      // Set focus to IDD_DESC
      //
      SetFocus(GetDlgItem(hDlg, IDD_DESC));

      return DE_RETRY;
   }

   GetDlgItemText(hDlg, IDD_DESC, szDesc, COUNTOF(szDesc));

   uOffset = pFileType->uDesc;
   if (dwError = FileTypeAddString(pFileType, szDesc, &uOffset))
      goto Error;

   //
   // We add 1 because we substitute our paren here
   //
   pFileType->lpszBuf[uOffset] = CHAR_OPENPAREN;
   pFileType->uExe = uOffset+1;

   for (i=0; i < DDETYPEMAX; i++) {
      p = pAssociateFileDlgInfo->DDEInfo[i].szCommand;

      //
      // Get Application: add %1 if not present and does not use dde
      // and there is at least one non-space character
      //

      if (!pAssociateFileDlgInfo->DDEInfo[i].bUsesDDE) {

         // Scan through for % after space
         for(p2 = p, bSpace = FALSE, bNotSpace = FALSE; *p2; p2++) {

            if (CHAR_SPACE == *p2)
               bSpace = TRUE;
            else
               bNotSpace = TRUE;

            if (szSpacePercentOne[1] == *p2 && bSpace)
               break;
         }

         // lstrcat only if enough space and % wasn't found

         if (!*p2 && bNotSpace &&
            lstrlen(p) < COUNTOF(pAssociateFileDlgInfo->DDEInfo[i].szCommand) -
            COUNTOF(szSpacePercentOne) -1) {

            lstrcat(p, szSpacePercentOne);
         }
      }
   }


   uOffset = pFileType->uExe;

   if (dwError = FileTypeAddString(pFileType,
      pAssociateFileDlgInfo->DDEInfo[0].szCommand, &uOffset)) {

      goto Error;
   }

   // Write out friendly name (desc)
   // Add +1?  cf. other RegSetValues!

   dwError = RegSetValue(hk, pFileType->lpszBuf, REG_SZ,
      &pFileType->lpszBuf[pFileType->uDesc],
      lstrlen(&pFileType->lpszBuf[pFileType->uDesc]));

   if (ERROR_SUCCESS != dwError)
      goto Error;

   // Write out DDE Stuff.

   for (i=0; i < DDETYPEMAX; i++) {
      // Read in everything
      if (ERROR_SUCCESS != (dwError = DDEWrite(pAssociateFileDlgInfo, i))) {
         goto Error;
      }
   }

   // Link up pFileType if this is new!
   if (IDD_CONFIG != pAssociateFileDlgInfo->mode) {
      pFileType->next = pFileTypeBase;
      pFileTypeBase = pFileType;
   }

   return ERROR_SUCCESS;

Error:

   if (IDD_CONFIG != pAssociateFileDlgInfo->mode) {
      LocalFree((HLOCAL)pFileType);
   }
   return dwError;
}

/////////////////////////////////////////////////////////////////////
//
// Name:     DDERead
//
// Synopsis: Reads in all DDE info about a filetype
//
// INOUT     pAssociateFileDlgInfo   --
// IN        iIndex                      INT  index of DDE info to read
//
// Return:   DWORD     = ERROR_SUCCESS success
//                    = else failed, = error code
//
// Assumes:  pAssociateFileDlgInfo set up correctly
//
// Effects:  pAssociateFileDlgInfo-> DDEInfo
//
// Notes:    reads registry
//
/////////////////////////////////////////////////////////////////////

#define ERRORCHECK \
   {  if (ERROR_SUCCESS!=dwError && ERROR_FILE_NOT_FOUND!=dwError) \
      return dwError; }

#define BUSESDDECHECK \
   {  if (ERROR_FILE_NOT_FOUND != dwError) \
         pAssociateFileDlgInfo->DDEInfo[i].bUsesDDE = TRUE; }

// lSize => _byte_ count, not character count

#define REGREAD(str)                                                      \
   {                                                                      \
       HKEY _hkey;                                                        \
                                                                          \
       str[0] = CHAR_NULL;                                                \
       lSize = sizeof(str);                                               \
       dwError = 0;                                                       \
       if (RegOpenKey(HKEY_CLASSES_ROOT, szKey, &_hkey) == ERROR_SUCCESS) \
       {                                                                  \
           dwError = (DWORD)RegQueryValueEx( _hkey,                       \
                                             TEXT(""),                    \
                                             NULL,                        \
                                             NULL,                        \
                                             (LPBYTE)str,                 \
                                             &lSize );                    \
           RegCloseKey(_hkey);                                            \
       }                                                                  \
   }

DWORD
DDERead(PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, INT i)
{
   TCHAR szKey[MAX_PATH];
   INT iPoint;
   LONG lSize;
   DWORD dwError;
   LPTSTR p, p2;

   pAssociateFileDlgInfo->DDEInfo[i].bUsesDDE = FALSE;

   // BONK!  Should do length checking here.

   lstrcpy(szKey, pAssociateFileDlgInfo->pFileType->lpszBuf);
   lstrcat(szKey, szShell);
   lstrcat(szKey, aDDEType[i].lpszRegistry);

   iPoint = lstrlen(szKey);

   lstrcat(szKey, szCommand);

   REGREAD(pAssociateFileDlgInfo->DDEInfo[i].szCommand);

   ERRORCHECK;

   lstrcpy(&szKey[iPoint],szDDEExec);

   REGREAD(pAssociateFileDlgInfo->DDEInfo[i].szDDEMesg);

   ERRORCHECK;
   BUSESDDECHECK;

   iPoint = lstrlen(szKey);
   lstrcat(szKey, szApp);

   REGREAD(pAssociateFileDlgInfo->DDEInfo[i].szDDEApp);

   ERRORCHECK;
   BUSESDDECHECK;

   // Per win31, default to first (' '=delimiter) token's
   // last filespec ('\'=delimiter)
   p2 = pAssociateFileDlgInfo->DDEInfo[i].szDDEApp;

   if (ERROR_SUCCESS != dwError || !p2[0]) {


      lstrcpy(p2, pAssociateFileDlgInfo->DDEInfo[i].szCommand);

      //
      // For the application string, default to the
      // executable name without the extensions
      //
      for(p=p2;*p;p++) {
         if (CHAR_DOT == *p || CHAR_SPACE == *p) {
            *p = CHAR_NULL;
            break;
         }
      }

      StripPath(p2);

      if (*p2)
         *p2=(TCHAR)CharUpper((LPTSTR)*p2);
   }

   lstrcpy(&szKey[iPoint],szTopic);

   REGREAD(pAssociateFileDlgInfo->DDEInfo[i].szDDETopic);

   ERRORCHECK;
   BUSESDDECHECK;

   // The default for no topic is system.

   if (ERROR_SUCCESS != dwError || !pAssociateFileDlgInfo->DDEInfo[i].szDDETopic[0]) {
      lstrcpy(pAssociateFileDlgInfo->DDEInfo[i].szDDETopic, szDDEDefaultTopic);
   }

   lstrcpy(&szKey[iPoint],szIFExec);

   REGREAD(pAssociateFileDlgInfo->DDEInfo[i].szDDENotRun);

   ERRORCHECK;
   BUSESDDECHECK;

   return ERROR_SUCCESS;
}

#undef ERRORCHECK
#undef BUSESDDECHECK
#undef REGREAD



/////////////////////////////////////////////////////////////////////
//
// Name:     DDEWrite
//
// Synopsis: Writes in all DDE info about a filetype
//
// IN        pAssociateFileDlgInfo   --
// IN        iIndex                      INT  index of DDE info to read
//
// Return:   DWORD     = ERROR_SUCCESS succeed
//                    = else fail, = error code
//
// Assumes:  pAssociateFileDlgInfo set up correctly
//
// Effects:
//
// Notes:    writes registry
//           does not flushkey
//
/////////////////////////////////////////////////////////////////////

#define ERRORCHECK \
   {  if (ERROR_SUCCESS != dwError) \
         return dwError; }


// +1 for RegSetValue???  (lstrlen)

#define DDEREGSET(lpsz) \
   {  \
      dwSize = lstrlen(lpsz) * sizeof(*lpsz); \
      dwError = RegSetValue(HKEY_CLASSES_ROOT,  \
         szKey,                              \
         REG_SZ,                             \
         lpsz,                               \
         dwSize);\
   }

DWORD
DDEWrite(PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo, INT i)
{
   TCHAR szKey[MAX_PATH];
   INT iPoint;
   DWORD dwSize;
   DWORD dwError;

   //
   // LATER:  Should do length checking here.
   //
   lstrcpy(szKey, pAssociateFileDlgInfo->pFileType->lpszBuf);
   lstrcat(szKey, szShell);
   lstrcat(szKey, aDDEType[i].lpszRegistry);

   //
   // If we are not writing the main command (open) and
   // there is no szCommand, then delete the node.
   //
   if (i && !pAssociateFileDlgInfo->DDEInfo[i].szCommand[0]) {
      dwError = RegNodeDelete(HKEY_CLASSES_ROOT, szKey);
      return dwError;
   }

   //
   // Save this point in the string since we will replace
   // open with print.
   //
   iPoint = lstrlen(szKey);
   lstrcat(szKey, szCommand);

   dwSize = lstrlen(pAssociateFileDlgInfo->DDEInfo[i].szCommand) *
      sizeof(*pAssociateFileDlgInfo->DDEInfo[i].szCommand);

   dwError = RegSetValue(HKEY_CLASSES_ROOT,
      szKey,
      REG_SZ,
      pAssociateFileDlgInfo->DDEInfo[i].szCommand,
      dwSize);

   ERRORCHECK;

   lstrcpy(&szKey[iPoint],szDDEExec);

   if (!pAssociateFileDlgInfo->DDEInfo[i].bUsesDDE) {
      dwError = RegNodeDelete(HKEY_CLASSES_ROOT, szKey);
      return dwError;
   }

   DDEREGSET(pAssociateFileDlgInfo->DDEInfo[i].szDDEMesg);
   ERRORCHECK;

   iPoint = lstrlen(szKey);
   lstrcat(szKey, szApp);

   DDEREGSET(pAssociateFileDlgInfo->DDEInfo[i].szDDEApp);
   ERRORCHECK;

   lstrcpy(&szKey[iPoint],szTopic);

   DDEREGSET(pAssociateFileDlgInfo->DDEInfo[i].szDDETopic);
   ERRORCHECK;

   lstrcpy(&szKey[iPoint],szIFExec);

   if (!pAssociateFileDlgInfo->DDEInfo[i].szDDENotRun[0]) {

      dwError = RegNodeDelete(HKEY_CLASSES_ROOT, szKey);
   } else  {

      DDEREGSET(pAssociateFileDlgInfo->DDEInfo[i].szDDENotRun);
   }

   return dwError;
}

#undef DDEREGSET
#undef ERRORCHECK

/////////////////////////////////////////////////////////////////////
//
// Name:      ActionUpdate
//
// Synopsis:  Updates screen based on new action
//
// IN hDlg                  HWND  Dlg to modify
// IN pAssociateFileDlgInfo --    Info source
//
//
//
// Return:    VOID
//
// Assumes:
//
// Side effects:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
ActionUpdate(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
    INT i;

    i = SendDlgItemMessage(hDlg, IDD_ACTION, CB_GETCURSEL, 0, 0L);

    //
    // Update out internal variable
    //
    pAssociateFileDlgInfo->iAction = i;
    SetDlgItemText( hDlg,
                    IDD_COMMAND,
                    pAssociateFileDlgInfo->DDEInfo[i].szCommand );

    DDEUpdate(hDlg, pAssociateFileDlgInfo, i);
}


VOID
ActionDlgRead(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
   INT iAction = pAssociateFileDlgInfo->iAction;

   GetDlgItemText(hDlg, IDD_COMMAND,
      pAssociateFileDlgInfo->DDEInfo[iAction].szCommand,
      COUNTOF(pAssociateFileDlgInfo->DDEInfo[iAction].szCommand));

   DDEDlgRead(hDlg, pAssociateFileDlgInfo, iAction);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     DDEDlgRead
//
// Synopsis: Reads in the dde info from the dialog
//
// IN        hDlg                   HWND
// OUT       pAssociateFileDlgInfo  --    ->DDEInfo[iAction] filled
// INT       iAction                --    Which DDEInfo to fill
//
//
// Return:   VOID
//
//
// Assumes:  Dlg is set up
//           Must be called with the IDD_DDE button in the true
//           state (before it is toggled if we want to preserve state.
//
// Effects:  DDEInfo[iAction]
//
// Notes:    If bUsesDDE is off in DDEInfo[iAction], nothing is read
//
/////////////////////////////////////////////////////////////////////


VOID
DDEDlgRead(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo,
   INT iAction)
{
   if (!pAssociateFileDlgInfo->DDEInfo[iAction].bUsesDDE)
      return;

   GetDlgItemText(hDlg, IDD_DDEMESG, pAssociateFileDlgInfo->DDEInfo[iAction].szDDEMesg,
      COUNTOF(pAssociateFileDlgInfo->DDEInfo[iAction].szDDEMesg));

   GetDlgItemText(hDlg, IDD_DDEAPP, pAssociateFileDlgInfo->DDEInfo[iAction].szDDEApp,
      COUNTOF(pAssociateFileDlgInfo->DDEInfo[iAction].szDDEApp));

   GetDlgItemText(hDlg, IDD_DDENOTRUN, pAssociateFileDlgInfo->DDEInfo[iAction].szDDENotRun,
      COUNTOF(pAssociateFileDlgInfo->DDEInfo[iAction].szDDENotRun));

   GetDlgItemText(hDlg, IDD_DDETOPIC, pAssociateFileDlgInfo->DDEInfo[iAction].szDDETopic,
      COUNTOF(pAssociateFileDlgInfo->DDEInfo[iAction].szDDETopic));
}

VOID
DDEUpdate(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo,
   INT iAction)
{
   BOOL bEnable;

   if ( pAssociateFileDlgInfo->DDEInfo[iAction].bUsesDDE ) {
      // Signal on
      bEnable = TRUE;

      SetDlgItemText(hDlg, IDD_DDEMESG, pAssociateFileDlgInfo->DDEInfo[iAction].szDDEMesg);
      SetDlgItemText(hDlg, IDD_DDEAPP, pAssociateFileDlgInfo->DDEInfo[iAction].szDDEApp);
      SetDlgItemText(hDlg, IDD_DDENOTRUN, pAssociateFileDlgInfo->DDEInfo[iAction].szDDENotRun);
      SetDlgItemText(hDlg, IDD_DDETOPIC, pAssociateFileDlgInfo->DDEInfo[iAction].szDDETopic);
   } else {
      bEnable = FALSE;

      SetDlgItemText(hDlg, IDD_DDEMESG, szNULL);
      SetDlgItemText(hDlg, IDD_DDEAPP, szNULL);
      SetDlgItemText(hDlg, IDD_DDENOTRUN, szNULL);
      SetDlgItemText(hDlg, IDD_DDETOPIC, szNULL);
   }


   SendDlgItemMessage(hDlg, IDD_DDE, BM_SETCHECK, bEnable, 0L);

   if (!pAssociateFileDlgInfo->bReadOnly) {
      EnableWindow(GetDlgItem(hDlg, IDD_DDEMESG), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDEAPP), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDENOTRUN), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDETOPIC), bEnable);

      EnableWindow(GetDlgItem(hDlg, IDD_DDEMESGTEXT), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDEAPPTEXT), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDENOTRUNTEXT), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDETOPICTEXT), bEnable);
      EnableWindow(GetDlgItem(hDlg, IDD_DDEOPTIONALTEXT), bEnable);
   }
}




/////////////////////////////////////////////////////////////////////
//
// Name:     FileTypeGrow
//
// Synopsis: Grows the FileType block; initializes it if necessary
//
// INOUT pFileType    --    pFileType to grow (lpszBuf member)
// INC   cchNewSiz    --    requested buffer size (chars)
//
// Return:   BOOL     = TRUE for success
//                    = FALSE for fail:     (FileType fails)
//
// Assumes:
//
// Effects:  pFileType (local)
//           pFileType->lpszBuf   (grown)
//           pFileType->cchBufSiz  (updated)
//
// Notes:    if FileType.cchBufSiz == 0, creates first chunk
//           pFileType->lpszBuf is volatile and may move during growth
//
/////////////////////////////////////////////////////////////////////

BOOL
FileTypeGrow(PFILETYPE pFileType, UINT cchNewSiz)
{
   cchNewSiz = ((cchNewSiz-1)/FILETYPEBLOCK + 1) * FILETYPEBLOCK;

   //
   // If empty, initialize
   //
   if (!pFileType->cchBufSiz) {

      pFileType->cchBufSiz = cchNewSiz;
      pFileType->lpszBuf = (LPTSTR) LocalAlloc(LPTR,ByteCountOf(cchNewSiz));

      if (!pFileType->lpszBuf)
          return FALSE;

      return TRUE;
   }

   //
   // Grow the structure
   //
   pFileType->cchBufSiz = cchNewSiz;
   pFileType->lpszBuf = (LPTSTR) LocalReAlloc(pFileType->lpszBuf,
      ByteCountOf(pFileType->cchBufSiz), LMEM_MOVEABLE);

   if (!pFileType->lpszBuf)
      return FALSE;

   return TRUE;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ExtLinkToFileType
//
// Synopsis: Takes a pExt and fills in the pFileType field to
//           the matching file type (based on pFileTypeBase global)
//
// IN       pExt       PEXT  extension to link up
// INC      lpszIdent  --    filetype ident
//
// Return:  BOOL    = TRUE found one; successful link
//                  = FALSE failed
//
// Assumes: pFileTypeBase set up correctly
//          It REALLY IS linked to lpszIdent in the registry.  Flushed, too.
//
// Effects: pExt->pFileType
//              ->pftNext
//              ->pftOrig
//          pFileType      (only if successfully linked)
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL
ExtLinkToFileType(PEXT pExt, LPTSTR lpszIdent)
{
    PFILETYPE pftCur;
    PFILETYPE pftNext;


    if (CHAR_DOT == lpszIdent[0])
    {
        return (FALSE);
    }

    // Linear search of pFileTypeBase.
    // Slow, but that's ok.

    for (pftCur = pFileTypeBase; pftCur; pftCur = pftNext)
    {
        pftNext = pftCur->next;

        if (!lstrcmpi(pftCur->lpszBuf, lpszIdent))
        {
            //
            // Now link the ext to the pFileType
            //
            ExtLink(pExt, pftCur);

            //
            // This linkage is already written in the registry
            //
            pExt->pftOrig = pftCur;

            return (TRUE);
        }
    }

    pExt->pftNext = NULL;
    pExt->pFileType = NULL;
    pExt->pftOrig = NULL;

    return (FALSE);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     AssociateFileDlgExtDelete
//
// Synopsis: Deletes an extension to a file type
//
// IN    hDlg                   HWND
// INOUT pAssociateFileDlgInfo  --
//
// Return:   BOOL    TRUE = successful
//
// Assumes:  The extension to be deleted is highlighted in the cb dropdownbox
//           It _must_already_exist_ in the current pFileType!
//
// Effects:  Modifies matching pExt by turning off the deleted ext.
//
//
// Notes:    bDelete is turned on; bAdd is turned off.
//
//           Only the data structures are changed.  The registry
//           is not written here.
//
/////////////////////////////////////////////////////////////////////

BOOL
AssociateFileDlgExtDelete(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
    PEXT pExt;
    INT i;


    GetDlgItemText(hDlg, IDD_EXT, pAssociateFileDlgInfo->szExt, COUNTOF(pAssociateFileDlgInfo->szExt));
    ExtClean(pAssociateFileDlgInfo->szExt);

    i = SendDlgItemMessage( hDlg,
                            IDD_EXTLIST,
                            LB_FINDSTRINGEXACT,
                            (WPARAM)-1,
                            (LPARAM)&pAssociateFileDlgInfo->szExt[1]);
    if (LB_ERR == i)
    {
        return (FALSE);
    }

    pExt = (PEXT) SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_GETITEMDATA, (WPARAM)i, 0L);
    pExt->bAdd = FALSE;
    pExt->bDelete = TRUE;

    SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_DELETESTRING, (WPARAM)i, 0L);

    return (TRUE);
}

/////////////////////////////////////////////////////////////////////
//
// Name:     AssociateFileDlgExtAdd
//
// Synopsis: Adds an extension to a file type
//
// IN    hDlg                   HWND
// INOUT pAssociateFileDlgInfo  --
//
// Return:   BOOL    TRUE = successful
//
// Assumes:  IDD_EXT cb edit field has a valid extension
//           It _must_not_already_exist_ in the current pFileType!
//
// Effects:  Modifies pExtBase by adding a new ext or reactivating
//           a deleted one.  pFileType also modified.
//
//
// Notes:    If the ext is already used by a different file type,
//           then a confirm overwrite dialog appears
//
//           Only the data structures are changed.  The registry
//           is not written here.
//
// Invariant: pExtBase [->next] links all pExt's in the system, including
//            "deleted" ones.  These pExts are ExtFree'd when the registry
//            is written
//
//            pFileType->pExt  [->pftNext] are all the pExt's associated
//            with the file type pFileType
//
//            pExt->pFileType indicates which filetype a pExt is
//            associated with
//
//            A = new, D = delete
//
//            (not modified:) pFileType [->next] are all filetypes,
//            even deleted ones.
//
/////////////////////////////////////////////////////////////////////

BOOL
AssociateFileDlgExtAdd(HWND hDlg,
   PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
   PEXT pExt;
   PFILETYPE pFileType = pAssociateFileDlgInfo->pFileType;
   INT i;

   // Get text

   GetDlgItemText(hDlg, IDD_EXT, pAssociateFileDlgInfo->szExt,
      COUNTOF(pAssociateFileDlgInfo->szExt));

   //
   // Search through the known universe and beyond for intelligent
   // life, or at least a matching extension
   //
   pExt = BaseExtFind(pAssociateFileDlgInfo->szExt);

   //
   // BaseExtFind does a ExtClean, so don't do it here.
   //
   // ExtClean(pAssociateFileDlgInfo->szExt);
   //

   if (pExt) {

      // If not deleted, we need to confirm with the user.

      if (!pExt->bDelete) {

         // Oops, already exists, and since we can safely assume
         // That it belongs to some other pFileType, we must ask
         // the user if we really can do this.

         TCHAR szText[MAX_PATH];
         TCHAR szTitle[MAX_PATH];
         TCHAR szTemp[MAX_PATH];

         //
         // If it's already associated to something that's
         // NOT deleted, warn the user.
         //
         if (pExt->pFileType) {

            LoadString(hAppInstance, IDS_ADDEXTTITLE, szTitle, COUNTOF(szTitle));
            LoadString(hAppInstance, IDS_ADDEXTTEXT, szTemp, COUNTOF(szTemp));

            wsprintf(szText, szTemp,
               pExt->szExt,
               &pExt->pFileType->lpszBuf[pExt->pFileType->uDesc]);

            if (IDYES != MessageBox(hDlg, szText, szTitle, MB_TASKMODAL|MB_YESNO|MB_ICONEXCLAMATION)) {
               goto Fail;
            }
         } else {

            // !! LATER  associated with .exe, not friendly !!
         }
      }

      //
      // Is this extension already associated with a filetype?
      // (Even if this extension was already "deleted.")
      //
      if (pExt->pFileType) {
         //
         // split it out
         //
         ExtDelink(pExt);
      }

      //
      // Relink into pftExt chain
      //

      ExtLink(pExt, pFileType);

      pExt->bAdd = TRUE;
      pExt->bDelete = FALSE;

   } else {

      // Ok, doesn't exist so let's just throw it in.
      pExt = (PEXT)LocalAlloc (LPTR, sizeof(EXT));
      if (!pExt)
         return FALSE;

      lstrcpy(pExt->szExt,pAssociateFileDlgInfo->szExt);

      // Set it's flags
      pExt->bAdd = TRUE;
      pExt->bDelete = FALSE;

      // Now link it up
      ExtLink(pExt, pFileType);

      // Since it's new, no original owner
      pExt->pftOrig = NULL;

      // Now put it in pExtBase
      pExt->next = pExtBase;
      pExtBase = pExt;
   }

   // Success!  Now add it into the listbox!

   CharLower(&pExt->szExt[1]);
   i = (INT) SendDlgItemMessage(hDlg,IDD_EXTLIST,
      LB_ADDSTRING,0,(LPARAM)&pExt->szExt[1]);

   SendDlgItemMessage(hDlg, IDD_EXTLIST, LB_SETITEMDATA, i, (LPARAM)pExt);

   return TRUE;

Fail:
   return FALSE;
}




/////////////////////////////////////////////////////////////////////
//
// Name:     RegExtAdd
//
// Synopsis: Adds ext to pfiletype in registry and updates
//           IDD_EXT in hDlg
//
// INOUT     hDlg      HWND hDlg of the first dialog
// IN        hk        HKEY HKey to add to
// IN        pExt      --   ext to add to registry
// IN        pFileType --   pFileType to write it to
//
// Return:   DWORD      Error code, ERROR_SUCCESS = no error
//
// Assumes:  RegSetValue can overwrite keys (i.e., preexisting keys)
//           Won't attempt to add to current filetype
//           pExt->pftOrig == pFileType
//
// Effects:  Registry written
//
//
// Notes:    if pExt is NULL, ERROR_SUCCESS is returned
//           does not flushkey
//
/////////////////////////////////////////////////////////////////////


DWORD
RegExtAdd(HWND hDlg, HKEY hk, PEXT pExt, PFILETYPE pFileType)
{
    DWORD dwError;

   
    if (!pExt)
    {
        return (ERROR_SUCCESS);
    }
   
    if (pExt->pftOrig == pFileType)
    {
        pExt->bAdd = FALSE;
        pExt->bDelete = FALSE;
   
        return (ERROR_SUCCESS);
    }
   
    // Now add it to the parent IDD_EXT if not new
   
    dwError = RegExtAddHelper(hk, pExt->szExt, pFileType);
   
    if (ERROR_SUCCESS != dwError)
    {
        return (dwError);
    }
   
    if (!pExt->pftOrig)
    {
        CharLower(&pExt->szExt[1]);
        SendDlgItemMessage( hDlg,
                            IDD_EXT,
                            CB_ADDSTRING,
                            0,
                            (LPARAM) &pExt->szExt[1] );
    }
   
    //
    // Now we set this because it is written in the registry.
    // Note that above we didn't write if this was set correctly.
    //
    pExt->pftOrig = pFileType;

    pExt->bAdd = FALSE;
    pExt->bDelete = FALSE;
   
    return (dwError);
}

DWORD
RegExtAddHelper(HKEY hk, LPTSTR lpszExt, PFILETYPE pFileType)
{
   return ( (DWORD) RegSetValue( hk,
                                 lpszExt,
                                 REG_SZ,
                                 pFileType->lpszBuf,
                                 pFileType->uDesc + 1 ) );
}


/////////////////////////////////////////////////////////////////////
//
// Name:     RegExtDelete
//
// Synopsis: Delete Ext from registry and from IDD_EXT
//
// INOUT     hDlg   HWND hDlg of the first associate dlg
// IN        hk     HKEY HKey to use
// IN        pExt   --   Ext to delete
//
// Return:   DWORD    error code (ERROR_SUCCESS = ok)
//
//
// Assumes:  pExt->bDelete is set and therefore be safely deleted
//           from existence
//
// Effects:  Updates the pExtBase to a stable state.
//           pExt is either deleted from the backbone or changed to mach ext
//
// Notes:    if pExt is NULL, ERROR_SUCCESS is returned
//           pExt->szExt may point to something else in the registry
//           does not flushkey
//
//           Callee can safely call this during a traversal, of either
//           the backbone or pExt->pftNext since ExtLink PREPENDS.
//
//           !! Does not update pExt->szIdent !!
//
/////////////////////////////////////////////////////////////////////

DWORD
RegExtDelete(HWND hDlg, HKEY hk, PEXT pExt)
{
    DWORD dwError;
    INT i;
    PEXT pExt2;


    if (!pExt)
    {
        return (ERROR_SUCCESS);
    }
   
    //
    // Axe it in winini
    //
    WriteProfileString(szExtensions, pExt->szExt + 1, NULL);
   
    if (!pExt->pftOrig)
    {
        //
        // Doesn't exist in registry
        //
        return (ERROR_SUCCESS);
    }
   
    dwError = RegNodeDelete(hk, pExt->szExt);
   
    if (ERROR_SUCCESS == dwError)
    {
        i = SendDlgItemMessage( hDlg,
                                IDD_EXT,
                                CB_FINDSTRINGEXACT,
                                (WPARAM)-1,
                                (LPARAM)&pExt->szExt[1] );
        if (CB_ERR != i)
        {
            SendDlgItemMessage(hDlg, IDD_EXT, CB_DELETESTRING, i, 0L);
        }
       
        //
        // No longer associated with a filetype
        //
        ExtDelink(pExt);

        //
        // Remove from backbone
        //
        if (pExt == pExtBase)
        {
            pExtBase = pExt->next;
        }
        else
        {
            pExt2 = pExtBase;
            while (pExt2->next != pExt)
            {
                pExt2 = pExt2->next;
            }
            pExt2->next = pExt->next;
        }

        ExtFree(pExt);
    }
   
    return (dwError);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     RegNodeDelete
//
// Synopsis: Deletes a node and its keys/values, etc
//
// IN        hk        HKEY    reg key to use  (open or HKEY_*)
// IN        lpszKey   LPTSTR   subkeynode to delete
//
// Return:   DWORD      Error code, ERROR_SUCCESS = OK.
//
// Assumes:
//
// Effects:  Registry filetype is deleted
//
// Notes:    does not flushkey
//
// !! BUGBUG !!
// This code violates the RegEnumKey edict (api32wh.hlp) that no one
// should change the database while enumerating.
//
/////////////////////////////////////////////////////////////////////

DWORD
RegNodeDelete(HKEY hk, LPTSTR lpszKey)
{
    HKEY hkNode;
    DWORD dwError;
    TCHAR szKey[MAX_PATH];
   
   
    dwError = RegOpenKey(hk, lpszKey, &hkNode);
   
    if (ERROR_SUCCESS != dwError)
    {
        if (ERROR_FILE_NOT_FOUND == dwError)
        {
            dwError = ERROR_SUCCESS;
        }
   
        return (dwError);
    }
   
    // RegEnum key and recurse...
   
    while (TRUE)
    {
        dwError = RegEnumKey(hkNode, 0, szKey, COUNTOF(szKey));
        if (dwError)
        {
            break;
        }
   
        dwError = RegNodeDelete(hkNode, szKey);
        if (dwError)
        {
            break;
        }
    }
   
    RegCloseKey(hkNode);
   
    if (ERROR_NO_MORE_ITEMS != dwError)
    {
        return (dwError);
    }
   
    return ( RegDeleteKey(hk, lpszKey) );
}


/////////////////////////////////////////////////////////////////////
//
// Name:     BaseExtFind
//
// Synopsis: Finds a string lpExt in pExtBase
//
// IN     lpszExt LPTSTR  extension string to look for
//                        may have a dot or not.
//
//                        lstrlen(lpszExt) <= EXTSIZ+1
//
// Return:   PEXT         NULL = none found
//
//
// Assumes:  pExtBase set up
//
// Effects:  lpszExt is cleaned up
//
// Notes:    pExt is returned, even if it is marked deleted
//
/////////////////////////////////////////////////////////////////////

PEXT
BaseExtFind(LPTSTR lpszExt)
{
    PEXT pExt;
    PEXT pExtNext;


    // Clean up lpszExt
    ExtClean(lpszExt);

    for (pExt = pExtBase; pExt; pExt = pExtNext)
    {
        pExtNext = pExt->next;

        if (!lstrcmpi(pExt->szExt, lpszExt))
        {
            return (pExt);
        }
    }

    return (NULL);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     FileAssociateErrorCheck
//
// Synopsis: Checks for error and prints one if it exists
//
// IN    hwnd       --    HWND to own dialog
// IN    idsTitle   --    IDS_ string for title
// IN    idsText    --    IDS_ string for text (operation)
//                        If 0L, none used (Not NULL!)
// IN    dwError    --    Error code
//
// Return:   VOID
//
// Assumes:
//
// Effects:  Puts up error dialog with exclmtn & OK button
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
FileAssociateErrorCheck(HWND hwnd,
   UINT idsTitle,
   UINT idsText,
   DWORD dwError)
{
    WCHAR szTitle[MAXTITLELEN];
    WCHAR szText[MAXMESSAGELEN];
    BOOL bNullString = TRUE;
   

    if (ERROR_SUCCESS == dwError)
    {
       return;
    }
   
    LoadString(hAppInstance, idsTitle, szTitle, COUNTOF(szTitle));
   
    if (idsText)
    {
        if (LoadString(hAppInstance, idsText, szText, COUNTOF(szText)))
        {
            bNullString = FALSE;
        }
    }
   
    FormatError(bNullString, szText, COUNTOF(szText), dwError);
    MessageBox(hwnd, szText, szTitle, MB_OK | MB_ICONSTOP);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     AssociateDlgInit
//
// Synopsis: Sets up the IDD_CLASSLIST and IDD_EXT list/combo
//           boxes and pExtBaseN, pFileTypeBase.
//
// IN        hDlg     HWND
// IN        lpszExt  LPTSTR   Extension edit control is set to this
//                             If NULL, not set
// IN        iSel     INT      Selection # to select in ClassList
//                             = -1 Do UpdateSelectionExt
//                             > # in listbox selects 0 index
//
// Return:   BOOL     TRUE = success
//
//
// Assumes:  hDlg points to AssociateFileDlg
//
// Effects:  pExtBase, pFileTypeBase, ClassList and ext boxes
//           szNone initialized.
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL
AssociateDlgInit(HWND hDlg, LPTSTR lpszExt, INT iSel)
{
   INT iItem;
   PEXT pExtBuf;
   PEXT pExtNext;
   PFILETYPE pFileType;
   INT iNum;

   LoadString(hAppInstance, IDS_ASSOCNONE, szNone, COUNTOF(szNone));

   // Read in the database

   if (!RegLoad()) {
      FileAssociateErrorCheck(hwndFrame, IDS_EXTTITLE, 0L,GetLastError());

      RegUnload();
      return FALSE;
   }

   //
   // Eliminate messy flicker
   //
   SendDlgItemMessage(hDlg, IDD_CLASSLIST, WM_SETREDRAW, FALSE, 0L);

   // Clear the boxes
   SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_RESETCONTENT, 0, 0L);
   SendDlgItemMessage(hDlg, IDD_EXT, CB_RESETCONTENT, 0, 0L);

   if (lpszExt) {
      ExtClean(lpszExt);
      SetDlgItemText(hDlg, IDD_EXT, lpszExt+1);
   }

   // Add all extensions into dropdown cb
   for (pExtBuf = pExtBase; pExtBuf; pExtBuf = pExtNext)
   {
      pExtNext = pExtBuf->next;

      // Only if not deleted

      if (!pExtBuf->bDelete)
      {
         CharLower(&pExtBuf->szExt[1]);
         iItem = (INT) SendDlgItemMessage(hDlg,IDD_EXT,
            CB_ADDSTRING,0,(LPARAM)&pExtBuf->szExt[1]);
      }
   }

   // Put in the entries in the listbox

   for (iNum = 0,pFileType=pFileTypeBase;
        pFileType;
        pFileType=pFileType->next, iNum++)
   {
      ClassListFileTypeAdd(hDlg, pFileType);
   }

   // Add the (None) entry at the beginning.
   SendDlgItemMessage(hDlg,IDD_CLASSLIST, LB_INSERTSTRING,0,(LPARAM)szNone);

   if (-1 == iSel) {
      UpdateSelectionExt(hDlg, FALSE);
   } else {
      SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_SETCURSEL,
         (iSel <= iNum) ? iSel: 0, 0L);
   }

   //
   // Eliminate messy flicker; now redraw
   //
   SendDlgItemMessage(hDlg, IDD_CLASSLIST, WM_SETREDRAW, TRUE, 0L);
   InvalidateRect(GetDlgItem(hDlg, IDD_CLASSLIST), NULL, TRUE);

   UpdateWindow(GetDlgItem(hDlg, IDD_CLASSLIST));

   return TRUE;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     ExtClean
//
// Synopsis: Cleans up ext by truncating trailing spaces and
//           prepending '.' if necessary.
//
// INOUT     lpszExt    --  extension to clean
//
// Return:   VOID
//
//
// Assumes:  lpszExt is a valid ext with enough space
//
// Effects:  lpszExt is turned into a dotted extension
//
// Notes:    "." is not a valid extension and will be transmuted
//           into "".
//
//           When "" is returned, two 0Ls are actually returned
//           so that &lpszExt[1] is also "".
//
/////////////////////////////////////////////////////////////////////

VOID
ExtClean(LPTSTR lpszExt)
{
    LPTSTR p;
    TCHAR szExt[EXTSIZ + 1];


    // Kill off trailing spaces
    for (p = &lpszExt[lstrlen(lpszExt)-1]; lpszExt<=p && CHAR_SPACE == *p; p--)
        ;

    *(++p) = CHAR_NULL;

    // Kill off leading '.'
    for (p = lpszExt; *p && CHAR_DOT == *p; p++)
        ;

    // If NULL then zero and exit.
    if (!*p)
    {
        lpszExt[0] = CHAR_NULL;
        lpszExt[1] = CHAR_NULL;
        return;
    }

    szExt[0]=CHAR_DOT;

    lstrcpy(&szExt[1], p);
    lstrcpy(lpszExt, szExt);

    return;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     AssociateFileWrite
//
// Synopsis: Writes out everything from the AssociateFileDlg
//           and adds new filetypes to pFileType data structure.
//
// IN        hDlg                   HWND FileDlgInfo window
// INOUT     pAssociateFileDlgInfo  --   data to write out
//
// Return:   DWORD error code
//
//
// Assumes:
//
// Effects:  Registry and pFileType
//
//           Except when DE_RETRY is returned, pAssociateFileDlgInfo.bRefresh
//           is set to true.  (Even in error, since we don't know how far
//           we got.)
//
//           In all other error cases, handles error message box.
//
// Notes:    If Adding (IDD_NEW) then pFileTypeBase is prepended
//           with the new item for AssociateDlgProc to LB_ADDSTRING
//           Flushes key
//
/////////////////////////////////////////////////////////////////////

DWORD
AssociateFileWrite(HWND hDlg, PASSOCIATEFILEDLGINFO pAssociateFileDlgInfo)
{
    PEXT pExt;
    PEXT pExtNext;
    DWORD dwError;
    DWORD dwLastError;


    dwError = FileTypeWrite( hDlg,
                             pAssociateFileDlgInfo,
                             HKEY_CLASSES_ROOT,
                             szNULL );
    if (DE_RETRY == dwError)
    {
        return (DE_RETRY);
    }

    FileAssociateErrorCheck( hDlg,
                             IDS_EXTTITLE,
                             IDS_FILETYPEADDERROR,
                             dwError );
    dwLastError = dwError;

    //
    // Traverse through pFileType and update everything
    //
    for (pExt = pExtBase; pExt; pExt = pExtNext)
    {
        pExtNext = pExt->next;

        if (pExt->bAdd)
        {
            //
            // !! LATER !!
            // Handle dwError better... What is the status of the
            // pExt backbone and bAdd/bDelete?
            //
            dwError = RegExtAdd( pAssociateFileDlgInfo->hDlg,
                                 HKEY_CLASSES_ROOT,
                                 pExt,
                                 pExt->pFileType );

            FileAssociateErrorCheck( hDlg,
                                     IDS_EXTTITLE,
                                     IDS_EXTADDERROR,
                                     dwError );

            if (dwError)
                dwLastError = dwError;

            //
            // Assume we hit something in the registry!
            //
            pAssociateFileDlgInfo->bRefresh = TRUE;
        }
        else if (pExt->bDelete)
        {
            dwError = RegExtDelete( pAssociateFileDlgInfo->hDlg,
                                    HKEY_CLASSES_ROOT,
                                    pExt );

            FileAssociateErrorCheck( hDlg,
                                     IDS_EXTTITLE,
                                     IDS_EXTDELERROR,
                                     dwError );

            if (dwError)
                dwLastError = dwError;

            //
            // Assume we hit something in the registry!
            //
            pAssociateFileDlgInfo->bRefresh = TRUE;
        }
    }

    RegFlushKey(HKEY_CLASSES_ROOT);

    return (dwLastError);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     ClassListFileTypeAdd
//
// Synopsis: Adds a pFileType to the IDD_CLASSLIST
//
// INOUT     hDlg        HWND     hWnd to modify
// IN        pFileType   --       item to add
//
// Return:   INT    Index of stored item.
//                  -1 = error
//
// Assumes:  IDD_CLASSLIST valid in hDlg, pFileType setup
//           Relies on the strict ordering of pFileType->lpszBuf
//
// Effects:  Listbox IDD_CLASSLIST
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

INT
ClassListFileTypeAdd(HWND hDlg, PFILETYPE pFileType)
{
    PTCHAR p;
    INT iItem;
    TCHAR c, c2;
    BOOL bInQuotes;


    p = &pFileType->lpszBuf[pFileType->uExe];

    //
    // add closing paren
    //
    for (iItem = 0, bInQuotes = FALSE; *p; p++, iItem++)
    {
        if (CHAR_SPACE == *p && !bInQuotes)
        {
            break;
        }
 
        if (CHAR_DQUOTE == *p)
        {
            bInQuotes = !bInQuotes;
        }
    }

    c = *p;
    c2 = *(p+1);

    *p = CHAR_CLOSEPAREN;
    *(p+1) = CHAR_NULL;

    pFileType->uExeSpace = iItem+pFileType->uExe;

    //
    // Remove null and add space
    //
    pFileType->lpszBuf[pFileType->uExe-2] = CHAR_SPACE;

    iItem = (INT)SendDlgItemMessage( hDlg,
                                     IDD_CLASSLIST,
                                     LB_ADDSTRING,
                                     0,
                                     (LPARAM)&pFileType->lpszBuf[pFileType->uDesc] );

    //
    // Restore paren to previous
    //
    *p = c;
    *(p + 1) = c2;

    pFileType->lpszBuf[pFileType->uExe - 2] = CHAR_NULL;

    SendDlgItemMessage( hDlg,
                        IDD_CLASSLIST,
                        LB_SETITEMDATA,
                        iItem,
                        (LPARAM) pFileType );

    return (iItem);
}


VOID
FileTypeFree(PFILETYPE pFileType)
{
    if (pFileType->lpszBuf)
    {
        LocalFree(pFileType->lpszBuf);
    }
    LocalFree(pFileType);
}


VOID
ExtFree(PEXT pExt)
{
    LocalFree(pExt);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     FileTypeAddString
//
// Synopsis: Adds a string to pFileType
//
// IN    pFileType   --     pFileType to modify
// IN    LPTSTR       --     string to add
// INOUT pcchOffset  PDWORD point to Offset to add to lpszBuf
//
//
// Return:   DWORD      Error code
//           pcchOffset Holds slot for next string
//
// Assumes:
//
// Effects:  pFileType->lpszBuf grown if nec, string added
//           pcchOffset holds next free location
//           (one PAST the written '\0')
//
// Notes:    One character past the current string is guaranteed to
//           be usable (i.e. lstrcat(pFileType->xxx, " ")) is ok.
//
/////////////////////////////////////////////////////////////////////

DWORD
FileTypeAddString(PFILETYPE pFileType, LPTSTR lpstr, PUINT pcchOffset)
{
    INT dwSize;
    INT dwLen;
   

    dwLen = lstrlen(lpstr) + 1;
    dwSize = pFileType->cchBufSiz - *pcchOffset;
   
    if (dwSize <= dwLen)
    {
        if (!FileTypeGrow(pFileType, pFileType->cchBufSiz + dwLen))
        {
            return ( GetLastError() );
        }
   
        dwSize = pFileType->cchBufSiz - *pcchOffset;
    }
   
    lstrcpy(&pFileType->lpszBuf[*pcchOffset], lpstr);
   
    *pcchOffset += dwLen;
   
    return (ERROR_SUCCESS);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ExtDupCheck
//
// Synopsis: Check for duplicate extensions
//
// INC       lpszExt   --  Extension to check
// INC       pExt      --  place to start
//
// Return:   BOOL      TRUE = dup
//                     FALSE = no dup
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
ExtDupCheck(LPTSTR lpszExt, PEXT pExt)
{
    for (; pExt; pExt = pExt->next)
    {
        if (!lstrcmpi(lpszExt, pExt->szExt))
        {
            return TRUE;
        }
    }

    return (FALSE);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     FileTypeDupIdentCheck
//
// Synopsis: Ensures an identifier is unique and legal
//
// IN     hDlg        --   Parent window for dialogs
// IN     uIDD_FOCUS  UINT Control to set focus to on retry
// INOUT  lpszIdent   --   Identifier to check.
//
//
// Return:   BOOL  TRUE  = unique
//                 FALSE = error displayed, focus set, try again.
//
// Assumes:  pFileTypeBase set up
//           lpszIdent has szFileManPrefix already
//
// Effects:  lpszIdent is modified:
//           # if szFileManPrefix incremented
//           BackSlashes are changed to colons
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL
FileTypeDupIdentCheck(HWND hDlg, UINT uIDD_FOCUS, LPTSTR lpszIdent)
{
   PFILETYPE pft2;
   PFILETYPE pft2Next;
   BOOL bDup;

   INT iCount = 1;
   PTCHAR p;

   //
   // First remove all illegal backslashes
   //
   for (p=lpszIdent; *p; p++) {
      if (CHAR_BACKSLASH == *p) {
         *p = CHAR_COLON;
      }
   }

   do {
      bDup = FALSE;

      for (pft2 = pFileTypeBase; pft2; pft2 = pft2Next)
      {
         pft2Next = pft2->next;

         if (!lstrcmpi(pft2->lpszBuf,lpszIdent))
         {
            //
            // Recreate new FileType
            // But if > 0xfff, we're dead.
            //

            if ( iCount > MAX_PREFIX )
            {
               MyMessageBox(hDlg, IDS_EXTTITLE, IDS_FILETYPEDUPDESCERROR,
                  MB_TASKMODAL|MB_OK|MB_ICONEXCLAMATION);

               // Set focus to IDD_DESC
               SetFocus(GetDlgItem(hDlg, uIDD_FOCUS));

               return TRUE;
            }
            else
            {
               wsprintf(szFileManPrefix, szFileManPrefixGen, iCount++);

               //
               // Copy the new number over;
               // Don't copy over the null terminator
               //
               StrNCpy(lpszIdent, szFileManPrefix, COUNTOF(szFileManPrefix) -1);

               //
               // Set flag to recheck
               //
               bDup = TRUE;

               break;
            }
         }
      }
   } while (bDup);

   return FALSE;
}




/////////////////////////////////////////////////////////////////////
//
// Name:     CommandWrite
//
// Synopsis: Writes out command when user creates assoc from 1st dlg
//
// IN   hDlg         --  Parent window for dialogs
// IN   lpszExt      --  extension to create
// IN   lpszCommand  --  Command.exe to associate with
//
// Return:   VOID
//
//
// Assumes:  lpszCommand can grow at least 6 characters
//
// Effects:  Creates registry entries for new file type and extension
//
//
// Notes:    Registry won't be written correctly unless the lstrlen
//           of lpszCommand < cchCommand - ".exe %1" since we may
//           need to append this.
//
/////////////////////////////////////////////////////////////////////

DWORD
CommandWrite(HWND hDlg, LPTSTR lpszExt, LPTSTR lpszCommand)
{
   TCHAR szIdentBuf[DESCSIZ+COUNTOF(szFileManPrefix)+COUNTOF(szShellOpenCommand)];
   DWORD dwError;
   UINT i;
   DWORD cbData;
   LPTSTR p, lpszIdent;

   TCHAR szCommand[COMMANDSIZ];

   lstrcpy(szIdentBuf, szFileManPrefix);

   lstrcat(szIdentBuf, lpszCommand);

   //
   // Clean up ident
   //
   for(lpszIdent = szIdentBuf+lstrlen(szIdentBuf); lpszIdent != szIdentBuf;
      lpszIdent--) {

      if (CHAR_COLON == *lpszIdent || CHAR_BACKSLASH == *lpszIdent) {
         lpszIdent++;
         break;
      }
   }

   p = StrChrQuote(lpszIdent, CHAR_SPACE);
   if (p) {
      *p = CHAR_NULL;
   }

   //
   // Write out \x\shell\open\command
   //
   if (FileTypeDupIdentCheck(hDlg, IDD_COMMAND, lpszIdent)) {
      dwError = DE_RETRY;
      goto Error;
   }

   i = lstrlen(lpszIdent);
   lstrcat(lpszIdent, szShellOpenCommand);

   //
   // Beautify lpszCommand:  Add %1 if there isn't one,
   // add .exe if that's missing too.
   // Assumes space at end!
   //

   if (!*GetExtension(lpszCommand)) {

      //
      // Be careful adding .exe to {c:\<directory>\<file> -p}
      //

      p = StrChrQuote(lpszCommand, CHAR_SPACE);

      if (p) {

         *p = CHAR_NULL;

         lstrcpy(szCommand, lpszCommand);
         lstrcat(szCommand, szDotEXE);

         *p = CHAR_SPACE;
         lstrcat(szCommand, p);

         lpszCommand = szCommand;

      } else {

         lstrcat(lpszCommand, szDotEXE);
      }
   }

   lstrcat(lpszCommand, szSpacePercentOne);

   cbData = ByteCountOf(lstrlen(lpszCommand));

   dwError = RegSetValue(HKEY_CLASSES_ROOT,
      lpszIdent,
      REG_SZ,
      lpszCommand,
      cbData);

   if (dwError)
      goto Error;

   p = GenerateFriendlyName(lpszCommand);

   cbData = ByteCountOf(lstrlen(p));

   lpszIdent[i] = CHAR_NULL;
   dwError = RegSetValue(HKEY_CLASSES_ROOT,
      lpszIdent,
      REG_SZ,
      p,
      cbData);

   if (dwError)
      goto Error;

   //
   // Now write extension
   //

   dwError = RegSetValue(HKEY_CLASSES_ROOT,
      lpszExt,
      REG_SZ,
      lpszIdent,
      ByteCountOf(lstrlen(lpszIdent)));

Error:

   if (dwError && DE_RETRY != dwError) {

      FileAssociateErrorCheck(hDlg, IDS_EXTTITLE, 0L, dwError);
   }
   return dwError;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     ExtDelink
//
// Synopsis: Takes a pExt out of the pExt->pftNext chain
//
// INC       pExt  --  Ext to delink
//
// Return:   VOID
//
// Assumes:  Stable data structure
//
// Effects:  pExtBase structure
//
//
// Notes:    Does not free pExt or modify pExt at all
//           Does not remove pExt from pExtBase backbone.
//
/////////////////////////////////////////////////////////////////////

VOID
ExtDelink(PEXT pExt)
{
    PEXT pExt2;


    /*
     *  Update old pFileType->pExt if the one we are deleting
     *  happens to be the first one.
     */
    if (pExt->pFileType->pExt == pExt) 
    {
        pExt->pFileType->pExt = pExt->pftNext;
    }
    else
    {
        /*
         *  Not first pftExt so relink the previous one to
         *  the one following the current one.
         */
        pExt2 = pExt->pFileType->pExt;
        while (pExt2->pftNext != pExt)
        {
             pExt2 = pExt2->pftNext;
        }

        pExt2->pftNext = pExt->pftNext;
    }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     ExtLink
//
// Synopsis: Links pExt to pFileType by PREPENDING
//
// INOUT     pExt      --  object to insert
// INOUT     pFileType --  associated filetype
//
// Return:   VOID
//
// Assumes:
//
// Effects:  pExt->pFileType
//           pExt->pftNext
//           pFileType->pExt chain [->pftNext]
//
//
// Notes:    Does not modify pExt->pftOrig
//           or pExt->{bAdd,bDelete,bMach,bHider} fields
//
/////////////////////////////////////////////////////////////////////

VOID
ExtLink(PEXT pExt, PFILETYPE pFileType)
{
    pExt->pFileType = pFileType;

    pExt->pftNext = pFileType->pExt;
    pFileType->pExt = pExt;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     StrChrQuote
//
// Synopsis: Find given character in given string that isn't
//           contained in quotes.
//
// IN        lpszString - pointer to string
//           c          - character to find
//
// Return:   LPTSTR  Pointer to given character if found;
//                   otherwise NULL
//
// Assumes:
//
// Effects:  
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

LPTSTR
StrChrQuote(LPTSTR lpszString, TCHAR c)
{
    BOOL bInQuotes = FALSE;


    while (*lpszString)
    {
        if (*lpszString == c && !bInQuotes)
        {
            return (lpszString);
        }

        if (CHAR_DQUOTE == *lpszString)
        {
            bInQuotes = !bInQuotes;
        }

        lpszString++;
    }

    return (NULL);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     GenerateFriendlyName
//
// Synopsis: Generates friendly name based on command
//
// IN        -- lpszCommand
//
//
//
//
// Return:   LPTSTR  Pointer to friendly part
//
//
// Assumes:
//
// Effects:  Trashes lpszCommand
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

LPTSTR
GenerateFriendlyName(LPTSTR lpszCommand)
{
    LPTSTR p, p2;


    /*
     *  Tighten up the friendly name by stripping down lpszCommand.
     *  Take last file spec and eliminate ext.
     */
    for (p = lpszCommand + lstrlen(lpszCommand); p != lpszCommand; p--)
    {
        if (CHAR_COLON == *p || CHAR_BACKSLASH == *p)
        {
            p++;
            break;
        }
    }

    p2 = p;
    while ( (*p2) &&
            (CHAR_DOT != *p2) &&
            (CHAR_SPACE != *p2) &&
            (CHAR_DQUOTE != *p2) )
    {
        p2++;
    }

    *p2 = CHAR_NULL;

    return (p);
}

