/**************************************************************************

   winfile.h

   Include for WINFILE program

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

**************************************************************************/

#define TOOLBAR
#define NOCOMM
#define WIN31
#define NTFS

#include <windows.h>
#include <windowsX.h>
#include <setjmp.h>
#include <string.h>
#include <memory.h>
#include "mpr.h"
#include <npapi.h>
#include <wfext.h>
#include <commdlg.h>
#include <commctrl.h>
#include "fmifs.h"
#include <shellapi.h>
#include <shlwapi.h>
#include "suggest.h"
#include "numfmt.h"

#include "wfexti.h"
#include "wfhelp.h"

#include "wfdocb.h"
#include "wfmem.h"
#ifdef HEAPCHECK
#include "heap.h"
#endif
//
// Japan markers:
//

#define JAPANBEGIN
#define JAPANEND
#define KOREAJAPANBEGIN
#define KOREAJAPANEND

#define bJAPAN bJapan
#define bKOREAJAPAN bJapan

#define STKCHK()

#ifdef UNICODE
#ifdef atoi
#undef atoi
#endif

// old winuserp.h
#define WM_DROPOBJECT                   0x022A
#define WM_QUERYDROPOBJECT              0x022B
#define WM_BEGINDRAG                    0x022C
#define WM_DRAGLOOP                     0x022D
#define WM_DRAGSELECT                   0x022E
#define WM_DRAGMOVE                     0x022F

#define WM_LBTRACKPOINT                 0x0131

#ifdef  UNICODE                     // r_winnt
typedef WCHAR TUCHAR, *PTUCHAR;
#else   /* UNICODE */               // r_winnt
typedef unsigned char TUCHAR, *PTUCHAR;
#endif /* UNICODE */                // r_winnt


////////////////////////////////////////////////////////////////////////////
//
//  File Compression stuff
//
//  NOTE: This should be removed when FS_FILE_COMPRESSION is defined in a
//        global header file.
////////////////////////////////////////////////////////////////////////////

#ifndef FS_FILE_COMPRESSION
#define FS_FILE_COMPRESSION 0x0010
#endif  //  FS_FILE_COMPRESSION


#define atoi atoiW
INT atoiW(LPWSTR sz);
#endif //UNICODE

#define SIZENOMDICRAP       944
#define MAX_TAB_COLUMNS     10

#define MAXDOSFILENAMELEN   12+1            // includes the NULL
#define MAXDOSPATHLEN       (68+MAXDOSFILENAMELEN)  // includes the NULL

#define MAXLFNFILENAMELEN   260
#define MAXLFNPATHLEN       260

#define MAXFILENAMELEN      MAXLFNFILENAMELEN
#define MAXPATHLEN          MAXLFNPATHLEN

#define MAXTITLELEN         128
#define MAXSUGGESTLEN       260    // for non-expanding suggest message
#define MAXERRORLEN         (MAXPATHLEN + MAXSUGGESTLEN)
#define MAXMESSAGELEN       (MAXPATHLEN * 2 + MAXSUGGESTLEN)

#define MAX_WINDOWS         27
#define MAX_DRIVES          26

// struct for volume info

#define MAX_VOLNAME             MAXPATHLEN
#define MAX_FILESYSNAME         MAXPATHLEN

// Maximum size of an extension, including NULL
#define EXTSIZ 8

#define TA_LOWERCASE    0x01
#define TA_BOLD     0x02
#define TA_ITALIC   0x04
#define TA_LOWERCASEALL 0x08


#define FF_NULL 0x0
#define FF_ONLYONE 0x1000
#define FF_PRELOAD 0x2000
#define FF_RETRY   0x4000

#define SZ_NTLDR          TEXT("NTLDR")

#define SZ_DQUOTE         TEXT("\"")
#define SZ_DOT            TEXT(".")
#define SZ_DOTDOT         TEXT("..")
#define SZ_QUESTION       TEXT("?")
#define SZ_ACOLONSLASH    TEXT("A:\\")
#define SZ_ACOLON         TEXT("A:")

#define SZ_PERCENTD       TEXT("%d")
#define SZ_PERCENTFORMAT  TEXT("%3d%%")

#define SZ_NTFSNAME       TEXT("NTFS")
#define SZ_FATNAME        TEXT("FAT")
#define SZ_FILESYSNAMESEP TEXT(" - ")
#define SZ_CLOSEBRACK     TEXT("]")
#define SZ_BACKSLASH      TEXT("\\")
#define SZ_COLON          TEXT(":")
#define SZ_STAR           TEXT("*")
#define SZ_DOTSTAR        TEXT(".*")
#define SZ_COLONONE       TEXT(":1")
#define SZ_SPACEDASHSPACE TEXT(" - ")


#define CHAR_DASH TEXT('-')
#define CHAR_CARET TEXT('^')
#define CHAR_UNDERSCORE TEXT('_')
#define CHAR_AND TEXT('&')
#define CHAR_TAB TEXT('\t')
#define CHAR_LESS TEXT('<')
#define CHAR_GREATER TEXT('>')
#define CHAR_EQUAL TEXT('=')
#define CHAR_PLUS TEXT('+')
#define CHAR_SEMICOLON TEXT(';')
#define CHAR_COMMA TEXT(',')
#define CHAR_PIPE TEXT('|')
#define CHAR_BACKSLASH TEXT('\\')
#define CHAR_SLASH TEXT('/')
#define CHAR_OPENBRACK TEXT('[')
#define CHAR_CLOSEBRACK TEXT(']')
#define CHAR_ZERO TEXT('0')
#define CHAR_COLON TEXT(':')
#define CHAR_SPACE TEXT(' ')
#define CHAR_NEWLINE TEXT('\n')

#define CHAR_DOT TEXT('.')
#define CHAR_OPENPAREN TEXT('(')
#define CHAR_CLOSEPAREN TEXT(')')
#define CHAR_HASH TEXT('#')
#define CHAR_DQUOTE TEXT('"')

#define CHAR_NULL TEXT('\0')
#define CHAR_QUESTION TEXT('?')
#define CHAR_STAR TEXT('*')
#define CHAR_PERCENT TEXT('%')

#define CHAR_A TEXT('A')
#define CHAR_a TEXT('a')
#define CHAR_Z TEXT('Z')

// Default char for untranslatable unicode
// MUST NOT BE an acceptable char for file systems!!
// (GetNextPair scans for this and uses altname)
#define CHAR_DEFAULT CHAR_QUESTION

#define FM_EXT_PROC_ENTRYA "FMExtensionProc"
#define FM_EXT_PROC_ENTRYW "FMExtensionProcW"

#define UNDELETE_ENTRYA "UndeleteFile"
#define UNDELETE_ENTRYW "UndeleteFileW"

//
// Moved from wfcopy.h
// The problem is that IsTheDiskReallyThere/CheckDrive
// uses these codes so they really need to be global
//

#define FUNC_MOVE       0x0001
#define FUNC_COPY       0x0002
#define FUNC_DELETE     0x0003
#define FUNC_RENAME     0x0004

//
// These should not be used in the move/copy code;
// only for IsTheDiskReallyThere
//
#define FUNC_SETDRIVE       0x0005
#define FUNC_EXPAND     0x0006
#define FUNC_LABEL      0x0007


#define FILE_NOTIFY_CHANGE_FLAGS (FILE_NOTIFY_CHANGE_FILE_NAME | \
   FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE)

#define DwordAlign(cb)      ((cb + 3) & ~3)
#define ISDOTDIR(x)  (x[0]==CHAR_DOT && (!x[1] || (x[1] == CHAR_DOT && !x[2])))
#define ISUNCPATH(x) (CHAR_BACKSLASH == x[0] && CHAR_BACKSLASH == x[1])
#define DRIVESET(str, drive) str[0] = CHAR_A + drive
#define COUNTOF(x) (sizeof(x)/sizeof(*x))
#define ByteCountOf(x) ((x)*sizeof(TCHAR))
#define abs(x) ((x < 0) ? -x : x)

#define DRIVEID(path) ((path[0] - CHAR_A)&31)

#define IsDocument(lpszPath)       IsBucketFile(lpszPath, ppDocBucket)
#define IsProgramFile(lpszPath)    IsBucketFile(lpszPath, ppProgBucket)
#define IsProgramIconFile(lpszPath) IsBucketFile(lpszPath, ppProgIconBucket)

//
// Some typedefs
//

typedef HWND *PHWND;
typedef INT DRIVE;
typedef INT DRIVEIND;

#include "wfinfo.h"

typedef struct _CANCEL_INFO {
   HWND hCancelDlg;
   BOOL bCancel;
   HANDLE hThread;
   BOOL fmifsSuccess;
   UINT dReason;
   UINT fuStyle;                      // Message box style
   INT  nPercentDrawn;                // percent drawn so FAR
   enum _CANCEL_TYPE {
      CANCEL_NULL=0,
      CANCEL_FORMAT,
      CANCEL_COPY,
      CANCEL_BACKUP,
      CANCEL_RESTORE,
      CANCEL_COMPRESS,
      CANCEL_UNCOMPRESS
   } eCancelType;
   BOOL bModal;
   struct _INFO {
      struct _FORMAT {
         INT iFormatDrive;
         FMIFS_MEDIA_TYPE fmMediaType;
         BOOL fQuick;
         DWORD fFlags;                 // FF_ONLYONE = 0x1000
         TCHAR szLabel[13];
      } Format;
      struct _COPY {
         INT iSourceDrive;
         INT iDestDrive;
         BOOL bFormatDest;
      } Copy;
   } Info;
} CANCEL_INFO, *PCANCEL_INFO;


typedef struct _SEARCH_INFO {
   HWND hSearchDlg;
   INT iDirsRead;
   INT iFileCount;
   DWORD dwError;
   HANDLE hThread;
   HWND hwndLB;
   BOOL bUpdateStatus;
   BOOL bCancel;
   BOOL bDontSearchSubs;
   BOOL bIncludeSubDirs;
   BOOL bCasePreserved;
   INT iRet;
   LPXDTALINK lpStart;
   enum _SEARCH_STATUS {
      SEARCH_NULL=0,
      SEARCH_CANCEL,
      SEARCH_ERROR,
      SEARCH_MDICLOSE
   } eStatus;
   WCHAR szSearch[MAXPATHLEN+1];
   FILETIME ftSince;			// UTC
} SEARCH_INFO, *PSEARCH_INFO;

typedef struct _COPYINFO {
   LPTSTR pFrom;
   LPTSTR pTo;
   DWORD dwFunc;
   BOOL bUserAbort;
} COPYINFO, *PCOPYINFO;

typedef enum eISELTYPE {
   SELTYPE_ALL = 0,
   SELTYPE_FIRST = 1,
   SELTYPE_TESTLFN = 2,
   SELTYPE_QUALIFIED = 4,
   SELTYPE_FILESPEC = 8,
   SELTYPE_NOCHECKESC = 16,
   SELTYPE_SHORTNAME = 32
} ISELTYPE;

// struct for save and restore of window positions

typedef struct {

   //
   // *2 since may have huge filter
   //
   WCHAR szDir[2*MAXPATHLEN];

   //
   // Next block of fields must be together (11 DWORDS)
   //
   RECT  rc;
   POINT pt;
   INT  sw;
   DWORD dwView;
   DWORD dwSort;
   DWORD dwAttribs;
   INT   nSplit;
} WINDOW, *PWINDOW;

typedef struct _SELINFO *PSELINFO;

//--------------------------------------------------------------------------
//
//  Function Templates
//
//--------------------------------------------------------------------------


// WFDLGS3.C

VOID FormatDiskette(HWND hwnd, BOOL bModal);
VOID CopyDiskEnd(VOID);
VOID FormatEnd(VOID);
VOID DestroyCancelWindow(VOID);
VOID  UpdateConnections(BOOL bUpdateDriveList);


// WFDLGS.C

VOID ActivateCommonContextMenu(HWND hwnd, HWND hwndLB, LPARAM lParam);
VOID KillQuoteTrailSpace( LPTSTR szFile );
VOID SaveWindows(HWND hwndMain);
VOID NewFont(VOID);


// WFCHGNOT.C

VOID InitializeWatchList(VOID);
VOID ModifyWatchList(HWND hwndWatch, LPTSTR lpPath, DWORD fdwFilter);
VOID DestroyWatchList(VOID);
VOID NotifyPause(DRIVE drive, UINT uType);
VOID NotifyResume(DRIVE drive, UINT uType);
VOID ChangeNotify(INT iEvent);
VOID ChangeNotifyRefresh(DWORD iEvent);
VOID vWaitMessage();


// WFCOMMAN.C

VOID RedoDriveWindows(HWND);
BOOL FmifsLoaded(VOID);
VOID  ChangeFileSystem(DWORD dwOper, LPWSTR lpPath, LPWSTR lpTo);
HWND  CreateDirWindow(register LPWSTR szPath, BOOL bReplaceOpen, HWND hwndActive);
HWND CreateTreeWindow(LPWSTR szPath, INT x, INT y, INT dx, INT dy, INT dxSplit);
VOID SwitchToSafeDrive();
DWORD ReadMoveStatus();
VOID UpdateMoveStatus(DWORD dwEffect);


// WFDOS.C

VOID GetDiskSpace(DRIVE drive, PLARGE_INTEGER pqFreeSpace, PLARGE_INTEGER pqTotalSpace);
INT   ChangeVolumeLabel(DRIVE, LPTSTR);
DWORD GetVolumeLabel(DRIVE, LPTSTR*, BOOL);
DWORD
FillVolumeInfo(DRIVE drive, LPTSTR lpszVolName, PDWORD pdwVolumeSerialNumber,
   PDWORD pdwMaximumComponentLength, PDWORD pdwFileSystemFlags,
   LPTSTR lpszFileSysName);

DWORD  WF_CreateDirectory(HWND, LPTSTR, LPTSTR);


// WFEXT.C

VOID ExtSelItemsInvalidate();


// WFUTIL.C

LPWSTR pszNextComponent(LPWSTR pszCmdLine);
VOID cdecl SetStatusText(INT nPane, UINT nFormat, LPCTSTR szFormat, ...);
VOID RefreshWindow(HWND hwndActive, BOOL bUpdateDriveList, BOOL bFlushCache);
BOOL IsLastWindow(VOID);
LPTSTR AddCommasInternal(LPTSTR szBuf, DWORD dw);

VOID InvalidateChildWindows(HWND hwnd);
BOOL IsValidDisk(DRIVE drive);
LPTSTR GetSelection(INT iSelType, PBOOL pbDir);
LPTSTR GetNextFile(LPTSTR pCurSel, LPTSTR szFile, INT size);

VOID  SetWindowDirectory(VOID);
VOID  SetDlgDirectory(HWND hDlg, LPTSTR pszPath);
VOID  WritePrivateProfileBool(LPTSTR szKey, BOOL bParam);
VOID  WritePrivateProfileInt(LPTSTR szKey, INT wParam);
BOOL  IsWild(LPTSTR lpszPath);
UINT  AddBackslash(LPTSTR lpszPath);
VOID  StripBackslash(LPTSTR lpszPath);
VOID  StripFilespec(LPTSTR lpszPath);
VOID  StripPath(LPTSTR lpszPath);
LPTSTR GetExtension(LPTSTR pszFile);
BOOL  FindExtensionInList(LPTSTR pszExt, LPTSTR pszList);
INT   MyMessageBox(HWND hWnd, DWORD idTitle, DWORD idMessage, DWORD dwStyle);
DWORD ExecProgram(LPTSTR,LPTSTR,LPTSTR,BOOL,BOOL);
PDOCBUCKET IsBucketFile(LPTSTR lpszPath, PPDOCBUCKET ppDocBucket);
BOOL  IsNTFSDrive(DRIVE);
BOOL  IsCasePreservedDrive(DRIVE);

BOOL  IsRemovableDrive(DRIVE);
BOOL  IsRemoteDrive(DRIVE);
VOID  SetMDIWindowText(HWND hwnd, LPTSTR szTitle);
INT   GetMDIWindowText(HWND hwnd, LPTSTR szTitle, INT size);
BOOL  ResizeSplit(HWND hWnd, INT dxSplit);
VOID  CheckEsc(LPTSTR);
VOID  GetMDIWindowVolume(HWND hWnd, LPTSTR szTitle, INT size);
BOOL TypeAheadString(WCHAR ch, LPWSTR szT);

VOID SaveHistoryDir(HWND hwnd, LPWSTR szDir);
BOOL GetPrevHistoryDir(BOOL forward, HWND *phwnd, LPWSTR szDir);

// WFDIR.C

VOID   UpdateStatus(HWND hWnd);
LPWSTR DirGetSelection(HWND hwndDir, HWND hwndView, HWND hwndLB, INT iSelType, BOOL *pfDir, PINT piLastSel);
VOID   FillDirList(HWND hwndDir, LPXDTALINK lpStart);
VOID   CreateLBLine(register DWORD dwLineFormat, LPXDTA lpxdta, LPTSTR szBuffer);
INT    GetMaxExtent(HWND hwndLB, HANDLE hDTA, BOOL bNTFS);
VOID   UpdateSelection(HWND hwndLB);

INT  PutDate(LPFILETIME lpftDate, LPTSTR szStr);
INT  PutTime(LPFILETIME lpftTime, LPTSTR szStr);
INT  PutSize(PLARGE_INTEGER pqSize, LPTSTR szOutStr);
INT  PutAttributes(register DWORD dwAttribute, register LPTSTR szStr);
HWND GetMDIChildFromDecendant(HWND hwnd);
VOID SetLBFont(HWND hwnd, HWND hwndLB, HANDLE hNewFont, DWORD dwViewFlags, LPXDTALINK lpStart);


// WFDIRRD.C

BOOL  InitDirRead(VOID);
VOID  DestroyDirRead(VOID);
LPXDTALINK CreateDTABlock(HWND hwnd, LPWSTR pPath, DWORD dwAttribs, BOOL bDontSteal);
VOID  FreeDTA(HWND hwnd);
VOID  DirReadDestroyWindow(HWND hwndDir);
LPXDTALINK DirReadDone(HWND hwndDir, LPXDTALINK lpStart, INT iError);
VOID  BuildDocumentString(VOID);
VOID  BuildDocumentStringWorker(VOID);

// WFDIRSRC.C

HCURSOR  GetMoveCopyCursor(VOID);
VOID  DrawItem(HWND hwnd, DWORD dwViewOpts, LPDRAWITEMSTRUCT lpLBItem, BOOL bHasFocus);
VOID  DSDragLoop(HWND hwndLB, WPARAM wParam, LPDROPSTRUCT lpds);
BOOL  DSRectItem(HWND hwndLB, INT iSel, BOOL bFocusOn, BOOL bSearch);
VOID  DSDragScrollSink(LPDROPSTRUCT lpds);
INT   DSTrackPoint(HWND hWnd, HWND hwndLB, WPARAM wParam, LPARAM lParam, BOOL bSearch);
VOID  DSSetSelection(HWND hwndLB, BOOL bSelect, LPTSTR szSpec, BOOL bSearch);
BOOL  DSDropObject(HWND hwndHolder, HWND hwndLB, LPDROPSTRUCT lpds, BOOL bSearch);
INT   FixTabsAndThings(HWND hwndLB, WORD *pwTabs, INT iMaxWidthFileName, INT iMaxWidthNTFSFileName, DWORD dwViewOpts);
LPWSTR SkipPathHead(LPWSTR lpszPath);


// WFPRINT.C

DWORD  WFPrint(LPTSTR szFile);


// WINFILE.C

BOOL InitPopupMenus(UINT uMenus, HMENU hMenu, HWND hwndActive);
INT  MessageFilter(INT nCode, WPARAM wParam, LPMSG lParam);


// WFTREE.C

BOOL  CompactPath(HDC hdc, LPTSTR szPath, DWORD dx);
VOID  ResizeWindows(HWND hwndParent, INT dxWindow, INT dyWindow);
VOID  GetTreeWindows(HWND hwnd, PHWND phwndTree, PHWND phwndDir);
HWND  GetTreeFocus(HWND hWnd);
VOID  SwitchDriveSelection(HWND hwndActive, BOOL bSelectToolbarDrive);


// WFINIT.C

VOID  GetInternational(VOID);
BOOL  LoadBitmaps(VOID);
BOOL  InitFileManager(HANDLE hInstance, LPTSTR lpCmdLine, INT nCmdShow);
VOID  InitDriveBitmaps(VOID);
VOID  InitExtensions(VOID);
VOID  FreeFileManager(VOID);
VOID  DeleteBitmaps(VOID);
BOOL  CreateSavedWindows(VOID);
VOID  InitExtensions(VOID);
INT   GetDriveOffset(register DRIVE drive);
VOID  InitMenus(VOID);
VOID  LoadFailMessage(VOID);
UINT  FillDocType(PPDOCBUCKET ppDoc, LPCWSTR pszSection, LPCWSTR pszDefault);
BOOL  CheckDirExists(LPWSTR szDir);

DWORD StartBuildingDirectoryTrie();


// WFCOPY.C

DWORD  DMMoveCopyHelper(LPTSTR pFrom, LPTSTR pTo, BOOL bCopy);
DWORD  WFMoveCopyDriver(PCOPYINFO pCopyInfo);
VOID   WFMoveCopyDriverThread(PCOPYINFO pCopyInfo);

BOOL  IsDirectory(LPTSTR pPath);
BOOL  IsTheDiskReallyThere(HWND hwnd, register LPTSTR pPath, DWORD wFunc, BOOL bModal);
BOOL  QualifyPath(LPTSTR);
INT   CheckMultiple(LPTSTR pInput);
VOID  SetDlgItemPath(HWND hDlg, INT id, LPTSTR pszPath);
DWORD NetCheck(LPTSTR pPath, DWORD dwType);

VOID DialogEnterFileStuff(register HWND hwnd);


// WFUTIL.C

VOID  GetAllDirectories(LPTSTR rgszDirs[]);
BOOL  GetDriveDirectory(INT iDrive, LPTSTR pszDir);
VOID  GetSelectedDirectory(INT iDrive, LPTSTR pszDir);
VOID  SaveDirectory(LPTSTR pszDir);
INT   GetSelectedDrive(VOID);
VOID  GetTextStuff(HDC hdc);
INT   GetHeightFromPointsString(LPTSTR szPoints);
INT   GetDrive(HWND hwnd, POINT pt);
VOID  CheckSlashies(LPTSTR);
// DWORD IsNetDrive(DRIVE drive);
BOOL  IsCDRomDrive(DRIVE drive);
BOOL  IsRamDrive(DRIVE drive);
VOID  CleanupMessages();
HWND  GetRealParent(HWND hwnd);
VOID  WFHelp(HWND hwnd);


// WFDRIVES.C

BOOL CheckDrive(HWND hwnd, DRIVE drive, DWORD dwFunc);
VOID  NewTree(DRIVE drive, HWND hWnd);
VOID  GetDriveRect(DRIVEIND driveInd, PRECT prc);


// Wnd Procs

LRESULT FrameWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
BOOL  AppCommandProc(DWORD id);
LRESULT TreeWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT DriveWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT DrivesWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT VolumeWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT TreeChildWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT TreeControlWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT DirWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

LRESULT SearchWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT DirListBoxWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
VOID (*lpfnFormat)(PWSTR,FMIFS_MEDIA_TYPE,PWSTR,PWSTR,BOOLEAN,FMIFS_CALLBACK);
VOID (*lpfnDiskCopy)(PWSTR,PWSTR,BOOLEAN,FMIFS_CALLBACK);
BOOLEAN (*lpfnSetLabel)(PWSTR,PWSTR);
BOOLEAN (*lpfnQuerySupportedMedia)(PWSTR,PFMIFS_MEDIA_TYPE,DWORD,PDWORD);
BOOL Callback_Function(FMIFS_PACKET_TYPE PacketType, DWORD PacketLength, PVOID PacketData);

BOOL CancelDlgProc  (HWND, UINT, WPARAM, LPARAM);
INT_PTR  DrivesDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  AssociateDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  SearchDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  RunDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  SelectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  FontDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  SuperDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  AttribsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  MakeDirDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  ExitDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  DiskLabelDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  ChooseDriveDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  FormatDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  OtherDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

INT_PTR  ProgressDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  SortByDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  IncludeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  ConfirmDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  GotoDirDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

VOID SetCurrentPathOfWindow(LPWSTR szPath);

// WFSEARCH.C

VOID GetSearchPath(HWND hwnd, LPWSTR szTemp);
LRESULT CALLBACK SearchProgDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID UpdateSearchStatus(HWND hwndLB, INT nCount);
VOID SearchEnd(VOID);


// WFFILE.C
BOOL WFCheckCompress(HWND hDlg, LPTSTR szNameSpec, DWORD dwNewAttrs, BOOL bPropertyDlg, BOOL *bIgnoreAll);
BOOL GetRootPath(LPTSTR szPath, LPTSTR szReturn);


// TBAR.C

VOID  CreateFMToolbar(VOID);
DWORD DriveListMessage(UINT wMsg, WPARAM wParam, LPARAM lParam, UINT* puiRetVal);

VOID  SelectToolbarDrive(DRIVEIND DriveInd);
VOID  FillToolbarDrives(DRIVE drive);
VOID  EnableCheckTBButtons(HWND hwndActive);
VOID  CheckTBButton(DWORD idCommand);
VOID  InitToolbarButtons(VOID);
VOID  EnableDisconnectButton(VOID);
VOID  EnableStopShareButton(VOID);
BOOL  InitToolbarExtension(INT iExt);
VOID  FreeToolbarExtensions(VOID);

VOID  SaveRestoreToolbar(BOOL bSave);
VOID  BuildDriveLine(LPTSTR* lpszTemp, INT i, BOOL fGetFloppyLabel, DWORD dwType);


// LFN.C

DWORD WFCopy(LPTSTR,LPTSTR);
DWORD WFRemove(LPTSTR pszFile);
DWORD WFMove(LPTSTR pszFrom, LPTSTR pszTo, PBOOL pbErrorOnDest, BOOL bSilent);


// TREECTL.C

VOID  wfYield(VOID);
VOID  InvalidateAllNetTypes(VOID);
VOID  GetTreeUNCName(HWND hwndTree, LPTSTR szBuf, INT nBuf);
BOOL  RectTreeItem(HWND hwndLB, register INT iItem, BOOL bFocusOn);


//--------------------------------------------------------------------------
//
//  Defines
//
//--------------------------------------------------------------------------

#define SST_RESOURCE 0X1
#define SST_FORMAT   0x2

#define DRIVE_INFO_NAME_HEADER 4

#define DO_LISTOFFILES      1L

#define WS_MDISTYLE (WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX)
#define WS_DIRSTYLE (WS_CHILD | LBS_SORT | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT)
#define WS_SEARCHSTYLE  (WS_DIRSTYLE | LBS_HASSTRINGS | WS_VSCROLL)


//
// Extra Window Word Offsets
//

// NOTE: see winfile.c for a description of the overall window structure.

//
// Idx  Tree         Search         Dir
// 0    SPLIT        HDTA           HDTA
// 1    PATHLEN      TABARRAY       TABARRAY
// 2    VOLNAME      LISTPARMS      LISTPARMS
// 3    NOTIFYPAUSE  IERROR         IERROR
// 4    TYPE         TYPE           HDTAABORT
// 5    VIEW         VIEW           INITIALDIRSEL
// 6    SORT         SORT           NEXTHWND
// 7    OLEDROP      n/a            OLEDROP
// 8    ATTRIBS      ATTRIBS        
// 9    FCSFLAG      FSCFLAG
// 10   LASTFOCUS    LASTFOCUS
//


#define GWL_SPLIT         (0*sizeof(LONG_PTR))
#define GWL_HDTA          (0*sizeof(LONG_PTR))

#define GWL_PATHLEN       (1*sizeof(LONG_PTR))
#define GWL_TABARRAY      (1*sizeof(LONG_PTR))

#define GWL_VOLNAME       (2*sizeof(LONG_PTR))
#define GWL_LISTPARMS     (2*sizeof(LONG_PTR))

#define GWL_NOTIFYPAUSE  (3*sizeof(LONG_PTR))
#define GWL_IERROR       (3*sizeof(LONG_PTR))

#define GWL_TYPE         (4*sizeof(LONG_PTR))     // > 0 Tree, -1 = search
#define GWL_HDTAABORT    (4*sizeof(LONG_PTR))

#define GWL_VIEW         (5*sizeof(LONG_PTR))
#define GWL_SELINFO      (5*sizeof(LONG_PTR))

#define GWL_SORT         (6*sizeof(LONG_PTR))
#define GWL_NEXTHWND     (6*sizeof(LONG_PTR))

#define GWL_OLEDROP      (7*sizeof(LONG_PTR))

#define GWL_ATTRIBS      (8*sizeof(LONG_PTR))

#define GWL_FSCFLAG      (9*sizeof(LONG_PTR))

#define GWL_LASTFOCUS    (10*sizeof(LONG_PTR))

// szDrivesClass...

#define GWL_CURDRIVEIND     (0*sizeof(LONG_PTR))   // current selection in drives window
#define GWL_CURDRIVEFOCUS   (1*sizeof(LONG_PTR))   // current focus in drives window
#define GWL_LPTSTRVOLUME    (2*sizeof(LONG_PTR))   // LPTSTR to Volume/Share string

// szTreeControlClass

#define GWL_READLEVEL       (0*sizeof(LONG_PTR))   // iReadLevel for each tree control window
#define GWL_XTREEMAX        (1*sizeof(LONG_PTR))   // max text extent for each tree control window

// GWL_TYPE numbers

#define TYPE_TREE           0   // and all positive numbers (drive number)
#define TYPE_SEARCH         -1

/* WM_FILESYSCHANGE (WM_FSC) message wParam value */
#define FSC_CREATE          0
#define FSC_DELETE          1
#define FSC_RENAME          2
#define FSC_ATTRIBUTES      3
#define FSC_NETCONNECT      4
#define FSC_NETDISCONNECT   5
#define FSC_REFRESH         6
#define FSC_MKDIR           7
#define FSC_RMDIR           8
#define FSC_RMDIRQUIET      9
#define FSC_MKDIRQUIET      10

#define WM_LBTRACKPT        0x131

#define TC_SETDRIVE         0x944
#define TC_GETCURDIR        0x945
#define TC_EXPANDLEVEL      0x946
#define TC_COLLAPSELEVEL    0x947
#define TC_GETDIR           0x948
#define TC_SETDIRECTORY     0x949
#define TC_TOGGLELEVEL      0x950
#define TC_RECALC_EXTENT    0x951

#define FS_CHANGEDISPLAY    (WM_USER+0x100)
#define FS_CHANGEDRIVES     (WM_USER+0x101)
#define FS_GETSELECTION     (WM_USER+0x102)
#define FS_GETDIRECTORY     (WM_USER+0x103)
#define FS_GETDRIVE         (WM_USER+0x104)
#define FS_SETDRIVE         (WM_USER+0x107)
#define FS_GETFILESPEC      (WM_USER+0x108)
#define FS_SETSELECTION     (WM_USER+0x109)

// modeless format/copy support
#define FS_CANCELBEGIN      (WM_USER+0x10A)
#define FS_CANCELEND        (WM_USER+0x10B)
#define FS_SEARCHEND        (WM_USER+0x10C)
#define FS_SEARCHLINEINSERT (WM_USER+0x10D)

#define FS_SEARCHUPDATE     (WM_USER+0x10E)
#define FS_CANCELUPDATE     (WM_USER+0x10F)

#define FS_CANCELMESSAGEBOX        (WM_USER+0x110)
#define FS_CANCELCOPYFORMATDEST    (WM_USER+0x111)
#define FS_UPDATEDRIVETYPECOMPLETE (WM_USER+0x112)
#define FS_UPDATEDRIVELISTCOMPLETE (WM_USER+0x113)
#define FS_FSCREQUEST              (WM_USER+0x114)
#define FS_NOTIFYRESUME            (WM_USER+0x115)
#define FS_COPYDONE                (WM_USER+0x116)
#define FS_DIRREADDONE             (WM_USER+0x117)
#define FS_REBUILDDOCSTRING        (WM_USER+0x118)

#define FS_TESTEMPTY               (WM_USER+0x119)

#define WM_FSC                     (WM_USER+0x120)

#define FS_ENABLEFSC               (WM_USER+0x121)
#define FS_DISABLEFSC              (WM_USER+0x122)

#define ATTR_READWRITE      0x0000
#define ATTR_READONLY       FILE_ATTRIBUTE_READONLY     // == 0x0001
#define ATTR_HIDDEN         FILE_ATTRIBUTE_HIDDEN       // == 0x0002
#define ATTR_SYSTEM         FILE_ATTRIBUTE_SYSTEM       // == 0x0004
#define ATTR_VOLUME         0x0008                      // == 0x0008
#define ATTR_DIR            FILE_ATTRIBUTE_DIRECTORY    // == 0x0010
#define ATTR_ARCHIVE        FILE_ATTRIBUTE_ARCHIVE      // == 0x0020
#define ATTR_NORMAL         FILE_ATTRIBUTE_NORMAL       // == 0x0080
#define ATTR_TEMPORARY      FILE_ATTRIBUTE_TEMPORARY    // == 0x0100
#define ATTR_REPARSE_POINT  FILE_ATTRIBUTE_REPARSE_POINT // == 0x0400  
#define ATTR_COMPRESSED     FILE_ATTRIBUTE_COMPRESSED   // == 0x0800
#define ATTR_NOT_INDEXED    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED // == 0x2000
#define ATTR_USED           0x2DBF						// ATTR we use that are returned from FindFirst/NextFile

#define ATTR_PARENT         0x0040  // my hack DTA bits
#define ATTR_LOWERCASE      0x4000
#define ATTR_LFN           0x10000  // my hack DTA bits
#define ATTR_JUNCTION      0x20000
#define ATTR_SYMBOLIC      0x40000

#define ATTR_RWA            (ATTR_READWRITE | ATTR_ARCHIVE)
#define ATTR_ALL            (ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_DIR | ATTR_ARCHIVE | ATTR_NORMAL | ATTR_COMPRESSED | ATTR_REPARSE_POINT)
#define ATTR_PROGRAMS       0x0100
#define ATTR_DOCS           0x0200
#define ATTR_OTHER          0x1000
#define ATTR_EVERYTHING     (ATTR_ALL | ATTR_PROGRAMS | ATTR_DOCS | ATTR_OTHER | ATTR_PARENT)
#define ATTR_DEFAULT        (ATTR_EVERYTHING & ~(ATTR_HIDDEN | ATTR_SYSTEM))
#define ATTR_HS             (ATTR_HIDDEN | ATTR_SYSTEM)


#define ATTR_RETURNED       0x8000  /* used in DTA's by copy */

#define CD_PATH             0x0001
#define CD_VIEW             0x0002
#define CD_SORT             0x0003
#define CD_PATH_FORCE       0x0004
#define CD_SEARCHUPDATE     0x0005
#define CD_SEARCHFONT       0x0006

#define CD_DONTSTEAL        0x4000
#define CD_ALLOWABORT       0x8000

#define VIEW_NAMEONLY       0x0000
#define VIEW_UPPERCASE      0x0001
#define VIEW_SIZE           0x0002
#define VIEW_DATE           0x0004
#define VIEW_TIME           0x0008
#define VIEW_FLAGS          0x0010
#define VIEW_PLUSES         0x0020
#define VIEW_DOSNAMES       0x0040

#define VIEW_EVERYTHING     (VIEW_SIZE | VIEW_TIME | VIEW_DATE | VIEW_FLAGS)

#define CBSECTORSIZE        512

#define INT13_READ          2
#define INT13_WRITE         3

#define ERR_USER            0xF000

/* Child Window IDs */
#define IDCW_DRIVES         1
#define IDCW_DIR            2
#define IDCW_TREELISTBOX    3	// list in tree control
#define IDCW_TREECONTROL    5
#define IDCW_LISTBOX        6   // list in directory and search


#define HasDirWindow(hwnd)      GetDlgItem(hwnd, IDCW_DIR)
#define HasTreeWindow(hwnd)     GetDlgItem(hwnd, IDCW_TREECONTROL)
#define GetSplit(hwnd)          ((int)GetWindowLongPtr(hwnd, GWL_SPLIT))


/* Menu Command Defines */
#define IDM_FILE            0
#define IDM_OPEN            101
#define IDM_PRINT           102
#define IDM_ASSOCIATE       103
#define IDM_SEARCH          104
#define IDM_RUN             105
#define IDM_MOVE            106
#define IDM_COPY            107
#define IDM_DELETE          108
#define IDM_RENAME          109
#define IDM_ATTRIBS         110
#define IDM_MAKEDIR         111
#define IDM_SELALL          112
#define IDM_DESELALL        113
#define IDM_UNDO            114
#define IDM_EXIT            115
#define IDM_SELECT          116
#define IDM_UNDELETE        117
#define IDM_COPYTOCLIPBOARD 118
#define IDM_COMPRESS        119
#define IDM_UNCOMPRESS      120
#define IDM_PASTE           121
#define IDM_EDIT            122
#define IDM_CUTTOCLIPBOARD  123
#define IDM_STARTCMDSHELL   124
#define IDM_GOTODIR         125
#define IDM_HISTORYBACK     126
#define IDM_HISTORYFWD      127
#define IDM_STARTPOWERSHELL 128

// This IDM_ is reserved for IDH_GROUP_ATTRIBS
#define IDM_GROUP_ATTRIBS   199

#define IDM_DISK            1
#define IDM_DISKCOPY        201
#define IDM_LABEL           202
#define IDM_FORMAT          203
//#define IDM_SYSDISK         204
#define IDM_CONNECT         205
#define IDM_DISCONNECT      206
#define IDM_DRIVESMORE      251
#define IDM_CONNECTIONS     252
#define IDM_SHAREDDIR       253

// AS added to end
#define IDM_SHAREAS         254
#define IDM_STOPSHARE       255

#define IDM_TREE            2
#define IDM_EXPONE          301
#define IDM_EXPSUB          302
#define IDM_EXPALL          303
#define IDM_COLLAPSE        304
#define IDM_NEWTREE         305

#define IDM_VIEW            3
#define IDM_VNAME           401
#define IDM_VDETAILS        402
#define IDM_VOTHER          403

#define IDM_BYNAME          404
#define IDM_BYTYPE          405
#define IDM_BYSIZE          406
#define IDM_BYDATE          407	// reverse date sort
#define IDM_BYFDATE			408	// forward date sort

#define IDM_VINCLUDE        409
#define IDM_REPLACE         410

#define IDM_TREEONLY        411
#define IDM_DIRONLY         412
#define IDM_BOTH            413
#define IDM_SPLIT           414

#define IDM_ESCAPE          420

#define IDM_OPTIONS         4
#define IDM_CONFIRM         501
#define IDM_LOWERCASE       502
#define IDM_STATUSBAR       503
#define IDM_MINONRUN        504
#define IDM_ADDPLUSES       505
#define IDM_EXPANDTREE      506

#define IDM_DRIVEBAR      507   /* Options->Drivebar */
#define IDM_TOOLBAR     508   /* Options->Toolbar */
#define IDM_NEWWINONCONNECT 509  /* Options->New Window On Connect */

#define IDM_FONT            510
#define IDM_SAVESETTINGS    511

#define IDM_TOOLBARCUST     512

#ifdef PROGMAN
#define IDM_SAVENOW         513
#endif

#define IDM_SECURITY        5
#define IDM_PERMISSIONS     605      // !! WARNING HARD CODED !!
#define IDM_AUDITING        606
#define IDM_OWNER           607

#define IDM_EXTENSIONS      6

#define IDM_WINDOW           16
#define IDM_CASCADE          1701
#define IDM_TILE             1702

#define IDM_TILEHORIZONTALLY 1703
#define IDM_REFRESH          1704
#define IDM_ARRANGE          1705
#define IDM_NEWWINDOW        1706
#define IDM_CHILDSTART       1707

#define IDM_HELP            17
#define IDM_HELPINDEX       1801
#define IDM_HELPKEYS        0x001E
#define IDM_HELPCOMMANDS    0x0020
#define IDM_HELPPROCS       0x0021
#define IDM_HELPHELP        1802
#define IDM_ABOUT           1803

#define IDM_DRIVELISTJUMP 2000  /* for defining an accelerator */


/* Control ID's; these must not conflict with an IDM_* */
#define IDC_TOOLBAR  3000
#define IDC_STATUS   3001
#define IDC_DRIVES   3002
#define IDC_EXTENSIONS  3003


#define BITMAPS             100

#define IDB_TOOLBAR  101
#define IDB_EXTRATOOLS  102

#define FILES_WIDTH         16
#define FILES_HEIGHT        16
#define MINIDRIVE_WIDTH     16
#define MINIDRIVE_HEIGHT    9
#define DRIVES_WIDTH        23
#define DRIVES_HEIGHT       14

#define APPICON             200
#define TREEICON            201
#define DIRICON             202
#define WINDOWSICON         203
#define TREEDIRICON         204

#define SINGLEMOVECURSOR    300 // move is even
#define MULTMOVECURSOR      302
#define SINGLECOPYCURSOR    301 // copy is odd
#define MULTCOPYCURSOR      303

#define APPCURSOR           300
#define DIRCURSOR           301
#define DOCCURSOR           302
#define FILECURSOR          304
#define FILESCURSOR         305
#define SPLITCURSOR         306

#define APPCURSORC          310
#define DIRCURSORC          311
#define DOCCURSORC          312
#define FILECURSORC         314
#define FILESCURSORC        315

#define WFACCELTABLE        400

#define FRAMEMENU           500

/* Indexes into the mondo bitmap */
#define BM_IND_APP          0
#define BM_IND_DOC          1
#define BM_IND_FIL          2
#define BM_IND_RO           3
#define BM_IND_DIRUP        4
#define BM_IND_CLOSE        5
#define BM_IND_CLOSEPLUS    6
#define BM_IND_OPEN         7
#define BM_IND_OPENPLUS     8
#define BM_IND_OPENMINUS    9
#define BM_IND_CLOSEMINUS   10
#define BM_IND_CLOSEDFS     11
#define BM_IND_OPENDFS      12

//#define IDS_ENDSESSION      40  /* Must be > 32 */
//#define IDS_ENDSESSIONMSG   41
#define IDS_COPYDISK        50
#define IDS_INSERTDEST      51
#define IDS_INSERTSRC       52
#define IDS_INSERTSRCDEST   53
#define IDS_FORMATTINGDEST  54
#define IDS_COPYDISKERR     55
#define IDS_COPYDISKERRMSG  56
#define IDS_COPYDISKSELMSG  57
#define IDS_COPYSRCDESTINCOMPAT 58
#define IDS_PERCENTCOMP     60
#define IDS_CREATEROOT      61
#define IDS_COPYSYSFILES    62
#define IDS_FORMATERR       63
//#define IDS_FORMATERRMSG    64
//#define IDS_FORMATCURERR    65
#define IDS_FORMATCOMPLETE  66
#define IDS_FORMATANOTHER   67
#define IDS_FORMATCANCELLED 68
//#define IDS_SYSDISK         70
//#define IDS_SYSDISKRUSURE   71
//#define IDS_SYSDISKERR      72
//#define IDS_SYSDISKNOFILES  73
//#define IDS_SYSDISKSAMEDRIVE    74
//#define IDS_SYSDISKADDERR   75
#define IDS_NETERR          80
//#define IDS_NETCONERRMSG    81
//#define IDS_NETDISCONCURERR 82
#define IDS_NETDISCONWINERR 83
//#define IDS_NETDISCON       84
//#define IDS_NETDISCONRUSURE 85
//#define IDS_NETDISCONERRMSG 86
//#define IDS_FILESYSERR      90
#define IDS_ATTRIBERR       91
#define IDS_MAKEDIRERR      92
#define IDS_LABELDISKERR    93
//#define IDS_SEARCHERR       94
#define IDS_SEARCHNOMATCHES 95
//#define IDS_MAKEDIREXISTS   96
#define IDS_SEARCHREFRESH   97
#define IDS_LABELACCESSDENIED  98
#define IDS_ASSOCFILE       100
#define IDS_DRIVETEMP       101
#define IDS_EXECERRTITLE    110
#define IDS_UNKNOWNMSG      111
#define IDS_NOMEMORYMSG     112
#define IDS_FILENOTFOUNDMSG 113
#define IDS_BADPATHMSG      114
//#define IDS_MANYOPENFILESMSG    115
#define IDS_NOASSOCMSG      116
//#define IDS_MULTIPLEDSMSG   117
#define IDS_ASSOCINCOMPLETE 118
#define IDS_MOUSECONFIRM    120
#define IDS_COPYMOUSECONFIRM    121
#define IDS_MOVEMOUSECONFIRM    122
#define IDS_EXECMOUSECONFIRM    123
#define IDS_WINFILE         124
//#define IDS_ONLYONE         125
#define IDS_TREETITLE       126
#define IDS_SEARCHTITLE     127
//#define IDS_NOFILESTITLE    130
//#define IDS_NOFILESMSG      131
#define IDS_TOOMANYTITLE    132
#define IDS_OOMTITLE        133
#define IDS_OOMREADINGDIRMSG    134
#define IDS_CURDIRIS        140
#define IDS_COPY            141
#define IDS_ANDCOPY         142
#define IDS_RENAME          143
#define IDS_ANDRENAME       144
#define IDS_FORMAT          145
#define IDS_FORMATSELDISK   146
//#define IDS_MAKESYSDISK     147
// moved #define IDS_DISCONNECT      148
//#define IDS_DISCONSELDISK   149
#define IDS_CREATINGMSG     150
#define IDS_REMOVINGMSG     151
#define IDS_COPYINGMSG      152
#define IDS_RENAMINGMSG     153
#define IDS_MOVINGMSG       154
#define IDS_DELETINGMSG     155
#define IDS_PRINTINGMSG     156
//#define IDS_NOSUCHDRIVE     160
#define IDS_MOVEREADONLY    161
#define IDS_RENAMEREADONLY  162
#define IDS_CONFIRMREPLACE  163
#define IDS_CONFIRMREPLACERO    164 /* Confirm/readonly */
#define IDS_CONFIRMRMDIR    165 /* Must be confirm + 1 */
#define IDS_CONFIRMRMDIRRO  166
#define IDS_CONFIRMDELETE   167
#define IDS_CONFIRMDELETERO 168
#define IDS_COPYINGTITLE    169
#define IDS_REMOVINGDIRMSG  170
#define IDS_STATUSMSG       180
#define IDS_DIRSREAD        181
#define IDS_DRIVEFREE       182
#define IDS_SEARCHMSG       183
#define IDS_DRIVE           184
#define IDS_SELECTEDFILES   185
#define IDS_NETDISCONOPEN   186
#define IDS_STATUSMSG2      187
#define IDS_DRIVENOTREADY   188
#define IDS_UNFORMATTED     189

//#define IDS_CANTPRINTTITLE  190
//#define IDS_PRINTFNF        191
#define IDS_PRINTDISK       192
#define IDS_PRINTMEMORY     193
#define IDS_PRINTERROR      194
#define IDS_TREEABORT       195
#define IDS_TREEABORTTITLE  196
#define IDS_DESTFULL        197
#define IDS_WRITEPROTECTFILE    198
#define IDS_FORMATQUICKFAILURE  199

//#define IDS_OS2APPMSG       200
//#define IDS_NEWWINDOWSMSG   201
//#define IDS_PMODEONLYMSG    202

#define IDS_DDEFAIL         203
#define IDS_FMIFSLOADERR    204

#define IDS_SHAREDDIR       209
#define IDS_FORMATCONFIRM   210
#define IDS_FORMATCONFIRMTITLE  211
#define IDS_DISKCOPYCONFIRM 212
#define IDS_DISKCOPYCONFIRMTITLE    213
#define IDS_ANDCLOSE           214
#define IDS_CLOSE              215
// moved #define IDS_UNDELETE        215 Taken.
// moved #define IDS_CONNECT         216
// moved #define IDS_CONNECTIONS     217
#define IDS_PATHNOTTHERE    218
#define IDS_PROGRAMS        219
#define IDS_ASSOCIATE       220
#define IDS_RUN             221
#define IDS_PRINTERRTITLE   222
#define IDS_WINHELPERR      223
#define IDS_NOEXEASSOC          224
#define IDS_ASSOCNOTEXE         225
#define IDS_ASSOCNONE           226
#define IDS_NOFILES             227
#define IDS_PRINTONLYONE        228
//#define IDS_COMPRESSEDEXE       229
#define IDS_INVALIDDLL          230
#define IDS_SHAREERROR          231
#define IDS_CREATELONGDIR       232
#define IDS_CREATELONGDIRTITLE  233
#define IDS_BYTES               234
#define IDS_SBYTES              235
#define IDS_NOCOPYTOCLIP        236

#define IDS_MENUANDITEM         237

#define IDS_DRIVELABEL          238 /* label for drive list */
#define IDS_STATUSMSGSINGLE     239 /* for building 1-file status display */

#define IDS_CONNECTHELP         240 /* status bar text for tbar buttons */
#define IDS_DISCONHELP          241
#define IDS_CONNECTIONHELP      242
#define IDS_SHAREASHELP         243
#define IDS_STOPSHAREHELP       244
#define IDS_VDETAILSHELP        245
#define IDS_VNAMEHELP           246
#define IDS_BYNAMEHELP          247
#define IDS_BYTYPEHELP          248
#define IDS_BYSIZEHELP          249
#define IDS_BYDATEHELP          250
#define IDS_NEWWINHELP          251
#define IDS_COPYHELP            252
#define IDS_MOVEHELP            253

#define IDS_DIRNAMELABEL        254 /* "&Directory Name:" in props dlg */
//#define IDS_FILEVERSIONKEY      255 /* base key name for getting ver info */
#define IDS_DRIVENOTAVAILABLE   256

// moved #define IDS_SHAREAS    257 /* "Share As..." menu item */
// moved #define IDS_STOPSHARE  258 /* "Stop Sharing..." menu item */

#define IDS_SHAREDAS    259 /* "Shared as %s" for status bar */
#define IDS_NOTSHARED   260 /* "Not shared" for status bar */

#define IDS_DELHELP     261

#define IDS_DRIVE_COMPRESSED    262

#define IDS_DRAG_COPYING        263
#define IDS_DRAG_MOVING         264
#define IDS_DRAG_EXECUTING      265

#define IDS_ORDERB      266
#define IDS_ORDERKB     267
#define IDS_ORDERMB     268
#define IDS_ORDERGB     269
#define IDS_ORDERTB     270


#define IDS_NOACCESSDIR  280
#define IDS_NOACCESSFILE  281

// for ERROR_BAD_PATHNAME 161L
//#define IDS_BADPATHNAME      282

#define IDS_DRIVEBUSY_COPY   283
#define IDS_DRIVEBUSY_FORMAT 284

#define IDS_COPYMOVENOTCOMPLETED 285
#define IDS_DIRREMAINS           286

#define IDS_NOSUCHDIRTITLE    287
#define IDS_NOSUCHDIR         288

#define IDS_BADNETNAMETITLE   289
#define IDS_BADNETNAME        290

//#define IDS_DIREXISTSASFILE   291

#define IDS_ALLFILES          292

#define IDS_ASSOC_OPEN        294
#define IDS_ASSOC_PRINT       295

#define IDS_ADDEXTTITLE       298
#define IDS_ADDEXTTEXT        299
#define IDS_EXTTITLE          300

#define IDS_EXTADDERROR       301
#define IDS_EXTDELERROR       302
#define IDS_FILETYPEADDERROR  303
#define IDS_FILETYPEDELERROR  304
#define IDS_FILETYPEREADERROR 305

#define IDS_FILETYPENULLDESCERROR 306
#define IDS_FILETYPEDUPDESCERROR  307

#define IDS_FILETYPEDELCONFIRMTITLE 308
#define IDS_FILETYPEDELCONFIRMTEXT  309
#define IDS_FILETYPEDELCONFIRMUSERTEXT  310
#define IDS_FILETYPEUSERIZETEXT  311
#define IDS_FILETYPECOMMANDNULLTEXT 312

#define IDS_NEWFILETYPETITLE  320
#define IDS_COPYINGDISKTITLE  321
#define IDS_SEARCHING         322

#define IDS_EXTTEXT           323
#define IDS_BUSYFORMATQUITVERIFY    324
#define IDS_BUSYCOPYQUITVERIFY      325

#define IDS_PERCENTCOMPLETE   326

#define IDS_DRIVEBASE       350
#define IDS_12MB            354
#define IDS_360KB           353
#define IDS_144MB           356
#define IDS_720KB           355
#define IDS_288MB           357
#define IDS_DEVICECAP       358
#define IDS_QSUPMEDIA       359
#define IDS_2080MB          360
#define IDS_REMOVEMED       361
#define IDS_CANTFORMATTITLE 362
#define IDS_CANTFORMAT      363

#if defined(JAPAN) && defined(i386)
//
// FMR jul.21.1994 JY
// We added 640KB/1.23MB media types.
//
#define IDS_123MB           364
#define IDS_640KB           365

/* ADD KBNES. NEC MEDIATYPE START */
#define IDS_125MB           370
#define IDS_256KB           371
#define IDS_128MB           373
/* ADD KBNES. NEC MEDIATYPE END */
#endif

#define IDS_FFERR_INCFS        400
#define IDS_FFERR_ACCESSDENIED 401
#define IDS_FFERR_DISKWP       402
#define IDS_FFERR_CANTLOCK     403
#define IDS_FFERR_CANTQUICKF   404
#define IDS_FFERR_SRCIOERR     405
#define IDS_FFERR_DSTIOERR     406
#define IDS_FFERR_SRCDSTIOERR  407
#define IDS_FFERR_GENIOERR     408
//#define IDS_FFERR_SYSFILES  409
//#define IDS_FFERR_MEDIASENSE    410
#define IDS_FFERR          411
#define IDS_FFERR_BADLABEL 412

#define IDS_OPENINGMSG          420
#define IDS_CLOSINGMSG          421
#define IDS_TOOMANYWINDOWS      422

#define IDS_QUICKFORMATTINGTITLE 423

#define IDS_INITUPDATEFAIL       424
#define IDS_INITUPDATEFAILTITLE  425
#define IDS_READING              426

#define IDS_COMPRESSDIR          427
#define IDS_UNCOMPRESSDIR        428
#define IDS_COMPRESS_ATTRIB_ERR  429
#define IDS_NTLDRCOMPRESSERR     430
#define IDS_MULTICOMPRESSERR     431

#define IDS_VERNAME_BASE          500
#define IDS_VN_COMMENTS           (IDS_VERNAME_BASE + 0)
#define IDS_VN_COMPANYNAME        (IDS_VERNAME_BASE + 1)
#define IDS_VN_FILEDESCRIPTION    (IDS_VERNAME_BASE + 2)
#define IDS_VN_INTERNALNAME       (IDS_VERNAME_BASE + 3)
#define IDS_VN_LEGALTRADEMARKS    (IDS_VERNAME_BASE + 4)
#define IDS_VN_ORIGINALFILENAME   (IDS_VERNAME_BASE + 5)
#define IDS_VN_PRIVATEBUILD       (IDS_VERNAME_BASE + 6)
#define IDS_VN_PRODUCTNAME        (IDS_VERNAME_BASE + 7)
#define IDS_VN_PRODUCTVERSION     (IDS_VERNAME_BASE + 8)
#define IDS_VN_SPECIALBUILD       (IDS_VERNAME_BASE + 9)

#define IDS_VN_LANGUAGE    (IDS_VERNAME_BASE + 10)
#define IDS_VN_LANGUAGES   (IDS_VERNAME_BASE + 11)

#define IDS_FFERROR     (800-256)
// Note that the next 256 entries are reserved for strings that will appear
// in the directory listing if there is an error reading the drive.

// These are all the ID's for the strings that may be inserted into various
// menus at init time.  Note that tbar.c depends on the order of these strings.

// was 608
#define MS_EXTRA              800
#define IDS_CONNECT           (MS_EXTRA+0)
#define IDS_DISCONNECT        (MS_EXTRA+1)
#define IDS_CONNECTIONS       (MS_EXTRA+2)
#define IDS_SHAREAS           (MS_EXTRA+3)
#define IDS_STOPSHARE         (MS_EXTRA+4)
#define IDS_SHARES            (MS_EXTRA+5)
#define IDS_UNDELETE          (MS_EXTRA+6)
#define IDS_NEWWINONCONNECT   (MS_EXTRA+7)

#define IDS_COPYERROR       1000
#define IDS_VERBS           1010
#define IDS_ACTIONS         1020
#define IDS_REPLACING       1030
#define IDS_CREATING        1031

//#define IDS_REASONS       1040    // error codes strings (range += 255)

// IDS_ from 1100 to 1199 reserved for suggestions!

JAPANBEGIN
#define IDS_KK_COPYFROMSTR              2000
#define IDS_KK_COPYTOSTR                2001
#define IDS_KK_RENAMEFROMSTR            2002
#define IDS_KK_RENAMETOSTR              2003
#define IDS_KK_COPY                     2004
#define IDS_WRNNOSHIFTJIS               2005
JAPANEND

/* This is for the menuhelp messages.  Pretty much all ID's after this should
 * be reserved for this purpose.
 */
#define MH_POPUP            (4000-16)
#define MH_MYITEMS          4000

#include "wfdlgs.h"

typedef struct _DRIVE_INFO {

   INT   iBusy;
   BOOL  bRemembered : 1;
   BOOL  bUpdating   : 1;

//-----------------------------------
   STATUSNAME(Type);
   UINT  uType;

//-----------------------------------
   INT   iOffset;

//-----------------------------------
   STATUSNAME(NetCon);
   LPWNET_CONNECTIONINFO lpConnectInfo;
   DWORD  dwConnectInfoMax;

   DWORD dwAltNameError;
   LPTSTR lpszRemoteNameMinusFour[MAX_ALTNAME];
   DWORD dwRemoteNameMax[MAX_ALTNAME];
   DWORD dwLines[MAX_ALTNAME];

//-----------------------------------
   STATUSNAME(VolInfo);
   DWORD     dwVolumeSerialNumber;
   DWORD     dwMaximumComponentLength;
   DWORD     dwFileSystemFlags;
   DWORD     dwDriveType;
   DWORD     dwVolNameMax;
   TCHAR      szVolNameMinusFour[MAX_VOLNAME+DRIVE_INFO_NAME_HEADER];
                                               // there is no easy way (+4hdr)
   TCHAR      szFileSysName[MAX_FILESYSNAME];   // to predetermine length

//-----------------------------------
   BOOL  bShareChkTried: 1;
   BOOL  bShareChkFail : 1;

   STATUSNAME(Space);
   LARGE_INTEGER  qFreeSpace;
   LARGE_INTEGER  qTotalSpace;
} DRIVEINFO, *PDRIVEINFO;

#define SC_SPLIT            100

// These errors aren't in shellapi.h as yet. till such time...
/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE      27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

#define NONE             0
#define TOOLBAR_FLAG     1
#define DRIVEBAR_FLAG    2


#ifdef _GLOBALS
#define Extern
#define EQ(x) = x
#else
#define Extern extern
#define EQ(x)
#endif



//----------------------------
//
//  Lazy load comdlg support
//
//----------------------------

#define COMDLG_DLL TEXT("comdlg32.dll")
Extern HANDLE hComdlg            EQ( NULL );

Extern DWORD (*lpfnChooseFontW)(LPCHOOSEFONTW);
Extern DWORD (*lpfnGetOpenFileNameW)(LPOPENFILENAMEW);

#define COMDLG_ChooseFontW                "ChooseFontW"
#define COMDLG_GetOpenFileNameW           "GetOpenFileNameW"

#define ChooseFontW         (*lpfnChooseFontW)
#define GetOpenFileNameW    (*lpfnGetOpenFileNameW)

//----------------------------
//
//  Lazy load network support
//
//----------------------------

#define MPR_DLL      TEXT("mpr.dll")
#define NTLANMAN_DLL TEXT("ntlanman.dll")
#define ACLEDIT_DLL  TEXT("acledit.dll")

#define WAITNET()      WaitLoadEvent(TRUE)
#define WAITACLEDIT()  WaitLoadEvent(FALSE)

#define WAITNET_DONE        bNetDone
#define WAITNET_ACLEDITDONE bNetAcleditDone

#define WAITNET_LOADED      bNetLoad
#define WAITNET_TYPELOADED  bNetTypeLoad
#define WAITNET_SHARELOADED bNetShareLoad

Extern DWORD (*lpfnWNetCloseEnum)(HANDLE);
Extern DWORD (*lpfnWNetConnectionDialog2)(HWND, DWORD, LPWSTR, UINT);
Extern DWORD (*lpfnWNetDisconnectDialog2)(HWND, UINT, LPWSTR, UINT);
Extern DWORD (*lpfnWNetEnumResourceW)(HANDLE, LPDWORD, LPVOID, LPDWORD);
Extern DWORD (*lpfnWNetGetConnection2W)(LPWSTR, WNET_CONNECTIONINFO *, LPDWORD);
Extern DWORD (*lpfnWNetGetDirectoryTypeW)(LPWSTR, LPDWORD, BOOL);
Extern DWORD (*lpfnWNetGetLastErrorW)(LPDWORD, LPWSTR, DWORD, LPWSTR, DWORD);
Extern DWORD (*lpfnWNetGetPropertyTextW)(WORD, WORD, LPWSTR, LPWSTR, WORD, WORD);
Extern DWORD (*lpfnWNetOpenEnumW)(DWORD, DWORD, DWORD, LPNETRESOURCE, LPHANDLE);
Extern DWORD (*lpfnWNetPropertyDialogW)(HWND, WORD, WORD, LPWSTR, WORD);
Extern DWORD (*lpfnWNetRestoreConnectionW)(HWND, LPWSTR);
Extern DWORD (*lpfnWNetRestoreSingleConnectionW)(HWND, LPWSTR, BOOL);
Extern DWORD (*lpfnWNetFormatNetworkNameW)(
                    LPCWSTR  lpProvider,
                    LPCWSTR  lpRemoteName,
                    LPWSTR   lpFormattedName,
                    LPDWORD  lpnLength,
                    DWORD    dwFlags,
                    DWORD    dwAveCharPerLine
                    );
Extern DWORD (*lpfnShareCreate)(HWND);
Extern DWORD (*lpfnShareStop)(HWND);

#ifdef NETCHECK
Extern DWORD (*lpfnWNetDirectoryNotifyW)(HWND, LPWSTR, DWORD);
#endif

#define NETWORK_WNetCloseEnum          "WNetCloseEnum"
#define NETWORK_WNetConnectionDialog2  "WNetConnectionDialog2"
#define NETWORK_WNetDisconnectDialog2  "WNetDisconnectDialog2"
#define NETWORK_WNetEnumResourceW      "WNetEnumResourceW"
#define NETWORK_WNetGetConnection2W    "WNetGetConnection2W"
#define NETWORK_WNetGetDirectoryTypeW  "WNetGetDirectoryTypeW"
#define NETWORK_WNetGetLastErrorW      "WNetGetLastErrorW"
#define NETWORK_WNetGetPropertyTextW   "WNetGetPropertyTextW"
#define NETWORK_WNetOpenEnumW          "WNetOpenEnumW"
#define NETWORK_WNetPropertyDialogW    "WNetPropertyDialogW"
#define NETWORK_WNetRestoreConnectionW "WNetRestoreConnectionW"
#define NETWORK_WNetRestoreSingleConnectionW "WNetRestoreSingleConnectionW"
#define NETWORK_WNetFormatNetworkNameW "WNetFormatNetworkNameW"
#define NETWORK_ShareCreate            "ShareCreate"
#define NETWORK_ShareStop              "ShareStop"

#ifdef NETCHECK
#define NETWORK_WNetDirectoryNotifyW   "WNetDirectoryNotifyW"
#endif

#define WNetCloseEnum              (*lpfnWNetCloseEnum)
#define WNetConnectionDialog2      (*lpfnWNetConnectionDialog2)
#define WNetDisconnectDialog2      (*lpfnWNetDisconnectDialog2)
#define WNetEnumResourceW          (*lpfnWNetEnumResourceW)
#define WNetGetConnection2W        (*lpfnWNetGetConnection2W)
#define WNetGetDirectoryTypeW      (*lpfnWNetGetDirectoryTypeW)
#define WNetGetLastErrorW          (*lpfnWNetGetLastErrorW)
#define WNetGetPropertyTextW       (*lpfnWNetGetPropertyTextW)
#define WNetOpenEnumW              (*lpfnWNetOpenEnumW)
#define WNetPropertyDialogW        (*lpfnWNetPropertyDialogW)
#define WNetRestoreConnectionW     (*lpfnWNetRestoreConnectionW)
#define WNetRestoreSingleConnectionW     (*lpfnWNetRestoreSingleConnectionW)
#define WNetFormatNetworkNameW     (*lpfnWNetFormatNetworkNameW)
#define ShareCreate                (*lpfnShareCreate)
#define ShareStop                  (*lpfnShareStop)

#ifdef NETCHECK
#define WNetDirectoryNotifyW       (*lpfnWNetDirectoryNotifyW)
#endif


Extern FM_EXT_PROC lpfnAcledit;

Extern HANDLE hVersion             EQ( NULL );
Extern HANDLE hMPR                 EQ( NULL );
Extern HANDLE hNTLanman            EQ( NULL );
Extern HANDLE hAcledit             EQ( NULL );


//--------------------------------------------------------------------------
//
//  Global Externs
//
//--------------------------------------------------------------------------

Extern HANDLE  hEventNetLoad              EQ( NULL );
Extern HANDLE  hEventAcledit              EQ( NULL );
Extern BOOL    bNetLoad                   EQ( FALSE );
Extern BOOL    bNetTypeLoad               EQ( FALSE );
Extern BOOL    bNetShareLoad              EQ( FALSE );
Extern BOOL    bNetDone                   EQ( FALSE );
Extern BOOL    bNetAcleditDone            EQ( FALSE );

#if defined(JAPAN) && defined(i386) // cf. DBCS
#include "machine.h"
Extern DWORD   gdwMachineId               EQ( MACHINEID_MICROSOFT );
#endif // defined(JAPAN) && defined(JAPAN)

//----------------------------
//
//  aDriveInfo support
//
//----------------------------

#define rgiDrive rgiDriveReal[iUpdateReal]

Extern INT       iUpdateReal              EQ( 0 );
Extern DRIVE     rgiDriveReal[2][26];
Extern DRIVEINFO aDriveInfo[26];

Extern UINT   uMenuID;
Extern HMENU  hMenu;
Extern UINT   uMenuFlags;
Extern BOOL   bMDIFrameSysMenu;

Extern ATOM atomDirListBox;

Extern PPDOCBUCKET ppDocBucket;
Extern PPDOCBUCKET ppProgBucket;

Extern CRITICAL_SECTION CriticalSectionPath;

Extern LCID   lcid;

JAPANBEGIN
Extern BOOL   bJapan      EQ( FALSE );
JAPANEND

Extern BOOL bMinOnRun     EQ( FALSE );
Extern BOOL bStatusBar    EQ( TRUE );

Extern BOOL bDriveBar        EQ( TRUE );
Extern BOOL bToolbar         EQ( TRUE );
Extern BOOL bNewWinOnConnect EQ( TRUE );

Extern BOOL bExitWindows     EQ( FALSE );
Extern BOOL bConfirmDelete   EQ( TRUE );
Extern BOOL bConfirmSubDel   EQ( TRUE );
Extern BOOL bConfirmReplace  EQ( TRUE );
Extern BOOL bConfirmMouse    EQ( TRUE );
Extern BOOL bConfirmFormat   EQ( TRUE );
Extern BOOL bConfirmReadOnly EQ( TRUE );

Extern BOOL bSaveSettings   EQ( TRUE );

Extern BOOL bConnectable       EQ( FALSE );
Extern BOOL fShowSourceBitmaps EQ( TRUE );
Extern BOOL bFSCTimerSet       EQ( FALSE );

Extern TCHAR        chFirstDrive;           // 'A' or 'a'

Extern TCHAR        szExtensions[]          EQ( TEXT("Extensions") );
Extern TCHAR        szFrameClass[]          EQ( TEXT("WFS_Frame") );
Extern TCHAR        szTreeClass[]           EQ( TEXT("WFS_Tree") );
Extern TCHAR        szDrivesClass[]         EQ( TEXT("WFS_Drives") );
Extern TCHAR        szTreeControlClass[]    EQ( TEXT("DirTree") );
Extern TCHAR        szDirClass[]            EQ( TEXT("WFS_Dir") );
Extern TCHAR        szSearchClass[]         EQ( TEXT("WFS_Search") );

Extern TCHAR        szDriveBar[]            EQ( TEXT("DriveBar") );
Extern TCHAR        szToolbar[]             EQ( TEXT("ToolBar") );
Extern TCHAR        szNewWinOnNetConnect[]  EQ( TEXT("NewWinOnNetConnect") );

Extern TCHAR        szMinOnRun[]            EQ( TEXT("MinOnRun") );
Extern TCHAR        szStatusBar[]           EQ( TEXT("StatusBar") );
Extern TCHAR        szSaveSettings[]        EQ( TEXT("Save Settings") );

Extern TCHAR        szConfirmDelete[]       EQ( TEXT("ConfirmDelete") );
Extern TCHAR        szConfirmSubDel[]       EQ( TEXT("ConfirmSubDel") );
Extern TCHAR        szConfirmReplace[]      EQ( TEXT("ConfirmReplace") );
Extern TCHAR        szConfirmMouse[]        EQ( TEXT("ConfirmMouse") );
Extern TCHAR        szConfirmFormat[]       EQ( TEXT("ConfirmFormat") );
Extern TCHAR        szConfirmReadOnly[]     EQ( TEXT("ConfirmSystemHiddenReadOnly") );

Extern TCHAR        szDriveListFace[]          EQ( TEXT("DriveListFace") );

Extern TCHAR        szChangeNotifyTime[]    EQ( TEXT("ChangeNotifyTime") );
Extern UINT         uChangeNotifyTime       EQ( 3000 );

Extern TCHAR        szDirKeyFormat[]        EQ( TEXT("dir%d") );
Extern TCHAR        szWindow[]              EQ( TEXT("Window") );
Extern TCHAR        szWindows[]             EQ( TEXT("Windows") );


Extern TCHAR        szFace[]                EQ( TEXT("Face") );
Extern TCHAR        szSize[]                EQ( TEXT("Size") );
Extern TCHAR        szLowerCase[]           EQ( TEXT("LowerCase") );
Extern TCHAR        szFaceWeight[]          EQ( TEXT("FaceWeight") );

JAPANBEGIN
Extern TCHAR        szSaveCharset[]         EQ( TEXT("Charset") );
JAPANEND

Extern TCHAR        szAddons[]              EQ( TEXT("AddOns") );
Extern TCHAR        szUndelete[]            EQ( TEXT("UNDELETE.DLL") );

Extern TCHAR        szDefPrograms[]         EQ( TEXT("EXE COM BAT PIF") );
Extern TCHAR        szRoamINIPath[]         EQ( TEXT("\\Microsoft\\Winfile"));
Extern TCHAR        szBaseINIFile[]         EQ( TEXT("WINFILE.INI") );
Extern TCHAR        szPrevious[]            EQ( TEXT("Previous") );
Extern TCHAR        szSettings[]            EQ( TEXT("Settings") );
Extern TCHAR        szInternational[]       EQ( TEXT("Intl") );
Extern TCHAR        szStarDotStar[]         EQ( TEXT("*.*") );
Extern TCHAR        szNULL[]                EQ( TEXT("") );
Extern TCHAR        szBlank[]               EQ( TEXT(" ") );
Extern TCHAR        szEllipses[]            EQ( TEXT("...") );
Extern TCHAR        szNetwork[]             EQ( TEXT("Network") );
Extern TCHAR        szSpace[]               EQ( TEXT(" ") );
Extern TCHAR        szDirsRead[32];
Extern TCHAR        szCurrentFileSpec[14]   EQ( TEXT("*.*") );

Extern TCHAR        szComma[4]      EQ( TEXT(",") );
Extern TCHAR        szDecimal[4]    EQ( TEXT(".") );

Extern TCHAR        szListbox[]     EQ( TEXT("ListBox") );        // window style

Extern TCHAR        szTitle[128];

Extern TCHAR        szMessage[MAXMESSAGELEN];

Extern TCHAR        szStatusTree[80];
Extern TCHAR        szStatusDir[80];

Extern TCHAR        szOriginalDirPath[MAXPATHLEN]; // was OEM string!!!!!!

Extern TCHAR        szTheINIFile[MAXPATHLEN];		// ini file location in %APPDATA%

Extern TCHAR szBytes[20];
Extern TCHAR szSBytes[10];

Extern INT  cDrives;
Extern INT  dxDrive;
Extern INT  dyDrive;
Extern INT  dxDriveBitmap;
Extern INT  dyDriveBitmap;
Extern INT  dxEllipses;
Extern INT  dxFolder;
Extern INT  dyFolder;
Extern INT  dyBorder;       // System Border Width/Height
Extern INT  dyBorderx2;     // System Border Width/Height * 2
Extern INT  dxText;         // System Font Width 'M'
Extern INT  dyText;         // System Font Height
Extern INT  cchDriveListMax; // ave # chars in drive list
Extern INT  dyIcon          EQ( 32 );
Extern INT  dxIcon          EQ( 32 );

Extern INT  dyFileName;
Extern INT  nFloppies;       // Number of Removable Drives

Extern INT   iSelHilite     EQ( -1 );

Extern INT   cDisableFSC    EQ( 0 );     // has fsc been disabled?
Extern INT   iReadLevel     EQ( 0 );     // global.  if !0 someone is reading a tree
Extern INT   dxFrame;
Extern INT   dxClickRect;
Extern INT   dyClickRect;
Extern INT   iNumWindows     EQ( 0 );

Extern INT   dyToolbar        EQ( 27 );
Extern INT   dxButtonSep      EQ( 8 );
Extern INT   dxButton         EQ( 24 );
Extern INT   dyButton         EQ( 22 );
Extern INT   dxDriveList      EQ( 205 );
Extern INT   dyDriveItem      EQ( 17 );
Extern INT   xFirstButton;
Extern HFONT hfontDriveList;
Extern HFONT hFont;
Extern HFONT hFontStatus;

Extern HANDLE hfmifsDll EQ( NULL );

Extern HANDLE  hAccel            EQ( NULL );
Extern HINSTANCE  hAppInstance;
Extern HANDLE  hModUndelete      EQ( NULL );

Extern HBITMAP  hbmBitmaps         EQ( NULL );
Extern HDC  hdcMem                 EQ( NULL );

Extern INT  iCurDrag  EQ( 0 );

Extern HICON    hicoTree      EQ( NULL );
Extern HICON    hicoTreeDir   EQ( NULL );
Extern HICON    hicoDir       EQ( NULL );

Extern HWND    hdlgProgress;
Extern HWND    hwndFrame       EQ( NULL );
Extern HWND    hwndMDIClient   EQ( NULL );
Extern HWND    hwndSearch      EQ( NULL );
Extern HWND    hwndDragging    EQ( NULL );

Extern HWND  hwndDriveBar      EQ( NULL );
Extern HWND  hwndToolbar       EQ( NULL );
Extern HWND  hwndDriveList     EQ( NULL );
Extern HWND  hwndDropChild     EQ( NULL );  // for tree windows forwarding to drivebar

Extern BOOL bCancelTree;

Extern WORD wTextAttribs       EQ( 0 );
Extern DWORD dwSuperDlgMode;

Extern UINT wHelpMessage;
Extern UINT wBrowseMessage;


//
// Warning: When this is set, creating a directory window
// will cause this file spec to be selected.  This must be
// alloc'd and freed by the callee.  It then must be set
// to null before the dir window is called again.
//
Extern LPWSTR pszInitialDirSel;
Extern DWORD dwNewView         EQ( VIEW_NAMEONLY );
Extern DWORD dwNewSort         EQ( IDD_NAME );
Extern DWORD dwNewAttribs    EQ( ATTR_DEFAULT );



Extern LARGE_INTEGER qFreeSpace;
Extern LARGE_INTEGER qTotalSpace;

Extern HWND hwndStatus        EQ( NULL );

Extern TCHAR szWinfileHelp[]  EQ( TEXT("WINFILE.HLP") );
Extern WCHAR wszWinfileHelp[] EQ( L"WINFILE.HLP" );

Extern INT iNumExtensions     EQ( 0 );
Extern EXTENSION extensions[MAX_EXTENSIONS];

Extern DWORD (*lpfpUndelete)(HWND, LPTSTR) EQ( NULL );

#ifdef UNICODE
Extern BOOL  bUndeleteUnicode EQ( FALSE );
#endif

Extern HHOOK hhkMsgFilter     EQ( NULL );

Extern DWORD dwContext       EQ( 0 );
Extern DWORD nLastDriveInd   EQ( 0 );
Extern DWORD fFormatFlags    EQ( 0 );
Extern TCHAR szFmifsDll[]    EQ( TEXT("fmifs.dll") );

Extern   CANCEL_INFO CancelInfo;
Extern   SEARCH_INFO SearchInfo;

#ifdef _GLOBALS
   DWORD dwMenuIDs[] = {
      MH_MYITEMS, MH_POPUP,
      MH_POPUP+IDM_WINDOW, 0,   // The 0's are placeholders for menu handles
      MH_POPUP+IDM_HELP, 0,
      0, 0                      // We need to NULL terminate this list
   };
#else
   Extern DWORD dwMenuIDs[];
#endif

#if 0
Extern TCHAR szReplace[];
Extern TCHAR szCurrentView[];
Extern TCHAR szCurrentSort[];
Extern TCHAR szCurrentAttribs[];
Extern TCHAR szTreeKey[];
Extern TCHAR szDated[];
Extern TCHAR szWith[];

Extern TCHAR szNTlanman[];
Extern INT  cKids;
Extern INT  dxBraces;
Extern INT  dyTitle;

Extern INT nCopyMaxQueue;
#endif

#undef Extern
#undef EQ


