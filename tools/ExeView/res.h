//*************************************************************
//  File name: res.h
//
//  Description: 
//      header file for resources and resource types
//
//  History:    Date       Author     Comment
//               1/21/92   MSM        Created
//
// Written by Microsoft Product Support Services, Windows Developer Support
// Copyright (c) 1992 Microsoft Corporation. All rights reserved.
//*************************************************************


//*** Dialog control IDs
#define IDL_NAMES       0x100    





//*** Structure definitions
typedef struct
{
    PEXEINFO    pExeInfo;
    PRESTYPE    prt;
    PRESINFO    pri;
    LONG        lSize;
    LPSTR       lpMem;
    int         fFile;
} RESPACKET, FAR *LPRESPACKET;

typedef struct
{
    WORD    wBytes;
    WORD    wTypeOrd;
    WORD    wIDOrd;
//  char    szType[];
//  char    szID[];

} NAMEENTRY, FAR *LPNAMEENTRY;

typedef struct
{
    BYTE    bWidth;         // Width in pixels
    BYTE    bHeight;        // Height in pixels
    BYTE    bColorCount;    // Number of colors
    BYTE    bReserved;      // Reserved
    WORD    wPlanes;        // Number of color planes
    WORD    wBitsPerPel;    // Number of bits per pixel    
    DWORD   dwBytesInRes;   // size of resource in bytes
    WORD    wOrdinalNumber; // Ordinal value
} ICONENTRY, FAR *LPICONENTRY;

typedef struct
{
    WORD    wReserved;      // reserved, 0
    WORD    wType;          // Type (3 for icons)
    WORD    wCount;         // Number of Icons in the Icons[] array
    ICONENTRY Icons[1];     // More than 1 at run-time, but this
                            // Sets up the array header for the compiler
} ICONDIR, FAR *LPICONDIR;

typedef struct
{
    WORD    wWidth;         // Width in pixels
    WORD    wHeight;        // Height in pixels
    WORD    wPlanes;        // Number of color planes
    WORD    wBitsPerPel;    // Number of bits per pixel
    DWORD   dwBytesInRes;   // size of resource in bytes
    WORD    wOrdinalNumber; // Ordinal value
} CURSORENTRY, FAR *LPCURSORENTRY;

typedef struct
{
    WORD    wReserved;      // reserved, 0
    WORD    wType;          // Type (3 for icons)
    WORD    wCount;         // Number of Icons in the Icons[] array
    CURSORENTRY Cursors[1]; // More than 1 at run-time, but this
                            // Sets up the array header for the compiler
} CURSORDIR, FAR *LPCURSORDIR;

typedef struct
{
    int         xHotSpot;
    int         yHotSpot;
    BITMAPINFO  bi;
} CURSORIMAGE, FAR *LPCURSORIMAGE;



typedef struct
{
    BYTE    fFlags;
    WORD    wEvent;
    WORD    wID;
} ACCELENTRY, FAR *LPACCELENTRY;

#define AF_VIRTKEY  0x01
#define AF_NOINVERT 0x02
#define AF_SHIFT    0x04    
#define AF_CONTROL  0x08
#define AF_ALT      0x10

typedef struct
{
    WORD    fontOrdinal;
    WORD    dfVersion;
    DWORD   dfSize;
    char    dfCopyright[60];
    WORD    dfType;
    WORD    dfPoints;
    WORD    dfVertRes;
    WORD    dfHorzRes;
    WORD    dfAscent;
    WORD    dfIntLeading;
    WORD    dfExtLeading;
    BYTE    dfItalic;
    BYTE    dfUnderline;
    BYTE    dfStrikeOut;
    WORD    dfWeight;
    BYTE    dfCharSet;
    WORD    dfPixWidth;
    WORD    dfPixHeight;
    BYTE    dfPitchAndFamily;
    WORD    dfAvgWidth;
    WORD    dfMaxWidth;
    BYTE    dfFirstChar;
    BYTE    dfLastChar;
    BYTE    dfDefaultChar;
    BYTE    dfBreakChar;
    WORD    dfWidthBytes;
    DWORD   dfDevice;
    DWORD   dfFace;
    DWORD   dfReserved;
    char    szDeviceName[1];

} FONTENTRY, FAR *LPFONTENTRY;


//*** function prototypes
//*** bmp.c
BOOL ShowBitmap (LPRESPACKET );
LONG FAR PASCAL ShowBitmapProc (HWND, WORD, WORD, LONG);
HBITMAP MakeBitmap (LPRESPACKET );
HPALETTE CreateNewPalette  (LPSTR, WORD, WORD);

//*** res.c
BOOL DisplayResource (PEXEINFO, PRESTYPE, PRESINFO );

BOOL FillLBWithNameTable (HWND, LPRESPACKET);
BOOL FAR PASCAL NameTableProc (HWND, WORD, WORD, LONG);

BOOL FillLBWithIconGroup (HWND, LPRESPACKET);
BOOL FAR PASCAL IconGroupProc (HWND, WORD, WORD, LONG);
            
BOOL FillLBWithCursorGroup (HWND, LPRESPACKET);
BOOL FAR PASCAL CursorGroupProc (HWND, WORD, WORD, LONG);

BOOL FillLBWithAccelTable (HWND, LPRESPACKET);
BOOL FAR PASCAL AccelTableProc (HWND, WORD, WORD, LONG);

BOOL FillLBWithStringTable (HWND, LPRESPACKET);
BOOL FAR PASCAL StringTableProc (HWND, WORD, WORD, LONG);

BOOL FillLBWithFontDir (HWND, LPRESPACKET);
BOOL FAR PASCAL FontDirProc (HWND, WORD, WORD, LONG);


//*** iconcur.c
BOOL FAR PASCAL ShowIconProc (HWND, WORD, WORD, LONG);
HICON   MakeIcon ( LPRESPACKET );

BOOL FAR PASCAL ShowCursorProc (HWND, WORD, WORD, LONG);
HCURSOR MakeCursor ( LPRESPACKET );

//*** menudlg.c
BOOL ShowMenu (LPRESPACKET );
LONG FAR PASCAL ShowMenuProc (HWND, WORD, WORD, LONG);
BOOL FAR PASCAL ShowDialogProc (HWND, WORD, WORD, LONG);


//*** EOF: res.h
