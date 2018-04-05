/********************************************************************

   winexp.h

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#ifndef NOATOM
/* atom manager internals */
#define ATOMSTRUC struct atomstruct
typedef ATOMSTRUC *PATOM;
typedef ATOMSTRUC {
    PATOM chain;
    WORD  usage;             /* Atoms are usage counted. */
    BYTE  len;               /* length of ASCIZ name string */
    BYTE  name;              /* beginning of ASCIZ name string */
} ATOMENTRY;

typedef struct {
    int     numEntries;
    PATOM   pAtom[ 1 ];
} ATOMTABLE;
ATOMTABLE * PASCAL pAtomTable;
#endif

LPTSTR FAR PASCAL lstrbscan(LPTSTR, LPTSTR);
LPTSTR FAR PASCAL lstrbskip(LPTSTR, LPTSTR);

int  FAR PASCAL OpenPathName(LPTSTR, int);
int  FAR PASCAL DeletePathName(LPTSTR);
WORD FAR PASCAL _ldup(int);


/* scheduler things that the world knows not */
BOOL        far PASCAL WaitEvent( HANDLE );
BOOL        far PASCAL PostEvent( HANDLE );
BOOL        far PASCAL KillTask( HANDLE );

/* print screen hooks */
BOOL        FAR PASCAL SetPrtScHook(FARPROC);
FARPROC     FAR PASCAL GetPrtScHook(void);

/* module stuff */
HANDLE  FAR PASCAL GetDSModule( WORD );
HANDLE  FAR PASCAL GetDSInstance( WORD );
