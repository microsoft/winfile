/**************************************************************************

   wfdrop.h

   Include for WINFILE program

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

**************************************************************************/

#ifndef WFDROP_INC
#define WFDROP_INC
#include <ole2.h>

// data object:

typedef struct {
	IDataObject ido;
	int ref_count;
	FORMATETC *m_pFormatEtc;
	STGMEDIUM *m_pStgMedium;
	LONG	   m_nNumFormats;
	LONG m_lRefCount;
} WF_IDataObject;


typedef struct {
	IEnumFORMATETC ief;
	int ref_count;
	int ix;
	LONG		m_lRefCount;		// Reference count for this COM interface
	ULONG		m_nIndex;			// current enumerator index
	ULONG		m_nNumFormats;		// number of FORMATETC members
	FORMATETC * m_pFormatEtc;
} WF_IEnumFORMATETC;

typedef struct {
	IDropSource ids;
	LONG	   m_lRefCount;
}	WF_IDropSource;

typedef struct {
	IDropTarget idt;
	LONG	m_lRefCount;
	HWND	m_hWnd;
	BOOL  m_fAllowDrop;
	DWORD m_iItemSelected;
	IDataObject *m_pDataObject;
} WF_IDropTarget;

typedef struct WF_IDataObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            WF_IDataObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            WF_IDataObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            WF_IDataObject __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetData )( 
            WF_IDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
            /* [out] */ STGMEDIUM __RPC_FAR *pmedium);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDataHere )( 
            WF_IDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [out][in] */ STGMEDIUM __RPC_FAR *pmedium);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryGetData )( 
            WF_IDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCanonicalFormatEtc )( 
            WF_IDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
            /* [out] */ FORMATETC __RPC_FAR *pformatetcOut);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetData )( 
            WF_IDataObject __RPC_FAR * This,
            /* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
            /* [in] */ BOOL fRelease);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumFormatEtc )( 
            WF_IDataObject __RPC_FAR * This,
            /* [in] */ DWORD dwDirection,
            /* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DAdvise )( 
            WF_IDataObject __RPC_FAR * This,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ DWORD advf,
            /* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
            /* [out] */ DWORD __RPC_FAR *pdwConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DUnadvise )( 
            WF_IDataObject __RPC_FAR * This,
            /* [in] */ DWORD dwConnection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumDAdvise )( 
            WF_IDataObject __RPC_FAR * This,
            /* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise);
        
        END_INTERFACE
    } WF_IDataObjectVtbl;

typedef struct WF_IEnumFORMATETCVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            WF_IEnumFORMATETC __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            WF_IEnumFORMATETC __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            WF_IEnumFORMATETC __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            WF_IEnumFORMATETC __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ FORMATETC __RPC_FAR *rgelt,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            WF_IEnumFORMATETC __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            WF_IEnumFORMATETC __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            WF_IEnumFORMATETC __RPC_FAR * This,
            /* [out] */ WF_IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } WF_IEnumFORMATETCVtbl;


typedef struct WF_IDropSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            WF_IDropSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            WF_IDropSource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            WF_IDropSource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryContinueDrag )( 
            WF_IDropSource __RPC_FAR * This,
            /* [in] */ BOOL fEscapePressed,
            /* [in] */ DWORD grfKeyState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GiveFeedback )( 
            WF_IDropSource __RPC_FAR * This,
            /* [in] */ DWORD dwEffect);
        
        END_INTERFACE
    } WF_IDropSourceVtbl;

typedef struct WF_IDropTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            WF_IDropTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            WF_IDropTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            WF_IDropTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DragEnter )( 
            WF_IDropTarget __RPC_FAR * This,
            /* [unique][in] */ WF_IDataObject __RPC_FAR *pDataObj,
            /* [in] */ DWORD grfKeyState,
            /* [in] */ POINTL pt,
            /* [out][in] */ DWORD __RPC_FAR *pdwEffect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DragOver )( 
            WF_IDropTarget __RPC_FAR * This,
            /* [in] */ DWORD grfKeyState,
            /* [in] */ POINTL pt,
            /* [out][in] */ DWORD __RPC_FAR *pdwEffect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DragLeave )( 
            WF_IDropTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Drop )( 
            WF_IDropTarget __RPC_FAR * This,
            /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
            /* [in] */ DWORD grfKeyState,
            /* [in] */ POINTL pt,
            /* [out][in] */ DWORD __RPC_FAR *pdwEffect);
        
        END_INTERFACE
    } WF_IDropTargetVtbl;

typedef WF_IDataObject* LPWBDATAOBJECT;
typedef WF_IEnumFORMATETC* LPWBFORMATETC;

WF_IEnumFORMATETC *WF_IEnumFORMATETC_new (UINT, FORMATETC *);

void RegisterDropWindow(HWND hwnd, WF_IDropTarget **ppDropTarget);
void UnregisterDropWindow(HWND hwnd, IDropTarget *pDropTarget);
#endif


LPWSTR QuotedDropList(IDataObject *pDataObj);
LPWSTR QuotedContentList(IDataObject *pDataObj);
HDROP CreateDropFiles(POINT pt, BOOL fNC, LPTSTR pszFiles);