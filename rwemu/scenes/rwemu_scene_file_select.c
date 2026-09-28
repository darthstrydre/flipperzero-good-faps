#include "../rwemu_app_i.h"
#include "furi_hal_power.h"

static bool rwemu_file_select(RWEmuApp* app) {
    furi_assert(app);

    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(
        &browser_options, RWEMU_APP_EXTENSION, &I_rwemu_10px);
    browser_options.base_path = RWEMU_APP_PATH_FOLDER;
    browser_options.hide_ext = false;

    // Input events and views are managed by file_select
    bool res = dialog_file_browser_show(
        app->dialogs, app->file_path, app->file_path, &browser_options);
    return res;
}

void rwemu_scene_file_select_on_enter(void* context) {
    RWEmuApp* app = context;

    if(rwemu_file_select(app)) {
        if(!furi_hal_usb_is_locked()) {
            scene_manager_next_scene(app->scene_manager, RWEmuSceneWork);
        } else {
            scene_manager_next_scene(app->scene_manager, RWEmuSceneUsbLocked);
        }
    } else {
        scene_manager_previous_scene(app->scene_manager);
    }
}

bool rwemu_scene_file_select_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    // RWEmuApp* app = context;
    return false;
}

void rwemu_scene_file_select_on_exit(void* context) {
    UNUSED(context);
}
