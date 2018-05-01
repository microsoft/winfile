/*
2018-04-30	dbj@dbj.org		created
*/
#include <windows.h>
// #include <ntdef.h>
#include "winfile_internals.h"

/// <summary>
/// non-linguistic byte level comparison of strings
/// note: RtlCompare[Unicode]String is now part of WDK and is thus
/// not a feasible solution for legacy code
/// Thus what seems as a cludge is to use CompareString in a locale invariant mode
/// alernative is to develop a solution using strcmp() and friends ...
/// So this is not a ordinal comparison but only for ASCI build of winfile
/// which is if not non existent than very rare variant of winfile
/// </summary>
int winfile_ordinal_string_compareA(const char * str1, const char * str2, unsigned char ignore_case) {

	return CompareStringA(
		/* _In_ LCID    */ LOCALE_INVARIANT,
		/* _In_ DWORD   */ ignore_case == 1 ? LINGUISTIC_IGNORECASE : NORM_LINGUISTIC_CASING,
		/* _In_ LPCTSTR */ str1,
		/* _In_ int     */ -1,
		/* _In_ LPCTSTR */ str2,
		/* _In_ int     */ -1);
}

/// <summary>
/// CompareStringOrdinal simply does not exist for asci strings
/// which is telling, in the context of one will never
/// need it, because one will not build non unicode apps 
/// from XP onwards that is
/// </summary>
int winfile_ordinal_string_compareW(const wchar_t * str1, const wchar_t * str2, unsigned char ignore_case) {
	return CompareStringOrdinal(
		str1, -1, str2, -1, ignore_case
	);
}

/// <summary>
/// This is to be used for "UI sting comparisons" or sorting and a such
/// this is locale sensitive, legacy aka "pre vista" solution 
/// when locales had ID's not names
/// and when CompareStringEx was not
/// </summary>
int winfile_ui_string_compare(LPCTSTR str1, LPCTSTR str2, unsigned char ignore_case)
{
	/// <summary>
	/// it is highly unlikely the user will change the
	/// locale between the calls, so we could have this
	/// as static 
	/// </summary>
	/* LCID aka DWORD */ unsigned long
		current_locale = GetUserDefaultLCID(); // not LOCALE_INVARIANT

	return CompareString(
		/* _In_ LCID    */ current_locale,
		/* _In_ DWORD   */ ignore_case == 1 ? LINGUISTIC_IGNORECASE : NORM_LINGUISTIC_CASING,
		/* _In_ LPCTSTR */ str1,
		/* _In_ int     */ -1,
		/* _In_ LPCTSTR */ str2,
		/* _In_ int     */ -1
	);
}
