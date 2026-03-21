#include "pedal_graph_dashboard.h"
#include "dashboard_core.h"
#include "gfx_font.h"
#include "dashboard_fonts.h"

#include "ssd1963.h"
#include <string.h>

typedef struct {
    int screen_w;
    int screen_h;

    Rect left_section;
    Rect mid_section;
    Rect right_section;
    Rect graph_section;

    VerticalBarWidget soc_bar;
    VerticalBarWidget invtemp_bar;

    NumericBlockWidget lap_block;
    NumericBlockWidget temp_block;
    NumericBlockWidget speed_block;

    DualTraceGraphWidget pedal_graph;
} PedalGraphDashboardLayout;

static Dashboard prev;
static int inited = 0;

static DashboardTheme g_theme = {
    .screen_w = DASH_W,
    .screen_h = DASH_H,

    .top_margin = 12,
    .section_w = 200,
    .section_h = 300,
    .section_gap = 50,
    .side_margin = 50,

    .bar_w = 150,
    .bar_h = 240,
    .bar_top_offset = 42,

    .right_block_top_gap = 20,
    .right_block_h = 88,
    .right_block_gap = 10
};

static PedalGraphDashboardLayout g_layout;

static void pedal_graph_build_layout(PedalGraphDashboardLayout *L,
                                     const DashboardTheme *T)
{
    int x0 = T->side_margin;
    int x1 = x0 + T->section_w + T->section_gap;

    /* Push right column a bit further right */
    int right_shift = 20;
    int x2 = x1 + T->section_w + T->section_gap + right_shift;

    /* Push whole upper layout slightly higher */
    int top_y = 15;

    /* Use more vertical space for right-side stack */
    int right_blocks_y0 = top_y + 8;
    int right_block_h = 90;
    int right_block_gap = 18;

    L->screen_w = T->screen_w;
    L->screen_h = T->screen_h;

    L->left_section  = make_rect(x0, top_y, T->section_w, 300);
    L->mid_section   = make_rect(x1, top_y, T->section_w, 300);
    L->right_section = make_rect(x2, top_y, T->section_w, 320);

    /* Keep graph lower so overall layout uses the screen height better */
    L->graph_section = make_rect(50, 338, 700, 130);

    L->soc_bar.area = L->left_section;
    L->soc_bar.bar_rect = make_rect(
        L->left_section.x + (L->left_section.w - 150) / 2,
        L->left_section.y + 42,
        150,
        220
    );
    L->soc_bar.title = "SoC";
    L->soc_bar.min_value = 0;
    L->soc_bar.max_value = 100;
    L->soc_bar.clamp_value_text = 1;
    L->soc_bar.bg_color = COL_BG;
    L->soc_bar.frame_color = COL_TEXT;
    L->soc_bar.text_color = COL_TEXT;
    L->soc_bar.color_fn = battery_color_for_percent;
    L->soc_bar.unit_draw_fn = draw_percent;

    L->invtemp_bar.area = L->mid_section;
    L->invtemp_bar.bar_rect = make_rect(
        L->mid_section.x + (L->mid_section.w - 150) / 2,
        L->mid_section.y + 42,
        150,
        220
    );
    L->invtemp_bar.title = "TEMP";
    L->invtemp_bar.min_value = 20;
    L->invtemp_bar.max_value = 60;
    L->invtemp_bar.clamp_value_text = 0;
    L->invtemp_bar.bg_color = COL_BG;
    L->invtemp_bar.frame_color = COL_TEXT;
    L->invtemp_bar.text_color = COL_TEXT;
    L->invtemp_bar.color_fn = temp_color_for_value;
    L->invtemp_bar.unit_draw_fn = draw_C;

    L->lap_block.area = make_rect(
        x2,
        right_blocks_y0,
        200,
        right_block_h
    );
    L->lap_block.label = "LAP";
    L->lap_block.label_y_offset = 0;
    L->lap_block.value_y_offset = 2;
    L->lap_block.fg_color = COL_TEXT;
    L->lap_block.bg_color = COL_BG;
    L->lap_block.label_font = &FreeSansBold18pt7b;
    L->lap_block.value_font = &F1Bold40pt7b;

    L->temp_block.area = make_rect(
        x2,
        right_blocks_y0 + right_block_h + right_block_gap,
        200,
        right_block_h
    );
    L->temp_block.label = "TEMP";
    L->temp_block.label_y_offset = 0;
    L->temp_block.value_y_offset = 2;
    L->temp_block.fg_color = COL_TEXT;
    L->temp_block.bg_color = COL_BG;
    L->temp_block.label_font = &FreeSansBold18pt7b;
    L->temp_block.value_font = &F1Bold40pt7b;

    L->speed_block.area = make_rect(
        x2,
        right_blocks_y0 + 2 * (right_block_h + right_block_gap),
        200,
        right_block_h
    );
    L->speed_block.label = "SPEED";
    L->speed_block.label_y_offset = 0;
    L->speed_block.value_y_offset = 2;
    L->speed_block.fg_color = COL_TEXT;
    L->speed_block.bg_color = COL_BG;
    L->speed_block.label_font = &FreeSansBold18pt7b;
    L->speed_block.value_font = &F1Bold40pt7b;

    graph_init(&L->pedal_graph, L->graph_section);
}

static void pedal_graph_draw_static(const PedalGraphDashboardLayout *L)
{
    SSD1963_Fill(COL_BG);

    draw_vertical_bar_static(&L->soc_bar);
    draw_vertical_bar_static(&L->invtemp_bar);

    draw_numeric_block_static(&L->lap_block);
    draw_numeric_block_static(&L->temp_block);
    draw_numeric_block_static(&L->speed_block);

    draw_graph_static(&L->pedal_graph);
}

static void pedal_graph_draw_values_full(const PedalGraphDashboardLayout *L,
                                         int battery_charge,
                                         int cell_temperature,
                                         int speed,
                                         int lap)
{
    draw_vertical_bar_value_full(&L->soc_bar, battery_charge);
    draw_vertical_bar_value_full(&L->invtemp_bar, cell_temperature);

    draw_lap_counter_full(&L->lap_block, lap, LAP_TOTAL_DEFAULT, COL_ACC, lap);

    draw_numeric_block_value_full(&L->temp_block,
                                  cell_temperature,
                                  temp_color_for_value(cell_temperature));

    draw_numeric_block_value_full(&L->speed_block, speed, COL_TEXT);

    draw_graph_full(&L->pedal_graph);
}

void pedal_graph_dash_init(double initial_battery_charge,
                           double initial_cell_temperature,
                           int initial_lap,
                           int initial_speed)
{
    pedal_graph_build_layout(&g_layout, &g_theme);

    memset(&prev, 0, sizeof(prev));
    prev.battery_charge = (uint8_t)clampi((int)initial_battery_charge, 0, 100);
    prev.cell_temperature = (uint8_t)clampi((int)initial_cell_temperature, 0, 150);
    prev.water_temperature = 0;
    prev.speed = (uint16_t)clampi(initial_speed, 0, 999);
    prev.lap = (uint8_t)clampi(initial_lap, 0, 99);
    prev.throttle_percent = 0;
    prev.brake_percent = 0;

    graph_clear(&g_layout.pedal_graph);

    pedal_graph_draw_static(&g_layout);
    pedal_graph_draw_values_full(&g_layout,
                                 prev.battery_charge,
                                 prev.cell_temperature,
                                 prev.speed,
                                 prev.lap);

    inited = 1;
}

void pedal_graph_dash_update(const Dashboard *d)
{
    if (!d) {
        return;
    }

    if (!inited) {
        pedal_graph_dash_init(d->battery_charge,
                              d->cell_temperature,
                              d->lap,
                              d->speed);

        prev.throttle_percent = d->throttle_percent;
        prev.brake_percent = d->brake_percent;

        graph_push_sample(&g_layout.pedal_graph,
                          d->throttle_percent,
                          d->brake_percent);
        draw_graph_full(&g_layout.pedal_graph);
        return;
    }

    if ((int)d->battery_charge != (int)prev.battery_charge) {
        draw_vertical_bar_value_delta(&g_layout.soc_bar,
                                      (int)prev.battery_charge,
                                      (int)d->battery_charge);
        prev.battery_charge = d->battery_charge;
    }

    if ((int)d->cell_temperature != (int)prev.cell_temperature) {
        int old_temp = (int)prev.cell_temperature;
        int new_temp = (int)d->cell_temperature;
        uint16_t old_temp_col = temp_color_for_value(old_temp);
        uint16_t new_temp_col = temp_color_for_value(new_temp);

        draw_vertical_bar_value_delta(&g_layout.invtemp_bar, old_temp, new_temp);

        if (old_temp_col != new_temp_col) {
            draw_numeric_block_value_full(&g_layout.temp_block,
                                          new_temp,
                                          new_temp_col);
        } else {
            draw_numeric_block_value_delta(&g_layout.temp_block,
                                           old_temp,
                                           new_temp,
                                           new_temp_col);
        }

        prev.cell_temperature = d->cell_temperature;
    }

    if ((int)d->speed != (int)prev.speed) {
        draw_numeric_block_value_delta(&g_layout.speed_block,
                                       (int)prev.speed,
                                       (int)d->speed,
                                       COL_TEXT);
        prev.speed = d->speed;
    }

    if ((int)d->lap != (int)prev.lap) {
        draw_lap_counter_delta(&g_layout.lap_block,
                               (int)prev.lap,
                               (int)d->lap,
                               LAP_TOTAL_DEFAULT,
                               COL_ACC);
        prev.lap = d->lap;
    }

    /* Always sample the pedals once per dashboard tick (20 ms). */
    graph_push_sample(&g_layout.pedal_graph,
                      d->throttle_percent,
                      d->brake_percent);
    draw_graph_full(&g_layout.pedal_graph);

    prev.throttle_percent = d->throttle_percent;
    prev.brake_percent = d->brake_percent;
}

void pedal_graph_dash_push_inputs(int throttle_percent, int brake_percent)
{
    if (!inited) {
        return;
    }

    graph_push_sample(&g_layout.pedal_graph, throttle_percent, brake_percent);
    draw_graph_full(&g_layout.pedal_graph);
}