/**************************************************************************

    VerifyResources.h

    Verify the consistency of resources so that dialogs, menus and strings
    all show up equally in all languages available in the module.

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT License.

**************************************************************************/

#include "stdafx.h"
#include <Windows.h>

#include "Resources.h"

#define COUNTOF(x) (sizeof(x)/sizeof(*x))

BOOL CALLBACK ProcessTypesInMod(HMODULE hMod, LPWSTR lpType, LONG_PTR lParam);
BOOL CALLBACK ProcessNamesInMod(HMODULE hMod, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam);
BOOL CALLBACK ProcessLangInMod(HMODULE hMod, LPCTSTR lpszType, LPCTSTR lpszName, WORD wLang, LONG_PTR lParam);

// tracks the languages we are processing; filled on first RT which has mulitple languages
#define ILOCALE_ENG 0
int iLocaleMac = 0;
LCID rglcidInUse[MAXLANG];

VOID VerifyLangDialogs(PROCRES *pprocres);
VOID VerifyLangMenus(PROCRES *pprocres);
VOID VerifyLangStrings(HMODULE hMod, PROCRES *pprocres);

int wmain(int argc, wchar_t *argv[])
{
    if (argc != 2)
    {
        printf("Usage: VerifyResources <exe>\n");
        return 1;
    }

    HMODULE hMod = LoadLibraryEx(argv[1], NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hMod == NULL)
    {
        printf("Error: can't load %ls; error %d\n", argv[0], GetLastError());
        return 1;
    }

    // en-US always "in use" and always the first one
    rglcidInUse[ILOCALE_ENG] = (LCID)MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    iLocaleMac++;

    EnumResourceTypes(hMod, ProcessTypesInMod, 0);

    return 0;
}

LPCTSTR MapRTToString(LPCTSTR lpType)
{
    if (IS_INTRESOURCE(lpType))
    {
        switch ((int)lpType)
        {
        case (int)RT_CURSOR: return L"RT_CURSOR";
        case (int)RT_BITMAP: return L"RT_BITMAP";
        case (int)RT_ICON: return L"RT_ICON";
        case (int)RT_MENU: return L"RT_MENU";
        case (int)RT_DIALOG: return L"RT_DIALOG";
        case (int)RT_STRING: return L"RT_STRING";
        case (int)RT_FONTDIR: return L"RT_FONTDIR";
        case (int)RT_FONT: return L"RT_FONT";
        case (int)RT_ACCELERATOR: return L"RT_ACCELERATOR";
        case (int)RT_RCDATA: return L"RT_RCDATA";
        case (int)RT_MESSAGETABLE: return L"RT_MESSAGETABLE";
        case (int)RT_MANIFEST: return L"RT_MANIFEST";
        case (int)RT_VERSION: return L"RT_VERSION";
        case (int)RT_GROUP_CURSOR: return L"RT_GROUP_CURSOR";
        case (int)RT_GROUP_ICON: return L"RT_GROUP_ICON";

        default: return L"RT_Unknown";
        }
    }
    else
    {
        return lpType;
    }
}

BOOL CALLBACK ProcessTypesInMod(HMODULE hMod, LPWSTR lpType, LONG_PTR lParam)
{
    printf("%ls\n", MapRTToString(lpType));

    return EnumResourceNames(hMod, lpType, ProcessNamesInMod, 0);
}

BOOL CALLBACK ProcessNamesInMod(HMODULE hMod, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
    wchar_t szName[100];
    PROCRES procres;

    if (IS_INTRESOURCE(lpszName))
    {
        wsprintf(szName, L"%d", (int)lpszName);
    }
    else
    {
        wcscpy_s(szName, COUNTOF(szName), lpszName);
    }

    ZeroMemory(&procres, sizeof(procres));
    procres.bHasMultiLang = lpszType == RT_DIALOG || lpszType == RT_MENU || lpszType == RT_STRING;
    procres.bGatherLang = iLocaleMac == 1 && procres.bHasMultiLang;
    procres.lpType = lpszType;
    procres.lpName = lpszName;
    procres.szName = szName;

    BOOL bRet = EnumResourceLanguages(hMod, lpszType, lpszName, ProcessLangInMod, (LPARAM)&procres);

    if (procres.bHasMultiLang)
    {
        // check that all languages were found
        for (int iloc = 0; iloc < iLocaleMac; iloc++)
        {
            if (procres.rghglobSeen[iloc] == NULL)
            {
                wchar_t szLocale[25];

                LCIDToLocaleName(rglcidInUse[iloc], szLocale, COUNTOF(szLocale), 0);

                printf("Error: missing lanaguage %ls for resource type %ls, name %ls\n", szLocale, MapRTToString(lpszType), szName);
            }
        }
    }
    else
    {
        if (procres.rghglobSeen[ILOCALE_ENG] == NULL)
        {
            printf("Error: missing lanaguage en-US for resource type %ls, name %ls\n", MapRTToString(lpszType), szName);
        }

        // extra language entries are caught below when scanning with !bGatherLang
    }
    
    // compare all version of resource
    if (procres.bHasMultiLang)
    {
        if (lpszType == RT_DIALOG)
        {
            VerifyLangDialogs(&procres);
        }
        else if (lpszType == RT_MENU )
        {
            VerifyLangMenus(&procres);
        }
        else /* lpszType == RT_STRING */
        {
            // this is called once per block of strings (16 max per block)
            VerifyLangStrings(hMod, &procres);
        }
    }

    return bRet;
}

BOOL CALLBACK ProcessLangInMod(HMODULE hMod, LPCTSTR lpszType, LPCTSTR lpszName, WORD wLang, LONG_PTR lParam)
{
    PROCRES *pprocres = (PROCRES *)lParam;

    // check that language is in global list and get that index
    int ilocale;
    for (ilocale = 0; ilocale < iLocaleMac; ilocale++)
    {
        if (rglcidInUse[ilocale] == (LCID)wLang)
            break;
    }

    if (ilocale == iLocaleMac)
    {
        if (!pprocres->bGatherLang)
        {
            wchar_t szLocale[25];

            LCIDToLocaleName((LCID)wLang, szLocale, COUNTOF(szLocale), 0);
            printf("Error: extra language %ls for resource type %ls, name %ls\n", szLocale, MapRTToString(pprocres->lpType), pprocres->szName);
            return TRUE;
        }

        // adding languages to global list
        ilocale = iLocaleMac;
        rglcidInUse[iLocaleMac++] = (LCID)wLang;
    }

    pprocres->rghglobSeen[ilocale] = LoadResource(hMod, FindResourceEx(hMod, lpszType, lpszName, wLang));

    return TRUE;
}

VOID DecodeDlgItem(WORD **ppwT, BOOL bExtended, DLGITEMDECODE *pdecitem)
{
    WORD *pwT = *ppwT;

    if (bExtended)
    {
        // after the helpID, the fields match DLGITEMTEMPLATE except
        // that the style and extended style fields are reversed and
        // that the id field is a DWORD here; WORD normally.

        pwT += 2;       // skip helpID

        DLGITEMTEMPLATE *pitemT = (DLGITEMTEMPLATE *)pwT;

        pdecitem->itemtempl.style = pitemT->dwExtendedStyle;
        pdecitem->itemtempl.dwExtendedStyle = pitemT->style;
        pdecitem->itemtempl.x = pitemT->x;
        pdecitem->itemtempl.y = pitemT->y;
        pdecitem->itemtempl.cx = pitemT->cy;
        pdecitem->itemtempl.cy = pitemT->cy;

        pdecitem->itemtempl.id = pitemT->id;    // reads lower word of id

        pwT++;      // skip upper word of id field; kind of a kludge

        pdecitem->pitem = &pdecitem->itemtempl;
    }
    else
    {
        pdecitem->pitem = (DLGITEMTEMPLATE *)pwT;
    }

    pwT = (WORD *)((char *)pwT + sizeof(DLGITEMTEMPLATE));

    // 0xffff means we have a integer class identitifer; otherwise a string;
    // all other cases like this are the same.
    if (*pwT == 0xffff)
    {
        pwT++;
        pdecitem->lpszClass = NULL;
        pdecitem->wClass = *pwT;
        pwT++;
    }
    else
    {
        pdecitem->lpszClass = (LPCTSTR)pwT;
        pwT += wcslen(pdecitem->lpszClass) + 1;
    }

    if (*pwT == 0xffff)
    {
        pwT++;
        pdecitem->lpszTitle = NULL;
        pdecitem->wTitle = *pwT;
        pwT++;
    }
    else
    {
        pdecitem->lpszTitle = (LPCTSTR)pwT;
        pwT += wcslen(pdecitem->lpszTitle) + 1;
    }

    pdecitem->cbCreate = *pwT;

    // skip creation data
    if (pdecitem->cbCreate == 0)
        pwT++;
    else
        pwT = (WORD *)((char *)pwT + pdecitem->cbCreate);

    // align on DWORD boundary
    pwT = (WORD *)RAWINPUT_ALIGN((INT_PTR)pwT);

    *ppwT = pwT;
}

VOID DecodeDialog(VOID *lpv, DLGDECODE *pdecdlg)
{
    BOOL bExtended;
    WORD *pwT = (WORD *)lpv;

    if (*pwT == 1 && *(pwT + 1) == 0xffff)
    {
        // dlgVer == 1 and signature == 0xffff means extended dialog;
        // after those fields and the helpID field, the fields in memory
        // match those of the DLGTEMPLATE except that the extended style and style are revsersed.

        pwT += 4; // skip over dlgVer, signature, and helpID

        DLGTEMPLATE *pdlgT = (DLGTEMPLATE *)pwT;

        pdecdlg->dlgtempl.style = pdlgT->dwExtendedStyle;
        pdecdlg->dlgtempl.dwExtendedStyle = pdlgT->style;
        pdecdlg->dlgtempl.cdit = pdlgT->cdit;
        pdecdlg->dlgtempl.x = pdlgT->x;
        pdecdlg->dlgtempl.y = pdlgT->y;
        pdecdlg->dlgtempl.cx = pdlgT->cy;
        pdecdlg->dlgtempl.cy = pdlgT->cy;

        pdecdlg->pdlg = &pdecdlg->dlgtempl;

        bExtended = TRUE;
    }
    else
    {
        pdecdlg->pdlg = (DLGTEMPLATE *)lpv;

        bExtended = FALSE;
    }

    // in both cases, continue after the DLGTEMPLATE
    pwT = (WORD *)((char *)pwT + sizeof(DLGTEMPLATE));

    // 0xffff means we have a integer class identitifer; otherwise a string;
    // all other cases like this are the same.
    if (*pwT == 0xffff)
    {
        pwT++;
        pdecdlg->lpszMenu = NULL;
        pdecdlg->wMenu = *pwT;
        pwT++;
    }
    else
    {
        pdecdlg->lpszMenu = (LPCTSTR)pwT;
        pwT += wcslen(pdecdlg->lpszMenu) + 1;
    }

    if (*pwT == 0xffff)
    {
        pwT++;
        pdecdlg->lpszClass = NULL;
        pdecdlg->wClass = *pwT;
        pwT++;
    }
    else
    {
        pdecdlg->lpszClass = (LPCTSTR)pwT;
        pwT += wcslen(pdecdlg->lpszClass) + 1;
    }

    pdecdlg->lpszTitle = (LPCTSTR)pwT;
    pwT += wcslen(pdecdlg->lpszTitle) + 1;

    if (pdecdlg->pdlg->style & DS_SETFONT)
    {
        pdecdlg->wFont = *pwT++;

        if (bExtended)
        {
            // if extended, skip weight, italic, charset
            pwT += 2;
        }

        pdecdlg->lpszFont = (LPCTSTR)pwT;
        pwT += wcslen(pdecdlg->lpszFont) + 1;
    }

    // align on DWORD boundary
    pwT = (WORD *)RAWINPUT_ALIGN((INT_PTR)pwT);

    if (pdecdlg->pdlg->cdit > MAXDLGITEMS)
    {
        printf("Error: dialog has more than %d items\n", MAXDLGITEMS);
        pdecdlg->pdlg->cdit = MAXDLGITEMS;
    }

    for (int i = 0; i < pdecdlg->pdlg->cdit; i++)
    {
        DLGITEMDECODE *pitem = &pdecdlg->rgitems[i];

        ZeroMemory(pitem, sizeof(*pitem));

        DecodeDlgItem(&pwT, bExtended, pitem);
    }
}

VOID VerifyLangDialogs(PROCRES *pprocres)
{
    DLGDECODE rgdecdlg[MAXLANG];       // ilocaleMac in use

    for (int ilocale = 0; ilocale < iLocaleMac; ilocale++)
    {
        LPVOID lpv = LockResource(pprocres->rghglobSeen[ilocale]);

        DLGDECODE *pdecdlg = &rgdecdlg[ilocale];

        ZeroMemory(pdecdlg, sizeof(*pdecdlg));

        DecodeDialog(lpv, pdecdlg);
    }

    // make sure all dialogs have the same number of items, style and same class
    DLGDECODE *pdecdlgEng = &rgdecdlg[ILOCALE_ENG];
    int citems = pdecdlgEng->pdlg->cdit;
    DWORD style = pdecdlgEng->pdlg->style;
    LPCTSTR lpszClass = pdecdlgEng->lpszClass;
    if (lpszClass == NULL) lpszClass = L"EMPTY";
    WORD wClass = pdecdlgEng->wClass;
    for (int ilocale = 1; ilocale < iLocaleMac; ilocale++)
    {
        LPCTSTR classCmp = L"EMPTY";
        wchar_t szLocale[25];
        DLGDECODE *pdecdlgT = &rgdecdlg[ilocale];

        LCIDToLocaleName(rglcidInUse[ilocale], szLocale, COUNTOF(szLocale), 0);

        if (pdecdlgT->pdlg->cdit != citems)
        {
            printf("Error: dialog %ls for language %ls does not have the required number of items %d\n", pdecdlgEng->lpszTitle, szLocale, citems);
            return;
        }

        if (pdecdlgT->lpszClass != NULL)
            classCmp = pdecdlgT->lpszClass;

        if (pdecdlgT->wClass != wClass || wcscmp(classCmp, lpszClass) != 0)
        {
            printf("Error: dialog %ls for language %ls does not have the required class %d\n", pdecdlgEng->lpszTitle, szLocale, wClass);
            return;
        }

        // warn on style diff
        if (pdecdlgT->pdlg->style != style)
        {
            printf("Warning: dialog %ls for language %ls does have the required style 0x%x\n", pdecdlgEng->lpszTitle, szLocale, style);
        }
    }

    // make sure all languages have the same items
    for (int i = 0; i < citems; i++)
    {
        int id = pdecdlgEng->rgitems[i].pitem->id;
        DWORD style = pdecdlgEng->rgitems[i].pitem->style;
        WORD wClass = pdecdlgEng->rgitems[i].wClass;

        for (int j = 1; j < iLocaleMac; j++)
        {
            wchar_t szLocale[25];
            DLGITEMDECODE *pdecitem = &rgdecdlg[j].rgitems[i];

            LCIDToLocaleName(rglcidInUse[j], szLocale, COUNTOF(szLocale), 0);

            if (pdecitem->pitem->id != id)
            {
                printf("Error: dialog %ls for language %ls does not have the required item %d\n", pdecdlgEng->lpszTitle, szLocale, id);
                break;
            }

            if (pdecitem->wClass != wClass)
            {
                printf("Error: dialog %ls item id %d for language %ls does have the required class %d\n", pdecdlgEng->lpszTitle, id, szLocale, wClass);
                break;
            }

            // warn on style diff
            if (pdecitem->pitem->style != style)
            {
                printf("Warning: dialog %ls item #%d, id %d for language %ls does have the required style 0x%x\n", pdecdlgEng->lpszTitle, i, id, szLocale, style);
            }
        }
    }
}

VOID DecodeMenu(VOID *lpv, MENUDECODE *pmenu)
{
    WORD *pwT = (WORD *)lpv;

    pmenu->wVersion = *pwT++;
    pmenu->offset = *pwT++;
    pmenu->flags = *pwT++;

    pmenu->lpszTitle = (LPCTSTR)pwT;
    pwT += wcslen(pmenu->lpszTitle) + 1;

    pmenu->cItem = 0;
    MENUITEM *pitem = &pmenu->rgitem[0];
    while (*pwT != MFR_END)
    {
        pitem->flags = *pwT++;
        pitem->id = *pwT++;

        pitem->lpszMenu = (LPCTSTR)pwT;
        pwT += wcslen(pitem->lpszMenu) + 1;

        pmenu->cItem++;
        pitem++;
    }
}

VOID VerifyLangMenus(PROCRES *pprocres)
{
    MENUDECODE rgmenu[MAXLANG];       // ilocaleMac in use

    for (int ilocale = 0; ilocale < iLocaleMac; ilocale++)
    {
        LPVOID lpv = LockResource(pprocres->rghglobSeen[ilocale]);

        MENUDECODE *pmenu = &rgmenu[ilocale];

        ZeroMemory(pmenu, sizeof(*pmenu));

        DecodeMenu(lpv, pmenu);
    }

    // make sure all menus have the same number of items
    int citems = rgmenu[ILOCALE_ENG].cItem;
    for (int ilocale = 1; ilocale < iLocaleMac; ilocale++)
    {
        MENUDECODE *pmenu = &rgmenu[ilocale];

        if (pmenu->cItem != citems)
        {
            wchar_t szLocale[25];

            LCIDToLocaleName(rglcidInUse[ilocale], szLocale, COUNTOF(szLocale), 0);

            printf("Error: menu %ls for language %ls does not have the required number of items %d\n", pmenu->lpszTitle, szLocale, citems);
            return;
        }
    }

    // make sure all languages have the same items
    for (int i = 0; i < citems; i++)
    {
        int id = rgmenu[ILOCALE_ENG].rgitem[i].id;

        for (int j = 1; j < iLocaleMac; j++)
        {
            wchar_t szLocale[25];
            MENUITEM *pitem = &rgmenu[j].rgitem[i];

            LCIDToLocaleName(rglcidInUse[j], szLocale, COUNTOF(szLocale), 0);

            if (pitem->id != id)
            {
                printf("Error: menu %ls for language %ls does not have the required item %d\n", rgmenu[ILOCALE_ENG].lpszTitle, szLocale, id);
                break;
            }
        }
    }
}

// string table blocks are exactly 16 strings;
// zero length strings are not present and don't have an id;
// for the given block, we return a bitmask for each non-zero length string.
WORD GetUsedStringsMask(HGLOBAL hglob)
{
    WORD wMask = 0;

    if (hglob)
    {
        LPCTSTR psz = (LPCTSTR)LockResource(hglob);
        for (int i = 0; i < 16; i++)
        {
            if ((WORD)*psz != 0)
                wMask |= (1 << i);

            psz += 1 + (WORD)*psz;
        }
    }

    return wMask;
}

// called once per block; for more information see https://blogs.msdn.microsoft.com/oldnewthing/20040130-00/?p=40813/
VOID VerifyLangStrings(HMODULE hMod, PROCRES *pprocres)
{
    HGLOBAL hglobEng = pprocres->rghglobSeen[ILOCALE_ENG];
    WORD wUsedEng = GetUsedStringsMask(hglobEng);

    // printf("Found eng block %d with mask 0x%x\n", iblock, wUsedEng);

    for (int ilocale = 1; ilocale < iLocaleMac; ilocale++)
    {
        WORD wUsedT = 0;
        wchar_t szLocale[25];

        LCIDToLocaleName(rglcidInUse[ilocale], szLocale, COUNTOF(szLocale), 0);

        wUsedT = GetUsedStringsMask(pprocres->rghglobSeen[ilocale]);

        // printf("Found %ls block %d with mask 0x%x\n", szLocale, iblock, wUsedT);

        if (wUsedT != wUsedEng)
        {
            printf("Error: string table block usage mismatch for language %ls block %d\n", szLocale, (int)pprocres->lpName);
        }
    }
}
