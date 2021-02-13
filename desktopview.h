#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_errors.h>

typedef struct shm_subbuf_t {
  signed char lock;
  uint32_t id;
  unsigned char *addr;
} shm_subbuf_t;

typedef struct shm_framebuffer_t {
  int width;
  int height;
  size_t size;
  //Here we have the number of buffers for a single window
  unsigned char number_of_buffers;
  unsigned char read_this_buffer;
  shm_subbuf_t *data;
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
  //Here we have the number of buffers we have for different windows
  unsigned int number_of_buffers;
  shm_framebuffer_t *framebuffer;
} ovr_sldo_context_t;
