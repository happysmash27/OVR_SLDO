#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_errors.h>

//Number of buffers we start with for a normal window
//For now we use 9 as we don't have dynamic re-allocation programmed yet, and 9 stops us from running out of buffers to fill to
#define INITIAL_BUFNUM 9
//Max buffers we will allocate. 255 is the maximum capacity of our index, but we don't want to go that high to use less memory
#define MAX_BUFNUM 16

typedef struct shm_subbuf_t {
  signed char lock;
  uint32_t id;
  unsigned char *addr;
} shm_subbuf_t;

typedef struct shm_framebuffer_t {
  //Which framebuffer are we?
  int bufnum;
  int width;
  int height;
  size_t size;
  //Here we have the number of buffers for a single window
  unsigned char number_of_subbufs;
  unsigned char read_this_buffer;
  shm_subbuf_t *subbuf;
} shm_framebuffer_t;

typedef struct xcb_context_t {
  xcb_connection_t *connection;
  int screen_number;
  xcb_screen_iterator_t scrn_itr;
  //Array of arrays of shm segments, that for now, will match the number of shm_framebuffer_t and number of framebuffers inside each
  xcb_shm_seg_t **shm_segment;
  xcb_errors_context_t *errors_context;
} xcb_context_t;

typedef struct ovr_sldo_context_t {
  xcb_context_t *xcb_context;
  size_t num_threads;
  pthread_t *threads;
  //Here we have the number of buffers we have for different windows
  unsigned int number_of_buffers;
  shm_framebuffer_t *framebuffer;
} ovr_sldo_context_t;

ovr_sldo_context_t *ovr_sldo;

xcb_context_t *xcb_context;
