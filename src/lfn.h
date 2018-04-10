/********************************************************************

   lfn.h

   declaration of lfn aware functions

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#define CCHMAXFILE      MAXPATHLEN         // max size of a long name
#define CCHMAXPATHCOMP  256
#define LFNCANON_MASK           1
#define PATHLEN1_1 13

#define FILE_83_CI  0
#define FILE_83_CS  1
#define FILE_LONG   2

#define ERROR_OOM   8

// we need to add an extra field to distinguish DOS vs. LFNs

typedef struct {
   HANDLE hFindFile;           // handle returned by FindFirstFile()
   DWORD dwAttrFilter;         // search attribute mask.
   DWORD err;                  // error info if failure.
   WIN32_FIND_DATA fd;         // FindFirstFile() data structure;
   INT   nSpaceLeft;           // Space left for deeper paths
} LFNDTA, *LPLFNDTA, * PLFNDTA;

VOID  LFNInit( VOID );
VOID  InvalidateVolTypes( VOID );

DWORD  GetNameType(LPTSTR);
BOOL  IsLFN(LPTSTR pName);
// BOOL  IsLFNDrive(LPTSTR);

BOOL  WFFindFirst(LPLFNDTA lpFind, LPTSTR lpName, DWORD dwAttrFilter);
BOOL  WFFindNext(LPLFNDTA);
BOOL  WFFindClose(LPLFNDTA);


DWORD  I_LFNCanon( USHORT CanonType, LPTSTR InFile, LPTSTR OutFile );
DWORD  LFNParse(LPTSTR,LPTSTR,LPTSTR);
WORD I_LFNEditName( LPTSTR lpSrc, LPTSTR lpEd, LPTSTR lpRes, INT iResBufSize );

BOOL  WFIsDir(LPTSTR);
BOOL  LFNMergePath(LPTSTR,LPTSTR);

BOOL  IsLFNSelected(VOID);


