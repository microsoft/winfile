/********************************************************************

   suggest.h

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#include "windows.h"

#define DE_BIT              29              // bit for app errors
#define DE_BEGIN            (1 << DE_BIT)   // beginning of DE_*


// Suggestion flags

#define SUG_NULL 0
#define SUG_IGN_FORMATMESSAGE 1

extern HINSTANCE hAppInstance;


DWORD FormatError(BOOL bNullString, LPTSTR lpBuf, INT iSize, DWORD dwError);
PDWORD FormatSuggest(DWORD dwError);

// Internal error numbers begin here
// They will not conflict with any system ones as long as DE_BEGIN
// has bit 29 set.

#define DE_OPCANCELLED      DE_BEGIN
#define DE_INSMEM           (DE_BEGIN+1)
#define DE_ROOTDIR          (DE_BEGIN+3)
#define DE_DESTSUBTREE      (DE_BEGIN+4)
#define DE_WINDOWSFILE      (DE_BEGIN+5)
#define DE_MANYDEST         (DE_BEGIN+6)
#define DE_SAMEFILE         (DE_BEGIN+7)
#define DE_RENAMREPLACE     (DE_BEGIN+8)
#define DE_DIFFDIR          (DE_BEGIN+9)
#define DE_HOWDIDTHISHAPPEN (DE_BEGIN+10)
#define DE_MANYSRC1DEST     (DE_BEGIN+12)
#define DE_RETRY            (DE_BEGIN+13)
#define DE_DIREXISTSASFILE  (DE_BEGIN+14)
#define DE_MAKEDIREXISTS    (DE_BEGIN+15)
#define DE_UPDATING         (DE_BEGIN+16)
#define DE_DELEXTWRONGMODE  (DE_BEGIN+17)
#define DE_REGNAME          (DE_BEGIN+18)
