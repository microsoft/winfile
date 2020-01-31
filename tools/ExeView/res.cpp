//*************************************************************
//  File name: res.c
//
//  Description: 
//      Code for displaying resources
//
//  History:    Date       Author     Comment
//               1/21/92   MSM        Created
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
#include "string.h"

extern const char *rc_types[];

BOOL LoadResourcePacket(PEXEINFO pExeInfo, PRESTYPE prt, PRESINFO pri, LPRESPACKET lprp)
{
    int      fFile;
    OFSTRUCT of;
    int      nErr = 0;
    LONG     lSize;
    LONG     lOffset;
    HANDLE   hMem;
    LPSTR    lpMem;
    char    *lpTMem;

    if (!pExeInfo || !prt || !pri)
    {
        MessageBox(ghWndMain,"Unable to display resource!","EXEVIEW",MB_OK);
        return FALSE;
    }

    if (pExeInfo->NewHdr.wExpVersion < 0x0300)  // Win 2.x app??
    {
        MessageBox(ghWndMain,"Unable to display resource!  Windows 2.X app.","EXEVIEW",MB_OK);
        return FALSE;
    }

    fFile = OpenFile( pExeInfo->pFilename, &of, OF_READ );
    if (!fFile)
    {
        MessageBox(ghWndMain,"Could not open file!","EXEVIEW",MB_OK);
        return FALSE;
    }

    // Calculate the position in the the file and move file pointer
    lOffset = ((LONG)pri->wOffset)<<(pExeInfo->wShiftCount);

    _llseek( fFile, lOffset, 0 );

    // Allocate memory for resource
    lSize   = ((LONG)pri->wLength)<<(pExeInfo->wShiftCount);
    hMem = GlobalAlloc( GHND, lSize );

    if (!hMem)
    {
        _lclose( fFile );
        MessageBox(ghWndMain,"Could not allocate memory for resource!","EXEVIEW",MB_OK);
        return FALSE;
    }

    lpMem = (LPSTR)GlobalLock(hMem);
    if (!lpMem)
    {
        _lclose( fFile );
        MessageBox(ghWndMain,"Could not lock memory for resource!","EXEVIEW",MB_OK);
        GlobalFree( hMem );
        return FALSE;
    }

    // Read in resource from file
    lpTMem = (char *)lpMem;

    SetCursor( LoadCursor(NULL, IDC_WAIT) );
    while (lSize)
    {
        WORD wSize;

        if (lSize>=32767)
            wSize = 32767;
        else
            wSize = (WORD)lSize;

        // _lread is limited to 32K chunks, thus these gyrations
        if (_lread(fFile, (LPSTR)lpTMem, wSize) != wSize)
        {
            _lclose( fFile );
            MessageBox(ghWndMain,"Error reading from file!","EXEVIEW",MB_OK);
            GlobalUnlock( hMem );
            GlobalFree( hMem );
            return FALSE;
        }
        lSize -= wSize;
        lpTMem += wSize;
    }
    SetCursor( LoadCursor(NULL, IDC_ARROW) );


    // Build a resource packet.  This allows the passing of all the
    // the needed info with one 32 bit pointer.
    lprp->pExeInfo = pExeInfo;
    lprp->prt = prt;
    lprp->pri = pri;
    lprp->lSize = ((LONG)pri->wLength)<<(pExeInfo->wShiftCount);
    lprp->lpMem = lpMem;
    lprp->fFile = fFile;
    lprp->hMem = hMem;

    return TRUE;
}


VOID FreeResourcePacket(LPRESPACKET lprp)
{
    _lclose(lprp->fFile);
    GlobalUnlock(lprp->hMem);
    GlobalFree(lprp->hMem);
}


//*************************************************************
//
//  DisplayResource
//
//  Purpose:
//      Attempts to display the given resource
//
//
//  Parameters:
//      PEXEINFO pExeInfo
//      PRESTYPE ptr
//      PRESINFO pri
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

BOOL DisplayResource ( PEXEINFO pExeInfo, PRESTYPE prt, PRESINFO pri )
{
    int      nErr = 0;
    RESPACKET rp;
    LPRESPACKET lprp = &rp;

    if (!LoadResourcePacket(pExeInfo, prt, pri, lprp))
        return FALSE;

    if (prt->wType & 0x8000)
    {
        switch (prt->wType & 0x7fff)
        {
            case NAMETABLE:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(NameTableProc, ghInst );
                nErr = !DialogBoxParam( ghInst, "NAMETABLE_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case GROUP_ICON:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(IconGroupProc, ghInst );

                // Re-use the NAMETABLE_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "NAMETABLE_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case GROUP_CURSOR:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(CursorGroupProc, ghInst );

                // Re-use the NAMETABLE_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "NAMETABLE_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case (int)RT_ACCELERATOR:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(AccelTableProc, ghInst );

                // Re-use the NAMETABLE_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "NAMETABLE_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case (int)RT_STRING:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(StringTableProc, ghInst );

                // Re-use the NAMETABLE_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "NAMETABLE_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case (int)RT_FONTDIR:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(FontDirProc, ghInst );

                // Re-use the NAMETABLE_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "NAMETABLE_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case (int)RT_ICON:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(ShowIconProc, ghInst );

                // Re-use the GRAPHIC_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "GRAPHIC_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case (int)RT_CURSOR:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(ShowCursorProc, ghInst );

                // Re-use the GRAPHIC_DLG with a different DlgProc
                nErr = !DialogBoxParam( ghInst, "GRAPHIC_DLG", ghWndMain, lpProc,
                    (LPARAM)lprp );
                FreeProcInstance( lpProc );
            }
            break;

            case (int)RT_MENU:
                ShowMenu( lprp );
            break;

            case (int)RT_BITMAP:
                ShowBitmap( lprp );
            break;

            case (int)RT_DIALOG:
            {
                DLGPROC lpProc;

                lpProc = MakeProcInstance(ShowDialogProc, ghInst );

                // NOTE: 16bit dialogs will not display at all on 32/64 bit Windows.  Just saying...

                nErr = DialogBoxIndirect(ghInst, (LPCDLGTEMPLATE)lprp->lpMem, ghWndMain,lpProc);
                FreeProcInstance( lpProc );
                if (nErr!=-1)
                    nErr = 0;
            }
            break;

            default:
                nErr = TRUE;
            break;
        }
        if (!nErr)
        {
            FreeResourcePacket(lprp);
            return TRUE;
        }
    }

    MessageBox(ghWndMain,"Unable to display resource!","EXEVIEW",MB_OK);
    FreeResourcePacket(lprp);
    return FALSE;

} //*** DisplayResource

//*************************************************************
//
//  NameTableProc
//
//  Purpose:
//      Dialog box procedure for displaying a name table
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

INT_PTR FAR PASCAL NameTableProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;
            HWND     hWnd = GetDlgItem( hDlg, IDL_NAMES );

            FillLBWithNameTable( hWnd, lprp );
            return TRUE;
        }
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** NameTableProc

//*************************************************************
//
//  FillLBWithNameTable
//
//  Purpose:
//      Fills the list box with the name table
//
//
//  Parameters:
//      HWND hWnd
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
//               1/21/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithNameTable (HWND hWnd, LPRESPACKET lprp)
{
    char        szBuff[255];
    LPSTR       lp = (LPSTR)szBuff;
    LPNAMEENTRY lpne = (LPNAMEENTRY)lprp->lpMem;
    LONG        lSize = 0;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L );

    lstrcpy( lp, "Type#    ID       Resource Type        Resource Name" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    // Loop through the table
    while (lSize < lprp->lSize)        
    {
        LPCSTR lpType, lpID;
        WORD wType;

        // Check for end of table
        if (lpne->wBytes==0)
            break;
        
        wType =  lpne->wTypeOrd&0x7fff;

        // Point to type
        lpType = (LPCSTR)(lpne+1);

        // Point to ID Name
        lpID = lpType + lstrlen( lpType ) + 1;

        if (*lpType==0)
            if (!(wType==0 || wType==11 || wType==13 || wType > 16))
                lpType = rc_types[ wType ];

        wsprintf( lp, "%#04x   %#04x   %-20s %s",
            lpne->wTypeOrd, lpne->wIDOrd, lpType, lpID );
        ADDITEM();

        lSize += lpne->wBytes;
        lpne = (LPNAMEENTRY)( (LPSTR)lpne + lpne->wBytes );
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithNameTable

//*************************************************************
//
//  IconGroupProc
//
//  Purpose:
//      Dialog box procedure for displaying a icon group
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

INT_PTR FAR PASCAL IconGroupProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;
            HWND     hWnd = GetDlgItem( hDlg, IDL_NAMES );

            SetWindowText( hDlg, "Viewing Icon Group" );
            FillLBWithIconGroup( hWnd, lprp );
            return TRUE;
        }
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** IconGroupProc

//*************************************************************
//
//  FillLBWithIconGroup
//
//  Purpose:
//      Fills the list box with the icon group
//
//
//  Parameters:
//      HWND hWnd
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
//               1/21/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithIconGroup (HWND hWnd, LPRESPACKET lprp)
{
    char        szBuff[255];
    LPSTR       lp = (LPSTR)szBuff;
    WORD        wI;
    LPICONDIR   lpid = (LPICONDIR)lprp->lpMem;



    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L );

    lstrcpy( lp, "Width  Height  Colors  Planes  Bits/Pel  Size(bytes)  Ordinal" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    for (wI=0; wI<lpid->wCount; wI++)
    {
        LPICONENTRY lpie = &lpid->Icons[wI];

        wsprintf( lp, " %-4u    %-4u    %-4u    %-4u    %-4u     %-10lu  %#04x",
            lpie->bWidth, lpie->bHeight, lpie->bColorCount, lpie->wPlanes,
            lpie->wBitsPerPel, lpie->dwBytesInRes, lpie->wOrdinalNumber );
        ADDITEM();
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithIconGroup

//*************************************************************
//
//  CursorGroupProc
//
//  Purpose:
//      Dialog box procedure for displaying a cursor group
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

INT_PTR FAR PASCAL CursorGroupProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;
            HWND     hWnd = GetDlgItem( hDlg, IDL_NAMES );

            SetWindowText( hDlg, "Viewing Cursor Group" );
            FillLBWithCursorGroup( hWnd, lprp );
            return TRUE;
        }
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** CursorGroupProc

//*************************************************************
//
//  FillLBWithCursorGroup
//
//  Purpose:
//      Fills the list box with the cursor group
//
//
//  Parameters:
//      HWND hWnd
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
//               1/21/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithCursorGroup (HWND hWnd, LPRESPACKET lprp)
{
    char        szBuff[255];
    LPSTR       lp = (LPSTR)szBuff;
    WORD        wI;
    LPCURSORDIR lpcd = (LPCURSORDIR)lprp->lpMem;


    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L );

    lstrcpy( lp, "Width  Height  Planes  Bits/Pel  Size(bytes)  Ordinal" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    for (wI=0; wI<lpcd->wCount; wI++)
    {
        LPCURSORENTRY lpce = &lpcd->Cursors[wI];

        wsprintf( lp, " %-4u    %-4u    %-4u    %-4u     %-10lu  %#04x",
            lpce->wWidth, lpce->wHeight/2, lpce->wPlanes,
            lpce->wBitsPerPel, lpce->dwBytesInRes, lpce->wOrdinalNumber );
        ADDITEM();
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithCursorGroup

//*************************************************************
//
//  AccelTableProc
//
//  Purpose:
//      Dialog box procedure for displaying a accel table
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

INT_PTR FAR PASCAL AccelTableProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;
            HWND     hWnd = GetDlgItem( hDlg, IDL_NAMES );

            SetWindowText( hDlg, "Viewing Accelerator Table" );
            FillLBWithAccelTable( hWnd, lprp );
            return TRUE;
        }
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** AccelTableProc

//*************************************************************
//
//  FillLBWithAccelTable
//
//  Purpose:
//      Fills the list box with the name table
//
//
//  Parameters:
//      HWND hWnd
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
//               1/21/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithAccelTable (HWND hWnd, LPRESPACKET lprp)
{
    char        szBuff[255];
    LPSTR       lp = (LPSTR)szBuff;
    LPACCELENTRY lpae = (LPACCELENTRY)lprp->lpMem;


    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L );

    lstrcpy( lp, "Event   ID      Flags" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    while (lpae)
    {

        wsprintf( lp, "%#04x  %#04x  (%3u) ", lpae->wEvent, lpae->wID,
            lpae->fFlags&0x007f );

        if (lpae->fFlags & AF_VIRTKEY)
            lstrcat( lp, "VIRTKEY " );
        else
            lstrcat( lp, "        " );

        if (lpae->fFlags & AF_ALT)
            lstrcat( lp, "ALT " );
        else
            lstrcat( lp, "    " );

        if (lpae->fFlags & AF_SHIFT)
            lstrcat( lp, "SHIFT " );
        else
            lstrcat( lp, "      " );

        if (lpae->fFlags & AF_CONTROL)
            lstrcat( lp, "CONTROL " );
        else
            lstrcat( lp, "        " );

        if (lpae->fFlags & AF_NOINVERT)
            lstrcat( lp, "NOINVERT " );
        else
            lstrcat( lp, "         " );

        ADDITEM();
        // Last item??
        if (lpae->fFlags & 0x80)
            lpae = NULL;
        else
            lpae++;
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithAccelTable

//*************************************************************
//
//  StringTableProc
//
//  Purpose:
//      Dialog box procedure for displaying a string table
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

INT_PTR FAR PASCAL StringTableProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;
            HWND     hWnd = GetDlgItem( hDlg, IDL_NAMES );

            SetWindowText( hDlg, "Viewing String Table" );
            FillLBWithStringTable( hWnd, lprp );
            return TRUE;
        }
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** StringTableProc

//*************************************************************
//
//  FillLBWithStringTable
//
//  Purpose:
//      Fills the list box with the name table
//
//
//  Parameters:
//      HWND hWnd
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
//               1/21/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithStringTable (HWND hWnd, LPRESPACKET lprp)
{
    char  szBuff[270];
    LPSTR lpS, lp = (LPSTR)szBuff;
    int   nI, nOrdinal;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    nOrdinal = (lprp->pri->wID-1) & 0x7fff;
    nOrdinal <<= 4;
    lpS      = lprp->lpMem;

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L );

    lstrcpy( lp, "Ordinal  String" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    for (nI=0; nI<16; nI++)
    {
        BYTE bLen = *lpS++;

        wsprintf( lp, "%#04x   ", nOrdinal + nI );

        if (bLen)
        {
            strncat_s( lp, 270, lpS, (WORD)bLen );
            lpS += (int)bLen;
            ADDITEM();
        }
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithStringTable

//*************************************************************
//
//  FontDirProc
//
//  Purpose:
//      Dialog box procedure for displaying a font directory
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

INT_PTR FAR PASCAL FontDirProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPRESPACKET lprp = (LPRESPACKET)lParam;
            HWND     hWnd = GetDlgItem( hDlg, IDL_NAMES );

            SetWindowText( hDlg, "Viewing Font Directory" );
            FillLBWithFontDir( hWnd, lprp );
            return TRUE;
        }
        break;

        case WM_COMMAND:
            if (wParam==IDOK || wParam==IDCANCEL)
                EndDialog( hDlg, TRUE );
            return TRUE;
        break;        
    }
    return FALSE;

} //*** FontDirProc

//*************************************************************
//
//  FillLBWithFontDir
//
//  Purpose:
//      Fills the list box with the name table
//
//
//  Parameters:
//      HWND hWnd
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
//               1/21/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithFontDir (HWND hWnd, LPRESPACKET lprp)
{
    char  szBuff[255];
    LPSTR lp = (LPSTR)szBuff;
    int   nI, nFonts;
    LPFONTENTRY lpfe;

    nFonts = *((LPINT)lprp->lpMem);
    lpfe = (LPFONTENTRY)(lprp->lpMem + sizeof(WORD));

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );
    SendMessage( hWnd, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L );

    for (nI=1; nI<=nFonts; nI++)
    {
        LPSTR lpDev, lpFace;

        lpDev = (LPSTR)lpfe->szDeviceName;
        lpFace = lpDev + lstrlen(lpDev) + 1;

        if (nI!=1)
        {
            lstrcpy( lp, "" );
            ADDITEM();
            ADDITEM();
        }

        wsprintf( lp, "Font Entry #%d", nI );
        ADDITEM();

        lstrcpy( lp, "---------------------------------------------------------------" );
        ADDITEM();

        lstrcpy( lp, lpfe->dfCopyright );
        ADDITEM();

        wsprintf( lp, "Ordinal: %#04x    Version:  %#04x    Size: %#08lx",
            lpfe->fontOrdinal, lpfe->dfVersion, lpfe->dfSize );
        ADDITEM();

        wsprintf( lp, "Type:      %#04x    Points:   %#04x    VertRes:  %#04x    HorzRes:  %#04x",
            lpfe->dfType, lpfe->dfPoints, lpfe->dfVertRes, lpfe->dfHorzRes );
        ADDITEM();

        wsprintf( lp, "Ascent:    %#04x    IntLead:  %#04x    ExtLead:  %#04x",
            lpfe->dfAscent, lpfe->dfIntLeading, lpfe->dfExtLeading );
        ADDITEM();
    
        wsprintf( lp, "Italic:    %#04x    Underline:%#04x    StrikeOut:%#04x",
            lpfe->dfItalic, lpfe->dfUnderline, lpfe->dfStrikeOut );
        ADDITEM();
        
        wsprintf( lp, "Weight:    %#04x    CharSet:  %#04x    PixWidth: %#04x    PixHeight: %#04x",
            lpfe->dfWeight, lpfe->dfCharSet, lpfe->dfPixWidth, lpfe->dfPixHeight );
        ADDITEM();

        wsprintf( lp, "Pitch:     %#04x    AveWidth: %#04x    MaxWidth: %#04x",
            lpfe->dfPitchAndFamily, lpfe->dfAvgWidth, lpfe->dfMaxWidth );
        ADDITEM();

        wsprintf( lp, "FirstChar: %#04x    LastChar: %#04x    DefChar:  %#04x    BreakChar: %#04x",
            lpfe->dfFirstChar, lpfe->dfLastChar, lpfe->dfDefaultChar, lpfe->dfBreakChar );
        ADDITEM();

        wsprintf( lp, "WidthBytes:%#04x    Device: %#08lx  Face: %#08lx",
            lpfe->dfWidthBytes, lpfe->dfDevice, lpfe->dfFace );
        ADDITEM();

        wsprintf( lp, "DeviceName: %s", lpDev );
        ADDITEM();

        wsprintf( lp, "FaceName:   %s", lpFace );
        ADDITEM();

        lpfe = (LPFONTENTRY)(lpFace + lstrlen(lpFace) + 1);
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithFontDir


//*** EOF: res.c
