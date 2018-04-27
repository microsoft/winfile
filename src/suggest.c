/********************************************************************

   Suggest.c

   Handles error code messages and suggestions

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

//
// Create array
//
#define SUGGEST(id,err,flags,str) err, flags, id+IDS_SUGGESTBEGIN,
DWORD adwSuggest[][3] = {
#include "lang\suggest_en-US.db"
   0,0,0
};
#undef SUGGEST


/////////////////////////////////////////////////////////////////////
//
// Name:     FormatError
//
// Synopsis: Takes an error code and adds explanation/suggestions
//
// IN        bNullString --     lpBuf is empty?
// INOUT     lpBuf      LPTSTR  buffer with initial string
//                              !! Should end in "  " or '\n' for formatting !!
//                              lpBuf[0] must be 0 if no initial string
//
// IN        iSize      --      size of buffer _in_characters_
// IN        dwError    --      Error code from GetLastError()
//
// Return:   DWORD      # characters added
//
// Assumes:  iError has DE_BIT off for system errors
//           lpBuf[0] if no initial string
//
// Effects:  lpBuf appended with text
//
// Notes:    FormatMessage usually takes lpArgList for any text
//           substitutions.  This blocks out _all_ present strings
//           that would normally require lpArgList != NULL.
//
/////////////////////////////////////////////////////////////////////

DWORD
FormatError(
   BOOL bNullString,
   LPTSTR lpBuf,
   INT iSize,
   DWORD dwError)
{
   INT iLen;
   DWORD dwNumChars = 0;
   PDWORD pdwSuggest;
   INT iAddNewline = 0;

   WORD wLangId;
   BOOL bTryAgain;

   //
   // If error == 0, just return...
   //
   if (!dwError)
      return 0;

   if (bNullString)
      lpBuf[0] = 0;

   iLen = lstrlen(lpBuf);

   lpBuf += iLen;
   iSize -= iLen;

   if (iSize <=0)
      return 0;

   // Check suggestion flags before calling FormatMessage
   // in case we want to use our own string.

   pdwSuggest = FormatSuggest( dwError );

   //
   // Only do a FormatMessage if the DE_BIT is off and the
   // SUG_IGN_FORMATMESSAGE bit is off.
   // (If no suggestion, default use format message)
   //

   if ( !(dwError & DE_BEGIN) &&
      !(pdwSuggest && pdwSuggest[1] & SUG_IGN_FORMATMESSAGE) ) {

      // if extended error, use WNetErrorText!
      if ( ERROR_EXTENDED_ERROR == dwError ) {
         DWORD dwErrorCode;
         TCHAR szProvider[128];


         if (WAITNET_LOADED) {

            // !! BUG: szProvider size hard coded, doesn't print provider !!

            WNetGetLastError( &dwErrorCode, lpBuf, iSize, szProvider,
               COUNTOF(szProvider));
         }

         return lstrlen(lpBuf);
      }

      //
      // Begin with language from thread.
      //
      // loop again only if the there is an error,
      //    the error is that the resource lang isn't found,
      //    and we are not using the neutral language.
      //
      // If so, repeat query using neutral language.
      //
      wLangId = LANGIDFROMLCID(lcid);

      do {
         dwNumChars = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM |
               FORMAT_MESSAGE_IGNORE_INSERTS |
               FORMAT_MESSAGE_MAX_WIDTH_MASK,
            NULL, dwError, wLangId,
            lpBuf, iSize*sizeof(lpBuf[0]), NULL );

         bTryAgain = !dwNumChars &&
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL) != wLangId &&
            ERROR_RESOURCE_LANG_NOT_FOUND == GetLastError();

         wLangId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

      } while (bTryAgain);

      iAddNewline = 2;
   }

   //
   // if !dwNumChars, NULL terminate for safety.
   //
   if (!dwNumChars) {
      lpBuf[0] = CHAR_NULL;
   }

   //
   // Now add suggestion if it exists.
   //
   if (pdwSuggest && pdwSuggest[2]) {

      DWORD dwNumTemp = 0;

      // Make sure we have space:

      lpBuf += dwNumChars + iAddNewline;
      iSize -= dwNumChars + iAddNewline;

      if (!iSize)
         goto SuggestPunt;

      //
      // We found one, add a new line in for formatting
      //

      for(;iAddNewline; iAddNewline--)
         lpBuf[-iAddNewline] = CHAR_NEWLINE;

      dwNumTemp = LoadString ( hAppInstance, pdwSuggest[2], lpBuf, iSize );

      return dwNumTemp+dwNumChars+iAddNewline;
   }

SuggestPunt:

   // if dwNumChars != 0 then just make sure last char
   // isn't \n.  if it is, strip it!

   if ( dwNumChars ) {
      if ( CHAR_NEWLINE == lpBuf[dwNumChars-1] ) {
         lpBuf[dwNumChars-1] = CHAR_NULL;
      }

      if ( dwNumChars > 1 ) {
         if ( 0x000D == lpBuf[dwNumChars-2] ) {
            lpBuf[dwNumChars-2] = CHAR_NULL;
         }
      }
   }

   return dwNumChars;
}

PDWORD
FormatSuggest( DWORD dwError )
{
   PDWORD pdwReturn = NULL;
   INT i;

   // If error == 0, just return...
   if (!dwError)
      return NULL;

   // scan through all suggests

   switch(dwError) {
   case ERROR_EXE_MARKED_INVALID:
   case ERROR_ITERATED_DATA_EXCEEDS_64k:
   case ERROR_INVALID_STACKSEG:
   case ERROR_INVALID_STARTING_CODESEG:
   case ERROR_INVALID_MODULETYPE:
   case ERROR_INVALID_MINALLOCSIZE:
   case ERROR_INVALID_SEGDPL:
   case ERROR_RELOC_CHAIN_XEEDS_SEGLIM:
   case ERROR_INFLOOP_IN_RELOC_CHAIN:
      dwError = ERROR_INVALID_ORDINAL;

      // no break

   default:
      for (i=0;adwSuggest[i][0]; i++) {
         if ( adwSuggest[i][0] == dwError ) {
            pdwReturn = adwSuggest[i];
            break;
         }
      }
   }
   return pdwReturn;
}
