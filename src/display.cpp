#include "rfb.hpp"

#include "x_rfb_client_automaton.hpp"
#include "channel_automaton.hpp"

#include <ioa/global_fifo_scheduler.hpp>


class rfb_server_automaton :
  public ioa::automaton
{
private:
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
  
  std::queue<ioa::const_shared_ptr<ioa::buffer_interface> > m_sendq;
  ioa::buffer m_buffer;

  bool m_send_protocol_version;
  bool m_send_security_types;
  bool m_send_security_type;
  bool m_send_security_result;
  bool m_send_server_init;

  bool m_recv_ignore;
  bool m_recv_protocol_version;
  bool m_recv_security_type;
  bool m_recv_client_init;
  bool m_recv_set_pixel_format;
  bool m_recv_set_encodings;
  bool m_recv_framebuffer_update_request;
  bool m_recv_unknown_message;

  const rfb::protocol_version_t HIGHEST_VERSION;
  rfb::protocol_version_t m_protocol_version;
  bool m_connect_error;
  std::set<rfb::security_t> m_security_types;
  rfb::security_t m_security;
  const rfb::pixel_format_t PIXEL_FORMAT;
  static const uint16_t WIDTH = 240;
  static const uint16_t HEIGHT = 160;
  const rfb::server_init_t SERVER_INIT;
  rfb::pixel_format_t m_client_format;
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
    m_send_protocol_version (true),
    m_send_security_types (false),
    m_send_security_type (false),
    m_send_security_result (false),
    m_send_server_init (false),
    m_recv_ignore (true),
    m_recv_protocol_version (false),
    m_recv_security_type (false),
    m_recv_client_init (false),
    m_recv_set_pixel_format (false),
    m_recv_set_encodings (false),
    m_recv_framebuffer_update_request (false),
    m_recv_unknown_message (false),
    HIGHEST_VERSION (rfb::PROTOCOL_VERSION_3_3),
    m_connect_error (false),
    PIXEL_FORMAT (32, 24, ntohl (1) == 1, true, 255, 255, 255, 16, 8, 0),
    SERVER_INIT (WIDTH, HEIGHT, PIXEL_FORMAT, ""),
    m_client_format (PIXEL_FORMAT),
    m_image_changed (false),
    m_outstanding_request (false)
  {
    m_security_types.insert (rfb::NONE);
    std::cout << "server: big_endian = " << int (PIXEL_FORMAT.big_endian_flag) << std::endl;

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
    
    schedule ();
  }

private:
  void schedule () const {
    if (send_protocol_version_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_protocol_version);
    }
    if (send_security_types_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_security_types);
    }
    if (send_security_type_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_security_type);
    }
    if (send_security_result_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_security_result);
    }
    if (send_server_init_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_server_init);
    }
    if (send_framebuffer_update_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_framebuffer_update);
    }
    if (send_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send);
    }
    if (recv_ignore_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_ignore);
    }
    if (recv_protocol_version_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_protocol_version);
    }
    if (recv_security_type_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_security_type);
    }
    if (recv_client_init_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_client_init);
    }
    if (recv_set_pixel_format_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_set_pixel_format);
    }
    if (recv_set_encodings_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_set_encodings);
    }
    if (recv_framebuffer_update_request_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_framebuffer_update_request);
    }
    if (recv_unknown_message_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_unknown_message);
    }

    if (update_image_precondition ()) {
      ioa::schedule (&rfb_server_automaton::update_image);
    }

    bool some_recv = m_recv_protocol_version || m_recv_security_type || m_recv_client_init || m_recv_set_pixel_format || m_recv_set_encodings || m_recv_framebuffer_update_request || m_recv_unknown_message;
    assert ((m_recv_ignore && !some_recv) || (!m_recv_ignore && some_recv));
  }

  // Send the ProtocolVersion message.
  bool send_protocol_version_precondition () const {
    return m_send_protocol_version;
  }

  void send_protocol_version_effect () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    HIGHEST_VERSION.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_protocol_version = false;
    m_recv_ignore = false;
    m_recv_protocol_version = true;
  }

  UP_INTERNAL (rfb_server_automaton, send_protocol_version);

  // Send the SecurityTypes message.
  bool send_security_types_precondition () const {
    return m_send_security_types;
  }

  void send_security_types_effect () {
    std::cout << "server: " << __func__ << std::endl;
    if (!m_connect_error) {
      ioa::buffer* buf = new ioa::buffer ();
      rfb::security_types_t msg (m_security_types);
      msg.write_to_buffer (*buf);
      m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
      m_send_security_types = false;
      m_recv_ignore = false;
      m_recv_security_type = true;
    }
    else {
      // TODO
      assert (false);
    }
  }

  UP_INTERNAL (rfb_server_automaton, send_security_types);

  // Send the SecurityType message.
  bool send_security_type_precondition () const {
    return m_send_security_type;
  }

  void send_security_type_effect () {
    std::cout << "server: " << __func__ << std::endl;

    m_send_security_type = false;

    if (!m_connect_error) {
      ioa::buffer* buf = new ioa::buffer ();
      // We only support no security.
      m_security = rfb::NONE;
      rfb::security_type_t msg (m_security);
      msg.write_to_buffer (*buf);
      m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
      m_recv_ignore = false;
      m_recv_client_init = true;
    }
    else {
      // TODO
      assert (false);
    }
  }

  UP_INTERNAL (rfb_server_automaton, send_security_type);

  // Send the SecurityResult message.
  bool send_security_result_precondition () const {
    return m_send_security_result;
  }

  void send_security_result_effect () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    // We only support no security.
    rfb::security_result_t msg (m_security == rfb::NONE);
    msg.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));

    if (m_security != rfb::NONE && m_protocol_version >= rfb::PROTOCOL_VERSION_3_8) {
      // Send error message.
      // TODO
      assert (false);
    }

    m_send_security_result = false;
    m_recv_ignore = false;
    m_recv_client_init = true;
  }

  UP_INTERNAL (rfb_server_automaton, send_security_result);

  // Send the ServerInit message.
  bool send_server_init_precondition () const {
    return m_send_server_init;
  }

  void send_server_init_effect () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    SERVER_INIT.write_to_buffer (*buf);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));

    m_send_server_init = false;
    m_recv_ignore = false;
    m_recv_set_pixel_format = true;
    m_recv_set_encodings = true;
    m_recv_framebuffer_update_request = true;
    m_recv_unknown_message = true;
  }

  UP_INTERNAL (rfb_server_automaton, send_server_init);

  // Send the FramebufferUpdate message.
  bool send_framebuffer_update_precondition () const {
    return m_outstanding_request && m_image_changed;
  }

  void send_framebuffer_update_effect () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer ();
    rfb::framebuffer_update_t update;
    update.add_rectangle (new rfb::raw_rectangle_t (m_request_x0,
						    m_request_y0,
						    m_request_x1 - m_request_x0,
						    m_request_y1 - m_request_y0));
    update.write_to_buffer (*buf,
			    PIXEL_FORMAT,
			    m_client_format,
			    m_data,
			    WIDTH,
			    HEIGHT);
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
  // Ignore received bytes.
  bool recv_ignore_precondition () const {
    return m_recv_ignore && !m_buffer.empty ();
  }

  void recv_ignore_effect () {
    std::cerr << "server: ignoring " << m_buffer.size () << " bytes" << std::endl;
    m_buffer.clear ();
  }

  UP_INTERNAL (rfb_server_automaton, recv_ignore);

  // Receive the ProtocolVersion message.
  bool recv_protocol_version_precondition () const {
    return m_recv_protocol_version && rfb::protocol_version_t::could_read_from_buffer (m_buffer);
  }

  void recv_protocol_version_effect () {
    std::cout << "server: " << __func__ << std::endl;
    m_protocol_version.read_from_buffer (m_buffer);

    m_recv_protocol_version = false;

    if (!((m_protocol_version == rfb::PROTOCOL_VERSION_3_3) ||
	  (m_protocol_version == rfb::PROTOCOL_VERSION_3_7) ||
	  (m_protocol_version == rfb::PROTOCOL_VERSION_3_8))) {
      // The client sent us a version we don't understand.
      // Ignore them.
      m_protocol_version = HIGHEST_VERSION;
      m_connect_error = true;
    }

    // The client wants to use a protocol we understand.
    std::string s (m_protocol_version.version, rfb::PROTOCOL_VERSION_STRING_LENGTH - 1);
    std::cout << "server: protocol version = " << s << std::endl;;

    if (m_protocol_version >= rfb::PROTOCOL_VERSION_3_7) {
      m_send_security_types = true;
      m_recv_ignore = true;
    }
    else {
      m_send_security_type = true;
      m_recv_ignore = true;
    }
  }

  UP_INTERNAL (rfb_server_automaton, recv_protocol_version);

  // Receive the SecurityType message.
  bool recv_security_type_precondition () const {
    return m_recv_security_type && rfb::security_type_t::could_read_from_buffer (m_buffer);
  }

  void recv_security_type_effect () {
    std::cout << "server: " << __func__ << std::endl;
    rfb::security_type_t msg;
    msg.read_from_buffer (m_buffer);
    m_recv_security_type = false;

    m_security = msg.security;

    m_send_security_result = true;
    m_recv_ignore = true;
  }

  UP_INTERNAL (rfb_server_automaton, recv_security_type);

  // Receive the ClientInit message.
  bool recv_client_init_precondition () const {
    return m_recv_client_init && rfb::client_init_t::could_read_from_buffer (m_buffer);
  }

  void recv_client_init_effect () {
    std::cout << "server: " << __func__ << std::endl;
    rfb::client_init_t msg;
    msg.read_from_buffer (m_buffer);
    // We explicity ignore the share flag.
    m_recv_client_init = false;
    m_recv_ignore = true;
    m_send_server_init = true;
  }

  UP_INTERNAL (rfb_server_automaton, recv_client_init);

  // Receive a SetPixelFormat message.
  bool recv_set_pixel_format_precondition () const {
    return m_recv_set_pixel_format && rfb::set_pixel_format_t::could_read_from_buffer (m_buffer);
  }

  void recv_set_pixel_format_effect () {
    std::cout << "server: " << __func__ << std::endl;
    rfb::set_pixel_format_t msg;
    msg.read_from_buffer (m_buffer);
    m_client_format = msg.pixel_format;
  }

  UP_INTERNAL (rfb_server_automaton, recv_set_pixel_format);

  // Receive a SetEncodings message.
  bool recv_set_encodings_precondition () const {
    return m_recv_set_encodings && rfb::set_encodings_t::could_read_from_buffer (m_buffer);
  }

  void recv_set_encodings_effect () {
    std::cout << "server: " << __func__ << std::endl;
    rfb::set_encodings_t msg;
    msg.read_from_buffer (m_buffer);
    m_client_encodings = msg.encodings;
  }

  UP_INTERNAL (rfb_server_automaton, recv_set_encodings);

  // Receive a FramebufferUpdateRequest message.
  bool recv_framebuffer_update_request_precondition () const {
    return m_recv_framebuffer_update_request && rfb::framebuffer_update_request_t::could_read_from_buffer (m_buffer);
  }

  void recv_framebuffer_update_request_effect () {
    std::cout << "server: " << __func__ << std::endl;
    rfb::framebuffer_update_request_t request;
    request.read_from_buffer (m_buffer);

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

  UP_INTERNAL (rfb_server_automaton, recv_framebuffer_update_request);

  // Receive an unknown message.
  bool recv_unknown_message_precondition () const {
    return false;
  }

  void recv_unknown_message_effect () {
    // TODO
    assert (false);
  }

  UP_INTERNAL (rfb_server_automaton, recv_unknown_message);

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    if (val.get () != 0) {
      m_buffer.append (*val.get ());
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
