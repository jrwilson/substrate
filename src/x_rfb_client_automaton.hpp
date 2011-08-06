#ifndef __x_rfb_client_automaton_hpp__
#define __x_rfb_client_automaton_hpp__

#include "rfb.hpp"

#include <ioa/ioa.hpp>

#include <iostream>
#include <queue>
#include <functional>

#include <fcntl.h>
// #include <stdio.h>
#include <X11/Xlib.h>

// TODO:  Clean-up this file.

class x_rfb_client_automaton :
  public ioa::automaton
{
private:

  struct recv_protocol_version_gramel :
    public rgram::gramel
  {
    x_rfb_client_automaton& m_client;
    rfb::protocol_version_gramel m_protocol_version;

    recv_protocol_version_gramel (x_rfb_client_automaton& client) :
      m_client (client)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_protocol_version.put (buf);
      if (done ()) {
	m_client.recv_protocol_version (m_protocol_version.get ());
      }
    }

    bool done () const {
      return m_protocol_version.done ();
    }

    void reset () {
      m_protocol_version.reset ();
    }
  };

  struct recv_security_type_gramel :
    public rgram::gramel
  {
    x_rfb_client_automaton& m_client;
    rfb::security_type_gramel m_security_type;

    recv_security_type_gramel (x_rfb_client_automaton& client) :
      m_client (client)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_security_type.put (buf);
      if (done ()) {
	m_client.recv_security_type (m_security_type.get ());
      }
    }

    bool done () const {
      return m_security_type.done ();
    }

    void reset () {
      m_security_type.reset ();
    }
  };

  struct recv_server_init_gramel :
    public rgram::gramel
  {
    x_rfb_client_automaton& m_client;
    rfb::server_init_gramel m_init;

    recv_server_init_gramel (x_rfb_client_automaton& client) :
      m_client (client)
    {
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_init.put (buf);
      if (done ()) {
	m_client.recv_server_init (m_init.get ());
      }
    }

    bool done () const {
      return m_init.done ();
    }

    void reset () {
      m_init.reset ();
    }
  };

  struct recv_server_message_gramel :
    public rgram::gramel
  {
    x_rfb_client_automaton& m_client;
    rfb::server_message_gramel m_message;

    recv_server_message_gramel (x_rfb_client_automaton& client) :
      m_client (client)
    {
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_message.put (buf);
      if (done ()) {
	switch (m_message.m_choice.get ()) {
	case rfb::FRAMEBUFFER_UPDATE_TYPE:
	  m_client.recv_framebuffer_update ();
	  break;
	default:
	  std::cerr << "Unknown server message.  Type = " << int (m_message.m_choice.get ()) << std::endl;
	  abort ();
	}
	// Reset to start over.
	m_message.reset ();
      }
    }

    bool done () const {
      return m_message.done ();
    }

    void reset () {
      m_message.reset ();
    }

    void add_encoding (const int32_t type,
		       rfb::pixel_data_gramel* gramel) {
      m_message.add_encoding (type, gramel);
    }
  };

  struct protocol_gramel :
    public rgram::gramel
  {
    recv_protocol_version_gramel m_recv_protocol;
    recv_security_type_gramel m_recv_security;
    recv_server_init_gramel m_recv_init;
    recv_server_message_gramel m_recv_server;
    rgram::sequence_gramel m_sequence;

    protocol_gramel (x_rfb_client_automaton& client) :
      m_recv_protocol (client),
      m_recv_security (client),
      m_recv_init (client),
      m_recv_server (client)
    {
      m_sequence.append (&m_recv_protocol);
      m_sequence.append (&m_recv_security);
      m_sequence.append (&m_recv_init);
      m_sequence.append (&m_recv_server);
    }
    
    void put (rgram::buffer& buf) {
      assert (!done ());
      m_sequence.put (buf);
    }

    bool done () const {
      return m_sequence.done ();
    }

    void reset () {
      m_sequence.reset ();
    }

    void add_encoding (const int32_t type,
		       rfb::pixel_data_gramel* gramel) {
      m_recv_server.add_encoding (type, gramel);
    }
  };

  class pixel_gramel :
    public rgram::gramel
  {
  private:
    uint32_t m_uint;
    uint8_t m_count;

  public:
    typedef uint32_t value_type;

    pixel_gramel () :
      m_count (0)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_count += buf.consume (&m_uint, sizeof (m_uint) - m_count);
    }

    bool done () const {
      return m_count == sizeof (m_uint);
    }

    uint32_t get () const {
      return m_uint;
    }

    void reset () {
      m_count = 0;
    }
  };

  struct raw_pixel_data_gramel :
    public rfb::pixel_data_gramel
  {
    x_rfb_client_automaton& m_client;
    uint16_t x_position;
    uint16_t y_position;
    uint16_t width;
    uint16_t height;
    uint16_t x_off;
    uint16_t y_off;
    pixel_gramel pixel;
    bool dimensions_set;

    raw_pixel_data_gramel (x_rfb_client_automaton& client) :
      m_client (client),
      x_off (0),
      y_off (0),
      dimensions_set (false)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());

      while (!done () && !buf.empty ()) {
	pixel.put (buf);
	if (pixel.done ()) {
	  m_client.m_data[(y_position + y_off) * m_client.WIDTH + (x_position + x_off)] = pixel.get ();
	  ++x_off;
	  if (x_off == width) {
	    x_off = 0;
	    ++y_off;
	  }
	  pixel.reset ();
	}
      }
    }

    bool done () const {
      return dimensions_set && y_off == height;
    }

    void reset () {
      x_off = 0;
      y_off = 0;
      pixel.reset ();
      dimensions_set = false;
    }

    void set_dimensions (const uint16_t xpos,
			 const uint16_t ypos,
			 const uint16_t w,
			 const uint16_t h) {
      x_position = xpos;
      y_position = ypos;
      width = w;
      height = h;
      dimensions_set = true;
    }
  };

  raw_pixel_data_gramel m_raw_pixel_data;
  protocol_gramel m_protocol;
  std::queue<ioa::const_shared_ptr<ioa::buffer_interface> > m_sendq;
  const rfb::protocol_version_t HIGHEST_VERSION;
  rfb::protocol_version_t m_protocol_version;
  rfb::pixel_format_t m_pixel_format;
  std::vector<int32_t> m_encodings;
  bool m_incremental;
  static const uint16_t WIDTH = 640;
  static const uint16_t HEIGHT = 480;
  uint16_t m_server_width;
  uint16_t m_server_height;

  enum state_t {
    SCHEDULE_READ_READY,
    READ_READY_WAIT,
  };

  static const unsigned int X_OFFSET = 0;
  static const unsigned int Y_OFFSET = 0;
  static const unsigned int BORDER_WIDTH = 0;

  state_t m_state;
  Display* m_display;
  int m_fd;
  int m_screen;
  Window m_window;
  Atom m_del_window;
  uint32_t m_data[WIDTH * HEIGHT];
  XImage* m_image;

public:
  x_rfb_client_automaton () :
    m_raw_pixel_data (*this),
    m_protocol (*this),
    HIGHEST_VERSION (rfb::PROTOCOL_VERSION_3_3),
    m_incremental (false), // Request entire screen first time.
    m_state (SCHEDULE_READ_READY)
  {
    m_protocol.add_encoding (rfb::RAW, &m_raw_pixel_data);
    m_encodings.push_back (rfb::RAW);
    // m_encodings.push_back (rfb::COPY_RECT);
    // m_encodings.push_back (rfb::RRE);
    // m_encodings.push_back (rfb::HEXTILE);
    // m_encodings.push_back (rfb::ZRLE);

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
    
    m_pixel_format.depth = DefaultDepth (m_display, m_screen);
    Visual* visual = DefaultVisual (m_display, m_screen);
    
    // Set the pixel format.
    m_pixel_format.bits_per_pixel = 32;
    assert (m_pixel_format.depth == 24);
    m_pixel_format.big_endian_flag = (htonl(1) == 1);
    m_pixel_format.true_colour_flag = true;
    if (visual->red_mask == 0x00FF0000 &&
	visual->green_mask == 0x0000FF00 &&
	visual->blue_mask  == 0x000000FF) {
      m_pixel_format.red_max = 0xFF;
      m_pixel_format.green_max = 0xFF;
      m_pixel_format.blue_max = 0xFF;
      m_pixel_format.red_shift = 16;
      m_pixel_format.green_shift = 8;
      m_pixel_format.blue_shift = 0;
    }
    else {
      std::cerr << "Unknown X pixel format" << std::endl;
      exit (EXIT_FAILURE);
    }
    
    std::cout << "client: big_endian = " << int (m_pixel_format.big_endian_flag) << std::endl;

    m_image = XCreateImage (m_display,
    			    CopyFromParent,
    			    m_pixel_format.depth,
    			    ZPixmap,
    			    0,
    			    reinterpret_cast<char*> (m_data),
    			    WIDTH,
    			    HEIGHT,
    			    32,
    			    0);
    
    assert (1 == XInitImage (m_image));

    // Black it out.
    memset (m_data, 0, HEIGHT * WIDTH * sizeof (uint32_t));

    schedule ();
  }

private:
  void schedule () const {
    if (send_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send);
    }
    if (schedule_read_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::schedule_read);
    }
  }

private:

  void recv_protocol_version (const rfb::protocol_version_t& version) {
    std::cout << "client: " << __func__ << std::endl;

    if (version == rfb::PROTOCOL_VERSION_3_3) {
      // The server send us a version we can understand.
      // Use the lowest version mutually supported.
      m_protocol_version = std::min (version, HIGHEST_VERSION);
    }
    else {
      // The version is either too high, too low, or garbage.
      // Use our own and let the server disconnect if not supported.
      m_protocol_version = HIGHEST_VERSION;
    }

    send_protocol_version ();
  }

  void send_protocol_version () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    m_protocol_version.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void recv_security_type (const rfb::security_type_t& msg) {
    std::cout << "client: " << __func__ << std::endl;
    assert (msg.security == rfb::NONE);

    send_client_init ();
  }
  
  void send_client_init () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::client_init_t msg (true);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void recv_server_init (const rfb::server_init_t& msg) {
    std::cout << "client: " << __func__ << std::endl;
    m_server_width = std::min (WIDTH, msg.framebuffer_width);
    m_server_height = std::min (HEIGHT, msg.framebuffer_height);
    // We are going to ignore the server pixel format and send our own.

    send_set_pixel_format ();
    send_set_encodings ();
    send_framebuffer_update_request ();
  }

  void send_set_pixel_format () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::set_pixel_format_t msg (m_pixel_format);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void send_set_encodings () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::set_encodings_t msg (m_encodings);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void send_framebuffer_update_request () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    // We want the whole image.
    rfb::framebuffer_update_request_t msg (m_incremental, 0, 0, m_server_width, m_server_height);
    // Can switch to incremental.
    m_incremental = true;
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void recv_framebuffer_update () {
    std::cout << "client: " << __func__ << std::endl;

    ioa::time now = ioa::time::now ();
    std::cout << "@(" << now.sec () << "," << now.usec () << ")" << std::endl;

    // Show the image.
    // TODO:  Only show the part that changed.
    XPutImage (m_display,
    	       m_window,
    	       DefaultGC (m_display, m_screen),
    	       m_image,
    	       0, 0,
    	       0, 0,
    	       WIDTH, HEIGHT);
    XFlush (m_display);

    // Request.
    send_framebuffer_update_request ();
  }

  // Send a message.
  bool send_precondition () const {
    return !m_sendq.empty () && ioa::binding_count (&x_rfb_client_automaton::send) != 0;
  }

  ioa::const_shared_ptr<ioa::buffer_interface> send_effect () {
    ioa::const_shared_ptr<ioa::buffer_interface> retval = m_sendq.front ();
    m_sendq.pop ();
    return retval;
  }

public:
  V_UP_OUTPUT (x_rfb_client_automaton, send, ioa::const_shared_ptr<ioa::buffer_interface>);

private:

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    if (val.get () != 0) {
      rgram::buffer rbuf (*val.get ());
      m_protocol.put (rbuf);
    }
  }

public:
  V_UP_INPUT (x_rfb_client_automaton, receive, ioa::const_shared_ptr<ioa::buffer_interface>);

private:
  bool schedule_read_precondition () const {
    return m_state == SCHEDULE_READ_READY;
  }

  void schedule_read_effect () {
    ioa::schedule_read_ready (&x_rfb_client_automaton::read_ready, m_fd);
    m_state = READ_READY_WAIT;
  }

  UP_INTERNAL (x_rfb_client_automaton, schedule_read);

  bool read_ready_precondition () const {
    return m_state == READ_READY_WAIT;
  }

  void read_ready_effect () {
    XEvent event;
    
    // Handle XEvents and flush the input.
    if (XPending (m_display)) {
      XNextEvent (m_display, &event);
      if (event.type == Expose) {
  	XPutImage (m_display, 
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
  
  UP_INTERNAL (x_rfb_client_automaton, read_ready);

};

const uint16_t x_rfb_client_automaton::HEIGHT;
const uint16_t x_rfb_client_automaton::WIDTH;

#endif
