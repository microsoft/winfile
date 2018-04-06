/********************************************************************

   wfdocb.h

   Doc Bucket header

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#ifndef _WFDOCB_H
#define _WFDOCB_H
//
// Doc prototypes; typdefs
//

typedef struct _DOC_BUCKET** PPDOCBUCKET;
typedef struct _DOC_BUCKET* PDOCBUCKET;

PPDOCBUCKET DocConstruct(VOID);
VOID DocDestruct(PPDOCBUCKET ppDocBucket);
INT DocInsert(PPDOCBUCKET ppDocBucket, LPTSTR lpszExt, LPTSTR lpszFileIcon);
PDOCBUCKET DocFind(PPDOCBUCKET ppDocBucket, LPTSTR lpszExt);
HICON DocGetIcon(PDOCBUCKET pDocBucket);

#ifdef DOCENUM
typedef struct _DOC_ENUM DOCENUM, *PDOCENUM;

PDOCENUM DocOpenEnum(PPDOCBUCKET ppDocBucket);
LPTSTR DocEnum(PDOCENUM pDocEnum, PDWORD pdwParm);
VOID DocCloseEnum(PDOCENUM pDocEnum);
#endif

#endif // ndef _WFDOCB_H

