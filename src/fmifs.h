/********************************************************************

   fmifs.h

   This header file contains the specification of the interface
   between the file manager and fmifs.dll for the purposes of
   accomplishing IFS functions.

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/

#if !defined( _FMIFS_DEFN_ )

#define _FMIFS_DEFN_


//
// These are the defines for 'PacketType'.
//

typedef enum _FMIFS_PACKET_TYPE {
    FmIfsPercentCompleted,
    FmIfsFormatReport,
    FmIfsInsertDisk,
    FmIfsIncompatibleFileSystem,
    FmIfsFormattingDestination,
    FmIfsIncompatibleMedia,
    FmIfsAccessDenied,
    FmIfsMediaWriteProtected,
    FmIfsCantLock,
    FmIfsCantQuickFormat,
    FmIfsIoError,
    FmIfsFinished,
    FmIfsBadLabel,
#if defined( DBLSPACE_ENABLED )
    FmIfsDblspaceCreateFailed,
    FmIfsDblspaceMountFailed,
    FmIfsDblspaceDriveLetterFailed,
    FmIfsDblspaceCreated,
    FmIfsDblspaceMounted,
#endif // DBLSPACE_ENABLED
    FmIfsCheckOnReboot,
    FmIfsTextMessage,
    FmIfsHiddenStatus
} FMIFS_PACKET_TYPE, *PFMIFS_PACKET_TYPE;

typedef struct _FMIFS_PERCENT_COMPLETE_INFORMATION {
    ULONG   PercentCompleted;
} FMIFS_PERCENT_COMPLETE_INFORMATION, *PFMIFS_PERCENT_COMPLETE_INFORMATION;

typedef struct _FMIFS_FORMAT_REPORT_INFORMATION {
    ULONG   KiloBytesTotalDiskSpace;
    ULONG   KiloBytesAvailable;
} FMIFS_FORMAT_REPORT_INFORMATION, *PFMIFS_FORMAT_REPORT_INFORMATION;

// The packet for FmIfsDblspaceCreated is a Unicode string
// giving the name of the Compressed Volume File; it is not
// necessarily zero-terminated.
//

#define DISK_TYPE_GENERIC           0
#define DISK_TYPE_SOURCE            1
#define DISK_TYPE_TARGET            2
#define DISK_TYPE_SOURCE_AND_TARGET 3

typedef struct _FMIFS_INSERT_DISK_INFORMATION {
    ULONG   DiskType;
} FMIFS_INSERT_DISK_INFORMATION, *PFMIFS_INSERT_DISK_INFORMATION;

typedef struct _FMIFS_IO_ERROR_INFORMATION {
    ULONG   DiskType;
    ULONG   Head;
    ULONG   Track;
} FMIFS_IO_ERROR_INFORMATION, *PFMIFS_IO_ERROR_INFORMATION;

typedef struct _FMIFS_FINISHED_INFORMATION {
    BOOLEAN Success;
} FMIFS_FINISHED_INFORMATION, *PFMIFS_FINISHED_INFORMATION;

typedef struct _FMIFS_CHECKONREBOOT_INFORMATION {
    OUT BOOLEAN QueryResult; // TRUE for "yes", FALSE for "no"
} FMIFS_CHECKONREBOOT_INFORMATION, *PFMIFS_CHECKONREBOOT_INFORMATION;

typedef enum _TEXT_MESSAGE_TYPE {
    MESSAGE_TYPE_PROGRESS,
    MESSAGE_TYPE_RESULTS,
    MESSAGE_TYPE_FINAL
} TEXT_MESSAGE_TYPE, *PTEXT_MESSAGE_TYPE;

typedef struct _FMIFS_TEXT_MESSAGE {
    IN TEXT_MESSAGE_TYPE    MessageType;
    IN PSTR                 Message;
} FMIFS_TEXT_MESSAGE, *PFMIFS_TEXT_MESSAGE;




//
// This is a list of supported floppy media types for format.
//

typedef enum _FMIFS_MEDIA_TYPE {
    FmMediaUnknown,
    FmMediaF5_160_512,      // 5.25", 160KB,  512 bytes/sector
    FmMediaF5_180_512,      // 5.25", 180KB,  512 bytes/sector
    FmMediaF5_320_512,      // 5.25", 320KB,  512 bytes/sector
    FmMediaF5_320_1024,     // 5.25", 320KB,  1024 bytes/sector
    FmMediaF5_360_512,      // 5.25", 360KB,  512 bytes/sector
    FmMediaF3_720_512,      // 3.5",  720KB,  512 bytes/sector
    FmMediaF5_1Pt2_512,     // 5.25", 1.2MB,  512 bytes/sector
    FmMediaF3_1Pt44_512,    // 3.5",  1.44MB, 512 bytes/sector
    FmMediaF3_2Pt88_512,    // 3.5",  2.88MB, 512 bytes/sector
    FmMediaF3_20Pt8_512,    // 3.5",  20.8MB, 512 bytes/sector
    FmMediaRemovable,       // Removable media other than floppy
    FmMediaFixed,
    FmMediaF3_120M_512      // 3.5", 120M Floppy
} FMIFS_MEDIA_TYPE, *PFMIFS_MEDIA_TYPE;

//
// The structure below defines information to be passed into ChkdskEx.
// When new fields are added, the version number will have to be upgraded
// so that only new code will reference those new fields.
//
typedef struct {
    UCHAR   Major;      // initial version is 1.0
    UCHAR   Minor;
    ULONG   Flags;
    PWSTR   PathToCheck;
} FMIFS_CHKDSKEX_PARAM, *PFMIFS_CHKDSKEX_PARAM;

//
// Internal definitions for Flags field in FMIFS_CHKDSKEX_PARAM
//
#define FMIFS_CHKDSK_RECOVER_FREE_SPACE       0x00000002UL
#define FMIFS_CHKDSK_RECOVER_ALLOC_SPACE      0x00000004UL

//
// External definitions for Flags field in FMIFS_CHKDSKEX_PARAM
//

// FMIFS_CHKDSK_VERBOSE
//  - For FAT, chkdsk will print every filename being processed
//  - For NTFS, chkdsk will print clean up messages
// FMIFS_CHKDSK_RECOVER
//  - Perform sector checking on free and allocated space
// FMIFS_CHKDSK_EXTEND
//  - For NTFS, chkdsk will extend a volume
// FMIFS_CHKDSK_DOWNGRADE (for NT 5 or later but obsolete anyway)
//  - For NTFS, this downgrade a volume from most recent NTFS version
// FMIFS_CHKDSK_ENABLE_UPGRADE (for NT 5 or later but obsolete anyway)
//  - For NTFS, this upgrades a volume to most recent NTFS version
// FMIFS_CHKDSK_CHECK_IF_DIRTY
//  - Perform consistency check only if the volume is dirty
// FMIFS_CHKDSK_FORCE (for NT 5 or later)
//  - Forces the volume to dismount first if necessary
// FMIFS_CHKDSK_SKIP_INDEX_SCAN
//  - Skip the scanning of each index entry
// FMIFS_CHKDSK_SKIP_CYCLE_SCAN
//  - Skip the checking of cycles within the directory tree


#define FMIFS_CHKDSK_VERBOSE                  0x00000001UL
#define FMIFS_CHKDSK_RECOVER                  (FMIFS_CHKDSK_RECOVER_FREE_SPACE | \
                                               FMIFS_CHKDSK_RECOVER_ALLOC_SPACE)
#define FMIFS_CHKDSK_EXTEND                   0x00000008UL
#define FMIFS_CHKDSK_DOWNGRADE                0x00000010UL
#define FMIFS_CHKDSK_ENABLE_UPGRADE           0x00000020UL
#define FMIFS_CHKDSK_CHECK_IF_DIRTY           0x00000080UL
#define FMIFS_CHKDSK_FORCE                    0x00000100UL
#define FMIFS_CHKDSK_SKIP_INDEX_SCAN          0x00000200UL
#define FMIFS_CHKDSK_SKIP_CYCLE_SCAN          0x00000400UL

//
// Function types/interfaces.
//

typedef BOOLEAN
(*FMIFS_CALLBACK)(
    IN  FMIFS_PACKET_TYPE   PacketType,
    IN  ULONG               PacketLength,
    IN  PVOID               PacketData
    );

typedef
VOID
(*PFMIFS_FORMAT_ROUTINE)(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  FMIFS_CALLBACK      Callback
    );

typedef
VOID
(*PFMIFS_FORMATEX_ROUTINE)(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  ULONG               ClusterSize,
    IN  FMIFS_CALLBACK      Callback
    );

typedef
BOOLEAN
(*PFMIFS_ENABLECOMP_ROUTINE)(
    IN  PWSTR               DriveName,
    IN  USHORT              CompressionFormat
    );


typedef
VOID
(*PFMIFS_CHKDSK_ROUTINE)(
    IN  PWSTR               DriveName,
    IN  PWSTR               FileSystemName,
    IN  BOOLEAN             Fix,
    IN  BOOLEAN             Verbose,
    IN  BOOLEAN             OnlyIfDirty,
    IN  BOOLEAN             Recover,
    IN  PWSTR               PathToCheck,
    IN  BOOLEAN             Extend,
    IN  FMIFS_CALLBACK      Callback
    );

typedef
VOID
(*PFMIFS_CHKDSKEX_ROUTINE)(
    IN  PWSTR                   DriveName,
    IN  PWSTR                   FileSystemName,
    IN  BOOLEAN                 Fix,
    IN  PFMIFS_CHKDSKEX_PARAM   Param,
    IN  FMIFS_CALLBACK          Callback
    );

typedef
VOID
(*PFMIFS_EXTEND_ROUTINE)(
    IN  PWSTR               DriveName,
    IN  BOOLEAN             Verify,
    IN  FMIFS_CALLBACK      Callback
    );


typedef
VOID
(*PFMIFS_DISKCOPY_ROUTINE)(
    IN  PWSTR           SourceDrive,
    IN  PWSTR           DestDrive,
    IN  BOOLEAN         Verify,
    IN  FMIFS_CALLBACK  Callback
    );

typedef
BOOLEAN
(*PFMIFS_SETLABEL_ROUTINE)(
    IN  PWSTR   DriveName,
    IN  PWSTR   Label
    );

typedef
BOOLEAN
(*PFMIFS_QSUPMEDIA_ROUTINE)(
    IN  PWSTR               DriveName,
    OUT PFMIFS_MEDIA_TYPE   MediaTypeArray  OPTIONAL,
    IN  ULONG               NumberOfArrayEntries,
    OUT PULONG              NumberOfMediaTypes
    );


typedef
VOID
(*PFMIFS_DOUBLESPACE_CREATE_ROUTINE)(
    IN PWSTR           HostDriveName,
    IN ULONG           Size,
    IN PWSTR           Label,
    IN PWSTR           NewDriveName,
    IN FMIFS_CALLBACK  Callback
    );

#if defined( DBLSPACE_ENABLED )
typedef
VOID
(*PFMIFS_DOUBLESPACE_DELETE_ROUTINE)(
    IN PWSTR           DblspaceDriveName,
    IN FMIFS_CALLBACK  Callback
    );

typedef
VOID
(*PFMIFS_DOUBLESPACE_MOUNT_ROUTINE)(
    IN PWSTR           HostDriveName,
    IN PWSTR           CvfName,
    IN PWSTR           NewDriveName,
    IN FMIFS_CALLBACK  Callback
    );

typedef
VOID
(*PFMIFS_DOUBLESPACE_DISMOUNT_ROUTINE)(
    IN PWSTR           DblspaceDriveName,
    IN FMIFS_CALLBACK  Callback
    );

typedef
BOOLEAN
(*PFMIFS_DOUBLESPACE_QUERY_INFO_ROUTINE)(
    IN  PWSTR       DosDriveName,
    OUT PBOOLEAN    IsRemovable,
    OUT PBOOLEAN    IsFloppy,
    OUT PBOOLEAN    IsCompressed,
    OUT PBOOLEAN    Error,
    OUT PWSTR       NtDriveName,
    IN  ULONG       MaxNtDriveNameLength,
    OUT PWSTR       CvfFileName,
    IN  ULONG       MaxCvfFileNameLength,
    OUT PWSTR       HostDriveName,
    IN  ULONG       MaxHostDriveNameLength
    );

typedef
BOOLEAN
(*PFMIFS_DOUBLESPACE_SET_AUTMOUNT_ROUTINE)(
    IN  BOOLEAN EnableAutomount
    );

#endif // DBLSPACE_ENABLED


VOID
Format(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  FMIFS_CALLBACK      Callback
    );

VOID
FormatEx(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  ULONG               ClusterSize,
    IN  FMIFS_CALLBACK      Callback
    );

BOOLEAN
EnableVolumeCompression(
    IN  PWSTR               DriveName,
    IN  USHORT              CompressionFormat
    );

VOID
Chkdsk(
    IN  PWSTR               DriveName,
    IN  PWSTR               FileSystemName,
    IN  BOOLEAN             Fix,
    IN  BOOLEAN             Verbose,
    IN  BOOLEAN             OnlyIfDirty,
    IN  BOOLEAN             Recover,
    IN  PWSTR               PathToCheck,
    IN  BOOLEAN             Extend,
    IN  FMIFS_CALLBACK      Callback
    );

VOID
ChkdskEx(
    IN  PWSTR                   DriveName,
    IN  PWSTR                   FileSystemName,
    IN  BOOLEAN                 Fix,
    IN  PFMIFS_CHKDSKEX_PARAM   Param,
    IN  FMIFS_CALLBACK          Callback
    );

VOID
Extend(
    IN  PWSTR               DriveName,
    IN  BOOLEAN             Verify,
    IN  FMIFS_CALLBACK      Callback
    );

VOID
DiskCopy(
    IN  PWSTR           SourceDrive,
    IN  PWSTR           DestDrive,
    IN  BOOLEAN         Verify,
    IN  FMIFS_CALLBACK  Callback
    );

BOOLEAN
SetLabel(
    IN  PWSTR   DriveName,
    IN  PWSTR   Label
    );

BOOLEAN
QuerySupportedMedia(
    IN  PWSTR               DriveName,
    OUT PFMIFS_MEDIA_TYPE   MediaTypeArray  OPTIONAL,
    IN  ULONG               NumberOfArrayEntries,
    OUT PULONG              NumberOfMediaTypes
    );

VOID
DoubleSpaceCreate(
    IN PWSTR           HostDriveName,
    IN ULONG           Size,
    IN PWSTR           Label,
    IN PWSTR           NewDriveName,
    IN FMIFS_CALLBACK  Callback
    );

#if defined( DBLSPACE_ENABLED )

VOID
DoubleSpaceDelete(
    IN PWSTR           DblspaceDriveName,
    IN FMIFS_CALLBACK  Callback
    );

VOID
DoubleSpaceMount(
    IN PWSTR           HostDriveName,
    IN PWSTR           CvfName,
    IN PWSTR           NewDriveName,
    IN FMIFS_CALLBACK  Callback
    );

VOID
DoubleSpaceDismount(
    IN PWSTR           DblspaceDriveName,
    IN FMIFS_CALLBACK  Callback
    );

// Miscellaneous prototypes:
//
BOOLEAN
FmifsQueryDriveInformation(
    IN  PWSTR       DosDriveName,
    OUT PBOOLEAN    IsRemovable,
    OUT PBOOLEAN    IsFloppy,
    OUT PBOOLEAN    IsCompressed,
    OUT PBOOLEAN    Error,
    OUT PWSTR       NtDriveName,
    IN  ULONG       MaxNtDriveNameLength,
    OUT PWSTR       CvfFileName,
    IN  ULONG       MaxCvfFileNameLength,
    OUT PWSTR       HostDriveName,
    IN  ULONG       MaxHostDriveNameLength
    );

BOOLEAN
FmifsSetAutomount(
    IN  BOOLEAN EnableAutomount
    );

#endif


#endif // _FMIFS_DEFN_
