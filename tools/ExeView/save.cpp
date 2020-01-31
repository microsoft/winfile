/*************************************************************
    File name: save.c

    Description:
        Code for saving resources

    Copyright(c) Microsoft Corporation.All rights reserved.
    Licensed under the MIT License.

********************************************************************/

#include "stdafx.h"
#include "string.h"

extern const char *rc_types[];

BOOL SaveMenu(HFILE hFile, LPRESPACKET lprp);
BOOL SaveDialog(HFILE hFile, LPRESPACKET lprp);
BOOL SaveStringTable(HFILE hFile, LPRESPACKET lprp);

#define ADDITEM() _lwrite(hFile, lp, lstrlen(lp))

// save menu, dialog, and string resources to the file given
BOOL SaveResources(HWND hWnd, PEXEINFO pExeInfo, LPSTR lpFile)
{
    char     szBuff[255];
    LPSTR    lp = (LPSTR)szBuff;
    char     szRCType[30];
    LPCSTR   lpn;

    if (pExeInfo == NULL)
    {
        MessageBox(hWnd, "NO file open!",
            "ExeView Error", MB_ICONSTOP | MB_OK);

        return false;
    }

    PRESTYPE prt = pExeInfo->pResTable;
    HFILE    hFile = 0;

    hFile = _lcreat(lpFile, 0);
    if (!hFile)
    {
        MessageBox(hWnd, "Error opening file for write!",
            "ExeView Error", MB_ICONSTOP | MB_OK);

        return false;
    }

    lstrcpy(lp, "/********************************************************************\n\n    Resources that were extracted from a binary that ran on 16-bit Windows.\n\n");
    ADDITEM();

    lstrcpy(lp, "    Copyright(c) Microsoft Corporation.All rights reserved.\n    Licensed under the MIT License.\n\n********************************************************************/\n\n");
    ADDITEM();

    while (prt)
    {
        int      restype = 0;
        PRESINFO pri = prt->pResInfoArray;
        WORD     wI = 0;

        if (prt->wType & 0x8000)
        {
            WORD wType = prt->wType & 0x7fff;

            if (wType == 0 || wType == 11 || wType == 13 || wType > 16)
            {
                lpn = (LPSTR)szRCType;
                wsprintf(szRCType, "Unknown Type: %#04X\0", prt->wType);
            }
            else
            {
                lpn = rc_types[wType];
                restype = wType;
            }
        }
        else
        {
            lpn = prt->pResourceType;
            // restype == 0
        }

        if (restype == (int)RT_MENU || restype == (int)RT_DIALOG || restype == (int)RT_STRING)
        {
            while (wI < prt->wCount)
            {
                RESPACKET rp;
                LPRESPACKET lprp = &rp;

                if (LoadResourcePacket(pExeInfo, prt, pri, lprp))
                {
                    // for menu, dialog and string, dump details of this item
                    switch (restype)
                    {
                    case (int)RT_MENU:
                        SaveMenu(hFile, lprp);
                        break;

                    case (int)RT_DIALOG:
                        SaveDialog(hFile, lprp);
                        break;

                    case (int)RT_STRING:
                        SaveStringTable(hFile, lprp);
                        break;
                    }

                    FreeResourcePacket(lprp);

                    lstrcpy(lp, "\n");
                    ADDITEM();
                }

                pri++;
                wI++;
            }
            prt = prt->pNext;
            if (prt)
            {
                lstrcpy(lp, "\n");
                ADDITEM();
            }
        }
        else
        {
            prt = prt->pNext;
        }
    }

    _lclose(hFile);

    return TRUE;
}

// NOTE: copied from VerifyResources.cpp
WORD* DecodeMenu(WORD *lpv, MENUDECODE *pmenu)
{
    WORD *pwT = (WORD *)lpv;

    pmenu->flags = *pwT++;
    pmenu->lpszTitle = (LPCTSTR)pwT;
    pwT = (WORD *)((BYTE *)pwT + lstrlen(pmenu->lpszTitle) + 1);

    pmenu->cItem = 0;
    MENUITEM *pitem = &pmenu->rgitem[0];
    WORD flags;
    do
    {
        flags = pitem->flags = *pwT++;
        pitem->id = *pwT++;

        pitem->lpszMenu = (LPCTSTR)pwT;
        pwT = (WORD *)((BYTE *)pwT + lstrlen(pitem->lpszMenu) + 1);

        pmenu->cItem++;
        pitem++;
    } while (flags != MFR_END);

    return pwT;
}

BOOL SaveMenu(HFILE hFile, LPRESPACKET lprp)
{
    char     szBuff[255];
    LPSTR    lp = (LPSTR)szBuff;
    MENUDECODE menu;

    WORD *pwT = (WORD *)lprp->lpMem;

    char     szNTI[255];            // Name, Title, or Icon

    if (lprp->pri->wID & 0x8000)
    {
        wsprintf(szNTI, "%d", lprp->pri->wID & 0x7fff);
    }
    else
    {
        lstrcpy(szNTI, lprp->pri->pResourceName);
    }

    WORD wVersion = *pwT++;
    WORD offset = *pwT++;

    wsprintf(lp, "%s MENU\n", szNTI);
    ADDITEM();

    lstrcpy(lp, "BEGIN\n");
    ADDITEM();

    // first word is menu flags; if zero, we have hit the end
    while (*pwT != 0)
    {
        pwT = DecodeMenu(pwT, &menu);

        wsprintf(lp, "    POPUP   \"%s\"\n", menu.lpszTitle);
        ADDITEM();

        lstrcpy(lp, "    BEGIN\n");
        ADDITEM();

        for (int i = 0; i < menu.cItem; i++)
        {
            wsprintf(lp, "    MENUITEM    \"%s\", %#04X\n", menu.rgitem[i].lpszMenu, menu.rgitem[i].id);
            ADDITEM();
        }

        lstrcpy(lp, "    END\n");
        ADDITEM();
    }

    lstrcpy(lp, "END\n");
    ADDITEM();

    return TRUE;
}

// NOTE: copied from VerifyResources.cpp
VOID DecodeDlgItem(WORD **ppwT, DLGITEMDECODE *pdecitem)
{
    WORD *pwT = *ppwT;

    pdecitem->pitem = (DLGITEMTEMPLATE16 *)pwT;

    pwT = (WORD *)((char *)pwT + sizeof(DLGITEMTEMPLATE16));

    if (*(BYTE *)pwT == 0xff)
    {
        // 0xff at start of name is followed by a WORD
        pwT = (WORD *)((char *)pwT + 1);
        pdecitem->iconid = *pwT++;
        pdecitem->lpszTitle = "<iconid>";
    }
    else
    {
        pdecitem->lpszTitle = (LPCTSTR)pwT;
        pwT = (WORD *)((BYTE *)pwT + lstrlen(pdecitem->lpszTitle) + 1);
    }

    // not sure what this is used for...
    pdecitem->lpszSecond = (LPCTSTR)pwT;
    pwT = (WORD *)((BYTE *)pwT + lstrlen(pdecitem->lpszSecond) + 1);

    *ppwT = pwT;
}

// NOTE: copied from VerifyResources.cpp
VOID DecodeDialog(VOID *lpv, DLGDECODE *pdecdlg)
{
    WORD *pwT = (WORD *)lpv;

    pdecdlg->pdlg = (DLGTEMPLATE16 *)lpv;

    // continue after the DLGTEMPLATE16
    pwT = (WORD *)((char *)pwT + sizeof(DLGTEMPLATE16));

    pdecdlg->lpszTitle = (LPCTSTR)pwT;
    pwT = (WORD *)((BYTE *)pwT + lstrlen(pdecdlg->lpszTitle) + 1);

    if (pdecdlg->pdlg->style & DS_SETFONT)
    {
        pdecdlg->wFont = *pwT++;
        pdecdlg->lpszFont = (LPCTSTR)pwT;
        pwT = (WORD *)((BYTE *)pwT + lstrlen(pdecdlg->lpszFont) + 1);
    }

    if (pdecdlg->pdlg->cdit > MAXDLGITEMS)
    {
        // printf("Error: dialog has more than %d items\n", MAXDLGITEMS);
        pdecdlg->pdlg->cdit = MAXDLGITEMS;
    }

    for (int i = 0; i < pdecdlg->pdlg->cdit; i++)
    {
        DLGITEMDECODE *pitem = &pdecdlg->rgitems[i];

        ZeroMemory(pitem, sizeof(*pitem));

        DecodeDlgItem(&pwT, pitem);
    }
}

BOOL SaveDialog(HFILE hFile, LPRESPACKET lprp)
{
    DLGDECODE decdlg;

    ZeroMemory(&decdlg, sizeof(decdlg));

    DecodeDialog(lprp->lpMem, &decdlg);

    char     szBuff[255];
    LPSTR    lp = (LPSTR)szBuff;
    char     szNTI[255];            // Name, Title, or Icon

    if (lprp->pri->wID & 0x8000)
    {
        wsprintf(szNTI, "%d", lprp->pri->wID & 0x7fff);
    }
    else
    {
        lstrcpy(szNTI, lprp->pri->pResourceName);
    }

    wsprintf(lp, "%s DIALOG %#04x, %d, %d, %d, %d\n", szNTI, decdlg.pdlg->extra, decdlg.pdlg->x, decdlg.pdlg->y, decdlg.pdlg->cx, decdlg.pdlg->cy);
    ADDITEM();

    wsprintf(lp, "CAPTION \"%s\"\n", decdlg.lpszTitle);
    ADDITEM();

    if (decdlg.lpszFont != NULL)
    {
        wsprintf(lp, "FONT %d, \"%s\"\n", decdlg.wFont, decdlg.lpszFont);
        ADDITEM();
    }
    
    wsprintf(lp, "STYLE %#04x\n", decdlg.pdlg->style);
    ADDITEM();

    for (int i = 0; i < decdlg.pdlg->cdit; i++)
    {
        DLGITEMDECODE *pdecitem = &decdlg.rgitems[i];

        if (pdecitem->iconid != 0)
        {
            wsprintf(szNTI, "%d", pdecitem->iconid);
        }
        else
        {
            wsprintf(szNTI, "\"%s\"", pdecitem->lpszTitle);
        }

        wsprintf(lp, "    CONTROL %#02x, %s, %d, %#04x, %d, %d, %d, %d\n", pdecitem->pitem->kind, szNTI, pdecitem->pitem->id, pdecitem->pitem->style, pdecitem->pitem->x, pdecitem->pitem->y, pdecitem->pitem->cx, pdecitem->pitem->cy);
        ADDITEM();
    }

    lstrcpy(lp, "END\n");
    ADDITEM();

    return TRUE;
}

BOOL SaveStringTable(HFILE hFile, LPRESPACKET lprp)
{
    char  szBuff[270];
    LPSTR lpS, lp = (LPSTR)szBuff;
    int   nI, nOrdinal;

    nOrdinal = (lprp->pri->wID - 1) & 0x7fff;
    nOrdinal <<= 4;
    lpS = lprp->lpMem;

    lstrcpy(lp, "Ordinal  String\n");
    ADDITEM();

    lstrcpy(lp, "---------------------------------------------------------------\n");
    ADDITEM();

    for (nI = 0; nI < 16; nI++)
    {
        BYTE bLen = *lpS++;

        wsprintf(lp, "%#04x   \"", nOrdinal + nI);

        if (bLen)
        {
            strncat_s(lp, 270, lpS, (WORD)bLen);
            lpS += (int)bLen;
            lstrcat(lp, "\"\n");
            ADDITEM();
        }
    }

    return TRUE;
}
