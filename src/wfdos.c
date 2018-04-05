/********************************************************************

   wfdos.c

   Ported code from wfdos.asm

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

// Warning: assumes bytes per sector < 2 gigs

VOID
GetDiskSpace(DRIVE drive,
   PLARGE_INTEGER pqFreeSpace,
   PLARGE_INTEGER pqTotalSpace)
{
   DWORD dwSectorsPerCluster;
   DWORD dwBytesPerSector;
   DWORD dwFreeClusters;
   DWORD dwTotalClusters;

   TCHAR szDriveRoot[] = SZ_ACOLONSLASH;

   DRIVESET(szDriveRoot, drive);

   if (GetDiskFreeSpace(szDriveRoot,
      &dwSectorsPerCluster,
      &dwBytesPerSector,
      &dwFreeClusters,
      &dwTotalClusters)) {

      *pqFreeSpace = TriMultiply(dwFreeClusters,dwSectorsPerCluster,dwBytesPerSector);
      *pqTotalSpace= TriMultiply(dwTotalClusters,dwSectorsPerCluster,dwBytesPerSector);

   } else {
      LARGE_INTEGER_NULL(*pqFreeSpace);
      LARGE_INTEGER_NULL(*pqTotalSpace);
   }
}


INT
ChangeVolumeLabel(DRIVE drive, LPTSTR lpNewVolName)
{
   TCHAR szDrive[] = SZ_ACOLON;

   DRIVESET(szDrive,drive);

   return (*lpfnSetLabel)(szDrive, lpNewVolName);
}


//
//  All references to szVolumeName should go through GetVolumelabel!
//  (To assure correct ] ending!)
//

DWORD
GetVolumeLabel(DRIVE drive, LPTSTR* ppszVol, BOOL bBrackets)
{
   U_VolInfo(drive);

   *ppszVol = aDriveInfo[drive].szVolNameMinusFour+4;

   if (GETRETVAL(VolInfo,drive) || !**ppszVol) {

      return GETRETVAL(VolInfo,drive);
   }

   (*ppszVol)[aDriveInfo[drive].dwVolNameMax] = CHAR_NULL;

   if (bBrackets) {

      (*ppszVol)--;
      (*ppszVol)[0] = CHAR_OPENBRACK;

      lstrcat(*ppszVol, SZ_CLOSEBRACK);
   }
   return ERROR_SUCCESS;
}


DWORD
FillVolumeInfo(DRIVE drive, LPTSTR lpszVolName, PDWORD pdwVolumeSerialNumber,
   PDWORD pdwMaximumComponentLength, PDWORD pdwFileSystemFlags,
   LPTSTR lpszFileSysName)
{
   TCHAR szDrive[] = SZ_ACOLONSLASH;
   PDRIVEINFO pDriveInfo = &aDriveInfo[drive];

   DRIVESET(szDrive,drive);

   if (!(GetVolumeInformation(szDrive,
      lpszVolName, COUNTOF(pDriveInfo->szVolNameMinusFour)-4,
      pdwVolumeSerialNumber,
      pdwMaximumComponentLength,
      pdwFileSystemFlags,
      lpszFileSysName, COUNTOF(pDriveInfo->szFileSysName)))) {

      lpszVolName[0] = CHAR_NULL;

      *pdwVolumeSerialNumber = 0;
      *pdwMaximumComponentLength = 0;
      *pdwFileSystemFlags = 0;

      lpszFileSysName[0] = CHAR_NULL;

      return GetLastError();
   }

   return ERROR_SUCCESS;
}
