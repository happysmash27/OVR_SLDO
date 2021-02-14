#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "desktopview.h"

#include <unistd.h>
#include <time.h>

//Ignoring these functions for now until I figure out how to grab individual windows
//shm_framebuffer_t *create_framebuffer_from_X11_drawable(xcb_context_t *xcb_context, xcb_drawable_t drawable);
//
//void new_framebuffer_for_X11_drawable(ovr_sldo_context_t *ovr_sldo, xcb_drawable_t drawable);

ovr_sldo_context_t *init();

void deinit(ovr_sldo_context_t *ovr_sldo);

void print_root_window_test(xcb_context_t *xcb_context);

void capture_root_window(ovr_sldo_context_t *ovr_sldo);

void print_screen_info(xcb_context_t *xcb_context);

//Ignoring this function for now until I figure out how to grab individual windows
/*int grab_X11_drawable(xcb_drawable_t drawable){
  return 0;
}*/

//Instead, I will use this one that only grabs the root window
void *update_framebuffer_with_root(void *args){
  shm_framebuffer_t *framebuffer = (shm_framebuffer_t *) args;
  
  int exit_code = EXIT_SUCCESS;

  //Find which buffer to write to. It must not be the one pointed to for the next read, nor being written to or read from
  int write_this_buffer;
  for (write_this_buffer=0; write_this_buffer<framebuffer->number_of_buffers; write_this_buffer++){
    if ((write_this_buffer == framebuffer->read_this_buffer) || (framebuffer->data[write_this_buffer].lock)){
      continue;
    } else {
      framebuffer->data[write_this_buffer].lock = 1;
      break;
    }
  }

  
  //Grab our image

  //First set up variables
  xcb_shm_get_image_cookie_t image_request;
  xcb_shm_get_image_reply_t *image_reply;
  xcb_generic_error_t *e = NULL;
  
  //Now request our image to fill the framebuffer
  //Use XCB_IMAGE_FORMAT_Z_PIXMAP, as I am not yet sure how to read XCB_IMAGE_FORMAT_XY_PIXMAP due to lack of examples
  //Currently only use 0 on screen_itr. This should be changed to support multiple screens in the future
  image_request = xcb_shm_get_image(xcb_context->connection, xcb_context->scrn_itr.data->root, 0, 0, framebuffer->width, framebuffer->height, ~0, XCB_IMAGE_FORMAT_Z_PIXMAP, xcb_context->shm_segment[0][write_this_buffer], 0);

   //To see if libXCB is fast enough, we will create variables for the start and end time of requesting an image
  struct timespec start_time, end_time;
  //Capture our start time
  clock_gettime(CLOCK_REALTIME, &start_time);
  //Immediately request a reply, so we wait until the framebuffer has been filled
  image_reply = xcb_shm_get_image_reply(xcb_context->connection, image_request, &e);

  //Now capture the end time of the request and calculate the time ellapsed in milliseconds
  //Capture end time
  clock_gettime(CLOCK_REALTIME, &end_time);
  //Now we have time to convert our variables to long ints of milliseconds. 
  long long int start_time_ms, end_time_ms, ellapsed_time_ms;
  start_time_ms = start_time.tv_sec*1000 + start_time.tv_nsec/1000000;
  end_time_ms = end_time.tv_sec*1000 + end_time.tv_nsec/1000000;
  //And calculate the difference in time into one easy-to-print variable that won't print our seconds if it is not needed (and it shouldn't be needed, as this capture is supposed to take 16 ms at max)
  ellapsed_time_ms = end_time_ms-start_time_ms;
  //Print the time ellapsed
  fprintf(stderr, "XCB wait time: %lld ms\n", ellapsed_time_ms);

  //Error handling, including very vague messages about what went wrong
  //I used this to figure out that 3 bytes per pixel wasn't enough. Maybe it will also be useful in the future
  if (e){
    fprintf(stderr, "Error getting image reply!\n"
	    "\n"
	    "Response type %u\n"
	    "Error code %u: %s\n"
	    "Sequence number %u\n"
	    "Resource ID %u\n"
	    "Major opcode %u: %s\n"
	    "Minor opcode %u: %s\n"
	    "\n", e->response_type, e->error_code, xcb_errors_get_name_for_error(xcb_context->errors_context, e->error_code, NULL), e->sequence, e->resource_id, e->major_code, xcb_errors_get_name_for_major_code(xcb_context->errors_context, e->major_code), e->minor_code, xcb_errors_get_name_for_minor_code(xcb_context->errors_context, e->major_code, e->minor_code));
    exit_code = EXIT_FAILURE;
  }

  //This needs to be freed at the end
  free(image_reply);
  
  //Remove our lock, tell our program to read the freshest structure, and exit
  framebuffer->data[write_this_buffer].lock = 0;
  framebuffer->read_this_buffer = write_this_buffer;
  pthread_exit(&exit_code);  
}

void write_image_BGRA_ppm(shm_framebuffer_t *framebuffer){
  //Just going to print to stdout for now, ppm format
  //Hardcode 255, because this is only meant as a placeholder, and doesn't need to be dynamic
  fprintf(stdout, "P3\n%d %d 255\n", framebuffer->width, framebuffer->height);
  
  for (int i=0; i < framebuffer->size; i+=4){
    fprintf(stdout, "%hhu %hhu, %hhu\n",
	    framebuffer->data[framebuffer->read_this_buffer].addr[i+2],
	    framebuffer->data[framebuffer->read_this_buffer].addr[i+1],
	    framebuffer->data[framebuffer->read_this_buffer].addr[i]);
  }
  
}

int main (int argc, char **argv){
  int exit_code = EXIT_SUCCESS;
  void *thread_retval_ptr;
  
  ovr_sldo_context_t *ovr_sldo = init();

  
  //To see if we are fast enough, we will create variables for our start time and end time to time our function
  struct timespec start_time, end_time;
  //Capture our start time
  clock_gettime(CLOCK_REALTIME, &start_time);

  //Create threads to write our framebuffer twice, to see if our problem is speed or latency
  ovr_sldo->num_threads = 60;
  ovr_sldo->threads = calloc(ovr_sldo->num_threads, sizeof(pthread_t));
  
  //Capture ovr_sldo->num_threads frames for our speed vs latency testing
  for (int i=0; i<ovr_sldo->num_threads; i++){
    if (pthread_create(&(ovr_sldo->threads[i]), NULL,
		       update_framebuffer_with_root,
		       &(ovr_sldo->framebuffer[0]))){
      fprintf(stderr, "Error creating thread %d!\n", i);
      exit_code = EXIT_FAILURE;
    }
    //Wait 16 ms for next frame
    usleep(16*1000);
  }

  //Wait for our last thread to exit
  if(pthread_join(ovr_sldo->threads[ovr_sldo->num_threads-1], &thread_retval_ptr)){
    fprintf(stderr, "Error joining thread!\n");
    exit_code = EXIT_FAILURE;
  }
  if (*((int*)thread_retval_ptr)){
    fprintf(stderr, "Image failed to capture!\n");
    exit_code = EXIT_FAILURE;
  }

  //Now capture our end time and calculate our time ellapsed in milliseconds
  //Capture end time ASAP
  clock_gettime(CLOCK_REALTIME, &end_time);
  //Now we have time to convert our variables to long ints of milliseconds. 
  long long int start_time_ms, end_time_ms, ellapsed_time_ms;
  start_time_ms = start_time.tv_sec*1000 + start_time.tv_nsec/1000000;
  end_time_ms = end_time.tv_sec*1000 + end_time.tv_nsec/1000000;
  //And calculate the difference in time into one easy-to-print variable that won't print our seconds if it is not needed (and it shouldn't be needed, as this capture is supposed to take 16 ms at max)
  ellapsed_time_ms = end_time_ms-start_time_ms;
  //Print our time ellapsed
  fprintf(stderr, "Capture time: %lld ms\n", ellapsed_time_ms);

  //Sleep to make sure we have enough time to get a full frame
  usleep(40*1000);

  //Read our framebuffer
  write_image_BGRA_ppm(&(ovr_sldo->framebuffer[0]));

  deinit(ovr_sldo);
  
  return exit_code;
}

void print_screen_info(xcb_context_t *xcb_context){
  xcb_screen_t *xcb_screen;
  xcb_screen_iterator_t screen_iterator;

  printf("Screen number: %d\n", xcb_context->screen_number);

  //XCB example does some weird thing to get to screen number 0. Look into this to improve this later, but as this likely isn't very important, I will leave this as is for now. 
  screen_iterator = xcb_setup_roots_iterator(xcb_get_setup(xcb_context->connection));
  xcb_screen = screen_iterator.data;

  printf("Width: %d\n", xcb_screen->width_in_pixels);
  printf("Height: %d\n", xcb_screen->height_in_pixels);
  printf("White Pixel: %d\n", xcb_screen->white_pixel);
  printf("Root depth: %d\n", xcb_screen->root_depth);
}

ovr_sldo_context_t *init(){
  //Set up out main context
  //ovr_sldo_context_t *ovr_sldo = malloc(sizeof (ovr_sldo_context_t));
  ovr_sldo = malloc(sizeof (ovr_sldo_context_t));

  //Set up xcb context
  ovr_sldo->xcb_context = malloc(sizeof (xcb_context_t));
  xcb_context=ovr_sldo->xcb_context;
  ovr_sldo->xcb_context->connection = xcb_connect(NULL, &ovr_sldo->xcb_context->screen_number);
  xcb_errors_context_new(ovr_sldo->xcb_context->connection, &(ovr_sldo->xcb_context->errors_context));

  //For now, we will only read from the root window, and will therefore have only one framebuffer struct
  ovr_sldo->number_of_buffers = 1;

  //Initialise the framebuffer struct
  ovr_sldo->framebuffer = calloc(1, sizeof (shm_framebuffer_t));
  //We have 3 framebuffers, so that if we finish writing to a fresh framebuffer but another thread is reading from an old one, we can write to a third framebuffer so that when this reading is done, the other thread can immediately read the fresh one we just wrote instead of waiting for us to write it again
  ovr_sldo->framebuffer[0].number_of_buffers = 6;

  //Also initialise out single pointer to an array of shm segments (for framebuffer[0]->data
  ovr_sldo->xcb_context->shm_segment = calloc((size_t) ovr_sldo->number_of_buffers,
  						 sizeof(xcb_shm_seg_t*));
  //Initialise the array itself
  ovr_sldo->xcb_context->shm_segment[0] = calloc((size_t) ovr_sldo->framebuffer[0].number_of_buffers,
  						 sizeof(xcb_shm_seg_t));

  //Initialise our iterator for screen data
  ovr_sldo->xcb_context->scrn_itr = xcb_setup_roots_iterator(xcb_get_setup(ovr_sldo->xcb_context->connection));
  //Use this data to initialise our width and height. Eventually this should be dynamic, as I detail later.
  ovr_sldo->framebuffer[0].width = ovr_sldo->xcb_context->scrn_itr.data->width_in_pixels;
  ovr_sldo->framebuffer[0].height = ovr_sldo->xcb_context->scrn_itr.data->height_in_pixels;
  //Now use our width and height to figure out how big our framebuffers should be. As XCB_IMAGE_FORMAT_Z_PIXMAP comprises 4 bytes of data in a B8G8R8A8 format, we will multiply the width times height by 4.
  ovr_sldo->framebuffer[0].size = (ovr_sldo->framebuffer[0].width)*(ovr_sldo->framebuffer[0].height)*4;

  //Initialise the framebuffers with our shared memory ID, shared memory address, and lock indicator
  //The memory allocation should be dynamically generated in the runtime fubnction eventually to allow it to change size, but as I am still unclear on how to re-allocate shared memory, for now I will do it here.
  //First generate an array of sub-buffers. We use this, rather than an array of pointers, so we don't keep looking up pointers so much... And because I realised that's what my datatype suggests, so I may as well go with that accidental extra efficiency. We won't be adding extra framebuffers to our 3 too often, so we don't really need to make it faster.
  ovr_sldo->framebuffer[0].data = calloc((size_t) ovr_sldo->framebuffer[0].number_of_buffers,
					 sizeof(shm_subbuf_t));
  //Do our loop of initialisation! 
  for (int i=0; i < ovr_sldo->framebuffer[0].number_of_buffers; i++){
    //Request shared memory, and store the ID
    //0666 means permission to read and write by the user, the group, and others. Not quite sure how group and others applies to a process... but may as well try to enable everything so it works. Well, other than execute, which shouldn't affect anything. 
    ovr_sldo->framebuffer[0].data[i].id = shmget(IPC_PRIVATE, ovr_sldo->framebuffer[0].size, IPC_CREAT | 0666);
    //Get the address of our shared memory
    //Not using SHM_RDONLY for the last option, just in case. Might change this in the future
    ovr_sldo->framebuffer[0].data[i].addr = (unsigned char *) shmat(ovr_sldo->framebuffer[0].data[i].id, NULL, 0);
    //Initialise the lock on this shared memory as unlocked
    ovr_sldo->framebuffer[0].data[i].lock = 0;

    //Create XCB SHM segment for getting out data from XCB via SHM, initialise it, and attach it to our array
    ovr_sldo->xcb_context->shm_segment[0][i] = xcb_generate_id(ovr_sldo->xcb_context->connection);
    
    xcb_shm_attach(ovr_sldo->xcb_context->connection,
		   ovr_sldo->xcb_context->shm_segment[0][i],
		   ovr_sldo->framebuffer[0].data[i].id, 0);
  }
  
  //Initialising this to the second buffer so our capture function uses the first one first
  ovr_sldo->framebuffer[0].read_this_buffer = 1;

  return ovr_sldo;
}

void deinit(ovr_sldo_context_t *ovr_sldo){
  //Free framebuffers and shm segments
  int i, j;
  for (i=0; i < ovr_sldo->number_of_buffers; i++){
    free(ovr_sldo->xcb_context->shm_segment[i]);
    for (j=0; j < ovr_sldo->framebuffer[i].number_of_buffers; j++){
      shmdt(ovr_sldo->framebuffer[i].data[j].addr);
    }
    free(ovr_sldo->framebuffer[i].data);
    free(ovr_sldo->framebuffer);
  }
  free(ovr_sldo->xcb_context->shm_segment);

  //Free XCB context
  xcb_errors_context_free(ovr_sldo->xcb_context->errors_context);
  xcb_disconnect(ovr_sldo->xcb_context->connection);
  free(ovr_sldo->xcb_context);

  //Free our main context struct
  free(ovr_sldo);
}
