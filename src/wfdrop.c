/********************************************************************

   wfdrop.c

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#define INITGUID 

#define COBJMACROS

#include "winfile.h"
#include "wfdrop.h"
#include "treectl.h"

#include <ole2.h>
#include <shlobj.h>

#ifndef GUID_DEFINED
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IDropSource,             0x00000121, 0, 0);
DEFINE_OLEGUID(IID_IDropTarget,             0x00000122, 0, 0);
#endif

HRESULT CreateDropTarget(HWND hwnd, WF_IDropTarget **ppDropTarget);
void DropData(WF_IDropTarget *This, IDataObject *pDataObject, DWORD dwEffect);

void PaintRectItem(WF_IDropTarget *This, POINTL *ppt)
{
	HWND hwndLB;
	DWORD iItem;
	POINT pt;
	BOOL fTree;
	
	// could be either tree control or directory list box
	hwndLB = GetDlgItem(This->m_hWnd, IDCW_LISTBOX);
	fTree = FALSE;
	if (hwndLB == NULL)
	{
		hwndLB = GetDlgItem(This->m_hWnd, IDCW_TREELISTBOX);
		fTree = TRUE;

		if (hwndLB == NULL)
			return;
	}

	if (ppt != NULL)
	{
		pt.x = ppt->x;
		pt.y = ppt->y;
		ScreenToClient(hwndLB, &pt);
	
		iItem = (DWORD)SendMessage(hwndLB, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
		iItem &= 0xffff;
		if (This->m_iItemSelected != -1 && This->m_iItemSelected == iItem)
			return;
	}

	// unpaint old item
	if (This->m_iItemSelected != -1)
	{
	    if (fTree)
			RectTreeItem(hwndLB, This->m_iItemSelected, FALSE);
		else
			DSRectItem(hwndLB, This->m_iItemSelected, FALSE, FALSE);

		This->m_iItemSelected = (DWORD)-1;
	}

	// if new item, paint it.
	if (ppt != NULL)
	{
	    if (fTree)
		{
			if (RectTreeItem(hwndLB, iItem, TRUE))
				This->m_iItemSelected = iItem;
		}
		else
		{
			if (DSRectItem(hwndLB, iItem, TRUE, FALSE))
				This->m_iItemSelected = iItem;
		}
	}
}


LPWSTR QuotedDropList(IDataObject *pDataObject)
{
	HDROP hdrop;
	DWORD cFiles, iFile, cchFiles;
	LPWSTR szFiles = NULL, pch;
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed;

	if (pDataObject->lpVtbl->GetData(pDataObject, &fmtetc, &stgmed) == S_OK)
	{
		// Yippie! the data is there, so go get it!
		hdrop = stgmed.hGlobal;

		cFiles = DragQueryFile(hdrop, 0xffffffff, NULL, 0);
		cchFiles = 0;
		for (iFile = 0; iFile < cFiles; iFile++)
			cchFiles += DragQueryFile(hdrop, iFile, NULL, 0) + 1 + 2;

		pch = szFiles = (LPWSTR)LocalAlloc(LMEM_FIXED, cchFiles * sizeof(WCHAR));
		for (iFile = 0; iFile < cFiles; iFile++)
		{
			DWORD cchFile;

			*pch++ = CHAR_DQUOTE;
			
			cchFile = DragQueryFile(hdrop, iFile, pch, cchFiles);
			pch += cchFile;
			*pch++ = CHAR_DQUOTE;

			if (iFile+1 < cFiles)
				*pch++ = CHAR_SPACE;
			else
				*pch = CHAR_NULL;
				
			cchFiles -= cchFile + 1 + 2;
		}

		// release the data using the COM API
		ReleaseStgMedium(&stgmed);
	}

	return szFiles;
}


HDROP CreateDropFiles(POINT pt, BOOL fNC, LPTSTR pszFiles)
{
    HANDLE hDrop;
    LPBYTE lpList;
    SIZE_T cbList;
	LPTSTR szSrc;

    LPDROPFILES lpdfs;
    TCHAR szFile[MAXPATHLEN];

	cbList = sizeof(DROPFILES) + sizeof(TCHAR);

	szSrc = pszFiles;
    while (szSrc = GetNextFile(szSrc, szFile, COUNTOF(szFile))) 
	{
        QualifyPath(szFile);

		cbList += (wcslen(szFile) + 1)*sizeof(TCHAR);
	}

    hDrop = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE|GMEM_ZEROINIT, cbList);
    if (!hDrop)
        return NULL;

    lpdfs = (LPDROPFILES)GlobalLock(hDrop);

    lpdfs->pFiles = sizeof(DROPFILES);
	lpdfs->pt = pt;
	lpdfs->fNC = fNC;
    lpdfs->fWide = TRUE;

	lpList = (LPBYTE)lpdfs + sizeof(DROPFILES);
	szSrc = pszFiles;

    while (szSrc = GetNextFile(szSrc, szFile, COUNTOF(szFile))) {

       QualifyPath(szFile);

       lstrcpy((LPTSTR)lpList, szFile);

       lpList += (wcslen(szFile) + 1)*sizeof(TCHAR);
    }

	GlobalUnlock(hDrop);

	return hDrop;
}

#define BLOCK_SIZE 512

static HRESULT StreamToFile(IStream *stream, TCHAR *szFile)
{
    byte buffer[BLOCK_SIZE];
    DWORD bytes_read;
    DWORD bytes_written;
    HRESULT hr;
	HANDLE hFile;

    hFile = CreateFile( szFile,
          FILE_READ_DATA | FILE_WRITE_DATA,
          FILE_SHARE_READ | FILE_SHARE_WRITE,
          NULL,
          CREATE_ALWAYS,
          FILE_ATTRIBUTE_TEMPORARY,
          NULL );

    if (hFile != INVALID_HANDLE_VALUE) {
        do {
            hr = stream->lpVtbl->Read(stream, buffer, BLOCK_SIZE, &bytes_read);
			bytes_written = 0;
            if (SUCCEEDED(hr) && bytes_read)
			{
				if (!WriteFile(hFile, buffer, bytes_read, &bytes_written, NULL))
				{
					hr = HRESULT_FROM_WIN32(GetLastError());
					bytes_written = 0;
				}
			}
        } while (S_OK == hr && bytes_written != 0);
        CloseHandle(hFile);
		if (FAILED(hr))
			DeleteFile(szFile);
		else
			hr = S_OK;
    }
    else
	    hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}


LPWSTR QuotedContentList(IDataObject *pDataObject)
{
    FILEGROUPDESCRIPTOR *file_group_descriptor;
    FILEDESCRIPTOR file_descriptor;
	HRESULT hr;
	LPWSTR szFiles = NULL;

    unsigned short cp_format_descriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    unsigned short cp_format_contents = RegisterClipboardFormat(CFSTR_FILECONTENTS);

    //Set up format structure for the descriptor and contents
    FORMATETC descriptor_format = {cp_format_descriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    FORMATETC contents_format = {cp_format_contents, NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM};

    // Check for descriptor format type
    hr = pDataObject->lpVtbl->QueryGetData(pDataObject, &descriptor_format);
    if (hr == S_OK) 
	{ 
		// Check for contents format type
        hr = pDataObject->lpVtbl->QueryGetData(pDataObject, &contents_format);
        if (hr == S_OK)
		{ 
            // Get the descriptor information
            STGMEDIUM sm_desc= {0,0,0};
            STGMEDIUM sm_content = {0,0,0};
			unsigned int file_index;
            size_t cchTempPath, cchFiles;
            WCHAR szTempPath[MAXPATHLEN +1];

            hr = pDataObject->lpVtbl->GetData(pDataObject, &descriptor_format, &sm_desc);
			if (hr != S_OK)
				return NULL;

            file_group_descriptor = (FILEGROUPDESCRIPTOR *) GlobalLock(sm_desc.hGlobal);

			GetTempPath(MAXPATHLEN, szTempPath);
			cchTempPath = wcslen(szTempPath);

			// calc total size of file names
			cchFiles = 0;
            for (file_index = 0; file_index < file_group_descriptor->cItems; file_index++) 
			{
                file_descriptor = file_group_descriptor->fgd[file_index];
				cchFiles += 1 + cchTempPath + 1 + wcslen(file_descriptor.cFileName) + 2;
			}

			szFiles = (LPWSTR)LocalAlloc(LMEM_FIXED, cchFiles * sizeof(WCHAR));
			szFiles[0] = '\0';

            // For each file, get the name and copy the stream to a file
            for (file_index = 0; file_index < file_group_descriptor->cItems; file_index++)
			{
                file_descriptor = file_group_descriptor->fgd[file_index];
                contents_format.lindex = file_index;
				memset(&sm_content, 0, sizeof(sm_content));
                hr = pDataObject->lpVtbl->GetData(pDataObject, &contents_format, &sm_content);

                if (hr == S_OK) 
				{
					// Dump stream to a file
					TCHAR szTempFile[MAXPATHLEN*2+1];

					lstrcpy(szTempFile, szTempPath);
		            AddBackslash(szTempFile);
				    lstrcat(szTempFile, file_descriptor.cFileName);

					// TODO: make sure all directories between the temp directory and the file have been created
					// paste from zip archives result in file_descriptor.cFileName with intermediate directories

					hr = StreamToFile(sm_content.pstm, szTempFile);

					if (hr == S_OK)
					{
						CheckEsc(szTempFile);

						if (szFiles[0] != '\0')
							lstrcat(szFiles, TEXT(" "));
						lstrcat(szFiles, szTempFile);
					}

					ReleaseStgMedium(&sm_content);
                }
            }

            GlobalUnlock(sm_desc.hGlobal);
            ReleaseStgMedium(&sm_desc);

			if (szFiles[0] == '\0')
			{
				// nothing to copy
				MessageBeep(0);
				LocalFree((HLOCAL)szFiles);	
				szFiles = NULL;
			}
        }
	}
    return szFiles;
}

//
//	QueryDataObject private helper routine
//
static BOOL QueryDataObject(WF_IDataObject *pDataObject)
{
	FORMATETC fmtetc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    unsigned short cp_format_descriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
    FORMATETC descriptor_format = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	descriptor_format.cfFormat = cp_format_descriptor;

	// does the data object support CF_HDROP using a HGLOBAL?
	return pDataObject->ido.lpVtbl->QueryGetData((LPDATAOBJECT)pDataObject, &fmtetc) == S_OK || 
			pDataObject->ido.lpVtbl->QueryGetData((LPDATAOBJECT)pDataObject, &descriptor_format) == S_OK;
}

//
//	DropEffect private helper routine
//
static DWORD DropEffect(DWORD grfKeyState, POINTL pt, DWORD dwAllowed)
{
	DWORD dwEffect = 0;

	// 1. check "pt" -> do we allow a drop at the specified coordinates?
	
	// 2. work out that the drop-effect should be based on grfKeyState
	if(grfKeyState & MK_CONTROL)
	{
		dwEffect = dwAllowed & DROPEFFECT_COPY;
	}
	else if(grfKeyState & MK_SHIFT)
	{
		dwEffect = dwAllowed & DROPEFFECT_MOVE;
	}
	
	// 3. no key-modifiers were specified (or drop effect not allowed), so
	//    base the effect on those allowed by the dropsource
	if(dwEffect == 0)
	{
		if(dwAllowed & DROPEFFECT_COPY) dwEffect = DROPEFFECT_COPY;
		if(dwAllowed & DROPEFFECT_MOVE) dwEffect = DROPEFFECT_MOVE;
	}
	
	return dwEffect;
}

static void CopySTGMEDIUM(STGMEDIUM *pdest, const STGMEDIUM *psrc, const FORMATETC *pfmt)
{
	pdest->tymed = psrc->tymed;
	switch (psrc->tymed) {
	case TYMED_HGLOBAL:
		pdest->hGlobal = (HGLOBAL)OleDuplicateData(psrc->hGlobal, pfmt->cfFormat, 0);
		break;
	case TYMED_GDI:
		pdest->hBitmap = (HBITMAP)OleDuplicateData(psrc->hBitmap, pfmt->cfFormat, 0);
		break;
	case TYMED_MFPICT:
		pdest->hMetaFilePict = (HMETAFILEPICT)OleDuplicateData(psrc->hMetaFilePict, pfmt->cfFormat, 0);
		break;
	case TYMED_ENHMF:
		pdest->hEnhMetaFile = (HENHMETAFILE)OleDuplicateData(psrc->hEnhMetaFile, pfmt->cfFormat, 0);
		break;
	case TYMED_FILE:
		pdest->lpszFileName = (LPOLESTR)OleDuplicateData(psrc->lpszFileName, pfmt->cfFormat, 0);
		break;
	case TYMED_ISTREAM:
		pdest->pstm = psrc->pstm;
		IStream_AddRef(psrc->pstm);
		break;
	case TYMED_ISTORAGE:
		pdest->pstg = psrc->pstg;
		IStorage_AddRef(psrc->pstg);
		break;
	case TYMED_NULL:
	default:
		break;
	}

	pdest->pUnkForRelease = psrc->pUnkForRelease;
	if (psrc->pUnkForRelease != NULL)
		IUnknown_AddRef(psrc->pUnkForRelease);
}


//
//	IUnknown::AddRef
//
static ULONG STDMETHODCALLTYPE idroptarget_addref (WF_IDropTarget* This)
{
  return InterlockedIncrement(&This->m_lRefCount);
}

//
//	IUnknown::QueryInterface
//
static HRESULT STDMETHODCALLTYPE
idroptarget_queryinterface(WF_IDropTarget *This, REFIID riid, LPVOID* ppvObject)
{
	*ppvObject = NULL;

	//  PRINT_GUID (riid);
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDropTarget))
	{
		idroptarget_addref(This);
		*ppvObject = This;
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}


//
//	IUnknown::Release
//
static ULONG STDMETHODCALLTYPE idroptarget_release(WF_IDropTarget* This)
{
	LONG count = InterlockedDecrement(&This->m_lRefCount);

	if (count == 0)
		free(This);
	return count;
}

//
//	IDropTarget::DragEnter
//
//
//
static HRESULT STDMETHODCALLTYPE idroptarget_dragenter(WF_IDropTarget* This, WF_IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	// does the dataobject contain data we want?
	This->m_fAllowDrop = QueryDataObject(pDataObject);
	
	if(This->m_fAllowDrop)
	{
		// get the dropeffect based on keyboard state
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);

		SetFocus(This->m_hWnd);

		PaintRectItem(This, &pt);
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

//
//	IDropTarget::DragOver
//
//
//
static HRESULT STDMETHODCALLTYPE idroptarget_dragover(WF_IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	if(This->m_fAllowDrop)
	{
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);
		PaintRectItem(This, &pt);
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

//
//	IDropTarget::DragLeave
//
static HRESULT STDMETHODCALLTYPE idroptarget_dragleave(WF_IDropTarget* This)
{
	PaintRectItem(This, NULL);
	return S_OK;
}

//
//	IDropTarget::Drop
//
//
static HRESULT STDMETHODCALLTYPE idroptarget_drop(WF_IDropTarget* This, IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	if(This->m_fAllowDrop)
	{
		*pdwEffect = DropEffect(grfKeyState, pt, *pdwEffect);

		DropData(This, pDataObject, *pdwEffect);
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

	return S_OK;
}

static WF_IDropTargetVtbl idt_vtbl = {
  idroptarget_queryinterface,
  idroptarget_addref,
  idroptarget_release,
  idroptarget_dragenter,
  idroptarget_dragover,
  idroptarget_dragleave,
  idroptarget_drop
};

void DropData(WF_IDropTarget *This, IDataObject *pDataObject, DWORD dwEffect)
{
	// construct a FORMATETC object
	HWND hwndLB;
	BOOL fTree;
	LPWSTR szFiles = NULL;
	WCHAR     szDest[MAXPATHLEN];

	hwndLB = GetDlgItem(This->m_hWnd, IDCW_LISTBOX);
	fTree = FALSE;
	if (hwndLB == NULL)
	{
		hwndLB = GetDlgItem(This->m_hWnd, IDCW_TREELISTBOX);
		fTree = TRUE;

		if (hwndLB == NULL)
			return;
	}

	// if item selected, add path
	if (fTree)
	{
	    PDNODE pNode;
		
		// odd
		if (This->m_iItemSelected == -1)
			return;

		if (SendMessage(hwndLB, LB_GETTEXT, This->m_iItemSelected, (LPARAM)&pNode) == LB_ERR)
			return;

		GetTreePath(pNode, szDest);
	}
	else
	{
		LPXDTA    lpxdta;

		SendMessage(This->m_hWnd, FS_GETDIRECTORY, COUNTOF(szDest), (LPARAM)szDest);

		if (This->m_iItemSelected != -1)
		{
			SendMessage(hwndLB, LB_GETTEXT, This->m_iItemSelected,
				(LPARAM)(LPTSTR)&lpxdta);
		
			AddBackslash(szDest);
			lstrcat(szDest, MemGetFileName(lpxdta));
		}
	}

    AddBackslash(szDest);
    lstrcat(szDest, szStarDotStar);   // put files in this dir

    CheckEsc(szDest);

	// See if the dataobject contains any TEXT stored as a HGLOBAL
	if ((szFiles = QuotedDropList(pDataObject)) == NULL)
	{
		szFiles = QuotedContentList(pDataObject);
		dwEffect = DROPEFFECT_MOVE; 
	}

	if (szFiles != NULL)
	{
		SetFocus(This->m_hWnd);

		DMMoveCopyHelper(szFiles, szDest, dwEffect == DROPEFFECT_COPY);

		LocalFree((HLOCAL)szFiles);
	}
}

void RegisterDropWindow(HWND hwnd, WF_IDropTarget **ppDropTarget)
{
	WF_IDropTarget *pDropTarget;
	
	CreateDropTarget(hwnd, &pDropTarget);

	// acquire a strong lock
	CoLockObjectExternal((struct IUnknown*)pDropTarget, TRUE, FALSE);

	// tell OLE that the window is a drop target
	RegisterDragDrop(hwnd, (LPDROPTARGET)pDropTarget);

	*ppDropTarget = pDropTarget;
}

void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget)
{
	// remove drag+drop
	RevokeDragDrop(hwnd);

	// remove the strong lock
	CoLockObjectExternal((struct IUnknown*)pDropTarget, FALSE, TRUE);

	// release our own reference
	pDropTarget->lpVtbl->Release(pDropTarget);
}

//	Constructor for the CDropTarget class
//

WF_IDropTarget * WF_IDropTarget_new(HWND hwnd)
{
  WF_IDropTarget *result;

  result = calloc(1, sizeof(WF_IDropTarget));

  if (result)
  {
	  result->idt.lpVtbl = (IDropTargetVtbl*)&idt_vtbl;

	  result->m_lRefCount = 1;
	  result->m_hWnd = hwnd;
	  result->m_fAllowDrop = FALSE;

	  // return result;
  }

  return result;
}

HRESULT CreateDropTarget(HWND hwnd, WF_IDropTarget **ppDropTarget) 
{
	if(ppDropTarget == 0)
		return E_INVALIDARG;

	*ppDropTarget = WF_IDropTarget_new(hwnd);

	return (*ppDropTarget) ? S_OK : E_OUTOFMEMORY;

}

//
// IEnumFORMATETC
//
static HRESULT STDMETHODCALLTYPE ienumformatetc_queryinterface(WF_IEnumFORMATETC* This, REFIID riid, LPVOID *ppvObject)
{
	*ppvObject = NULL;
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumFORMATETC)) {
		*ppvObject = This;
		ienumformatetc_addref(This);
		return S_OK;
	}
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE ienumformatetc_addref(WF_IEnumFORMATETC* This)
{
	return InterlockedIncrement(&This->m_lRefCount);
}

static ULONG STDMETHODCALLTYPE ienumformatetc_release(WF_IEnumFORMATETC* This)
{
	ULONG refCount = InterlockedDecrement(&This->m_lRefCount);
	if (refCount == 0)
		WF_IEnumFORMATETC_delete(This);
	return refCount;
}

static HRESULT STDMETHODCALLTYPE ienumformatetc_next(WF_IEnumFORMATETC* This, ULONG celt, LPFORMATETC pfmt, ULONG *pceltFetched)
{
	ULONG num = celt;

	if (pceltFetched != NULL)
		*pceltFetched = 0;

	if (celt <= 0 || pfmt == NULL || This->m_nIndex >= This->m_nNumFormats)
		return S_FALSE;

	if (pceltFetched == NULL && celt != 1)
		return S_FALSE;

	while (This->m_nIndex < This->m_nNumFormats && num > 0) {
		*pfmt++ = This->m_pFormatEtc[This->m_nIndex++];
		--num;
	}
	if (pceltFetched != NULL)
		*pceltFetched = celt - num;

	return (num == 0) ? S_OK : S_FALSE;
}

static HRESULT STDMETHODCALLTYPE ienumformatetc_skip(WF_IEnumFORMATETC* This, ULONG celt)
{
	if ((This->m_nIndex + celt) >= This->m_nNumFormats)
		return S_FALSE;
	This->m_nIndex += celt;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE ienumformatetc_reset(WF_IEnumFORMATETC* This)
{
	This->m_nIndex = 0;
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE ienumformatetc_clone(WF_IEnumFORMATETC* This, WF_IEnumFORMATETC **ppenum)
{
	WF_IEnumFORMATETC* pNewFormat;

	if (ppenum == NULL)
		return E_POINTER;

	pNewFormat = WF_IEnumFORMATETC_copy(This);
	if (pNewFormat == NULL)
		return E_OUTOFMEMORY;
	ienumformatetc_addref(pNewFormat);
	*ppenum = pNewFormat;
	return S_OK;
}

static WF_IEnumFORMATETCVtbl ief_vtbl = {
  ienumformatetc_queryinterface,
  ienumformatetc_addref,
  ienumformatetc_release,
  ienumformatetc_next,
  ienumformatetc_skip,
  ienumformatetc_reset,
  ienumformatetc_clone
};

static WF_IEnumFORMATETC* WF_IEnumFORMATETC_new(FORMATETC **array, ULONG size)
{
	WF_IEnumFORMATETC* ptr;
	ULONG i;

	ptr = calloc(1, sizeof(WF_IEnumFORMATETC));
	if (ptr != NULL) {
		ptr->ief.lpVtbl = (IEnumFORMATETCVtbl*)&ief_vtbl;
		ptr->m_lRefCount = 1;
		ptr->m_pFormatEtc = calloc(size, sizeof(FORMATETC));
		for (i = 0; i < size; i++)
			ptr->m_pFormatEtc[i] = *array[i];
		ptr->m_nNumFormats = size;
		ptr->m_nIndex = 0;
	}
	return ptr;
}

static WF_IEnumFORMATETC* WF_IEnumFORMATETC_copy(WF_IEnumFORMATETC* src)
{
	WF_IEnumFORMATETC* ptr;

	ptr = calloc(1, sizeof(WF_IEnumFORMATETC));
	if (ptr != NULL) {
		ptr->ief.lpVtbl = (IEnumFORMATETCVtbl*)&ief_vtbl;
		ptr->m_lRefCount = 0;
		ptr->m_pFormatEtc = calloc(src->m_nNumFormats, sizeof(FORMATETC));
		CopyMemory(ptr->m_pFormatEtc, src->m_pFormatEtc, sizeof(FORMATETC) * src->m_nNumFormats);
		ptr->m_nNumFormats = src->m_nNumFormats;
		ptr->m_nIndex = src->m_nIndex;
	}
	return ptr;
}

static void WF_IEnumFORMATETC_delete(WF_IEnumFORMATETC* ptr)
{
	free(ptr->m_pFormatEtc);
	free(ptr);
}


//
// IDataObject
//
static HRESULT STDMETHODCALLTYPE idataobject_queryinterface(WF_IDataObject *This, REFIID riid,	LPVOID *ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDataObject))	{
		*ppvObject = This;
		idataobject_addref(This);
		return S_OK;
	} 
	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE idataobject_addref(WF_IDataObject* This)
{
	return InterlockedIncrement(&This->m_lRefCount);
}

static ULONG STDMETHODCALLTYPE idataobject_release(WF_IDataObject* This)
{
	LONG refCount = InterlockedDecrement(&This->m_lRefCount);

	if (refCount == 0)
		WF_IDataObject_delete(This);

	return refCount;
}

static HRESULT STDMETHODCALLTYPE idataobject_getdata(
	WF_IDataObject* This,
	FORMATETC* pformatetcIn,
	STGMEDIUM* pmedium)
{
	FORMATETC *pfmt;
	ULONG i;

	if (pformatetcIn == NULL || pmedium == NULL)
		return E_INVALIDARG;

	pmedium->hGlobal = NULL;
	for (i = 0; i < This->m_size; i++) {
		pfmt = This->m_pFormatEtc[i];
		if (pformatetcIn->tymed & pfmt->tymed
			&& pformatetcIn->dwAspect == pfmt->tymed
			&& pformatetcIn->cfFormat == pfmt->cfFormat) {
				CopySTGMEDIUM(pmedium, This->m_pStgMedium[i], pfmt);
				return S_OK;
		}
	}
	return DV_E_FORMATETC;
}


static HRESULT STDMETHODCALLTYPE idataobject_getdatahere(
	WF_IDataObject* This,
	FORMATETC* pformatetc,
	STGMEDIUM* pmedium)
{
	return E_NOTIMPL;
}


static HRESULT STDMETHODCALLTYPE idataobject_querygetdata(
	WF_IDataObject* This,
	FORMATETC* pformatetc)
{
	HRESULT result = DV_E_TYMED;
	ULONG i;

	if (pformatetc == NULL)
		return E_INVALIDARG;

	if (!(DVASPECT_CONTENT & pformatetc->dwAspect))
		return DV_E_DVASPECT;

	for (i = 0; i < This->m_size; i++) {
		if (pformatetc->tymed & This->m_pFormatEtc[i]->tymed) {
			if (pformatetc->cfFormat == This->m_pFormatEtc[i]->cfFormat)
				return S_OK;
			result = DV_E_CLIPFORMAT;
		} else {
			result = DV_E_TYMED;
		}
	}
	return result;
}

static HRESULT STDMETHODCALLTYPE idataobject_getcanonicalformatetc(
	WF_IDataObject* This,
	FORMATETC* pformatetcIn,
	FORMATETC* pformatetcOut)
{
	if (pformatetcOut == NULL)
		return E_INVALIDARG;

	return DATA_S_SAMEFORMATETC;
}


static HRESULT STDMETHODCALLTYPE idataobject_setdata(
	WF_IDataObject* This,
	FORMATETC* pformatetc,
	STGMEDIUM* pmedium,
	BOOL fRelease)
{
	FORMATETC *pfmt;
	STGMEDIUM *pstg;

	if (pformatetc == NULL || pmedium == NULL)
		return E_INVALIDARG;

	pfmt = calloc(1, sizeof(FORMATETC));
	pstg = calloc(1, sizeof(STGMEDIUM));
	if (pfmt == NULL || pstg == NULL) {
		free(pfmt);
		free(pstg);
		return E_OUTOFMEMORY;
	}

	*pfmt = *pformatetc;
	if (fRelease) {
		*pstg = *pmedium;
	}
	else {
		CopySTGMEDIUM(pstg, pmedium, pformatetc);
	}

	if (This->m_size == 0) {
		This->m_pFormatEtc = calloc(1, sizeof(FORMATETC *));
		This->m_pStgMedium = calloc(1, sizeof(STGMEDIUM *));
		This->m_pFormatEtc[0] = pfmt;
		This->m_pStgMedium[0] = pstg;
		This->m_size = 1;
	} else {
		FORMATETC **oldfmts = This->m_pFormatEtc;
		STGMEDIUM **oldstgs = This->m_pStgMedium;
		ULONG oldsize = This->m_size;

		This->m_pFormatEtc = calloc(oldsize + 1, sizeof(FORMATETC *));
		This->m_pStgMedium = calloc(oldsize + 1, sizeof(STGMEDIUM *));
		CopyMemory(This->m_pFormatEtc, oldfmts, sizeof(FORMATETC *) * oldsize);
		CopyMemory(This->m_pStgMedium, oldstgs, sizeof(STGMEDIUM *) * oldsize);
		This->m_pFormatEtc[oldsize] = pfmt;
		This->m_pStgMedium[oldsize] = pstg;
		This->m_size = oldsize + 1;
		free(oldfmts);
		free(oldstgs);
	}
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE idataobject_enumformatetc(
	WF_IDataObject* This,
	DWORD dwDirection,
	IEnumFORMATETC** ppenumFormatEtc)
{
	if (ppenumFormatEtc == NULL)
		return E_POINTER;

	*ppenumFormatEtc = NULL;
	switch (dwDirection) {
	case DATADIR_GET:
		*ppenumFormatEtc = (IEnumFORMATETC *)WF_IEnumFORMATETC_new(This->m_pFormatEtc, This->m_size);
		if (*ppenumFormatEtc == NULL)
			return E_OUTOFMEMORY;
		IEnumFORMATETC_AddRef(*ppenumFormatEtc);
		break;
	case DATADIR_SET:
		/* no implementation */
	default:
		return E_NOTIMPL;
	}

	return S_OK;
}

static HRESULT STDMETHODCALLTYPE idataobject_dadvise(
	WF_IDataObject* This,
	FORMATETC* pformatetc,
	DWORD advf,
	IAdviseSink* pAdvSink,
	DWORD* pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE idataobject_dunadvise(
	WF_IDataObject* This,
	DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE idataobject_enumdadvise(
	WF_IDataObject* This,
	IEnumSTATDATA** ppenumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}




static WF_IDataObjectVtbl ido_vtbl = {
  idataobject_queryinterface,
  idataobject_addref,
  idataobject_release,
  idataobject_getdata,
  idataobject_getdatahere,
  idataobject_querygetdata,
  idataobject_getcanonicalformatetc,
  idataobject_setdata,
  idataobject_enumformatetc,
  idataobject_dadvise,
  idataobject_dunadvise,
  idataobject_enumdadvise
};


WF_IDataObject * WF_IDataObject_new()
{
   WF_IDataObject *result;

   result = calloc(1, sizeof(WF_IDataObject));

   if (result)
   {
      result->ido.lpVtbl = (IDataObjectVtbl*)&ido_vtbl;

      result->m_lRefCount = 1;

	  result->m_pFormatEtc = NULL;
	  result->m_pStgMedium = NULL;
	  result->m_size = 0;
   }

   return result;
}

static void WF_IDataObject_delete(WF_IDataObject *ptr)
{
	ULONG i;
	for (i = 0; i < ptr->m_size; i++) {
      free(ptr->m_pFormatEtc[i]);
      ReleaseStgMedium(ptr->m_pStgMedium[i]);
      free(ptr->m_pStgMedium[i]);
   }
   free(ptr->m_pFormatEtc);
   free(ptr->m_pStgMedium);
   free(ptr);
}

IDataObject *CreateIDataObject()
{
	return (IDataObject *)WF_IDataObject_new();
}


//
// Drop Source
//
static HRESULT STDMETHODCALLTYPE idropsource_queryinterface(
	WF_IDropSource *This,
	REFIID          riid,
	LPVOID         *ppvObject)
{
	*ppvObject = NULL;

	//  PRINT_GUID (riid);
	if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDropSource))
	{
		idropsource_addref(This);
		*ppvObject = This;
		return S_OK;
	}

	return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE idropsource_addref(WF_IDropSource* This)
{
	return InterlockedIncrement(&This->m_lRefCount);
}

static ULONG STDMETHODCALLTYPE idropsource_release(
	WF_IDropSource* This)
{
	LONG refCount = InterlockedDecrement(&This->m_lRefCount);

	if (refCount == 0)
		free(This);
	return refCount;
}

static HRESULT STDMETHODCALLTYPE idropsource_querycontinuedrag(WF_IDropSource* This, BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;

	if (!(grfKeyState & (MK_LBUTTON | MK_RBUTTON)))
		return DRAGDROP_S_DROP;
	
	return S_OK;
}

static HRESULT STDMETHODCALLTYPE idropsource_givefeedback(WF_IDropSource* This, DWORD pdwEffect)
{

	return DRAGDROP_S_USEDEFAULTCURSORS;
}

static WF_IDropSourceVtbl ids_vtbl = {
  idropsource_queryinterface,
  idropsource_addref,
  idropsource_release,
  idropsource_querycontinuedrag,
  idropsource_givefeedback
};

WF_IDropSource * WF_IDropSource_new()
{
	WF_IDropSource *result;

	result = calloc(1, sizeof(WF_IDropSource));

	if (result)
	{
		result->ids.lpVtbl = (IDropSourceVtbl*)&ids_vtbl;

		result->m_lRefCount = 1;
	}

	return result;
}

HRESULT RegisterDropSource(WF_IDropSource **ppDropSource)
{
   WF_IDropSource* pDropSource;
   WF_IDataObject* pDataObject;

   pDropSource = WF_IDropSource_new();
   pDataObject = WF_IDataObject_new();


   wchar_t *files = L"C:\\tmp\\reparse\\001.jpg\0\0";
   //wchar_t *files = "C:\\tmp\\reparse\\001.jpg\0C:\\tmp\\reparse\\002.jpg\0\0";
 //  wchar_t *files = L"C:\\tmp\\AVL_impf.png\0\0";

   FORMATETC fmtetc;
   STGMEDIUM medium;
   wchar_t* p;
   DWORD effect;
   HRESULT hr;

   fmtetc.cfFormat = CF_HDROP;
   fmtetc.ptd = NULL;
   fmtetc.dwAspect = DVASPECT_CONTENT;
   fmtetc.lindex = -1;
   fmtetc.tymed = TYMED_HGLOBAL;

   size_t len = (wcslen(files) + 3) * sizeof(wchar_t);
   medium.tymed = TYMED_HGLOBAL;
   medium.hGlobal = GlobalAlloc(
      GMEM_MOVEABLE,
      sizeof(DROPFILES) + len
   );
   medium.pUnkForRelease = NULL;
   p = GlobalLock(medium.hGlobal);
   ((DROPFILES *)p)->pFiles = sizeof(DROPFILES);
   ((DROPFILES *)p)->fWide = TRUE;
   CopyMemory(p + sizeof(DROPFILES), files, len);
   GlobalUnlock(medium.hGlobal);

   // Set the data
   idataobject_setdata(pDataObject, &fmtetc, &medium, FALSE);

   hr = DoDragDrop(
      (LPDATAOBJECT)pDataObject,
      (LPDROPSOURCE)pDropSource,
      DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK,
      &effect
   );

   idataobject_release(pDataObject);
   idropsource_release(pDropSource);

   *ppDropSource = pDropSource;

   return hr;
}


