#include "group.h"

#include "config.h"
#include "window.h"
#include "ewmh.h"

short unsigned int groups = 0;
unsigned int focused_group = 1;

group_state_t
get_group_state(unsigned int group)
{
    debug_output("get_group_state(): called");
    group_state_t state;

    if (group < 1 || group > 16 || group > Configuration->groups) {
        return UNMAPPED;
    }

    if ((groups & (1 << (group - 1))) > 0) {
        state = MAPPED;
    } else {
        state = UNMAPPED;
    }

    if (focused_group == group) {
        state = FOCUSED;
    }

    return state;
}

short unsigned int
window_is_in_group(Window *window, unsigned int group)
{
    if ((window->groups & (1 << (group - 1))) > 0) {
        return 1;
    } else {
        return 0;
    }
}

void
attach_window_to_group(xcb_window_t window_id, unsigned int group)
{
    debug_output("attach_window_to_group(): called");
    if (group < 1 || group > 16 || group > Configuration->groups) {
        return;
    }
    Window *window = window_list_get_window(window_id);

    if (window) {
        window->groups |= (1 << (group - 1));
    }

    if ((window->groups & groups) > 0) {
        map_window(window->id);
    }
}

void
detach_window_from_group(xcb_window_t window_id, unsigned int group)
{
    debug_output("detach_window_to_group(): called");
    if (group < 1 || group > 16 || group > Configuration->groups) {
        return;
    }
    Window *window = window_list_get_window(window_id);

    if (window) {
        if ((window->groups & ~(1 << (group - 1))) > 0) {
            window->groups &= ~(1 << (group - 1));

            if ((window->groups & groups) == 0) {
                unmap_window(window->id);
            }
        }
    }
}

void
focus_group(unsigned int group)
{
    debug_output("focus_group(): called");
    if (group < 1 || group > 16 || group > Configuration->groups) {
        return;
    }

    if (focused_window) {
        if (!window_is_in_group(focused_window, group)) {
            unfocus_window();
        }
    }

    struct WindowLinkedListElement *element = window_list_head;
    Window *window = NULL;
    xcb_window_t last_window = XCB_WINDOW_NONE;

    while (element) {
        window = element->window;

        if (window_is_in_group(window, group)) {
            map_window(window->id);
            last_window = window->id;
        } else {
            unmap_window(window->id);
        }

        element = element->next;
    }

    groups = (1 << (group - 1));

    focused_group = group;
    xcb_ewmh_set_current_desktop(ewmh_connection, 0, focused_group);

    if (last_window != XCB_WINDOW_NONE) {
        focus_on_window(last_window);
        raise_window(last_window);
    }
}

void
map_group(unsigned int group)
{
    debug_output("map_group(): called");
    struct WindowLinkedListElement *element = window_list_head;
    Window *window = NULL;

    while (element) {
        window = element->window;

        if (window_is_in_group(window, group)) {
            map_window(window->id);
        }

        element = element->next;
    }

    groups |= (1 << (group - 1));
}

void
unmap_group(unsigned int group)
{
    debug_output("unmap_group(): called");
    struct WindowLinkedListElement *element = window_list_head;
    Window *window = NULL;

    if (focused_group == group) {
        return;
    }

    while (element) {
        window = element->window;

        if (window_is_in_group(window, group)) {
            if (!window_is_in_group(window, focused_group)) {
                unmap_window(window->id);
            }
        }

        element = element->next;
    }

    groups &= ~(1 << (group - 1));
}
