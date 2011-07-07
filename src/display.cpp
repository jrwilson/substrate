#include <ioa/ioa.hpp>
#include <ioa/global_fifo_scheduler.hpp>

#include "x_rfb_client_automaton.hpp"
#include "channel_automaton.hpp"

#include "rfb.hpp"
#include <arpa/inet.h>

class rfb_server_automaton :
  public ioa::automaton
{
private:
  std::queue<ioa::const_shared_ptr<ioa::buffer_interface> > m_sendq;
  ioa::buffer m_buffer;

  bool m_send_protocol_version;
  bool m_send_security_types;
  bool m_send_security_type;
  bool m_send_security_result;
  bool m_send_init;

  bool m_recv_protocol_version;
  bool m_recv_security_type;
  bool m_recv_init;

  const std::string m_supported_version;
  std::string m_version;
  rfb_security_t m_security_type;
  uint32_t m_security_result;

  static const uint16_t WIDTH = 640;
  static const uint16_t HEIGHT = 480;
  static const uint8_t BITS_PER_PIXEL = 32;
  static const uint8_t DEPTH = 24;
  const uint8_t BIG_ENDIAN_FLAG;
  static const uint8_t TRUE_COLOUR_FLAG = 1;
  static const uint16_t RED_MAX = 255;
  static const uint16_t GREEN_MAX = 255;
  static const uint16_t BLUE_MAX = 255;
  static const uint8_t RED_SHIFT = 0;
  static const uint8_t GREEN_SHIFT = 0;
  static const uint8_t BLUE_SHIFT = 0;
  const std::string m_name;

public:
  rfb_server_automaton () :
    m_send_protocol_version (true),
    m_send_security_types (false),
    m_send_security_type (false),
    m_send_security_result (false),
    m_send_init (false),
    m_recv_protocol_version (false),
    m_recv_security_type (false),
    m_recv_init (false),
    m_supported_version (RFB_PROTOCOL_VERSION_3_8, RFB_PROTOCOL_VERSION_STRING_LENGTH),
    m_security_type (RFB_SECURITY_INVALID),
    BIG_ENDIAN_FLAG (ntohl (1) == 1)
  {
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
    if (send_init_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send_init);
    }
    if (send_precondition ()) {
      ioa::schedule (&rfb_server_automaton::send);
    }
    if (recv_protocol_version_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_protocol_version);
    }
    if (recv_security_type_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_security_type);
    }
    if (recv_init_precondition ()) {
      ioa::schedule (&rfb_server_automaton::recv_init);
    }
  }

  // Send the protocol version to the client.
  bool send_protocol_version_precondition () const {
    return m_send_protocol_version;
  }

  void send_protocol_version_effect () {
    std::cout << "server: " << __func__ << std::endl;
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer> (new ioa::buffer (m_supported_version.c_str (), m_supported_version.size ())));
    m_send_protocol_version = false;
    m_recv_protocol_version = true;
  }

  UP_INTERNAL (rfb_server_automaton, send_protocol_version);

  // Send the security types.
  bool send_security_types_precondition () const {
    return m_send_security_types;
  }

  void send_security_types_effect () {
    std::cout << "server: " << __func__ << std::endl;
    ioa::buffer* buf = new ioa::buffer (2);
    uint8_t* data = static_cast<uint8_t*> (buf->data ());
    // Number of security types comes first.
    data[0] = 1;
    // Then the types.
    data[1] = RFB_SECURITY_NONE;
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));
    m_send_security_types = false;
    m_recv_security_type = true;
  }

  UP_INTERNAL (rfb_server_automaton, send_security_types);

  // Send the security type.
  bool send_security_type_precondition () const {
    return m_send_security_type;
  }

  void send_security_type_effect () {
    // TODO
    assert (false);
  }

  UP_INTERNAL (rfb_server_automaton, send_security_type);

  // Send the security result.
  bool send_security_result_precondition () const {
    return m_send_security_result;
  }

  void send_security_result_effect () {
    std::cout << "server: " << __func__ << std::endl;
    uint32_t res = htonl (m_security_result);
    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (new ioa::buffer (&res, sizeof (uint32_t))));
    if (m_security_result == 1 && m_version >= RFB_PROTOCOL_VERSION_3_8) {
      // Send error message.
      // TODO
      assert (false);
    }

    m_send_security_result = false;
    m_recv_init = true;
  }

  UP_INTERNAL (rfb_server_automaton, send_security_result);

  // Send the init message.
  bool send_init_precondition () const {
    return m_send_init;
  }

  void send_init_effect () {
    std::cout << "server: " << __func__ << std::endl;

    server_init_t init;
    init.framebuffer_width = WIDTH;
    init.framebuffer_height = HEIGHT;
    init.server_pixel_format.bits_per_pixel = BITS_PER_PIXEL;
    init.server_pixel_format.depth = DEPTH;
    init.server_pixel_format.big_endian_flag = BIG_ENDIAN_FLAG;
    init.server_pixel_format.true_colour_flag = TRUE_COLOUR_FLAG;
    init.server_pixel_format.red_max = RED_MAX;
    init.server_pixel_format.green_max = GREEN_MAX;
    init.server_pixel_format.blue_max = BLUE_MAX;
    init.server_pixel_format.red_shift = RED_SHIFT;
    init.server_pixel_format.green_shift = GREEN_SHIFT;
    init.server_pixel_format.blue_shift = BLUE_SHIFT;
    init.name_length = m_name.size ();

    init.host_to_net ();

    ioa::buffer* buf = new ioa::buffer (sizeof (server_init_t) + m_name.size ());
    memcpy (buf->data (), &init, sizeof (server_init_t));
    memcpy (static_cast<char *> (buf->data ()) + sizeof (server_init_t), m_name.c_str (), m_name.size ());

    m_sendq.push (ioa::const_shared_ptr<ioa::buffer_interface> (buf));

    m_send_init = false;
  }

  UP_INTERNAL (rfb_server_automaton, send_init);

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
    std::cout << "server: " << __func__ << std::endl;
    // Get the protocol version returned by the client.
    std::string client_version (static_cast<const char*> (m_buffer.data ()), RFB_PROTOCOL_VERSION_STRING_LENGTH);
    consume (RFB_PROTOCOL_VERSION_STRING_LENGTH);
    
    if (((client_version == RFB_PROTOCOL_VERSION_3_3) ||
	 (client_version == RFB_PROTOCOL_VERSION_3_7) ||
	 (client_version == RFB_PROTOCOL_VERSION_3_8)) &&
	client_version <= m_supported_version) {
      // Use the version returned by the client.
      m_version = client_version;
      std::cout << "server: protocol version = " << m_version;
      m_recv_protocol_version = false;

      if (m_version >= RFB_PROTOCOL_VERSION_3_7) {
	m_send_security_types = true;
      }
      else {
	m_send_security_type = true;
      }
    }
    else {
      // TODO:  Client requested an unsupported protocol version.
      assert (false);
    }
  }

  UP_INTERNAL (rfb_server_automaton, recv_protocol_version);

  // Receive the security type.
  bool recv_security_type_precondition () const {
    return m_recv_security_type && m_buffer.size () >= 1;
  }

  void recv_security_type_effect () {
    std::cout << "server: " << __func__ << std::endl;
    const uint8_t* data = static_cast<const uint8_t*> (m_buffer.data ());
    m_security_type = static_cast<rfb_security_t> (*data);
    std::cout << "server: security type = " << int (m_security_type) << std::endl;
    consume (1);
    m_recv_security_type = false;

    if (m_security_type == RFB_SECURITY_NONE) {
      if (m_version >= RFB_PROTOCOL_VERSION_3_8) {
	m_security_result = 0;
	m_send_security_result = true;
      }
      else {
	// TODO
	assert (false);
      }
    }
    else {
      // Client chose security we don't have.
      // TODO
      assert (false);
    }
  }

  UP_INTERNAL (rfb_server_automaton, recv_security_type);

  // Receive the init from the client.
  bool recv_init_precondition () const {
    return m_recv_init && m_buffer.size () >= 1;
  }

  void recv_init_effect () {
    std::cout << "server: " << __func__ << std::endl;
    // We are going to explicity ignore the share flag.
    consume (1);
    m_recv_init = false;
    m_send_init = true;
  }

  UP_INTERNAL (rfb_server_automaton, recv_init);

  void receive_effect (const ioa::const_shared_ptr<ioa::buffer_interface>& val) {
    if (val.get () != 0) {
      append (val);
    }
  }

public:
  V_UP_INPUT (rfb_server_automaton, receive, ioa::const_shared_ptr<ioa::buffer_interface>);

};

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
