/**************************************************************************

   res.h

   Include for WINFILE resources

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

**************************************************************************/

//
// Japan markers:
//

#define JAPANBEGIN
#define JAPANEND
#define KOREAJAPANBEGIN
#define KOREAJAPANEND

#define bJAPAN bJapan
#define bKOREAJAPAN bJapan

//
// Moved from wfcopy.h
// The problem is that IsTheDiskReallyThere/CheckDrive
// uses these codes so they really need to be global
//

#define FUNC_MOVE       0x0001
#define FUNC_COPY       0x0002
#define FUNC_DELETE     0x0003
#define FUNC_RENAME     0x0004

//
// These should not be used in the move/copy code;
// only for IsTheDiskReallyThere
//
#define FUNC_SETDRIVE       0x0005
#define FUNC_EXPAND     0x0006
#define FUNC_LABEL      0x0007


/* This is for the menuhelp messages.  Pretty much all ID's after this should
 * be reserved for this purpose.
 */
#define MH_POPUP            (4000-16)
#define MH_MYITEMS          4000

#include "wfdlgs.h"

/* Menu Command Defines */
#define IDM_FILE            0
#define IDM_OPEN            101
#define IDM_PRINT           102
#define IDM_ASSOCIATE       103
#define IDM_SEARCH          104
#define IDM_RUN             105
#define IDM_MOVE            106
#define IDM_COPY            107
#define IDM_DELETE          108
#define IDM_RENAME          109
#define IDM_ATTRIBS         110
#define IDM_MAKEDIR         111
#define IDM_SELALL          112
#define IDM_DESELALL        113
#define IDM_UNDO            114
#define IDM_EXIT            115
#define IDM_SELECT          116
#define IDM_UNDELETE        117
#define IDM_COPYTOCLIPBOARD 118
#define IDM_COMPRESS        119
#define IDM_UNCOMPRESS      120
#define IDM_PASTE           121
#define IDM_EDIT            122
#define IDM_CUTTOCLIPBOARD  123
#define IDM_STARTCMDSHELL   124
#define IDM_GOTODIR         125
#define IDM_HISTORYBACK     126
#define IDM_HISTORYFWD      127
#define IDM_STARTPOWERSHELL 128
#define IDM_STARTBASHSHELL  129

// This IDM_ is reserved for IDH_GROUP_ATTRIBS
#define IDM_GROUP_ATTRIBS   199

#define IDM_DISK            1
#define IDM_DISKCOPY        201
#define IDM_LABEL           202
#define IDM_FORMAT          203
//#define IDM_SYSDISK         204
#define IDM_CONNECT         205
#define IDM_DISCONNECT      206
#define IDM_DRIVESMORE      251
#define IDM_CONNECTIONS     252
#define IDM_SHAREDDIR       253

// AS added to end
#define IDM_SHAREAS         254
#define IDM_STOPSHARE       255

#define IDM_TREE            2
#define IDM_EXPONE          301
#define IDM_EXPSUB          302
#define IDM_EXPALL          303
#define IDM_COLLAPSE        304
#define IDM_NEWTREE         305

#define IDM_VIEW            3
#define IDM_VNAME           401
#define IDM_VDETAILS        402
#define IDM_VOTHER          403

#define IDM_BYNAME          404
#define IDM_BYTYPE          405
#define IDM_BYSIZE          406
#define IDM_BYDATE          407    // reverse date sort
#define IDM_BYFDATE         408    // forward date sort

#define IDM_VINCLUDE        409
#define IDM_REPLACE         410

#define IDM_TREEONLY        411
#define IDM_DIRONLY         412
#define IDM_BOTH            413
#define IDM_SPLIT           414

#define IDM_ESCAPE          420

#define IDM_OPTIONS         4
#define IDM_CONFIRM         501
#define IDM_LOWERCASE       502
#define IDM_STATUSBAR       503
#define IDM_MINONRUN        504
#define IDM_ADDPLUSES       505
#define IDM_EXPANDTREE      506

#define IDM_DRIVEBAR      507   /* Options->Drivebar */
#define IDM_TOOLBAR     508   /* Options->Toolbar */
#define IDM_NEWWINONCONNECT 509  /* Options->New Window On Connect */

#define IDM_FONT            510
#define IDM_SAVESETTINGS    511

#define IDM_TOOLBARCUST     512

#ifdef PROGMAN
#define IDM_SAVENOW         513
#endif

#define IDM_INDEXONLAUNCH   514

#define IDM_PREF          515

#define IDM_SECURITY        5
#define IDM_PERMISSIONS     605      // !! WARNING HARD CODED !!
#define IDM_AUDITING        606
#define IDM_OWNER           607

#define IDM_EXTENSIONS      6

#define IDM_WINDOW           16
#define IDM_CASCADE          1701
#define IDM_TILE             1702

#define IDM_TILEHORIZONTALLY 1703
#define IDM_REFRESH          1704
#define IDM_ARRANGE          1705
#define IDM_NEWWINDOW        1706
#define IDM_CHILDSTART       1707

#define IDM_HELP            17
#define IDM_HELPINDEX       1801
#define IDM_HELPKEYS        0x001E
#define IDM_HELPCOMMANDS    0x0020
#define IDM_HELPPROCS       0x0021
#define IDM_HELPHELP        1802
#define IDM_ABOUT           1803

#define IDM_DRIVELISTJUMP 2000  /* for defining an accelerator */


/* Control ID's; these must not conflict with an IDM_* */
#define IDC_TOOLBAR  3000
#define IDC_STATUS   3001
#define IDC_DRIVES   3002
#define IDC_EXTENSIONS  3003


#define BITMAPS             100

#define IDB_TOOLBAR  101
#define IDB_EXTRATOOLS  102

#define FILES_WIDTH         16
#define FILES_HEIGHT        16
#define MINIDRIVE_WIDTH     16
#define MINIDRIVE_HEIGHT    9
#define DRIVES_WIDTH        23
#define DRIVES_HEIGHT       14

#define APPICON             200
#define TREEICON            201
#define DIRICON             202
#define WINDOWSICON         203
#define TREEDIRICON         204

#define SINGLEMOVECURSOR    300 // move is even
#define MULTMOVECURSOR      302
#define SINGLECOPYCURSOR    301 // copy is odd
#define MULTCOPYCURSOR      303

#define APPCURSOR           300
#define DIRCURSOR           301
#define DOCCURSOR           302
#define FILECURSOR          304
#define FILESCURSOR         305
#define SPLITCURSOR         306

#define APPCURSORC          310
#define DIRCURSORC          311
#define DOCCURSORC          312
#define FILECURSORC         314
#define FILESCURSORC        315

#define WFACCELTABLE        400

#define FRAMEMENU           500

//#define IDS_ENDSESSION      40  /* Must be > 32 */
//#define IDS_ENDSESSIONMSG   41
#define IDS_COPYDISK        50
#define IDS_INSERTDEST      51
#define IDS_INSERTSRC       52
#define IDS_INSERTSRCDEST   53
#define IDS_FORMATTINGDEST  54
#define IDS_COPYDISKERR     55
#define IDS_COPYDISKERRMSG  56
#define IDS_COPYDISKSELMSG  57
#define IDS_COPYSRCDESTINCOMPAT 58
#define IDS_PERCENTCOMP     60
#define IDS_CREATEROOT      61
#define IDS_COPYSYSFILES    62
#define IDS_FORMATERR       63
//#define IDS_FORMATERRMSG    64
//#define IDS_FORMATCURERR    65
#define IDS_FORMATCOMPLETE  66
#define IDS_FORMATANOTHER   67
#define IDS_FORMATCANCELLED 68
//#define IDS_SYSDISK         70
//#define IDS_SYSDISKRUSURE   71
//#define IDS_SYSDISKERR      72
//#define IDS_SYSDISKNOFILES  73
//#define IDS_SYSDISKSAMEDRIVE    74
//#define IDS_SYSDISKADDERR   75
#define IDS_NETERR          80
//#define IDS_NETCONERRMSG    81
//#define IDS_NETDISCONCURERR 82
#define IDS_NETDISCONWINERR 83
//#define IDS_NETDISCON       84
//#define IDS_NETDISCONRUSURE 85
//#define IDS_NETDISCONERRMSG 86
//#define IDS_FILESYSERR      90
#define IDS_ATTRIBERR       91
#define IDS_MAKEDIRERR      92
#define IDS_LABELDISKERR    93
//#define IDS_SEARCHERR       94
#define IDS_SEARCHNOMATCHES 95
//#define IDS_MAKEDIREXISTS   96
#define IDS_SEARCHREFRESH   97
#define IDS_LABELACCESSDENIED  98
#define IDS_ASSOCFILE       100
#define IDS_DRIVETEMP       101
#define IDS_EXECERRTITLE    110
#define IDS_UNKNOWNMSG      111
#define IDS_NOMEMORYMSG     112
#define IDS_FILENOTFOUNDMSG 113
#define IDS_BADPATHMSG      114
//#define IDS_MANYOPENFILESMSG    115
#define IDS_NOASSOCMSG      116
//#define IDS_MULTIPLEDSMSG   117
#define IDS_ASSOCINCOMPLETE 118
#define IDS_MOUSECONFIRM    120
#define IDS_COPYMOUSECONFIRM    121
#define IDS_MOVEMOUSECONFIRM    122
#define IDS_EXECMOUSECONFIRM    123
#define IDS_WINFILE         124
//#define IDS_ONLYONE         125
#define IDS_TREETITLE       126
#define IDS_SEARCHTITLE     127
//#define IDS_NOFILESTITLE    130
//#define IDS_NOFILESMSG      131
#define IDS_TOOMANYTITLE    132
#define IDS_OOMTITLE        133
#define IDS_OOMREADINGDIRMSG    134
#define IDS_CURDIRIS        140
#define IDS_COPY            141
#define IDS_ANDCOPY         142
#define IDS_RENAME          143
#define IDS_ANDRENAME       144
#define IDS_FORMAT          145
#define IDS_FORMATSELDISK   146
//#define IDS_MAKESYSDISK     147
// moved #define IDS_DISCONNECT      148
//#define IDS_DISCONSELDISK   149
#define IDS_CREATINGMSG     150
#define IDS_REMOVINGMSG     151
#define IDS_COPYINGMSG      152
#define IDS_RENAMINGMSG     153
#define IDS_MOVINGMSG       154
#define IDS_DELETINGMSG     155
#define IDS_PRINTINGMSG     156
//#define IDS_NOSUCHDRIVE     160
#define IDS_MOVEREADONLY    161
#define IDS_RENAMEREADONLY  162
#define IDS_CONFIRMREPLACE  163
#define IDS_CONFIRMREPLACERO    164 /* Confirm/readonly */
#define IDS_CONFIRMRMDIR    165 /* Must be confirm + 1 */
#define IDS_CONFIRMRMDIRRO  166
#define IDS_CONFIRMDELETE   167
#define IDS_CONFIRMDELETERO 168
#define IDS_COPYINGTITLE    169
#define IDS_REMOVINGDIRMSG  170
#define IDS_STATUSMSG       180
#define IDS_DIRSREAD        181
#define IDS_DRIVEFREE       182
#define IDS_SEARCHMSG       183
#define IDS_DRIVE           184
#define IDS_SELECTEDFILES   185
#define IDS_NETDISCONOPEN   186
#define IDS_STATUSMSG2      187
#define IDS_DRIVENOTREADY   188
#define IDS_UNFORMATTED     189

//#define IDS_CANTPRINTTITLE  190
//#define IDS_PRINTFNF        191
#define IDS_PRINTDISK       192
#define IDS_PRINTMEMORY     193
#define IDS_PRINTERROR      194
#define IDS_TREEABORT       195
#define IDS_TREEABORTTITLE  196
#define IDS_DESTFULL        197
#define IDS_WRITEPROTECTFILE    198
#define IDS_FORMATQUICKFAILURE  199

//#define IDS_OS2APPMSG       200
//#define IDS_NEWWINDOWSMSG   201
//#define IDS_PMODEONLYMSG    202

#define IDS_DDEFAIL         203
#define IDS_FMIFSLOADERR    204

#define IDS_SHAREDDIR       209
#define IDS_FORMATCONFIRM   210
#define IDS_FORMATCONFIRMTITLE  211
#define IDS_DISKCOPYCONFIRM 212
#define IDS_DISKCOPYCONFIRMTITLE    213
#define IDS_ANDCLOSE           214
#define IDS_CLOSE              215
// moved #define IDS_UNDELETE        215 Taken.
// moved #define IDS_CONNECT         216
// moved #define IDS_CONNECTIONS     217
#define IDS_PATHNOTTHERE    218
#define IDS_PROGRAMS        219
#define IDS_ASSOCIATE       220
#define IDS_RUN             221
#define IDS_PRINTERRTITLE   222
#define IDS_WINHELPERR      223
#define IDS_NOEXEASSOC          224
#define IDS_ASSOCNOTEXE         225
#define IDS_ASSOCNONE           226
#define IDS_NOFILES             227
#define IDS_PRINTONLYONE        228
//#define IDS_COMPRESSEDEXE       229
#define IDS_INVALIDDLL          230
#define IDS_SHAREERROR          231
#define IDS_CREATELONGDIR       232
#define IDS_CREATELONGDIRTITLE  233
#define IDS_BYTES               234
#define IDS_SBYTES              235
#define IDS_NOCOPYTOCLIP        236

#define IDS_MENUANDITEM         237

#define IDS_DRIVELABEL          238 /* label for drive list */
#define IDS_STATUSMSGSINGLE     239 /* for building 1-file status display */

#define IDS_CONNECTHELP         240 /* status bar text for tbar buttons */
#define IDS_DISCONHELP          241
#define IDS_CONNECTIONHELP      242
#define IDS_SHAREASHELP         243
#define IDS_STOPSHAREHELP       244
#define IDS_VDETAILSHELP        245
#define IDS_VNAMEHELP           246
#define IDS_BYNAMEHELP          247
#define IDS_BYTYPEHELP          248
#define IDS_BYSIZEHELP          249
#define IDS_BYDATEHELP          250
#define IDS_NEWWINHELP          251
#define IDS_COPYHELP            252
#define IDS_MOVEHELP            253

#define IDS_DIRNAMELABEL        254 /* "&Directory Name:" in props dlg */
//#define IDS_FILEVERSIONKEY      255 /* base key name for getting ver info */
#define IDS_DRIVENOTAVAILABLE   256

// moved #define IDS_SHAREAS    257 /* "Share As..." menu item */
// moved #define IDS_STOPSHARE  258 /* "Stop Sharing..." menu item */

#define IDS_SHAREDAS    259 /* "Shared as %s" for status bar */
#define IDS_NOTSHARED   260 /* "Not shared" for status bar */

#define IDS_DELHELP     261

#define IDS_DRIVE_COMPRESSED    262

#define IDS_DRAG_COPYING        263
#define IDS_DRAG_MOVING         264
#define IDS_DRAG_EXECUTING      265

#define IDS_ORDERB      266
#define IDS_ORDERKB     267
#define IDS_ORDERMB     268
#define IDS_ORDERGB     269
#define IDS_ORDERTB     270


#define IDS_NOACCESSDIR  280
#define IDS_NOACCESSFILE  281

// for ERROR_BAD_PATHNAME 161L
//#define IDS_BADPATHNAME      282

#define IDS_DRIVEBUSY_COPY   283
#define IDS_DRIVEBUSY_FORMAT 284

#define IDS_COPYMOVENOTCOMPLETED 285
#define IDS_DIRREMAINS           286

#define IDS_NOSUCHDIRTITLE    287
#define IDS_NOSUCHDIR         288

#define IDS_BADNETNAMETITLE   289
#define IDS_BADNETNAME        290

//#define IDS_DIREXISTSASFILE   291

#define IDS_ALLFILES          292

#define IDS_ASSOC_OPEN        294
#define IDS_ASSOC_PRINT       295

#define IDS_ADDEXTTITLE       298
#define IDS_ADDEXTTEXT        299
#define IDS_EXTTITLE          300

#define IDS_EXTADDERROR       301
#define IDS_EXTDELERROR       302
#define IDS_FILETYPEADDERROR  303
#define IDS_FILETYPEDELERROR  304
#define IDS_FILETYPEREADERROR 305

#define IDS_FILETYPENULLDESCERROR 306
#define IDS_FILETYPEDUPDESCERROR  307

#define IDS_FILETYPEDELCONFIRMTITLE 308
#define IDS_FILETYPEDELCONFIRMTEXT  309
#define IDS_FILETYPEDELCONFIRMUSERTEXT  310
#define IDS_FILETYPEUSERIZETEXT  311
#define IDS_FILETYPECOMMANDNULLTEXT 312

#define IDS_NEWFILETYPETITLE  320
#define IDS_COPYINGDISKTITLE  321
#define IDS_SEARCHING         322

#define IDS_EXTTEXT           323
#define IDS_BUSYFORMATQUITVERIFY    324
#define IDS_BUSYCOPYQUITVERIFY      325

#define IDS_PERCENTCOMPLETE   326

#define IDS_FORMATSELECTDLGTITLE 327

#define IDS_DRIVEBASE       350
#define IDS_12MB            354
#define IDS_360KB           353
#define IDS_144MB           356
#define IDS_720KB           355
#define IDS_288MB           357
#define IDS_DEVICECAP       358
#define IDS_QSUPMEDIA       359
#define IDS_2080MB          360
#define IDS_REMOVEMED       361
#define IDS_CANTFORMATTITLE 362
#define IDS_CANTFORMAT      363

#if defined(JAPAN) && defined(i386)
//
// FMR jul.21.1994 JY
// We added 640KB/1.23MB media types.
//
#define IDS_123MB           364
#define IDS_640KB           365

/* ADD KBNES. NEC MEDIATYPE START */
#define IDS_125MB           370
#define IDS_256KB           371
#define IDS_128MB           373
/* ADD KBNES. NEC MEDIATYPE END */
#endif

#define IDS_FFERR_INCFS        400
#define IDS_FFERR_ACCESSDENIED 401
#define IDS_FFERR_DISKWP       402
#define IDS_FFERR_CANTLOCK     403
#define IDS_FFERR_CANTQUICKF   404
#define IDS_FFERR_SRCIOERR     405
#define IDS_FFERR_DSTIOERR     406
#define IDS_FFERR_SRCDSTIOERR  407
#define IDS_FFERR_GENIOERR     408
//#define IDS_FFERR_SYSFILES  409
//#define IDS_FFERR_MEDIASENSE    410
#define IDS_FFERR          411
#define IDS_FFERR_BADLABEL 412

#define IDS_OPENINGMSG          420
#define IDS_CLOSINGMSG          421
#define IDS_TOOMANYWINDOWS      422

#define IDS_QUICKFORMATTINGTITLE 423

#define IDS_INITUPDATEFAIL       424
#define IDS_INITUPDATEFAILTITLE  425
#define IDS_READING              426

#define IDS_COMPRESSDIR          427
#define IDS_UNCOMPRESSDIR        428
#define IDS_COMPRESS_ATTRIB_ERR  429
#define IDS_NTLDRCOMPRESSERR     430
#define IDS_MULTICOMPRESSERR     431

#define IDS_EDITFILTER           432

#define IDS_VERNAME_BASE          500
#define IDS_VN_COMMENTS           (IDS_VERNAME_BASE + 0)
#define IDS_VN_COMPANYNAME        (IDS_VERNAME_BASE + 1)
#define IDS_VN_FILEDESCRIPTION    (IDS_VERNAME_BASE + 2)
#define IDS_VN_INTERNALNAME       (IDS_VERNAME_BASE + 3)
#define IDS_VN_LEGALTRADEMARKS    (IDS_VERNAME_BASE + 4)
#define IDS_VN_ORIGINALFILENAME   (IDS_VERNAME_BASE + 5)
#define IDS_VN_PRIVATEBUILD       (IDS_VERNAME_BASE + 6)
#define IDS_VN_PRODUCTNAME        (IDS_VERNAME_BASE + 7)
#define IDS_VN_PRODUCTVERSION     (IDS_VERNAME_BASE + 8)
#define IDS_VN_SPECIALBUILD       (IDS_VERNAME_BASE + 9)

#define IDS_VN_LANGUAGE    (IDS_VERNAME_BASE + 10)
#define IDS_VN_LANGUAGES   (IDS_VERNAME_BASE + 11)

#define IDS_FFERROR     (800-256)
// Note that the next 256 entries are reserved for strings that will appear
// in the directory listing if there is an error reading the drive.

// These are all the ID's for the strings that may be inserted into various
// menus at init time.  Note that tbar.c depends on the order of these strings.

// was 608
#define MS_EXTRA              800
#define IDS_CONNECT           (MS_EXTRA+0)
#define IDS_DISCONNECT        (MS_EXTRA+1)
#define IDS_CONNECTIONS       (MS_EXTRA+2)
#define IDS_SHAREAS           (MS_EXTRA+3)
#define IDS_STOPSHARE         (MS_EXTRA+4)
#define IDS_SHARES            (MS_EXTRA+5)
#define IDS_UNDELETE          (MS_EXTRA+6)
#define IDS_NEWWINONCONNECT   (MS_EXTRA+7)

#define IDS_COPYERROR       1000
#define IDS_VERBS           1010
#define IDS_ACTIONS         1020
#define IDS_REPLACING       1030
#define IDS_CREATING        1031

//#define IDS_REASONS       1040    // error codes strings (range += 255)

// IDS_ from 1100 to 1199 reserved for suggestions!

JAPANBEGIN
#define IDS_KK_COPYFROMSTR              2000
#define IDS_KK_COPYTOSTR                2001
#define IDS_KK_RENAMEFROMSTR            2002
#define IDS_KK_RENAMETOSTR              2003
#define IDS_KK_COPY                     2004
#define IDS_WRNNOSHIFTJIS               2005
JAPANEND
