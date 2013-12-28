#include "time_layer.h"

/* Called by the graphics layers when the time layer needs to be updated.
*/
void time_layer_update_proc(TimeLayer *tl, GContext* ctx)
{
    if (tl->background_color != GColorClear)
    {
        graphics_context_set_fill_color(ctx, tl->background_color);
        graphics_fill_rect(ctx, layer_get_bounds(tl->layer), 0, GCornerNone);
    }
    graphics_context_set_text_color(ctx, tl->text_color);

    if (tl->hour_text && tl->minute_text)
    {
        GSize hour_sz =
            graphics_text_layout_get_content_size(tl->hour_text,
                                                  tl->hour_font,
                                                  layer_get_bounds(tl->layer),
                                                  tl->overflow_mode,
                                                  GTextAlignmentLeft);
        GSize minute_sz =
            graphics_text_layout_get_content_size(tl->minute_text,
                                                  tl->minute_font,
                                                  layer_get_bounds(tl->layer),
                                                  tl->overflow_mode,
                                                  GTextAlignmentLeft);
        int width = minute_sz.w + hour_sz.w;
        int half = layer_get_bounds(tl->layer).size.w / 2;
        GRect hour_bounds = layer_get_bounds(tl->layer);
        GRect minute_bounds = layer_get_bounds(tl->layer);

        hour_bounds.size.w = half - (width / 2) + hour_sz.w;
        minute_bounds.origin.x = hour_bounds.size.w + 1;
        minute_bounds.size.w = minute_sz.w;

        graphics_draw_text(ctx,
                           tl->hour_text,
                           tl->hour_font,
                           hour_bounds,
                           tl->overflow_mode,
                           GTextAlignmentRight,
                           tl->layout_cache);
        graphics_draw_text(ctx,
                           tl->minute_text,
                           tl->minute_font,
                           minute_bounds,
                           tl->overflow_mode,
                           GTextAlignmentLeft,
                           tl->layout_cache);
    }
}


/* Set the hour and minute text and mark the layer dirty. NOTE that the
* two strings must be static because we're only storing a pointer to them,
* not making a copy.
*/
void time_layer_set_text(TimeLayer *tl, char *hour_text, char *minute_text)
{
    tl->hour_text = hour_text;
    tl->minute_text = minute_text;

    layer_mark_dirty(tl->layer);
}


/* Set the time fonts. Hour and minute fonts can be different.
*/
void time_layer_set_fonts(TimeLayer *tl, GFont hour_font, GFont minute_font)
{
    tl->hour_font = hour_font;
    tl->minute_font = minute_font;

    if (tl->hour_text && tl->minute_text)
    {
        layer_mark_dirty(tl->layer);
    }
}


/* Set the text color of the time layer.
*/
void time_layer_set_text_color(TimeLayer *tl, GColor color)
{
    tl->text_color = color;

    if (tl->hour_text && tl->minute_text)
    {
        layer_mark_dirty(tl->layer);
    }
}


/* Set the background color of the time layer.
*/
void time_layer_set_background_color(TimeLayer *tl, GColor color)
{
    tl->background_color = color;

    if (tl->hour_text && tl->minute_text)
    {
        layer_mark_dirty(tl->layer);
    }
}


/* Initialize the time layer with default colors and fonts.
*/
void time_layer_create(TimeLayer *tl, GRect frame)
{
    //layer_init(&tl->layer, frame);
    tl->layer = layer_create(frame);
    //tl->layer.update_proc = (LayerUpdateProc)time_layer_update_proc;
    layer_set_update_proc(tl->layer, (LayerUpdateProc)time_layer_update_proc); // Correct?

    tl->text_color = GColorWhite;
    tl->background_color = GColorClear;
    tl->overflow_mode = GTextOverflowModeWordWrap;

    tl->hour_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    tl->minute_font = tl->hour_font;
}