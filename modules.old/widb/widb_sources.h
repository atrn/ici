#ifndef WIDB_SOURCES_H
#define WIDB_SOURCES_H

#define WIDB_MAX_SOURCES 40
BOOL widb_show_file_line(char const *filename, long line_num);
void widb_show_current_line();
void widb_show_source(int new_source_index);
void widb_resize_sources(int cx, int cy);
void widb_uninit_sources();

#endif
