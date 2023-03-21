/********************************************************************

    wfgoto.cpp

    This file contains code that supports the goto directory command

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT License.

********************************************************************/

#include "BagOValues.h"
#include <iterator>

extern "C"
{
#include "winfile.h"
#include "treectl.h"
#include "lfn.h"
#include "resize.h"
}

void BuildDirectoryBagOValues(BagOValues<PDNODE> *pbov, LPTSTR szRoot, PDNODE pNode, DWORD scanEpoc, LPTSTR szCachedRootLower);
void FreeDirectoryBagOValues(BagOValues<PDNODE> *pbov);

DWORD g_driveScanEpoc;				// incremented when a refresh is requested; old bags are discarded; scans are aborted if epoc changes
BagOValues<PDNODE> *g_pBagOCDrive;	// holds the values from the scan per g_driveScanEpoc
vector<PDNODE> *g_allNodes;			// holds the nodes we created to make freeing them simpler (e.g., because some are reused)

// compare path starting at the root; returns:
// 0: paths are the same length and same names
// -2: first difference in the paths has a sort lower
// -1: path a is a prefix of path b
// +1: path b is a prefix of path a
// +2: first difference in the paths has b sort lower
int ParentOrdering(const PDNODE& a, const PDNODE& b)
{
	int wCmp;
	if (a->nLevels == b->nLevels)
	{
		// when they are the same elements, definitely the same name; no need to recurse up
		if (a == b)
			return 0;

		if (a->nLevels != 0)
		{
			wCmp = ParentOrdering(a->pParent, b->pParent);

			// if parents are different, that is the result (-2 or 2)
			if (wCmp != 0)
				return wCmp;
		}

		wCmp = lstrcmpi(a->szName, b->szName);
		if (wCmp < 0)
			wCmp = -2;
		else if (wCmp > 0)
			wCmp = 2;

		return wCmp;
	}

	// if not same level, find the parent which makes the levels the same
	PDNODE pa = a;
	PDNODE pb = b;
	if (a->nLevels < b->nLevels)
	{
		while (pa->nLevels != pb->nLevels)
		{
			pb = pb->pParent;
		}
	}
	else
	{
		while (pa->nLevels != pb->nLevels)
		{
			pa = pa->pParent;
		}
	}

	wCmp = ParentOrdering(pa, pb);
	if (wCmp == 0)
	{
		// parents for matching levels are equal; return -1 or 1 based on which is longer (a or b)
		return (a->nLevels < b->nLevels) ? -1 : 1;
	}
	else
	{
		// return match based on subset of parents (-2 or 2)
		return wCmp;
	}
}

// returns true if a strictly less than b
bool CompareNodes(const PDNODE& a, const PDNODE& b)
{
	return ParentOrdering(a, b) < 0;
}

vector<PDNODE> FilterBySubtree(vector<PDNODE> const& parents, vector<PDNODE>  const& children)
{
	vector<PDNODE> results;

	// for each child, if parent in parents, return
	std::copy_if(std::cbegin(children),
				 std::cend(children),
				 std::back_inserter(results),
				 [&parents](auto const& child)
	{
		PDNODE parent = child->pParent;
		return (find(std::cbegin(parents), std::cend(parents), parent) != std::end(parents));
	});

	return results;
}

vector<PDNODE> TreeIntersection(vector<vector<PDNODE>>& trees)
{
	vector<PDNODE> result;

	if (trees.empty())
		return result;

	// If any tree is empty, return empty
	if (std::any_of(std::cbegin(trees), std::cend(trees), [](auto& tree) { return tree.size() == 0; }))
		return result;

	size_t maxOutput = 0;
	for (auto& tree : trees)
	{
		sort(tree.begin(), tree.end(), CompareNodes);
		if (tree.size() > maxOutput)
			maxOutput = tree.size();
	}

	// if just one, return it (after sort above)
	size_t count = trees.size();
	if (count == 1)
		return trees.at(0);

	// use up to two outputs and switch back and forth; lastOutput is last number output 
	vector<PDNODE> outputA(maxOutput);
	vector<PDNODE> outputB(maxOutput);
	vector<PDNODE> *combined = nullptr;
	size_t lastOutput = 0;

	// first is left side of merge; changes each time through the loop
	vector<PDNODE>* first = nullptr;

	// for all other result sets, merge
	for (size_t i = 1; i < count; i++)
	{
		size_t out = 0;			// always start writing to the beginning of the output

		size_t first1 = 0;		// scan index for last result in combined result (thus far)
		size_t last1;			// count of items in 'first'; set below

		if (i == 1)
		{
			// on first time through loop, output is A and 'first' is trees[0];
			first = &trees[0];
			last1 = first->size();
			combined = &outputA;
		}
		else if (i % 2 == 0)
		{
			// even passes: output is B and 'first' is outputA; create output B if it doesn't exist yet
			first = &outputA;
			last1 = lastOutput;

			combined = &outputB;
		}
		else
		{
			// odd passes except first: output is A and 'first' is B; both outputs already exist
			first = &outputB;
			last1 = lastOutput;
			combined = &outputA;
		}

		auto second = &trees[i];	// second results
		size_t first2 = 0;		// scan index for second results
		size_t last2 = second->size();	// end of second results

		// while results in both sets
		while (first1 < last1 && first2 < last2)
		{
			PDNODE& p1 = first->at(first1);
			PDNODE& p2 = second->at(first2);

			int wCmp = ParentOrdering(p1, p2);
			switch (wCmp)
			{
			case -2:
				// p1 is first in any case; skip first
				first1++;
				break;

			case -1:
				// p1 is prefix of p2; take p2; skip past p2
				combined->at(out) = p2;
				out++;
				first2++;
				break;

			case 0: // p1 == p2; take p1; skip both
				combined->at(out) = p1;
				out++;
				first1++;
				first2++;
				break;

			case 1: // p2 is prefix of p1; take p1; skip past p1
				combined->at(out) = p1;
				out++;
				first1++;
				break;

			case 2:
				// p2 is first in any case; skip second
				first2++;
				break;
			}
		}

		// shrink logical output to actual items written in each round
		lastOutput = out;
	}

	// shrink actual vector to final size
	combined->resize(lastOutput);

	return (*combined);
}

PDNODE CreateNode(PDNODE pParentNode, WCHAR *szName, DWORD dwAttribs)
{
	PDNODE pNode;
	DWORD len = lstrlen(szName);

	pNode = (PDNODE)LocalAlloc(LPTR, sizeof(DNODE) + ByteCountOf(len));
	if (!pNode)
	{
		return nullptr;
	}

	pNode->pParent = pParentNode;
	pNode->nLevels = pParentNode ? (pParentNode->nLevels + (BYTE)1) : (BYTE)0;
	pNode->wFlags = (BYTE)NULL;
	pNode->dwNetType = (DWORD)-1;
	pNode->dwAttribs = dwAttribs;
	pNode->dwExtent = (DWORD)-1;

	lstrcpy(pNode->szName, szName);

	if (pParentNode)
		pParentNode->wFlags |= TF_HASCHILDREN;      // mark the parent

	return pNode;
}

// for some reason this causes an error in xlocnum
#undef abs

#include <sstream>

vector<wstring> SplitIntoWords(LPCTSTR szText)
{
	vector<wstring> words;

	wchar_t tempStr[MAXPATHLEN];
	wcscpy_s(tempStr, szText);
	PWCHAR token{ nullptr };
	PWCHAR word = wcstok_s(tempStr, szPunctuation, &token);
	while (word)
	{
		words.push_back(word);
		word = wcstok_s(nullptr, szPunctuation, &token);
	}

	return words;
}

void FreeDirectoryBagOValues(BagOValues<PDNODE> *pbov, vector<PDNODE> *pNodes)
{
	// free all PDNODE in BagOValues
	for (PDNODE p : *pNodes)
	{
		LocalFree(p);
	}

	// free that vector and the BagOValues itself
	delete pNodes;
	delete pbov;
}

BOOL BuildDirectoryBagOValues(BagOValues<PDNODE> *pbov, vector<PDNODE> *pNodes, LPCTSTR szRoot, PDNODE pNodeParent, DWORD scanEpoc, LPTSTR szCachedRootLower)
{
	LFNDTA lfndta;
	WCHAR szPath[MAXPATHLEN];
	LPWSTR szEndPath;

	lstrcpy(szPath, szRoot);
	if (lstrlen(szPath) + 1 >= COUNTOF(szPath))
	{
		// path too long
		return TRUE;
	}

	AddBackslash(szPath);
	szEndPath = szPath + lstrlen(szPath);

	if (pNodeParent == nullptr)
	{
		// create first one; assume directory; "name" is full path starting with <drive>:
		// normally name is just directory name by itself
		pNodeParent = CreateNode(nullptr, szPath, FILE_ATTRIBUTE_DIRECTORY);
		if (pNodeParent == nullptr)
		{
			// out of memory
			return TRUE;
		}

		pNodes->push_back(pNodeParent);
		pbov->Add(szPath, pNodeParent);
	}

	if (lstrlen(szPath) + lstrlen(szStarDotStar) >= COUNTOF(szPath))
	{
		// path too long
		return TRUE;
	}

	// add *.* to end of path
	lstrcat(szPath, szStarDotStar);

	BOOL bFound = WFFindFirst(&lfndta, szPath, ATTR_DIR);

	while (bFound)
	{
		if (g_driveScanEpoc != scanEpoc)
		{
			// new scan started; abort this one
			WFFindClose(&lfndta);
			return FALSE;
		}

		// for all directories at this level, insert into BagOValues
        // do not insert the directories '.' or '..'; or insert empty directory names (cf. issue #194)

		if ((lfndta.fd.dwFileAttributes & ATTR_DIR) == 0 || ISDOTDIR(lfndta.fd.cFileName) || lfndta.fd.cFileName[0] == CHAR_NULL)
		{
			bFound = WFFindNext(&lfndta);
			continue;
		}

		PDNODE pNodeChild = CreateNode(pNodeParent, lfndta.fd.cFileName, lfndta.fd.dwFileAttributes);
		if (pNodeChild == nullptr)
		{
			// out of memory
			break;
		}
		pNodes->push_back(pNodeChild);

		// if spaces, each word individually (and not whole thing)
		vector<wstring> words = SplitIntoWords(lfndta.fd.cFileName);

		for (auto word : words)
		{
			// TODO: how to mark which word is primary to avoid double free?
			pbov->Add(word, pNodeChild);
		}

		//
		// Construct the path to this new subdirectory.
		//
		*szEndPath = CHAR_NULL;
		if (lstrlen(szPath) + 1 + lstrlen(lfndta.fd.cFileName) >= COUNTOF(szPath))
		{
			// path too long
			return TRUE;
		}

		AddBackslash(szPath);
		lstrcat(szPath, lfndta.fd.cFileName);         // cFileName is ANSI now

		// do not follow inner reparse points
		BOOL bFollow = FALSE;
		if (lfndta.fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			// Check which reparsepoint
			wchar_t szTemp[MAXPATHLEN];
			DWORD tag = DecodeReparsePoint(szPath, szTemp, MAXPATHLEN);
			switch (tag) 
			{
			// We always want to follow OneDrive
			case IO_REPARSE_TAG_CLOUD:
			case IO_REPARSE_TAG_CLOUD_1:
			case IO_REPARSE_TAG_CLOUD_2:
			case IO_REPARSE_TAG_CLOUD_3:
			case IO_REPARSE_TAG_CLOUD_4:
			case IO_REPARSE_TAG_CLOUD_5:
			case IO_REPARSE_TAG_CLOUD_6:
			case IO_REPARSE_TAG_CLOUD_7:
			case IO_REPARSE_TAG_CLOUD_8:
			case IO_REPARSE_TAG_CLOUD_9:
			case IO_REPARSE_TAG_CLOUD_A:
			case IO_REPARSE_TAG_CLOUD_B:
			case IO_REPARSE_TAG_CLOUD_C:
			case IO_REPARSE_TAG_CLOUD_D:
			case IO_REPARSE_TAG_CLOUD_E:
			case IO_REPARSE_TAG_CLOUD_F:
				bFollow = TRUE;
				break;

			case IO_REPARSE_TAG_SYMLINK:
			case IO_REPARSE_TAG_MOUNT_POINT:
				// Check if it is an inner reparsepoint, because we don't want to follow 
				// inner peparse points, because they would cause double enumeration

				// If the anchor is not part of the path then it is an outer reparse point
				// e.g. outer reparsepoint: szCachedRootLower == c:\bla, szTemp == d:\foo
				// e.g. inner reparsepoint: szCachedRootLower == c:\bla, szTemp == c:\bla\foo
				_wcslwr_s(szTemp, MAXPATHLEN);
				if (!wcsstr(szTemp, szCachedRootLower))
					bFollow = TRUE;
				break;

			default:
				bFollow = FALSE;
				break;
			}
		}
		else {
			bFollow = TRUE;
		}

		if (bFollow)
		{
			// add directories in subdir
			if (!BuildDirectoryBagOValues(pbov, pNodes, szPath, pNodeChild, scanEpoc, szCachedRootLower))
			{
				WFFindClose(&lfndta);
				return FALSE;
			}
		}

		bFound = WFFindNext(&lfndta);
	}

	WFFindClose(&lfndta);

	return TRUE;
}

vector<PDNODE> GetDirectoryOptionsFromText(LPCTSTR szText, BOOL *pbLimited)
{
	if (g_pBagOCDrive == nullptr)
		return vector<PDNODE>{};

	vector<wstring> words = SplitIntoWords(szText);

	vector<vector<PDNODE>> options_per_word;

	for (auto word : words)
	{
		vector<PDNODE> options;
		size_t pos = word.find_first_of(L'\\');
		if (pos == word.size() - 1)
		{
			// '\' at end; remove
			word = word.substr(0, pos);
			pos = string::npos;
		}
		bool fPrefix = true;
		if (word[0] == L'\'')
		{
			fPrefix = false;
			word = word.substr(1);
		}
		if (pos == string::npos)
		{
			options = g_pBagOCDrive->Retrieve(word, fPrefix, 1000);

			if (options.size() == 1000)
				*pbLimited = TRUE;
		}
		else
		{
			// "foo\bar" -> find candidates foo* which have subdir bar*
			wstring first = word.substr(0, pos);
			wstring second = word.substr(pos + 1);

			vector<PDNODE> options1 = std::move(g_pBagOCDrive->Retrieve(first, fPrefix, 1000));
			vector<PDNODE> options2 = std::move(g_pBagOCDrive->Retrieve(second, fPrefix, 1000));

			if (options1.size() == 1000 ||
				options2.size() == 1000)
				*pbLimited = TRUE;

			options = std::move(FilterBySubtree(options1, options2));
		}

		options_per_word.emplace_back(std::move(options));
	}

	vector<PDNODE> final_options = std::move(TreeIntersection(options_per_word));

	return final_options;
}

VOID UpdateGotoList(HWND hDlg)
{
	BOOL bLimited = FALSE;
	TCHAR szText[MAXPATHLEN];

	DWORD dw = GetDlgItemText(hDlg, IDD_GOTODIR, szText, COUNTOF(szText));

	vector<PDNODE> options = GetDirectoryOptionsFromText(szText, &bLimited);

	HWND hwndLB = GetDlgItem(hDlg, IDD_GOTOLIST);

	// Since we're probably about to add a lot of items, suppress redraw
	// and do it once at the end
	SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0);
	SendMessage(hwndLB, LB_RESETCONTENT, 0, 0);

	if (!options.empty())
	{
		const size_t resultCount = min((size_t)1000, options.size());

		// Try to allocate enough space in the list to avoid repeated
		// allocations.  This is an optimization and doesn't need to be
		// perfect, although being slightly too large is probably better
		// than slightly too small.  Since building the paths for this
		// seems needlessly expensive, assume each entry will be MAX_PATH.
		SendMessage(hwndLB, LB_INITSTORAGE, resultCount, resultCount * MAX_PATH);
		for (size_t i = 0; i < resultCount; ++i)
		{
			GetTreePath(options.at(i), szText);

			SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szText);
		}

		if (bLimited)
		{
			SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)TEXT("... limited ..."));
		}
		else if (options.size() > resultCount)
		{
			SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)TEXT("... more ..."));
		}

		SendMessage(hwndLB, LB_SETCURSEL, 0, 0);
	}

	// Perform the redraw
	SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(hwndLB, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GotoDirDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WNDPROC wpOrigEditProc;

// Subclass procedure: use arrow keys to change selection in listbox below.
LRESULT APIENTRY GotoEditSubclassProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_GETDLGCODE:
		if (lParam) {
			LPMSG lpmsg = (LPMSG)lParam;

			if ((lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_KEYUP) && 
				(lpmsg->wParam == VK_DOWN ||
				 lpmsg->wParam == VK_UP ||
				 lpmsg->wParam == VK_HOME ||
				 lpmsg->wParam == VK_END ||
				 lpmsg->wParam == VK_PRIOR ||
				 lpmsg->wParam == VK_NEXT)) {

				HWND hwndDlg = GetParent(hwnd);
				SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, lpmsg->message, lpmsg->wParam, lpmsg->lParam);
				return DLGC_WANTALLKEYS;
			}
		}
		break;
	}
	return CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
}

VOID
SetCurrentPathOfWindow(LPWSTR szPath)
{
	HWND hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

	HWND hwndNew = CreateDirWindow(szPath, TRUE, hwndActive);

	HWND hwndTree = HasTreeWindow(hwndNew);
	if (hwndTree)
	{
		SetFocus(hwndTree);
	}
}

INT_PTR
CALLBACK
GotoDirDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndEdit;
	DWORD command_id;

	if (ResizeDialogProc(hDlg, wMsg, wParam, lParam)) {
		return TRUE;
	}

	switch (wMsg)
	{
	case WM_INITDIALOG:
		// Retrieve the handle to the edit control. 
		hwndEdit = GetDlgItem(hDlg, IDD_GOTODIR);

		// Subclass the edit control. 
		wpOrigEditProc = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)GotoEditSubclassProc);

		SendDlgItemMessage(hDlg, IDD_GOTOLIST, LB_ADDSTRING, 0, (LPARAM)TEXT("<type name fragments into edit box>"));
		break;

	case WM_COMMAND:
		command_id = GET_WM_COMMAND_ID(wParam, lParam);
		switch (command_id)
		{
		case IDD_GOTODIR:
			switch (HIWORD(wParam))
			{
			case EN_UPDATE:
				// repopulate listbox with candidate directories; select first one
				UpdateGotoList(hDlg);
				break;
			}
			break;

		case IDD_HELP:
			goto DoHelp;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		case IDOK:
		{
			TCHAR szPath[MAXPATHLEN];
			DWORD iSel;

			EndDialog(hDlg, TRUE);

			iSel = (DWORD)SendDlgItemMessage(hDlg, IDD_GOTOLIST, LB_GETCURSEL, 0, 0);
			if (iSel == LB_ERR)
			{
				if (GetDlgItemText(hDlg, IDD_GOTODIR, szPath, COUNTOF(szPath)) != 0)
				{
					if (PathIsDirectory(szPath))
						iSel = 0;
				}
			}
			else
			{
				if (SendDlgItemMessage(hDlg, IDD_GOTOLIST, LB_GETTEXT, iSel, (LPARAM)szPath) == LB_ERR ||
					!PathIsDirectory(szPath))
					iSel = LB_ERR;
			}

			if (iSel != LB_ERR)
			{
				SetCurrentPathOfWindow(szPath);
			}
			break;
		}

		default:
			return(FALSE);
		}
		break;

	case WM_DESTROY:
		hwndEdit = GetDlgItem(hDlg, IDD_GOTODIR);

		// Remove the subclass from the edit control. 
		SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)wpOrigEditProc);
		break;

	default:
		if (wMsg == wHelpMessage) {
		DoHelp:
			WFHelp(hDlg);

			return TRUE;
		}
		else
			return FALSE;
	}
	return TRUE;
}

DWORD WINAPI
BuildDirectoryTreeBagOValues(PVOID pv)
{
	DWORD scanEpocNew = InterlockedIncrement(&g_driveScanEpoc);

	BagOValues<PDNODE> *pBagNew = new BagOValues<PDNODE>();
	vector<PDNODE> *pNodes = new vector<PDNODE>();

	SendMessage(hwndStatus, SB_SETTEXT, 2, (LPARAM)TEXT("BUILDING GOTO CACHE"));

	// Read pathes, which shall be cached from winfile.ini
	GetPrivateProfileString(szSettings, szGotoCachePunctuation, TEXT("- _."), szPunctuation, MAXPATHLEN, szTheINIFile);

	// Read pathes, which shall be cached from winfile.ini
	GetPrivateProfileString(szSettings, szCachedPath, TEXT("c:\\"), szCachedPathIni, MAXPATHLEN, szTheINIFile);

	// Create a local copy, because once we save it to winfile.ini on exit we need the original value
	TCHAR szCached[MAXPATHLEN];
	lstrcpy(szCached, szCachedPathIni);

	// Iterate through ; seperated list of to be cached pathes
	BOOL     buildBag{ FALSE };
	WCHAR 	seps[]{ L";" };
	PWCHAR   token{ nullptr };
	PWCHAR   szCachedRoot = wcstok_s(szCached, seps, &token);
	TCHAR    szCachedRootLower[MAXPATHLEN];

	while (szCachedRoot)
	{
		lstrcpy(szCachedRootLower, szCachedRoot);
		_wcslwr_s(szCachedRootLower, MAXPATHLEN - (szCachedRoot - szCached));
		buildBag |= BuildDirectoryBagOValues(pBagNew, pNodes, szCachedRoot, nullptr, scanEpocNew, szCachedRootLower);

		szCachedRoot = wcstok_s(NULL, seps, &token);
	}

	// If at least one cache location has been read successfully, build the bag
	if (buildBag)
	{
		pBagNew->Sort();

		pBagNew = (BagOValues<PDNODE> *)InterlockedExchangePointer((PVOID *)&g_pBagOCDrive, pBagNew);
		pNodes = (vector<PDNODE> *)InterlockedExchangePointer((PVOID *)&g_allNodes, pNodes);
	}

	if (pBagNew != nullptr)
	{
		FreeDirectoryBagOValues(pBagNew, pNodes);
	}

	UpdateMoveStatus(ReadMoveStatus());

	return ERROR_SUCCESS;
}

// We're building a Trie structure (not just a directory tree)
DWORD
StartBuildingDirectoryTrie()
{
	HANDLE hThreadCopy;
	DWORD dwIgnore;

	//
	// Move/Copy things.
	//
	hThreadCopy = CreateThread(nullptr,
		0L,
		BuildDirectoryTreeBagOValues,
		nullptr,
		0L,
		&dwIgnore);

	if (!hThreadCopy) {
		return GetLastError();
	}

	SetThreadPriority(hThreadCopy, THREAD_PRIORITY_BELOW_NORMAL);

	CloseHandle(hThreadCopy);

	return 0;
}
