#ifndef WIDB_WND_H
#define WIDB_WND_H

extern HINSTANCE widb_hInst;
extern HWND widb_wnd;
extern int widb_step_over_depth;
extern ici_obj_t *widb_scope;
extern char widb_old_dir[_MAX_PATH];
void widb_debug(ici_src_t *src);
void widb_wnd_ensure_window_available();
#define WINDOW_MENU_INDEX 3

#endif
