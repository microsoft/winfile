/********************************************************************

   wfinfo.c

   Handles caching and refreshing of drive information.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "wnetcaps.h"
#include <commctrl.h>


#define U_HEAD(type) \
   VOID \
   U_##type (DRIVE drive) { \
   PDRIVEINFO pDriveInfo = &aDriveInfo[drive];

//
// Warning:
//
// Never jump into or out of a IF_READ END_IF or ENTER_MODIFY EXIT_MODIFY
// blocks for obvious critical section reasons.
//
#define IF_READ(type) \
   if (!pDriveInfo->s##type.bValid || pDriveInfo->s##type.bRefresh) {

#define ENTER_MODIFY(type) \
   EnterCriticalSection(&CriticalSectionInfo##type); \
   if (!pDriveInfo->s##type.bValid || pDriveInfo->s##type.bRefresh) {

#define EXIT_MODIFY(type) \
      pDriveInfo->s##type.bValid   = TRUE; \
      pDriveInfo->s##type.bRefresh = FALSE; \
   } \
   LeaveCriticalSection(&CriticalSectionInfo##type);

#define END_IF(type)     }

#define SET_RETVAL(type, val) \
   pDriveInfo->s##type.dwRetVal = (val)

#define U_CLOSE(type) \
   return; }

VOID NetCon_UpdateLines(DRIVE drive, DWORD dwType);
INT  UpdateDriveListWorker(VOID);


CRITICAL_SECTION CriticalSectionUpdate;

//
// Translation table from ALTNAME -> WNFMT_*
//
DWORD adwAltNameTrans[MAX_ALTNAME] = {
   WNFMT_MULTILINE,           // Must match ALTNAME_MULTI
   WNFMT_ABBREVIATED          // Must match ALTNAME_SHORT
};

//
// In characters
//
#define REMOTE_DEFAULT_SIZE (64-DRIVE_INFO_NAME_HEADER)


//
// Initialize/destroy Info handler
//
VOID
M_Info(VOID)
{
   InitializeCriticalSection(&CriticalSectionUpdate);
}

VOID
D_Info(VOID)
{
   DeleteCriticalSection(&CriticalSectionUpdate);
}

U_HEAD(Type)

   TCHAR szDrive[] = SZ_ACOLONSLASH;
   UINT uType;

   DRIVESET(szDrive, drive);

   IF_READ(Type)
      uType = GetDriveType(szDrive);

      ENTER_MODIFY(Type)

         pDriveInfo->uType = uType;

      EXIT_MODIFY(Type)

   END_IF(Type)

U_CLOSE(Type)

U_HEAD(Space)

   LARGE_INTEGER qFreeSpace;
   LARGE_INTEGER qTotalSpace;

   IF_READ(Space)
      GetDiskSpace(drive, &qFreeSpace, &qTotalSpace);

      ENTER_MODIFY(Space)

         aDriveInfo[drive].qFreeSpace = qFreeSpace;
         aDriveInfo[drive].qTotalSpace= qTotalSpace;

      EXIT_MODIFY(Space)

   END_IF(Space)

U_CLOSE(Space)


// Must leave DRIVE_INFO_NAME_HEADER (4) characters header free!

U_HEAD(VolInfo)

   TCHAR szVolName[COUNTOF(pDriveInfo->szVolNameMinusFour)-4];
   DWORD dwVolumeSerialNumber;
   DWORD dwMaximumComponentLength;
   DWORD dwFileSystemFlags;
   TCHAR szFileSysName[COUNTOF(pDriveInfo->szFileSysName)];
   TCHAR szTemp[MAX_FILESYSNAME];

   DWORD dwRetVal;

   IF_READ(VolInfo)
      dwRetVal = FillVolumeInfo( drive,
                                 szVolName,
                                 &dwVolumeSerialNumber,
                                 &dwMaximumComponentLength,
                                 &dwFileSystemFlags,
                                 szFileSysName );

      ENTER_MODIFY(VolInfo)

         SET_RETVAL(VolInfo, dwRetVal);

         lstrcpy(pDriveInfo->szVolNameMinusFour+4, szVolName);
         pDriveInfo->dwVolumeSerialNumber = dwVolumeSerialNumber;
         pDriveInfo->dwMaximumComponentLength = dwMaximumComponentLength;
         pDriveInfo->dwFileSystemFlags = dwFileSystemFlags;

         lstrcpy(pDriveInfo->szFileSysName, szFileSysName);
         if (dwFileSystemFlags & FS_VOL_IS_COMPRESSED)
         {
             /*
              *  Drive is compressed, so get the "compressed" string.
              */
             LoadString( hAppInstance,
                         IDS_DRIVE_COMPRESSED,
                         szTemp,
                         COUNTOF(szTemp) );

             /*
              *  Append the "compressed" string to the file system name.
              */
             lstrcat( pDriveInfo->szFileSysName, szTemp );
         }

         pDriveInfo->dwVolNameMax = lstrlen(szVolName);

      EXIT_MODIFY(VolInfo)

   END_IF(VolInfo)

U_CLOSE(VolInfo)


/////////////////////////////////////////////////////////////////////
//
// Name:     NetCon
//
// Synopsis: Handles all WNetGetConnection2/FMT information
//
// Assumes:  Never called for remembered connections unless an active
//           connection has taken over a remembered one.
//           (remembered defined as bRemembered bit)
//
// Effects:  lpConnectInfo
//           dwConnectInfoMax
//           lpszRemoteName{Short,Multi}MinusFour
//           dwRemoteName{Short,Multi}Max
//           dwLines
//           dwAltNameError
//
// Notes:    The status of NetCon is true iff WNetGetConnection2.
//           Value of dwAltNameError is independent of NetCon status.
//
// Status state:
//
// >> bRemembered == 1
//
//    Nothing changes
//
// >> ERROR_*
//
//    No strings are valid.
//
/////////////////////////////////////////////////////////////////////


U_HEAD(NetCon)

   DWORD dwSize;
   WNET_CONNECTIONINFO * lpConnectInfo = pDriveInfo->lpConnectInfo;

   TCHAR szDrive[] = SZ_ACOLON;
   DWORD dwRetVal;

   //
   // If not a remote drive, just return.
   //
   if (!IsRemoteDrive(drive))
      goto DoneSafe;

   if (!WAITNET_LOADED) {
      SET_RETVAL(NetCon, ERROR_DLL_INIT_FAILED);
      goto DoneSafe;
   }

   //
   // If remembered connection, simply validate and return
   //
   if (pDriveInfo->bRemembered)
      goto DoneSafe;

   DRIVESET(szDrive,drive);

   IF_READ(NetCon)

      ENTER_MODIFY(NetCon)

         // If error, zero it out!
         // DRIVE_INFO_NAME_HEADER characters before string must be allocated!

         dwSize = pDriveInfo->dwConnectInfoMax;

         if (!dwSize)
            dwSize = REMOTE_DEFAULT_SIZE;

         if (!lpConnectInfo) {
Retry:
            lpConnectInfo = (WNET_CONNECTIONINFO *) LocalAlloc(LPTR, dwSize);
            pDriveInfo->lpConnectInfo = lpConnectInfo;
         }

         if (!lpConnectInfo) {

            pDriveInfo->dwConnectInfoMax = 0;
            SET_RETVAL(NetCon,ERROR_NOT_ENOUGH_MEMORY);

            //
            // Go ahead and validate
            //

            goto Done;
         }

         pDriveInfo->dwConnectInfoMax = dwSize;

         dwRetVal = WNetGetConnection2(szDrive, lpConnectInfo, &dwSize);

         if (ERROR_MORE_DATA == dwRetVal) {

            LocalFree((HLOCAL)lpConnectInfo);
            goto Retry;
         }

         SET_RETVAL(NetCon,dwRetVal);

         //
         // Now get the multiline version
         //
         NetCon_UpdateAltName(drive, dwRetVal);

Done:

   EXIT_MODIFY(NetCon)

   END_IF(NetCon)
DoneSafe:
U_CLOSE(NetCon)


D_PROTO(NetCon)
{
   INT i;
   DRIVE drive;
   PDRIVEINFO pDriveInfo;

   for (drive = 0, pDriveInfo = aDriveInfo;
        drive < MAX_DRIVES;
        drive++, pDriveInfo++)
   {
      if (pDriveInfo->lpConnectInfo)
      {
         LocalFree((HLOCAL)pDriveInfo->lpConnectInfo);
      }

      for (i = 0; i < MAX_ALTNAME; i++)
      {
         if (pDriveInfo->lpszRemoteNameMinusFour[i])
         {
            LocalFree((HLOCAL)pDriveInfo->lpszRemoteNameMinusFour[i]);
         }
      }
   }
   D_Destroy(NetCon);
}


INT
NetCon_UpdateAltName(DRIVE drive, DWORD dwRetVal)
{
   PDRIVEINFO pDriveInfo = &aDriveInfo[drive];
   LPTSTR lpszBuf;
   DWORD dwSize;
   WNET_CONNECTIONINFO * lpConnectInfo = pDriveInfo->lpConnectInfo;
   DWORD i;

   for (i = 0; i < MAX_ALTNAME; i++) {

      //
      // If dwRetVal is in error state,
      // fail for all.
      //
      if (dwRetVal)
         break;

      lpszBuf = pDriveInfo->lpszRemoteNameMinusFour[i];
      dwSize = pDriveInfo->dwRemoteNameMax[i];

      if (!dwSize)
         dwSize = REMOTE_DEFAULT_SIZE;

      if (!lpszBuf) {
Retry:
         lpszBuf = (LPTSTR) LocalAlloc(LPTR, ByteCountOf(dwSize + DRIVE_INFO_NAME_HEADER));
      }


      pDriveInfo->lpszRemoteNameMinusFour[i] = lpszBuf;

      if (!lpszBuf) {

         pDriveInfo->dwRemoteNameMax[i]=0;

         dwRetVal = ERROR_NOT_ENOUGH_MEMORY;
         goto Done;
      }

      dwRetVal = WNetFormatNetworkNameW(lpConnectInfo->lpProvider,
         lpConnectInfo->lpRemoteName,
         lpszBuf + DRIVE_INFO_NAME_HEADER,
         &dwSize,
         adwAltNameTrans[i],
         cchDriveListMax);

      if (ERROR_MORE_DATA == dwRetVal) {

         //
         // If need more space, free buffer and retry
         // (dwSize is updated by WNetFormatNetworkName)
         //

         LocalFree((HLOCAL)lpszBuf);
         goto Retry;
      }

      if (dwRetVal) {
         break;
      }

      NetCon_UpdateLines(drive, i);
   }
Done:
   if (dwRetVal)
   {
      //
      // Set everything to 1!
      //
      for (i = 0; i < MAX_ALTNAME; i++)
      {
         pDriveInfo->dwLines[i] = 1;
      }
   }
   pDriveInfo->dwAltNameError = dwRetVal;
   return dwRetVal;
}



VOID
NetCon_UpdateLines(DRIVE drive, DWORD dwType)
{
   LPTSTR lpNext;
   DWORD dwLines=0;

   //
   // Scan for the number of \n in the text
   //

   lpNext = aDriveInfo[drive].lpszRemoteNameMinusFour[dwType] +
      DRIVE_INFO_NAME_HEADER;

   do
   {
      dwLines++;
      lpNext = StrChr(lpNext, CHAR_NEWLINE);
   } while (lpNext++);

   aDriveInfo[drive].dwLines[dwType] = dwLines;
}



/////////////////////////////////////////////////////////////////////
//
// Doc implementation
//
// Fixed size hashing function:
// Hashing on first char, first DOCBUCKETMAX bits
//
// 6 buckets wasted.
//
/////////////////////////////////////////////////////////////////////

//
// Warning: DOCBUCKETMAXBIT can't go past the number of bits in
// lpszExt[0].
//

#define DOCBUCKETMAXBIT 5
#define DOCBUCKETMAX (1 << DOCBUCKETMAXBIT)

#define DOCHASHFUNC(x) (x[0] & ~(~0 << DOCBUCKETMAXBIT))

struct _DOC_BUCKET {
   PDOCBUCKET next;
   TCHAR szExt[EXTSIZ];
   HICON hIcon;
   LPTSTR lpszFI;
} DOCBUCKET;


/////////////////////////////////////////////////////////////////////
//
// Name:     DocConstruct
//
// Synopsis: Creates and initializes Doc structure for IsDocument
//
// IN        VOID
//
// Return:   PPDOCBUCKET or NULL
//
// Assumes:
//
// Effects:
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

PPDOCBUCKET
DocConstruct(VOID)
{
   return (PPDOCBUCKET) LocalAlloc(LPTR, sizeof(PDOCBUCKET)*DOCBUCKETMAX);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     DocDestruct
//
// Synopsis: Frees doc structure
//
// INC       PPDOCBUCKET -- Doc structure to free
//
// Return:   VOID
//
// Assumes:
//
// Effects:  PDOCBUCKET is destroyed
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
DocDestruct(PPDOCBUCKET ppDocBucket)
{
   INT i;
   PDOCBUCKET pDocBucket;
   PDOCBUCKET pDocBucketNext;

   if (!ppDocBucket)
      return;

   for(i=0; i<DOCBUCKETMAX; i++) {

      for(pDocBucket = ppDocBucket[i]; pDocBucket;
         pDocBucket=pDocBucketNext) {

         pDocBucketNext = pDocBucket->next;
       	 DestroyIcon(pDocBucket->hIcon);
         LocalFree((HLOCAL)pDocBucket->lpszFI);
         LocalFree((HLOCAL)pDocBucket);
      }
   }
   LocalFree(ppDocBucket);
}


/////////////////////////////////////////////////////////////////////
//
// Name:     RemoveEndQuote
//
// Synopsis: Removes the quote at the end of the extension string.
//
// INC       lpszExt     -- Extension string
//
// Return:   Nothing
//
// Assumes:  No leading quote.  Will remove all quotes in the string
//           starting from the end of the string.
//
// Effects:  lpszExt is modified
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
RemoveEndQuote(
    LPTSTR lpszExt)
{
    LPTSTR ptr;

    if (lpszExt)
    {
        ptr = lpszExt + (lstrlen(lpszExt) - 1);
        while ((ptr >= lpszExt) && (*ptr == CHAR_DQUOTE))
        {
            *ptr = CHAR_NULL;
            ptr--;
        }
    }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     DocInsert
//
// Synopsis: Inserts an extension into a pDocBucket structure
//
// INOUTC    ppDocBucket  --  Doc struct to add to
// INOUTC    lpszExt      --  Extension to add
//
// Return:   INT   -1  Item already exists
//                 0   Error
//                 1   Successfully added
//
// Assumes:
//
// Effects:  ppDocBucket is updated
//           lpszExt is lowercased.
//
//
// Notes:    Stores everything in lowercase
//
/////////////////////////////////////////////////////////////////////

INT
DocInsert(PPDOCBUCKET ppDocBucket,
         LPTSTR lpszExt,
         LPTSTR lpszFileIcon)
{
   PDOCBUCKET pDocBucket;
   INT iBucket;
   TCHAR szExt[EXTSIZ];


   //
   // Only allow certain lengths; if invalid ppDocBucket, fail
   //
   if (lstrlen(lpszExt) >= EXTSIZ || !ppDocBucket)
      return FALSE;

   //
   // Disallow duplicates
   //
   if (DocFind(ppDocBucket, lpszExt)) {
      return -1;
   }

   pDocBucket = (PDOCBUCKET) LocalAlloc(LPTR,sizeof(DOCBUCKET));

   if (!pDocBucket) {
      return 0;
   }

   iBucket = DOCHASHFUNC(lpszExt);

   //
   // Set up bucket; always char lower
   //
   pDocBucket->next = ppDocBucket[iBucket];

   CharLower(lpszExt);
   lstrcpy(szExt, lpszExt);
   RemoveEndQuote(szExt);
   lstrcpy(pDocBucket->szExt, szExt);

   pDocBucket->hIcon = NULL;
   pDocBucket->lpszFI = NULL;
   
   if (lpszFileIcon != NULL)
	   pDocBucket->lpszFI = (LPTSTR) LocalAlloc(LPTR, ByteCountOf(lstrlen(lpszFileIcon)+1));		
   if (pDocBucket->lpszFI != NULL)
	  lstrcpy(pDocBucket->lpszFI, lpszFileIcon);

   ppDocBucket[iBucket] = pDocBucket;

   return 1;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     DocFind
//
// Synopsis: Finds if lpszExt is a document
//
// INC       pDocBucket  -- Structure to search
// INC       lpszExt     -- Ext to check
//
// Return:   pDocBucket, if found,
//           NULL if not.
//
// Assumes:  Properly formed ext, no leading dot.
//
// Effects:  Nothing
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

PDOCBUCKET
DocFind(PPDOCBUCKET ppDocBucket, LPTSTR lpszExt)
{
   PDOCBUCKET pDocBucket;
   TCHAR szExt[EXTSIZ];

   //
   // Disallow long exts; if invalid ppDocBucket, fail
   //
   if (lstrlen(lpszExt) >= EXTSIZ || !ppDocBucket)
      return FALSE;

   lstrcpy(szExt, lpszExt);

   CharLower(szExt);
   RemoveEndQuote(szExt);

   for (pDocBucket=ppDocBucket[DOCHASHFUNC(szExt)]; pDocBucket; pDocBucket = pDocBucket->next) {

      if (!lstrcmp(pDocBucket->szExt, szExt)) {

         return pDocBucket;
      }
   }

   return NULL;
}



HICON DocGetIcon(PDOCBUCKET pDocBucket)
{
   if (pDocBucket == NULL)
		return NULL;

   if (pDocBucket->hIcon == NULL && pDocBucket->lpszFI != NULL)
   {
      TCHAR *pchT = wcsrchr(pDocBucket->lpszFI, ',');

      if (pchT != NULL)
      {
      	  INT index = atoi(pchT+1);
      	  HICON hIcon;

		  *pchT = '\0';
      	  if (ExtractIconEx(pDocBucket->lpszFI, index, NULL, &hIcon, 1) == 1)
      	  	pDocBucket->hIcon = hIcon;
      }
   }
   return pDocBucket->hIcon;
}


#ifdef DOCENUM

//
// Enumeration routines for DOCs
//
// On if DOCENUM is defined.  Currently not used.
//
struct _DOC_ENUM {
   PPDOCBUCKET ppDocBucketBase;
   INT iCurChain;
   PDOCBUCKET pDocBucketCur;
};


/////////////////////////////////////////////////////////////////////
//
// Name:     DocOpenEnum
//
// Synopsis: Enumerates all items in the doc struct
//
// INC       ppDocBucket -- ppDocBucket to traverse
//
//
//
// Return:   PDOCENUM, NULL if fail
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

PDOCENUM
DocOpenEnum(PPDOCBUCKET ppDocBucket)
{
   PDOCENUM pDocEnum;

   pDocEnum = LocalAlloc(LMEM_FIXED, sizeof(DOCENUM));

   if (!pDocEnum)
      return NULL;

   pDocEnum->ppDocBucketBase = ppDocBucket;
   pDocEnum->iCurChain = 0;
   pDocEnum->pDocBucketCur = *ppDocBucket;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     DocEnum
//
// Synopsis: Enumerates the pDocEnum structure
//
// IN        pDocEnum
// OUT       phIcon
//
// Return:   szExt
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

LPTSTR
DocEnum(register PDOCENUM pDocEnum, PHICON phIcon)
{
   LPTSTR pszExt;

   while (!pDocEnum->pDocBucketCur) {

      pDocEnum->iCurChain++;

      //
      // Check if last chain
      //
      if (DOCBUCKETMAX == pDocEnum->iCurChain) {
         pDocEnum->pDocBucketCur = NULL;
         return NULL;
      }

      pDocEnum->pDocBucketCur = pDocEnum->ppDocBucketBase[pDocEnum->iCurChain];
   }

   *phIcon = pDocEnum->pDocBucketCur->hIcon;
   pszExt = pDocEnum->pDocBucketCur->szExt;

   //
   // Now update to the next one
   //
   pDocEnum->pDocBucketCur =  pDocEnum->pDocBucketCur->next;


   return pszExt;
}


/////////////////////////////////////////////////////////////////////
//
// Name:     DocCloseEnum
//
// Synopsis: Closes enumeration
//
// IN        pDocEnum
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
DocCloseEnum(PDOCENUM pDocEnum)
{
   LocalFree(pDocEnum);
}
#endif


/////////////////////////////////////////////////////////////////////
//
// Update Implementation
//
// Handle background updates for quicker response
//
// Basic Strategy: Do a partial update on the current aDriveInfo and related
// variables.  Signal the main thread, then update all information in the
// aDriveInfo structure.  When this is completed, send a message to the
// main thread (stable sync time) to update everyone.
//
// ** NOTE **  Update and aDriveInfo data is provided only for the main
//             thread.  All other threads must synchronize on their own.
//
// Assume that:
//
// 1. WNetOpenEnum(), WNetEnumResource() are slow [finding remembered drives]
// 2. WNetGetConnection2(), WNetFormatNetworkName() are slow [share names]
// 3. GetDriveType() is fast.
//
// The worker thread waits for hEventUpdate, then:
//
// 1. Calls GetDriveType() on all drives.
// 2. Updates cDrives, rgiDrive[]
// 3. Turn on bUpdating in all drives.
//
// 4. Sets hEventUpdatePartial
//    This allows the main thread to continue processing, since we are at a
//    clean state.
//
// 5. Get remembered connections
//    (WNetEnumResource)
//
// 6. Send a message to main thread to update the drive bar
//    FS_UPDATEDRIVETYPECOMPLETE
//
// 7. Update aDriveInfo
//    (WNetGetConnection2, WNetFormatNetworkName)
//    Since bUpdating is TRUE, the network info is safe to update.
//
// 8. Update phantom rgiDrive[]
//
//    As soon as a drive is completely read, flip bUpdating off for that drive.
//
// 9. Send a message to the main thread to update everything
//    In the send message, phantom and real rgiDrive are swapped.
//    FS_UPDATEDRIVELISTCOMPLETE
//
// A. Clear hEventUpdate.  Do this last to prevent setting while in above steps.
// B. Couldn't see its shadow, go back to sleep.
//
//
// Callee responsibility sequence (main thread):
//
// 1. Call UpdateDriveList()
//
// ... continue processing, stable state.
//
//
// Any conflicts of U_{Type, Space, VolInfo}(drive) called with the same
// drive but from different threads won't be a problem since:
//
// 1. Slow work goes to temporary stack space
//
// 2. Critical sections guarantee single update; lagging thread discards
//    its information instead of modifying state.
//
// 3. It is assumed that all threads will attempt U_ before using info.
//
// This technique isn't used for share name reads.  Instead a guard bit
// is used (bUpdating), since this is a common slow case.
//
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//
// Name:     UpdateInit
//
// Synopsis: Handles update worker thread
//
// IN        VOID
//
// Return:   VOID
//
//
// Assumes:
//
// Effects:  cDrives
//           aDriveInfo
//
//
// Notes:    Until hEventUpdatePartial, no one can access:
//           cDrives, aDriveInfo.
//
//           This is guaranteed to be synchronous with the main thread.
//
/////////////////////////////////////////////////////////////////////

VOID
UpdateInit(PVOID ThreadParameter)
{
   INT cDrivesTmp;

   while (bUpdateRun) {

      WaitForSingleObject(hEventUpdate, INFINITE);

      if (!bUpdateRun)
         break;

      if (!WAITNET_LOADED) {

         if (!NetLoad()) {

            //
            // Do something friendly here before we quit!
            //
            LoadFailMessage();

            ExitProcess(1);
         }
         SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);

      } else {

         //
         // ResetDriveInfo called much earlier in main thread in
         // InitFileManager.
         //
         ResetDriveInfo();
         SetEvent(hEventUpdatePartial);
      }

      cDrivesTmp = UpdateDriveListWorker();

      PostMessage(hwndFrame, FS_UPDATEDRIVELISTCOMPLETE, cDrivesTmp, 0L);

      //
      // We must protect hEventUpdate{,Partial} to prevent deadlock.
      //
      // Possible scenario w/o CriticalSectionUpdate:
      //
      // Main:                         Worker:
      // Set hEventUpdate
      //                               Reset hEventUpdate
      //                               Reset hEventUpdatePartial
      // Wait hEventUpdatePartial
      //
      // Worker never wakes up since hEventUpdate is reset.
      //
      EnterCriticalSection(&CriticalSectionUpdate);
      ResetEvent(hEventUpdate);
      ResetEvent(hEventUpdatePartial);
      LeaveCriticalSection(&CriticalSectionUpdate);

   }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     UpdateDriveListWorker
//
// Synopsis: Updates the drive information for worker thread.
//
// IN        VOID
//
// Return:   new cDrives value
//
//
// Assumes:
//
// Effects:  Fills rgiDrives[]
//           aDriveInfo updated
//           cDrives updated
//
//
// Notes:    Also checks for remembered connections
//
/////////////////////////////////////////////////////////////////////

#define BUF_SIZ 0x4000      // 16k buffer

INT
UpdateDriveListWorker(VOID)
{
   INT cRealDrives = 0;
   INT i;
   HANDLE hEnum;
   LPTCH pcBuf;       // 16k buffer.  blech.
   DWORD dwEntries;
   DRIVE drive;
   DWORD dwBufSiz = BUF_SIZ;
   BOOL bCheckEnum = FALSE;
   DWORD dwLen, dwLen2;
   PDRIVEINFO pDriveInfo;
   LPTCH pcBufT;

#define bFirst TRUE

   BOOL bOpenEnumSucceed = FALSE;
   DWORD dwDrivesRemembered = 0;

   INT iUpdatePhantom = iUpdateReal ^ 1;


   //
   // GetLogicalDrives simply calls GetDriveType,
   // so just do that here since we need to do it later
   // anyway.
   //

   //
   // !! NOTE !!
   // This really should be IsValidDisk(drive), but this macro
   // is faster.
   //
#define VALIDDRIVE(drive)                               \
   ( (aDriveInfo[drive].uType != DRIVE_UNKNOWN) &&      \
     (aDriveInfo[drive].uType != DRIVE_NO_ROOT_DIR) )

   // Now toss in a few remote drives

   // Initialize enumeration for all remembered disks that
   // are connectable of any type.

   // New "if" added if not connected, don't show remembered!
   // No else clause needed since defaults to no remembered connections.

   //
   // bFirst static added  (Always TRUE)
   //
   if (bFirst && WAITNET_LOADED) {

      pcBuf = (LPTCH) LocalAlloc(LPTR, ByteCountOf(BUF_SIZ));

      if (pcBuf) {

         if (NO_ERROR == WNetOpenEnum(RESOURCE_REMEMBERED,
            RESOURCETYPE_DISK, RESOURCEUSAGE_CONNECTABLE,NULL,&hEnum)) {

            bOpenEnumSucceed = TRUE;

            // Enumerate all the resources.
            // take no prisoners or error messages.
            // BONK!  Fix this in the future.

EnumRetry:
            // Get all entries
            dwEntries = 0xffffffff;

            switch (WNetEnumResource(hEnum, &dwEntries, pcBuf, &dwBufSiz)) {

            case NO_ERROR:

               // Yes, we have no error so allow the next loop to
               // check the pcBuf for remembered connections.

               bCheckEnum = TRUE;

               // Setup Bitfield for remembered connections
               for (i = 0; i < (INT)dwEntries; i++) {

                  // Check if lpLocalName is non-NULL

                  if ( ((LPNETRESOURCE) pcBuf)[i].lpLocalName ) {

                     //
                     // Make sure this is a drive letter
                     //

                     if (((LPNETRESOURCE) pcBuf)[i].lpLocalName[1] != CHAR_COLON)
                        continue;

                     drive = (((LPNETRESOURCE) pcBuf)[i].lpLocalName[0] & 0x001f) - 1;

                     //
                     // If this is also an active drive, it isn't
                     // a remembered drive; continue
                     //

                     if (VALIDDRIVE(drive))
                        continue;

                     dwDrivesRemembered |= (1 << drive);

                     pDriveInfo = &aDriveInfo[drive];

                     //
                     // Free buffer if used
                     //
                     if (pDriveInfo->lpConnectInfo)
                        LocalFree((HLOCAL)pDriveInfo->lpConnectInfo);

                     //
                     // To avoid redundancy and preserve persistent connection
                     // remote names when LanmanWorkstation stops, we
                     // save the names enumerated here.
                     //
                     dwLen = lstrlen( ((LPNETRESOURCE) pcBuf)[i].lpRemoteName) + 1;
                     dwLen2 = lstrlen( ((LPNETRESOURCE) pcBuf)[i].lpProvider) + 1;

                     pDriveInfo->dwConnectInfoMax = ByteCountOf(dwLen + dwLen2) +
                        sizeof(WNET_CONNECTIONINFO);


                     pDriveInfo->lpConnectInfo =
                        (LPWNET_CONNECTIONINFO) LocalAlloc(LPTR, pDriveInfo->dwConnectInfoMax);

                     //
                     // Memory error handling
                     //
                     if (!pDriveInfo->lpConnectInfo) {
                        C_NetCon(drive, ERROR_NOT_ENOUGH_MEMORY);
                        continue;
                     }


                     //
                     // setup fake ConnectInfo structure
                     //
                     // NOTE: ConnectInfo assumed WORD aligned when this
                     // thing goes UNICODE: (which it coincidentally is)
                     //
                     // LATER: WORDUP sizeof ConnectInfo to prevent
                     // misalignment on MIPS.
                     //

                     pDriveInfo->lpConnectInfo->lpRemoteName = (LPTSTR)
                        (((LPBYTE)pDriveInfo->lpConnectInfo) +
                        sizeof(WNET_CONNECTIONINFO));

                     lstrcpy(pDriveInfo->lpConnectInfo->lpRemoteName,
                        ((LPNETRESOURCE) pcBuf)[i].lpRemoteName);

                     pDriveInfo->lpConnectInfo->lpProvider =
                        pDriveInfo->lpConnectInfo->lpRemoteName + dwLen;

                     lstrcpy(pDriveInfo->lpConnectInfo->lpProvider,
                        ((LPNETRESOURCE) pcBuf)[i].lpProvider);

                     //
                     // Now get the multiline and short names
                     //

                     NetCon_UpdateAltName(drive, ERROR_SUCCESS);
                  }
               }


               // Must continue til ERROR_NO_MORE_ITEMS
               goto EnumRetry;

            case ERROR_MORE_DATA:

               // Buffer is too small; realloc with bigger buffer
               dwBufSiz += BUF_SIZ;

               pcBufT = pcBuf;
               pcBuf = (LPTCH) LocalReAlloc((HLOCAL)pcBuf, ByteCountOf(dwBufSiz), LMEM_MOVEABLE);

               // Only retry if pcBuf is successfully reallocated.
               // If it wasn't, then just fall through since
               // bCheckEnum is defaulted false and we won't use pcBuf.

               if (pcBuf)
                  goto EnumRetry;

               // Failed memory allocation, free pcBufT
               LocalFree((HLOCAL)pcBufT);

            case ERROR_NO_MORE_ITEMS:
               break;

            default:
               break;
            }
         }
      }


   } else {

      // Set pcBuf to Null so we don't free it below
      pcBuf = NULL;
   }


   // In this else case (Not connected to net), don't use pcBuf below
   // bCheckEnum defaults to FALSE;

   for (i = 0, pDriveInfo = &aDriveInfo[0];
        i < MAX_DRIVES;
        i++, pDriveInfo++)
   {
      //
      // Take only active drives--ignore remembered ones.
      // This is ok since UpdateInit calls ResetDriveInfo which
      // turns off bRemembered for real drives.
      //
      if (VALIDDRIVE(i) && !pDriveInfo->bRemembered) {
         rgiDriveReal[iUpdatePhantom][cRealDrives++] = i;

         R_NetCon(i);

         //
         // Force a refresh
         //
         // No need for Type; done above.
         //

      } else if (bCheckEnum && (1 << i) & dwDrivesRemembered) {

         //
         // This handles remembered connections that don't already exist.
         //

         // if the enumerator successed (bCheckEnum)
         // and there are entries remaining, check for drives remembered.

         // Since we must do things in order (rgiDrive must hold
         // drive sequentially, from a-z), plop in our enumerated ones
         // only when they are next.

         rgiDriveReal[iUpdatePhantom][cRealDrives++] = i;

         pDriveInfo->bRemembered = TRUE;
         pDriveInfo->uType =  DRIVE_REMOTE;
         pDriveInfo->iOffset = GetDriveOffset(i);

         C_Type(i, ERROR_SUCCESS);
         C_NetCon(i, ERROR_CONNECTION_UNAVAIL);

      } else {

         //
         // No need for Type; done above.
         //
         I_NetCon(i);      // Invalidate NetCon!

         //
         // No longer remembered, either.  We must clear this out because
         // during the first phase of updates, we pretend that this bit
         // is valid since it doesn't change much.
         //
         pDriveInfo->bRemembered = FALSE;
      }

      //
      // Now the drive is in a safe state.
      //
   }

   // Clear out other drives

   for (i=cRealDrives; i < MAX_DRIVES; i++) {
      rgiDriveReal[iUpdatePhantom][i] = 0;
   }

   if (bOpenEnumSucceed)
      WNetCloseEnum(hEnum);

   if (pcBuf)
      LocalFree((HANDLE)pcBuf);


   PostMessage(hwndFrame, FS_UPDATEDRIVETYPECOMPLETE, (WPARAM)cRealDrives, 0L);

   //
   // Now go through and update all the VolInfo/NetCon stuff
   //
   for (i = 0; i < cRealDrives; i++) {

      drive = rgiDriveReal[iUpdatePhantom][i];

      if (IsRemoteDrive(drive)) {

         U_NetCon(drive);
         aDriveInfo[drive].bUpdating = FALSE;

      } else {

         if (!IsRemovableDrive(drive) && !IsCDRomDrive(drive)) {

            U_VolInfo(drive);
         }
      }
   }

   return cRealDrives;

#undef BUF_SIZ
#undef VALIDDRIVE
}


/////////////////////////////////////////////////////////////////////
//
// Name:     WFGetConnection
//
// Synopsis: gets connection information including disconnected drives.
//
// INC   drive           Drive # to look up
// INC   bConvertClosed  BOOL   FALSE => convert closed/err drives ret SUCCESS
// OUTC  ppPath          LPTSTR* Net name; user must NOT free!
//                       ppPath[-4] .. ppPath[-1] ARE valid!
//                       ppPath may be NULL!
// INC   dwType          Format of Net Con string (valid for net con only)
//           ALTNAME_MULTI:   multiline format, header valid
//           ALTNAME_SHORT:   short format, header valid
//           ALTNAME_REG:     standard default, header INVALID
//
// Returns:  ERROR_*     error code
//           DE_REGNAME  Regname returned when ALTNAME requested.
//
// Assumes:  If second char is colon (':'), assumes first char is a valid
//           drive letter ([A-Z]).
//
//           drive is a network drive
//
// Effects:  aDriveInfo NetCon cache
//
//
// Notes:    Callee must not free or modify returned buffer. ***  BUT  ***
//           they can modify ppPath[-4] .. ppPath[-1] inclusive if
//           called with any style other than ALTNAME_REG for Networks.
//
//           Header is only valid if return value is 0 and not ALTNAME_REG!!!
//
/////////////////////////////////////////////////////////////////////

DWORD
WFGetConnection(DRIVE drive, LPTSTR* ppPath, BOOL bConvertClosed, DWORD dwType)
{
   register DWORD dwRetVal;
   BOOL bConverted = FALSE;

   //
   // If bUpdating, skip the U_NetCon for speed.
   //
   if (!aDriveInfo[drive].bUpdating) {
      U_NetCon(drive);
   }

   //
   // Get the status of the main network name.
   //
   dwRetVal = GETRETVAL(NetCon,drive);

   //
   // Convert error codes here
   //
   // ERROR_NO_NETWORK         -> ERROR_NOT_CONNECTED
   // ERROR_CONNECTION_UNAVAIL -> remembered
   //
   if (dwRetVal == ERROR_NO_NETWORK) {
      dwRetVal = ERROR_NOT_CONNECTED;
   } else {

      if (!bConvertClosed) {
         if (dwRetVal == ERROR_CONNECTION_UNAVAIL &&
            aDriveInfo[drive].bRemembered) {

            //
            // Since bRemembered is set, we know that the string
            // is valid since it was successfully allocated in
            // UpdateDriveListWorker().
            //
            dwRetVal = ERROR_SUCCESS;
            bConverted = TRUE;
         }
      }
   }

   //
   // Check if we want to return a share name
   //
   if (ppPath) {

      //
      // If updating, return error updating.
      //
      if (aDriveInfo[drive].bUpdating) {

         return DE_UPDATING;
      }

      //
      // Check if we want an altname.
      //
      if (dwType < MAX_ALTNAME) {

         if (aDriveInfo[drive].dwAltNameError) {

            //
            // We had an error, check if main name is ok
            //
            if (!dwRetVal) {

               //
               // Yes, return the main name with an error.
               //
               dwRetVal = DE_REGNAME;
               goto UseRegName;
            }

            //
            // Return this error code
            //
            dwRetVal = aDriveInfo[drive].dwAltNameError;

         } else {

            *ppPath = aDriveInfo[drive].lpszRemoteNameMinusFour[dwType] +
               DRIVE_INFO_NAME_HEADER;
         }

      } else {

         //
         // We want to use the regular name.
         //
         if (!dwRetVal) {

            //
            // No error occurred, this is ok.
            //
UseRegName:
            *ppPath = aDriveInfo[drive].lpConnectInfo->lpRemoteName;
         }
      }
   }
   return dwRetVal;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     UpdateDriveListComplete
//
// Synopsis: Worker thread read completed, update everything.
//
// IN        VOID
//
// Return:   VOID
//
//
// Assumes:  We are called via a SendMessage from the worker thread
//           to hwndFrame.  This ensures that we are in a safe state
//           for updating.
//
//           bUpdating for everyone is off
//
//           iUpdateReal has been updated
//
// Effects:  TBC
//
//
// Notes:    UpdateConnections should already close any open windows for
//           quick response time based on rgiDriveType.
//
/////////////////////////////////////////////////////////////////////

VOID
UpdateDriveListComplete(VOID)
{
   HWND hwnd, hwndNext;
   DRIVE drive;
   DRIVEIND driveInd;
   INT CurSel;
   TCHAR szPath[2*MAXPATHLEN];
   LPTSTR lpszVol, lpszOldVol;

   for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = hwndNext) {

      hwndNext = GetWindow(hwnd, GW_HWNDNEXT);

      // ignore the titles and search window
      if (GetWindow(hwnd, GW_OWNER) || hwnd == hwndSearch)
         continue;

      drive = GetWindowLongPtr(hwnd, GWL_TYPE);

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

         if (!WFGetConnection(drive, &lpszVol, FALSE, ALTNAME_REG)) {
            lpszOldVol = (LPTSTR) GetWindowLongPtr(hwnd, GWL_VOLNAME);

            if (lpszOldVol && lpszVol) {

               if (lstrcmpi(lpszVol, lpszOldVol)) {

                  //
                  // Share has changed, refresh.
                  // Don't call UpdateDriveList... we just did that!
                  //
                  RefreshWindow(hwnd, FALSE, FALSE);

                  continue;
               }
            }
         }

         //
         // Just update drive window title
         //

         GetMDIWindowText(hwnd, szPath, COUNTOF(szPath));
         SetMDIWindowText(hwnd, szPath);
      }
   }

   //
   // Redo all of the drives.
   //
   if (hwndDriveList)
   {
       SendMessage(hwndDriveList, WM_SETREDRAW, FALSE, 0);
       CurSel = SendMessage(hwndDriveList, CB_GETCURSEL, 0, 0);
       for (driveInd = 0; driveInd < cDrives; driveInd++)
       {
           if (aDriveInfo[rgiDrive[driveInd]].dwLines[ALTNAME_MULTI] != 1)
           {
              SendMessage(hwndDriveList, CB_DELETESTRING, driveInd, 0);
              SendMessage(hwndDriveList, CB_INSERTSTRING, driveInd, rgiDrive[driveInd]);
           }
       }
       SendMessage(hwndDriveList, CB_SETCURSEL, CurSel, 0);
       SendMessage(hwndDriveList, WM_SETREDRAW, TRUE, 0);

       InvalidateRect(hwndDriveList, NULL, TRUE);
       UpdateWindow(hwndDriveList);
   }
}


VOID
UpdateDriveList(VOID)
{
   if (!WAITNET_LOADED)
      return;

   EnterCriticalSection(&CriticalSectionUpdate);

   SetEvent(hEventUpdate);
   WaitForSingleObject(hEventUpdatePartial, INFINITE);

   LeaveCriticalSection(&CriticalSectionUpdate);
}


VOID
UpdateWaitQuit(VOID)
{
   bUpdateRun = FALSE;

   EnterCriticalSection(&CriticalSectionUpdate);
   SetEvent(hEventUpdate);
   LeaveCriticalSection(&CriticalSectionUpdate);

   WaitForSingleObject(hThreadUpdate, INFINITE);
}





/////////////////////////////////////////////////////////////////////
//
//  Background net stuff.
//
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
//
// Name:     NetLoad
//
// Synopsis: Loads the net after initfilemanager is called.
//
// IN        VOID
//
// Return:   BOOL  T/F success/fail
//
//
// Assumes:  hEventWaitNet initialized
//
// Effects:  hEventNetLoad set when done
//           bNetShareLoad set when done
//           bNetTypeLoad set when done
//           bNetLoad set when done
//
//           Updates screen after calling WNetGetDirectoryType on all
//           windows (before bNetTypeLoad set).
//
// Notes:    Calls InitMenus to load extensions after net loads.
//           We _cannot_ do any SendMessages here!
//
//           ** This code must not do any synchronous calls to
//           ** the main thread (including SendMessages).  The
//           ** main thread may be waiting on one of the events that
//           ** we set here!
//
/////////////////////////////////////////////////////////////////////

BOOL
NetLoad(VOID)
{
   HMENU hMenuFrame;
   FMS_LOAD ls;
   const WORD bias = (IDM_SECURITY + 1) * 100;

   HWND hwnd, hwndT;
   DWORD dwType;
   DRIVE drive;

   TCHAR szPath[] = SZ_ACOLONSLASH;

   if (WNetStat(NS_CONNECT))  {
      hMPR = LoadLibrary(MPR_DLL);

      if (!hMPR)
         return FALSE;


      //
      // This is used to reduce typing.
      // Each function (e.g. Foo) has three things:
      //
      // lpfnFoo                  pointer to function
      // NETWORK_Foo              name for GetProcAddress
      // #define Foo (*lpfnFoo)   make it transparent
      //

#define GET_PROC(x) \
      if (!(lpfn##x = (PVOID) GetProcAddress(hMPR,NETWORK_##x))) \
         return FALSE

      GET_PROC(WNetCloseEnum);
      GET_PROC(WNetConnectionDialog2);
      GET_PROC(WNetDisconnectDialog2);

      GET_PROC(WNetEnumResourceW);
      GET_PROC(WNetGetConnection2W);
      GET_PROC(WNetGetDirectoryTypeW);
      GET_PROC(WNetGetLastErrorW);
      GET_PROC(WNetGetPropertyTextW);
      GET_PROC(WNetOpenEnumW);
      GET_PROC(WNetPropertyDialogW);
      GET_PROC(WNetFormatNetworkNameW);

      if ((lpfnWNetRestoreSingleConnectionW = (PVOID) GetProcAddress(hMPR,"WNetRestoreSingleConnectionW")) == NULL)
      {
         GET_PROC(WNetRestoreConnectionW);
      }

#ifdef NETCHECK
      GET_PROC(WNetDirectoryNotifyW);
#endif

#undef GET_PROC

      bNetLoad = TRUE;
   }

   if (WNetStat(NS_SHAREDLG)) {

      hNTLanman = LoadLibrary(NTLANMAN_DLL);

      if (hNTLanman) {

#define GET_PROC(x) \
         if (!(lpfn##x = (PVOID) GetProcAddress(hNTLanman, NETWORK_##x))) \
            goto Fail

         GET_PROC(ShareCreate);
         GET_PROC(ShareStop);
#undef GET_PROC

         //
         // If bNetShareLoad is FALSE, then we know that the share stuff
         // is not available.  Therefore, we won't try to use
         // WNetGetDirectoryType below, which leaves bNetTypeLoad FALSE,
         // which prevents future use of WNetGetDirectoryType.
         //
         bNetShareLoad = TRUE;

      } else {
Fail:
         //
         // Disable the share buttons/menus
         // Since WNetStat(NS_SHAREDLG) ret'd true, then we added the
         // buttons.  Mistake.  Disable them now.
         //
         PostMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_SHAREAS, FALSE);
         PostMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_STOPSHARE, FALSE);

         EnableMenuItem(GetMenu(hwndFrame), IDM_SHAREAS,
            MF_BYCOMMAND | MF_GRAYED );

         EnableMenuItem(GetMenu(hwndFrame), IDM_STOPSHARE,
            MF_BYCOMMAND | MF_GRAYED );
      }
   }

   SetEvent(hEventNetLoad);
   bNetDone = TRUE;

   //
   // Try loading acledit.  If we fail, then gray out the button and
   // remove the popup menu.
   //
   hAcledit = LoadLibrary(ACLEDIT_DLL);

   hMenuFrame = GetMenu(hwndFrame);

   if (hAcledit) {

      lpfnAcledit = (FM_EXT_PROC) GetProcAddress(hAcledit, FM_EXT_PROC_ENTRYW);
      if (!lpfnAcledit)
         lpfnAcledit = (FM_EXT_PROC) GetProcAddress(hAcledit, FM_EXT_PROC_ENTRYA);

      ls.wMenuDelta = bias;
      ls.hMenu = GetSubMenu(hMenuFrame, IDM_SECURITY);

      if (!lpfnAcledit ||
         !(*lpfnAcledit)(hwndFrame, FMEVENT_LOAD, (LPARAM)(LPFMS_LOAD)&ls)) {

         FreeLibrary(hAcledit);

         lpfnAcledit = NULL;
      }
   }

   if (!lpfnAcledit) {

	  INT iMax;
	  HWND hwndActive;
	  hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
	  if (hwndActive && GetWindowLongPtr(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
	     iMax = 1;
	  else
	     iMax = 0;

      DeleteMenu(hMenuFrame, IDM_SECURITY + iMax, MF_BYPOSITION);
      DrawMenuBar(hwndFrame);

      PostMessage(hwndToolbar, TB_ENABLEBUTTON, IDM_PERMISSIONS, FALSE);
   }

   SetEvent(hEventAcledit);
   bNetAcleditDone = TRUE;

   //
   // We need to check both, since this is a sharing thing,
   // but the api is in network.
   //
   if (bNetShareLoad && bNetLoad) {

      //
      // Now go through and call WNetGetDirectoryType for all windows
      // to pre-cache this info without stalling the user.
      //

      for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

         if (hwnd != hwndSearch && !GetWindow(hwnd, GW_OWNER)) {

            drive = GetWindowLongPtr(hwnd, GWL_TYPE);
            DRIVESET(szPath, drive);

            if (!aDriveInfo[drive].bShareChkTried  &&
               WN_SUCCESS != WNetGetDirectoryType(szPath, &dwType, TRUE)) {

               aDriveInfo[drive].bShareChkFail = TRUE;
            }

            aDriveInfo[drive].bShareChkTried = TRUE;
         }
      }

      bNetTypeLoad = TRUE;

      for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

         if (hwnd != hwndSearch && !GetWindow(hwnd, GW_OWNER)) {

            if (hwndT = HasTreeWindow(hwnd)) {
               InvalidateRect(GetDlgItem(hwndT, IDCW_TREELISTBOX), NULL, FALSE);
            }
            if (hwndT = HasDirWindow(hwnd)) {
               InvalidateRect(GetDlgItem(hwndT, IDCW_LISTBOX), NULL, FALSE);
            }
         }
      }
   }

   return TRUE;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ResetDriveInfo
//
// Synopsis: Resets the drive info struct to a stable, minimum info state.
//
// IN      VOID
//
// Return: VOID
//
//
// Assumes:
//
// Effects: cDrives, aDriveInfo, rgiDrive
//
//
// Notes:   !! BUGBUG !!
//
//          Race condition with IsNetDir!
//
/////////////////////////////////////////////////////////////////////

VOID
ResetDriveInfo()
{
   PDRIVEINFO pDriveInfo;
   DRIVE drive;
   INT i;

   //
   // Initialize the count of drives.
   //
   cDrives = 0;

   //
   // We must be quick until the reset events...
   //
   for (drive = 0, pDriveInfo = &aDriveInfo[0];
        drive < MAX_DRIVES;
        drive++, pDriveInfo++)
   {
      R_Type(drive);
      U_Type(drive);

      R_Space(drive);
      R_VolInfo(drive);

      //
      // Should call IsValidDisk, but this is faster.
      //
      if ( (pDriveInfo->uType != DRIVE_UNKNOWN) &&
           (pDriveInfo->uType != DRIVE_NO_ROOT_DIR) )
      {
         //
         // Update cDrives
         //
         rgiDrive[cDrives] = drive;

         pDriveInfo->bRemembered = FALSE;
         pDriveInfo->iOffset = GetDriveOffset(drive);

         if (IsRemoteDrive(drive))
         {
            //
            // Update dwLines for WM_MEASUREITEM
            //
            for (i = 0; i < MAX_ALTNAME; i++)
            {
               pDriveInfo->dwLines[i] = 1;
            }
            C_NetCon(drive, ERROR_SUCCESS);
         }
         else
         {
            C_NetCon(drive, ERROR_NO_NETWORK);
         }

         cDrives++;
      }
      else if (pDriveInfo->bRemembered)
      {
         //
         // Hack: assume remembered connections don't change too much.
         //

         //
         // Also, at this point, the state information in aDriveInfo
         // hasn't changed for this remembered connection.
         //
         rgiDrive[cDrives] = drive;
         cDrives++;
      }

      //
      // Any clearing of drive information should be done
      // here.  As soon as a drive is invalid, we reset the
      // necessary stuff when it becomes valid.
      // (Must be set for "good" drives elsewhere)
      //

      //
      // Clear all invalid drives' bShareChkFail.
      //
      // (This bool checks if a IsNetDir / IsNetPath fails-- due to
      // WNetGetDirectoryType failing (due to not being administrator on
      // remote machine).  Only call WNetGetDirectory once, since it is
      // s-l-o-w for fails.  It's also cached, too.
      //
      pDriveInfo->bShareChkFail  = FALSE;
      pDriveInfo->bShareChkTried = FALSE;

      aDriveInfo[drive].bUpdating = TRUE;
   }
}



/////////////////////////////////////////////////////////////////////
//
// Name:     LoadComdlg
//
// Synopsis: Loads funcs GetOpenFileName and ChooseFont dynamically
//
// IN:       VOID
//
// Return:   BOOL  T=Success, F=FAILURE
//
//
// Assumes:
//
// Effects:  hComdlg, lpfn{GetOpenFilename, ChooseFont}
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

BOOL
LoadComdlg(VOID)
{
   UINT uErrorMode;

   //
   // Have we already loaded it?
   //
   if (hComdlg)
      return TRUE;

   //
   // Let the system handle errors here
   //
   uErrorMode = SetErrorMode(0);
   hComdlg = LoadLibrary(COMDLG_DLL);
   SetErrorMode(uErrorMode);

   if (!hComdlg)
      return FALSE;

#define GET_PROC(x) \
   if (!(lpfn##x = (PVOID) GetProcAddress(hComdlg,COMDLG_##x))) \
      return FALSE

   GET_PROC(ChooseFontW);
   GET_PROC(GetOpenFileNameW);

#undef GET_PROC

   return TRUE;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     WaitLoadEvent
//
// Synopsis: Waits for event and puts up hourglass
//
// INC:      BOOL   bNet    TRUE   wait on net
//                          FALSE  wait on acledit
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
WaitLoadEvent(BOOL bNet)
{
   HCURSOR hCursor;

   if (!(bNet ? bNetDone : bNetAcleditDone)) {

      hCursor = LoadCursor(NULL, IDC_WAIT);

      if (hCursor)
         hCursor = SetCursor(hCursor);

      ShowCursor(TRUE);

      SetThreadPriority(hThreadUpdate, THREAD_PRIORITY_NORMAL);

      WaitForSingleObject(bNet ?
            hEventNetLoad :
            hEventAcledit,
         INFINITE);

      SetThreadPriority(hThreadUpdate, THREAD_PRIORITY_BELOW_NORMAL);

      if (hCursor)
         SetCursor(hCursor);

      ShowCursor(FALSE);
   }
}


