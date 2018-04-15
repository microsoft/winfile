/********************************************************************

   wfchgnot.c

   WinFile Change Notify module.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

//
// Forward Declarations
//
VOID NotifyReset();
VOID NotifyDeleteHandle(INT i);
VOID NotifyAddHandle(INT i, HWND hwnd, LPTSTR lpPath, DWORD dwFilter);

//
// Maximum number of windows that are viewable
// at once.
//
HWND   ahwndWindows[MAX_WINDOWS];
DRIVE  adrive[MAX_WINDOWS];
HANDLE ahEvents[MAX_WINDOWS];
INT    nHandles;

#define bNOTIFYACTIVE uChangeNotifyTime


VOID
vWaitMessage()
{
   DWORD dwEvent;

   dwEvent = MsgWaitForMultipleObjects(nHandles,
                                       ahEvents,
                                       FALSE,
                                       INFINITE,
                                       QS_ALLINPUT);

   if (dwEvent != (DWORD) (WAIT_OBJECT_0 + nHandles)) {

      if (dwEvent == (DWORD)-1) {

         NotifyReset();

      } else {

         dwEvent -= WAIT_OBJECT_0;

         //
         // Check to see if this handle has been removed already, if so
         // return before we try to refresh a non-existent window.
         //
         if ((dwEvent >= MAX_WINDOWS) || ahEvents[dwEvent] == NULL)
            return;

         //
         // Modify GWL_FSCFLAG directly.
         //
         // We do the ModifyWatchList right before we read (close then
         // open on the same path).  This clears out any extra notifications
         // caused by fileman's move/copy etc.
         //
         SetWindowLongPtr(ahwndWindows[dwEvent], GWL_FSCFLAG, TRUE);
         PostMessage(hwndFrame, FS_FSCREQUEST, 0, 0L);

         if (FindNextChangeNotification(ahEvents[dwEvent]) == FALSE) {

            //
            // If we can't find the next change notification, remove it.
            //
            NotifyDeleteHandle(dwEvent);
         }
      }
   }
}


/////////////////////////////////////////////////////////////////////
//
// Name:     InitializeWatchList
//
// Synopsis: Setups up change notifications
//
// IN        VOID
//
// Return:   VOID
//
//
// Assumes:  GetSettings has initialized uChangeNotifyTime
//
// Effects:  nHandles, ahwndWindows, ahEvents
//
//
// Notes:    If not successful, bNOTIFYACTIVE = FALSE
//
/////////////////////////////////////////////////////////////////////

VOID
InitializeWatchList(VOID)
{
   INT i;

   //
   // Change notify system is off if uChangeNotifyTime == 0
   // No, this doesn't mean zero time.
   //
   if (!bNOTIFYACTIVE)
      return;

   for (i = 0; i < MAX_WINDOWS; i++) {
      ahwndWindows[i] = NULL;
      ahEvents[i] = NULL;
   }

   nHandles = 0;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     DestroyWatchList
//
// Synopsis: Initializes change notify system
//
// IN        VOID
//
// Return:   VOID
//
//
// Assumes:  InitializeWatchList has been called:
//
// Effects:  watchlist closed
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
DestroyWatchList(VOID)
{
   PHANDLE phChange;

   //
   // Only destroy if successfully started
   //
   if (bNOTIFYACTIVE) {

      //
      // Clean up handles in hChange!
      //
      for(phChange = ahEvents;
         nHandles;
         nHandles--, phChange++) {

         FindCloseChangeNotification(*phChange);
      }
   }
}


VOID
NotifyReset()
{
   NotifyPause(-1, (UINT)-2);
   nHandles = 0;
}

/////////////////////////////////////////////////////////////////////
//
// Name:     NotifyPause
//
// Synopsis: Pause notification on a set of drives
//
// INC       -- drive    -2 == ALL
//                       -1 == All of type uType
//                       0 < drive < 26 is the drive to stop
// INC       -- uType    Type of drive to stop notification on
//
//
// Return:   VOID
//
//
// Assumes:  Called by main thread (drive windows stable)
//
// Effects:  notification variables
//           if (drive != -1) GWL_FSCFLAG on that drive is cleared.
//
//
// Notes:    Setting drive limits notification ignore to just 1 drive.
//           This can be called many times
//
/////////////////////////////////////////////////////////////////////

VOID
NotifyPause(DRIVE drive, UINT uType)
{
   INT i;
   DRIVE driveCurrent;

   if (!bNOTIFYACTIVE)
      return;


   for(i=0;i<nHandles;i++) {

      driveCurrent = adrive[i];

      if (-2 == drive ||
          ((-1 == drive || drive == driveCurrent) &&
          ((UINT)-1 == uType || aDriveInfo[driveCurrent].uType == uType))) {

         if (-2 != drive)
            SetWindowLongPtr(ahwndWindows[i], GWL_NOTIFYPAUSE, 1L);

         NotifyDeleteHandle(i);

         //
         // This is the traverse while compact problem.
         // We get around this by decrementing i so the i++ is
         // negated.
         //
         i--;
      }
   }
}



/////////////////////////////////////////////////////////////////////
//
// Name:     NotifyResume
//
// Synopsis: Resume all notifications
//
// INC       -- drive    -1 == ALL
//                       0 < drive < 26 is the drive to stop
// INC       -- uType    Type of drive to stop notification on
//
// Return:   VOID
//
//
// Assumes:  Called by main thread (drive windows stable)
//           Assumes worker thread not changing data structures
//           (Valid since main thread blocks on all change requests)
//
// Effects:  notification variables
//
//
// Notes:    scans through all windows looking for dirs
//
/////////////////////////////////////////////////////////////////////

VOID
NotifyResume(DRIVE drive, UINT uType)
{
   DRIVE driveCurrent;
   HWND hwnd;

   if (!bNOTIFYACTIVE)
      return;

   //
   // Scan though all open windows looking for dir windows that
   // are not matched up.
   //
   for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
      hwnd;
      hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

      driveCurrent = GetWindowLongPtr(hwnd, GWL_TYPE);

      //
      // Skip search window
      //
      if (-1 == driveCurrent)
         continue;

      //
      // Check if this drive is paused and meets the restart criteria
      //

      if (-2 == drive ||
          ((-1 == drive || drive == driveCurrent) &&
          ((UINT)-1 == uType || aDriveInfo[driveCurrent].uType == uType) &&
          GetWindowLongPtr(hwnd, GWL_NOTIFYPAUSE))) {

         //
         // Restart notifications on this window
         //

         SendMessage(hwnd, FS_NOTIFYRESUME, 0, 0L);
         SetWindowLongPtr(hwnd, GWL_NOTIFYPAUSE, 0L);
      }
   }
}



/////////////////////////////////////////////////////////////////////
//
// Name:     ModifyWatchList
//
// Synopsis: Modifies the handles watched
//
// IN        hwnd         hwnd to update
// IN        lpPath       path
//                        NULL = terminate watch
// IN        fdwFilter    notification events to watch
//
// Return:   VOID
//
//
// Assumes:  Single threaded implementation.  All calls in this file
//           are non reentrant.  (Multithreaded approach removed around
//           9/15/93 to eliminate a thread.)
//
// Effects:  ahwndWindows
//           ahChange
//           adrives
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
ModifyWatchList(HWND hwnd,
                LPTSTR lpPath,
                DWORD fdwFilter)
{
   INT i;

   if (!bNOTIFYACTIVE)
      return;

   //
   // First we must see if this window already has a watch handle
   // assigned to it.  If so, then we must close that watch, and
   // restart a new one for it.  UNLESS lpPath is NULL, in which
   // case the window will be removed from our list.
   //

   for (i = 0;
      i < nHandles && ahwndWindows[i] != NULL &&
      ahwndWindows[i] != hwnd; i++ )

      ;

   if (i < nHandles && ahwndWindows[i] != NULL) {

      //
      // Now check to see if we are just changing the handle or removing
      // the window all together.
      //

      if (lpPath == NULL) {

         NotifyDeleteHandle(i);
         //
         // Now we need to clean up and exit.
         //
         return;

      } else {

         if (FindCloseChangeNotification(ahEvents[i]) == FALSE) {
            //
            // BUGBUG:   In the event that this fails we will need to do
            //           something about it....I am not sure what at this
            //           point.
            //
         }

         //
         // We now need to issue a new FindFirstChangeNotification for
         // the new path and modify the structures appropriately.
         //

         NotifyAddHandle(i, hwnd, lpPath, fdwFilter);

         //
         // Now we need to clean up and exit.
         //
         return;

      }
   } else {
      if (lpPath == NULL) {
         //
         // BUGBUG: We didn't find the window handle but we are being
         //         asked to remove it from the list....Something went
         //         wrong.
         //
         return;
      }
   }

   //
   // We now need to issue a new FindFirstChangeNotification for
   // the new path and modify the structures appropriately.
   //

   NotifyAddHandle(i, hwnd, lpPath, fdwFilter);
}



/////////////////////////////////////////////////////////////////////
//
// Name:     NotifyDeleteHandle
//
// Synopsis: Deletes open associated handle handle.
//
// INC       i  index in ahwndWindows[] to delete
//
//
// Return:   VOID
//
//
// Assumes:  Exclusive, safe access to globals.
//
// Effects:  ahwndWindows, nHandles, adrives
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
NotifyDeleteHandle(INT i)
{
   if (INVALID_HANDLE_VALUE != ahEvents[i] &&
      FindCloseChangeNotification(ahEvents[i]) == FALSE) {
   }

   //
   // We now need to delete the window all together...So compact
   // the list of change handles and window handles to reflect this.
   // Since order isn't important, just copy the last
   // item to the empty spot, and write NULL in the
   // last position.
   //
   nHandles--;

   ahwndWindows[i]  = ahwndWindows[ nHandles ];
   ahEvents[i] = ahEvents[nHandles];
   adrive[i] = adrive[nHandles];

   ahwndWindows[nHandles]  = NULL;
   ahEvents[nHandles] = NULL;

   return;
}



/////////////////////////////////////////////////////////////////////
//
// Name:     NotifyAddHandle
//
// Synopsis: Adds handle and associated data
//
// INC       i         INT     Index to modify
// INC       hwnd      --      hwnd of window to watch (hwndTree, not hwndDir)
// INC       lpPath    --      path to watch, -1 == same as old
// INC       fdwFilter --      filters for notification
//
// Return:   VOID
//
// Assumes:  i is a valid index; no boundary checking done.
//
// Effects:  nHandles is incremented if necessary.
//
//
// Notes:
//
/////////////////////////////////////////////////////////////////////

VOID
NotifyAddHandle(INT i, HWND hwnd, LPTSTR lpPath, DWORD fdwFilter)
{
   adrive[i] = DRIVEID(lpPath);

   ahwndWindows[i] = hwnd;

   ahEvents[i] = FindFirstChangeNotification(lpPath,
      FALSE,
      fdwFilter);

   if (nHandles == i)
      nHandles++;

   if (ahEvents[i] == INVALID_HANDLE_VALUE) {

      //
      // Since this handle is invalid, delete it.
      //
      NotifyDeleteHandle(i);
   }
}

