/********************************************************************

wfloc.c

Winfile localization selection procedures

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.

********************************************************************/
#include "winfile.h"

/*
The Language names are sorted like this because CBS_SORT Flag for comboboxes causes bugs.
*/
LPCTSTR szLCIDs[] = { 
    TEXT("zh-CN"),    // Chinese, Simplified
    TEXT("en-US")     // English, United States
};

// Should be EXACTLY as many items are in the array above
CONST UINT cuCount = 2; 

VOID InitLangList(HWND hCBox)
{
    // Propogate the list
    for (UINT i = 0; i <= (cuCount - 1); i++)
    {
        TCHAR szLangName[MAX_PATH] = { 0 };
        LCID lcidTemp = LocaleNameToLCID((LPCTSTR)szLCIDs[i], 0);        

        LoadString(hAppInstance, (10000 + lcidTemp), szLangName, MAX_PATH);

        SendMessage(hCBox, CB_ADDSTRING, 0, (LPCTSTR)szLangName);

        //Select entry if its the currently active language
        if (lcidTemp == lcid) { 
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
