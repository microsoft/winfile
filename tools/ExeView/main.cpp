//*************************************************************
//  File name: main.c
//
//  Description: 
//      WinMain and the WndProcs
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

#include "stdafx.h"

HINSTANCE   ghInst      = NULL;
HWND        ghWndMain   = NULL;

char        szMainMenu[]    = "MainMenu";
char        szMainClass[]   = "MainClass";
PEXEINFO    gpExeInfo;

//*************************************************************
//
//  WinMain()
//
//  Purpose:
//		Entry point for all windows apps
//
//
//  Parameters:
//      HANDLE hInstance
//      HANDLE hPrevInstance
//      LPSTR lpCmdLine
//      int nCmdShow
//      
//
//  Return: (int PASCAL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//              12/12/91   MSM        Created
//
//*************************************************************

INT APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    if (!hPrevInstance && !InitApplication(hInstance))
            return (FALSE);       

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    EnableChildWindows( ghWndMain, NULL );

    while (GetMessage(&msg, NULL, NULL, NULL))
    {
        TranslateMessage(&msg);      
        DispatchMessage(&msg);       
    }
    return (msg.wParam);      

} //*** WinMain()

//*************************************************************
//
//  MainWndProc()
//
//  Purpose:
//		Main Window procedure
//
//
//  Parameters:
//      HWND hWnd
//      unsigned msg
//      WORD wParam
//      LONG lParam
//      
//
//  Return: (long FAR PASCAL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//              12/12/91   MSM        Created
//
//*************************************************************

LRESULT CALLBACK MainWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DLGPROC lpProc;
    HWND    hLB = GetDlgItem( hWnd, IDL_EXEHDR );

    switch (msg) 
    {
        case WM_COMMAND: 
            switch ( LOWORD(wParam) )
            {
                case IDM_OPEN:
                {
                    OPENFILENAME of;
                    char         szFile[120];

                    memset( &of, 0, sizeof(OPENFILENAME) );

                    szFile[0] = 0;

                    of.lStructSize  = sizeof(OPENFILENAME);
                    of.hwndOwner    = ghWndMain;
                    of.hInstance    = ghInst;
                    of.lpstrFilter  = (LPSTR)"EXE file\0*.EXE\0DLL library\0*.DLL\0Font\0*.FON\0\0";
                    of.nFilterIndex = 0;
                    of.lpstrFile    = (LPSTR)szFile;
                    of.nMaxFile     = (DWORD)256;
                    of.lpstrTitle   = (LPSTR)"Enter File";
                    of.Flags        = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
                    of.lpstrDefExt  = (LPSTR)"EXE";

                    if( GetOpenFileName( &of ) )
                    {
                        // Kill the old exe info if it exists
                        if (gpExeInfo)
                        {
                            FreeExeInfoMemory( gpExeInfo );
                            gpExeInfo = NULL;
                        }

                        gpExeInfo = LoadExeInfo( szFile );

                        switch ( (int)gpExeInfo )
                        {
                            case LERR_OPENINGFILE:
                                MessageBox(hWnd,"Error opening file!",
                                    "ExeView Error", MB_ICONSTOP|MB_OK );
                                SetWindowText( hWnd, "Windows Executable Viewer" );
                                gpExeInfo = NULL;
                            break;

                            case LERR_NOTEXEFILE:
                                MessageBox(hWnd,"Not a valid EXE file!",
                                    "ExeView Error", MB_ICONSTOP|MB_OK );
                                SetWindowText( hWnd, "Windows Executable Viewer" );
                                gpExeInfo = NULL;
                            break;

                            case LERR_READINGFILE:
                                MessageBox(hWnd,"Error reading file!",
                                    "ExeView Error", MB_ICONSTOP|MB_OK );
                                SetWindowText( hWnd, "Windows Executable Viewer" );
                                gpExeInfo = NULL;
                            break;

                            case LERR_MEMALLOC:
                                MessageBox(hWnd,"Memory allocation denied!",
                                    "ExeView Error", MB_ICONSTOP|MB_OK );
                                SetWindowText( hWnd, "Windows Executable Viewer" );
                                gpExeInfo = NULL;
                            break;

                            default:
                            {
                                char szBuff[120];

                                if (gpExeInfo->NewHdr.wNewSignature)
                                    FillLBWithNewExeHeader(hLB, gpExeInfo );
                                else
                                    FillLBWithOldExeHeader( hLB, gpExeInfo );

                                wsprintf( szBuff, "ExeView - %s", (LPSTR)szFile );
                                SetWindowText( hWnd, szBuff );
                                SetFocus( hLB );
                            }
                            break;
                        }
                        EnableChildWindows( hWnd, gpExeInfo );
                        if (!gpExeInfo)
                            SendMessage( hLB, LB_RESETCONTENT, 0, 0L );
                    }
                }
                break;

                case IDM_ABOUT:
                    lpProc = MakeProcInstance(About, ghInst);
                    DialogBox(ghInst, "AboutBox", hWnd, lpProc);    
                    FreeProcInstance(lpProc);
                break;

                case IDM_EXIT:
                    PostMessage( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0L );
                break;

                case IDB_OLDHDR:
                    FillLBWithOldExeHeader(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_NEWHDR:
                    FillLBWithNewExeHeader(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_ENTRYTABLE:
                    FillLBWithEntryTable(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_SEGMENTS:
                    FillLBWithSegments(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_RESOURCES:
                    FillLBWithResources(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_RESIDENTNAMES:
                    FillLBWithResidentNames(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_IMPORTEDNAMES:
                    FillLBWithImportedNames(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDB_NONRESIDENTNAMES:
                    FillLBWithNonResidentNames(hLB, gpExeInfo );
                    SetFocus( hLB );
                break;

                case IDL_EXEHDR:
                {
                    int  nItem = (int)SendMessage( hLB, LB_GETCURSEL,0,0L );
                    LONG lData;

                    if (HIWORD(wParam)!=LBN_DBLCLK)
                        break;

                    if (nItem<0)
                        break;

                    lData = SendMessage( hLB, LB_GETITEMDATA,nItem,0L );

                    if (lData==NULL)
                        break;

                    return (LONG)DisplayResource(gpExeInfo, ((PRESINFO)lData)->pResType, (PRESINFO)lData);
                }
                break;

                case IDM_SAVERES:
                {
                    OPENFILENAME of;
                    char         szFile[120];

                    memset( &of, 0, sizeof(OPENFILENAME) );

                    szFile[0] = 0;

                    of.lStructSize  = sizeof(OPENFILENAME);
                    of.hwndOwner    = ghWndMain;
                    of.hInstance    = ghInst;
                    of.lpstrFilter  = (LPSTR)"Resource file\0*.rc\0\0";
                    of.nFilterIndex = 0;
                    of.lpstrFile    = (LPSTR)szFile;
                    of.nMaxFile     = (DWORD)sizeof(szFile);
                    of.lpstrTitle   = (LPSTR)"Resource File";
                    of.Flags        = OFN_HIDEREADONLY|OFN_PATHMUSTEXIST;
                    of.lpstrDefExt  = (LPSTR)"rc";

                    if (GetSaveFileName(&of))
                    {
                        SaveResources(hWnd, gpExeInfo, szFile);
                    }
                }
                break;
            }
        break;

        case WM_SIZE:
            ResizeChildWindows( hWnd );
        break;

        case WM_DESTROY:
            if (gpExeInfo)
            {
                FreeExeInfoMemory( gpExeInfo );
                gpExeInfo = NULL;
            }
            PostQuitMessage(0);
        break;
    }
    return (DefWindowProc(hWnd, msg, wParam, lParam));

} //*** MainWndProc()

//*************************************************************
//
//  About()
//
//  Purpose:
//		the About dialog box procedure
//
//
//  Parameters:
//      HWND hDlg
//      unsigned msg
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
//              12/12/91   MSM        Created
//
//*************************************************************

INT_PTR CALLBACK About (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
        case WM_INITDIALOG: 
            return (TRUE);

        case WM_COMMAND:
            if (wParam == IDOK || wParam == IDCANCEL) 
            {
                EndDialog(hDlg, TRUE);         
                return (TRUE);
            }
        break;
    }
    return (FALSE);                  /* Didn't process a message    */

} //*** About()

//*************************************************************
//
//  ResizeChildWindows
//
//  Purpose:
//      Resizes the child windows after the parent has changed size
//
//
//  Parameters:
//      HWND hWnd
//      
//
//  Return: (VOID)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

VOID ResizeChildWindows ( HWND hMainWnd )
{
    HWND hWnd = GetDlgItem( hMainWnd, IDB_OLDHDR );
    RECT rc;
    int  cx = GetSystemMetrics( SM_CXFRAME );
    int  cy = GetSystemMetrics( SM_CYFRAME );
    int  x, y, h, w;
    TEXTMETRIC  tm;

    // cx and cy are used as spacing between controls and are calculated
    // based of a system metric for device independence.

    if (!hWnd)      // Child windows haven't been created yet
        return;
    else            // Get info for button height
    {
        HDC         hDC = GetDC( hWnd );

        GetTextMetrics( hDC, &tm );
        ReleaseDC( hWnd, hDC );
    }

    // Button height will be 25% bigger than the text height
    x = cx;
    y = cy;
    h = (tm.tmHeight * 6) / 4;
    w = tm.tmAveCharWidth * 15;
    
    GetClientRect( hMainWnd, &rc );

    // Turn redraw off during sizing
    SendMessage( hMainWnd, WM_SETREDRAW, 0, 0L );

    // Resize listbox
    if (hWnd=GetDlgItem(hMainWnd,IDL_EXEHDR))
        MoveWindow(hWnd, x, y, rc.right-2*cx, rc.bottom-4*cy-2*h, FALSE);

    y = rc.bottom - 2*h - 2*cy;

    // Resize buttons
    if (hWnd=GetDlgItem(hMainWnd,IDB_OLDHDR))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    x += w+cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_NEWHDR))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    x += w+cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_ENTRYTABLE))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    x += w+cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_SEGMENTS))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    x += w+cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_RESOURCES))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    // Do second row
    w = tm.tmAveCharWidth * 25;
    y += h+cy;
    x = cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_RESIDENTNAMES))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    x += w+2*cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_IMPORTEDNAMES))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    x += w+2*cy;
    if (hWnd=GetDlgItem(hMainWnd,IDB_NONRESIDENTNAMES))
        MoveWindow(hWnd, x, y, w, h, FALSE);

    SendMessage( hMainWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hMainWnd, NULL, TRUE );
    UpdateWindow( hMainWnd );

} //*** ResizeChildWindows

//*************************************************************
//
//  EnableChildWindows
//
//  Purpose:
//      Enables/Disables child windows based on the EXE type
//
//
//  Parameters:
//      HWND hMainWnd
//      
//
//  Return: (VOID)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//
//*************************************************************

VOID EnableChildWindows ( HWND hMainWnd, PEXEINFO pExeInfo )
{
    #define CWIN(X) GetDlgItem(hMainWnd, X)

    if (pExeInfo==NULL)     // No exe info at all
    {
        EnableWindow( CWIN(IDB_OLDHDR), FALSE );
        EnableWindow( CWIN(IDB_NEWHDR), FALSE );
        EnableWindow( CWIN(IDB_ENTRYTABLE), FALSE );
        EnableWindow( CWIN(IDB_SEGMENTS), FALSE );
        EnableWindow( CWIN(IDB_RESOURCES), FALSE );
        EnableWindow( CWIN(IDB_RESIDENTNAMES), FALSE );
        EnableWindow( CWIN(IDB_IMPORTEDNAMES), FALSE );
        EnableWindow( CWIN(IDB_NONRESIDENTNAMES), FALSE );
        return;
    }

    if (pExeInfo->NewHdr.wNewSignature==0)  // Old info only
    {
        EnableWindow( CWIN(IDB_OLDHDR), TRUE);
        EnableWindow( CWIN(IDB_NEWHDR), FALSE );
        EnableWindow( CWIN(IDB_ENTRYTABLE), FALSE );
        EnableWindow( CWIN(IDB_SEGMENTS), FALSE );
        EnableWindow( CWIN(IDB_RESOURCES), FALSE );
        EnableWindow( CWIN(IDB_RESIDENTNAMES), FALSE );
        EnableWindow( CWIN(IDB_IMPORTEDNAMES), FALSE );
        EnableWindow( CWIN(IDB_NONRESIDENTNAMES), FALSE );
    }
    else                                    // Old and new
    {
        EnableWindow( CWIN(IDB_OLDHDR), TRUE);
        EnableWindow( CWIN(IDB_NEWHDR), TRUE );
        EnableWindow( CWIN(IDB_ENTRYTABLE), TRUE );
        EnableWindow( CWIN(IDB_SEGMENTS), TRUE );

        if (pExeInfo->pResTable)
            EnableWindow( CWIN(IDB_RESOURCES), TRUE );
        else
            EnableWindow( CWIN(IDB_RESOURCES), FALSE );

        EnableWindow( CWIN(IDB_RESIDENTNAMES), TRUE );
        EnableWindow( CWIN(IDB_IMPORTEDNAMES), TRUE );
        EnableWindow( CWIN(IDB_NONRESIDENTNAMES), TRUE );
    }

} //*** EnableChildWindows

//*** EOF: main.c
