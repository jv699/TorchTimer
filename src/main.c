#include <pebble.h>

static Window *s_main_window;
static SimpleMenuLayer *s_menu_layer;
static SimpleMenuSection s_menu_sections[1];
static SimpleMenuItem s_menu_items[5];

static void menu_select_callback(int index, void *ctx) {
  (void)ctx;
}

static void main_window_load(Window *window) {
  (void)window;

  s_menu_items[0] = (SimpleMenuItem){
    .title = "Item 1",
    .callback = menu_select_callback,
  };

  s_menu_items[1] = (SimpleMenuItem){
    .title = "Item 2",
    .callback = menu_select_callback,
  };

  s_menu_items[2] = (SimpleMenuItem){
    .title = "Item 3",
    .callback = menu_select_callback,
  };

  s_menu_items[3] = (SimpleMenuItem){
    .title = "Item 4",
    .callback = menu_select_callback,
  };

  s_menu_items[4] = (SimpleMenuItem){
    .title = "Item 5",
    .callback = menu_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection){
    .title = "TorchTimer",
    .items = s_menu_items,
    .num_items = 5,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, 1, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload(Window *window) {
  (void)window;
  simple_menu_layer_destroy(s_menu_layer);
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
