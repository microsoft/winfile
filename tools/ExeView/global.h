//*************************************************************
//  File name: global.h
//
//  Description: 
//      Global include file for #defines and prototypes
//
//  History:    Date       Author     Comment
//              12/31/91   MSM        Created
//                 
// Written by Microsoft Product Support Services, Windows Developer Support
// Copyright (c) 1992 Microsoft Corporation. All rights reserved.
//*************************************************************


#include <windows.h>
#include <commdlg.h>
#include <memory.h>

#include "exehdr.h"
#include "res.h"



//*** Menu Defines
    #define IDM_OPEN                0x50
    #define IDM_ABOUT               0x51
    #define IDM_EXIT                0x52

//*** Control defines
    #define IDL_EXEHDR              0x100
    #define IDB_OLDHDR              0x101
    #define IDB_NEWHDR              0x102
    #define IDB_ENTRYTABLE          0x103
    #define IDB_SEGMENTS            0x104
    #define IDB_RESOURCES           0x105
    #define IDB_RESIDENTNAMES       0x106
    #define IDB_IMPORTEDNAMES       0x107
    #define IDB_NONRESIDENTNAMES    0x108



//*** Function Prototypes
//*** Main.c
    int PASCAL WinMain (HANDLE, HANDLE, LPSTR, int);
    long FAR PASCAL MainWndProc (HWND, UINT, WPARAM, LPARAM);
    BOOL FAR PASCAL About (HWND, unsigned, WORD, LONG);
    VOID            ResizeChildWindows ( HWND hMainWnd );
    VOID            EnableChildWindows ( HWND, PEXEINFO);

//*** Init.c
    BOOL InitApplication (HANDLE);
    BOOL InitInstance (HANDLE, int);


//*** EXTERNS for Global Variables
    extern HANDLE       ghInst;
    extern HWND         ghWndMain;

    extern char         szMainMenu[];
    extern char         szMainClass[];
    extern PEXEINFO     gpExeInfo;

//*** EOF: global.h
