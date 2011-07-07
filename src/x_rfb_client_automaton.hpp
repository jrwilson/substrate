#ifndef __x_rfb_client_automaton_hpp__
#define __x_rfb_client_automaton_hpp__

#include <ioa/ioa.hpp>
#include <ioa/buffer.hpp>

#include <iostream>
#include <queue>

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <jpeglib.h>

#include "rfb.hpp"

class x_rfb_client_automaton :
  public ioa::automaton
{
private:
  std::queue<ioa::const_shared_ptr<ioa::buffer_interface> > m_sendq;
  ioa::buffer m_buffer;

  bool m_send_protocol_version;
  bool m_send_security_type;
  bool m_send_init;
  bool m_send_set_pixel_format;

  bool m_recv_protocol_version;
  bool m_recv_security_types;
  bool m_recv_security_type;
  bool m_recv_security_result;
  bool m_recv_init;

  const std::string m_supported_version;
  std::string m_version;
  rfb_security_t m_security_type;

  // enum state_t {
  //   SCHEDULE_READ_READY,
  //   READ_READY_WAIT,
  // };

  // struct rgb_t {
  //   uint8_t blue;
  //   uint8_t green;
  //   uint8_t red;
  //   uint8_t unused;
  // };

  // static const unsigned int X_OFFSET = 0;
  // static const unsigned int Y_OFFSET = 0;
  // // I assume a fixed size.  Generalize later.
  // static const unsigned int WIDTH = 352;
  // static const unsigned int HEIGHT = 288;
  // static const unsigned int BORDER_WIDTH = 0;

  // state_t m_state;
  // Display* m_display;
  // int m_fd;
  // int m_screen;
  // Window m_window;
  // Atom m_del_window;
  // rgb_t m_data[WIDTH * HEIGHT];
  // XImage* m_image;
  // ioa::const_shared_ptr<ioa::buffer> m_buffer;

public:
  x_rfb_client_automaton () :
    m_send_protocol_version (false),
    m_send_security_type (false),
    m_send_init (false),
    m_send_set_pixel_format (false),
    m_recv_protocol_version (true),
    m_recv_security_types (false),
    m_recv_security_type (false),
    m_recv_security_result (false),
    m_recv_init (false),
    m_supported_version (RFB_PROTOCOL_VERSION_3_8, RFB_PROTOCOL_VERSION_STRING_LENGTH),
    m_security_type (RFB_SECURITY_INVALID)
  //m_state (SCHEDULE_READ_READY)
  {
    // // Open connection with the X server.
    // m_display = XOpenDisplay (NULL);
    // if (m_display == NULL) {
    //   std::cerr << "Cannot open display" << std::endl;
    //   exit (EXIT_FAILURE);
    // }

    // // Get the connection file descriptor.
    // m_fd = ConnectionNumber (m_display);

    // // Get the flags.
    // int flags = fcntl (m_fd, F_GETFL, 0);
    // if (flags < 0) {
    //   perror ("fcntl");
    //   exit (EXIT_FAILURE);
    // }

    // // Set non-blocking.
    // flags |= O_NONBLOCK;
    // if (fcntl (m_fd, F_SETFL, flags) == -1) {
    //   perror ("fcntl");
    //   exit (EXIT_FAILURE);
    // }

    // m_screen = DefaultScreen (m_display);

    // // Create a window.
    // m_window = XCreateSimpleWindow (m_display,
    // 				    RootWindow (m_display, m_screen),
    // 				    X_OFFSET, Y_OFFSET, WIDTH, HEIGHT, BORDER_WIDTH,
    // 				    BlackPixel (m_display, m_screen),
    // 				    WhitePixel (m_display, m_screen));
  
    // // Prosses Window Close Event through event handler so XNextEvent does Not fail
    // m_del_window = XInternAtom (m_display, "WM_DELETE_WINDOW", 0);
    // XSetWMProtocols (m_display, m_window, &m_del_window, 1);
    
    // // Select events in which we are interested.
    // XSelectInput (m_display, m_window, StructureNotifyMask | ExposureMask);
  
    // // Show the window.
    // XMapWindow (m_display, m_window);
    // XFlush (m_display);
    
    // int depth = DefaultDepth (m_display, m_screen);
    // Visual* visual = DefaultVisual (m_display, m_screen);
    
    // // I assume these.  Generalize later.
    // assert (depth == 24);
    // assert (visual->red_mask   == 0x00FF0000);
    // assert (visual->green_mask == 0x0000FF00);
    // assert (visual->blue_mask  == 0x000000FF);
    
    // m_image = XCreateImage (m_display,
    // 			    CopyFromParent,
    // 			    depth,
    // 			    ZPixmap,
    // 			    0,
    // 			    reinterpret_cast<char*> (m_data),
    // 			    WIDTH,
    // 			    HEIGHT,
    // 			    32,
    // 			    0);
    
    // assert (1 == XInitImage (m_image));

    schedule ();
  }
  
  ~x_rfb_client_automaton () {
    // delete[] m_data;
  }

private:
  void schedule () const {
    if (send_protocol_version_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_protocol_version);
    }
    if (send_security_type_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_security_type);
    }
    if (send_init_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_init);
    }
    if (send_set_pixel_format_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send_set_pixel_format);
    }
    if (send_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::send);
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
    if (recv_init_precondition ()) {
      ioa::schedule (&x_rfb_client_automaton::recv_init);
    }
    // if (schedule_read_precondition ()) {
    //   ioa::schedule (&x_rfb_client_automaton::schedule_read);
    // }
  }

  // bool schedule_read_precondition () const {
  //   return m_state == SCHEDULE_READ_READY;
  // }

  // void schedule_read_effect () {
  //   ioa::schedule_read_ready (&x_rfb_client_automaton::read_ready, m_fd);
  //   m_state = READ_READY_WAIT;
  // }

  // UP_INTERNAL (x_rfb_client_automaton, schedule_read);

  // bool read_ready_precondition () const {
  //   return m_state == READ_READY_WAIT;
  // }

  // void read_ready_effect () {
  //   XEvent event;
    
  //   // Handle XEvents and flush the input.
  //   if (XPending (m_display)) {
  //     XNextEvent (m_display, &event);
  //     if (event.type == Expose) {
  // 	XPutImage(m_display, 
  // 		  m_window,
  // 		  DefaultGC (m_display, m_screen),
  // 		  m_image,
  // 		  0, 0,
  // 		  0, 0,
  // 		  WIDTH, HEIGHT);
  //     }
  //   }

  //   m_state = SCHEDULE_READ_READY;
  // }

  // UP_INTERNAL (x_rfb_client_automaton, read_ready);


  // static void init_source (struct jpeg_decompress_struct* cinfo) {
  //   // Initialized before so do nothing.
  // }
  
  // static boolean fill_input_buffer (struct jpeg_decompress_struct* cinfo) {
  //   assert (false);
  // }
  
  // static void skip_input_data (struct jpeg_decompress_struct* cinfo,
  // 			       long numbytes) {
  //   cinfo->src->next_input_byte += numbytes;
  //   cinfo->src->bytes_in_buffer -= numbytes;
  // }
  
  // static void term_source (struct jpeg_decompress_struct* cinfo) {
  //   /* Do nothing. */
  // }

private:
  // Send the protocol version to the server.
  bool send_protocol_version_precondition () const {
    return m_send_protocol_version;
  }

  void send_protocol_version_effect () {
    std::cout << "client: " << __func__ << std::endl;
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer> (new ioa::buffer (m_version.c_str (), m_version.size ())));
    m_send_protocol_version = false;

    if (m_version >= RFB_PROTOCOL_VERSION_3_7) {
      m_recv_security_types = true;
    }
    else {
      m_recv_security_type = true;
    }
  }

  UP_INTERNAL (x_rfb_client_automaton, send_protocol_version);

  // Send the security type.
  bool send_security_type_precondition () const {
    return m_send_security_type;
  }

  void send_security_type_effect () {
    std::cout << "client: " << __func__ << std::endl;
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (new ioa::buffer (&m_security_type, sizeof (uint8_t))));
    m_send_security_type = false;
    m_recv_security_result = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_security_type);

  // Send the init message.
  bool send_init_precondition () const {
    return m_send_init;
  }

  void send_init_effect () {
    std::cout << "client: " << __func__ << std::endl;
    uint8_t shared_flag = 1;
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (new ioa::buffer (&shared_flag, sizeof (uint8_t))));
    m_send_init = false;
    m_recv_init = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, send_init);

  // Send the SetPixelFormat message.
  bool send_set_pixel_format_precondition () const {
    return m_send_set_pixel_format;
  }

  void send_set_pixel_format_effect () {
    std::cout << "client: " << __func__ << std::endl;
    assert (false);
  }

  UP_INTERNAL (x_rfb_client_automaton, send_set_pixel_format);

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
  // Append received bytes to a buffer.
  void append (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    const size_t original_size = m_buffer.size ();
    m_buffer.resize (original_size + val->size ());
    memcpy (static_cast<char *> (m_buffer.data ()) + original_size, val->data (), val->size ());
  }

  // Remove processed bytes from a buffer.
  void consume (const size_t nbytes) {
    const size_t new_size = m_buffer.size () - nbytes;
    memmove (m_buffer.data (), static_cast<char *> (m_buffer.data ()) + nbytes, new_size);
    m_buffer.resize (new_size);
  }

  // Receive the protocol version message.
  bool recv_protocol_version_precondition () const {
    return m_recv_protocol_version && m_buffer.size () >= RFB_PROTOCOL_VERSION_STRING_LENGTH;
  }

  void recv_protocol_version_effect () {
    std::cout << "client: " << __func__ << std::endl;
    // Get the highest protocol version supported by the server.
    std::string server_version (static_cast<const char*> (m_buffer.data ()), RFB_PROTOCOL_VERSION_STRING_LENGTH);
    consume (RFB_PROTOCOL_VERSION_STRING_LENGTH);

    if ((server_version == RFB_PROTOCOL_VERSION_3_3) ||
	(server_version == RFB_PROTOCOL_VERSION_3_7) ||
	(server_version == RFB_PROTOCOL_VERSION_3_8)) {
      // The version we will use is the minimum of the server's and ours.
      m_version = std::min (server_version, m_supported_version);
      m_recv_protocol_version = false;
      m_send_protocol_version = true;
    }
    else {
      // TODO:  Server reported a weird version.
      assert (false);
    }
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_protocol_version);

  // Receive security types.
  bool recv_security_types_precondition () const {
    if (m_recv_security_types && m_buffer.size () >= 1) {
      // Get a pointer to the array describing the security types.
      const uint8_t* data = static_cast<const uint8_t*> (m_buffer.data ());
      const uint8_t security_type_count = *data;
      if (security_type_count + 1u <= m_buffer.size ()) {
   	// We can process the message.
	return true;
      }
    }
    return false;
  }

  void recv_security_types_effect () {
    std::cout << "client: " << __func__ << std::endl;
    uint8_t security_type_count = 0;
    {
      // Get a pointer to the array describing the security types.
      const uint8_t* data = static_cast<const uint8_t*> (m_buffer.data ());
      security_type_count = *data;
      ++data;

      for (uint8_t i = 0; i < security_type_count; ++i) {
	if (data[i] == RFB_SECURITY_NONE) {
	  m_security_type = static_cast<rfb_security_t> (data[i]);
	  break;
	}
      }
      
      // Consume.
      consume (security_type_count + 1);
    }

    if (security_type_count == 0) {
      // Server didn't like our protocol.
      // TODO
      assert (false);
    }
    else if (m_security_type != RFB_SECURITY_INVALID) {
      // Found an acceptable security type.
      m_recv_security_types = false;
      m_send_security_type = true;
    }
    else {
      // Didn't find an acceptable security type.
      // TODO
      assert (false);
    }
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_security_types);
  
  // Receive the security type.
  bool recv_security_type_precondition () const {
    // TODO: Check size.
    return m_recv_security_type;
  }

  void recv_security_type_effect () {
    std::cout << "client: " << __func__ << std::endl;
    assert (false);
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_security_type);

  // Receive the security result.
  bool recv_security_result_precondition () const {
    return m_recv_security_result && m_buffer.size () >= sizeof (uint32_t);
  }
  
  void recv_security_result_effect () {
    std::cout << "client: " << __func__ << std::endl;
    const uint32_t* res = static_cast<const uint32_t*> (m_buffer.data ());
    const uint32_t security_result = ntohl (*res);
    consume (sizeof (uint32_t));

    m_recv_security_result = false;

    if (security_result == 0) {
      // Success.
      m_send_init = true;
    }
    else {
      // Failure.
      // TODO
      assert (false);
    }
  }
  
  UP_INTERNAL (x_rfb_client_automaton, recv_security_result);

  // Receive the init message from the server.
  bool recv_init_precondition () const {
    return m_recv_init && m_buffer.size () >= sizeof (server_init_t);
  }

  void recv_init_effect () {
    std::cout << "client: " << __func__ << std::endl;

    server_init_t init;
    memcpy (&init, m_buffer.data (), sizeof (server_init_t));
    consume (sizeof (server_init_t));
    init.net_to_host ();
    consume (init.name_length);

    // We are going to ignore the server and send our own.
    m_recv_init = false;
    m_send_set_pixel_format = true;
  }

  UP_INTERNAL (x_rfb_client_automaton, recv_init);

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    if (val.get () != 0) {
      append (val);
    }

    // if (buf.get () != 0) {
    //   m_buffer = buf;
    //   struct jpeg_decompress_struct cinfo;
    //   struct jpeg_error_mgr jerr;
    //   struct jpeg_source_mgr source;

    //   source.next_input_byte = static_cast<const JOCTET*> (m_buffer->data ());
    //   source.bytes_in_buffer = m_buffer->size ();
    
    //   source.init_source = init_source;
    //   source.fill_input_buffer = fill_input_buffer;
    //   source.skip_input_data = skip_input_data;
    //   source.resync_to_restart = jpeg_resync_to_restart;
    //   source.term_source = term_source;
      
    //   cinfo.err = jpeg_std_error (&jerr);
    //   jpeg_create_decompress (&cinfo);
    //   cinfo.src = &source;
    //   jpeg_read_header (&cinfo, TRUE);

    //   jpeg_start_decompress (&cinfo);

    //   assert (cinfo.out_color_components == 3);

    //   assert (cinfo.output_width == WIDTH);
    //   assert (cinfo.output_height == HEIGHT);

    //   size_t row_stride = sizeof (JSAMPLE) * 3 * cinfo.output_width;
    //   JSAMPROW row_pointer[1];
    //   row_pointer[0] = new JSAMPLE[3 * cinfo.output_width * cinfo.rec_outbuf_height];
    //   while (cinfo.output_scanline < cinfo.output_height) {
    // 	JDIMENSION num_lines = jpeg_read_scanlines (&cinfo, row_pointer, cinfo.rec_outbuf_height);
    // 	JDIMENSION y = cinfo.output_scanline - 1;
    //    	for (JDIMENSION s = 0; s < num_lines; ++s, ++y) {
    //    	  JDIMENSION x;
    //    	  for (x = 0; x < cinfo.output_width; ++x) {
    // 	    m_data[y * cinfo.output_width + x].red = row_pointer[0][s * row_stride + 3 * x];
    // 	    m_data[y * cinfo.output_width + x].green = row_pointer[0][s * row_stride + 3 * x + 1];
    // 	    m_data[y * cinfo.output_width + x].blue = row_pointer[0][s * row_stride + 3 * x + 2];
    // 	  }
    //    	}
    //   }
    //   delete[] row_pointer[0];

    //   jpeg_finish_decompress (&cinfo);
      
    //   jpeg_destroy_decompress (&cinfo);

    //   XPutImage (m_display,
    // 		 m_window,
    // 		 DefaultGC (m_display, m_screen),
    // 		 m_image,
    // 		 0, 0,
    // 		 0, 0,
    // 		 WIDTH, HEIGHT);
    //   XFlush (m_display);
    // }
  }

public:
  V_UP_INPUT (x_rfb_client_automaton, receive, ioa::const_shared_ptr<ioa::buffer_interface>);
};

#endif
