#include <ioa/ioa.hpp>
#include <ioa/global_fifo_scheduler.hpp>
#include <ioa/alarm_automaton.hpp>
#include <ioa/buffer.hpp>

#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <jpeglib.h>

class camera_automaton :
  public ioa::automaton
{
private:
  enum state_t {
    SET_READY,
    ALARM_WAIT,
    SEND_READY,
  };

  state_t m_state;
  ioa::handle_manager<camera_automaton> m_self;
  bool m_send_red;
  ioa::const_shared_ptr<ioa::buffer> m_red;
  ioa::const_shared_ptr<ioa::buffer> m_green;

public:
  camera_automaton () :
    m_state (SET_READY),
    m_self (ioa::get_aid ())
  {
    int fd;
    struct stat stats;

    fd = open ("red.jpg", O_RDONLY);
    if (fd == -1) {
      perror ("open");
      exit (EXIT_FAILURE);
    }

    if (fstat (fd, &stats) == -1) {
      perror ("fstat");
      exit (EXIT_FAILURE);
    }

    ioa::buffer* red_buffer = new ioa::buffer (stats.st_size);
    if (read (fd, red_buffer->data (), stats.st_size) != stats.st_size) {
      perror ("read");
      exit (EXIT_FAILURE);
    }

    close (fd);

    fd = open ("green.jpg", O_RDONLY);
    if (fd == -1) {
      perror ("open");
      exit (EXIT_FAILURE);
    }

    if (fstat (fd, &stats) == -1) {
      perror ("fstat");
      exit (EXIT_FAILURE);
    }

    ioa::buffer* green_buffer = new ioa::buffer (stats.st_size);
    if (read (fd, green_buffer->data (), stats.st_size) != stats.st_size) {
      perror ("read");
      exit (EXIT_FAILURE);
    }

    close (fd);

    m_red.reset (red_buffer);
    m_green.reset (green_buffer);

    ioa::automaton_manager<ioa::alarm_automaton>* alarm = new ioa::automaton_manager<ioa::alarm_automaton> (this, ioa::make_generator<ioa::alarm_automaton> ());

    ioa::make_binding_manager (this,
			       &m_self, &camera_automaton::set,
			       alarm, &ioa::alarm_automaton::set);

    ioa::make_binding_manager (this,
			       alarm, &ioa::alarm_automaton::alarm,
			       &m_self, &camera_automaton::alarm);

    schedule ();
  }

private:
  void schedule () const {
    if (set_precondition ()) {
      ioa::schedule (&camera_automaton::set);
    }
    if (send_precondition ()) {
      ioa::schedule (&camera_automaton::send);
    }
  }

private:
  bool set_precondition () const {
    return m_state == SET_READY && ioa::binding_count (&camera_automaton::set) != 0;
  }

  ioa::time set_effect () {
    m_state = ALARM_WAIT;
    return ioa::time (1, 0);
  }

  V_UP_OUTPUT (camera_automaton, set, ioa::time);

  void alarm_effect () {
    assert (m_state == ALARM_WAIT);
    m_state = SEND_READY;
  }

  UV_UP_INPUT (camera_automaton, alarm);

  bool send_precondition () const {
    return m_state == SEND_READY;  // Don't care if its bound.
  }

  ioa::const_shared_ptr<ioa::buffer> send_effect () {
    m_send_red = !m_send_red;
    m_state = SET_READY;  
    if (m_send_red) {
      return m_red;
    }
    else {
      return m_green;
    }
  }

public:
  V_UP_OUTPUT (camera_automaton, send, ioa::const_shared_ptr<ioa::buffer>);
};

class display_automaton :
  public ioa::automaton
{
private:
  enum state_t {
    SCHEDULE_READ_READY,
    READ_READY_WAIT,
  };

  struct rgb_t {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t unused;
  };

  static const unsigned int X_OFFSET = 0;
  static const unsigned int Y_OFFSET = 0;
  // I assume a fixed size.  Generalize later.
  static const unsigned int WIDTH = 352;
  static const unsigned int HEIGHT = 288;
  static const unsigned int BORDER_WIDTH = 0;

  state_t m_state;
  Display* m_display;
  int m_fd;
  int m_screen;
  Window m_window;
  Atom m_del_window;
  rgb_t m_data[WIDTH * HEIGHT];
  XImage* m_image;
  ioa::const_shared_ptr<ioa::buffer> m_buffer;

public:
  display_automaton () :
    m_state (SCHEDULE_READ_READY)
  {
    // Open connection with the X server.
    m_display = XOpenDisplay (NULL);
    if (m_display == NULL) {
      std::cerr << "Cannot open display" << std::endl;
      exit (EXIT_FAILURE);
    }

    // Get the connection file descriptor.
    m_fd = ConnectionNumber (m_display);

    // Get the flags.
    int flags = fcntl (m_fd, F_GETFL, 0);
    if (flags < 0) {
      perror ("fcntl");
      exit (EXIT_FAILURE);
    }

    // Set non-blocking.
    flags |= O_NONBLOCK;
    if (fcntl (m_fd, F_SETFL, flags) == -1) {
      perror ("fcntl");
      exit (EXIT_FAILURE);
    }

    m_screen = DefaultScreen (m_display);

    // Create a window.
    m_window = XCreateSimpleWindow (m_display,
				    RootWindow (m_display, m_screen),
				    X_OFFSET, Y_OFFSET, WIDTH, HEIGHT, BORDER_WIDTH,
				    BlackPixel (m_display, m_screen),
				    WhitePixel (m_display, m_screen));
  
    // Prosses Window Close Event through event handler so XNextEvent does Not fail
    m_del_window = XInternAtom (m_display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols (m_display, m_window, &m_del_window, 1);
    
    // Select events in which we are interested.
    XSelectInput (m_display, m_window, StructureNotifyMask | ExposureMask);
  
    // Show the window.
    XMapWindow (m_display, m_window);
    XFlush (m_display);
    
    int depth = DefaultDepth (m_display, m_screen);
    Visual* visual = DefaultVisual (m_display, m_screen);
    
    // I assume these.  Generalize later.
    assert (depth == 24);
    assert (visual->red_mask   == 0x00FF0000);
    assert (visual->green_mask == 0x0000FF00);
    assert (visual->blue_mask  == 0x000000FF);
    
    m_image = XCreateImage (m_display,
			    CopyFromParent,
			    depth,
			    ZPixmap,
			    0,
			    reinterpret_cast<char*> (m_data),
			    WIDTH,
			    HEIGHT,
			    32,
			    0);
    
    assert (1 == XInitImage (m_image));
    schedule ();
  }
  
  ~display_automaton () {
    delete[] m_data;
  }

private:
  void schedule () const {
    if (schedule_read_precondition ()) {
      ioa::schedule (&display_automaton::schedule_read);
    }
  }

  bool schedule_read_precondition () const {
    return m_state == SCHEDULE_READ_READY;
  }

  void schedule_read_effect () {
    ioa::schedule_read_ready (&display_automaton::read_ready, m_fd);
    m_state = READ_READY_WAIT;
  }

  UP_INTERNAL (display_automaton, schedule_read);

  bool read_ready_precondition () const {
    return m_state == READ_READY_WAIT;
  }

  void read_ready_effect () {
    XEvent event;
    
    // Handle XEvents and flush the input.
    if (XPending (m_display)) {
      XNextEvent (m_display, &event);
      if (event.type == Expose) {
	XPutImage(m_display, 
		  m_window,
		  DefaultGC (m_display, m_screen),
		  m_image,
		  0, 0,
		  0, 0,
		  WIDTH, HEIGHT);
      }
    }

    m_state = SCHEDULE_READ_READY;
  }

  UP_INTERNAL (display_automaton, read_ready);


  static void init_source (struct jpeg_decompress_struct* cinfo) {
    // Initialized before so do nothing.
  }
  
  static boolean fill_input_buffer (struct jpeg_decompress_struct* cinfo) {
    assert (false);
  }
  
  static void skip_input_data (struct jpeg_decompress_struct* cinfo,
			       long numbytes) {
    cinfo->src->next_input_byte += numbytes;
    cinfo->src->bytes_in_buffer -= numbytes;
  }
  
  static void term_source (struct jpeg_decompress_struct* cinfo) {
    /* Do nothing. */
  }

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer>& buf) {
    if (buf.get () != 0) {
      m_buffer = buf;
      struct jpeg_decompress_struct cinfo;
      struct jpeg_error_mgr jerr;
      struct jpeg_source_mgr source;

      source.next_input_byte = static_cast<const JOCTET*> (m_buffer->data ());
      source.bytes_in_buffer = m_buffer->size ();
    
      source.init_source = init_source;
      source.fill_input_buffer = fill_input_buffer;
      source.skip_input_data = skip_input_data;
      source.resync_to_restart = jpeg_resync_to_restart;
      source.term_source = term_source;
      
      cinfo.err = jpeg_std_error (&jerr);
      jpeg_create_decompress (&cinfo);
      cinfo.src = &source;
      jpeg_read_header (&cinfo, TRUE);

      jpeg_start_decompress (&cinfo);

      assert (cinfo.out_color_components == 3);

      assert (cinfo.output_width == WIDTH);
      assert (cinfo.output_height == HEIGHT);

      size_t row_stride = sizeof (JSAMPLE) * 3 * cinfo.output_width;
      JSAMPROW row_pointer[1];
      row_pointer[0] = new JSAMPLE[3 * cinfo.output_width * cinfo.rec_outbuf_height];
      while (cinfo.output_scanline < cinfo.output_height) {
	JDIMENSION num_lines = jpeg_read_scanlines (&cinfo, row_pointer, cinfo.rec_outbuf_height);
	JDIMENSION y = cinfo.output_scanline - 1;
       	for (JDIMENSION s = 0; s < num_lines; ++s, ++y) {
       	  JDIMENSION x;
       	  for (x = 0; x < cinfo.output_width; ++x) {
	    m_data[y * cinfo.output_width + x].red = row_pointer[0][s * row_stride + 3 * x];
	    m_data[y * cinfo.output_width + x].green = row_pointer[0][s * row_stride + 3 * x + 1];
	    m_data[y * cinfo.output_width + x].blue = row_pointer[0][s * row_stride + 3 * x + 2];
	  }
       	}
      }
      delete[] row_pointer[0];

      jpeg_finish_decompress (&cinfo);
      
      jpeg_destroy_decompress (&cinfo);

      XPutImage (m_display,
		 m_window,
		 DefaultGC (m_display, m_screen),
		 m_image,
		 0, 0,
		 0, 0,
		 WIDTH, HEIGHT);
      XFlush (m_display);
    }
  }

public:
  V_UP_INPUT (display_automaton, receive, ioa::const_shared_ptr<ioa::buffer>);
};

class display_driver_automaton :
  public ioa::automaton
{
public:
  display_driver_automaton () {
    ioa::automaton_manager<camera_automaton>* camera = new ioa::automaton_manager<camera_automaton> (this, ioa::make_generator<camera_automaton> ());
    ioa::automaton_manager<display_automaton>* display = new ioa::automaton_manager<display_automaton> (this, ioa::make_generator<display_automaton> ());

    ioa::make_binding_manager (this,
			       camera, &camera_automaton::send,
			       display, &display_automaton::receive);
			       
  }
};

int main (int argc, char* argv[]) {
  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<display_driver_automaton> ());
  return 0;
}
