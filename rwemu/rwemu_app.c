#include "rwemu_app_i.h"
#include <furi.h>
#include <storage/storage.h>
#include <lib/toolbox/path.h>

static bool rwemu_app_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    RWEmuApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool rwemu_app_back_event_callback(void* context) {
    furi_assert(context);
    RWEmuApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void rwemu_app_tick_event_callback(void* context) {
    furi_assert(context);
    RWEmuApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

void rwemu_app_show_loading_popup(RWEmuApp* app, bool show) {
    if(show) {
        // Raise timer priority so that animations can play
        furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);
        view_dispatcher_switch_to_view(app->view_dispatcher, RWEmuAppViewLoading);
    } else {
        // Restore default timer priority
        furi_timer_set_thread_priority(FuriTimerThreadPriorityNormal);
    }
}

RWEmuApp* rwemu_app_alloc(char* arg) {
    RWEmuApp* app = malloc(sizeof(RWEmuApp));
    app->file_path = furi_string_alloc();

    if(arg != NULL) {
        furi_string_set_str(app->file_path, arg);
    } else {
        furi_string_set_str(app->file_path, RWEMU_APP_PATH_FOLDER);
    }

    app->gui = furi_record_open(RECORD_GUI);
    app->fs_api = furi_record_open(RECORD_STORAGE);
    app->dialogs = furi_record_open(RECORD_DIALOGS);

    app->view_dispatcher = view_dispatcher_alloc();

    app->scene_manager = scene_manager_alloc(&rwemu_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, rwemu_app_tick_event_callback, 500);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, rwemu_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, rwemu_app_back_event_callback);

    app->rw_view = rwemu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        RWEmuAppViewWork,
        rwemu_get_view(app->rw_view));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, RWEmuAppViewTextInput, text_input_get_view(app->text_input));

    app->loading = loading_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, RWEmuAppViewLoading, loading_get_view(app->loading));

    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        RWEmuAppViewStart,
        variable_item_list_get_view(app->variable_item_list));

    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, RWEmuAppViewWidget, widget_get_view(app->widget));

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    if(storage_file_exists(app->fs_api, furi_string_get_cstr(app->file_path))) {
        if(!furi_hal_usb_is_locked()) {
            scene_manager_next_scene(app->scene_manager, RWEmuSceneWork);
        } else {
            scene_manager_next_scene(app->scene_manager, RWEmuSceneUsbLocked);
        }
    } else {
        scene_manager_next_scene(app->scene_manager, RWEmuSceneStart);
    }

    return app;
}

void rwemu_app_free(RWEmuApp* app) {
    furi_assert(app);

    // Views
    view_dispatcher_remove_view(app->view_dispatcher, RWEmuAppViewWork);
    view_dispatcher_remove_view(app->view_dispatcher, RWEmuAppViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, RWEmuAppViewStart);
    view_dispatcher_remove_view(app->view_dispatcher, RWEmuAppViewLoading);
    view_dispatcher_remove_view(app->view_dispatcher, RWEmuAppViewWidget);

    rwemu_free(app->rw_view);
    text_input_free(app->text_input);
    variable_item_list_free(app->variable_item_list);
    loading_free(app->loading);
    widget_free(app->widget);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    furi_string_free(app->file_path);

    // Close records
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_DIALOGS);

    free(app);
}

int32_t rwemu_app(void* p) {
    RWEmuApp* app = rwemu_app_alloc((char*)p);
    view_dispatcher_run(app->view_dispatcher);
    rwemu_app_free(app);
    return 0;
}
