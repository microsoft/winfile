//*************************************************************
//  File name: init.c
//
//  Description: 
//      Initializes the app and instance
//
//  History:    Date       Author     Comment
//              12/12/91   MSM        Created
//
// Written by Microsoft Product Support Services, Windows Developer Support
// Copyright (c) 1992 Microsoft Corporation. All rights reserved.
//*************************************************************
// COPYRIGHT:
//
//   (C) Copyright Microsoft Corp. 1993.  All rights reserved.
//
//   You have a royalty-free right to use, modify, reproduce and
//   distribute the Sample Files (and/or any modified version) in
//   any way you find useful, provided that you agree that
//   Microsoft has no warranty obligations or liability for any
//   Sample Application Files which are modified.
//

#include "global.h"

//*************************************************************
//
//  InitApplication()
//
//  Purpose:
//		Initializes the application (window classes)
//
//
//  Parameters:
//      HANDLE hInstance - hInstance from WinMain
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//              12/12/91   MSM        Created
//
//*************************************************************

BOOL InitApplication (HANDLE hInstance)
{
    WNDCLASS  wc;

    wc.style = NULL;             
    wc.lpfnWndProc = MainWndProc;
                                 
    wc.cbClsExtra = 0;           
    wc.cbWndExtra = 0;           
    wc.hInstance = hInstance;    
    wc.hIcon = LoadIcon(hInstance, "MAINICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = COLOR_APPWORKSPACE+1;
    wc.lpszMenuName  = szMainMenu;  
    wc.lpszClassName = szMainClass;

    if ( !RegisterClass(&wc) )
        return FALSE;

    wc.lpfnWndProc = ShowMenuProc;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "MENUPOPUP";

    if ( !RegisterClass(&wc) )
        return FALSE;

    wc.lpfnWndProc = ShowBitmapProc;
    wc.lpszClassName = "BITMAPPOPUP";

    if ( !RegisterClass(&wc) )
        return FALSE;


    return TRUE;

} //*** InitApplication()

//*************************************************************
//
//  InitInstance()
//
//  Purpose:
//		Initializes each instance (window creation)
//
//
//  Parameters:
//      HANDLE hInstance
//      int nCmdShow
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//              12/12/91   MSM        Created
//
//*************************************************************

BOOL InitInstance (HANDLE hInstance, int nCmdShow)
{
    HWND hWnd;
    int  tabs = 36;

    ghInst = hInstance;

    ghWndMain = CreateWindow( szMainClass, "Windows Executable Viewer",  
        WS_OVERLAPPEDWINDOW,           
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL );

    if (!ghWndMain)
        return (FALSE);

    hWnd = CreateWindow( "LISTBOX", NULL, WS_CHILD|WS_VISIBLE|WS_BORDER|
        WS_VSCROLL|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY|LBS_USETABSTOPS,
        0,0,0,0,
        ghWndMain, IDL_EXEHDR, hInstance, NULL );

    if (!hWnd)
        return FALSE;

    SendMessage( hWnd, LB_SETTABSTOPS, 1, (LONG)(LPINT)&tabs );
    SendMessage( hWnd, WM_SETFONT, GetStockObject(SYSTEM_FIXED_FONT), 0L );

    hWnd = CreateWindow( "BUTTON", "Old Header", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_OLDHDR, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "New Header", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_NEWHDR, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "Entry Table", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_ENTRYTABLE, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "Segments", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_SEGMENTS, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "Resources", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_RESOURCES, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "Resident Names", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_RESIDENTNAMES, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "Imported Names", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_IMPORTEDNAMES, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    hWnd = CreateWindow( "BUTTON", "NonResident Names", WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, ghWndMain, IDB_NONRESIDENTNAMES, hInstance, NULL );
    if (!hWnd)
        return FALSE;

    ShowWindow(ghWndMain, nCmdShow);
    UpdateWindow(ghWndMain);
    return (TRUE);

} //*** InitInstance()

//*** EOF: init.c
