/********************************************************************

wfloc.c

Winfile localization selection procedures

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.

********************************************************************/
#include "winfile.h"

/*
The Language names are sorted like this because CBS_SORT Flag for comboboxes causes bugs.
cf. https://msdn.microsoft.com/en-us/library/cc233982.aspx
*/
LPCTSTR szLCIDs[] = {
    TEXT("zh-CN"),    // Chinese, Simplified
    TEXT("en-US"),    // English, United States
    TEXT("he-IL"),    // Hebrew, Israel
    TEXT("ja-JP"),    // Japanese, Japan
    TEXT("pl-PL"),    // Polish, Poland
};

VOID InitLangList(HWND hCBox)
{
    // Propogate the list
    for (UINT i = 0; i <= (COUNTOF(szLCIDs) - 1); i++)
    {
        TCHAR szLangName[MAX_PATH] = { 0 };
        LCID lcidTemp = LocaleNameToLCID(szLCIDs[i], 0);

        // TODO: need to test this on pre-Vista and on/after Win XP 64
        if (GetLocaleInfoEx(szLCIDs[i], LOCALE_SLOCALIZEDDISPLAYNAME, szLangName, COUNTOF(szLangName)) == 0)
            lstrcpy(szLangName, TEXT("BUGBUG"));

        // every entry in the array above needs to be addd to the list box;
        // SaveLang() below depends on each index in the listbox being valid.
        SendMessage(hCBox, CB_ADDSTRING, 0, (LPARAM)szLangName);

        //Select entry if its the currently active language
        if (lcidTemp == lcid)
        {
            SendMessage(hCBox, CB_SETCURSEL, i, 0);
        }
    }
}

VOID SaveLang(HWND hCBox)
{
    int iIndex = SendMessage(hCBox, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
        return; // do nothing

    WritePrivateProfileString(szSettings, szUILanguage, szLCIDs[iIndex], szTheINIFile);
}

DWORD MirrorUI()
{
    DWORD exStyle = 0L;

    switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        /* Additional Languages can be added */
        case LANG_HEBREW:
            if (!bToggleDir)
                exStyle |= WS_EX_LAYOUTRTL;
            else /* Classy Win3.11 look */
                exStyle |= (WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT);
            break;
        default:
            if (bToggleDir)
                exStyle |= WS_EX_LAYOUTRTL;
            break;
    }

    return exStyle;
}

VOID MirrorHDC(HDC hdc)
{
    if (GetLayout(hdc) == LAYOUT_RTL)
        SetLayout(hdc, LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED);
}
