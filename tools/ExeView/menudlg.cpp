//*************************************************************
//  File name: menudlg.c
//
//  Description: 
//      Code for displaying Menus and Dialogs
//
//  History:    Date       Author     Comment
//               1/24/92   MSM        Created
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

#include "stdafx.h"


//*************************************************************
//
//  ShowMenu
//
//  Purpose:
//      Opens a popup window and displays the menu    
//
//
//  Parameters:
//      LPRESPACKET lprp
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//
//*************************************************************

BOOL ShowMenu (LPRESPACKET lprp)
{
    HMENU hMenu = LoadMenuIndirect( lprp->lpMem );

    if (hMenu)
    {
        HWND hWnd;
        MSG  msg;

        hWnd = CreateWindow( "MENUPOPUP", "Viewing Menu",  WS_POPUPWINDOW|
            WS_CAPTION|WS_THICKFRAME|WS_VISIBLE, 30, 31, 300, 100,
            ghWndMain, hMenu, ghInst, NULL );

        if (!hWnd)
        {
            DestroyMenu( hMenu );
            MessageBox(ghWndMain,"Could not create view window!","EXEVIEW",MB_OK);
            return TRUE;
        }

        // Give the modal look
        EnableWindow( ghWndMain, FALSE );

        while ( IsWindow(hWnd) )    // While the popup is displayed
        {
            if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
        EnableWindow( ghWndMain, TRUE );

        // Menu gets destroyed with the window
        return TRUE;
    }
    else
        MessageBox(ghWndMain,"Could not create menu!","EXEVIEW",MB_OK);

    return TRUE;

} //*** ShowMenu

//*************************************************************
//
//  ShowMenuProc
//
//  Purpose:
//      Displays an menu for all the world to see
//
//
//  Parameters:
//      HWND hWnd
//      WORD msg
//      WORD wParam
//      LONG lParam
//      
//
//  Return: (LONG FAR PASCAL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/23/92   MSM        Created
//
//*************************************************************

LRESULT FAR PASCAL ShowMenuProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Release MODAL appearance
    if (msg==WM_CLOSE)
        EnableWindow( ghWndMain, TRUE );

    return DefWindowProc( hWnd, msg, wParam, lParam );

} //*** ShowMenuProc

//*************************************************************
//
//  ShowDialogProc
//
//  Purpose:
//      Dialog box procedure for displaying a dialog
//
//
//  Parameters:
//      HWND hDlg
//      WORD msg
//      WORD wParam
//      LONG lParam
//      
//
//  Return: (BOOL FAR PASCAL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/21/92   MSM        Created
//
//*************************************************************

INT_PTR FAR PASCAL ShowDialogProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return TRUE;
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** ShowDialogProc

//*** EOF: menudlg.c
