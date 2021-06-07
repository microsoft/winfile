#include <ShlObj.h>

//
// How to host an IContextMenu, part 1 - Initial foray
// https://devblogs.microsoft.com/oldnewthing/20040920-00/?p=37823
// 
// How to host an IContextMenu, part 2 - Displaying the context menu
// https://devblogs.microsoft.com/oldnewthing/20040922-00/?p=37793
// 
// How to host an IContextMenu, part 3 - Invocation location
// https://devblogs.microsoft.com/oldnewthing/20040923-00/?p=37773
//
// How to host an IContextMenu, part 4 - Key context
// https://devblogs.microsoft.com/oldnewthing/20040924-00/?p=37753
// 
// How to host an IContextMenu, part 5 - Handling menu messages
// https://devblogs.microsoft.com/oldnewthing/20040927-00/?p=37733
//

IContextMenu2 *pExplorerCm2;
IContextMenu3 *pExplorerCm3;

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
				pcm->lpVtbl->QueryInterface(pcm, &IID_IContextMenu2, (void **)&pExplorerCm2);
				pcm->lpVtbl->QueryInterface(pcm, &IID_IContextMenu3, (void **)&pExplorerCm3);
				int iCmd = TrackPopupMenuEx(hmenu, TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
				if (pExplorerCm2)
				{
					pExplorerCm2->lpVtbl->Release(pExplorerCm2);
					pExplorerCm2 = NULL;
				}
				if (pExplorerCm3)
				{
					pExplorerCm3->lpVtbl->Release(pExplorerCm3);
					pExplorerCm3 = NULL;
				}
				if (iCmd > 0)
				{
					CMINVOKECOMMANDINFOEX info = { 0 };
					info.cbSize = sizeof(info);
					info.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
					if (GetKeyState(VK_CONTROL) < 0)
					{
						info.fMask |= CMIC_MASK_CONTROL_DOWN;
					}
					if (GetKeyState(VK_SHIFT) < 0)
					{
						info.fMask |= CMIC_MASK_SHIFT_DOWN;
					}
					info.hwnd = hwnd;
					auto i = iCmd - SCRATCH_QCM_FIRST;
					info.lpVerb = MAKEINTRESOURCEA(i);
					info.lpVerbW = MAKEINTRESOURCEW(i);
					info.nShow = SW_SHOWNORMAL;
					info.ptInvoke.x = xPos;
					info.ptInvoke.y = yPos;
					HRESULT hr = pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&info);
				}
			}
			DestroyMenu(hmenu);
		}
		pcm->lpVtbl->Release(pcm);
	}
}
