/********************************************************************

   lfnmisc.c

   Long filename support for windows:  miscellaneous functions

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include <stdlib.h>

#include "winfile.h"
#include "lfn.h"


#define FIL_STANDARD      1

// Local Function prototypes
DWORD I_Is83File( LPTSTR lpFile ) ;

/****************************************************************************
 *
 *  I_LFNCanon()
 *
 *  Purpose:  basic canonicalization routine for LFN names.
 *
 *  Input:
 *       CanonType   Type of canonization required
 *       InFile      points to a file name
 *
 *  Output:
 *       OutFile     receives canonicalized name.
 *
 *  Returns:
 *       0                    If filename has been canonicalized
 *       INVALID_PARAMETER    If filename has invalid characters
 *
 ****************************************************************************/
DWORD
I_LFNCanon(WORD CanonType, LPTSTR InFile, LPTSTR OutFile)
{
   LPTSTR       ins = InFile;
   LPTSTR       outs = OutFile;
   unsigned    size;
   unsigned    trails;
   TCHAR        c;

   if (!InFile || !OutFile)
      return ERROR_INVALID_PARAMETER;

   if (*InFile == CHAR_NULL)
      return ERROR_INVALID_PARAMETER;


   /* First, check if we have a fully qualified name, or a relative name */
   if (*InFile != CHAR_BACKSLASH && *InFile != CHAR_SLASH) {
      if (isalpha(*InFile)) {
         if (InFile[1] == CHAR_COLON) {
            *outs++ = *ins++;
            *outs++ = *ins++;     // Copy over the drive and colon
         }
      }
   } else {
      ins++;
      *outs++ = CHAR_BACKSLASH;
   }

   /*  Note:  When calculating size, we ignore the d:\ that may appear.  This
    *  is because that is not considered part of the path by OS/2.  IT IS THE
    *  RESPONSIBILITY OF THE CALLING FUNCTION TO RESERVE ENOUGH SPACE FOR THE
    *  CANON NAME, WHICH CAN BE EITHER 256 or 260.
    */

   size = 0;
   do {
      c = *ins++;
      if ((c < 0x001f && c != CHAR_NULL) || (c == CHAR_DQUOTE) || (c == CHAR_COLON) || (c == CHAR_PIPE) ||
          (c == CHAR_GREATER) || (c == CHAR_LESS)) {
             *OutFile = CHAR_NULL;
             return ERROR_INVALID_PARAMETER;
      }
      if (CanonType != LFNCANON_MASK && ((c == CHAR_STAR) || (c == CHAR_QUESTION))) {
         *OutFile = CHAR_NULL;
         return ERROR_INVALID_PARAMETER;
      }
      if (c == CHAR_SLASH)
         c = CHAR_BACKSLASH;       // Convert / to \ for canon

      if (c == CHAR_BACKSLASH || c == CHAR_NULL) {   // Component separator:  Trim file name

         /*  We have to special case . and ..   . we resolve, but .. we pass
          *  through.
          */
         if (outs > OutFile) {
            if (*(outs - 1) == CHAR_DOT) {
               if  ((outs - 1) == OutFile || *(outs - 2) == CHAR_BACKSLASH) {   // Single dot
                  *(outs--) = c;
                  if (size)
                     size--;
                  continue;
               }
               if (*(outs - 2) == CHAR_DOT) {     // Possible ..
                  if ((outs - 2) == OutFile || *(outs - 3) == CHAR_BACKSLASH ||
                        *(outs - 3) == CHAR_COLON) {
                     *outs++ = c;
                     size++;
                     continue;
                  }
               }
            }
         }

         /*  Okay, the component is neither a . nor a .. so we go into trim
          *  trailing dots and spaces mode.
          */
         trails = 0;
         while (outs > OutFile && ((*(outs-1) == CHAR_DOT || *(outs-1) == CHAR_SPACE)
                 && (*(outs-1) != CHAR_BACKSLASH && *(outs - 1) != CHAR_COLON)) ) {
            outs--;
            trails++;
            if (size)
               size--;
         }
         if (outs == OutFile) {
            *OutFile = CHAR_NULL;
            return ERROR_INVALID_PARAMETER;
         }
         if (outs > OutFile && *(outs-1) == CHAR_BACKSLASH) {
            *OutFile = CHAR_NULL;
            return ERROR_INVALID_PARAMETER;
         }
      }
      *outs++ = c;
      size++;
      if (size > CCHMAXPATHCOMP) {
         *OutFile = CHAR_NULL;
         return ERROR_INVALID_PARAMETER;
      }
   } while (c);

   /*  Last check:  We don't allow paths to end in \ or /.  Since / has been
    *  mapped to \, we just check for \
    */
   if (outs != OutFile)
      if (*(outs-1) == CHAR_BACKSLASH) {
         *OutFile = CHAR_NULL;
         return ERROR_INVALID_PARAMETER;
      }

   return 0;

}

// Manifests local to LFNParse
#define FAT_FILE_COMP_LENGTH    8
#define FAT_FILE_EXT_LENGTH     3
#define DRIVE_COLON             CHAR_COLON
#define PATH_SEPARATOR          CHAR_BACKSLASH
#define DOT                     CHAR_DOT

static TCHAR ach83InvalidChars[] = TEXT("<>|[]=;+/, \"\x01\x02\x03\x04\x05\x06\x07\
\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\
\x1b\x1c\x1d\x1e\x1f") ;


/*****************************************************************************
 *
 *  I_LFNEditName
 *
 *  Purpose:    Given a file and a mask, I_LFNCombine will combine the two
 *      according to the rules stated in the OS/2 function DosEditName.
 *
 *  Entry:
 *   lpSrc    Pointer to file
 *   lpEd   Pointer to edit mask
 *   lpRes    Results of combining lpMask & lpFile
 *   uResBuffSize  Size of the result buffer
 *
 *  Exit:
 *
 *   lpResult contains the combined lpFile and lpMask.
 *
 *
 *  Return Codes:
 *   The return code is NO_ERROR if successful or:
 *      ERROR_INVALID_PARAMETER  A malformed name was passed or the result
 *             buffer was too small.
 *
 *****************************************************************************/

WORD I_LFNEditName( LPTSTR lpSrc, LPTSTR lpEd, LPTSTR lpRes, INT iResBufSize )
{
   INT ResLen = 0;     // Length of result

// This is turned off until we agree
// that cmd operates in the same way

#ifdef USELASTDOT
   LPTSTR lpChar;

   //
   // We have a special case hack for the dot, since when we do a
   // rename from foo.bar.baz to *.txt, we want to use the last dot, not
   // the first one (desired result = foo.bar.txt, not foo.txt)
   //
   // We find the dot (GetExtension rets first char after last dot if there
   // is a dot), then if the delimiter for the '*' is a CHAR_DOT, we continue
   // copying until we get to the last dot instead of finding the first one.
   //

   lpChar = GetExtension(lpSrc);
   if (*lpChar) {
      lpChar--;
   } else {
      lpChar = NULL;
   }
#endif

   while (*lpEd) {

      if (ResLen < iResBufSize) {

         switch (*lpEd) {

         case CHAR_STAR:
            {
               TCHAR delimit = *(lpEd+1);

#ifdef USELASTDOT

               //
               // For all other delimiters, we use the first
               // occurrence (e.g., *f.txt).
               //
               if (CHAR_DOT != delimit)
                  lpChar = NULL;

               while ((ResLen < iResBufSize) && ( *lpSrc != CHAR_NULL ) &&
                  ( *lpSrc != delimit || (lpChar && lpChar != lpSrc ))) {
#else
               while ((ResLen < iResBufSize) &&
                  ( *lpSrc != CHAR_NULL ) && ( *lpSrc != delimit )) {
#endif

                  *(lpRes++) = *(lpSrc++);
                  ResLen++;

               }
            }
            break;


         case CHAR_QUESTION:
            if ((*lpSrc != DOT ) && (*lpSrc != CHAR_NULL)) {

               if (ResLen < iResBufSize) {

                  *(lpRes++) = *(lpSrc++);
                  ResLen++;
               }
               else
                  return ERROR_INVALID_PARAMETER ;
            }
            break;

         case CHAR_DOT:
            while ((*lpSrc != DOT ) && (*lpSrc != CHAR_NULL))
               lpSrc++;

            *(lpRes++) = DOT ;       // from EditMask, even if src doesn't
                                     // have one, so always put one.
            ResLen++;
            if (*lpSrc)              // point one past CHAR_DOT
               lpSrc++;
               break;

         default:
            if ((*lpSrc != DOT ) && (*lpSrc != CHAR_NULL)) {

               lpSrc++;
            }

            if (ResLen < iResBufSize) {

               *(lpRes++) = *lpEd;
               ResLen++;
            }
            else
               return ERROR_INVALID_PARAMETER ;
            break;
         }
         lpEd++;

      }
      else {

         return ERROR_INVALID_PARAMETER ;
      }
   }

   if ((ResLen) < iResBufSize) {
      *lpRes = CHAR_NULL;
      return NO_ERROR ;
   }
   else
      return ERROR_INVALID_PARAMETER ;
}

