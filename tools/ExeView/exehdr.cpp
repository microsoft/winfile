//*************************************************************
//  File name: exehdr.c
//
//  Description: 
//      Routines for reading the exe headers and resources
//
//  History:    Date       Author     Comment
//               1/16/92   MSM        Created
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
//  LoadExeInfo
//
//  Purpose:
//      Loads in the header information from the EXE
//
//
//  Parameters:
//      LPSTR    lpFile
//
//  Return: (PEXEINFO)
//
//
//  Comments:
//      This function will allocate a EXEINFO structure and
//      fill it out from the given filename.  This routine,
//      if successful will return a pointer to this structure.
//      If it fails, it returns a LERR_???? code.
//
//  History:    Date       Author     Comment
//               1/16/92   MSM        Created
//
//*************************************************************

PEXEINFO LoadExeInfo (LPSTR lpFile)
{
    OFSTRUCT of;
    int      fFile=0, nLen, nErr=0;
    WORD     wSize;
    PEXEINFO pExeInfo;

#define ERROREXIT(X)    {nErr=X; goto error_out;}

// Allocate place main EXEINFO structure    
    pExeInfo = (PEXEINFO)LocalAlloc(LPTR, sizeof(EXEINFO));
    if (!pExeInfo)
        return (PEXEINFO)LERR_MEMALLOC;

// Open file and check for errors
    fFile = OpenFile( lpFile, &of, OF_READ );

    if (!fFile)
        ERROREXIT(LERR_OPENINGFILE);

// Allocate space for the filename
    pExeInfo->pFilename = (PSTR)LocalAlloc(LPTR, lstrlen(lpFile)+1 );
    if (!pExeInfo->pFilename)
        return (PEXEINFO)LERR_MEMALLOC;
        
    lstrcpy( pExeInfo->pFilename, lpFile );

// Read the OLD exe header
    nLen = (int)_lread(fFile, (LPSTR)&(pExeInfo->OldHdr), sizeof(OLDEXE));    

    if (nLen<sizeof(OLDEXE))
        ERROREXIT(LERR_READINGFILE);

    if (pExeInfo->OldHdr.wFileSignature != OLDSIG)
        ERROREXIT(LERR_NOTEXEFILE);

    if (pExeInfo->OldHdr.wFirstRelocationItem < 0x40)  // Old EXE
    {
        pExeInfo->NewHdr.wNewSignature = 0;
        _lclose( fFile );
        return pExeInfo;
    }

    _llseek( fFile, pExeInfo->OldHdr.lNewExeOffset, 0 );

// Read the NEW exe header
    nLen = (int)_lread(fFile, (LPSTR)&(pExeInfo->NewHdr), sizeof(NEWEXE));    

    if (nLen<sizeof(NEWEXE))
        ERROREXIT(LERR_READINGFILE);

    if (pExeInfo->NewHdr.wNewSignature != NEWSIG)
        ERROREXIT(LERR_NOTEXEFILE);

// Read entry table
    wSize = pExeInfo->NewHdr.wEntrySize;

    pExeInfo->pEntryTable=(PSTR)LocalAlloc(LPTR, wSize);

    if (!pExeInfo->pEntryTable)
        ERROREXIT(LERR_MEMALLOC);

    _llseek(fFile, pExeInfo->OldHdr.lNewExeOffset +
                   pExeInfo->NewHdr.wEntryOffset, 0 );

    nLen = _lread(fFile, (LPSTR)pExeInfo->pEntryTable, wSize );
    
    if (nLen != (int)wSize)
        ERROREXIT(LERR_READINGFILE);

// Read all the other tables
    if ( (nErr=ReadSegmentTable( fFile, pExeInfo )) < 0 )
        ERROREXIT(nErr);

// Do not read resources for OS/2 apps!!!!!!!
    if (pExeInfo->NewHdr.bExeType == 0x02)
    {
        if ( (nErr=ReadResourceTable( fFile, pExeInfo )) < 0 )
            ERROREXIT(nErr);
    }

    if ( (nErr=ReadResidentNameTable( fFile, pExeInfo )) < 0 )
        ERROREXIT(nErr);
    
    if ( (nErr=ReadImportedNameTable( fFile, pExeInfo )) < 0 )
        ERROREXIT(nErr);

    if ( (nErr=ReadNonResidentNameTable( fFile, pExeInfo )) < 0 )
        ERROREXIT(nErr);

    nErr = 1;

error_out:
// Close file and get outta here
    if (fFile)
        _lclose( fFile );

    if (nErr<=0)
    {
        FreeExeInfoMemory( pExeInfo );
        return (PEXEINFO)nErr;
    }
    return pExeInfo;

} //*** LoadExeInfo

//*************************************************************
//
//  FreeExeInfoMemory
//
//  Purpose:
//      Frees the memory associated created to store the info
//
//
//  Parameters:
//      PEXEINFO pExeInfo
//      
//
//  Return: (VOID)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/17/92   MSM        Created
//
//*************************************************************

VOID FreeExeInfoMemory (PEXEINFO pExeInfo)
{
    PNAME    pName = pExeInfo->pResidentNames;
    PRESTYPE prt   = pExeInfo->pResTable;

// Free Filename
    if (pExeInfo->pFilename)
        LocalFree( (HANDLE)pExeInfo->pFilename );

// Free Entry Table
    if (pExeInfo->pEntryTable)
        LocalFree( (HANDLE)pExeInfo->pEntryTable );

// Free Segment Table
    if (pExeInfo->pSegTable)
        LocalFree( (HANDLE)pExeInfo->pSegTable );

    while (prt) // Loop through the resource table
    {
        PRESTYPE prt_temp = prt->pNext;
        PRESINFO pri = prt->pResInfoArray;
        WORD     wI=0;

        // free if Resource array was allocated
        if (pri)
        {
            // Loop through and free any Resource Names
            while ( wI < prt->wCount )
            {
                if (pri->pResourceName)
                    LocalFree( (HANDLE)pri->pResourceName );
                wI++;
                pri++;
            }
            LocalFree( (HANDLE)prt->pResInfoArray );
        }

        // Free ResourceType name if there is one
        if (prt->pResourceType)
            LocalFree( (HANDLE)prt->pResourceType );

        // Free resource type header
        LocalFree( (HANDLE)prt );
        prt = prt_temp;
    }

// Free Resident Name Table
    while (pName)
    {
        PNAME pN2 = pName->pNext;

        LocalFree( (HANDLE)pName );
        pName = pN2;
    }

// Free Import Name Table
    pName = pExeInfo->pImportedNames;
    while (pName)
    {
        PNAME pN2 = pName->pNext;

        LocalFree( (HANDLE)pName );
        pName = pN2;
    }

// Free Non-Resident Name Table
    pName = pExeInfo->pNonResidentNames;
    while (pName)
    {
        PNAME pN2 = pName->pNext;

        LocalFree( (HANDLE)pName );
        pName = pN2;
    }

// Free PEXEINFO struct
    LocalFree( (HANDLE)pExeInfo );

} //*** FreeExeInfoMemory

//*************************************************************
//
//  ReadSegmentTable
//
//  Purpose:
//      LocalAllocs memory and reads in the segment table
//
//
//  Parameters:
//      int fFile
//      PEXEINFO pExeInfo
//      
//
//  Return: (int)
//      0 or error condition
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/17/92   MSM        Created
//
//*************************************************************

int ReadSegmentTable (int fFile, PEXEINFO pExeInfo)
{
    int       nLen;
    PSEGENTRY pSeg;
    long      lSegTable;
    WORD      wSegSize;

    wSegSize = sizeof(SEGENTRY)*pExeInfo->NewHdr.wSegEntries;

    if (wSegSize==0)
    {
        pExeInfo->pSegTable = NULL;
        return 0;
    }

// Allocate space for and read in the Segment table
    pSeg = (PSEGENTRY)LocalAlloc(LPTR, wSegSize );
    if (!pSeg)
        return LERR_MEMALLOC;

    lSegTable = pExeInfo->OldHdr.lNewExeOffset+pExeInfo->NewHdr.wSegOffset;

    _llseek( fFile, lSegTable, 0 );
    nLen = _lread( fFile, (LPSTR)pSeg, wSegSize );
    if (nLen != (int)wSegSize)
        return LERR_READINGFILE;

    pExeInfo->pSegTable = pSeg;
    return 0;

} //*** ReadSegmentTable

//*************************************************************
//
//  ReadResourceTable
//
//  Purpose:
//      LocalAllocs memory and reads in the resource headers
//
//
//  Parameters:
//      int fFile
//      PEXEINFO pExeInfo
//      
//
//  Return: (int)
//      0 or error condition
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/17/92   MSM        Created
//
//*************************************************************

int ReadResourceTable (int fFile, PEXEINFO pExeInfo)
{
    int       nLen;
    RESTYPE   rt;
    PRESTYPE  prt, prt_last=NULL;
    PRESINFO  pri;
    long      lResTable;
    WORD      wResSize, wI;
    int       ipri;

    rt.pResourceType = NULL;
    rt.pResInfoArray = NULL;
    rt.pNext         = NULL;

    if (pExeInfo->NewHdr.wResourceOffset == pExeInfo->NewHdr.wResOffset)
        return 0;       // No resources

    lResTable = pExeInfo->OldHdr.lNewExeOffset+pExeInfo->NewHdr.wResourceOffset;

    _llseek( fFile, lResTable, 0 );

    // Read shift count
    if (_lread(fFile, (LPSTR)&(pExeInfo->wShiftCount), 2)!=2)
        return LERR_READINGFILE;

    // Read all the resource types    
    while (TRUE)    
    {
        nLen = _lread(fFile, (LPSTR)&rt, sizeof(RTYPE));
        if (nLen != sizeof(RTYPE))
            return LERR_READINGFILE;
        if (rt.wType==0)
            break;

        prt = (PRESTYPE)LocalAlloc(LPTR, sizeof(RESTYPE) );
        if (!prt)
            return LERR_MEMALLOC;

        *prt = rt;

        if (prt_last==NULL)     // Is this the first entry??
            pExeInfo->pResTable = prt;
        else                    // Nope
            prt_last->pNext = prt;

        prt_last=prt;

        // Allocate buffer for 'Count' resources of this type
        wResSize = prt->wCount * sizeof( RESINFO2 );
        pri = (PRESINFO)LocalAlloc(LPTR, wResSize );
        if (!pri)
            return LERR_MEMALLOC;
        prt->pResInfoArray = pri;

        // Now read 'Count' resources of this type
        for (ipri = 0; ipri < prt->wCount; ipri++)
        {
            nLen = _lread(fFile, (LPSTR)(pri + ipri), sizeof(RINFO));
            if (nLen != sizeof(RINFO))
                return LERR_READINGFILE;

            (pri + ipri)->pResType = prt;
        }
    }

    // Now that the resources are read, read the names
    prt = pExeInfo->pResTable;

    while (prt)
    {
        if (prt->wType & 0x8000)        // Pre-defined type
            prt->pResourceType = NULL;
        else                            // Name is in the file
        {
             // wType is offset from beginning of Resource Table
            _llseek( fFile, lResTable + prt->wType, 0 );

            wResSize = 0;    
            // Read string size
            if (_lread(fFile, (LPSTR)&wResSize, 1)!=1)
                return LERR_READINGFILE;

            // +1 for the null terminator
            prt->pResourceType = (PSTR)LocalAlloc(LPTR, wResSize+1);
            if (!prt->pResourceType)
                return LERR_MEMALLOC;

            // Read string
            if (_lread(fFile, (LPSTR)prt->pResourceType, wResSize)!=wResSize)
                return LERR_READINGFILE;
            prt->pResourceType[ wResSize ] = 0;   // Null terminate string;
        }

        // Now do Resource Names for this type
        pri = prt->pResInfoArray;

        wI = 0;
        while ( wI < prt->wCount )
        {
            if (pri->wID & 0x8000)  // Integer resource
                pri->pResourceName = NULL;
            else                    // Named resource
            {
                // wID is offset from beginning of Resource Table
                _llseek( fFile, lResTable + pri->wID, 0 );

                wResSize = 0;
                // Read string size
                if (_lread(fFile, (LPSTR)&wResSize, 1)!=1)
                    return LERR_READINGFILE;

                // +1 for the null terminator
                pri->pResourceName = (PSTR)LocalAlloc(LPTR, wResSize+1);
                if (!pri->pResourceName)
                    return LERR_MEMALLOC;

                // Read string
                if (_lread(fFile, (LPSTR)pri->pResourceName, wResSize)!=wResSize)
                    return LERR_READINGFILE;
                pri->pResourceName[ wResSize ] = 0;   // Null terminate string;
            }
            pri++;
            wI++;
        }
        prt = prt->pNext;
    }
    return 0;

} //*** ReadResourceTable

//*************************************************************
//
//  ReadResidentNameTable
//
//  Purpose:
//		Reads in the Resident name table (First one being pModule)
//
//
//  Parameters:
//      int fFile
//      PEXEINFO pExeInfo
//      
//
//  Return: (int)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

int ReadResidentNameTable (int fFile, PEXEINFO pExeInfo)
{
    long         lResTable;
    WORD         wResSize;
    PNAME        pLast = NULL, pName = NULL;

    lResTable = pExeInfo->OldHdr.lNewExeOffset+pExeInfo->NewHdr.wResOffset;

    _llseek( fFile, lResTable, 0 );

    wResSize = 0;
    // Read string length
    if (_lread(fFile, (LPSTR)&wResSize, 1)!=1)
        return LERR_READINGFILE;

    while (wResSize)
    {
        pName = (PNAME)LocalAlloc(LPTR,sizeof(NAME)+wResSize);
        if (!pName)
            return LERR_MEMALLOC;

        if (!pLast)
            pExeInfo->pResidentNames = pName;
        else
            pLast->pNext = pName;
        pLast = pName;

        // Read string
        if (_lread(fFile, (LPSTR)pName->szName, wResSize)!=wResSize)
            return LERR_READINGFILE;
        pName->szName[ wResSize ] = 0;   // Null terminate string;

        // Read ordinal
        if (_lread(fFile, (LPSTR)&pName->wOrdinal, 2)!=2)
            return LERR_READINGFILE;

        wResSize = 0;
        // Read string length
        if (_lread(fFile, (LPSTR)&wResSize, 1)!=1)
            return LERR_READINGFILE;
    }
    return 0;

} /* ReadResidentNameTable() */

//*************************************************************
//
//  ReadImportedNameTable
//
//  Purpose:
//		Reads the Imported Name table
//
//
//  Parameters:
//      int fFile
//      PEXEINFO pExeInfo
//      
//
//  Return: (int)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

int ReadImportedNameTable (int fFile, PEXEINFO pExeInfo)
{
    long         lImpTable;
    long         lModTable;
    WORD         wImpSize, wModOffset, wImports;
    PNAME        pLast = NULL, pName = NULL;

    lModTable = pExeInfo->OldHdr.lNewExeOffset+pExeInfo->NewHdr.wModOffset;
    lImpTable = pExeInfo->OldHdr.lNewExeOffset+pExeInfo->NewHdr.wImportOffset;

    wImports = pExeInfo->NewHdr.wModEntries;
    if (!wImports)
        return 0;

    _llseek( fFile, lModTable, 0 );


// Read Import Names
    while (wImports)
    {

        // Load Module Reference Table offset
        _llseek( fFile, lModTable, 0 );

        // Move Module pointer to next string
        lModTable += 2L;
        if (_lread(fFile, (LPSTR)&wModOffset, 2)!=2)
            return LERR_READINGFILE;
        
        // Move file pointer to that offset in the Imported Table
        _llseek( fFile, lImpTable + wModOffset, 0 );

        wImpSize = 0;

        // Read string size
        if (_lread(fFile, (LPSTR)&wImpSize, 1)!=1)
            return LERR_READINGFILE;

        pName = (PNAME)LocalAlloc(LPTR,sizeof(NAME)+wImpSize);
        if (!pName)
            return LERR_MEMALLOC;

        if (!pLast)
            pExeInfo->pImportedNames = pName;
        else
            pLast->pNext = pName;
        pLast = pName;

        // Read string
        if (_lread(fFile, (LPSTR)pName->szName, wImpSize)!=wImpSize)
            return LERR_READINGFILE;
        pName->szName[ wImpSize ] = 0;   // Null terminate string;

        // Imported Names don't have ordinals
        pName->wOrdinal = 0;
        wImports--;
    }
    return 0;

} /* ReadImportedNameTable() */

//*************************************************************
//
//  ReadNonResidentNameTable
//
//  Purpose:
//		Reads in the NonResident name table (First one being pModuleDesc)
//
//
//  Parameters:
//      int fFile
//      PEXEINFO pExeInfo
//      
//
//  Return: (int)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

int ReadNonResidentNameTable (int fFile, PEXEINFO pExeInfo)
{
    long         lNonResTable;
    WORD         wNonResSize;
    PNAME        pLast = NULL, pName = NULL;

// Correction to the Sept. 1991 MSJ Article.  The value at
// offset 2CH is an offset from the beginning of the FILE not
// from the beginning of the New Executable Header.

    lNonResTable = pExeInfo->NewHdr.lNonResOffset;

    _llseek( fFile, lNonResTable, 0 );

    wNonResSize = 0;

    // Read string size
    if (_lread(fFile, (LPSTR)&wNonResSize, 1)!=1)
        return LERR_READINGFILE;

    while (wNonResSize)
    {
        pName = (PNAME)LocalAlloc(LPTR,sizeof(NAME)+wNonResSize);
        if (!pName)
            return LERR_MEMALLOC;

        if (!pLast)
            pExeInfo->pNonResidentNames = pName;
        else
            pLast->pNext = pName;
        pLast = pName;

        // Read string
        if (_lread(fFile, (LPSTR)pName->szName, wNonResSize)!=wNonResSize)
            return LERR_READINGFILE;
        pName->szName[ wNonResSize ] = 0;   // Null terminate string;

        // Read ordinal
        if (_lread(fFile, (LPSTR)&pName->wOrdinal, 2)!=2)
            return LERR_READINGFILE;

        wNonResSize = 0;
        // Read string size
        if (_lread(fFile, (LPSTR)&wNonResSize, 1)!=1)
            return LERR_READINGFILE;
    }
    return 0;

} /* ReadNonResidentNameTable() */

//*************************************************************
//
//  GetSegEntry
//
//  Purpose:
//      Retrieves a segment entry
//
//
//  Parameters:
//      PEXEINFO pExeInfo
//      int nIndex
//      
//
//  Return: (PSEGENTRY)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

PSEGENTRY GetSegEntry ( PEXEINFO pExeInfo, int nIndex )
{
    PSEGENTRY pSeg = pExeInfo->pSegTable;

    if (nIndex >= (int)pExeInfo->NewHdr.wSegEntries)
        return NULL;

    return (pSeg + nIndex);

} //*** GetSegEntry

//*************************************************************
//
//  GetModuleName
//
//  Purpose:
//      Retrieves the module name
//
//
//  Parameters:
//      PEXEINFO pExeInfo
//      
//
//  Return: (LPSTR)
//
//
//  Comments:
//      The module name is the first entry in the Resident Name Table
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

LPSTR GetModuleName ( PEXEINFO pExeInfo )
{
    if (pExeInfo->pResidentNames)
        return (LPSTR)(pExeInfo->pResidentNames->szName);
    else
        return (LPSTR)"";

} //*** GetModuleName

//*************************************************************
//
//  GetModuleDescription
//
//  Purpose:
//      Retrieves the module description
//
//
//  Parameters:
//      PEXEINFO pExeInfo
//      
//
//  Return: (LPSTR)
//
//
//  Comments:
//      The module description is the first entry in the NonResident Table
//
//  History:    Date       Author     Comment
//               1/20/92   MSM        Created
//
//*************************************************************

LPSTR GetModuleDescription ( PEXEINFO pExeInfo )
{
    if (pExeInfo->pNonResidentNames)
        return (LPSTR)(pExeInfo->pNonResidentNames->szName);
    else
        return (LPSTR)"";

} //*** GetModuleDescription

//*************************************************************
//
//  GetExeDataType
//
//  Purpose:
//      Retrieves the type of data for the executable
//
//
//  Parameters:
//      PEXEINFO pExeInfo
//      
//
//  Return: (LPSTR)
//
//
//  Comments:
//
//
//  History:    Date       Author     Comment
//               1/18/92   MSM        Created
//
//*************************************************************

LPSTR GetExeDataType (PEXEINFO pExeInfo)
{
    static    char szData[40];
    PSEGENTRY pSeg = pExeInfo->pSegTable;
    int       i;

    lstrcpy( szData, "NONE" );

    for (i=0; i<(int)pExeInfo->NewHdr.wSegEntries; i++)
    {
        if (pSeg->wFlags & F_DATASEG) // Data Segment
        {
            if (pSeg->wFlags & F_SHAREABLE)
                lstrcpy( szData, "SHARED" );
            else
                lstrcpy( szData, "NONSHARED" );
            return (LPSTR)szData;
        }
        pSeg++;
    }
    return (LPSTR)szData;    

} //*** GetExeDataType

//*** EOF: newexe.c
