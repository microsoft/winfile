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
WFWowInitialize()
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
WFWowDisableRedirection(PWF_WOW_STATE State)
{
    State->RevertRequired = FALSE;
    if (lpfnWow64DisableWow64FsRedirection != NULL) {
        if (Wow64DisableWow64FsRedirection(&State->OldValue)) {
            State->RevertRequired = TRUE;
        }
    }
}

VOID
WFWowRevertRedirection(PWF_WOW_STATE State)
{
    if (State->RevertRequired &&
        lpfnWow64RevertWow64FsRedirection != NULL) {

        Wow64RevertWow64FsRedirection(State->OldValue);
    }
}

BOOL
WFWowCopyFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    BOOL bFailIfExists
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowCopyFileEx(
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

    WFWowDisableRedirection(&WowState);
    Result = CopyFileEx(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowCreateDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = CreateDirectory(lpPathName, lpSecurityAttributes);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowCreateDirectoryEx(
    LPCWSTR lpTemplateDirectory,
    LPCWSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = CreateDirectoryEx(lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);
    WFWowRevertRedirection(&WowState);
    return Result;
}

HANDLE
WFWowCreateFile(
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

    WFWowDisableRedirection(&WowState);
    Result = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOLEAN
WFWowCreateSymbolicLink (
    LPCWSTR lpSymlinkFileName,
    LPCWSTR lpTargetFileName,
    DWORD dwFlags
    )
{
    WF_WOW_STATE WowState;
    BOOLEAN Result;

    WFWowDisableRedirection(&WowState);
    Result = CreateSymbolicLink(lpSymlinkFileName, lpTargetFileName, dwFlags);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowDeleteFile(
    LPCWSTR lpFileName
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = DeleteFile(lpFileName);
    WFWowRevertRedirection(&WowState);
    return Result;
}

HANDLE
WFWowFindFirstFile(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    WF_WOW_STATE WowState;
    HANDLE Result;

    WFWowDisableRedirection(&WowState);
    Result = FindFirstFile(lpFileName, lpFindFileData);
    WFWowRevertRedirection(&WowState);
    return Result;
}

HANDLE
WFWowFindFirstFileEx(
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

    WFWowDisableRedirection(&WowState);
    Result = FindFirstFileEx(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowFindNextFile(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAW lpFindFileData
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = FindNextFile(hFindFile, lpFindFileData);
    WFWowRevertRedirection(&WowState);
    return Result;
}

DWORD
WFWowGetCompressedFileSize(
    LPCWSTR lpFileName,
    LPDWORD lpFileSizeHigh
    )
{
    WF_WOW_STATE WowState;
    DWORD Result;

    WFWowDisableRedirection(&WowState);
    Result = GetCompressedFileSize(lpFileName, lpFileSizeHigh);
    WFWowRevertRedirection(&WowState);
    return Result;
}

DWORD
WFWowGetFileAttributes(
    LPCWSTR lpFileName
    )
{
    WF_WOW_STATE WowState;
    DWORD Result;

    WFWowDisableRedirection(&WowState);
    Result = GetFileAttributes(lpFileName);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowMoveFile(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = MoveFile(lpExistingFileName, lpNewFileName);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowRemoveDirectory(
    LPCWSTR lpPathName
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = RemoveDirectory(lpPathName);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = SetFileAttributes(lpFileName, dwFileAttributes);
    WFWowRevertRedirection(&WowState);
    return Result;
}

HINSTANCE
WFWowShellExecute(
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

    WFWowDisableRedirection(&WowState);
    Result = ShellExecute(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
    WFWowRevertRedirection(&WowState);
    return Result;
}

BOOL
WFWowShellExecuteEx(
    SHELLEXECUTEINFOW *pExecInfo
    )
{
    WF_WOW_STATE WowState;
    BOOL Result;

    WFWowDisableRedirection(&WowState);
    Result = ShellExecuteEx(pExecInfo);
    WFWowRevertRedirection(&WowState);
    return Result;
}


