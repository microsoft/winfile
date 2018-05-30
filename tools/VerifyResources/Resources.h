/**************************************************************************

    Resources.h

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT License.

    For more information on resource file formats see
    https://msdn.microsoft.com/en-us/library/windows/desktop/ms648007(v=vs.85).aspx.

**************************************************************************/

#pragma once

#define MAXLANG 100

// defines the data for one resource type, name combination; contains all languages
typedef struct
{
    // true -> this resource type as multiple languages (e.g., RT_DIALOG)
    BOOL bHasMultiLang;

    // true -> first time and we gather the languages
    BOOL bGatherLang;

    // type and name of resource
    LPCTSTR lpType;
    LPCTSTR lpName;
    LPCTSTR szName;

    // handle to the resource; array indices are parallel to rglcidInUse
    HGLOBAL rghglobSeen[MAXLANG];
} PROCRES;

#define MAXDLGITEMS 100

// one dialog item; pointers are into the locked resource memory
typedef struct
{
    DLGITEMTEMPLATE *pitem;
    DLGITEMTEMPLATE itemtempl;

    LPCTSTR lpszClass;
    WORD wClass;       // if predefined

    LPCTSTR lpszTitle;
    WORD wTitle;       // if resource id

    WORD cbCreate;
    VOID *pvCreate;
} DLGITEMDECODE;

// decoded dialog as a whole; pointers are into locked resource memory
typedef struct
{
    DLGTEMPLATE *pdlg;
    DLGTEMPLATE dlgtempl;   // used only for extended dialogs

    LPCTSTR lpszMenu;   // if string
    WORD wMenu;        // if resoure id

    LPCTSTR lpszClass;
    WORD wClass;       // if predefined

    LPCTSTR lpszTitle;

    // if DS_SETFONT
    WORD wFont;        // size in points
    LPCTSTR lpszFont;

    // count of items is in pdlg->cdit
    DLGITEMDECODE rgitems[MAXDLGITEMS];
} DLGDECODE;

#define MAXMENUITEMS 100

#define MFR_END 0x80

// used for both normal and popup items
typedef struct
{
    WORD flags;
    WORD id;
    LPCTSTR lpszMenu;
} MENUITEM;

// decoded menu; pointers are into locked resource memory
typedef struct
{
    WORD wVersion;
    WORD offset;
    WORD flags;
    LPCTSTR lpszTitle;

    int cItem;
    MENUITEM rgitem[MAXMENUITEMS];
} MENUDECODE;