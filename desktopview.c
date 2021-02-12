#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <xcb/xcb.h>
#include <xcb/shm.h>

#include <unistd.h>

typedef struct shm_framebuffer_t {
  int width;
  int height;
  size_t size;
  uint32_t id;
  unsigned char *data;
} shm_framebuffer_t;
  

void init(xcb_connection_t **xcb_connection, int *screen_number);
void deinit(xcb_connection_t *xcb_connection);

void print_root_window_test(xcb_connection_t *xcb_connection);

void print_screen_info(xcb_connection_t *xcb_connection, int *screen_number);

int write_image_ppm(uint8_t image_data);

uint8_t *grab_X11_drawable(xcb_drawable_t drawable) {
  return 0;
}

int main (int argc, char **argv){
  int screen_number;
  xcb_connection_t *xcb_connection;
  init(&xcb_connection, &screen_number);

  print_root_window_test(xcb_connection);

  deinit(xcb_connection);
  
  return EXIT_SUCCESS;
}

void print_root_window_test(xcb_connection_t *xcb_connection){
  //So, I'm not quite sure how this thing works, because documentation is terrible.
  //For this reason, I am creating a simple algorithm outside of the structure I have tried to set up, which will serve as a test for whether I understand XCB SHM correctly
  //My aim is to grab an SHM image, and print it as a PPM

  //Set up screen data. We will need to know how big our buffer needs to be
  xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(xcb_connection));
  xcb_screen_t *xcb_screen = screen_iterator.data;

  //We need a framebuffer for our data... I think. I'm not sure if I am supposed to get a shared memory buffer from XCB, or if I'm supposed to make it myself, but I'll try the latter for now as it looks the most promising.
  //Looks like I need to initialise it to make the pointer work.
  shm_framebuffer_t framebuffer1_direct;
  shm_framebuffer_t *framebuffer1 = &framebuffer1_direct;

  //Set width and height
  //Should probably make this more dynamic in the actual implementation, when it is polished to production
  framebuffer1->width = xcb_screen->width_in_pixels;
  framebuffer1->height = xcb_screen->height_in_pixels;
  //May as well store this statically, since it is a small amount of data for not running this instruction every time
  framebuffer1->size = (framebuffer1->width)*(framebuffer1->height)*3;

  //Allocate a shared memory buffer. 
  //0666 means permission to read and write by the user, the group, and others. Not quite sure how group and others applies to a process... but may as well try to enable everything so it works. Well, other than execute, which shouldn't affect anything. 
  framebuffer1->id = shmget(IPC_PRIVATE, framebuffer1->size, IPC_CREAT | 0666);
  
  //So, uh... I think what I am supposed to do is create an shm segment by requesting an ID from XCB, then attach that that shm segment to my shared memory using xcb_shm_attach? I think this is what ffmpeg does. 
  xcb_shm_seg_t shm_segment = xcb_generate_id(xcb_connection); //This thing has no man page :( . Why is documentation for this library so horrible?
  xcb_shm_attach(xcb_connection, shm_segment, framebuffer1->id, 0);

  //So, our SHM is attached... hopefully. If the program hasn't crashed by now, we will use xcb_shm_get_image, which I think tells it to fill the shared memory buffer with the image? I don't see any way to get the image from the actual reply. Then, we will attach to it. 

  //Set up variables first. 
  xcb_shm_get_image_cookie_t image_request;
  xcb_shm_get_image_reply_t *image_reply;
  xcb_generic_error_t *e = NULL;

  //Now we fill them.
  //I'm not sure what the differences are between the xcb_image_format_ts are, as yet again, I can find nothing online! Such horrible documentation! Anyways, I will do what ffmpeg does, using XCB_IMAGE_FORMAT_Z_PIXMAP, then see what happens if I use XCB_IMAGE_FORMAT_XY_PIXMAP instead, since that also sounds like something I would want
  image_request = xcb_shm_get_image(xcb_connection, xcb_screen->root, 0, 0, framebuffer1->width, framebuffer1->height, ~0, XCB_IMAGE_FORMAT_Z_PIXMAP , shm_segment, 0);

  //And now we immediately request it back. I think this causes it to wait until it has been gathered?
  //Try not to do error handling now, but if the null causes problems, I might have to add it to this early testing function
  image_reply = xcb_shm_get_image_reply(xcb_connection, image_request, &e);

  if (e){
    fprintf(stderr, "Error getting image reply!\n"
	    "\n"
	    "Response type: %u\n"
	    "Error code: %u\n"
	    "Sequence number: %u\n"
	    "Resource ID: %u\n"
	    "Minor opcode: %u\n"
	    "Major opcode: %u\n"
	    "\n", e->response_type, e->error_code, e->sequence, e->resource_id, e->minor_code, e->major_code);
  }

  //Flush things, just to be sure
  xcb_flush(xcb_connection);

  //Because ffmpeg did it, and something is stopping the capture from working
  free(image_reply);

  //Do we have the image yet? Let's try!

  //First, attach to the buffer
  framebuffer1->data = (unsigned char *) shmat(framebuffer1->id, NULL, 0); //Not using SHM_RDONLY to see if it fixes bug
  
  //Just going to print to stdout for now, ppm format
  //Hardcode 255, because this is a test function, and I don't feel like making it dynamic. It should be dynamic in the actual implementation
  fprintf(stdout, "P3\n%d %d 255\n", framebuffer1->width, framebuffer1->height);

  int test = framebuffer1->data[3];
  printf("Test: %d\n", test);
  sleep(1);
  
  for (int i=0; i < framebuffer1->size; i+=3){
    fprintf(stdout, "%hhd %hhd, %hhd\n", framebuffer1->data[i], framebuffer1->data[i+1], framebuffer1->data[i+2]);
  }

  //We are done. Close everything.
  xcb_flush(xcb_connection);
  //free(framebuffer1->data);
  shmdt(framebuffer1->data);
  
  
  
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
