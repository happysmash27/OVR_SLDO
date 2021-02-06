#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

void init(xcb_connection_t **xcb_connection, int *screen_number);
void deinit(xcb_connection_t *xcb_connection);

void print_screen_info(xcb_connection_t *xcb_connection, int *screen_number);

uint8_t *grab_X11_drawable(xcb_drawable_t drawable) {
  return 0;
}

int main (int argc, char **argv){
  int screen_number;
  xcb_connection_t *xcb_connection;
  init(&xcb_connection, &screen_number);

  print_screen_info(xcb_connection, &screen_number);

  deinit(xcb_connection);
  
  return EXIT_SUCCESS;
}

void print_screen_info(xcb_connection_t *xcb_connection, int *screen_number){
  xcb_screen_t *xcb_screen;
  xcb_screen_iterator_t screen_iterator;

  printf("Screen number: %d\n", *screen_number);

  //XCB example does some weird thing to get to screen number 0. Look into this to improve this later, but as this likely isn't very important, I will leave this as is for now. 
  screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(xcb_connection));
  xcb_screen = screen_iterator.data;

  printf("Width: %d\n", xcb_screen->width_in_pixels);
  printf("Height: %d\n", xcb_screen->height_in_pixels);
  printf("White Pixel: %d\n", xcb_screen->white_pixel);
  printf("Root depth: %d\n", xcb_screen->root_depth);
}

void init(xcb_connection_t **xcb_connection, int *screen_number){
  *xcb_connection = xcb_connect(NULL, screen_number);
}

void deinit(xcb_connection_t *xcb_connection){
  xcb_disconnect(xcb_connection);
}
