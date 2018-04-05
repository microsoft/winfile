/********************************************************************

   numfmt.h

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#define SPC_SET_INVALID(q) ((q).HighPart=(LONG)-1)
#define SPC_SET_NOTREE(q)  ((q).HighPart=(LONG)-2)
#define SPC_SET_HITDISK(q) ((q).HighPart=(LONG)-3)

#define SPC_IS_INVALID(q) ((q).HighPart==(LONG)-1)
#define SPC_IS_NOTREE(q)  ((q).HighPart==(LONG)-2)
#define SPC_IS_HITDISK(q) ((q).HighPart==(LONG)-3)

#define SPC_REFRESH(q) (SPC_IS_INVALID(q) || SPC_IS_HITDISK(q))

LPTSTR ShortSizeFormatInternal(LPTSTR szBuf, LARGE_INTEGER qw);

#define LARGE_INTEGER_NULL(q) ((q).LowPart = 0, (q).HighPart = 0)

LARGE_INTEGER TriMultiply(DWORD dw1, DWORD dw2, DWORD dwSmall);

