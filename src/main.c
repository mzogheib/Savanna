#include <pebble.h>

Window *my_window;
TextLayer *time_layer;
TextLayer *time_layer_bg; 

BitmapLayer *image_layer;
GBitmap *image;
GSize image_size;

GFont custom_font;

// Colours
#ifdef PBL_COLOR
  #define TOTAL_COLOURS 6
#else
  #define TOTAL_COLOURS 1
#endif
GColor time_layer_bg_colours[TOTAL_COLOURS];
int current_colour;
int next_colour;
// Function to init the colour array
void init_time_layer_bg_colours() {
  time_layer_bg_colours[0] = GColorBlack;
  #ifdef PBL_COLOR
    time_layer_bg_colours[1] = GColorOrange;
    time_layer_bg_colours[2] = GColorGreen;
    time_layer_bg_colours[3] = GColorMagenta;
    time_layer_bg_colours[4] = GColorRed;
    time_layer_bg_colours[5] = GColorBlue;
  #endif
}


// Images
#define TOTAL_IMAGES 3
// Array of image resource ids
uint8_t image_resource_ids[TOTAL_IMAGES];
int current_image;
int next_image;
// Function to init the image resource ids array
void init_image_resource_ids() {
  image_resource_ids[0] = RESOURCE_ID_IMAGE_GIRAFFE;
  image_resource_ids[1] = RESOURCE_ID_IMAGE_ZEBRA;
  image_resource_ids[2] = RESOURCE_ID_IMAGE_LEOPARD;

}



int init_finished; // Flag to stop the images/colours from changing just after startup if the watch starts up at the changeover times, e.g. every minute or every 5 minutes.

// Displays the image passed in by the resource id
void display_image(uint8_t image_res) {  
  
  // Destroy the current image
  gbitmap_destroy(image);
  
  // Then select a new random one. 
  image = gbitmap_create_with_resource(image_res);
  image_size = gbitmap_get_bounds(image).size;  
  image_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  layer_mark_dirty(bitmap_layer_get_layer(image_layer));
}
// The draw function, called whenever the passed in layer is marked dirty
static void image_layer_update_callback(Layer *layer, GContext* ctx) {
  // Transparency if applicable
  //graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, image, GRect(0, 0, image_size.w, image_size.h));
}

// Return a random number that is different to the current number
int get_random_number(int current_num, int total_num) {
  int i;
  
  // Get a random number
  i = rand()%total_num;
  
  // If it's the same as the current then try again until different
  while(i==current_num) {
    i = rand()%total_num;
  }
  
  return i;
}


// Returns the time
char *write_time(struct tm tick_time) {
  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", &tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", &tick_time);
  }
    
  // Strip leading zero 
  if(buffer[0]=='0') strcpy(buffer, buffer+1);
  
  return (char *)buffer;
  
}

// Run this function at every tick of the clock, i.e. second or minute
void handle_tick(struct tm *tick_time, TimeUnits units){  
  // Write the current time and date
  text_layer_set_text(time_layer, write_time(*tick_time));
  
  // Randomly change the colour every minute
  if(init_finished==1){
    if(tick_time->tm_sec == 0)  {
      next_colour = get_random_number(current_colour, TOTAL_COLOURS);
      text_layer_set_background_color(time_layer_bg, time_layer_bg_colours[next_colour]);
    }
    if(tick_time->tm_min%5 == 0)  {
      next_image = get_random_number(current_image, TOTAL_IMAGES);
      display_image(image_resource_ids[next_image]);    
    }
  }
  // On first pass after init this will be 0 and thus the above code block will not run.
  init_finished=1;
}

// Initialize err'thang
void handle_init(void) {
  my_window = window_create();
  Layer *window_layer = window_get_root_layer(my_window);
  
  // Flag to stop a background change if the watchface launches at the changeover time, e.g. 5mins
  init_finished = 0;
  
  // Init the colour and image resource ids
  init_time_layer_bg_colours();
  init_image_resource_ids();
  
  // Get random image and colour index
  srand(time(NULL));
  current_colour = -1;
  next_colour = get_random_number(current_colour, TOTAL_COLOURS);
  current_image = -1;
  next_image = get_random_number(current_image, TOTAL_IMAGES);
  
  // Init the image & layer. 
  display_image(image_resource_ids[next_image]);
  layer_set_update_proc(bitmap_layer_get_layer(image_layer), image_layer_update_callback);

  // Init the time layer
  int time_layer_w = 120;
  int time_layer_h = 50;
  // Position the time layer and it's background layer. Using two layers to tune the position of the text.
  time_layer = text_layer_create(GRect(144/2 - time_layer_w/2, 168/2 - time_layer_h/2 - 1, time_layer_w, time_layer_h));
  time_layer_bg = text_layer_create(GRect(144/2 - time_layer_w/2, 168/2 - time_layer_h/2, time_layer_w, time_layer_h));  
  // Init colours & alignment
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_background_color(time_layer_bg, time_layer_bg_colours[next_colour]);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  // Init font
  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_40));
  text_layer_set_font(time_layer, custom_font);  
  text_layer_set_text_color(time_layer, GColorWhite);
  // Finally, write the time
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);  // Find a way to not use temp
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  text_layer_set_text(time_layer, write_time(*tick_time));

  // Add all layers to the window layer
  layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));
  layer_add_child(window_layer, text_layer_get_layer(time_layer_bg));
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
  window_stack_push(my_window, true);

}

void handle_deinit(void) {
  text_layer_destroy(time_layer);
  text_layer_destroy(time_layer_bg);
  bitmap_layer_destroy(image_layer);
  gbitmap_destroy(image);
  fonts_unload_custom_font(custom_font);
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}