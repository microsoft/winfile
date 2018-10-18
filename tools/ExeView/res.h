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
    HANDLE      hMem;
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
    WORD    xHotSpot;
    WORD    yHotSpot;
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

#define MAXDLGITEMS 100

#pragma pack(push, 1)
typedef struct {
    WORD style;
    WORD extra;
    BYTE cdit;
    short x;
    short y;
    short cx;
    short cy;
    WORD flags;
} DLGTEMPLATE16;

typedef struct {
    short x;
    short y;
    short cx;
    short cy;
    short id;
    WORD style;
    WORD extra;
    BYTE kind;
} DLGITEMTEMPLATE16;
#pragma pack(pop)

// one dialog item; pointers are into the locked resource memory
typedef struct
{
    DLGITEMTEMPLATE16 *pitem;

    LPCTSTR lpszTitle;
    WORD iconid;

    LPCTSTR lpszSecond;
} DLGITEMDECODE;

// decoded dialog as a whole; pointers are into locked resource memory
typedef struct
{
    DLGTEMPLATE16 *pdlg;

    LPCTSTR lpszTitle;

    // if DS_SETFONT
    WORD wFont;        // size in points
    LPCTSTR lpszFont;

    // count of items is in pdlg->cdit
    DLGITEMDECODE rgitems[MAXDLGITEMS];
} DLGDECODE;

#define MAXMENUITEMS 100

#define MFR_END 0x80

// used for both normal and popup items
typedef struct
{
    WORD flags;
    WORD id;
    LPCTSTR lpszMenu;
} MENUITEM;

// decoded menu; pointers are into locked resource memory
typedef struct
{
    WORD flags;
    LPCTSTR lpszTitle;

    int cItem;
    MENUITEM rgitem[MAXMENUITEMS];
} MENUDECODE;

//*** function prototypes
//*** bmp.c
BOOL ShowBitmap (LPRESPACKET );
LRESULT FAR PASCAL ShowBitmapProc (HWND, UINT, WPARAM, LPARAM);
HBITMAP MakeBitmap (LPRESPACKET );
HPALETTE CreateNewPalette  (LPSTR, WORD, WORD);

//*** res.c
BOOL LoadResourcePacket(PEXEINFO pExeInfo, PRESTYPE prt, PRESINFO pri, LPRESPACKET lprp);
VOID FreeResourcePacket(LPRESPACKET lprp);
BOOL DisplayResource (PEXEINFO, PRESTYPE, PRESINFO );

BOOL FillLBWithNameTable (HWND, LPRESPACKET);
INT_PTR FAR PASCAL NameTableProc (HWND, UINT, WPARAM, LPARAM);

BOOL FillLBWithIconGroup (HWND, LPRESPACKET);
INT_PTR FAR PASCAL IconGroupProc (HWND, UINT, WPARAM, LPARAM);
            
BOOL FillLBWithCursorGroup (HWND, LPRESPACKET);
INT_PTR FAR PASCAL CursorGroupProc (HWND, UINT, WPARAM, LPARAM);

BOOL FillLBWithAccelTable (HWND, LPRESPACKET);
INT_PTR FAR PASCAL AccelTableProc (HWND, UINT, WPARAM, LPARAM);

BOOL FillLBWithStringTable (HWND, LPRESPACKET);
INT_PTR FAR PASCAL StringTableProc (HWND, UINT, WPARAM, LPARAM);

BOOL FillLBWithFontDir (HWND, LPRESPACKET);
INT_PTR FAR PASCAL FontDirProc (HWND, UINT, WPARAM, LPARAM);


//*** iconcur.c
INT_PTR FAR PASCAL ShowIconProc (HWND, UINT, WPARAM, LPARAM);
HICON   MakeIcon ( LPRESPACKET );

INT_PTR FAR PASCAL ShowCursorProc (HWND, UINT, WPARAM, LPARAM);
HCURSOR MakeCursor ( LPRESPACKET );

//*** menudlg.c
BOOL ShowMenu (LPRESPACKET );
LRESULT FAR PASCAL ShowMenuProc (HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL ShowDialogProc (HWND, UINT, WPARAM, LPARAM);


//*** EOF: res.h
