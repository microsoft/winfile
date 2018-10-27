//*************************************************************
//  File name: exehdr.h
//
//  Description: 
//      Structures for reading the exe headers and tables
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
// Written by Microsoft Product Support Services, Windows Developer Support
// Copyright (c) 1992 Microsoft Corporation. All rights reserved.
//*************************************************************

// Structures and definitions used by this program can be
// found in the Programmer's PC sourcebook, the MS-DOS Encyclopedia
// from MS Press and the Sept. 1991 issue of the Microsoft Systems Journal

typedef struct
{
    WORD    wFileSignature;         // 0x5A4D
    WORD    wLengthMod512;          // bytes on last page
    WORD    wLength;                // 512 byte pages
    WORD    wRelocationTableItems;  
    WORD    wHeaderSize;            // Paragraphs
    WORD    wMinAbove;              // Paragraphs
    WORD    wDesiredAbove;          // Paragraphs
    WORD    wStackDisplacement;     // Paragraphs
    WORD    wSP;                    // On entry
    WORD    wCheckSum;
    WORD    wIP;                    // On entry
    WORD    wCodeDisplacement;      // Paragraphs
    WORD    wFirstRelocationItem;   // Offset from beginning
    WORD    wOverlayNumber;
    WORD    wReserved[ 16 ];
    LONG    lNewExeOffset;          
} OLDEXE, *POLDEXE;

typedef struct
{
    WORD  wNewSignature;    // 0x454e
    char  cLinkerVer;       // Version number 
    char  cLinkerRev;       // Revision number 
    WORD  wEntryOffset;     // Offset to Entry Table
    WORD  wEntrySize;       // Number of bytes in Entry Table
    long  lChecksum;        // 32 bit check sum for the file
    WORD  wFlags;           // Flag word 
    WORD  wAutoDataSegment; // Seg number for automatic data seg
    WORD  wHeapInit;        // Initial heap allocation; 0 for no heap
    WORD  wStackInit;       // Initial stack allocation; 0 for libraries
    WORD  wIPInit;          // Initial IP setting 
    WORD  wCSInit;          // Initial CS segment number
    WORD  wSPInit;          // Initial SP setting 
    WORD  wSSInit;          // Initial SS segment number
    WORD  wSegEntries;      // Count of segment table entries
    WORD  wModEntries;      // Entries in Module Reference Table 
    WORD  wNonResSize;      // Size of non-resident name table (bytes)
    WORD  wSegOffset;       // Offset of Segment Table 
    WORD  wResourceOffset;  // Offset of Resource Table 
    WORD  wResOffset;       // Offset of resident name table 
    WORD  wModOffset;       // Offset of Module Reference Table 
    WORD  wImportOffset;    // Offset of Imported Names Table 
    long  lNonResOffset;    // Offset of Non-resident Names Table
                            // THIS FIELD IS FROM THE BEGINNING OF THE FILE
                            // NOT THE BEGINNING OF THE NEW EXE HEADER
    WORD  wMoveableEntry;   // Count of movable entries in entry table
    WORD  wAlign;           // Segment alignment shift count
    WORD  wResourceSegs;    // Count of resource segments
    BYTE  bExeType;         // Operating System flags  
    BYTE  bAdditionalFlags; // Additional exe flags 
    WORD  wFastOffset;      // offset to FastLoad area 
    WORD  wFastSize;        // length of FastLoad area 
    WORD  wReserved;
    WORD  wExpVersion;      // Expected Windows version number 
} NEWEXE, *PNEWEXE;

#define OLDSIG          0x5a4d
#define NEWSIG          0x454e
#define SINGLEDATA      0x0001
#define MULTIPLEDATA    0x0002
#define PMODEONLY       0x0008
#define LIBRARY         0x8000
#define FASTLOAD        0x0008



typedef struct
{
    BYTE    bFlags;
    WORD    wSegOffset;
} FENTRY, *PFENTRY;

typedef struct
{
    BYTE    bFlags;
    WORD    wINT3F;
    BYTE    bSegNumber;
    WORD    wSegOffset;
} MENTRY, *PMENTRY;

#define EXPORTED    0x01
#define SHAREDDATA  0x02


typedef struct
{
    WORD    wSector;
    WORD    wLength;
    WORD    wFlags;
    WORD    wMinAlloc;
} SEGENTRY, *PSEGENTRY;

#define F_DATASEG       0x0001
#define F_MOVEABLE      0x0010
#define F_SHAREABLE     0x0020
#define F_PRELOAD       0x0040
#define F_DISCARDABLE   0x1000



// The RTYPE and RINFO structures are never actually used
// they are just defined for use in the sizeof() macro when
// reading the info off the disk.  The actual data is read
// into the RESTYPE and RESINFO structures that contain these
// structures with some extra information declared at the end.

typedef struct
{
    WORD    wType;
    WORD    wCount;
    LONG    lReserved;
} RTYPE;

typedef struct
{
    WORD    wOffset;
    WORD    wLength;
    WORD    wFlags;
    WORD    wID;
    LONG    lReserved;
} RINFO;

// RESINFO2 is the same structure as RINFO with one modification.
// RESINFO2 structure includes a pointer to the containing type.

typedef struct tgRESTYPE *PRESTYPE;

typedef struct
{
    WORD     wOffset;
    WORD     wLength;
    WORD     wFlags;
    WORD     wID;
    PSTR     pResourceName;
    // NOTE: RINFO (what is in the file) matches the fields up to this point
    PRESTYPE pResType;
} RESINFO2, *PRESINFO;

typedef struct tgRESTYPE
{
    WORD        wType;              // Resource type
    WORD        wCount;             // Specifies ResInfoArray size
    LONG        lReserved;          // Reserved for runtime use
    // NOTE: RTYPE (what is in the file) matches the fields up to this point
    PSTR        pResourceType;      // Points to custom type name
    PRESINFO    pResInfoArray;      // First entry in array
    PRESTYPE    pNext;              // Next Resource type
} RESTYPE;

#define GROUP_CURSOR    12
#define GROUP_ICON      14
#define NAMETABLE       15



typedef struct tgNAME
{
    struct tgNAME  *pNext;
    WORD            wOrdinal;
    char            szName[1];      // Text goes here at allocation time
} NAME, *PNAME;




typedef struct
{
    PSTR        pFilename;          // File name
    OLDEXE      OldHdr;             // Old EXE header
    NEWEXE      NewHdr;             // New EXE header

    PSTR        pEntryTable;        // Points to mem that holds entry table
    PSEGENTRY   pSegTable;          // Pointer to first entry in ARRAY
    WORD        wShiftCount;        // Shift count for the resource table
    PRESTYPE    pResTable;          // Pointer to first entry in LIST

    PNAME       pResidentNames;     // Points to first entry in LIST
    PNAME       pImportedNames;     // Points to first entry in LIST
    PNAME       pNonResidentNames;  // Points to first entry in LIST

} EXEINFO, *PEXEINFO;




//*** Function prototypes
//*** exehdr.c
    PEXEINFO    LoadExeInfo(LPSTR);
        #define LERR_OPENINGFILE    -1
        #define LERR_NOTEXEFILE     -2
        #define LERR_READINGFILE    -3
        #define LERR_MEMALLOC       -4

    VOID        FreeExeInfoMemory (PEXEINFO);

    int         ReadSegmentTable (int, PEXEINFO );
    int         ReadResourceTable (int, PEXEINFO );
    int         ReadResidentNameTable (int, PEXEINFO );
    int         ReadImportedNameTable (int, PEXEINFO );
    int         ReadNonResidentNameTable (int, PEXEINFO );

    PSEGENTRY   GetSegEntry (PEXEINFO, int );
    LPSTR       FormatSegEntry (PEXEINFO, PSEGENTRY, LPSTR );

    PRESTYPE    GetResourceType (PEXEINFO, int );
    PRESINFO    GetResourceInfo (PRESTYPE, int );
    LPSTR       FormatResourceEntry (PEXEINFO, PRESINFO, LPSTR );

    LPSTR       GetModuleName (PEXEINFO );
    LPSTR       GetModuleDescription (PEXEINFO );
    PNAME       GetResidentName (PEXEINFO, int );
    PNAME       GetImportedName (PEXEINFO, int );
    PNAME       GetNonResidentName (PEXEINFO, int );


//*** filllb.c
    BOOL        FillLBWithOldExeHeader (HWND, PEXEINFO );
    BOOL        FillLBWithNewExeHeader (HWND, PEXEINFO );
    BOOL        FillLBWithEntryTable (HWND, PEXEINFO );
    BOOL        FillLBWithSegments (HWND, PEXEINFO );
    BOOL        FillLBWithResources (HWND, PEXEINFO );
    BOOL        FillLBWithResidentNames (HWND, PEXEINFO );
    BOOL        FillLBWithImportedNames (HWND, PEXEINFO );
    BOOL        FillLBWithNonResidentNames (HWND, PEXEINFO );
    LPSTR       GetExeDataType (PEXEINFO);


//*** save.c
    BOOL        SaveResources(HWND, PEXEINFO, LPSTR);

//*** EOF: exehdr.h
