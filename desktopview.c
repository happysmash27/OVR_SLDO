#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

int init(xcb_connection_t *xcb_connection, int *screen_number);
int deinit(xcb_connection_t *xcb_connection);

int print_screen_info(xcb_connection_t *xcb_connection, int *screen_number);

uint8_t *grab_X11_drawable(xcb_drawable_t drawable) {
  return 0;
}

int main (int argc, char **argv){
  int *screen_number;
  xcb_connection_t *xcb_connection = xcb_connect(NULL, screen_number);
  //if (init(xcb_connection, screen_number)){
  //  printf("Initialisation failure!");
  //  exit(EXIT_FAILURE);
  //}

  if (print_screen_info(xcb_connection, screen_number)){
    exit(EXIT_FAILURE);
  }

  if (deinit(xcb_connection)){
    printf("Deinitialisation failure!");
    exit(EXIT_FAILURE);
  }
  
  return EXIT_SUCCESS;
}

int init(xcb_connection_t *xcb_connection, int *screen_number){
  xcb_connection = xcb_connect(NULL, screen_number);

  return EXIT_SUCCESS;
}

int deinit(xcb_connection_t *xcb_connection){
  xcb_disconnect(xcb_connection);

  return EXIT_SUCCESS;
}

int print_screen_info(xcb_connection_t *xcb_connection, int *screen_number){
  xcb_screen_t *xcb_screen;

  printf("Screen number: %d\n", *screen_number);

  return EXIT_SUCCESS;
}
