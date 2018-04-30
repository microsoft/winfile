#pragma once

/// <summary>
/// this is where we keep reusable solutions
/// the C interface
/// </summary>


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

						/*
						use string comparisons bellow to combat:
						warning C6400:
						Using 'lstrcmpi' to perform a case-insensitive compare
						Yields unexpected results in non-English locales.
						*/
	int winfile_exact_string_compareA(const char * str1, const char * str2, unsigned char CaseInSensitive);
	int winfile_exact_string_compareW(const wchar_t * str1, const wchar_t * str2, unsigned char CaseInSensitive);

#if _UNICODE
#define winfile_exact_string_compare binary_string_compareW
#else
#define winfile_exact_string_compare binary_string_compareA
#endif

	int winfile_ui_string_compare(LPCTSTR str1, LPCTSTR str2, unsigned char ignore_case);

#ifdef __cplusplus
} // extern "C" 
#endif /* __cplusplus */