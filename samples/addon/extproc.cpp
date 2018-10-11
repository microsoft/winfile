/********************************************************************

   extproc.cpp

   Windows File Manager Sample Addon : extension entry point (UNICODE)

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "stdafx.h"

extern HMODULE hInstance;
WORD wMenuDelta; 
BOOL fToggle = FALSE; 

/*
LONG CALLBACK FMExtensionProc(HWND hwnd, WORD wMsg, LONG lParam)        -- ANSI
LONG CALLBACK FMExtensionProcW(HWND hwnd, WORD wMsg, LONG lParam)       -- UNICODE

Parameters:
	
	hwnd	Type: HWND
	A window handle to File Manager. 
	An extension uses this handle to specify the parent window for any dialog box or 
	message box it must display, and to send query messages to File Manager.

	wMsg	Type: WORD
	One of the following File Manager messages.

		1 through 99
		User selected an item from the extension-supplied menu. The value is the identifier of the selected menu item.

		FMEVENT_HELPMENUITEM
		User pressed F1 while selecting an extension menu or toolbar command item. Indicates that the extension should call WinHelp appropriately for the command item.

		FMEVENT_HELPSTRING
		User selected an extension menu or toolbar command item. Indicates that the extension should supply a Help string.

		FMEVENT_INITMENU
		User selected the extension's menu. The extension should initialize items in the menu.

		FMEVENT_LOAD
		File Manager is loading the extension DLL and prompts the DLL for information about the menu that the DLL supplies.

		FMEVENT_SELCHANGE
		Selection in the File Manager directory window or Search Results window has changed.

		FMEVENT_TOOLBARLOAD
		File Manager is creating the toolbar and prompts the extension DLL for information about any buttons the DLL adds to the toolbar.

		FMEVENT_UNLOAD
		File Manager is unloading the extension DLL.

		FMEVENT_USER_REFRESH
		User selected the Refresh command from the Window menu. The extension should update items in the menu, if necessary.
	
	lParam		Type: LONG
	Message-specific value.

	Return value	Type: LONG
	Returns a value dependent upon the wMsg parameter message.

    Usage: in order for Windows File Manager to load this dll, add a section to winfile.ini (located in %USERPROFILE%\Roaming\Microsoft\Winfile):

        [Addons]
        ext1=<path to dll>

    The key names ('ext1' in the example) don't matter, but must all be unique within the [Addons] section.  Specifying anything but a full path
    results in undefined behavior currently.

*/
LONG APIENTRY FMExtensionProcW(HWND hwnd, WPARAM wEvent, LPARAM lParam)
{
	// this is to export the function name as plain 'C'
	#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__) // my complain, but compiles well

	LONG returnValue = 0L;
    HMENU hMenu;
    TCHAR szBuf[200];
	
	switch (wEvent)
	{
	case FMEVENT_LOAD: 
		{
			/* File Manager is loading the extension DLL and prompts the DLL for information about the menu that the DLL supplies. */
			wMenuDelta = ((LPFMS_LOAD)lParam)->wMenuDelta; 
			
			/* Fill the FMS_LOAD structure. */ 
			((LPFMS_LOAD) lParam)->dwSize = sizeof(FMS_LOAD); 
			lstrcpy(((LPFMS_LOAD)lParam)->szMenuName, L"AddonSampleMenu");
			
			/* Return the handle to the menu. */ 
			HMENU hm = GetSubMenu(LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU)), 0); // IDR_MENU from resource.h

			((LPFMS_LOAD)lParam)->hMenu = hm;
			
			returnValue = (LONG)TRUE; 
		}
		break;
	case FMEVENT_UNLOAD: 
		{
			/* File Manager is unloading the extension DLL. */
		}
		break; 
	case FMEVENT_INITMENU: 
		{
			/* User selected the extension's menu. The extension should initialize items in the menu. */

			hMenu = (HMENU) lParam; 

			// Add check marks to menu items as appropriate. 
			// Add menu-item delta values to menu-item 
			// identifiers to specify the menu items to check.
			CheckMenuItem(hMenu, wMenuDelta + IDM_TOGGLE,  fToggle ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED); 
		}
		break; 
	case FMEVENT_TOOLBARLOAD: 
		{
			/* File Manager is creating the toolbar and prompts the extension DLL for information about any buttons the DLL adds to the toolbar. */
			/* Gets called before winfile shows the toolbar. */
			
			// EXT_BUTTON:
            // Parameter 1: <id>
            // Parameter 2: <help string id>
			// Parameter 3: 0=normal, 1=invisible/separator, 2=pressed-not pressed
			static EXT_BUTTON extbtn[ ] = { { IDM_FIRSTBUTTON, 0, 0} }; 
			
			// Fill the FMS_TOOLBARLOAD structure.
			((LPFMS_TOOLBARLOAD)lParam)->dwSize = sizeof(FMS_TOOLBARLOAD); 
			((LPFMS_TOOLBARLOAD)lParam)->lpButtons = (LPEXT_BUTTON) &extbtn; 
			((LPFMS_TOOLBARLOAD)lParam)->cButtons = 1; // Number of buttons in the list (incl. Seperator)
			((LPFMS_TOOLBARLOAD)lParam)->cBitmaps = 1; // Number of Buttons, shown with the current image
			((LPFMS_TOOLBARLOAD)lParam)->idBitmap = IDB_TEST; //IDB_TEST from resource.h 
			
			returnValue = (LONG)TRUE; 
		}
		break;
	case FMEVENT_USER_REFRESH: 
		{
			/* User selected the Refresh command from the Window menu. The extension should update items in the menu, if necessary. */
		}
		break; 
	case FMEVENT_SELCHANGE: 
		{
			/* Selection in the File Manager directory window or Search Results window has changed. */
			/* reacts only on the right half of the window and only left click */
		}
		break; 
	case FMEVENT_HELPSTRING:
		{
			/* User selected an extension menu or toolbar command item. Indicates that the extension should supply a Help string. */

			if ( ((LPFMS_HELPSTRING)lParam)->idCommand == -1 )
			{
				lstrcpy(((LPFMS_HELPSTRING)lParam)->szHelp, L"The mouse is over the menu item (Extension)."); 
			}
			else 
			{
				wsprintf(((LPFMS_HELPSTRING)lParam)->szHelp, L"Tooltip for item %d", ((LPFMS_HELPSTRING)lParam)->idCommand); 
			}
		}
		break; 
	case FMEVENT_HELPMENUITEM: 
		{
			/* 
			User pressed F1 while selecting an extension menu or toolbar command item. 
			Indicates that the extension should call WinHelp appropriately for the command item. 
			*/
			wsprintf(szBuf, L"Help for %d", lParam); 
			MessageBox(hwnd, szBuf, L"WinHelp call", MB_OK); 
			/* Use: WinHelp(hwnd, "ExtHelp.hlp", HELP_CONTEXT, lParam);  */ 
		}
		break; 
	// ***** Till here FMEVENT_... *****
	// now the handling of the self defined actions
	case IDM_FIRSTBUTTON: // (1)
		{
			INT focus = (INT)SendMessage(hwnd, FM_GETFOCUS, 0, 0);

			switch( focus ) 
			{
			case FMFOCUS_DIR:
				MessageBox(hwnd, L"Focus is on the right side.", L"Test-Plugin", MB_OK); 
				break;
			case FMFOCUS_TREE:
				MessageBox(hwnd, L"Focus is on the left side.", L"Test-Plugin", MB_OK); 
				break;
			};
		}
		break; 
	case IDM_TESTMENU: 
		{
			MessageBox(hwnd, L"Hi test!", L"IDM_TESTMENU", MB_OK);
		}
		break; 
	case IDM_TOGGLE: 
		{
			MessageBox(hwnd, fToggle ? L"Hi On" : L"Hi Off", L"IDM_TOGGLE", MB_OK);
			fToggle = !fToggle; 
		}
		break; 
	default: 
		{
			wsprintf(szBuf, L"Unrecognized idm: %d", (INT) wEvent);
			MessageBox(hwnd, szBuf, L"Error", MB_OK);
		}
		break;
	} 

	return returnValue; 
}
