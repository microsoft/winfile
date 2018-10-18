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

    #define IDM_SAVERES             0x60

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
    int APIENTRY WinMain (HINSTANCE, HINSTANCE, LPSTR, int);
    LRESULT CALLBACK MainWndProc (HWND, UINT, WPARAM, LPARAM);
    INT_PTR CALLBACK About (HWND, UINT, WPARAM, LPARAM);
    VOID            ResizeChildWindows ( HWND hMainWnd );
    VOID            EnableChildWindows ( HWND, PEXEINFO);

//*** Init.c
    BOOL InitApplication (HINSTANCE);
    BOOL InitInstance (HINSTANCE, int);


//*** EXTERNS for Global Variables
    extern HINSTANCE    ghInst;
    extern HWND         ghWndMain;

    extern char         szMainMenu[];
    extern char         szMainClass[];
    extern PEXEINFO     gpExeInfo;

//*** EOF: global.h
