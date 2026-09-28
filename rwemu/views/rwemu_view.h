#pragma once

#include <gui/view.h>

typedef struct RWEmuView RWEmuView;

RWEmuView* rwemu_alloc();

void rwemu_free(RWEmuView* view);

View* rwemu_get_view(RWEmuView* view);

void rwemu_set_file_name(RWEmuView* view, FuriString* name);

void rwemu_set_stats(RWEmuView* view, uint32_t read, uint32_t written);

void rwemu_set_connection_error(RWEmuView* view);
void rwemu_clear_connection_error(RWEmuView* view);
