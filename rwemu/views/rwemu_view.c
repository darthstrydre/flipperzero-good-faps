#include "rwemu_view.h"
#include "../rwemu_app_i.h"
#include <gui/elements.h>

struct RWEmuViewImpl {
    View* view;
};

typedef struct {
    FuriString *file_name, *status_string;
    uint32_t read_speed, write_speed;
    uint32_t bytes_read, bytes_written;
    uint32_t update_time;
    bool connection_error;
} RWEmuViewModel;

static void append_suffixed_byte_count(FuriString* string, uint32_t count) {
    if(count < 1024) {
        furi_string_cat_printf(string, "%luB", count);
    } else if(count < 1024 * 1024) {
        furi_string_cat_printf(string, "%luK", count / 1024);
    } else if(count < 1024 * 1024 * 1024) {
        furi_string_cat_printf(string, "%.3fM", (double)count / (1024 * 1024));
    } else {
        furi_string_cat_printf(string, "%.3fG", (double)count / (1024 * 1024 * 1024));
    }
}

static void rwemu_view_draw_callback(Canvas* canvas, void* _model) {
    RWEmuViewModel* model = _model;

    if(model->connection_error) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(
            canvas, canvas_width(canvas) / 2, 12, AlignCenter, AlignCenter, "USB Error");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(
            canvas, canvas_width(canvas) / 2, 28, AlignCenter, AlignCenter, "Check your USB cable");
        canvas_draw_str_aligned(
            canvas, canvas_width(canvas) / 2, 42, AlignCenter, AlignCenter, "and try again");
        elements_button_left(canvas, "Back");
        return;
    }

    canvas_draw_icon(canvas, 8, 14, &I_Drive_112x35);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, canvas_width(canvas) / 2, 0, AlignCenter, AlignTop, "RW Emu");

    canvas_set_font(canvas, FontSecondary);
    elements_string_fit_width(canvas, model->file_name, 89 - 2);
    canvas_draw_str_aligned(
        canvas, 50, 23, AlignCenter, AlignBottom, furi_string_get_cstr(model->file_name));

    furi_string_set_str(model->status_string, "R:");
    append_suffixed_byte_count(model->status_string, model->bytes_read);
    if(model->read_speed) {
        furi_string_cat_str(model->status_string, "; ");
        append_suffixed_byte_count(model->status_string, model->read_speed);
        furi_string_cat_str(model->status_string, "ps");
    }
    canvas_draw_str(canvas, 12, 34, furi_string_get_cstr(model->status_string));

    furi_string_set_str(model->status_string, "W:");
    append_suffixed_byte_count(model->status_string, model->bytes_written);
    if(model->write_speed) {
        furi_string_cat_str(model->status_string, "; ");
        append_suffixed_byte_count(model->status_string, model->write_speed);
        furi_string_cat_str(model->status_string, "ps");
    }
    canvas_draw_str(canvas, 12, 44, furi_string_get_cstr(model->status_string));
}

RWEmuView* rwemu_alloc() {
    RWEmuViewImpl* impl = malloc(sizeof(RWEmuViewImpl));

    impl->view = view_alloc();
    view_allocate_model(impl->view, ViewModelTypeLocking, sizeof(RWEmuViewModel));
    with_view_model(
        impl->view,
        RWEmuViewModel * model,
        {
            model->file_name = furi_string_alloc();
            model->status_string = furi_string_alloc();
        },
        false);
    view_set_context(impl->view, impl);
    view_set_draw_callback(impl->view, rwemu_view_draw_callback);

    return impl;
}

void rwemu_free(RWEmuView* view) {
    furi_assert(view);
    with_view_model(
        view->view,
        RWEmuViewModel * model,
        {
            furi_string_free(model->file_name);
            furi_string_free(model->status_string);
        },
        false);
    view_free(view->view);
    free(view);
}

View* rwemu_get_view(RWEmuView* view) {
    furi_assert(view);
    return view->view;
}

void rwemu_set_file_name(RWEmuView* view, FuriString* name) {
    furi_assert(name);
    with_view_model(
        view->view,
        RWEmuViewModel * model,
        { furi_string_set(model->file_name, name); },
        true);
}

void rwemu_set_connection_error(RWEmuView* view) {
    with_view_model(
        view->view, RWEmuViewModel * model, { model->connection_error = true; }, true);
}

void rwemu_clear_connection_error(RWEmuView* view) {
    with_view_model(
        view->view, RWEmuViewModel * model, { model->connection_error = false; }, false);
}

void rwemu_set_stats(RWEmuView* view, uint32_t read, uint32_t written) {
    with_view_model(
        view->view,
        RWEmuViewModel * model,
        {
            uint32_t now = furi_get_tick();
            model->read_speed = (read - model->bytes_read) * 1000 / (now - model->update_time);
            model->write_speed =
                (written - model->bytes_written) * 1000 / (now - model->update_time);
            model->bytes_read = read;
            model->bytes_written = written;
            model->update_time = now;
        },
        true);
}
