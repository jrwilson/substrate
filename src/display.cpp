#include <substrate/rgram.hpp>
#include "rfb.hpp"

#include "x_rfb_client_automaton.hpp"
#include "channel_automaton.hpp"

#include <ioa/global_fifo_scheduler.hpp>

// TODO:  Clean-up this file.

class rfb_server_automaton :
  public ioa::automaton
{
private:

  struct recv_protocol_version_gramel :
    public rgram::gramel
  {
    rfb_server_automaton& m_server;
    rfb::protocol_version_gramel m_protocol_version;
    
    recv_protocol_version_gramel (rfb_server_automaton& server) :
      m_server (server)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_protocol_version.put (buf);
      if (done ()) {
	m_server.recv_protocol_version (m_protocol_version.get ());
      }
    }

    bool done () const {
      return m_protocol_version.done ();
    }

    void reset () {
      m_protocol_version.reset ();
    }
  };

  struct recv_client_init_gramel :
    public rgram::gramel
  {
    rfb_server_automaton& m_server;
    rfb::client_init_gramel m_init;
    
    recv_client_init_gramel (rfb_server_automaton& server) :
      m_server (server)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_init.put (buf);
      if (done ()) {
	m_server.recv_client_init (m_init.get ());
      }
    }

    bool done () const {
      return m_init.done ();
    }

    void reset () {
      m_init.reset ();
    }
  };

  struct recv_client_message_gramel :
    public rgram::gramel
  {
    rfb_server_automaton& m_server;
    rfb::client_message_gramel m_message;
    
    recv_client_message_gramel (rfb_server_automaton& server) :
      m_server (server)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());

      m_message.put (buf);
      if (done ()) {
	switch (m_message.m_choice.get ()) {
	case rfb::SET_PIXEL_FORMAT_TYPE:
	  m_server.recv_set_pixel_format (m_message.m_set_pixel_format.get ());
	  break;
	case rfb::SET_ENCODINGS_TYPE:
	  m_server.recv_set_encodings (m_message.m_set_encodings.get ());
	  break;
	case rfb::FRAMEBUFFER_UPDATE_REQUEST_TYPE:
	  m_server.recv_framebuffer_update_request (m_message.m_framebuffer_update_request.get ());
	  break;
	default:
	  std::cerr << "Unknown client message.  Type = " << int (m_message.m_choice.get ()) << std::endl;
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
  };

  struct protocol_gramel :
    public rgram::gramel
  {
    recv_protocol_version_gramel m_recv_protocol;
    recv_client_init_gramel m_recv_init;
    recv_client_message_gramel m_recv_client;
    rgram::sequence_gramel m_sequence;

    protocol_gramel (rfb_server_automaton& server) :
      m_recv_protocol (server),
      m_recv_init (server),
      m_recv_client (server)
    {
      m_sequence.append (&m_recv_protocol);
      m_sequence.append (&m_recv_init);
      m_sequence.append (&m_recv_client);
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
  };

  protocol_gramel m_protocol;

  struct rgb_t
  {
    union {
      uint32_t val;
      struct {
  	uint8_t blue;
  	uint8_t green;
  	uint8_t red;
  	uint8_t unused;
      } little_endian;
      struct {
  	uint8_t unused;
  	uint8_t red;
  	uint8_t green;
  	uint8_t blue;
      } big_endian;
    };
  };

  struct raw_pixel_data_t :
    public rfb::pixel_data_t
  {
    rfb_server_automaton& m_server;
    const uint16_t x_position;
    const uint16_t y_position;
    const uint16_t width;
    const uint16_t height;

    raw_pixel_data_t (rfb_server_automaton& server,
		      const uint16_t x_pos,
		      const uint16_t y_pos,
		      const uint16_t w,
		      const uint16_t h) :
      m_server (server),
      x_position (x_pos),
      y_position (y_pos),
      width (w),
      height (h)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      int32_t x = htonl (rfb::RAW);
      buf.append (&x, sizeof (x));
      for (uint16_t y = 0; y < height; ++y) {
	for (uint16_t x = 0; x < width; ++x) {
	  const uint16_t x_pos = x_position + x;
	  const uint16_t y_pos = y_position + y;
	  buf.append (&m_server.m_data[y_pos * m_server.WIDTH + x_pos].val, sizeof (rgb_t));
	}
      }
    }
  };
  
  std::queue<ioa::const_shared_ptr<ioa::buffer_interface> > m_sendq;

  const rfb::protocol_version_t HIGHEST_VERSION;
  rfb::protocol_version_t m_protocol_version;
  const rfb::pixel_format_t PIXEL_FORMAT;
  static const uint16_t WIDTH = 240;
  static const uint16_t HEIGHT = 160;
  const rfb::server_init_t SERVER_INIT;
  rfb::pixel_format_t m_client_format;
  std::set<int32_t> m_supported_encodings;
  std::vector<int32_t> m_client_encodings;
  rgb_t m_data[WIDTH * HEIGHT];
  bool m_image_changed;
  bool m_outstanding_request;
  uint16_t m_request_x0;
  uint16_t m_request_y0;
  uint16_t m_request_x1;
  uint16_t m_request_y1;

public:
  rfb_server_automaton () :
    m_protocol (*this),
    HIGHEST_VERSION (rfb::PROTOCOL_VERSION_3_3),
    PIXEL_FORMAT (32, 24, ntohl (1) == 1, true, 255, 255, 255, 16, 8, 0),
    SERVER_INIT (WIDTH, HEIGHT, PIXEL_FORMAT, "This is an RFB server."),
    m_client_format (PIXEL_FORMAT),
    m_image_changed (false),
    m_outstanding_request (false)
  {
    std::cout << "server: big_endian = " << int (PIXEL_FORMAT.big_endian_flag) << std::endl;

    m_supported_encodings.insert (rfb::RAW);

    rgb_t color;
    const uint8_t red = 0x00; //0x12;
    const uint8_t green = 0x00; //0x34;
    const uint8_t blue = 0x00; //0x56;
    if (PIXEL_FORMAT.big_endian_flag) {
      color.big_endian.red = red;
      color.big_endian.green = green;
      color.big_endian.blue = blue;
    }
    else {
      color.little_endian.red = red;
      color.little_endian.green = green;
      color.little_endian.blue = blue;
    }

    for (uint16_t y = 0; y < HEIGHT; ++y) {
      for (uint16_t x = 0; x < WIDTH; ++x) {
	m_data[y * WIDTH + x] = color;
      }
    }

    send_protocol_version ();
    
    schedule ();
  }

private:
  void schedule () const {
    if (send_framebuffer_update_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_framebuffer_update);
    }
    if (send_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send);
    }
    if (update_image_precondition ()) {
      ioa::schedule (&rfb_server_automaton::update_image);
    }
  }

  void send_protocol_version () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    HIGHEST_VERSION.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void recv_protocol_version (const rfb::protocol_version_t& version) {
    std::cout << "server: " << __func__ << std::endl;
    
    m_protocol_version = version;

    if (m_protocol_version != rfb::PROTOCOL_VERSION_3_3) {
      // TODO:  Support other protocols.
      assert (false);
    }
    else {
      // The client sent us a version we don't understand.
      // Ignore them.
      m_protocol_version = HIGHEST_VERSION;
    }

    // The client wants to use a protocol we understand.
    std::string s (m_protocol_version.version, rfb::PROTOCOL_VERSION_STRING_LENGTH - 1);
    std::cout << "server: protocol version = " << s << std::endl;

    send_security_type ();
  }

  void send_security_type () {
    std::cout << "server: " << __func__ << std::endl;

    ioa::buffer* buf = new ioa::buffer ();
    // We only support no security.
    rfb::security_type_t msg (rfb::NONE);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void recv_client_init (const rfb::client_init_t& init) {
    std::cout << "server: " << __func__ << std::endl;
    // We explicity ignore the share flag.
    
    send_server_init ();
  }

  void send_server_init () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    SERVER_INIT.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
  }

  void recv_set_pixel_format (const rfb::set_pixel_format_t& msg) {
    std::cout << "server: " << __func__ << std::endl;
    m_client_format = msg.pixel_format;
    // TODO:  Relax these assumptions.
    assert (m_client_format.bits_per_pixel == PIXEL_FORMAT.bits_per_pixel);
    assert (m_client_format.depth == PIXEL_FORMAT.depth);
    assert (m_client_format.big_endian_flag == PIXEL_FORMAT.big_endian_flag);
    assert (m_client_format.true_colour_flag == PIXEL_FORMAT.true_colour_flag);
    assert (m_client_format.red_max == PIXEL_FORMAT.red_max);
    assert (m_client_format.blue_max == PIXEL_FORMAT.blue_max);
    assert (m_client_format.green_max == PIXEL_FORMAT.green_max);
    assert (m_client_format.red_shift == PIXEL_FORMAT.red_shift);
    assert (m_client_format.green_shift == PIXEL_FORMAT.green_shift);
    assert (m_client_format.blue_shift == PIXEL_FORMAT.blue_shift);
  }

  void recv_set_encodings (const rfb::set_encodings_t& msg) {
    std::cout << "server: " << __func__ << std::endl;
    // Make a copy.
    std::vector<int32_t> encodings = msg.encodings;
    // Everyone must support RAW.
    encodings.push_back (rfb::RAW);

    std::set<int32_t> s;

    m_client_encodings.clear ();
    for (std::vector<int32_t>::const_iterator pos = encodings.begin ();
	 pos != encodings.end ();
	 ++pos) {
      if (m_supported_encodings.count (*pos) != 0 && s.count (*pos) == 0) {
	m_client_encodings.push_back (*pos);
	s.insert (*pos);
      }
    }
  }

  void recv_framebuffer_update_request (const rfb::framebuffer_update_request_t& request) {
    std::cout << "server: " << __func__ << std::endl;

    if (request.x_position < WIDTH &&
			     request.y_position < HEIGHT &&
						  request.width > 0 &&
			     request.height > 0) {
      // Request will produce data.

      // Correct out-of-bounds width and height.
      const uint16_t new_width = std::min (WIDTH, uint16_t (request.x_position + request.width)) - request.x_position;
      const uint16_t new_height = std::min (HEIGHT, uint16_t (request.y_position + request.height)) - request.y_position;

      if (!m_outstanding_request) {
  	// If there is not outstanding request, send the bounds.
  	m_request_x0 = request.x_position;
  	m_request_y0 = request.y_position;
  	m_request_x1 = request.x_position + new_width;
  	m_request_y1 = request.y_position + new_height;
  	m_outstanding_request = true;
      }
      else {
  	// Expand the bounds.
  	m_request_x0 = std::min (m_request_x0, request.x_position);
  	m_request_y0 = std::min (m_request_y0, request.y_position);
  	m_request_x1 = std::max (m_request_x1, uint16_t (request.x_position + new_width));
  	m_request_y1 = std::max (m_request_y1, uint16_t (request.y_position + new_height));
      }

      std::cout << "New bounds (" << m_request_x0 << "," << m_request_y0 << ") -> (" << m_request_x1 << "," << m_request_y1 << ")" << std::endl;

      if (!request.incremental) {
  	// Fake an image change to force an immediate FramebufferUpdate.
  	m_image_changed = true;
      }
    }
  }

  // Send the FramebufferUpdate message.
  bool send_framebuffer_update_precondition () const {
    return m_outstanding_request && m_image_changed;
  }

  void send_framebuffer_update_effect () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::framebuffer_update_t update;
    update.add_rectangle (rfb::rectangle_t (m_request_x0,
					    m_request_y0,
					    m_request_x1 - m_request_x0,
					    m_request_y1 - m_request_y0,
					    new raw_pixel_data_t (*this,
								  m_request_x0,
								  m_request_y0,
								  m_request_x1 - m_request_x0,
								  m_request_y1 - m_request_y0)));
    update.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    
    m_outstanding_request = false;
    m_image_changed = false;
  }

  UP_INTERNAL (rfb_server_automaton, send_framebuffer_update);

  // Send a message.
  bool send_precondition () const {
    return !m_sendq.empty () && ioa::binding_count (&rfb_server_automaton::send) != 0;
  }

  ioa::const_shared_ptr<ioa::buffer_interface> send_effect () {
    ioa::const_shared_ptr<ioa::buffer_interface> retval = m_sendq.front ();
    m_sendq.pop ();
    return retval;
  }

public:
  V_UP_OUTPUT (rfb_server_automaton, send, ioa::const_shared_ptr<ioa::buffer_interface>);
  
private:

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    if (val.get () != 0) {
      rgram::buffer rbuf (*val.get ());
      m_protocol.put (rbuf);
    }
  }

public:
  V_UP_INPUT (rfb_server_automaton, receive, ioa::const_shared_ptr<ioa::buffer_interface>);

private:
  bool update_image_precondition () const {
    return true;
  }

  void update_image_effect () {
    for (uint16_t y = 0; y < HEIGHT; ++y) {
      for (uint16_t x = 0; x < WIDTH; ++x) {
	m_data[y * WIDTH + x].val = rand ();
      }
    }
    m_image_changed = true;
  }

  UP_INTERNAL (rfb_server_automaton, update_image);

};

const uint16_t rfb_server_automaton::HEIGHT;
const uint16_t rfb_server_automaton::WIDTH;

class display_driver_automaton :
  public ioa::automaton
{
public:
  display_driver_automaton () {
    ioa::automaton_manager<rfb_server_automaton>* server = new ioa::automaton_manager<rfb_server_automaton> (this, ioa::make_generator<rfb_server_automaton> ());
    ioa::automaton_manager<x_rfb_client_automaton>* client = new ioa::automaton_manager<x_rfb_client_automaton> (this, ioa::make_generator<x_rfb_client_automaton> ());

    ioa::automaton_manager<channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> > >* server_to_client = new ioa::automaton_manager<channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> > > (this, ioa::make_generator<channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> > > ());

    ioa::automaton_manager<channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> > >* client_to_server = new ioa::automaton_manager<channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> > > (this, ioa::make_generator<channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> > > ());

    ioa::make_binding_manager (this,
     			       server, &rfb_server_automaton::send,
     			       server_to_client, &channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> >::send);

    ioa::make_binding_manager (this,
     			       server_to_client, &channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> >::receive,
     			       client, &x_rfb_client_automaton::receive);

    ioa::make_binding_manager (this,
     			       client, &x_rfb_client_automaton::send,
     			       client_to_server, &channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> >::send);

    ioa::make_binding_manager (this,
     			       client_to_server, &channel_automaton<ioa::const_shared_ptr<ioa::buffer_interface> >::receive,
     			       server, &rfb_server_automaton::receive);
  }
};

int main (int argc, char* argv[]) {
  ioa::global_fifo_scheduler sched;
  ioa::run (sched, ioa::make_generator<display_driver_automaton> ());
  return 0;
}
