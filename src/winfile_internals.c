/*
2018-04-30	dbj@dbj.org		created
*/
#include <windows.h>
#include <ntddkbd.h>
#include "winfile_internals.h"

/// <summary>
/// non-linguistic byte level comparison of strings
/// note: RtlCompare[Unicode]String is called from CompareString[Ex]
/// </summary>
int winfile_exact_string_compareA(const char * str1, const char * str2, unsigned char CaseInSensitive) {
	return RtlCompareString(
		str1, str2, CaseInSensitive
	);
}

int winfile_exact_string_compareW(const wchar_t * str1, const wchar_t * str2, unsigned char CaseInSensitive) {
	return RtlCompareUnicodeString(
		str1, str2, CaseInSensitive
	);
}

/// <summary>
/// this is legacy aka "pre vista" solution 
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
