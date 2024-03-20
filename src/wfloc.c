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

typedef struct _FM_LANG {
    LPCWSTR String;
    LANGID Lang;
} FM_LANG;

FM_LANG fmLCIDs[] = {
    {TEXT("zh-CN"), MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)}, // Chinese, Simplified
    {TEXT("en-US"), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)},         // English, United States
    {TEXT("he-IL"), MAKELANGID(LANG_HEBREW, SUBLANG_HEBREW_ISRAEL)},       // Hebrew, Israel
    {TEXT("ja-JP"), MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN)},    // Japanese, Japan
    {TEXT("pl-PL"), MAKELANGID(LANG_POLISH, SUBLANG_POLISH_POLAND)},       // Polish, Poland
    {TEXT("de-DE"), MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN)},              // German, Germany
    {TEXT("tr-TR"), MAKELANGID(LANG_TURKISH, SUBLANG_TURKISH_TURKEY)},     // Turkish, Turkey
    {TEXT("pt-PT"), MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE)}       // Portuguese, Portugal
};

LCID
WFLocaleNameToLCID(LPCWSTR lpName, DWORD dwFlags)
{
    LCID lcidTemp;
    DWORD i;

    lcidTemp = LOCALE_USER_DEFAULT;

    if (LocaleNameToLCID != NULL)
    {
        lcidTemp = LocaleNameToLCID(lpName, 0);
    }

    for (i = 0; i <= COUNTOF(fmLCIDs) - 1; i++)
    {
        if (lstrcmpi(lpName, fmLCIDs[i].String) == 0)
        {
            lcidTemp = MAKELCID(fmLCIDs[i].Lang, SORT_DEFAULT);
            break;
        }
    }

    return lcidTemp;
}

VOID InitLangList(HWND hCBox)
{
    // Propogate the list
    for (UINT i = 0; i <= (COUNTOF(fmLCIDs) - 1); i++)
    {
        TCHAR szLangName[MAXPATHLEN] = { 0 };
        LCID lcidTemp;

        lcidTemp = WFLocaleNameToLCID(fmLCIDs[i].String, 0);

        if (GetLocaleInfoEx != NULL)
        {
            if (GetLocaleInfoEx(fmLCIDs[i].String, LOCALE_SLOCALIZEDDISPLAYNAME, szLangName, COUNTOF(szLangName)) == 0)
            {
                lstrcpy(szLangName, TEXT("BUGBUG"));
            }
        }
        else
        {
            if (GetLocaleInfo(lcidTemp, LOCALE_SLOCALIZEDDISPLAYNAME, szLangName, COUNTOF(szLangName)) == 0)
            {
                lstrcpy(szLangName, TEXT("BUGBUG"));
            }
        }

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
    int iIndex = (INT)SendMessage(hCBox, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
        return; // do nothing

    WritePrivateProfileString(szSettings, szUILanguage, fmLCIDs[iIndex].String, szTheINIFile);
}

// returns whether the current language is default RTL; ARABIC, HEBREW: true; else false;
// ideally this would call GetLocalInfoEx with LOCALE_IREADINGLAYOUT, but that API was >=  Win7
BOOL DefaultLayoutRTL()
{
    switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
    /* Additional Languages can be added */
    case LANG_ARABIC:
    case LANG_HEBREW:
        return TRUE;
    default:
        return FALSE;
    }
}

// returns the extended style bits for the main window;
// if bMirrorContent, essentially reverses the RTL setting (but not the main layout in RTL langauges)
DWORD MainWindowExStyle()
{
    DWORD exStyle = 0L;

    if (DefaultLayoutRTL())
    {
        exStyle = WS_EX_LAYOUTRTL;
        if (!bMirrorContent)
            exStyle |= WS_EX_NOINHERITLAYOUT;
    }
    else
    {
        exStyle = bMirrorContent ? WS_EX_LAYOUTRTL : 0;
    }

    return exStyle;
}

VOID PreserveBitmapInRTL(HDC hdc)
{
    if (GetLayout(hdc) == LAYOUT_RTL)
        SetLayout(hdc, LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED);
}
