/********************************************************************

   dbg.c

   Debugging utilities

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"
#include "dbg.h"

#if DBG

TCHAR szAsrtFmt[] = TEXT("Assertion Failure in %s at %d\n");
TCHAR szFmt[300];
unsigned long TraceFlags = 
      BF_START
//     | BF_PROCTRACE
//     | BF_PARMTRACE
//     | BF_MSGTRACE
//     | BF_DEFMSGTRACE
    ;     // set these to print on TRACEs

unsigned long BreakFlags = 0
    // | BF_START
    ;     // set these to break on TRACEs

VOID DbgAssert(
LPTSTR file,
int line)
{
	 wsprintf(szFmt,szAsrtFmt,file,line);
	 OutputDebugString(szFmt);
//    DbgPrint(szAsrtFmt, file, line);
    DebugBreak();
}


VOID DbgTrace(
DWORD tf,
LPTSTR lpstr)
{
    if (tf & TraceFlags) {
	 wsprintf(szFmt,TEXT("%s\n"),lpstr);
	 OutputDebugString(szFmt);
//        DbgPrint("%s\n", lpstr);
        if (tf & BreakFlags) {
            DebugBreak();
        }
    }
}


VOID DbgBreak(
DWORD bf,
LPTSTR file,
int line)
{
    if (bf & BreakFlags) {
	 wsprintf(szFmt,TEXT("BREAK at %s:%d\n"), file, line);
	 OutputDebugString(szFmt);
//        DbgPrint("BREAK at %s:%d\n", file, line);
        DebugBreak();
    }
}


VOID DbgPrint1(
DWORD tf,
LPTSTR fmt,
LPTSTR p1)
{
    if (tf & TraceFlags) {
	 wsprintf(szFmt,fmt, p1);
	 OutputDebugString(szFmt);
//	 DbgPrint("%d",GetTickCount());
	 OutputDebugString(TEXT("\n"));
//        DbgPrint(fmt, p1);
//        DbgPrint("\n");
    }
    if (tf & BreakFlags) {
        DebugBreak();
    }
}


VOID DbgEnter(
LPTSTR funName)
{
    DbgPrint1(BF_PROCTRACE, TEXT("> %s "), funName);
}

VOID DbgLeave(
LPTSTR funName)
{
    DbgPrint1(BF_PROCTRACE, TEXT(" <%s "), funName);
}


VOID DbgTraceMessage(
LPTSTR funName,
LPTSTR msgName)
{
    if (BF_MSGTRACE & TraceFlags) {
	 wsprintf(szFmt,TEXT("MSG: %s - %s\n"), funName, msgName);
	 OutputDebugString(szFmt);
//        DbgPrint("MSG: %s - %s\n", funName, msgName);
    }
    if (BF_MSGTRACE & BreakFlags) {
        DebugBreak();
    }
}

VOID DbgTraceDefMessage(
LPTSTR funName,
WORD msgId)
{
    if (BF_DEFMSGTRACE & TraceFlags) {
	 wsprintf(szFmt,TEXT("MSG: %s - default(0x%x)\n"), funName, msgId);
	 OutputDebugString(szFmt);
//        DbgPrint("MSG: %s - default(0x%x)\n", funName, msgId);
    }
    if (BF_DEFMSGTRACE & BreakFlags) {
        DebugBreak();
    }
}

#endif // DBG
