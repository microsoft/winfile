//*************************************************************
//  File name: iconcur.c
//
//  Description: 
//      Code for displaying icons and cursors
//
//  History:    Date       Author     Comment
//               1/23/92   MSM        Created
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
//  ShowIconProc
//
//  Purpose:
//      Displays an icon for all the world to see
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
//               1/23/92   MSM        Created
//
//*************************************************************

INT_PTR FAR PASCAL ShowIconProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HICON hIcon;
    static WORD  wWidth, wHeight;

    #define PADDING 10

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;

            SetWindowText( hDlg, "Viewing Icon" );
            hIcon = MakeIcon( lprp );

            wWidth = (GetSystemMetrics( SM_CXICON )+PADDING) * 2;
            wWidth = max( wWidth, 250 );

            wHeight = GetSystemMetrics( SM_CYICON )+PADDING*3;
            wHeight = max( wHeight, 100 );

            SetWindowPos( hDlg, NULL, 0, 0, wWidth, wHeight, 
                SWP_NOZORDER|SWP_NOMOVE );

            return TRUE;
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint( hDlg, &ps );

            if (hIcon)
            {
                PatBlt( ps.hdc, 0, 0, wWidth/2, wHeight, WHITENESS );
                PatBlt( ps.hdc, wWidth/2, 0, wWidth/2, wHeight, BLACKNESS );
                DrawIcon( ps.hdc, PADDING, PADDING, hIcon );
                DrawIcon( ps.hdc, PADDING+wWidth/2, PADDING, hIcon );
            }
            else
            {
                PatBlt( ps.hdc, 0, 0, wWidth, wHeight, WHITENESS );
                TextOut( ps.hdc, 10, 10, "Icon not created!", 17 );
            }
            EndPaint( hDlg, &ps );
        }
        break;                    

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        

        case WM_DESTROY:
            if (hIcon)
                DestroyIcon( hIcon );
        break;
    }
    return FALSE;

} //*** ShowIconProc

//*************************************************************
//
//  MakeIcon
//
//  Purpose:
//      Attempts to create an icon based on the info passed in
//
//
//  Parameters:
//      LPRESPACKET lprp
//      
//
//  Return: (HICON)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/23/92   MSM        Created
//  
//*************************************************************

HICON MakeIcon ( LPRESPACKET lprp )
{
    LPBITMAPINFOHEADER lpbihdr;
    LPBITMAPINFO       lpbinfo;

    HICON   hIcon = NULL;
    WORD    wH, wW, wXORSize, wANDSize, wColorTableSize, wScans;
    WORD    wIconW, wIconH;
    LPBYTE  lpANDbits;
    LPBYTE  lpXORbits;
    HDC     hDC, hSrcDC, hDestDC;
    HBITMAP hbmpXOR, hbmpAND;
    HBITMAP hbmpDestXOR, hbmpDestAND;
    HBITMAP hbmpOldSrc, hbmpOldDest;
    BITMAP  bmp;
    HANDLE  hXORbits, hANDbits;


    lpbihdr = (LPBITMAPINFOHEADER)lprp->lpMem;
    lpbinfo = (LPBITMAPINFO)lpbihdr;

    wW = (WORD)lpbihdr->biWidth;
    wH = (WORD)lpbihdr->biHeight / 2;

    wXORSize = (wW * wH * lpbihdr->biBitCount) / 8;
    wANDSize= (wW * wH) / 8;

    wColorTableSize = sizeof(RGBQUAD) * (0x0001<<lpbihdr->biBitCount);

    lpXORbits = ((LPBYTE)lpbihdr)+wColorTableSize+(WORD)lpbihdr->biSize;
    lpANDbits = lpXORbits + wXORSize;

    // Now we need to make a bitmap and convert these to DDB
    // Get a screen DC
    hDC = GetDC( NULL );
    if (!hDC)
        return NULL;

    
// XOR BITMAP
    // create color device-dependent bitmap for XOR mask
    hbmpXOR = CreateCompatibleBitmap( hDC, wW, wH );

    if (!hbmpXOR)
    {
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    // modify BITMAPINFOHEADER structure for color bitmap 
    lpbihdr->biHeight = wH;

    // convert XOR mask from DIB to DDB
    wScans = SetDIBits(hDC, hbmpXOR, 0, wH, lpXORbits, lpbinfo, DIB_RGB_COLORS);

    if (wScans == 0)
    {
        DeleteObject( hbmpXOR );
        ReleaseDC( NULL, hDC );
        return NULL;
    }


// AND BITMAP
    // create MONO device-dependent bitmap for AND mask
    hbmpAND= CreateBitmap( wW, wH, 1, 1, NULL );

    if (!hbmpAND)
    {
        DeleteObject( hbmpXOR );
        ReleaseDC( NULL, hDC );
        return NULL;
    }
    else
    {
        RGBQUAD rgbBlack = { 0x00, 0x00, 0x00, 0x00 };
        RGBQUAD rgbWhite = { 0xFF, 0xFF, 0xFF, 0x00 };

        // modify BITMAPINFOHEADER structure for monochrome bitmap
        lpbihdr->biHeight = wH;
        lpbihdr->biSizeImage = wANDSize;
        lpbihdr->biBitCount = 1;

        lpbinfo->bmiColors[0] = rgbBlack;
        lpbinfo->bmiColors[1] = rgbWhite;

        // convert AND mask from DIB to DDB
        wScans = SetDIBits(hDC,hbmpAND,0,wH,lpANDbits,lpbinfo,DIB_RGB_COLORS);

        if (wScans == 0)
        {
            DeleteObject( hbmpXOR );
            DeleteObject( hbmpAND );
            ReleaseDC( NULL, hDC );
            return NULL;
        }
    }


// Now that we have a AND bitmap and a XOR bitmap, we need to
// convert the icon size to that of the display by using StretchBlt.

    hSrcDC  = CreateCompatibleDC( hDC );
    if (!hSrcDC)
    {
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    hDestDC = CreateCompatibleDC( hDC );
    if (!hDestDC)
    {
        DeleteDC( hSrcDC );
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    wIconW = GetSystemMetrics( SM_CXICON );
    wIconH = GetSystemMetrics( SM_CYICON );

    hbmpDestXOR = CreateCompatibleBitmap( hDC, wIconW, wIconH );

    if (!hbmpDestXOR)
    {
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        DeleteDC( hSrcDC );
        DeleteDC( hDestDC );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    hbmpDestAND = CreateBitmap( wIconW, wIconH, 1, 1, NULL );

    if (!hbmpDestAND)
    {
        DeleteObject( hbmpDestXOR );
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        DeleteDC( hSrcDC );
        DeleteDC( hDestDC );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    hbmpOldSrc = (HBITMAP)SelectObject( hSrcDC, hbmpXOR );
    hbmpOldDest = (HBITMAP)SelectObject( hDestDC, hbmpDestXOR );
    SetStretchBltMode( hDestDC, COLORONCOLOR );

    StretchBlt(hDestDC,0,0,wIconW,wIconH,hSrcDC,0,0,wW,wH,SRCCOPY);

    SelectObject( hSrcDC, hbmpAND );
    SelectObject( hDestDC, hbmpDestAND );
    SetStretchBltMode( hDestDC, BLACKONWHITE );

    StretchBlt(hDestDC,0,0,wIconW,wIconH,hSrcDC,0,0,wW,wH,SRCCOPY);

    // Clean up all but the two new bitmaps
    SelectObject( hSrcDC, hbmpOldSrc );
    SelectObject( hDestDC, hbmpOldDest );
    DeleteDC( hSrcDC );
    DeleteDC( hDestDC );
    ReleaseDC( NULL, hDC );
    DeleteObject( hbmpXOR );
    DeleteObject( hbmpAND );


// Now that the bitmaps are the correct size, we need to retrieve
// the DDB bits again so we can CreateIcon
    
    GetObject( hbmpDestXOR, sizeof(BITMAP), (LPSTR)&bmp );

    wXORSize = (bmp.bmWidth * bmp.bmHeight) *
                (bmp.bmPlanes * bmp.bmBitsPixel) / 8;

    hXORbits = GlobalAlloc( GHND, wXORSize );

    if (hXORbits == NULL)
    {
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if ((lpXORbits = (LPBYTE)GlobalLock( hXORbits )) == NULL)
    {
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if (!GetBitmapBits( hbmpDestXOR, wXORSize, lpXORbits))
    {
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    GetObject( hbmpDestAND, sizeof(BITMAP), (LPSTR)&bmp );

    wANDSize = (bmp.bmWidth * bmp.bmHeight) / 8;

    hANDbits = GlobalAlloc( GHND, wANDSize );

    if (hANDbits == NULL)
    {
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if ((lpANDbits = (LPBYTE)GlobalLock( hANDbits )) == NULL)
    {
        GlobalFree( hANDbits );
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if (!GetBitmapBits( hbmpDestAND,wANDSize,lpANDbits))
    {
        GlobalUnlock( hANDbits );
        GlobalFree( hANDbits );
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }


// Now after all this we can finally make an icon.

    GetObject( hbmpDestXOR, sizeof(BITMAP), (LPSTR)&bmp );

    hIcon = CreateIcon( ghInst, wIconW, wIconH, bmp.bmPlanes,
        bmp.bmBitsPixel, lpANDbits, lpXORbits );

    GlobalUnlock( hANDbits );
    GlobalFree( hANDbits );
    GlobalUnlock( hXORbits );
    GlobalFree( hXORbits );
    DeleteObject( hbmpDestAND );
    DeleteObject( hbmpDestXOR );

    return hIcon;

} //*** MakeIcon

//*************************************************************
//
//  ShowCursorProc
//
//  Purpose:
//      Displays an cursor for all the world to see
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
//               1/23/92   MSM        Created
//
//*************************************************************

INT_PTR FAR PASCAL ShowCursorProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HCURSOR hCursor;
    static WORD    wWidth, wHeight;

    #define PADDING 10

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;

            SetWindowText( hDlg, "Viewing Cursor" );
            hCursor = MakeCursor( lprp );

            wWidth = (GetSystemMetrics( SM_CXICON )+PADDING) * 2;
            wWidth = max( wWidth, 250 );

            wHeight = GetSystemMetrics( SM_CYICON )+PADDING*3;
            wHeight = max( wHeight, 100 );

            SetWindowPos( hDlg, NULL, 0, 0, wWidth, wHeight, 
                SWP_NOZORDER|SWP_NOMOVE );

            return TRUE;
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint( hDlg, &ps );

            if (hCursor)
            {
                PatBlt( ps.hdc, 0, 0, wWidth/2, wHeight, WHITENESS );
                PatBlt( ps.hdc, wWidth/2, 0, wWidth/2, wHeight, BLACKNESS );
                DrawIcon( ps.hdc, PADDING, PADDING, hCursor );
                DrawIcon( ps.hdc, PADDING+wWidth/2, PADDING, hCursor );
            }
            else
            {
                PatBlt( ps.hdc, 0, 0, wWidth, wHeight, WHITENESS );
                TextOut( ps.hdc, 10, 10, "Cursor not created!", 17 );
            }
            EndPaint( hDlg, &ps );
        }
        break;                    

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        

        case WM_DESTROY:
            if (hCursor)
                DestroyCursor( hCursor );
        break;
    }
    return FALSE;

} //*** ShowCursorProc

//*************************************************************
//
//  MakeCursor
//
//  Purpose:
//      Attempts to create an cursor based on the info passed in
//
//
//  Parameters:
//      LPRESPACKET lprp
//      
//
//  Return: (HCURSOR)
//
//
//  Comments:
//      The differences between ICONs and CURSORs are subtle and
//      much of the above code could be placed into common routines
//      to be used by both.  I have not done this since the main
//      goal of this sample is not code compaction, but readability
//      and understanding.    
//
//  History:    Date       Author     Comment
//               1/23/92   MSM        Created
//  
//*************************************************************

HCURSOR MakeCursor ( LPRESPACKET lprp )
{
    LPBITMAPINFOHEADER  lpbihdr;
    LPBITMAPINFO        lpbinfo;
    LPCURSORIMAGE       lpci;

    HCURSOR hCursor = NULL;
    WORD    wH, wW, wXORSize, wANDSize, wScans, wColorTableSize;
    WORD    wCursorW, wCursorH;
    LPSTR   lpANDbits;
    LPSTR   lpXORbits;
    HDC     hDC, hSrcDC, hDestDC;
    HBITMAP hbmpXOR, hbmpAND;
    HBITMAP hbmpDestXOR, hbmpDestAND;
    HBITMAP hbmpOldSrc, hbmpOldDest;
    BITMAP  bmp;
    HANDLE  hXORbits, hANDbits;



    lpci    = (LPCURSORIMAGE)lprp->lpMem;

    lpbinfo = &lpci->bi;
    lpbihdr = (LPBITMAPINFOHEADER)lpbinfo;

    wW = (WORD)lpbihdr->biWidth;
    wH = (WORD)lpbihdr->biHeight / 2;

    wXORSize = (wW * wH) / 8;
    wANDSize = (wW * wH) / 8;

    wColorTableSize = sizeof(RGBQUAD) * (0x0001<<lpbihdr->biBitCount);

    lpXORbits = ((LPSTR)lpbihdr)+wColorTableSize+(WORD)lpbihdr->biSize;
    lpANDbits = lpXORbits + wXORSize;

    // Now we need to make a bitmap and convert these to DDB
    // Get a screen DC
    hDC = GetDC( NULL );
    if (!hDC)
        return NULL;

    

// XOR BITMAP
    // create MONO device-dependent bitmap for XOR mask
    hbmpXOR= CreateBitmap( wW, wH, 1, 1, NULL );

    if (!hbmpXOR)
    {
        ReleaseDC( NULL, hDC );
        return NULL;
    }
    else
    {
        RGBQUAD rgbBlack = { 0x00, 0x00, 0x00, 0x00 };
        RGBQUAD rgbWhite = { 0xFF, 0xFF, 0xFF, 0x00 };

        // modify BITMAPINFOHEADER structure for monochrome bitmap
        lpbihdr->biHeight = wH;
        lpbihdr->biSizeImage = wXORSize;
        lpbihdr->biBitCount = 1;

        lpbinfo->bmiColors[0] = rgbBlack;
        lpbinfo->bmiColors[1] = rgbWhite;

        // convert XOR mask from DIB to DDB
        wScans = SetDIBits(hDC,hbmpXOR,0,wH,lpXORbits,lpbinfo,DIB_RGB_COLORS);

        if (wScans == 0)
        {
            DeleteObject( hbmpXOR );
            ReleaseDC( NULL, hDC );
            return NULL;
        }
    }

// AND BITMAP
    // create MONO device-dependent bitmap for AND mask
    hbmpAND= CreateBitmap( wW, wH, 1, 1, NULL );

    if (!hbmpAND)
    {
        DeleteObject( hbmpXOR );
        ReleaseDC( NULL, hDC );
        return NULL;
    }
    else
    {
        RGBQUAD rgbBlack = { 0x00, 0x00, 0x00, 0x00 };
        RGBQUAD rgbWhite = { 0xFF, 0xFF, 0xFF, 0x00 };

        // modify BITMAPINFOHEADER structure for monochrome bitmap
        lpbihdr->biHeight = wH;
        lpbihdr->biSizeImage = wANDSize;
        lpbihdr->biBitCount = 1;

        lpbinfo->bmiColors[0] = rgbBlack;
        lpbinfo->bmiColors[1] = rgbWhite;

        // convert AND mask from DIB to DDB
        wScans = SetDIBits(hDC,hbmpAND,0,wH,lpANDbits,lpbinfo,DIB_RGB_COLORS);

        if (wScans == 0)
        {
            DeleteObject( hbmpXOR );
            DeleteObject( hbmpAND );
            ReleaseDC( NULL, hDC );
            return NULL;
        }
    }


// Now that we have a AND bitmap and a XOR bitmap, we need to
// convert the cursor size to that of the display by using StretchBlt.

    hSrcDC  = CreateCompatibleDC( hDC );
    if (!hSrcDC)
    {
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    hDestDC = CreateCompatibleDC( hDC );
    if (!hDestDC)
    {
        DeleteDC( hSrcDC );
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    wCursorW = GetSystemMetrics( SM_CXCURSOR );
    wCursorH = GetSystemMetrics( SM_CYCURSOR );

    hbmpDestXOR = CreateBitmap( wCursorW, wCursorH, 1, 1, NULL );
    if (!hbmpDestXOR)
    {
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        DeleteDC( hSrcDC );
        DeleteDC( hDestDC );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    hbmpDestAND = CreateBitmap( wCursorW, wCursorH, 1, 1, NULL );
    if (!hbmpDestAND)
    {
        DeleteObject( hbmpDestXOR );
        DeleteObject( hbmpXOR );
        DeleteObject( hbmpAND );
        DeleteDC( hSrcDC );
        DeleteDC( hDestDC );
        ReleaseDC( NULL, hDC );
        return NULL;
    }

    hbmpOldSrc = (HBITMAP)SelectObject( hSrcDC, hbmpXOR );
    hbmpOldDest = (HBITMAP)SelectObject( hDestDC, hbmpDestXOR );
    SetStretchBltMode( hDestDC, WHITEONBLACK );

    StretchBlt(hDestDC,0,0,wCursorW,wCursorH,hSrcDC,0,0,wW,wH,SRCCOPY);

    SelectObject( hSrcDC, hbmpAND );
    SelectObject( hDestDC, hbmpDestAND );
    SetStretchBltMode( hDestDC, BLACKONWHITE );

    StretchBlt(hDestDC,0,0,wCursorW,wCursorH,hSrcDC,0,0,wW,wH,SRCCOPY);

    // Clean up all but the two new bitmaps
    SelectObject( hSrcDC, hbmpOldSrc );
    SelectObject( hDestDC, hbmpOldDest );
    DeleteDC( hSrcDC );
    DeleteDC( hDestDC );
    ReleaseDC( NULL, hDC );
    DeleteObject( hbmpXOR );
    DeleteObject( hbmpAND );


// Now that the bitmaps are the correct size, we need to retrieve
// the DDB bits again so we can CreateCursor
    
    GetObject( hbmpDestXOR, sizeof(BITMAP), (LPSTR)&bmp );

    wXORSize = (bmp.bmWidth * bmp.bmHeight) *
                (bmp.bmPlanes * bmp.bmBitsPixel) / 8;

    hXORbits = GlobalAlloc( GHND, wXORSize );

    if (hXORbits == NULL)
    {
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if ((lpXORbits = (LPSTR)GlobalLock( hXORbits )) == NULL)
    {
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if (!GetBitmapBits( hbmpDestXOR, wXORSize, lpXORbits))
    {
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    GetObject( hbmpDestAND, sizeof(BITMAP), (LPSTR)&bmp );

    wANDSize = (bmp.bmWidth * bmp.bmHeight) / 8;

    hANDbits = GlobalAlloc( GHND, wANDSize );

    if (hANDbits == NULL)
    {
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if ((lpANDbits = (LPSTR)GlobalLock( hANDbits )) == NULL)
    {
        GlobalFree( hANDbits );
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }

    if (!GetBitmapBits( hbmpDestAND,wANDSize,lpANDbits))
    {
        GlobalUnlock( hANDbits );
        GlobalFree( hANDbits );
        GlobalUnlock( hXORbits );
        GlobalFree( hXORbits );
        DeleteObject( hbmpDestAND );
        DeleteObject( hbmpDestXOR );
        return NULL;
    }


// Now after all this we can finally make an cursor.

    GetObject( hbmpDestXOR, sizeof(BITMAP), (LPSTR)&bmp );

    hCursor = CreateCursor( ghInst, lpci->xHotSpot, lpci->yHotSpot, 
                wCursorW, wCursorH, lpANDbits, lpXORbits );

    GlobalUnlock( hANDbits );
    GlobalFree( hANDbits );
    GlobalUnlock( hXORbits );
    GlobalFree( hXORbits );
    DeleteObject( hbmpDestAND );
    DeleteObject( hbmpDestXOR );

    return hCursor;

} //*** MakeCursor


//*** EOF: iconcur.c
