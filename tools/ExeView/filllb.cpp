//*************************************************************
//  File name: filllb.c
//
//  Description: 
//      Contains all the code for filling the listbox with data
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
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

// index is RT_ values through RT_VERSION
const char *rc_types[] = {
    "",
    "CURSOR",
    "BITMAP",
    "ICON",
    "MENU",
    "DIALOG",
    "STRING",
    "FONTDIR",
    "FONT",
    "ACCELERATOR",
    "RCDATA",
    "",
    "GROUP CURSOR",
    "",
    "GROUP ICON",
    "NAME TABLE",
    "VERSION"
};

//*************************************************************
//
//  FillLBWithNewExeHeader
//
//  Purpose:
//      Fills the list box with the new exe info
//
//
//  Parameters:
//      HWND        hWnd
//      PEXEINFO    pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithNewExeHeader (HWND hWnd, PEXEINFO pExeInfo)
{
    char        szBuff[255];
    PNEWEXE     pne = &(pExeInfo->NewHdr);
    LPSTR       lp = (LPSTR)szBuff;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    lstrcpy( lp, "New EXE Header Info" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    if (pne->wFlags & LIBRARY)
        wsprintf( lp, "Library:\t\t\t\t%s", GetModuleName(pExeInfo) );
    else
        wsprintf( lp, "Module:\t\t\t\t%s", GetModuleName(pExeInfo)  );
    ADDITEM();

    wsprintf( lp, "Description:\t\t\t%s", GetModuleDescription(pExeInfo) );
    ADDITEM();

    wsprintf( lp, "Data:\t\t\t\t%s", GetExeDataType(pExeInfo) );
    ADDITEM();

    if (pne->bExeType == 0x02)  // Windows EXE
        wsprintf( lp, "Windows Version:\t\t\t%u.%u", HIBYTE(pne->wExpVersion),
            LOBYTE(pne->wExpVersion) );
    else
        lp[0] = 0;

    if (pne->wFlags & PMODEONLY)
        lstrcat( lp, " (PMODE ONLY!)" );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    wsprintf( lp, "Signature:\t\t\t%#04x", pne->wNewSignature );
    ADDITEM();

    wsprintf( lp, "Linker Version:\t\t\t%#04x", pne->cLinkerVer );
    ADDITEM();

    wsprintf( lp, "Linker Revision:\t\t\t%#04x", pne->cLinkerRev );
    ADDITEM();

    wsprintf( lp, "Entry Table Offset:\t\t%#04X", pne->wEntryOffset );
    ADDITEM();

    wsprintf( lp, "Entry Table Length:\t\t%#04X", pne->wEntrySize );
    ADDITEM();

    wsprintf(lp,"Checksum:\t\t\t%#08lX", pne->lChecksum );
    ADDITEM();
    
    wsprintf(lp,"Module Flags:\t\t\t%#04X", pne->wFlags );
    ADDITEM();

    wsprintf( lp, "DGROUP:\t\t\t\tseg %#04X", pne->wAutoDataSegment );
    ADDITEM();

    wsprintf( lp, "Heap Size:\t\t\t%#04X", pne->wHeapInit );
    ADDITEM();

    wsprintf( lp, "Stack Size:\t\t\t%#04X", pne->wStackInit );
    ADDITEM();

    wsprintf( lp, "Inital CS:IP:\t\t\tseg %#04X offset %#04x",
        pne->wCSInit, pne->wIPInit);
    ADDITEM();

    wsprintf( lp, "Inital SS:SP:\t\t\tseg %#04X offset %#04x",
        pne->wSSInit, pne->wSPInit);
    ADDITEM();

    wsprintf( lp, "Entries in Segment Table:\t\t%#04X", pne->wSegEntries );
    ADDITEM();

    wsprintf( lp, "Entries in Module Table:\t\t%#04X", pne->wModEntries );
    ADDITEM();

    wsprintf( lp, "Non-Resident Name Table Size:\t%#04X", pne->wNonResSize );
    ADDITEM();

    wsprintf( lp, "Segment Table Offset:\t\t%#04X", pne->wSegOffset );
    ADDITEM();

    wsprintf( lp, "Resource Table Offset:\t\t%#04X", pne->wResourceOffset );
    ADDITEM();

    wsprintf( lp, "Resident Name Table Offset:\t%#04X", pne->wResOffset );
    ADDITEM();

    wsprintf( lp, "Module Table Offset:\t\t%#04X", pne->wModOffset );
    ADDITEM();

    wsprintf( lp, "Imported Name Table Offset:\t%#04X", pne->wImportOffset );
    ADDITEM();

    wsprintf( lp, "Non-Resident Name Table Offset:\t%#08lX", pne->lNonResOffset );
    ADDITEM();

    wsprintf( lp, "Moveable entries:\t\t\t%#04X", pne->wMoveableEntry );
    ADDITEM();

    wsprintf( lp, "Alignment Shift Count:\t\t%#04X", pne->wAlign );
    ADDITEM();

    wsprintf( lp, "Resource Segments:\t\t%#04X", pne->wResourceSegs );
    ADDITEM();

    wsprintf( lp, "Executable Type:\t\t\t%#04X", pne->bExeType );
    ADDITEM();

    wsprintf( lp, "Additional Flags:\t\t\t%#04X", pne->bAdditionalFlags );
    ADDITEM();

    wsprintf( lp, "Fast Load Offset:\t\t\t%#04X", pne->wFastOffset );
    ADDITEM();

    wsprintf( lp, "Fast Load Length:\t\t\t%#04X", pne->wFastSize );
    ADDITEM();

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBNewExeInfo

//*************************************************************
//
//  FillLBWithOldExeHeader
//
//  Purpose:
//      Fills the listbox with the old exe info
//
//
//  Parameters:
//      HWND
//      LPOLDEXE
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithOldExeHeader (HWND hWnd, PEXEINFO pExeInfo )
{
    char  szBuff[255];
    LPSTR lp = (LPSTR)szBuff;
    POLDEXE pOldExeHdr = &(pExeInfo->OldHdr);

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    lstrcpy( lp, "Old EXE Header Info" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    wsprintf( lp, "File Signature:\t\t%#04X", pOldExeHdr->wFileSignature );
    ADDITEM();

    wsprintf( lp, "Bytes on last page:\t%#04X", pOldExeHdr->wLengthMod512 );
    ADDITEM();

    wsprintf( lp, "Pages in file:\t\t%#04X", pOldExeHdr->wLength );
    ADDITEM();

    wsprintf(lp,"Relocation items:\t\t%#04X",pOldExeHdr->wRelocationTableItems);
    ADDITEM();

    wsprintf(lp,"Paragraphs in header:\t%#04X",pOldExeHdr->wHeaderSize);
    ADDITEM();

    wsprintf(lp,"Extra paragraphs needed:\t%#04X",pOldExeHdr->wMinAbove);
    ADDITEM();

    wsprintf(lp,"Extra paragraphs desired:\t%#04X",pOldExeHdr->wDesiredAbove);
    ADDITEM();

    wsprintf(lp,"Initial SP value:\t\t%#04X",pOldExeHdr->wSP );
    ADDITEM();

    wsprintf(lp,"Checksum:\t\t%#04X",pOldExeHdr->wCheckSum );
    ADDITEM();

    wsprintf(lp,"Initial IP Value:\t\t%#04X",pOldExeHdr->wIP );
    ADDITEM();

    wsprintf(lp,"Code displacement:\t%#04X",pOldExeHdr->wCodeDisplacement );
    ADDITEM();

    wsprintf(lp,"Relocation table offset:\t%#04X",pOldExeHdr->wFirstRelocationItem );
    ADDITEM();

    wsprintf(lp,"Overlay Number:\t\t%#04X",pOldExeHdr->wOverlayNumber );
    ADDITEM();

    wsprintf(lp,"New EXE Offset:\t\t%#08lX",pOldExeHdr->lNewExeOffset );
    ADDITEM();

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithOldExeHeader

//*************************************************************
//
//  FillLBWithEntryTable
//
//  Purpose:
//      Fills the listbox with the Entry table Info
//
//
//  Parameters:
//      HWND hWnd
//      PEXEINFO pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithEntryTable (HWND hWnd, PEXEINFO pExeInfo )
{
    char  szBuff[255];
    LPSTR lp = (LPSTR)szBuff;
    PSTR  ptr;  
    WORD  wIndex=1;
    WORD  wI;
    BYTE  bBundles;
    BYTE  bFlags;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    lstrcpy( lp, "Entry Table" );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    lstrcpy( lp, "Ordinal\tSegment#   Offset   Stack Words   Flags" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    ptr = pExeInfo->pEntryTable;

    while (TRUE)
    {
        bBundles = (BYTE)*ptr++;
    
        if (bBundles==0)            // End of the table
            break;

        bFlags= (BYTE)*ptr++;

        switch (bFlags)
        {
            case 0x00:      // Placeholders
                if (bBundles == 1)
                    wsprintf( lp, "%d\t                                  Placeholder",
                        wIndex );
                else
                    wsprintf( lp, "%d-%d\t                                  Placeholders",
                        wIndex, wIndex + bBundles - 1 );
                ADDITEM();
                wIndex += bBundles;
            break;

            case 0xFF:      // MOVEABLE segments
                for (wI=0; wI<(WORD)bBundles; wI++)
                {
                    PMENTRY pe = (PMENTRY)ptr;
                    WORD    wS = ((WORD)pe->bFlags)>>2;

                    wsprintf( lp, "%d\t%#04x     %#04x   %#04x        MOVEABLE  ",
                        wIndex, pe->bSegNumber, pe->wSegOffset, wS );

                    if (pe->bFlags & EXPORTED)
                        lstrcat( lp, "EXPORTED " );
                    else
                        lstrcat( lp, "         " );

                    if (pe->bFlags & SHAREDDATA)
                        lstrcat( lp, "SHARED_DATA" );

                    ADDITEM();
                    wIndex++;
                    ptr += sizeof( MENTRY );
                }
            break;

            default:        // FIXED Segments
                for (wI=0; wI<(WORD)bBundles; wI++)
                {
                    PFENTRY pe = (PFENTRY)ptr;
                    WORD    wS = ((WORD)pe->bFlags)>>2;

                    wsprintf( lp, "%d\t%#04x     %#04x   %#04x        FIXED     ",
                        wIndex, bFlags, pe->wSegOffset, wS );

                    if (pe->bFlags & EXPORTED)
                        lstrcat( lp, "EXPORTED  " );
                    else
                        lstrcat( lp, "         " );

                    if (pe->bFlags & SHAREDDATA)
                        lstrcat( lp, "SHARED_DATA" );

                    ADDITEM();
                    wIndex++;
                    ptr += sizeof( FENTRY );
                }
            break;
        }
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithEntryTable

//*************************************************************
//
//  FillLBWithSegments
//
//  Purpose:
//      Fills the listbox with the Segment table Info
//
//
//  Parameters:
//      HWND hWnd
//      PEXEINFO pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithSegments (HWND hWnd, PEXEINFO pExeInfo )
{
    char      szBuff[255];
    LPSTR     lp = (LPSTR)szBuff;
    PSEGENTRY pSeg = GetSegEntry( pExeInfo, 0 );
    int       i=0;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    lstrcpy( lp, "Segment Table" );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    lstrcpy( lp, "Type   Sector(<<align)  Length   Memory   Flags" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    while (pSeg)
    {
        FormatSegEntry( pExeInfo, pSeg, lp );
        ADDITEM();
        i++;
        pSeg = GetSegEntry( pExeInfo, i );
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithSegments

//*************************************************************
//
//  FillLBWithResources
//
//  Purpose:
//      Fills the listbox with the Resource Table info
//
//
//  Parameters:
//      HWND hWnd
//      PEXEINFO pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithResources (HWND hWnd, PEXEINFO pExeInfo )
{
    char     szBuff[255];
    LPSTR    lp = (LPSTR)szBuff;
    char     szRCType[30];
    LPCSTR   lpn;
    PRESTYPE prt = pExeInfo->pResTable;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )
    #define SETDATA() SendMessage( hWnd, LB_SETITEMDATA, , (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    wsprintf( lp, "Shift Count: %d", pExeInfo->wShiftCount );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    lstrcpy( lp, "[TYPE]\t\tOffset       Length       Flags                  Name" );
    ADDITEM();

    lstrcpy( lp, "----------------------------------------------------------------------------" );
    ADDITEM();

    while (prt)
    {
        PRESINFO pri = prt->pResInfoArray;
        WORD     wI = 0;

        if (prt->wType & 0x8000)
        {
            WORD wType = prt->wType&0x7fff;

            if (wType==0 || wType==11 || wType==13 || wType > 16)
            {
                lpn = (LPSTR)szRCType;
                wsprintf( szRCType, "Unknown Type: %#04X\0", prt->wType );
            }
            else    
                lpn = rc_types[ prt->wType&0x7fff ];
        }
        else
            lpn = prt->pResourceType;

        wsprintf( lp, "[%s]", lpn );
        ADDITEM();

        while (wI < prt->wCount)
        {
            int nIndex;

            FormatResourceEntry( pExeInfo, pri, lp );
            if ((nIndex = (int)ADDITEM()) >= 0)
                SendMessage(hWnd, LB_SETITEMDATA, nIndex, (LPARAM)pri);
            // Set the item data for use when double clicking

            pri++;
            wI++;
        }

        prt = prt->pNext;
        if (prt)
        {
            lstrcpy( lp, "" );
            ADDITEM();
        }
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithResources

//*************************************************************
//
//  FillLBWithResidentNames
//
//  Purpose:
//      Fills the listbox with the Resident Name table
//
//
//  Parameters:
//      HWND hWnd
//      PEXEINFO pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithResidentNames (HWND hWnd, PEXEINFO pExeInfo )
{
    char  szBuff[255];
    LPSTR lp = (LPSTR)szBuff;
    PNAME pName = pExeInfo->pResidentNames;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    if (pName)                  // First item is module name
        pName = pName->pNext;

    lstrcpy( lp, "Resident Name Table" );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    lstrcpy( lp, "Ordinal  Name" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    while (pName)
    {
        wsprintf( lp, "%#04x   %s", pName->wOrdinal,
            (LPSTR)pName->szName );
        ADDITEM();
        pName = pName->pNext;
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithResidentNames

//*************************************************************
//
//  FillLBWithImportedNames
//
//  Purpose:
//      Fills the listbox with the Imported Name table Info
//
//
//  Parameters:
//      HWND hWnd
//      PEXEINFO pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithImportedNames (HWND hWnd, PEXEINFO pExeInfo )
{
    char  szBuff[255];
    LPSTR lp = (LPSTR)szBuff;
    PNAME pName = pExeInfo->pImportedNames;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    lstrcpy( lp, "Imported Name Table" );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    lstrcpy( lp, "Name" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    while (pName)
    {
        lstrcpy( lp, pName->szName );
        ADDITEM();
        pName = pName->pNext;
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithImportedNames

//*************************************************************
//
//  FillLBWithNonResidentNames
//
//  Purpose:
//      Fills the listbox with the NonResident Name table Info
//
//
//  Parameters:
//      HWND hWnd
//      PEXEINFO pExeInfo
//      
//
//  Return: (BOOL)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

BOOL FillLBWithNonResidentNames (HWND hWnd, PEXEINFO pExeInfo )
{
    char  szBuff[255];
    LPSTR lp = (LPSTR)szBuff;
    PNAME pName = pExeInfo->pNonResidentNames;

    #define ADDITEM() SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)lp )

    SendMessage( hWnd, WM_SETREDRAW, 0, 0L );
    SendMessage( hWnd, LB_RESETCONTENT, 0, 0L );

    if (pName)                  // First item is module definition
        pName = pName->pNext;
    
    lstrcpy( lp, "Non-Resident Name Table" );
    ADDITEM();

    lstrcpy( lp, "" );
    ADDITEM();

    lstrcpy( lp, "Ordinal  Name" );
    ADDITEM();

    lstrcpy( lp, "---------------------------------------------------------------" );
    ADDITEM();

    while (pName)
    {
        wsprintf( lp, "%#04x   %s", pName->wOrdinal,
            (LPSTR)pName->szName );
        ADDITEM();
        pName = pName->pNext;
    }

    SendMessage( hWnd, WM_SETREDRAW, 1, 0L );
    InvalidateRect( hWnd, NULL, TRUE );
    UpdateWindow( hWnd );
    return TRUE;

} //*** FillLBWithNonResidentNames

//*************************************************************
//
//  FormatSegEntry
//
//  Purpose:
//      Formats the entry structure and puts it in the 'lp' buffer
//
//
//  Parameters:
//      PEXEINFO  pExeInfo
//      PSEGENTRY pSeg
//      LPSTR     lp
//      
//
//  Return: (VOID)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

LPSTR FormatSegEntry ( PEXEINFO pExeInfo, PSEGENTRY pSeg, LPSTR lp )
{
    LPSTR lpType = NULL;
    WORD  wAlign = pExeInfo->NewHdr.wAlign;

    if (wAlign == 0)
        wAlign = 9;

    if (pSeg->wFlags & F_DATASEG)
        lpType = (LPSTR)"DATA";
    else
        lpType = (LPSTR)"CODE";

    wsprintf( lp, "%s   %#08lx       %#04x   %#04x   ", lpType,
        ((LONG)pSeg->wSector)<<wAlign,
    pSeg->wLength, pSeg->wMinAlloc );

    if (pSeg->wFlags & F_PRELOAD)
        lstrcat( lp, "PRELOAD " );
    else
        lstrcat( lp, "        " );

    if (pSeg->wFlags & F_MOVEABLE)
        lstrcat( lp, "(moveable) " );
    else
        lstrcat( lp, "           " );

    if (pSeg->wFlags & F_DISCARDABLE)
        lstrcat( lp, "(discardable)" );
    else
        lstrcat( lp, "             " );

    return lp;

} //*** FormatSegEntry

//*************************************************************
//
//  FormatResourceEntry
//
//  Purpose:
//      Formats the resource info and puts it in the 'lp' buffer
//
//
//  Parameters:
//      PEXEINFO  pExeInfo
//      PRESINFO  pri
//      LPSTR     lp
//      
//
//  Return: (VOID)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

LPSTR FormatResourceEntry ( PEXEINFO pExeInfo, PRESINFO pri, LPSTR lp )
{
    WORD  wShift = pExeInfo->wShiftCount;

    wsprintf( lp, "-->\t\t%#08lX   %#08lX   ", ((LONG)pri->wOffset)<<wShift,
        ((LONG)pri->wLength)<<wShift );

    if ( pri->wFlags & F_MOVEABLE )
        lstrcat( lp, "MOVEABLE " );
    else
        lstrcat( lp, "         " );

    if ( pri->wFlags & F_SHAREABLE )
        lstrcat( lp, "PURE " );
    else
        lstrcat( lp, "     " );

    if ( pri->wFlags & F_PRELOAD  )
        lstrcat( lp, "PRELOAD  " );
    else
        lstrcat( lp, "         " );

    if (pri->wID & 0x8000)  // Integer resource
    {
        char temp[10];

        wsprintf( (LPSTR)temp, "%#04X", pri->wID & 0x7fff);
        lstrcat( lp, temp );
    }
    else
        lstrcat( lp, pri->pResourceName );

    return lp;

} //*** FormatResourceEntry

//*** EOF: filllb.c
