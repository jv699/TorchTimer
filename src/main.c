#include <pebble.h>

#define MAX_TIMERS 10
#define NUM_PRESETS 4

typedef struct {
  char name[32];
  int remaining_secs;
  bool active;
} CountdownTimer;

typedef struct {
  const char *name;
  int duration_secs;
} LightPreset;

static const LightPreset s_presets[] = {
  { "Torch", 3600 },
  { "Lantern", 3600 },
  { "Oil Lamp", 7200 },
  { "Candle", 600 },
};

static Window *s_main_window;
static Window *s_pick_window;
static Window *s_setup_window;
static MenuLayer *s_menu_layer;
static MenuLayer *s_pick_menu_layer;
static TextLayer *s_time_layer;
static char s_menu_item_titles[MAX_TIMERS + 1][32];
static char s_menu_item_subtitles[MAX_TIMERS + 1][32];

static TextLayer *s_setup_title_layer;
static TextLayer *s_setup_time_layer;
static TextLayer *s_setup_hint_layer;
static int s_setup_minutes;

static CountdownTimer s_timers[MAX_TIMERS];
static int s_timer_count;

static void start_timer_with_duration(int secs, const char *name);
static void start_pick_light(void);
static void start_timer_setup(void);
static void timer_setup_window_load(Window *window);
static void timer_setup_window_unload(Window *window);

// Formats seconds into a human-readable duration (e.g. "1h 30m" or "45m")
static void format_duration(char *buf, size_t len, int secs) {
  int h = secs / 3600;
  int m = (secs % 3600) / 60;
  if (h > 0) {
    snprintf(buf, len, "%dh %dm", h, m);
  } else {
    snprintf(buf, len, "%dm", m);
  }
}

// Formats seconds into a countdown timer string (e.g. "1:30:45" or "5:30")
static void format_countdown(char *buf, size_t len, int secs) {
  int h = secs / 3600;
  int m = (secs % 3600) / 60;
  int s = secs % 60;
  if (h > 0) {
    snprintf(buf, len, "%d:%02d:%02d", h, m, s);
  } else {
    snprintf(buf, len, "%d:%02d", m, s);
  }
}

// Creates a new countdown timer with the given duration and name
static void start_timer_with_duration(int secs, const char *name) {
  if (s_timer_count < MAX_TIMERS) {
    CountdownTimer *t = &s_timers[s_timer_count];
    snprintf(t->name, sizeof(t->name), "%s", name);
    t->remaining_secs = secs;
    t->active = true;
    s_timer_count++;
    menu_layer_reload_data(s_menu_layer);
  }
}

// Called every second/minute: updates the clock display and decrements active timers
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  static char time_buf[8];
  strftime(time_buf, sizeof(time_buf), "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, time_buf);

  bool needs_update = false;
  for (int i = 0; i < s_timer_count; i++) {
    if (s_timers[i].active && s_timers[i].remaining_secs > 0) {
      s_timers[i].remaining_secs--;
      needs_update = true;
      if (s_timers[i].remaining_secs == 0) {
        s_timers[i].active = false;
      }
    }
  }
  if (needs_update) {
    menu_layer_reload_data(s_menu_layer);
  }
}

// Returns the number of rows in the main menu (active timers + "Add Light" button)
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  return s_timer_count + 1;
}

// Draws a single row in the main menu, showing timer name and countdown or "Done"
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  int i = cell_index->row;
  if (i < s_timer_count) {
    snprintf(s_menu_item_titles[i], sizeof(s_menu_item_titles[i]), "%s", s_timers[i].name);
    if (s_timers[i].active) {
      format_countdown(s_menu_item_subtitles[i], sizeof(s_menu_item_subtitles[i]), s_timers[i].remaining_secs);
    } else {
      snprintf(s_menu_item_subtitles[i], sizeof(s_menu_item_subtitles[i]), "Done");
    }
    menu_cell_basic_draw(ctx, cell_layer, s_menu_item_titles[i], s_menu_item_subtitles[i], NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "+ Add Light", NULL, NULL);
  }
}

// Handles selection on the main menu; opens the picker if "Add Light" is tapped
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  if (cell_index->row == s_timer_count) {
    start_pick_light();
  }
}

// Returns the number of rows in the light picker menu (presets + "Custom" option)
static uint16_t pick_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  return NUM_PRESETS + 1;
}

// Draws a single row in the light picker menu, showing preset name and duration
static void pick_menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  int i = cell_index->row;
  if (i < NUM_PRESETS) {
    static char buf[32];
    format_duration(buf, sizeof(buf), s_presets[i].duration_secs);
    menu_cell_basic_draw(ctx, cell_layer, s_presets[i].name, buf, NULL);
  } else {
    menu_cell_basic_draw(ctx, cell_layer, "Custom", NULL, NULL);
  }
}

// Handles selection in the picker; starts a preset timer or opens the custom setup
static void pick_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  int i = cell_index->row;
  if (i < NUM_PRESETS) {
    start_timer_with_duration(s_presets[i].duration_secs, s_presets[i].name);
    window_stack_pop(true);
  } else {
    start_timer_setup();
  }
}

// Sets up the main window: adds the clock display and timer list menu
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_time_layer = text_layer_create(GRect(0, 0, bounds.size.w, 28));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  static char time_buf[8];
  strftime(time_buf, sizeof(time_buf), "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, time_buf);

  GRect menu_bounds = GRect(0, 28, bounds.size.w, bounds.size.h - 28);
  s_menu_layer = menu_layer_create(menu_bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  tick_timer_service_subscribe(MINUTE_UNIT | SECOND_UNIT, tick_handler);
}

// Tears down the main window: unsubscribes from ticks and destroys UI layers
static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  text_layer_destroy(s_time_layer);
  menu_layer_destroy(s_menu_layer);
}

// Sets up the light picker window with preset options and custom entry
static void pick_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_pick_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_pick_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = pick_menu_get_num_rows_callback,
    .draw_row = pick_menu_draw_row_callback,
    .select_click = pick_menu_select_callback,
  });
  menu_layer_set_click_config_onto_window(s_pick_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_pick_menu_layer));
}

// Tears down the light picker window
static void pick_window_unload(Window *window) {
  menu_layer_destroy(s_pick_menu_layer);
}

// Increments the custom timer duration by 1 minute (max 99)
static void setup_increment(ClickRecognizerRef recognizer, void *context) {
  if (s_setup_minutes < 99) {
    s_setup_minutes++;
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d min", s_setup_minutes);
    text_layer_set_text(s_setup_time_layer, buf);
  }
}

// Decrements the custom timer duration by 1 minute (min 1)
static void setup_decrement(ClickRecognizerRef recognizer, void *context) {
  if (s_setup_minutes > 1) {
    s_setup_minutes--;
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d min", s_setup_minutes);
    text_layer_set_text(s_setup_time_layer, buf);
  }
}

// Confirms the custom timer, starts it, and returns to the main list
static void setup_confirm(ClickRecognizerRef recognizer, void *context) {
  if (s_timer_count < MAX_TIMERS && s_setup_minutes > 0) {
    start_timer_with_duration(s_setup_minutes * 60, "Custom");
  }
  window_stack_pop(true);
  window_stack_pop(true);
}

// Cancels custom timer setup and returns to the picker menu
static void setup_cancel(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

// Maps physical buttons to actions on the custom timer setup screen
static void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, setup_increment);
  window_single_click_subscribe(BUTTON_ID_DOWN, setup_decrement);
  window_single_click_subscribe(BUTTON_ID_SELECT, setup_confirm);
  window_single_click_subscribe(BUTTON_ID_BACK, setup_cancel);
}

// Builds the custom timer setup UI with title, duration display, and hints
static void timer_setup_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_setup_title_layer = text_layer_create(GRect(0, 10, bounds.size.w, 30));
  text_layer_set_text_alignment(s_setup_title_layer, GTextAlignmentCenter);
  text_layer_set_font(s_setup_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_setup_title_layer, "Custom Timer");
  layer_add_child(window_layer, text_layer_get_layer(s_setup_title_layer));

  s_setup_minutes = 60;
  static char time_buf[8];
  snprintf(time_buf, sizeof(time_buf), "%d min", s_setup_minutes);
  s_setup_time_layer = text_layer_create(GRect(0, 50, bounds.size.w, 60));
  text_layer_set_text_alignment(s_setup_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_setup_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(s_setup_time_layer, time_buf);
  layer_add_child(window_layer, text_layer_get_layer(s_setup_time_layer));

  s_setup_hint_layer = text_layer_create(GRect(10, 120, bounds.size.w - 20, 50));
  text_layer_set_text_alignment(s_setup_hint_layer, GTextAlignmentCenter);
  text_layer_set_font(s_setup_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(s_setup_hint_layer, "Up/Down: adjust\nSelect: start\nBack: cancel");
  layer_add_child(window_layer, text_layer_get_layer(s_setup_hint_layer));

  window_set_click_config_provider(window, config_provider);
}

// Tears down the custom timer setup UI
static void timer_setup_window_unload(Window *window) {
  text_layer_destroy(s_setup_title_layer);
  text_layer_destroy(s_setup_time_layer);
  text_layer_destroy(s_setup_hint_layer);
}

// Opens the light picker menu
static void start_pick_light(void) {
  window_stack_push(s_pick_window, true);
}

// Opens the custom timer duration setup screen
static void start_timer_setup(void) {
  window_stack_push(s_setup_window, true);
}

// App entry point: creates windows and pushes the main window onto the stack
static void init(void) {
  s_timer_count = 0;

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload,
  });

  s_pick_window = window_create();
  window_set_window_handlers(s_pick_window, (WindowHandlers){
    .load = pick_window_load,
    .unload = pick_window_unload,
  });

  s_setup_window = window_create();
  window_set_window_handlers(s_setup_window, (WindowHandlers){
    .load = timer_setup_window_load,
    .unload = timer_setup_window_unload,
  });

  window_stack_push(s_main_window, true);
}

// App exit point: unsubscribes from services and destroys all windows
static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
  window_destroy(s_pick_window);
  window_destroy(s_setup_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
