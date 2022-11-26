#include "mui_core.h"

#include "mui_defines.h"
#include "mui_u8g2.h"

static mui_view_port_t *
mui_find_view_port_enabled(mui_t *p_mui, mui_view_port_array_t view_port_array) {
    mui_view_port_array_it_t it;
    mui_view_port_array_it_last(it, view_port_array);
    while (!mui_view_port_array_end_p(it)) {
        mui_view_port_t *p_view_port = *mui_view_port_array_ref(it);
        if (p_view_port->enabled) {
            return p_view_port;
        }
        mui_view_port_array_previous(it);
    }
    return NULL;
}

static void mui_process_redraw(mui_t *p_mui, mui_event_t *p_event) {
    mui_view_port_t *p_view_port =
        mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_FULLSCREEN]);
    if (p_view_port) {
        p_view_port->draw_cb(p_view_port, &p_view_port->canvas);
        return;
    }

    p_view_port = mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_WINDOW]);
    if (p_view_port) {
        mui_view_port_t *p_view_port_status_bar =
            mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_STATUS_BAR]);
        p_view_port->draw_cb(p_view_port, &p_view_port->canvas);
        if (p_view_port_status_bar) {
            p_view_port_status_bar->draw_cb(p_view_port, &p_view_port->canvas);
        }
        return;
    }

    p_view_port = mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_DESKTOP]);
    if (p_view_port) {
        mui_view_port_t *p_view_port_status_bar =
            mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_STATUS_BAR]);
        p_view_port->draw_cb(p_view_port, &p_view_port->canvas);
        if (p_view_port_status_bar) {
            p_view_port_status_bar->draw_cb(p_view_port, &p_view_port->canvas);
        }
        return;
    }
}

static void mui_process_input(mui_t *p_mui, mui_event_t *p_event) {
    mui_view_port_t *p_view_port =
        mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_FULLSCREEN]);
    if (!p_view_port) {
        p_view_port = mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_WINDOW]);
    }
    if (!p_view_port) {
        p_view_port = mui_find_view_port_enabled(p_mui, p_mui->layers[MUI_LAYER_DESKTOP]);
    }
    if (p_view_port) {
        mui_input_event_t input_event;
        uint32_t arg = p_event->arg_int;

        input_event.key = arg & 0xFF;
        input_event.type = (arg >> 8) & 0xFF;

        p_view_port->input_cb(p_view_port, &input_event);
    }
}

static void mui_process_event(void *p_context, mui_event_t *p_event) {
    mui_t *p_mui = p_context;
    switch (p_event->id) {
    case MUI_EVENT_ID_REDRAW:
        mui_process_redraw(p_mui, p_event);
        break;

    case MUI_EVENT_ID_INPUT:
        mui_process_input(p_mui, p_event);
        break;
    }
}

mui_t *mui() {
    static mui_t mui;
    return &mui;
}

void mui_init(mui_t *p_mui) {
    mui_u8g2_init(&p_mui->u8g2);

    mui_event_set_callback(&p_mui->event_queue, mui_process_event, p_mui);
    mui_event_queue_init(&p_mui->event_queue);

    mui_input_init();
    p_mui->initialized = true;
}

void mui_deinit(mui_t *p_mui) {
    mui_u8g2_deinit(&p_mui->u8g2);
    p_mui->initialized = false;
}

void mui_post(mui_t *p_mui, mui_event_t *p_event) {
    mui_event_post(&p_mui->event_queue, p_event);
}

void mui_tick(mui_t *p_mui) { mui_event_dispatch(&p_mui->event_queue); }

void mui_panic(mui_t *p_mui, char *err) {
    if (p_mui->initialized) {
        u8g2_ClearBuffer(&p_mui->u8g2);
        u8g2_SetFont(&p_mui->u8g2, u8g2_font_wqy12_t_gb2312a);
        u8g2_DrawBox(&p_mui->u8g2, 0, 0, 128, 12);
        u8g2_SetDrawColor(&p_mui->u8g2, 0);
        u8g2_DrawUTF8(&p_mui->u8g2, 28, 10, "SYSTEM FAULT");

        u8g2_SetDrawColor(&p_mui->u8g2, 1);

        uint8_t x = 0;
        uint8_t y = 24;
        uint32_t i = 0;
        uint8_t m = u8g2_GetMaxCharWidth(&p_mui->u8g2);

        while (err[i] != 0 && y < 64) {
            uint8_t w = u8g2_DrawGlyph(&p_mui->u8g2, x, y, err[i]);
            x += w;
            if (x > 128 - m) {
                x = 0;
                y += 12;
            }
            i++;
        }

        u8g2_SendBuffer(&p_mui->u8g2);
    }
}
