#ifndef __x_rfb_client_automaton_hpp__
#define __x_rfb_client_automaton_hpp__

#include "rfb.hpp"

#include <ioa/ioa.hpp>

#include <iostream>
#include <queue>

#include <fcntl.h>
// #include <stdio.h>
#include <X11/Xlib.h>

class x_rfb_client_automaton :
  public ioa::automaton
{
private:
  std::queue<ioa::const_shared_ptr<ioa::buffer_interface> > m_sendq;
  ioa::buffer m_buffer;

  bool m_send_protocol_version;
  bool m_send_security_type;
  bool m_send_client_init;
  bool m_send_set_pixel_format;
  bool m_send_set_encodings;
  bool m_send_framebuffer_update_request;

  bool m_recv_ignore;
  bool m_recv_protocol_version;
  bool m_recv_security_types;
  bool m_recv_security_type;
  bool m_recv_security_result;
  bool m_recv_server_init;
  bool m_recv_framebuffer_update;
  bool m_recv_unknown_message;

  const rfb::protocol_version_t HIGHEST_VERSION;
  rfb::protocol_version_t m_protocol_version;
  rfb::security_t m_security;
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
    m_send_protocol_version (false),
    m_send_security_type (false),
    m_send_client_init (false),
    m_send_set_pixel_format (false),
    m_send_set_encodings (false),
    m_send_framebuffer_update_request (false),
    m_recv_ignore (false),
    m_recv_protocol_version (true),
    m_recv_security_types (false),
    m_recv_security_type (false),
    m_recv_security_result (false),
    m_recv_server_init (false),
    m_recv_framebuffer_update (false),
    m_recv_unknown_message (false),
    HIGHEST_VERSION (rfb::PROTOCOL_VERSION_3_3),
    m_incremental (false), // Request entire screen first time.
    m_state (SCHEDULE_READ_READY)
  {
    m_encodings.push_back (rfb::RAW);
    m_encodings.push_back (rfb::COPY_RECT);
    m_encodings.push_back (rfb::RRE);
    m_encodings.push_back (rfb::HEXTILE);
    m_encodings.push_back (rfb::ZRLE);

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
    if (send_protocol_version_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_protocol_version);
    }
    if (send_security_type_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_security_type);
    }
    if (send_client_init_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_client_init);
    }
    if (send_set_pixel_format_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_set_pixel_format);
    }
    if (send_set_encodings_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_set_encodings);
    }
    if (send_framebuffer_update_request_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_framebuffer_update_request);
    }
    if (send_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send);
    }
    if (recv_ignore_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_ignore);
    }
    if (recv_protocol_version_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_protocol_version);
    }
    if (recv_security_types_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_security_types);
    }
    if (recv_security_type_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_security_type);
    }
    if (recv_security_result_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_security_result);
    }
    if (recv_server_init_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_server_init);
    }
    if (recv_framebuffer_update_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_framebuffer_update);
    }
    if (recv_unknown_message_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_unknown_message);
    }

    bool some_recv = m_recv_protocol_version || m_recv_security_types || m_recv_security_type || m_recv_security_result || m_recv_server_init || m_recv_framebuffer_update || m_recv_unknown_message;

    assert ((m_recv_ignore && !some_recv) || (!m_recv_ignore) && some_recv);

    if (schedule_read_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::schedule_read);
    }
  }

private:
  // Send the ProtocolVersion message.
  bool send_protocol_version_precondition () const {
    return m_send_protocol_version;
  }

  void send_protocol_version_effect () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    m_protocol_version.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_protocol_version = false;
    m_recv_ignore = false;
    if (m_protocol_version >= rfb::PROTOCOL_VERSION_3_7) {
      m_recv_security_types = true;
    }
    else {
      m_recv_security_type = true;
    }
  }

  UP_INTERNAL (x_rfb_client_automaton, send_protocol_version);

  // Send the SecurityType message.
  bool send_security_type_precondition () const {
    return m_send_security_type;
  }

  void send_security_type_effect () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::security_type_t msg (m_security);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_security_type = false;
    m_recv_ignore = false;
    m_recv_security_result = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_security_type);

  // Send the ClientInit message.
  bool send_client_init_precondition () const {
    return m_send_client_init;
  }

  void send_client_init_effect () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::client_init_t msg (true);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_client_init = false;
    m_recv_ignore = false;
    m_recv_server_init = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_client_init);

  // Send the SetPixelFormat message.
  bool send_set_pixel_format_precondition () const {
    return m_send_set_pixel_format;
  }

  void send_set_pixel_format_effect () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::set_pixel_format_t msg (m_pixel_format);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_set_pixel_format = false;
    m_send_set_encodings = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_set_pixel_format);

  // Send the SetEncodings message.
  bool send_set_encodings_precondition () const {
    return m_send_set_encodings;
  }

  void send_set_encodings_effect () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::set_encodings_t msg (m_encodings);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_set_encodings = false;
    m_send_framebuffer_update_request = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_set_encodings);

  // Send the FramebufferUpdateRequest message.
  bool send_framebuffer_update_request_precondition () const {
    return m_send_framebuffer_update_request;
  }

  void send_framebuffer_update_request_effect () {
    std::cout << "client: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    // We want the whole image.
    rfb::framebuffer_update_request_t msg (m_incremental, 0, 0, m_server_width, m_server_height);
    // Can switch to incremental.
    m_incremental = true;
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_framebuffer_update_request = false;
    m_recv_ignore = false;
    m_recv_framebuffer_update = true;
    m_recv_unknown_message = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_framebuffer_update_request);

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
  // Ignore received bytes.
  bool recv_ignore_precondition () const {
    return m_recv_ignore && !m_buffer.empty ();
  }

  void recv_ignore_effect () {
    std::cerr << "client: ignoring " << m_buffer.size () << " bytes" << std::endl;
    m_buffer.clear ();
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_ignore);

  // Receive the ProtocolVersion message.
  bool recv_protocol_version_precondition () const {
    return m_recv_protocol_version && rfb::protocol_version_t::could_read_from_buffer (m_buffer);
  }

  void recv_protocol_version_effect () {
    std::cout << "client: " << __func__ << std::endl;
    rfb::protocol_version_t msg;
    msg.read_from_buffer (m_buffer);

    if ((msg == rfb::PROTOCOL_VERSION_3_3) ||
    	(msg == rfb::PROTOCOL_VERSION_3_7) ||
    	(msg == rfb::PROTOCOL_VERSION_3_8)) {
      // The server send us a version we can understand.
      // Use the lowest version mutually supported.
      m_protocol_version = std::min (msg, HIGHEST_VERSION);
    }
    else {
      // The version is either too high, too low, or garbage.
      // Use our own and let the server disconnect if not supported.
      m_protocol_version = HIGHEST_VERSION;
    }

    m_recv_protocol_version = false;
    m_recv_ignore = true;
    m_send_protocol_version = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_protocol_version);

  // Receive the SecurityTypes message.
  bool recv_security_types_precondition () const {
    return m_recv_security_types && rfb::security_types_t::could_read_from_buffer (m_buffer);
  }
  
  void recv_security_types_effect () {
    std::cout << "client: " << __func__ << std::endl;
    rfb::security_types_t msg;
    msg.read_from_buffer (m_buffer);

    m_recv_security_types = false;

    if (msg.security_types.size () == 0) {
      // Connection failed.
      // TODO
      assert (false);
    }
    else {
      m_security = rfb::INVALID;
      if (msg.security_types.count (rfb::NONE) != 0) {
	// Found an acceptable security type.
	m_security = rfb::NONE;
      }

      m_recv_ignore = true;
      m_send_security_type = true;
    }
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_security_types);
  
  // Receive the SecurityType message.
  bool recv_security_type_precondition () const {
    return m_recv_security_type && rfb::security_type_t::could_read_from_buffer (m_buffer);
  }

  void recv_security_type_effect () {
    std::cout << "client: " << __func__ << std::endl;
    rfb::security_type_t msg;
    msg.read_from_buffer (m_buffer);

    m_recv_security_type = false;

    if (msg.security == rfb::NONE) {
      m_recv_ignore = true;
      m_send_client_init = true;
    }
    else {
      // TODO
      assert (false);
    }
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_security_type);

  // Receive the SecurityResult message.
  bool recv_security_result_precondition () const {
    return m_recv_security_result && rfb::security_result_t::could_read_from_buffer (m_buffer);
  }
  
  void recv_security_result_effect () {
    std::cout << "client: " << __func__ << std::endl;
    rfb::security_result_t msg;
    msg.read_from_buffer (m_buffer);

    m_recv_security_result = false;

    if (msg.status == 0) {
      // Success.
      m_send_client_init = true;
      m_recv_ignore = true;
    }
    else {
      // Failure.
      // TODO
      assert (false);
    }
  }
  
  UP_INTERNAL (x_rfb_client_automaton, recv_security_result);

  // Receive the ServerInit message.
  bool recv_server_init_precondition () const {
    return m_recv_server_init && rfb::server_init_t::could_read_from_buffer (m_buffer);
  }

  void recv_server_init_effect () {
    std::cout << "client: " << __func__ << std::endl;
    rfb::server_init_t msg;
    msg.read_from_buffer (m_buffer);
    m_server_width = std::min (WIDTH, msg.framebuffer_width);
    m_server_height = std::min (HEIGHT, msg.framebuffer_height);
    // We are going to ignore the server pixel format and send our own.
    m_recv_server_init = false;
    m_recv_ignore = true;
    m_send_set_pixel_format = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_server_init);

  // Receive the FramebufferUpdate message.
  bool recv_framebuffer_update_precondition () const {
    return m_recv_framebuffer_update && rfb::framebuffer_update_t::could_read_from_buffer (m_buffer, m_pixel_format);
  }

  void recv_framebuffer_update_effect () {
    std::cout << "client: " << __func__ << std::endl;
    rfb::framebuffer_update_t msg;
    msg.read_from_buffer (m_buffer, m_pixel_format, m_data, WIDTH, HEIGHT);

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

    // Send a new request.
    m_send_framebuffer_update_request = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_framebuffer_update);

  // Receive an unknown message.
  bool recv_unknown_message_precondition () const {
    return false;
  }

  void recv_unknown_message_effect () {
    // TODO
    assert (false);
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_unknown_message);

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    if (val.get () != 0) {
      m_buffer.append (*val.get ());
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
