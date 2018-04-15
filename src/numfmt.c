/********************************************************************

   NumFmt.c

   Number formatting code

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "winfile.h"

static DWORD pdwOrders[] =
{
   IDS_ORDERB, IDS_ORDERKB, IDS_ORDERMB, IDS_ORDERGB, IDS_ORDERTB
} ;

static TCHAR szFormat[] = TEXT("%s%02d");
extern TCHAR szInternational[];

//
// atoi with decimal comma separators
//
LPTSTR
ShortSizeFormatInternal(LPTSTR szBuf, LARGE_INTEGER qw)
{
   INT i;
   UINT uInt, uLen, uDec;
   TCHAR szTemp[10], szOrder[20];

   LARGE_INTEGER qConstant;
   DWORD dw;

   qConstant.QuadPart = 1024L;

   if (qw.QuadPart < qConstant.QuadPart)
   {
      wsprintf(szTemp, SZ_PERCENTD, LOWORD(qw.LowPart));
      i = 0;
      goto AddOrder;
   }

   qConstant.QuadPart = (1024L * 1000L - 1);

   for ( i = 1;
         (qw.QuadPart > qConstant.QuadPart);
		 qw.QuadPart = qw.QuadPart / 1024L, ++i)

      /* do nothing */
      ;

   dw = qw.LowPart;

   uInt = (UINT) (dw / 1024);
   uLen = wsprintf(szTemp, SZ_PERCENTD, uInt);
   if (uLen < 3) {
      uDec = (dw - uInt * 1024L) * 1000 / 1024;

      //
      // At this point, uDec should be between 0 and 1000
      // we want get the top one (or two) digits.
      //
      uDec /= 10;
      if (uLen == 2) {
         uDec /= 10;
      }

      //
      // Note that we need to set the format before getting the
      // intl CHAR.
      //
      szFormat[4] = CHAR_ZERO + 3 - uLen;
      uLen += wsprintf(szTemp+uLen, szFormat, szDecimal, uDec);
   }

AddOrder:
   LoadString(hAppInstance, pdwOrders[i], szOrder, COUNTOF(szOrder));
   wsprintf(szBuf, szOrder, szTemp);

   return(szBuf);
}


LARGE_INTEGER
TriMultiply(DWORD dw1, DWORD dw2, DWORD dwSmall)
{
   LARGE_INTEGER Result;

   Result.QuadPart = UInt32x32To64(dw1, dw2);
   Result.QuadPart = Result.QuadPart * dwSmall;

   return Result;
}
