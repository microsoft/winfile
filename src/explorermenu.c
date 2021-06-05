#include <ShlObj.h>

//
// How to host an IContextMenu, part 1 – Initial foray
// https://devblogs.microsoft.com/oldnewthing/20040920-00/?p=37823
// 
// How to host an IContextMenu, part 2 – Displaying the context menu
// https://devblogs.microsoft.com/oldnewthing/20040922-00/?p=37793
//

static HRESULT GetUIObjectOfFile(HWND hwnd, LPCWSTR pszPath, REFIID riid, void **ppv)
{
	*ppv = NULL;
	HRESULT hr;
	LPITEMIDLIST pidl;
	SFGAOF sfgao;
	if (SUCCEEDED(hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, &sfgao)))
	{
		IShellFolder *psf;
		LPCITEMIDLIST pidlChild;
		if (SUCCEEDED(hr = SHBindToParent(pidl, &IID_IShellFolder, (void **)&psf, &pidlChild)))
		{
			hr = psf->lpVtbl->GetUIObjectOf(psf, hwnd, 1, &pidlChild, riid, NULL, ppv);
			psf->lpVtbl->Release(psf);
		}
		CoTaskMemFree(pidl);
	}
	return hr;
}

#define SCRATCH_QCM_FIRST 1
#define SCRATCH_QCM_LAST  0x7FFF

void ShowExplorerContextMenu(HWND hwnd, LPCWSTR pFileName, UINT xPos, UINT yPos)
{
	POINT pt = { (long)xPos, (long)yPos };
	if (pt.x == -1 && pt.y == -1)
	{
		pt.x = pt.y = 0;
		ClientToScreen(hwnd, &pt);
	}

	IContextMenu *pcm;
	if (SUCCEEDED(GetUIObjectOfFile(hwnd, pFileName, &IID_IContextMenu, (void **)&pcm)))
	{
		HMENU hmenu = CreatePopupMenu();
		if (hmenu)
		{
			if (SUCCEEDED(pcm->lpVtbl->QueryContextMenu(pcm, hmenu, 0, SCRATCH_QCM_FIRST, SCRATCH_QCM_LAST, CMF_NORMAL)))
			{
				int iCmd = TrackPopupMenuEx(hmenu, TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
				if (iCmd > 0)
				{
					CMINVOKECOMMANDINFOEX info = { 0 };
					info.cbSize = sizeof(info);
					info.fMask = CMIC_MASK_UNICODE;
					info.hwnd = hwnd;
					auto i = iCmd - SCRATCH_QCM_FIRST;
					info.lpVerb = MAKEINTRESOURCEA(i);
					info.lpVerbW = MAKEINTRESOURCEW(i);
					info.nShow = SW_SHOWNORMAL;
					HRESULT hr = pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&info);
				}
			}
			DestroyMenu(hmenu);
		}
		pcm->lpVtbl->Release(pcm);
	}
}
