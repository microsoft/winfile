/********************************************************************

   wfwow.c

   Windows File Manager Wow64 wrapper routines
   
   Since the file manager needs to display files as they exist on
   disk, Win32 functions querying or manipulating files need to
   suppress Wow64 redirection.  Any Win32 API that consumes a file
   path should be wrapped within this file.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "lfn.h"

#define KERNEL32_DLL TEXT("kernel32.dll")
BOOL (CALLBACK *lpfnWow64DisableWow64FsRedirection)(PVOID *);
BOOL (CALLBACK *lpfnWow64RevertWow64FsRedirection)(PVOID);

#define KERNEL32_Wow64DisableWow64FsRedirection "Wow64DisableWow64FsRedirection"
#define KERNEL32_Wow64RevertWow64FsRedirection  "Wow64RevertWow64FsRedirection"

#define Wow64DisableWow64FsRedirection   (*lpfnWow64DisableWow64FsRedirection)
#define Wow64RevertWow64FsRedirection    (*lpfnWow64RevertWow64FsRedirection)


VOID
WfWowInitialize()
{
   HANDLE hKernel32;
   hKernel32 = GetModuleHandle(KERNEL32_DLL);

   lpfnWow64DisableWow64FsRedirection = (PVOID)GetProcAddress(hKernel32, KERNEL32_Wow64DisableWow64FsRedirection);
   lpfnWow64RevertWow64FsRedirection = (PVOID)GetProcAddress(hKernel32, KERNEL32_Wow64RevertWow64FsRedirection);
}

typedef struct _WF_WOW_STATE {
    PVOID OldValue;
    BOOLEAN RevertRequired;
} WF_WOW_STATE, *PWF_WOW_STATE;

VOID
WfWowDisableRedirection(PWF_WOW_STATE State)
{
    State->RevertRequired = FALSE;
    if (lpfnWow64DisableWow64FsRedirection != NULL) {
        if (Wow64DisableWow64FsRedirection(&State->OldValue)) {
            State->RevertRequired = TRUE;
        }
    }
}

VOID
WfWowRevertRedirection(PWF_WOW_STATE State)
{
    if (State->RevertRequired &&
        lpfnWow64RevertWow64FsRedirection != NULL) {

        Wow64RevertWow64FsRedirection(State->OldValue);
    }
}

BOOL
WfWowCopyFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowCopyFileEx(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    LPBOOL pbCancel,
    DWORD dwCopyFlags
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = CopyFileEx(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowCreateDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = CreateDirectory(lpPathName, lpSecurityAttributes);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowCreateDirectoryEx(
    LPCWSTR lpTemplateDirectory,
    LPCWSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = CreateDirectoryEx(lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);
    WfWowRevertRedirection(&WowState);
    return Result;
}

HANDLE
WfWowCreateFile(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    WF_WOW_STATE WowState;
    HANDLE Result;

    WfWowDisableRedirection(&WowState);
    Result = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOLEAN
WfWowCreateSymbolicLink (
    LPCWSTR lpSymlinkFileName,
    LPCWSTR lpTargetFileName,
    DWORD dwFlags
    )
{
    WF_WOW_STATE WowState;
    BOOLEAN Result;

    WfWowDisableRedirection(&WowState);
    Result = CreateSymbolicLink(lpSymlinkFileName, lpTargetFileName, dwFlags);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowDeleteFile(
    LPCWSTR lpFileName
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = DeleteFile(lpFileName);
    WfWowRevertRedirection(&WowState);
    return Result;
}

HANDLE
WfWowFindFirstFile(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    WF_WOW_STATE WowState;
    HANDLE Result;

    WfWowDisableRedirection(&WowState);
    Result = FindFirstFile(lpFileName, lpFindFileData);
    WfWowRevertRedirection(&WowState);
    return Result;
}

HANDLE
WfWowFindFirstFileEx(
    LPCWSTR lpFileName,
    FINDEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFindFileData,
    FINDEX_SEARCH_OPS fSearchOp,
    LPVOID lpSearchFilter,
    DWORD dwAdditionalFlags
    )
{
    WF_WOW_STATE WowState;
    HANDLE Result;

    WfWowDisableRedirection(&WowState);
    Result = FindFirstFileEx(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowFindNextFile(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = FindNextFile(hFindFile, lpFindFileData);
    WfWowRevertRedirection(&WowState);
    return Result;
}

DWORD
WfWowGetCompressedFileSize(
    LPCWSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{
    WF_WOW_STATE WowState;
    DWORD Result;

    WfWowDisableRedirection(&WowState);
    Result = GetCompressedFileSize(lpFileName, lpFileSizeHigh);
    WfWowRevertRedirection(&WowState);
    return Result;
}

DWORD
WfWowGetFileAttributes(
    LPCWSTR lpFileName
    )
{
    WF_WOW_STATE WowState;
    DWORD Result;

    WfWowDisableRedirection(&WowState);
    Result = GetFileAttributes(lpFileName);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowMoveFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = MoveFile(lpExistingFileName, lpNewFileName);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowRemoveDirectory(
    LPCWSTR lpPathName
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = RemoveDirectory(lpPathName);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = SetFileAttributes(lpFileName, dwFileAttributes);
    WfWowRevertRedirection(&WowState);
    return Result;
}

HINSTANCE
WfWowShellExecute(
    HWND hwnd,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    INT nShowCmd
    )
{
    WF_WOW_STATE WowState;
    HINSTANCE Result;

    WfWowDisableRedirection(&WowState);
    Result = ShellExecute(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
    WfWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WfWowShellExecuteEx(
    SHELLEXECUTEINFOW *pExecInfo
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WfWowDisableRedirection(&WowState);
    Result = ShellExecuteEx(pExecInfo);
    WfWowRevertRedirection(&WowState);
    return Result;
}


