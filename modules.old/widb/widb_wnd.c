#ifndef NODEBUGGING
/*#include "fwd.h"
#include "set.h"
#include "exec.h"
#include "src.h"
#include "str.h"
*/
#include <ici.h>
#include <windows.h>
#include "widb-priv.h"

#include "widb_wnd.h"
#include "widb_sources.h"
#include "widb.h"
#include "widb_ici.h"
#include "resource.h"
#include <memory.h>
#include <direct.h>
#include <commctrl.h>


// Foward declarations of functions included in this code module:
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK about_wnd_proc(HWND, UINT, WPARAM, LPARAM);

/*
 * Signal used by the debug thread to tell the main thread that it should
 * continue running ICI code.
 */
HANDLE wnd_resume_app_thread;

/*
 * The application being debugger (which we are part of).
 */
HINSTANCE widb_hInst;

/*
 * Handle to the main window of the debugger.
 */
HWND widb_wnd;

/*
 * Whether the debugger is currently stopped in ICI code.
 */
BOOL wnd_in_debugger = FALSE;

/*
 * Handles to menus in the main debugger window.
 */
HMENU wnd_view_menu;
HMENU wnd_debug_menu;

/*
 * The depth of calls that we should ignore (implements step over vs step into).
 */
int widb_step_over_depth = 99999;

/*
 * The most recent scope we've encountered.
 */
ici_obj_t *widb_scope;

/*
 * The directory that was current when we were initialised.
 */
char widb_old_dir[_MAX_PATH];


/* EXTERN
 * init_window - Prepare for ICI debugging.
 *
 * Creates an invisible ICI debugger window, this window will be displayed
 * when the user starts debugging or invokes ICI_debug_show().
 *
 * Returns:
 * Whether initialisation was successful.
 */
static BOOL
init_window()
{
    static BOOL first_time = TRUE;
    static HMENU menu;
    char const *window_class_name = "widb";

    widb_hInst = GetModuleHandle("ici4widb.dll");

    /* If resources haven't been previously provided we assume they're in
     * the same modules as the executable. */
    if (widb_resources == NULL)
        widb_resources = widb_hInst;

    // Remember the current directory.
    getcwd(widb_old_dir, sizeof(widb_old_dir));

    if (first_time)
    {
        WNDCLASSEX wcex;

        // Initialise common controls.  Normally MFC does this for you, but if you're
        // not using MFC...
        {
    //        INITCOMMONCONTROLSEX init_ctrls;
    //        memset(&init_ctrls, 0, sizeof(init_ctrls));
    //        init_ctrls.dwSize = sizeof(init_ctrls);
    //        init_ctrls.dwICC = ICC_WIN95_CLASSES;
    //        VERIFY(InitCommonControlsEx(&init_ctrls));

            // I had to change to this because I couldn't debug properly on
            // Win95 with PhotoRecord.  Hope it still works in non-MFC stuff.
            InitCommonControls();
        }

        // Register the new window class.
        wcex.cbSize        = sizeof(WNDCLASSEX);
        wcex.style         = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc   = (WNDPROC)wnd_proc;
        wcex.cbClsExtra    = 0;
        wcex.cbWndExtra    = 0;
        wcex.hInstance     = widb_hInst;
        wcex.hIcon         = NULL;
        wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName  = NULL;
        wcex.lpszClassName = window_class_name;
        wcex.hIconSm       = NULL;
        if (!RegisterClassEx(&wcex))
            return FALSE;

        // Load the menu.
        menu = LoadMenu(widb_resources, MAKEINTRESOURCE(IDM_WIDB));
        _ASSERT(menu != NULL);

        // Retrieve sub-menus for use by enabling/disabling functions.
        wnd_view_menu = GetSubMenu(menu, 1);
        wnd_debug_menu = GetSubMenu(menu, 2);

        first_time = FALSE;
    }

    // Create the window.
    widb_wnd = CreateWindow
    (
        window_class_name,
        "Windows ICI Debugger",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        NULL,
        menu,
        widb_hInst,
        NULL
    );
    if (widb_wnd == NULL)
        return FALSE;

    return TRUE;
}


static DWORD WINAPI
debug_thread_proc(LPVOID lpParameter)
{
    HANDLE hAccelTable = NULL;
    MSG msg;

    // Create the debugger window.
    VERIFY(init_window());

    // Tell the main thread that we've initialised (that is, widb_wnd is
    // now valid).
    VERIFY(SetEvent(wnd_resume_app_thread));

    // Load the menu accelerators (ie. keyboard shortcuts).
    hAccelTable = LoadAccelerators(widb_resources, MAKEINTRESOURCE(IDA_WIDB));

    // Process messages until WM_QUIT.
    while (GetMessage(&msg, NULL, 0, 0))
    {
        // process this message
        if (!TranslateAccelerator(widb_wnd/*msg.hwnd*/, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}


/* INTERN
 * widb_debug - Gives the user the debugging window if needed and blocks the
 *              calling thread until the debugger is ready to continue running
 *              ICI code.
 *
 */
void
widb_debug(ici_src_t *src)
{
    DWORD result;
    static HANDLE thread;

    // Does the thread exist?
    if (thread == NULL)
    {
        DWORD thread_id;

        // Create event that the thread uses to tell us when it's done its
        // work.
        //
        // The first time the thread signals this means that it has created
        // its main window (and filled in widb_wnd).  Subsequent times
        // are when the debugger wants us (the main thread) to run again.
        wnd_resume_app_thread = CreateEvent(NULL, FALSE, FALSE, NULL);
        _ASSERT(wnd_resume_app_thread != NULL);

        // Create the thread.
        thread = CreateThread
                 (
                     NULL,
                     0,
                     debug_thread_proc,
                     NULL,
                     0,
                     &thread_id
                 );
        _ASSERT(thread != NULL);

        // Wait until the thread has initialised so we've got access to
        // widb_wnd.
        result = WaitForSingleObject(wnd_resume_app_thread, INFINITE);
        _ASSERT(result == WAIT_OBJECT_0);
    }

    // Tell the debugger what line we're stopped at.
    {
        char *filename = NULL;
        if (src->s_filename != NULL)
            filename = src->s_filename->s_chars;
        PostMessage(widb_wnd, WM_USER, (WPARAM)filename, (LPARAM)src->s_lineno);
    }

    // Wait until the debugger's ready for us to continue.
    result = WaitForSingleObject(wnd_resume_app_thread, INFINITE);
    _ASSERT(result == WAIT_OBJECT_0);
}


/*
 * Enables/disables menu items that should only be available while the
 * application being debugged is paused.
 */
static void
enable_debug_menus(BOOL enabled)
{
    int enable_flag = enabled ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem
    (
        wnd_view_menu,  ID_WIDB_VIEW_SCOPE,     MF_BYCOMMAND | enable_flag
    );
    EnableMenuItem
    (
        wnd_view_menu,  ID_WIDB_VIEW_EXECSTACK, MF_BYCOMMAND | enable_flag
    );
    EnableMenuItem
    (
        wnd_debug_menu, ID_WIDB_DEBUG_GO,       MF_BYCOMMAND | enable_flag
    );
    EnableMenuItem
    (
        wnd_debug_menu, ID_WIDB_DEBUG_STEPINTO, MF_BYCOMMAND | enable_flag
    );
    EnableMenuItem
    (
        wnd_debug_menu, ID_WIDB_DEBUG_STEPOVER, MF_BYCOMMAND | enable_flag
    );
    EnableMenuItem
    (
        wnd_debug_menu, ID_WIDB_DEBUG_STEPOUT,  MF_BYCOMMAND | enable_flag
    );
}


/*
 * Called when the user runs the ICI program again (even to execute just
 * one line).  It means we need to restart the application thread and disable
 * any debugging functions that will mess with ICI data (ICI is not thread
 * safe).
 */
static void
resume_application(HWND old_foreground_window)
{
    // Disable menus that shouldn't be used while the application thread
    // is running.
    wnd_in_debugger = FALSE;
    enable_debug_menus(FALSE);

    // Move the debugger back out of the way.
    if (old_foreground_window != NULL)
        SetForegroundWindow(old_foreground_window);

    // Give control back to the main thread.
    SetEvent(wnd_resume_app_thread);
}


/* INTERN
 * wnd_proc - Callback for the debugger's main window.
 *
 * Parameters:
 * hWnd, message, wParam, lParam
 *              Standard Windows window callback procedure.
 *
 * Returns:
 * Whatever is normal for Windows callbacks.
 */
LRESULT CALLBACK wnd_proc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND old_foreground_wnd = NULL;

    switch (message)
    {
        case WM_CREATE:
            // Add a menu for the open Windows.  We'd put an empty popup
            // menu in the resources but it seems to get de-popup'd because
            // it's empty.
            {
                HMENU menu_bar = GetMenu(wnd);
                _ASSERT(menu_bar != NULL);
                VERIFY
                (
                    InsertMenu
                    (
                        menu_bar,
                        WINDOW_MENU_INDEX,
                        MF_BYPOSITION | MF_STRING | MF_POPUP,
                        (UINT)CreateMenu(),
                        "&Window"
                    )
                );
            }

            break;

        // This message tells us to start debugging.
        case WM_USER:
            wnd_in_debugger = TRUE;

            // Make debugging menus available.
            enable_debug_menus(TRUE);

            // Show the appropriate source line in the debugger.
            widb_show_file_line((char *)wParam, lParam);

            // Bring the debugger to the front.
            old_foreground_wnd = GetForegroundWindow();
            if (!IsWindowVisible(widb_wnd))
                ShowWindow(widb_wnd, SW_SHOW);
            if (IsIconic(widb_wnd))
            {
                ShowWindow(widb_wnd, SW_RESTORE);

                // The current line was selected while the window was minimised, so
                // it may not be visible.
                widb_show_current_line();
            }
            SetForegroundWindow(widb_wnd);

            break;

        case WM_COMMAND:
        {
            WORD wmId, wmEvent;

            wmId    = LOWORD(wParam); // Remember, these are...
            wmEvent = HIWORD(wParam); // ...different for Win32!

            //Parse the menu selections:
            switch (wmId)
            {

                case ID_WIDB_ABOUT:
                    DialogBox
                    (
                        widb_resources,
                        MAKEINTRESOURCE(IDD_WIDB_ABOUTBOX),
                        wnd,
                        (DLGPROC)about_wnd_proc
                    );
                    break;

                case WM_SYSCOMMAND:
                    if (wParam != SC_CLOSE)
                        return DefWindowProc(wnd, message, wParam, lParam);

                    // FALL THROUGH to "closing" the window.

                case WM_CLOSE:
                case ID_WIDB_EXIT:
                    // They don't want the debugger?  We'll let them think
                    // it's gone!  He, he, he...
                    ShowWindow(wnd, SW_HIDE);

                    // FALL THROUGH to Go since they don't want to be stopped.

                case ID_WIDB_DEBUG_GO:
                    if (wnd_in_debugger)
                    {
                        // Make sure we don't stop at lines until the user
                        // calls debug_break() again.
                        widb_step_over_depth = 99999;

                        resume_application(old_foreground_wnd);
                    }
                    break;

                case ID_WIDB_VIEW_SCOPE:
                    if (wnd_in_debugger)
                        WIDB_view_object(widb_scope, wnd);
                    break;

                case ID_WIDB_VIEW_EXECSTACK:
                    if (wnd_in_debugger)
                        WIDB_view_object(objof(widb_exec_name_stack), wnd);
                    break;

                case ID_WIDB_DEBUG_BREAK:
                    // Stop at the next line.
                    widb_step_over_depth = -99999;
                    break;

                case ID_WIDB_DEBUG_STEPINTO:
                    if (wnd_in_debugger)
                    {
                        // Stop at the next line.
                        widb_step_over_depth = -99999;

                        resume_application(old_foreground_wnd);
                    }
                    break;

                case ID_WIDB_DEBUG_STEPOVER:
                    if (wnd_in_debugger)
                    {
                        // Stop at the next call, but ignore anything called.
                        widb_step_over_depth = 0;

                        resume_application(old_foreground_wnd);
                    }
                    break;

                case ID_WIDB_DEBUG_STEPOUT:
                    if (wnd_in_debugger)
                    {
                        // Run until the current routine ends.
                        widb_step_over_depth = 1;

                        resume_application(old_foreground_wnd);
                    }
                    break;

                // Here are all the other possible menu options,
                // all of these are currently disabled:
                case ID_WIDB_OPEN:
                case ID_WIDB_CLOSE:
                    MessageBox(wnd, "Not Implemented", "really", 0);
                    break;

                default:
                    if
                    (
                        wmId >= IDC_WND_LIST &&
                        wmId < IDC_WND_LIST + WIDB_MAX_SOURCES
                    )
                    {
                        // This is an item in the Window menu.
                        widb_show_source(wmId - IDC_WND_LIST);
                    }

                    return DefWindowProc(wnd, message, wParam, lParam);
            }
            break;
        }
        case WM_DESTROY:
            // Closing this window closes all the source windows too.
            widb_uninit_sources();

            // We're out of here, we better make sure no-one tries to reference
            // us.
            widb_wnd = NULL;
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(wnd, &ps);
            EndPaint(wnd, &ps);
            break;
        }
        case WM_SIZE:
            widb_resize_sources(LOWORD(lParam), HIWORD(lParam));
            break;

        default:
            return (DefWindowProc(wnd, message, wParam, lParam));
   }
   return (0);
}


//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
//       This version allows greater flexibility over the contents of the 'About' box,
//       by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
// WM_INITDIALOG - initialize dialog box
// WM_COMMAND    - Input received
//
//
static LRESULT CALLBACK about_wnd_proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, TRUE);
                return (TRUE);
            }
            break;
    }

    return FALSE;
}

#endif
