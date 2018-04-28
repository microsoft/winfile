/********************************************************************

   lfn.c

   This file contains code that combines winnet long filename API's and
   the DOS INT 21h API's into a single interface.  Thus, other parts of
   Winfile call a single piece of code with no worries about the
   underlying interface.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"                            // lfn includes
#include "wfcopy.h"

BOOL IsFATName(LPTSTR pName);


/* WFFindFirst -
 *
 * returns:
 *      TRUE for success - lpFind->fd,hFindFileset,attrFilter set.
 *      FALSE for failure
 *
 *  Performs the FindFirst operation and the first WFFindNext.
 */

BOOL
WFFindFirst(
   LPLFNDTA lpFind,
   LPTSTR lpName,
   DWORD dwAttrFilter)
{
   INT    nLen;
   LPTSTR pEnd;

   //
   // We OR these additional bits because of the way DosFindFirst works
   // in Windows. It returns the files that are specified by the attrfilter
   // and ORDINARY files too.
   //

   PVOID oldValue;
   Wow64DisableWow64FsRedirection(&oldValue);

   if ((dwAttrFilter & ~(ATTR_DIR | ATTR_HS)) == 0)
   {
	   // directories only (hidden or not)
	   lpFind->hFindFile = FindFirstFileEx(lpName, FindExInfoStandard, &lpFind->fd, FindExSearchLimitToDirectories, NULL, 0);
   }
   else
   {
	   // normal case: directories and files
	   lpFind->hFindFile = FindFirstFile(lpName, &lpFind->fd);
   }

   if (lpFind->hFindFile == INVALID_HANDLE_VALUE) {
       lpFind->err = GetLastError();
   } else {
       lpFind->err = 0;
   }

   // add in attr_* which we want to include in the match even though the caller didn't request them.
   dwAttrFilter |= ATTR_ARCHIVE | ATTR_READONLY | ATTR_NORMAL | ATTR_REPARSE_POINT |
	   ATTR_TEMPORARY | ATTR_COMPRESSED | ATTR_ENCRYPTED | ATTR_NOT_INDEXED;

   lpFind->fd.dwFileAttributes &= ATTR_USED;

   Wow64RevertWow64FsRedirection(oldValue);

   //
   // Keep track of length
   //
   nLen = lstrlen(lpName);
   pEnd = &lpName[nLen-1];

   while (CHAR_BACKSLASH != *pEnd) {
      pEnd--;
      nLen--;
   }

   lpFind->nSpaceLeft = MAXPATHLEN-nLen-1;

   if (lpFind->hFindFile != INVALID_HANDLE_VALUE) {
      lpFind->dwAttrFilter = dwAttrFilter;
      if ((~dwAttrFilter & lpFind->fd.dwFileAttributes) == 0L ||
         WFFindNext(lpFind)) {
         return(TRUE);
      } else {
         WFFindClose(lpFind);
         return(FALSE);
      }
   } else {
      return(FALSE);
   }
}



/* WFFindNext -
 *
 *  Performs a single file FindNext operation.  Only returns TRUE if a
 *  file matching the dwAttrFilter is found.  On failure WFFindClose is
 *  called.
 */
BOOL
WFFindNext(LPLFNDTA lpFind)
{
	PVOID oldValue;
	Wow64DisableWow64FsRedirection(&oldValue);
	
   while (FindNextFile(lpFind->hFindFile, &lpFind->fd)) {

	  lpFind->fd.dwFileAttributes &= ATTR_USED;
   
      //
      // only pick files that fit attr filter
      //
      if ((lpFind->fd.dwFileAttributes & ~lpFind->dwAttrFilter) != 0)
         continue;

      //
      // Don't allow viewage of files > MAXPATHLEN
      //
      if (lstrlen(lpFind->fd.cFileName) > lpFind->nSpaceLeft) {

         if (!lpFind->fd.cAlternateFileName[0] ||
            lstrlen(lpFind->fd.cAlternateFileName) > lpFind->nSpaceLeft) {

            continue;
         }

         //
         // Force longname to be same as shortname
         //
         lstrcpy(lpFind->fd.cFileName, lpFind->fd.cAlternateFileName);
      }

	  Wow64RevertWow64FsRedirection(oldValue);
      lpFind->err = 0;
      return TRUE;
   }

   lpFind->err = GetLastError();

   Wow64RevertWow64FsRedirection(oldValue);
   return(FALSE);
}


/* WFFindClose -
 *
 *  performs the find close operation
 */
BOOL
WFFindClose(LPLFNDTA lpFind)
{
    BOOL bRet;

//    ASSERT(lpFind->hFindFile != INVALID_HANDLE_VALUE);

// This section WAS #defined DBG, but removed
// Basically, when copying a directory into ITS subdirectory, we detect
// an error.  Since the error is detected late (after the file handle
// is closed), we jump to error code that pops up a message, then tries
// to close the same handle.  This cause the a bomb.  By un #define DBG ing
// this code, we allow multiple file closes.  This is sloppy, but works.

// A better solution is to put another flag in the error code "ret" in
// WFMoveCopyDriver right after it calls GetNextPair.

    if (lpFind->hFindFile == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    bRet = FindClose(lpFind->hFindFile);

// This section WAS #defined DBG, but removed
    lpFind->hFindFile = INVALID_HANDLE_VALUE;

    return(bRet);
}




/* WFIsDir
 *
 *  Determines if the specified path is a directory
 */
BOOL
WFIsDir(LPTSTR lpDir)
{
   DWORD attr = GetFileAttributes(lpDir);

   if (attr == INVALID_FILE_ATTRIBUTES)
      return FALSE;

   if (attr & ATTR_DIR)
      return TRUE;

   return FALSE;
}

/* GetNameType -
 *
 *  Shell around LFNParse.  Classifies name.
 *
 *  NOTE: this should work on unqualified names.  currently this isn't
 *        very useful.
 */
DWORD
GetNameType(LPTSTR lpName)
{
   if (CHAR_COLON == *(lpName+1)) {
      if (!IsLFNDrive(lpName))
         return FILE_83_CI;
   } else if(IsFATName(lpName))
      return FILE_83_CI;

   return(FILE_LONG);
}

#if 0
BOOL
IsLFNDrive(LPTSTR szDrive)
{
   DRIVE drive = DRIVEID(szDrive);

   if (CHAR_COLON == szDrive[1]) {

      U_VolInfo(drive);

      if (GETRETVAL(VolInfo,drive))
         return FALSE;

      return aDriveInfo[drive].dwMaximumComponentLength == MAXDOSFILENAMELEN-1 ?
         FALSE :
         TRUE;

   } else {

      DWORD dwVolumeSerialNumber;
      DWORD dwMaximumComponentLength;
      DWORD dwFileSystemFlags;

      INT i;
      LPTSTR p;

      TCHAR szRootPath[MAXPATHLEN];

      if (ISUNCPATH(szDrive)) {

         lstrcpy(szRootPath, szDrive);

         //
         // Stop at "\\foo\bar"
         //
         for (i=0, p=szRootPath+2; *p && i<2; p++) {

            if (CHAR_BACKSLASH == *p)
               i++;
         }

         switch (i) {
         case 0:

            return FALSE;

         case 1:

            if (lstrlen(szRootPath) < MAXPATHLEN-2) {

               *p = CHAR_BACKSLASH;
               *(p+1) = CHAR_NULL;

            } else {

               return FALSE;
            }
            break;

         case 2:

            *p = CHAR_NULL;
            break;
         }
      }

      if (GetVolumeInformation( szRootPath,
                                NULL,
                                0,
                                &dwVolumeSerialNumber,
                                &dwMaximumComponentLength,
                                &dwFileSystemFlags,
                                NULL,
                                0 ))
      {
         if (dwMaximumComponentLength == MAXDOSFILENAMELEN - 1)
            return FALSE;
         else
            return TRUE;
      }
      return FALSE;
   }
}
#endif

BOOL
IsFATName(
   IN  LPTSTR FileName
)
/*++

Routine Description:

   This routine computes whether or not the given file name would
   be appropriate under DOS's 8.3 naming convention.

Arguments:

   FileName    - Supplies the file name to check.

Return Value:

   FALSE   - The supplied name is not a DOS file name.
   TRUE    - The supplied name is a valid DOS file name.

--*/
{
   ULONG   i, n, name_length, ext_length;
   BOOLEAN dot_yet;
   PTUCHAR p;

   n = lstrlen(FileName);
   p = (PTUCHAR) FileName;
   name_length = n;
   ext_length = 0;

   if (n > 12) {
      return FALSE;
   }

   dot_yet = FALSE;
   for (i = 0; i < n; i++) {

      if (p[i] < 32) {
         return FALSE;
      }

      switch (p[i]) {
      case CHAR_STAR:
      case CHAR_QUESTION:
      case CHAR_SLASH:
      case CHAR_BACKSLASH:
      case CHAR_PIPE:
      case CHAR_COMMA:
      case CHAR_SEMICOLON:
      case CHAR_COLON:
      case CHAR_PLUS:
      case CHAR_EQUAL:
      case CHAR_LESS:
      case CHAR_GREATER:
      case CHAR_OPENBRACK:
      case CHAR_CLOSEBRACK:
      case CHAR_DQUOTE:
         return FALSE;

      case CHAR_DOT:
         if (dot_yet) {
            return FALSE;
         }

         dot_yet = TRUE;
         name_length = i;
         ext_length = n - i - 1;
         break;
      }
   }

   if (!name_length) {
      return dot_yet && n == 1;
   }

   if (name_length > 8 || p[name_length - 1] == CHAR_SPACE) {
      return FALSE;
   }

   if (!ext_length) {
      return !dot_yet;
   }

   if (ext_length > 3 || p[name_length + 1 + ext_length - 1] == CHAR_SPACE) {
      return FALSE;
   }

   return TRUE;
}


BOOL
IsLFN(LPTSTR pName)
{
    return !IsFATName(pName);
}

// LFNMergePath
//
// IN: lpMask = path with wildcard to be expanded (relative or full)
//              NonZero.
//     lpFile = non-wildcarded filename (filespec only or "\" for root)
//
// OUT: True (no error checking)
//
// Note: This is not the same as LFNParse, since here, lpMask can be an
//       absolute path, while lpFile is almost always a file spec.
//       (or \).  LFNParse assumes that lpFile a file path.
//       You may call LFNParse, and this will work since lpFile
//       just happens to be a filespec rather than a fully qualified path.
//       LFNParse will die lpFile is fully qualified.

BOOL
LFNMergePath(LPTSTR lpMask, LPTSTR lpFile)
{
   TCHAR szT[MAXPATHLEN*2];
   INT iResStrlen;

   //
   // Get the directory portion (from root to parent) of the destination.
   // (  a:\joe\martha\wilcox.*  ->  a:\joe\martha\ )
   //
   lstrcpy( szT, lpMask );
   RemoveLast( szT );

   //
   // Add a blackslash if there isn't one already.
   //
   AddBackslash(szT);

   // hack fix: Normally, I_LFNEditName will return a:\xxxx\\. when lpFile
   // is "\\" (C-style string, so the first \ is an escape char).
   // Only do the file masking if lpFile is NOT the root directory.
   // If it is, the return value is a:\xxxx\ which is what is expected
   // when a root is merged.

   if (!( CHAR_BACKSLASH == lpFile[0] && CHAR_NULL == lpFile[1] )) {

      iResStrlen = lstrlen( szT );

      I_LFNEditName(lpFile,                  // jack.bat
         FindFileName( lpMask ),             // *.*
         szT + iResStrlen,                   // right after "a:\more\beer\"
         COUNTOF(szT) - iResStrlen);

      // If there is a trailing '.', always but always kill it.

      iResStrlen = lstrlen( szT );
      if ((iResStrlen != 0) && CHAR_DOT == szT[iResStrlen - 1])
         szT[ iResStrlen-1 ] = CHAR_NULL;
   }

   lstrcpy(lpMask, szT);
   return TRUE;
}

/* WFCopy
 *
 *  Copies files
 */
DWORD
WFCopy(LPTSTR pszFrom, LPTSTR pszTo)
{
    DWORD dwRet;
    TCHAR szTemp[MAXPATHLEN];

    Notify(hdlgProgress, IDS_COPYINGMSG, pszFrom, pszTo);

    if (CopyFile(pszFrom, pszTo, FALSE))
    {
        ChangeFileSystem(FSC_CREATE, pszTo, NULL);
        dwRet = 0;
    }
    else
    {
        dwRet = GetLastError();
        if (dwRet == ERROR_INVALID_NAME)
        {
            //
            //  Try copying without the file name in the TO field.
            //  This is for the case where it's trying to copy to a print
            //  share.  CopyFile fails if the file name is tacked onto the
            //  end in the case of printer shares.
            //
            lstrcpy(szTemp, pszTo);
            RemoveLast(szTemp);
            if (CopyFile(pszFrom, szTemp, FALSE))
            {
                ChangeFileSystem(FSC_CREATE, szTemp, NULL);
                dwRet = 0;
            }

            // else ... use the original dwRet value.
        }
    }

    return (dwRet);
}

/* WFRemove
 *
 *  Deletes files
 */
DWORD
WFRemove(LPTSTR pszFile)
{
   DWORD dwRet;

   dwRet = FileRemove(pszFile);
   if (!dwRet)
      ChangeFileSystem(FSC_DELETE,pszFile,NULL);

   return dwRet;
}

/* WFMove
 *
 *  Moves files on a volume
 */
DWORD
WFMove(LPTSTR pszFrom, LPTSTR pszTo, PBOOL pbErrorOnDest, BOOL bSilent)
{
   DWORD dwRet;

   *pbErrorOnDest = FALSE;
   dwRet = FileMove(pszFrom,pszTo, pbErrorOnDest, bSilent);

   if (!dwRet)
      ChangeFileSystem(FSC_RENAME,pszFrom,pszTo);

   return dwRet;
}


