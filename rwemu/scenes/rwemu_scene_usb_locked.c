#include "../rwemu_app_i.h"

void rwemu_scene_usb_locked_on_enter(void* context) {
    RWEmuApp* app = context;

    widget_add_icon_element(app->widget, 78, 0, &I_ActiveConnection_50x64);
    widget_add_string_multiline_element(
        app->widget, 3, 2, AlignLeft, AlignTop, FontPrimary, "Connection\nis active!");
    widget_add_string_multiline_element(
        app->widget,
        3,
        30,
        AlignLeft,
        AlignTop,
        FontSecondary,
        "Disconnect from\nPC or phone to\nuse this function.");

    view_dispatcher_switch_to_view(app->view_dispatcher, RWEmuAppViewWidget);
}

bool rwemu_scene_usb_locked_on_event(void* context, SceneManagerEvent event) {
    RWEmuApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, RWEmuSceneFileSelect);
        if(!consumed) {
            consumed = scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, RWEmuSceneStart);
        }
    }

    return consumed;
}

void rwemu_scene_usb_locked_on_exit(void* context) {
    RWEmuApp* app = context;
    widget_reset(app->widget);
}
