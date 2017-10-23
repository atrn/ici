/*
 * This file manages open source files.
 */
#include <ici.h>
#ifndef NODEBUGGING
#include <windows.h>
#include "widb-priv.h"
//#include "object.h"
#include "widb_sources.h"
#include "widb_wnd.h"
#include "resource.h"
#include <search.h>
#include <stdio.h>


/*
 * Each file that we've read into the debugger is represented by one of these
 * structures.
 *
 * Only one of these sources is visible at once, hence only one 'list' is
 * WM_VISIBLE.
 */
typedef struct
{
    char path[_MAX_PATH];
    HWND list;
} Source;
/*
 * path     The path to the loaded source file.
 * list     The list box that represents it.
 */

/*
 * This is the array of sources.  Nicely sorted by qsort.
 */
Source sources[WIDB_MAX_SOURCES];
int cur_source_index = -1;
int num_sources;                // Number of valid entries in 'sources'.


/*
 * A function used by qsort() and bsearch() to compare elements in the 
 * 'sources' array.
 */
static int compare_sources(void const *s1, void const *s2)
{
    return strcmp(((Source *)s1)->path, ((Source *)s2)->path);
}


/*
 * makes sure that the user can see a particular source line in a source file,
 * if this is so it returns TRUE, otherwise FALSE.
 */
BOOL widb_show_file_line(char const *filename, long line_num)
{
    Source new_source;
    Source *source;
    int new_source_index;

    // Have we encountered this file before (and loaded it)?
    if (filename == NULL)
        return FALSE;
    strcpy(new_source.path, filename);
    source = (Source *)lfind
    (
        &new_source,
        &sources[0],
        &num_sources,
        sizeof(Source),
        compare_sources
    );

    if (source == NULL)
    {
        int id;

        //
        // No it's not, we must create this new source.
        //
        _ASSERT(num_sources < WIDB_MAX_SOURCES);
        new_source_index = num_sources ++;
        sources[new_source_index] = new_source;
        source = &sources[new_source_index];
        id = IDC_WND_LIST + new_source_index;

        // Create a list box for this source.
        {
            RECT rect;
            GetClientRect(widb_wnd, &rect);
            // Create our list box.
            source->list = CreateWindow
            (
                "LISTBOX",
                "",
                WS_CHILD | WS_VSCROLL | LBS_USETABSTOPS |
                LBS_NOINTEGRALHEIGHT,
                0,
                0,
                rect.right,     // Client rectangle is 0,0
                rect.bottom,    // Client rectangle is 0,0
                widb_wnd,
                (HMENU)id,
                widb_hInst,
                NULL
            );
            _ASSERT(source->list != NULL);
        }
    
        // Append this source to the "Window" menu (an item's number in the
        // menu is its control ID index in 'sources').
        {
            HMENU menu_bar, win_menu;
            
            menu_bar = GetMenu(widb_wnd);
            _ASSERT(menu_bar != NULL);
            win_menu = GetSubMenu(menu_bar, WINDOW_MENU_INDEX);
            _ASSERT(win_menu != NULL);
            VERIFY(AppendMenu(win_menu, MF_STRING, id, filename));
        }

        // Load the file into it.
        {
            FILE *f;
            char line[256];

            // Is it a relative path?
            char full_path[_MAX_PATH];
            if
            (
                strlen(filename) < 2
                ||
                !(isalpha(filename[0]) && filename[1] == ':')   // C:
                &&
                !(filename[0] == '\\' && filename[1] == '\\')   // \\asda\asd\s
            )
            {
                (void) sprintf(full_path, "%s\\%s", widb_old_dir, filename);
            }
            else
            {
                strcpy(full_path, filename);
            }

            f = fopen(full_path, "r");
            if (f == NULL)
            {
                // Can't load it in.
                SendMessage(source->list, LB_ADDSTRING, 0, (LPARAM)"Couldn't load");
                SendMessage(source->list, LB_ADDSTRING, 0, (LPARAM)filename);
                return FALSE;
            }

            // Success, add each line as an item in the list box.
            while (NULL != fgets(line, sizeof(line), f))
            {
                // The last character should be a newline.
                int line_length = strlen(line);
                if (line[line_length - 1] == '\n')
                {
                    // Yes, remove it.
                    line[line_length - 1] = '\0';
                }
                else
                {
                    int c;

                    // No?  Discard all characters until we find a newline (other-
                    // wise all the line numbers would be off by one).
                    do
                        c = fgetc(f);
                    while (c != EOF && c != '\n');
                }

                // Add it to the list box.
                SendMessage(source->list, LB_ADDSTRING, 0, (LPARAM)line);
            }

            fclose(f);
        }
    }
    else
    {
        // Found it, what's its index?
        new_source_index = source - &sources[0];
    }

    // The current source line is marked by a selection, so we must deselect
    // any.
    if (new_source_index != cur_source_index && cur_source_index != -1)
    {
        SendMessage(sources[cur_source_index].list, LB_SETCURSEL, -1, 0);
    }

    // Now we have the index of the file in our list.
    widb_show_source(new_source_index);

    // Set the selection to the current line number.
    SendMessage(source->list, LB_SETCURSEL, line_num - 1, 0);

    return TRUE;
}


/*
 * Called to ensure that the current line is visible (handy after a resize
 * or restore.
 */
void widb_show_current_line()
{
    HWND list = sources[cur_source_index].list;
    SendMessage
    (
        list,
        LB_SETTOPINDEX,
        SendMessage(list, LB_GETCURSEL, 0, 0),
        0
    );
}


/*
 * Called by the main debugger window when it gets a menu command ID
 * corresponding to an item in the "Windows" menu.  This means we must show
 * whichever source the user selected.
 *
 * Commands are in the range IDC_WND_LIST..IDC_WND_LIST+WIDB_MAX_SOURCES
 */
void widb_show_source(int new_source_index)
{
    if (new_source_index != cur_source_index)
    {
        // But it isn't the current source file
        if (cur_source_index != -1)
        {
            // Hide the old one.
            ShowWindow(sources[cur_source_index].list, SW_HIDE);
        }
        ShowWindow(sources[new_source_index].list, SW_SHOW);
        cur_source_index = new_source_index;
    }
}


/*
 * Called when the main window resizes, we must resize all the list boxes to fit.
 */
void widb_resize_sources(int cx, int cy)
{
    RECT r;
    int i;
    
    VERIFY(GetClientRect(widb_wnd, &r));

    for (i = 0; i < num_sources; i ++)
    {
        SetWindowPos
        (
            sources[i].list,
            NULL,
            0, 0, r.right, r.bottom,
            SWP_NOACTIVATE | SWP_NOZORDER
        );
    }

    // Make sure the current line is still visible.
    widb_show_current_line();
}


/*
 * This must be called when the debugger's window is closed because at that
 * point all loaded source lists are also automatically closed.
 */
void
widb_uninit_sources()
{
    cur_source_index = -1;
    num_sources = 0;
}

#endif
