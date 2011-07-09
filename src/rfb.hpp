#ifndef __rfb_hpp__
#define __rfb_hpp__

#include <string>
#include <vector>
#include <set>

#include <stdint.h>
#include <arpa/inet.h>

#include <ioa/buffer.hpp>
#include <ioa/shared_ptr.hpp>

#include <iostream>
// TODO: Make buffer processing much more efficient.
// Appends and more importantly consumes should be minimized.

namespace rfb {
  
  const size_t PROTOCOL_VERSION_STRING_LENGTH = 12;

  struct protocol_version_t
  {
    char version[PROTOCOL_VERSION_STRING_LENGTH];

    protocol_version_t () { }

    protocol_version_t (const char* v) {
      memcpy (version, v, PROTOCOL_VERSION_STRING_LENGTH);
    }

    int compare (const protocol_version_t& other) const {
      return memcmp (version, other.version, PROTOCOL_VERSION_STRING_LENGTH);
    }

    bool operator== (const protocol_version_t& other) const {
      return compare (other) == 0;
    }

    bool operator!= (const protocol_version_t& other) const {
      return compare (other) != 0;
    }

    bool operator< (const protocol_version_t& other) const {
      return compare (other) < 0;
    }

    bool operator> (const protocol_version_t& other) const {
      return compare (other) > 0;
    }

    bool operator<= (const protocol_version_t& other) const {
      return compare (other) <= 0;
    }

    bool operator>= (const protocol_version_t& other) const {
      return compare (other) >= 0;
    }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      return buf.size () >= PROTOCOL_VERSION_STRING_LENGTH;
    }

    void read_from_buffer (ioa::buffer& buf) {
      memcpy (version, buf.data (), PROTOCOL_VERSION_STRING_LENGTH);
      buf.consume (PROTOCOL_VERSION_STRING_LENGTH);
    }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (version, PROTOCOL_VERSION_STRING_LENGTH);
    }
  };

  const protocol_version_t PROTOCOL_VERSION_3_3 ("RFB 003.003\n");
  const protocol_version_t PROTOCOL_VERSION_3_7 ("RFB 003.007\n");
  const protocol_version_t PROTOCOL_VERSION_3_8 ("RFB 003.008\n");

  enum security_t {
    INVALID,
    NONE,
  };
  
  struct security_types_t
  {
    std::set<security_t> security_types;

    security_types_t () { }

    security_types_t (const std::set<security_t>& types) :
      security_types (types)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      if (buf.size () >= sizeof (uint8_t)) {
	const uint8_t* number_of_security_types = static_cast<const uint8_t*> (buf.data ());
	return buf.size () >= (*number_of_security_types + sizeof (uint8_t));
      }
      else {
	return false;
      }
    }

    void read_from_buffer (ioa::buffer& buf) {
      const uint8_t* data = static_cast<const uint8_t*> (buf.data ());
      const uint8_t number_of_security_types = *data;
      ++data;
      for (uint8_t i = 0; i < number_of_security_types; ++i) {
	security_types.insert (static_cast<security_t> (*data));
	++data;
      }
      buf.consume (number_of_security_types + sizeof (uint8_t));
    }

    void write_to_buffer (ioa::buffer& buf) const {
      uint8_t x;

      x = security_types.size ();
      buf.append (&x, sizeof (x));
      for (std::set<security_t>::const_iterator pos = security_types.begin ();
	   pos != security_types.end ();
	   ++pos) {
	x = *pos;
	buf.append (&x, sizeof (x));
      }
    }
  };

  struct security_type_t
  {
    security_t security;

    security_type_t () :
      security (INVALID)
    { }

    security_type_t (const security_t& s) :
      security (s)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      return buf.size () >= sizeof (uint8_t);
    }

    void read_from_buffer (ioa::buffer& buf) {
      uint8_t x;
      memcpy (&x, buf.data (), sizeof (x));
      security = static_cast<security_t> (x);
      buf.consume (sizeof (x));
    }

    void write_to_buffer (ioa::buffer& buf) const {
      uint8_t x = security;
      buf.append (&x, sizeof (x));
    }

  };

  struct security_result_t
  {
    uint32_t status;

    security_result_t () { }

    security_result_t (const bool success) {
      status = success ? 0 : 1;
    }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      return buf.size () >= sizeof (uint32_t);
    }

    void read_from_buffer (ioa::buffer& buf) {
      memcpy (&status, buf.data (), sizeof (status));
      status = ntohl (status);
      buf.consume (sizeof (status));
    }

    void write_to_buffer (ioa::buffer& buf) const {
      uint32_t x = htonl (status);
      buf.append (&x, sizeof (x));
    }
  };

  struct client_init_t
  {
    uint8_t shared_flag;

    client_init_t () { }

    client_init_t (const bool flag) :
      shared_flag (flag)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      return buf.size () >= sizeof (uint8_t);
    }

    void read_from_buffer (ioa::buffer& buf) {
      memcpy (&shared_flag, buf.data (), sizeof (shared_flag));
      buf.consume (sizeof (shared_flag));
    }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&shared_flag, sizeof (shared_flag));
    }
  };

  struct pixel_format_t {
    uint8_t bits_per_pixel;
    uint8_t depth;
    uint8_t big_endian_flag;
    uint8_t true_colour_flag;
    uint16_t red_max;
    uint16_t green_max;
    uint16_t blue_max;
    uint8_t red_shift;
    uint8_t green_shift;
    uint8_t blue_shift;

    pixel_format_t () { }

    pixel_format_t (const uint8_t bpp,
		    const uint8_t dep,
		    const uint8_t bef,
		    const uint8_t tcf,
		    const uint16_t rmax,
		    const uint16_t gmax,
		    const uint16_t bmax,
		    const uint8_t rshift,
		    const uint8_t gshift,
		    const uint8_t bshift) :
      bits_per_pixel (bpp),
      depth (dep),
      big_endian_flag (bef),
      true_colour_flag (tcf),
      red_max (rmax),
      green_max (gmax),
      blue_max (bmax),
      red_shift (rshift),
      green_shift (gshift),
      blue_shift (bshift)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&bits_per_pixel, sizeof (bits_per_pixel));
      buf.append (&depth, sizeof (depth));
      buf.append (&big_endian_flag, sizeof (big_endian_flag));
      buf.append (&true_colour_flag, sizeof (true_colour_flag));
      uint16_t x;
      x = htons (red_max);
      buf.append (&x, sizeof (x));
      x = htons (green_max);
      buf.append (&x, sizeof (x));
      x = htons (blue_max);
      buf.append (&x, sizeof (x));
      buf.append (&red_shift, sizeof (red_shift));
      buf.append (&green_shift, sizeof (green_shift));
      buf.append (&blue_shift, sizeof (blue_shift));
      // Three bytes of padding.
      buf.resize (buf.size () + 3);
    }

    void read_from_buffer (ioa::buffer& buf) {
      memcpy (&bits_per_pixel, buf.data (), sizeof (bits_per_pixel));
      buf.consume (sizeof (bits_per_pixel));
      memcpy (&depth, buf.data (), sizeof (depth));
      buf.consume (sizeof (depth));
      memcpy (&big_endian_flag, buf.data (), sizeof (big_endian_flag));
      buf.consume (sizeof (big_endian_flag));
      memcpy (&true_colour_flag, buf.data (), sizeof (true_colour_flag));
      buf.consume (sizeof (true_colour_flag));
      memcpy (&red_max, buf.data (), sizeof (red_max));
      red_max = ntohs (red_max);
      buf.consume (sizeof (red_max));
      memcpy (&green_max, buf.data (), sizeof (green_max));
      green_max = ntohs (green_max);
      buf.consume (sizeof (green_max));
      memcpy (&blue_max, buf.data (), sizeof (blue_max));
      blue_max = ntohs (blue_max);
      buf.consume (sizeof (blue_max));
      memcpy (&red_shift, buf.data (), sizeof (red_shift));
      buf.consume (sizeof (red_shift));
      memcpy (&green_shift, buf.data (), sizeof (green_shift));
      buf.consume (sizeof (green_shift));
      memcpy (&blue_shift, buf.data (), sizeof (blue_shift));
      buf.consume (sizeof (blue_shift));
      // Three bytes of padding.
      buf.consume (3);
    }

  };

  struct server_init_t {
    uint16_t framebuffer_width;
    uint16_t framebuffer_height;
    pixel_format_t server_pixel_format;
    std::string name;

    server_init_t () { }

    server_init_t (const uint16_t width,
		   const uint16_t height,
		   const pixel_format_t& format,
		   const std::string& n) :
      framebuffer_width (width),
      framebuffer_height (height),
      server_pixel_format (format),
      name (n)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      if (buf.size () >= 24) {
	const uint32_t* ptr = static_cast<const uint32_t*> (buf.data ());
	// Advance to name length.
	ptr += 5;
	const uint32_t name_len = ntohl (*ptr);
	return buf.size () >= (24 + name_len);
      }
      else {
	return false;
      }
    }

    void read_from_buffer (ioa::buffer& buf) {
      memcpy (&framebuffer_width, buf.data (), sizeof (framebuffer_width));
      framebuffer_width = ntohs (framebuffer_width);
      buf.consume (sizeof (framebuffer_width));
      memcpy (&framebuffer_height, buf.data (), sizeof (framebuffer_height));
      framebuffer_height = ntohs (framebuffer_height);
      buf.consume (sizeof (framebuffer_height));
      server_pixel_format.read_from_buffer (buf);
      uint32_t name_len;
      memcpy (&name_len, buf.data (), sizeof (name_len));
      name_len = ntohl (name_len);
      buf.consume (sizeof (name_len));
      name = std::string (static_cast<const char*> (buf.data ()), name_len);
      buf.consume (name_len);
    }

    void write_to_buffer (ioa::buffer& buf) const {
      uint16_t x;
      x = htons (framebuffer_width);
      buf.append (&x, sizeof (x));
      x = htons (framebuffer_height);
      buf.append (&x, sizeof (x));
      server_pixel_format.write_to_buffer (buf);
      uint32_t y;
      y = htonl (name.size ());
      buf.append (&y, sizeof (y));
      buf.append (name.c_str (), name.size ());
    }
  };

  struct set_pixel_format_t {
    uint8_t message_type;
    pixel_format_t pixel_format;

    set_pixel_format_t () { }

    set_pixel_format_t (const pixel_format_t& format) :
      message_type (0),
      pixel_format (format)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      if (buf.size () >= 20) {
	const uint8_t* ptr = static_cast<const uint8_t*> (buf.data ());
	return *ptr == 0;
      }
      else {
	return false;
      }
    }

    void read_from_buffer (ioa::buffer& buf) {
      message_type = 0;
      buf.consume (4);
      pixel_format.read_from_buffer (buf);
    }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&message_type, sizeof (message_type));
      // Padding.
      buf.resize (buf.size () + 3);
      pixel_format.write_to_buffer (buf);
    }
  };

  const int32_t RAW = 0;
  const int32_t COPY_RECT = 1;
  const int32_t RRE = 2;
  const int32_t HEXTILE = 5;
  const int32_t ZRLE = 16;

  struct set_encodings_t {
    uint8_t message_type;
    std::vector<int32_t> encodings;

    set_encodings_t () { }

    set_encodings_t (const std::vector<int32_t>& enc) :
      message_type (2),
      encodings (enc)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      if (buf.size () >= 4) {
	const uint8_t* ptr = static_cast<const uint8_t*> (buf.data ());
	if (*ptr == 2) {
	  const uint16_t* p = static_cast<const uint16_t*> (buf.data ());
	  ++p;
	  return buf.size () >= (4u + ntohs (*p));
	}
	else {
	  return false;
	}
      }
      else {
	return false;
      }
    }

    void read_from_buffer (ioa::buffer& buf) {
      message_type = 2;
      buf.consume (2);
      uint16_t number_of_encodings = ntohs (*static_cast<uint16_t *> (buf.data ()));
      buf.consume (2);
      int32_t* data = static_cast<int32_t*> (buf.data ());
      for (uint16_t i = 0; i < number_of_encodings; ++i) {
	encodings.push_back (ntohl (*data));
	++data;
      }
      
      buf.consume (number_of_encodings * sizeof (int32_t));
    }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&message_type, sizeof (message_type));
      // Padding.
      buf.resize (buf.size () + 1);
      uint16_t number_of_encodings = htons (encodings.size ());
      buf.append (&number_of_encodings, sizeof (number_of_encodings));
      for (std::vector<int32_t>::const_iterator pos = encodings.begin ();
	   pos != encodings.end ();
	   ++pos) {
	int32_t encoding = htonl (*pos);
	buf.append (&encoding, sizeof (encoding));
      }
    }
  };

  struct framebuffer_update_request_t
  {
    uint8_t message_type;
    uint8_t incremental;
    uint16_t x_position;
    uint16_t y_position;
    uint16_t width;
    uint16_t height;

    framebuffer_update_request_t () { }

    framebuffer_update_request_t (const bool incr,
				  const uint16_t xpos,
				  const uint16_t ypos,
				  const uint16_t w,
				  const uint16_t h) :
      message_type (3),
      incremental (incr),
      x_position (xpos),
      y_position (ypos),
      width (w),
      height (h)
    { }

    static bool could_read_from_buffer (const ioa::buffer& buf) {
      if (buf.size () >= 10) {
	const uint8_t* ptr = static_cast<const uint8_t*> (buf.data ());
	return *ptr == 3;
      }
      else {
	return false;
      }
    }

    void read_from_buffer (ioa::buffer& buf) {
      memcpy (&message_type, buf.data (), sizeof (message_type));
      buf.consume (sizeof (message_type));
      memcpy (&incremental, buf.data (), sizeof (incremental));
      buf.consume (sizeof (incremental));
      memcpy (&x_position, buf.data (), sizeof (x_position));
      x_position = ntohs (x_position);
      buf.consume (sizeof (x_position));
      memcpy (&y_position, buf.data (), sizeof (y_position));
      y_position = ntohs (y_position);
      buf.consume (sizeof (y_position));
      memcpy (&width, buf.data (), sizeof (width));
      width = ntohs (width);
      buf.consume (sizeof (width));
      memcpy (&height, buf.data (), sizeof (height));
      height = ntohs (height);
      buf.consume (sizeof (height));
    }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&message_type, sizeof (message_type));
      buf.append (&incremental, sizeof (incremental));
      uint16_t x;
      x = htons (x_position);
      buf.append (&x, sizeof (x));
      x = htons (y_position);
      buf.append (&x, sizeof (x));
      x = htons (width);
      buf.append (&x, sizeof (x));
      x = htons (height);
      buf.append (&x, sizeof (x));
    }
  };

  struct rectangle_t
  {
    uint16_t x_position;
    uint16_t y_position;
    uint16_t width;
    uint16_t height;
    int32_t encoding_type;

    rectangle_t (const uint16_t xpos,
		 const uint16_t ypos,
		 const uint16_t w,
		 const uint16_t h) :
      x_position (xpos),
      y_position (ypos),
      width (w),
      height (h)
    { }

    virtual ~rectangle_t () { }

    void write_to_buffer (ioa::buffer& buf,
			  const pixel_format_t& server_format,
			  const pixel_format_t& client_format,
			  const void* source_data,
			  const uint16_t source_width,
			  const uint16_t source_height) const {
      uint16_t x;
      x = htons (x_position);
      buf.append (&x, sizeof (x));
      x = htons (y_position);
      buf.append (&x, sizeof (x));
      x = htons (width);
      buf.append (&x, sizeof (x));
      x = htons (height);
      buf.append (&x, sizeof (x));
      int32_t y;
      y = htonl (encoding_type);
      buf.append (&y, sizeof (y));
      write_to_buffer_dispatch (buf, server_format, client_format, source_data, source_width, source_height);
    }

    virtual void write_to_buffer_dispatch (ioa::buffer& buf,
					   const pixel_format_t& server_format,
					   const pixel_format_t& client_format,
					   const void* source_data,
					   const uint16_t source_width,
					   const uint16_t source_height) const = 0;
  };

  struct raw_rectangle_t :
    public rectangle_t
  {
    raw_rectangle_t (const uint16_t xpos,
		     const uint16_t ypos,
		     const uint16_t w,
		     const uint16_t h) :
      rectangle_t (xpos, ypos, w, h)
    {
      encoding_type = RAW;
    }

    void write_to_buffer_dispatch (ioa::buffer& buf,
				   const pixel_format_t& server_format,
				   const pixel_format_t& client_format,
				   const void* source_data,
				   const uint16_t source_width,
				   const uint16_t source_height) const {
      // These restrictions need to be relaxed later.
      assert (server_format.bits_per_pixel == 32);
      assert (server_format.bits_per_pixel == client_format.bits_per_pixel);
      assert (server_format.depth == 24);
      assert (server_format.depth == client_format.depth);
      assert (server_format.big_endian_flag == client_format.big_endian_flag);
      assert (server_format.true_colour_flag == client_format.true_colour_flag);
      assert (server_format.red_max == client_format.red_max);
      assert (server_format.green_max == client_format.green_max);
      assert (server_format.blue_max == client_format.blue_max);
      assert (server_format.red_shift == client_format.red_shift);
      assert (server_format.green_shift == client_format.green_shift);
      assert (server_format.blue_shift == client_format.blue_shift);

      size_t offset = buf.size ();
      size_t data_size = width * height * client_format.bits_per_pixel / 8;
      buf.resize (buf.size () + data_size);
      const uint32_t* source = reinterpret_cast<const uint32_t*> (source_data);
      uint32_t* dest = reinterpret_cast<uint32_t*> (static_cast<char*> (buf.data ()) + offset);

      for (uint16_t y = 0; y < height; ++y) {
	for (uint16_t x = 0; x < width; ++x) {
	  const uint16_t x_pos = x_position + x;
	  const uint16_t y_pos = y_position + y;
	  *dest = source[y_pos * source_width + x_pos];
	  ++dest;
	}
      }
    }
  };

  struct framebuffer_update_t
  {
    uint8_t message_type;
    std::vector<ioa::shared_ptr<rectangle_t> > rectangles;

    framebuffer_update_t () :
      message_type (0)
    { }

    void add_rectangle (rectangle_t* rect) {
      if (rect != 0) {
	rectangles.push_back (ioa::shared_ptr<rectangle_t> (rect));
      }
    }

    static bool could_read_from_buffer (const ioa::buffer& buf,
					const pixel_format_t& format) {
      if (buf.size () >= 4) {
	const uint8_t* data = static_cast<const uint8_t*> (buf.data ());
	const uint8_t* end = data + buf.size ();
	if (*data == 0) {
	  // Skip over message type and padding.
	  data += 2;
	  const uint16_t number_of_rectangles = ntohs (*reinterpret_cast<const uint16_t*> (data));
	  data += 2;
	  for (uint16_t i = 0; i < number_of_rectangles; ++i) {
	    if (end - data >= 16) {
	      // Skip over positions.
	      data += 4;
	      // Get the width and height.
	      const uint16_t width = ntohs (*reinterpret_cast<const uint16_t*> (data));
	      data += 2;
	      const uint16_t height = ntohs (*reinterpret_cast<const uint16_t*> (data));
	      data += 2;
	      const int32_t encoding_type = ntohl (*reinterpret_cast<const int32_t*> (data));
	      data += 4;
	      switch (encoding_type) {
	      case RAW:
		data += width * height * format.bits_per_pixel / 8;
		if (end - data < 0) {
		  return false;
		}
		break;
	      default:
		// Unknown encoding.
		assert (false);
	      }
	    }
	    else {
	      return false;
	    }
	  }
	  // Made it through all the rectangles.
	  return true;
	}
      }

      return false;
    }

    void read_from_buffer (ioa::buffer& buf,
			   const pixel_format_t& format,
			   void* destination_data,
			   const uint16_t destination_width,
			   const uint16_t destination_height) {
      const uint8_t* data = static_cast<const uint8_t*> (buf.data ());
      message_type = *data;
      // Skip over padding.
      data += 2;
      const uint16_t number_of_rectangles = ntohs (*reinterpret_cast<const uint16_t*> (data));
      data += sizeof (uint16_t);
      for (uint16_t i = 0; i < number_of_rectangles; ++i) {
	// Get the position.
	const uint16_t x_position = ntohs (*reinterpret_cast<const uint16_t*> (data));
	data += sizeof (uint16_t);
	const uint16_t y_position = ntohs (*reinterpret_cast<const uint16_t*> (data));
	data += sizeof (uint16_t);
	// Get the width and height.
	const uint16_t width = ntohs (*reinterpret_cast<const uint16_t*> (data));
	data += sizeof (uint16_t);
	const uint16_t height = ntohs (*reinterpret_cast<const uint16_t*> (data));
	data += sizeof (uint16_t);
	// Get the encoding.
	const int32_t encoding_type = ntohl (*reinterpret_cast<const int32_t*> (data));
	data += sizeof (int32_t);
	switch (encoding_type) {
	case RAW:
	  assert (format.bits_per_pixel == 32);
	  assert (format.depth == 24);
	  const uint32_t* source = reinterpret_cast<const uint32_t*> (data);
	  uint32_t* destination = static_cast<uint32_t*> (destination_data);
	  for (uint16_t y = 0; y < height; ++y) {
	    for (uint16_t x = 0; x < width; ++x) {
	      const uint16_t x_pos = x_position + x;
	      const uint16_t y_pos = y_position + y;
	      if (x_pos < destination_width && y_pos < destination_height) {
		destination[y_pos * destination_width + x_pos] = *source;
	      }
	      ++source;
	    }
	  }
	  // Skip the data.
	  data += width * height * format.bits_per_pixel / 8;
	  break;
	default:
	  // Unknown encoding.
	  assert (false);
	}
      }

      // Consume the bytes.
      buf.consume (data - static_cast<uint8_t*> (buf.data ()));
    }

    void write_to_buffer (ioa::buffer& buf,
			  const pixel_format_t& server_format,
			  const pixel_format_t& client_format,
			  const void* source_data,
			  const uint16_t source_width,
			  const uint16_t source_height) const {
      buf.append (&message_type, sizeof (message_type));
      buf.resize (buf.size () + 1);
      uint16_t number_of_rectangles = htons (rectangles.size ());
      buf.append (&number_of_rectangles, sizeof (number_of_rectangles));
      for (std::vector<ioa::shared_ptr<rectangle_t> >::const_iterator pos = rectangles.begin ();
	   pos != rectangles.end ();
	   ++pos) {
	(*pos)->write_to_buffer (buf, server_format, client_format, source_data, source_width, source_height);
      }
    }

  };

}

#endif
