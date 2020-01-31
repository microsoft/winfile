//*************************************************************
//  File name: Bitmapc
//
//  Description: 
//      Code for displaying Bitmaps
//
//  History:    Date       Author     Comment
//               1/25/92   MSM        Created
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
//  ShowBitmap
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

BOOL ShowBitmap (LPRESPACKET lprp)
{
    HWND hWnd;
    MSG  msg;

    hWnd = CreateWindow( "BITMAPPOPUP", "Viewing Bitmap",  WS_POPUPWINDOW|
        WS_CAPTION|WS_THICKFRAME|WS_VISIBLE, 0, 0, 400, 300,
        ghWndMain, NULL, ghInst, (LPSTR)lprp );

    if (!hWnd)
    {
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

    return TRUE;

} //*** ShowBitmap

//*************************************************************
//
//  ShowBitmapProc
//
//  Purpose:
//      Displays an Bitmap for all the world to see
//
//
//  Parameters:
//      HWND hWnd
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
//               1/25/92   MSM        Created
//
//*************************************************************

LRESULT FAR PASCAL ShowBitmapProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hBitmap = NULL;

    switch (msg)
    {
        case WM_CREATE:
        {
            LPRESPACKET lprp;
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;

            lprp = (LPRESPACKET)lpcs->lpCreateParams;
            hBitmap = MakeBitmap( lprp );
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint( hWnd, &ps );

            if (hBitmap)
            {
                RECT rc;
                HDC  hDC = CreateCompatibleDC( ps.hdc );
                HBITMAP hBmp;

                GetClientRect( hWnd, &rc );
                hBmp = (HBITMAP)SelectObject( hDC, hBitmap );

                BitBlt(ps.hdc,0,0,rc.right,rc.bottom,hDC,0,0,SRCCOPY);
                SelectObject( hDC, hBmp );
                DeleteDC( hDC );
            }
            else
                TextOut( ps.hdc, 10, 10, "Bitmap not created!", 19 );
            EndPaint( hWnd, &ps );
            return 1L;
        }
        break;

        case WM_CLOSE:
            EnableWindow( ghWndMain, TRUE );
        break;        

        case WM_DESTROY:
            if (hBitmap)
                DeleteObject( hBitmap );
        break;
    }
    return DefWindowProc( hWnd, msg, wParam, lParam );

} //*** ShowBitmapProc

//*************************************************************
//
//  MakeBitmap
//
//  Purpose:
//      Attempts to create an Bitmap based on the info passed in
//
//
//  Parameters:
//      LPRESPACKET lprp
//      
//
//  Return: (HBITMAP)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/25/92   MSM        Created
//  
//*************************************************************

HBITMAP MakeBitmap ( LPRESPACKET lprp )
{
    LPBITMAPINFOHEADER  lpbihdr;
    LPBITMAPCOREHEADER  lpbchdr;
    LPSTR    lpDIBHdr, lpDIBBits;
    HBITMAP  hBitmap;
    HDC      hDC;
    WORD     wBitCount, wColorTable, fWin30;
    HPALETTE hPalette, hOldPalette = NULL;

    lpDIBHdr  = lprp->lpMem;
    lpbihdr   = (LPBITMAPINFOHEADER)lpDIBHdr;
    lpbchdr   = (LPBITMAPCOREHEADER)lpDIBHdr;

    fWin30 = (lpbihdr->biSize == sizeof(BITMAPINFOHEADER));

    // Determine size of the color table
    if (fWin30)
        wBitCount = lpbihdr->biBitCount;
    else
        wBitCount = lpbchdr->bcBitCount;

    switch (wBitCount)
    {
        case 1:
            wBitCount = 2;
        break;
        case 4:
            wBitCount = 16;
        break;
        case 8:
            wBitCount = 256;
        break;

        default:
            wBitCount = 0;
        break;
    }

    if (fWin30)
    {
        if (lpbihdr->biClrUsed)
            wColorTable = (WORD)lpbihdr->biClrUsed * sizeof(RGBQUAD);
        else
            wColorTable = wBitCount * sizeof(RGBQUAD);
    }
    else
        wColorTable = wBitCount * sizeof(RGBTRIPLE);

    // Calculate the pointer to the DIB bits
    if (fWin30)
        lpDIBBits = lpDIBHdr + lpbihdr->biSize + wColorTable;
    else
        lpDIBBits = lpDIBHdr + lpbchdr->bcSize + wColorTable;

    hDC = GetDC (NULL);

    if (!hDC)
        return NULL;

    hPalette = CreateNewPalette( lpDIBHdr, fWin30, wBitCount );

    if (hPalette)
      hOldPalette = SelectPalette (hDC, hPalette, FALSE);

    RealizePalette (hDC);

    hBitmap = CreateDIBitmap (hDC, lpbihdr, CBM_INIT, lpDIBBits,
                                (LPBITMAPINFO) lpDIBHdr, DIB_RGB_COLORS);

    if (hOldPalette)
        SelectPalette (hDC, hOldPalette, FALSE);

    ReleaseDC (NULL, hDC);
    DeleteObject( hPalette );

    return hBitmap;

} //*** MakeBitmap

//*************************************************************
//
//  CreateNewPalette
//
//  Purpose:
//      Creates a palette from the DIB
//
//
//  Parameters:
//      LPSTR lpbi  - pointer to the bitmapinfo
//      
//
//  Return: (HPALETTE)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/27/92   MSM        Created
//
//*************************************************************

HPALETTE CreateNewPalette (LPSTR lpbi, WORD fWin30, WORD wColors)
{
    LPLOGPALETTE     lpPal;
    HANDLE           hMem;
    HPALETTE         hPal = NULL;
    int              i;
    LPBITMAPINFO     lpbmi;
    LPBITMAPCOREINFO lpbmc;

    lpbmi = (LPBITMAPINFO) lpbi;
    lpbmc = (LPBITMAPCOREINFO) lpbi;

    if (wColors)
    {
        hMem = GlobalAlloc (GHND, sizeof (LOGPALETTE) + 
                    sizeof (PALETTEENTRY) * wColors);

        if (!hMem)
            return NULL;

        lpPal = (LPLOGPALETTE) GlobalLock (hMem);
        if (!lpPal)
        {
            GlobalFree( hMem);
            return NULL;
        }

        lpPal->palVersion    = 0x300;
        lpPal->palNumEntries = wColors;

        for (i = 0;  i < (int)wColors;  i++)
        {
            if (fWin30)
            {
                lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
                lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
                lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
            else
            {
                lpPal->palPalEntry[i].peRed   = lpbmc->bmciColors[i].rgbtRed;
                lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
                lpPal->palPalEntry[i].peBlue  = lpbmc->bmciColors[i].rgbtBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
        }

        hPal = CreatePalette (lpPal);

        GlobalUnlock (hMem);
        GlobalFree   (hMem);
    }
    return hPal;

} //*** CreateNewPalette

//*** EOF: Bitmap.c
