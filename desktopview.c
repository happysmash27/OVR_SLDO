#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

uint8_t *grab_X11_drawable(xcb_drawable_t drawable) {

}

int main (int argc, char **argv){
  xcb_connection_t *xcb_connection;
  init(xcb_connection);

  deinit(xcb_connection);
}

int init(xcb_connection_t *xcb_connection){
  xcb_connection = xcb_connect(NULL, NULL);
}

int deinit(xcb_connection_t *xcb_connection){
  xcb_disconnect xcb_connection;
}
