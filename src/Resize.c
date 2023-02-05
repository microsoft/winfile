
//
//  Demonstration code for a generic implementation of resizeable dialogs.
//  This code implements a window class (DIALOGRESIZECONTROLCLASS) which
//  can be added to dialog templates in order to define how controls
//  should be moved or resized as the dialog is resized.  An example
//  statement looks like:
//
//  DIALOGRESIZECONTROL { A, B, C, D }
//
//  Where:
//    A is the percentage of the new area to move the following control right;
//    B is the percentage of the new area to move the following control down;
//    C is the percentage of the new area to add to the following control's width;
//    D is the percentage of the new area to add to the following control's height.
//
//  Optionally, a second class is provided (DIALOGRESIZEDATACLASS) which
//  allows the resource to specify the bounds that the dialog can be resized
//  to.  By default the dialog can be resized freely.  An example statement
//  looks like:
//
//  DIALOGRESIZE { A, B }
//
//  Where:
//    A is the maximum percentage of the initial width allowable, or 0 if
//      unbounded;
//    B is the maximum percentage of the initial height allowable, or 0 if
//      unbounded.
//
//  Malcolm Smith, 11 Sep 2010
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "Resize.h"

//
//  For older 32-bit compilers, we won't have 64-bit support.  Use the 32-bit
//  variants; lack of 64-bit support doesn't matter when we're targeting
//  32-bit.
//

#ifndef SetWindowLongPtr
#define LONG_PTR LONG
#define DWLP_MSGRESULT DWL_MSGRESULT
#define DWLP_USER DWL_USER
#define SetWindowLongPtr( HWND, INDEX, DATA ) SetWindowLong( HWND, INDEX, DATA )
#define GetWindowLongPtr( HWND, INDEX ) GetWindowLong( HWND, INDEX )
#endif

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL         0x00400000L
#endif

//
//  This is the only window message defined by our resize dialog control class,
//  and it's fully internal.  We send this to each resize control to take care
//  of its buddy.
//

#define WM_RESIZEPARENT WM_USER

//
//  This is the only window message defined by our resize dialog data class,
//  and it's fully internal.  We send this to locate the user data that was
//  given to the control by the dialog manager.
//

#define WM_GETDIALOGMETADATA WM_USER

//
//  The following structure is allocated and stored uniquely for each
//  resizeable dialog that is currently active.  It describes the state
//  of the dialog when it was created.
//

#pragma pack(push, 2)
typedef struct _RESIZE_DIALOG_INFO {
    RECT  InitialClientRect;
    RECT  InitialWindowRect;
    HDWP  hDwp;
    ULONG NumberResizeControlsPresent;
} RESIZE_DIALOG_INFO, *PRESIZE_DIALOG_INFO;

//
//  This describes the data being passed into our resize metadata control
//  from the dialog manager as part of its creation (only on Windows NT and
//  successor systems.)  On Windows 95 and its successors, we lose all the
//  information we need to perform layout management, and this module is
//  effectively useless.
//

typedef struct _RESIZE_DIALOG_DATA_CREATION_INFO {
    USHORT SizeOfData;
    USHORT MaximumWidthPercent;
    USHORT MaximumHeightPercent;
} RESIZE_DIALOG_DATA_CREATION_INFO, *PRESIZE_DIALOG_DATA_CREATION_INFO;

//
//  This structure is attached to each resize metadata control.  It saves
//  off the bounds of the size that the dialog can accept.
//

typedef struct _RESIZE_DIALOG_DATA_WINDOW_EXTRA {
    USHORT MaximumWidthPercent;
    USHORT MaximumHeightPercent;
} RESIZE_DIALOG_DATA_WINDOW_EXTRA, *PRESIZE_DIALOG_DATA_WINDOW_EXTRA;

//
//  This structure is attached to each resize helper control.  It saves off
//  the initial size of its buddy and the amount of adjustment to apply to
//  the buddy when a resize occurs.
//

typedef struct _RESIZE_DIALOG_CONTROL_WINDOW_EXTRA {
    SMALL_RECT AdjustmentToBuddyRect;
    RECT InitialBuddyRect;
    BOOLEAN BuddyInitialized;
    BOOLEAN AlignmentPadding;
} RESIZE_DIALOG_CONTROL_WINDOW_EXTRA, *PRESIZE_DIALOG_CONTROL_WINDOW_EXTRA;
#pragma pack(pop)

//
//  This describes the data being passed into our resize helper control from
//  the dialog manager as part of its creation (only on Windows NT and
//  successor systems.)  On Windows 95 and its successors, we lose all the
//  information we need to perform layout management, and this module is
//  effectively useless.
//

#pragma pack( push )
#pragma pack( 2 )
typedef struct _RESIZE_DIALOG_CONTROL_CREATION_INFO {
    USHORT SizeOfData;
    SMALL_RECT AdjustmentToBuddyRect;
} RESIZE_DIALOG_CONTROL_CREATION_INFO, *PRESIZE_DIALOG_CONTROL_CREATION_INFO;
#pragma pack( pop )

BOOL CALLBACK
ProcessResizeOnChildren( HWND hChildWnd, LPARAM lParam );

BOOL CALLBACK
FindMetadataFromChildren( HWND hChildWnd, LPARAM lParam );

#ifdef DFC_SCROLL

//
//  This will display a sizing grip at the appropriate location on the dialog.
//
BOOL
RenderSizingGrip( HWND hDlg )
{
    RECT SizeGripArea;
    PAINTSTRUCT PaintStruct;
    HDC hDC;
    DWORD SizeBoxSize;

    //
    //  If the dialog isn't resizable, don't render the grip.
    //

    if ((GetWindowLong( hDlg, GWL_STYLE ) & WS_THICKFRAME) == 0) {
        return TRUE;
    }

    GetClientRect( hDlg, &SizeGripArea );

    SizeBoxSize = GetSystemMetrics( SM_CXHSCROLL );

    SizeGripArea.left = SizeGripArea.right - SizeBoxSize;
    SizeGripArea.top = SizeGripArea.bottom - SizeBoxSize;

    InvalidateRect( hDlg, &SizeGripArea, TRUE );

    hDC = BeginPaint( hDlg, &PaintStruct );

    DrawFrameControl( hDC, &SizeGripArea, DFC_SCROLL, DFCS_SCROLLSIZEGRIP );

    EndPaint( hDlg, &PaintStruct );

    return TRUE;
}

//
//  This will force the sizing grip to be redisplayed on resize
//
BOOL
InvalidateSizingGrip( HWND hWnd, PRECT NewSize )
{
    RECT SizeGripArea;
    DWORD SizeBoxSize;

    //
    //  First invalidate the old location of the grip.
    //

    GetClientRect( hWnd, &SizeGripArea );

    SizeBoxSize = GetSystemMetrics( SM_CXHSCROLL );

    SizeGripArea.left = SizeGripArea.right - SizeBoxSize;
    SizeGripArea.top = SizeGripArea.bottom - SizeBoxSize;

    InvalidateRect( hWnd, &SizeGripArea, FALSE );

    //
    //  Now invalidate the new location of the grip.
    //

    SizeGripArea = *NewSize;

    SizeGripArea.left = SizeGripArea.right - SizeBoxSize;
    SizeGripArea.top = SizeGripArea.bottom - SizeBoxSize;

    InvalidateRect( hWnd, &SizeGripArea, FALSE );

    return TRUE;
}

//
//  This will check if we're within the sizing grip, so that the
//  resize action will be performed if the user attempts to drag
//
BOOL
IsWithinSizingGrip( HWND hWnd, DWORD OriginalPoint )
{
    RECT SizeGripArea;
    DWORD SizeBoxSize;
    POINT MousePoint;

    //
    //  If the dialog isn't resizable, we have no grip.
    //

    if ((GetWindowLong( hWnd, GWL_STYLE ) & WS_THICKFRAME) == 0) {
        return FALSE;
    }

    MousePoint.x = GET_X_LPARAM( OriginalPoint );
    MousePoint.y = GET_Y_LPARAM( OriginalPoint );

    ScreenToClient( hWnd, &MousePoint );

    GetClientRect( hWnd, &SizeGripArea );

    SizeBoxSize = GetSystemMetrics( SM_CXHSCROLL );

    SizeGripArea.left = SizeGripArea.right - SizeBoxSize;
    SizeGripArea.top = SizeGripArea.bottom - SizeBoxSize;

    //
    //  We're in the right spot if the mouse is over the lower-right half
    //  of the sizing grip rectangle.  The first two checks place us in
    //  the rectangle, the final is to check if we're in the lower-right
    //  half.
    //

    if (MousePoint.x > SizeGripArea.left && MousePoint.x <= SizeGripArea.right &&
        MousePoint.y > SizeGripArea.top && MousePoint.y <= SizeGripArea.bottom &&
        (DWORD)(MousePoint.x + MousePoint.y - SizeGripArea.left - SizeGripArea.top) > SizeBoxSize) {

        if (GetWindowLong( hWnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) {
            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, HTBOTTOMLEFT );
        } else {
            SetWindowLongPtr( hWnd, DWLP_MSGRESULT, HTBOTTOMRIGHT );
        }
        return TRUE;
    }

    return FALSE;
}

#else

#define IsWithinSizingGrip(a, b) (FALSE)
#define InvalidateSizingGrip(a, b) (FALSE)
#define RenderSizingGrip(a) (TRUE)

#endif

//
//  This code is called by the dialog when processing its messages.  It
//  performs dialog-level operations. Returns TRUE to indicate that it
//  has processed a message and the caller should not continue; FALSE
//  indicates no processing or that the caller may continue
//

BOOL CALLBACK
ResizeDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PRESIZE_DIALOG_INFO DialogInfo;

    UNREFERENCED_PARAMETER(wParam);

    switch( uMsg ) {

        //
        //  We're done and need to clean up any state.
        //

        case WM_DESTROY:

            DialogInfo = (PRESIZE_DIALOG_INFO)GetWindowLongPtr( hDlg, DWLP_USER );

            if (DialogInfo != NULL) {
                HeapFree( GetProcessHeap(), 0, DialogInfo );
            }

            SetWindowLongPtr( hDlg, DWLP_USER, 0 );

            return FALSE;

        //
        //  We have to make sure we don't resize smaller than the initial
        //  size.  The layout management is concerned with how to allocate
        //  additional size - it won't work well if that additional size
        //  is less than zero.
        //

        case WM_GETMINMAXINFO:
            {
                LPMINMAXINFO MinMaxInfo = (LPMINMAXINFO)lParam;
                PRESIZE_DIALOG_DATA_WINDOW_EXTRA ExtraData = NULL;
                DialogInfo = (PRESIZE_DIALOG_INFO)GetWindowLongPtr( hDlg, DWLP_USER );

                EnumChildWindows( hDlg, FindMetadataFromChildren, (LPARAM)&ExtraData );

                if (ExtraData != NULL) {

                    if (ExtraData->MaximumWidthPercent != 0) {

                        MinMaxInfo->ptMaxTrackSize.x = (DialogInfo->InitialWindowRect.right - DialogInfo->InitialWindowRect.left) * ExtraData->MaximumWidthPercent / 100;

                    }

                    if (ExtraData->MaximumHeightPercent != 0) {

                        MinMaxInfo->ptMaxTrackSize.y = (DialogInfo->InitialWindowRect.bottom - DialogInfo->InitialWindowRect.top) * ExtraData->MaximumHeightPercent / 100;

                    }
                }
                MinMaxInfo->ptMinTrackSize.x = DialogInfo->InitialWindowRect.right - DialogInfo->InitialWindowRect.left;
                MinMaxInfo->ptMinTrackSize.y = DialogInfo->InitialWindowRect.bottom - DialogInfo->InitialWindowRect.top;
                return TRUE;
            }

        //
        //  Display and give effect to the sizing grip
        //

        case WM_PAINT:
            if (GetUpdateRect( hDlg, NULL, FALSE )) {
                RenderSizingGrip( hDlg );
            }
            return FALSE;

        case WM_NCHITTEST:
            return IsWithinSizingGrip( hDlg, (DWORD)lParam );

#ifdef WM_SIZING
        case WM_SIZING:
            InvalidateSizingGrip( hDlg, (PRECT)lParam );
            return FALSE;
#endif

        //
        //  Save off the initial window size and client area.  We use the
        //  initial window size to ensure the window can't be shrunk
        //  smaller than it, and we use the initial client size to
        //  determine how much the window is growing.
        //

        case WM_INITDIALOG:
            DialogInfo = (PRESIZE_DIALOG_INFO)HeapAlloc( GetProcessHeap(), 0, sizeof(*DialogInfo) );
            
            if (DialogInfo == NULL) {
                EndDialog( hDlg, -1 );
                return TRUE;
            }
            
            GetClientRect( hDlg, &DialogInfo->InitialClientRect );
            GetWindowRect( hDlg, &DialogInfo->InitialWindowRect );

            DialogInfo->NumberResizeControlsPresent = 0;

            SetWindowLongPtr( hDlg, DWLP_USER, (LONG_PTR)DialogInfo );
            break;

        //
        //  The user changed the size of the window.  Walk through all
        //  the resize helper controls and tell them this happened, and
        //  how big we were when created.  From there, the helper controls
        //  can resize their associated buddy controls appropriately.
        //

        case WM_SIZE:

            DialogInfo = (PRESIZE_DIALOG_INFO)GetWindowLongPtr( hDlg, DWLP_USER );

            RenderSizingGrip( hDlg );

            //
            //  If we "know" how many controls are present, use that.  If
            //  we don't know, start with a sensible default.
            //

            if (DialogInfo->NumberResizeControlsPresent != 0) {
                DialogInfo->hDwp = BeginDeferWindowPos( DialogInfo->NumberResizeControlsPresent );
            } else {
                DialogInfo->hDwp = BeginDeferWindowPos( 20 );
            }

            //
            //  We're enumerating again, and recalculating the current
            //  number of resize controls.
            //

            DialogInfo->NumberResizeControlsPresent = 0;

            EnumChildWindows( hDlg, ProcessResizeOnChildren, (LPARAM)DialogInfo );

            //
            //  It's possible that controls have been added or our initial
            //  guess was wrong, and we couldn't dynamically add space to
            //  accommodate the new controls.  In that case, start over
            //  and don't use the DeferWindowPos optimization.
            //

            if (DialogInfo->hDwp) {
                EndDeferWindowPos( DialogInfo->hDwp );
                DialogInfo->hDwp = NULL;
            } else {
                DialogInfo->NumberResizeControlsPresent = 0;
                EnumChildWindows( hDlg, ProcessResizeOnChildren, (LPARAM)DialogInfo );
            }
            return FALSE;

    }
    return FALSE;
}

//
//  We use this to walk through all the controls on the dialog and send the
//  appropriate messages to our resize helper controls to fix things up.
//

BOOL CALLBACK
FindMetadataFromChildren( HWND hChildWnd, LPARAM lParam )
{
    WCHAR szText[100];
    int CharsCopied;

    //
    //  GetClassName expects a count of characters, so we dance in case
    //  we're Unicode.
    //

    if (IsWindowUnicode( hChildWnd )) {

        CharsCopied = GetClassNameW( hChildWnd, (PWCHAR)szText, sizeof( szText ) / sizeof( WCHAR ));

        //
        //  If we couldn't get this into our buffer, assume it's not our
        //  class, but keep enumerating.
        //

        if (CharsCopied == 0 || CharsCopied >= (sizeof( szText ) / sizeof( WCHAR ))) {
            return TRUE;
        }

        //
        //  We only need to find one per-dialog metadata class.  If we
        //  find it, we're done.
        //

        if (wcscmp( (PWCHAR)szText, DIALOGRESIZEDATACLASSW ) == 0) {
            SendMessage( hChildWnd, WM_GETDIALOGMETADATA, 0, lParam );
            return FALSE;
        }

    } else {

        CharsCopied = GetClassNameA( hChildWnd, (PCHAR)szText, sizeof( szText ) / sizeof( CHAR ));

        //
        //  If we couldn't get this into our buffer, assume it's not our
        //  class, but keep enumerating.
        //

        if (CharsCopied == 0 || CharsCopied >= (sizeof( szText ) / sizeof( CHAR ))) {
            return TRUE;
        }

        //
        //  We only need to find one per-dialog metadata class.  If we
        //  find it, we're done.
        //

        if (strcmp( (PCHAR)szText, DIALOGRESIZEDATACLASSA ) == 0) {
            SendMessage( hChildWnd, WM_GETDIALOGMETADATA, 0, lParam );
            return FALSE;
        }
    }

    return TRUE;
}

//
//  This control exists once per resizeable dialog and specifies metadata
//  about how the dialog can be resized.
//

LRESULT CALLBACK
ResizeDialogDataWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PRESIZE_DIALOG_DATA_WINDOW_EXTRA ExtraData;

    switch( uMsg ) {
        case WM_CREATE:
        {
            LPCREATESTRUCT CreateStruct;
            PRESIZE_DIALOG_DATA_CREATION_INFO CreationInfo;

            //
            //  Allocate memory and save off the additional parameters
            //  defined for the resize metadata control.  These define
            //  how much the dialog can be resized by.
            //

            CreateStruct = (LPCREATESTRUCT)lParam;
            CreationInfo = (PRESIZE_DIALOG_DATA_CREATION_INFO)CreateStruct->lpCreateParams;

            ExtraData = (PRESIZE_DIALOG_DATA_WINDOW_EXTRA)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ExtraData) );
            if (ExtraData == NULL) {
                return -1;
            }

            //
            //  A dialog metadata control takes two WORDs as arguments to
            //  define how much the dialog should be resized.  Accordingly,
            //  if we have less than 2 * sizeof(WORD), we don't have enough
            //  data to operate.  Windows 95 and successor systems won't pass
            //  us this information through the dialog template, so if we
            //  have nothing, we don't allow the dialog to be resized.
            //

            if (CreationInfo != NULL && CreationInfo->SizeOfData >= (sizeof(WORD) * 2)) {

                //
                //  Record the amount to adjust the buddy control, and
                //  attach it to our window data.
                //

                ExtraData->MaximumWidthPercent = CreationInfo->MaximumWidthPercent;
                ExtraData->MaximumHeightPercent = CreationInfo->MaximumHeightPercent;

                //
                //  For sanity, don't allow a maximum size which is smaller
                //  than the minimum size.
                //

                if (ExtraData->MaximumWidthPercent != 0 && ExtraData->MaximumWidthPercent < 100) {
                    ExtraData->MaximumWidthPercent = 100;
                }

                if (ExtraData->MaximumHeightPercent != 0 && ExtraData->MaximumHeightPercent < 100) {
                    ExtraData->MaximumHeightPercent = 100;
                }

            } else {

                //
                //  Don't resize anything.
                //

                ExtraData->MaximumWidthPercent = 100;
                ExtraData->MaximumHeightPercent = 100;
            }

            SetWindowLongPtr( hWnd, 0, (LONG_PTR)ExtraData );
            break;
        }

        case WM_DESTROY:
            
            //
            //  Our resize helper control is being destroyed.  Tear down
            //  any data we're keeping to assist with resize operations.
            //

            ExtraData = (PRESIZE_DIALOG_DATA_WINDOW_EXTRA)GetWindowLongPtr( hWnd, 0 );
            if (ExtraData != NULL) {
                SetWindowLongPtr( hWnd, 0, 0 );
                HeapFree( GetProcessHeap(), 0, ExtraData );
            }
            break;

        case WM_GETDIALOGMETADATA:
        {
            ExtraData = (PRESIZE_DIALOG_DATA_WINDOW_EXTRA)GetWindowLongPtr( hWnd, 0 );
            *(PRESIZE_DIALOG_DATA_WINDOW_EXTRA *)lParam = ExtraData;
            return TRUE;
        }
    }
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

//
//  We use this to walk through all the controls on the dialog and send the
//  appropriate messages to our resize helper controls to fix things up.
//

BOOL CALLBACK
ProcessResizeOnChildren( HWND hChildWnd, LPARAM lParam )
{
    WCHAR szText[100];
    int CharsCopied;

    //
    //  GetClassName expects a count of characters, so we dance in case
    //  we're Unicode.
    //

    if (IsWindowUnicode( hChildWnd )) {

        CharsCopied = GetClassNameW( hChildWnd, (PWCHAR)szText, sizeof( szText ) / sizeof( WCHAR ));

        //
        //  If we couldn't get this into our buffer, assume it's not our
        //  class, but keep enumerating.
        //

        if (CharsCopied == 0 || CharsCopied >= (sizeof( szText ) / sizeof( WCHAR ))) {
            return TRUE;
        }

        if (wcscmp( (PWCHAR)szText, DIALOGRESIZECONTROLCLASSW ) == 0) {
            SendMessage( hChildWnd, WM_RESIZEPARENT, 0, lParam );
        }

    } else {

        CharsCopied = GetClassNameA( hChildWnd, (PCHAR)szText, sizeof( szText ) / sizeof( CHAR ));

        //
        //  If we couldn't get this into our buffer, assume it's not our
        //  class, but keep enumerating.
        //

        if (CharsCopied == 0 || CharsCopied >= (sizeof( szText ) / sizeof( CHAR ))) {
            return TRUE;
        }

        if (strcmp( (PCHAR)szText, DIALOGRESIZECONTROLCLASSA ) == 0) {
            SendMessage( hChildWnd, WM_RESIZEPARENT, 0, lParam );
        }
    }

    return TRUE;
}

//
//  The following code deals with how to process resize information for
//  controls on a dialog.
//

LRESULT CALLBACK
ResizeDialogControlWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hWndParent = GetParent( hWnd );

    PRESIZE_DIALOG_CONTROL_WINDOW_EXTRA ExtraData;

    switch( uMsg ) {
        case WM_CREATE:
        {
            LPCREATESTRUCT CreateStruct;
            PRESIZE_DIALOG_CONTROL_CREATION_INFO CreationInfo;

            //
            //  Allocate memory and save off the additional parameters
            //  defined for the resize helper control.  These define how
            //  much of the additional space to allocate to our buddy
            //  control. 
            //

            CreateStruct = (LPCREATESTRUCT)lParam;
            CreationInfo = (PRESIZE_DIALOG_CONTROL_CREATION_INFO)CreateStruct->lpCreateParams;

            ExtraData = (PRESIZE_DIALOG_CONTROL_WINDOW_EXTRA)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ExtraData) );
            if (ExtraData == NULL) {
                return -1;
            }

            //
            //  A dialog resize helper takes four WORDs as arguments
            //  to define how the buddy control should be resized.
            //  Accordingly, if we have less than 4 * sizeof(WORD),
            //  we don't have enough data to operate.  Windows 95 and
            //  successor systems won't pass us this information
            //  through the dialog template, so if we have nothing, we
            //  do nothing but allow the dialog to load.
            //

            if (CreationInfo != NULL && CreationInfo->SizeOfData >= sizeof(CreationInfo->AdjustmentToBuddyRect)) {

                //
                //  Record the amount to adjust the buddy control, and
                //  attach it to our window data.
                //

                ExtraData->AdjustmentToBuddyRect = CreationInfo->AdjustmentToBuddyRect;

            } else {

                //
                //  Don't resize anything.
                //

                ExtraData->AdjustmentToBuddyRect.Left = 0;
                ExtraData->AdjustmentToBuddyRect.Top = 0;
                ExtraData->AdjustmentToBuddyRect.Right = 0;
                ExtraData->AdjustmentToBuddyRect.Bottom = 0;
            }

            SetWindowLongPtr( hWnd, 0, (LONG_PTR)ExtraData );
            break;
        }

        case WM_DESTROY:
            
            //
            //  Our resize helper control is being destroyed.  Tear down
            //  any data we're keeping to assist with resize operations.
            //

            ExtraData = (PRESIZE_DIALOG_CONTROL_WINDOW_EXTRA)GetWindowLongPtr( hWnd, 0 );
            if (ExtraData != NULL) {
                SetWindowLongPtr( hWnd, 0, 0 );
                HeapFree( GetProcessHeap(), 0, ExtraData );
            }
            break;

        case WM_RESIZEPARENT:
        {
            PRESIZE_DIALOG_INFO DialogInfo = (PRESIZE_DIALOG_INFO)lParam;
            HWND hWndBuddy = GetNextWindow( hWnd, GW_HWNDNEXT );
            PRECT InitialParentRect;
            RECT CurrentParentRect;

            DWORD IncreaseHorizontal;
            DWORD IncreaseVertical;

            DWORD NewLeft;
            DWORD NewTop;
            DWORD NewWidth;
            DWORD NewHeight;

            //
            //  Initialize our brains.
            //

            ExtraData = (PRESIZE_DIALOG_CONTROL_WINDOW_EXTRA)GetWindowLongPtr( hWnd, 0 );
            InitialParentRect = &DialogInfo->InitialClientRect;

            //
            //  Count that we were here so the resizer has a good idea of
            //  how many controls to expect next time.
            //

            DialogInfo->NumberResizeControlsPresent++;

            //
            //  If this is the first time we've been sized, it must be for
            //  initial placement.  In that case, save off the initial
            //  location and size of the buddy control so we can transform
            //  it later.
            //

            if (!ExtraData->BuddyInitialized) {
                RECT buddyrect;

                //
                //  GetWindowRect returns coordinates relative to the top
                //  left of the display.  We need to transform this into
                //  coordinates relative to the top left of the dialog.
                //

                GetWindowRect( hWndBuddy, &buddyrect );

                MapWindowPoints( NULL, hWndParent, (LPPOINT)&buddyrect, 2 );

                ExtraData->InitialBuddyRect = buddyrect;

                ExtraData->BuddyInitialized = TRUE;
            }

            //
            //  Find out how big the dialog now is.
            //

            GetClientRect( hWndParent, &CurrentParentRect );

            //
            //  Now calculate how much the dialog has grown by since it
            //  was created.
            //

            IncreaseHorizontal = (CurrentParentRect.right - InitialParentRect->right);
            IncreaseVertical = (CurrentParentRect.bottom - InitialParentRect->bottom);

            //
            //  And from there, calculate where our buddy control belongs.
            //  We take the initial positions and add the adjustment as a percentage.
            //

            NewLeft = ExtraData->InitialBuddyRect.left + IncreaseHorizontal * ExtraData->AdjustmentToBuddyRect.Left / 100;
            NewTop = ExtraData->InitialBuddyRect.top + IncreaseVertical * ExtraData->AdjustmentToBuddyRect.Top / 100;
            NewWidth = (ExtraData->InitialBuddyRect.right - ExtraData->InitialBuddyRect.left) + IncreaseHorizontal * ExtraData->AdjustmentToBuddyRect.Right / 100;
            NewHeight = (ExtraData->InitialBuddyRect.bottom - ExtraData->InitialBuddyRect.top) + IncreaseVertical * ExtraData->AdjustmentToBuddyRect.Bottom / 100;

            //
            //  We're going to be moving and resizing our buddy, so make sure
            //  it repaints itself.
            //

            InvalidateRect( hWndBuddy, NULL, TRUE );

            //
            //  Now position the buddy control.  We will typically optimize this
            //  via DeferWindowPos, which will move all the controls when we're
            //  done; but if we can't do that, we'll move it here.
            //

            if (DialogInfo->hDwp != NULL) {
                DialogInfo->hDwp = DeferWindowPos( DialogInfo->hDwp,
                    hWndBuddy,
                    NULL,
                    NewLeft,
                    NewTop,
                    NewWidth,
                    NewHeight,
                    SWP_NOZORDER | SWP_NOACTIVATE );
            }

            //
            //  If the DeferWindowPos structure was not set up, or if we just
            //  tore it down above by adding more controls than it could deal
            //  with, move manually here.
            //

            if (DialogInfo->hDwp == NULL) {

                MoveWindow( hWndBuddy,
                    NewLeft,
                    NewTop,
                    NewWidth,
                    NewHeight,
                    TRUE );
            }

            break;
        }

    }
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


BOOL
ResizeDialogInitialize( HINSTANCE hInst )
{
    WNDCLASSW ResizeDialogClassW;
    WNDCLASSA ResizeDialogClassA;

    //
    //  Here we register our resize helper control class - twice.
    //  Once for unicode, once for non-unicode systems.
    //  This control is used to define how buddy controls should
    //  be manipulated when resizes occur.
    //

    ZeroMemory( &ResizeDialogClassW, sizeof( ResizeDialogClassW ));

    ResizeDialogClassW.style = 0;
    ResizeDialogClassW.lpfnWndProc = ResizeDialogControlWindowProc;
    ResizeDialogClassW.cbClsExtra = 0;
    ResizeDialogClassW.cbWndExtra = sizeof(PVOID);
    ResizeDialogClassW.hInstance = hInst;
    ResizeDialogClassW.hIcon = NULL;
    ResizeDialogClassW.hCursor = NULL;
    ResizeDialogClassW.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ResizeDialogClassW.lpszMenuName = NULL;
    ResizeDialogClassW.lpszClassName = DIALOGRESIZECONTROLCLASSW;

    RegisterClassW( &ResizeDialogClassW );

    ZeroMemory( &ResizeDialogClassA, sizeof( ResizeDialogClassA ));

    ResizeDialogClassA.style = 0;
    ResizeDialogClassA.lpfnWndProc = ResizeDialogControlWindowProc;
    ResizeDialogClassA.cbClsExtra = 0;
    ResizeDialogClassA.cbWndExtra = sizeof(PVOID);
    ResizeDialogClassA.hInstance = hInst;
    ResizeDialogClassA.hIcon = NULL;
    ResizeDialogClassA.hCursor = NULL;
    ResizeDialogClassA.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ResizeDialogClassA.lpszMenuName = NULL;
    ResizeDialogClassA.lpszClassName = DIALOGRESIZECONTROLCLASSA;

    RegisterClassA( &ResizeDialogClassA );

    ZeroMemory( &ResizeDialogClassW, sizeof( ResizeDialogClassW ));

    ResizeDialogClassW.style = 0;
    ResizeDialogClassW.lpfnWndProc = ResizeDialogDataWindowProc;
    ResizeDialogClassW.cbClsExtra = 0;
    ResizeDialogClassW.cbWndExtra = sizeof(PVOID);
    ResizeDialogClassW.hInstance = hInst;
    ResizeDialogClassW.hIcon = NULL;
    ResizeDialogClassW.hCursor = NULL;
    ResizeDialogClassW.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ResizeDialogClassW.lpszMenuName = NULL;
    ResizeDialogClassW.lpszClassName = DIALOGRESIZEDATACLASSW;

    RegisterClassW( &ResizeDialogClassW );

    ZeroMemory( &ResizeDialogClassA, sizeof( ResizeDialogClassA ));

    ResizeDialogClassA.style = 0;
    ResizeDialogClassA.lpfnWndProc = ResizeDialogDataWindowProc;
    ResizeDialogClassA.cbClsExtra = 0;
    ResizeDialogClassA.cbWndExtra = sizeof(PVOID);
    ResizeDialogClassA.hInstance = hInst;
    ResizeDialogClassA.hIcon = NULL;
    ResizeDialogClassA.hCursor = NULL;
    ResizeDialogClassA.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ResizeDialogClassA.lpszMenuName = NULL;
    ResizeDialogClassA.lpszClassName = DIALOGRESIZEDATACLASSA;

    RegisterClassA( &ResizeDialogClassA );

    return TRUE;
}

