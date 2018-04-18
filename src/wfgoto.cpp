/********************************************************************

    wfgoto.cpp

    This file contains code that supports the goto directory command

    Copyright (c) Microsoft Corporation. All rights reserved.
    Licensed under the MIT License.

********************************************************************/

#include "BagOValues.h"
#include <iterator>
#include <vector>
#include <sstream> // wstringstream
#include <memory>

extern "C"
{
#include "winfile.h"
#include "treectl.h"
#include "lfn.h"
}

// for some reason this causes an error in xlocnum
#undef abs

	using namespace std;

	using wstring_vector = vector<wstring>;
	using pdnode_vector = vector<PDNODE>;
	using pdnode_bag = winfile::BagOValues<PDNODE>;
	/*
	the aim is to use value semantics, not pointers
	-----------------------------------------------
	using pdnode_bag_ptr = std::shared_ptr<pdnode_bag>;
	using pdnode_vector_ptr = std::shared_ptr<pdnode_vector>;
	*/
	static BOOL BuildDirectoryBagOValues(
		pdnode_bag pbov,
		pdnode_vector pNodes,
		LPCTSTR szRoot,
		PDNODE pNodeParent,
		DWORD scanEpoc
	);

	static void FreeDirectoryBagOValues(pdnode_bag pbov, pdnode_vector pNodes);

	// incremented when a refresh is requested; old bags are discarded; 
	// scans are aborted if epoc changes
	// static DWORD g_driveScanEpoc{};
	winfile::internal::guardian<DWORD> atomic_epoc;

	// holds the nodes we created to make freeing them simpler 
	// (e.g., because some are reused)
	winfile::internal::guardian<pdnode_bag> atomic_bag;
	// std::atomic<pdnode_bag> atomic_bag ;
	// holds the nodes we created to make freeing them simpler 
	// (e.g., because some are reused)
	winfile::internal::guardian<pdnode_vector> atomic_nodes;

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

	pdnode_vector FilterBySubtree(
		pdnode_vector const& parents,
		pdnode_vector  const& children
	)
	{
		pdnode_vector results;

		// for each child, if parent in parents, return
		copy_if(cbegin(children),
			cend(children),
			back_inserter(results),
			[&parents](auto const& child)
		{
			PDNODE parent = child->pParent;
			return (find(cbegin(parents), cend(parents), parent) != end(parents));
		});

		return results;
	}

	pdnode_vector TreeIntersection(vector<pdnode_vector>& trees)
	{
		pdnode_vector result;

		if (trees.empty())
			return result;

		// If any tree is empty, return empty
		if (any_of(cbegin(trees), cend(trees), [](auto& tree) { return tree.size() == 0; }))
			return result;

		size_t maxOutput = 0;
		for (auto& tree : trees)
		{
			sort(tree.begin(), tree.end(), CompareNodes);
			if (tree.size() > maxOutput)
				maxOutput = tree.size();
		}

		// if just one, return it (after sort above)
		int count = trees.size();
		if (count == 1)
			return trees.at(0);

		// use up to two outputs and switch back and forth; lastOutput is last number output 
		pdnode_vector outputA(maxOutput);
		pdnode_vector outputB(maxOutput);
		pdnode_vector *combined = nullptr;
		size_t lastOutput = 0;

		// first is left side of merge; changes each time through the loop
		pdnode_vector* first = nullptr;

		// for all other result sets, merge
		for (int i = 1; i < count; i++)
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


	wstring_vector SplitIntoWords(LPCTSTR szText)
	{
		wstring_vector words{};

		wstringstream ss{};
		ss.str(szText);
		wstring item;
		while (getline(ss, item, L' '))
		{
			if (item.size() != 0)
				words.push_back(item);
		}

		return words;
	}

// we have moved to value semantics, this this is not necessary
// TBD
	void FreeDirectoryBagOValues(pdnode_bag pbov, pdnode_vector pNodes)
	{
		// free all PDNODE in BagOValues
		/*
		for (PDNODE p : *pNodes)
		{
			LocalFree(p);
		}
		*/
	}


	inline auto
		add_backslash(wstring path_)
	{
		if (path_.back() != CHAR_BACKSLASH) {
			path_.append(wstring{ CHAR_BACKSLASH });
		}
			return path_.size();
	}

	BOOL BuildDirectoryBagOValues(
		pdnode_bag pbov,
		pdnode_vector pNodes,
		LPCTSTR szRoot,
		PDNODE pNodeParent,
		DWORD scanEpoc
	)
	{
		LFNDTA lfndta{};
		wstring szPath{ MAXPATHLEN }; //  WCHAR szPath[MAXPATHLEN];

		// lstrcpy(szPath, szRoot);
		szPath = szRoot;
		// is path too long?
		if (szPath.size() > MAXPATHLEN) return TRUE;

		add_backslash(szPath);

		if (pNodeParent == nullptr)
		{
			// create first one; assume directory; "name" is full path starting with <drive>:
			// normally name is just directory name by itself
			pNodeParent = CreateNode(nullptr, (WCHAR*)szPath.data(), FILE_ATTRIBUTE_DIRECTORY);
			if (pNodeParent == nullptr)
			{
				_ASSERTE(pNodeParent != nullptr); // DBJ 
				// out of memory
				return TRUE;
			}

			pNodes.push_back(pNodeParent);
			pbov.Add(szPath, pNodeParent);
		}

		// add *.* to end of path
		szPath.append(szStarDotStar);
		// is path too long?
		if (szPath.size() > MAXPATHLEN) return TRUE;


		BOOL bFound = WFFindFirst(&lfndta, (WCHAR *)szPath.data(), ATTR_DIR);

		while (bFound)
		{
			if (atomic_epoc.load() != scanEpoc)
			{
				// new scan started; abort this one
				WFFindClose(&lfndta);
				return FALSE;
			}

			// for all directories at this level, insert into BagOValues

			if ((lfndta.fd.dwFileAttributes & ATTR_DIR) == 0 || ISDOTDIR(lfndta.fd.cFileName))
			{
				bFound = WFFindNext(&lfndta);
				continue;
			}

			PDNODE pNodeChild = CreateNode(pNodeParent, lfndta.fd.cFileName, lfndta.fd.dwFileAttributes);
			if (pNodeChild == nullptr)
			{
				_ASSERTE(pNodeChild != nullptr);
				// out of memory
				break;
			}
			pNodes.push_back(pNodeChild);

			// if spaces, each word individually (and not the whole thing)
			wstring_vector words = SplitIntoWords(lfndta.fd.cFileName);

			for (auto word : words)
			{
				// TODO: how to mark which word is primary to avoid double free?
				pbov.Add(word, pNodeChild);
			}

			//
			// Construct the path to this new subdirectory.
			//
			add_backslash(szPath);
			// is path too long?
			if (szPath.size() > MAXPATHLEN) return TRUE;

			szPath.append( lfndta.fd.cFileName);
			// is path too long?
			if (szPath.size() > MAXPATHLEN) return TRUE;

			// add directories in subdir
			if (!BuildDirectoryBagOValues(pbov, pNodes, szPath.data(), pNodeChild, scanEpoc))
			{
				WFFindClose(&lfndta);
				return FALSE;
			}

			bFound = WFFindNext(&lfndta);
		}

		WFFindClose(&lfndta);

		return TRUE;
	}

	pdnode_vector GetDirectoryOptionsFromText(LPCTSTR szText, BOOL *pbLimited)
	{
		auto shared_bag = atomic_bag.load();

		if (shared_bag.Empty()) return pdnode_vector{};

		wstring_vector words = SplitIntoWords(szText);

		vector<pdnode_vector> options_per_word{};

		for (wstring word : words)
		{
			pdnode_vector options{};
			bool fPrefix = true;

			size_t pos = word.find_first_of(L'\\');

			if (pos == word.size() - 1)
			{
				// '\' at end; remove
				word = word.substr(0, pos);
				pos = string::npos;
			}

			if (word[0] == L'\'')
			{
				fPrefix = false;
				word = word.substr(1);
			}

			if (pos == string::npos)
			{
				options = shared_bag.Retrieve(word, fPrefix, 1000);

				if (options.size() < 1) return {}; // DBJ

				if (options.size() == 1000) // DBJ: what is the logic here?
					*pbLimited = TRUE;      // what is the '1000' magic constant?
			}
			else
			{
				// "foo\bar" -> find candidates foo* which have subdir bar*
				wstring first = word.substr(0, pos);
				wstring second = word.substr(pos + 1);

				pdnode_vector options1 = std::move(shared_bag.Retrieve(first, fPrefix, 1000));
				pdnode_vector options2 = std::move(shared_bag.Retrieve(second, fPrefix, 1000));

				if (options1.size() == 1000 ||
					options2.size() == 1000)
					*pbLimited = TRUE;

				options = std::move(FilterBySubtree(options1, options2));
			}

			options_per_word.emplace_back(std::move(options));
		}

		pdnode_vector final_options = std::move(TreeIntersection(options_per_word));

		return final_options;
	}

	VOID UpdateGotoList(HWND hDlg)
	{
		BOOL bLimited = FALSE;
		TCHAR szText[MAXPATHLEN];

		DWORD dw = GetDlgItemText(hDlg, IDD_GOTODIR, szText, COUNTOF(szText));

		pdnode_vector options = GetDirectoryOptionsFromText(szText, &bLimited);

		HWND hwndLB = GetDlgItem(hDlg, IDD_GOTOLIST);
		SendMessage(hwndLB, LB_RESETCONTENT, 0, 0);

		if (options.empty())
			return;

		for (auto i = 0u; i < 10u && i < options.size(); i++)
		{
			GetTreePath(options.at(i), szText);

			SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szText);
		}

		if (bLimited)
		{
			SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)TEXT("... limited ..."));
		}
		else if (options.size() >= 10)
		{
			SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)TEXT("... more ..."));
		}

		SendMessage(hwndLB, LB_SETCURSEL, 0, 0);
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

				if (lpmsg->message == WM_KEYDOWN && (lpmsg->wParam == VK_DOWN || lpmsg->wParam == VK_UP || lpmsg->wParam == VK_HOME || lpmsg->wParam == VK_END)) {
					HWND hwndDlg = GetParent(hwnd);
					DWORD iSel = SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, LB_GETCURSEL, 0, 0);
					if (iSel == LB_ERR)
						iSel = 0;
					else if (lpmsg->wParam == VK_DOWN)
						iSel++;
					else if (lpmsg->wParam == VK_UP)
						iSel--;
					else if (lpmsg->wParam == VK_HOME)
						iSel = 0;
					else if (lpmsg->wParam == VK_END)
					{
						iSel = SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, LB_GETCOUNT, 0, 0) - 1;
					}
					if (SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, LB_SETCURSEL, iSel, 0) == LB_ERR) {
						if (lpmsg->wParam == VK_DOWN)
							SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, LB_SETCURSEL, 0, 0);
						else if (lpmsg->wParam == VK_UP)
							SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, LB_SETCURSEL, SendDlgItemMessage(hwndDlg, IDD_GOTOLIST, LB_GETCOUNT, 0, 0) - 1, 0);
					}
					return DLGC_WANTALLKEYS;
				}
			}
			break;
		}
		return CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
	}

	/*
	a 'worker' executed on a separate thread
	*/
	DWORD WINAPI BuildDirectoryTreeBagOValues(PVOID pv)
	{
		//   InterlockedIncrement(&atomic_epoc);
		DWORD scanEpocNew = atomic_epoc.store(
			1 + atomic_epoc.load()
		); 

		pdnode_bag		pBagNew{};
		pdnode_vector	pNodes{};

		SendMessage(hwndStatus, SB_SETTEXT, 2, (LPARAM)TEXT("BUILDING GOTO CACHE"));

		if (BuildDirectoryBagOValues(pBagNew, pNodes, TEXT("c:\\"), nullptr, scanEpocNew))
		{
			// papa's got the brand new bag, 
			// which is already sorted 
			// pBagNew->Sort();

			atomic_bag.store(pBagNew);
			atomic_nodes.store(pNodes);
		}
		/*
		not required, no more pointers
		if (! pBagNew.Empty())	{		FreeDirectoryBagOValues(pBagNew, pNodes);	}
		*/

		UpdateMoveStatus(ReadMoveStatus());

		return ERROR_SUCCESS;
	}



    /*
	----------------------------------------------------------------------------------------------
	declared in winfile.h
	*/
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

	INT_PTR
		GotoDirDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
	{
		HWND hwndEdit;
		DWORD command_id;

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

				EndDialog(hDlg, TRUE);

				DWORD iSel = SendDlgItemMessage(hDlg, IDD_GOTOLIST, LB_GETCURSEL, 0, 0);
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