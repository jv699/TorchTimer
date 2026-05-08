#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static MenuLayer *s_menu_layer;

static const char *s_menu_items[] = {
  "Item 1", "Item 2", "Item 3", "Item 4", "Item 5"
};

static void update_time(struct tm *tick_time, TimeUnits units_changed) {
  static char time_buffer[8];
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, time_buffer);
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  return 5;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  menu_cell_basic_draw(ctx, cell_layer, s_menu_items[cell_index->row], NULL, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  (void)menu_layer;
  (void)cell_index;
  (void)callback_context;
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 30));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  s_menu_layer = menu_layer_create(GRect(0, 30, bounds.size.w, bounds.size.h - 30));
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  update_time(tick_time, MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, update_time);
}

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  menu_layer_destroy(s_menu_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
