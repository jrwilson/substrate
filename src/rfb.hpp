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
// TODO:  Clean up this file.

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

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (version, PROTOCOL_VERSION_STRING_LENGTH);
    }
  };

  struct protocol_version_gramel :
    public rgram::gramel
  {
    rgram::fixed_array_gramel<rgram::char_gramel, rfb::PROTOCOL_VERSION_STRING_LENGTH> m_version_array;

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_version_array.put (buf);
    }

    bool done () const {
      return m_version_array.done ();
    }

    void reset () {
      m_version_array.reset ();
    }
    
    protocol_version_t get () const {
      return protocol_version_t (m_version_array.get ());
    }
  };

  const protocol_version_t PROTOCOL_VERSION_3_3 ("RFB 003.003\n");

  enum security_t {
    INVALID,
    NONE,
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

    void write_to_buffer (ioa::buffer& buf) const {
      uint32_t x = htonl (security);
      buf.append (&x, sizeof (x));
    }

  };

  struct security_type_gramel :
    public rgram::gramel
  {
    rgram::uint32_gramel m_security_type;

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_security_type.put (buf);
    }

    bool done () const {
      return m_security_type.done ();
    }

    void reset () {
      m_security_type.reset ();
    }

    security_type_t get () const {
      return security_type_t (security_t (m_security_type.get ()));
    }
  };

  struct client_init_t
  {
    uint8_t shared_flag;

    client_init_t () { }

    client_init_t (const bool flag) :
      shared_flag (flag)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&shared_flag, sizeof (shared_flag));
    }
  };

  struct client_init_gramel :
    public rgram::gramel
  {
    rgram::uint8_gramel m_init;

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_init.put (buf);
    }

    bool done () const {
      return m_init.done ();
    }

    void reset () {
      m_init.reset ();
    }

    client_init_t get () const {
      return client_init_t (m_init.get ());
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
    uint8_t padding[3];

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
      buf.append (padding, sizeof (padding));
    }

  };

  struct pixel_format_gramel :
    public rgram::gramel
  {
    rgram::uint8_gramel m_bits_per_pixel;
    rgram::uint8_gramel m_depth;
    rgram::uint8_gramel m_big_endian_flag;
    rgram::uint8_gramel m_true_colour_flag;
    rgram::uint16_gramel m_red_max;
    rgram::uint16_gramel m_green_max;
    rgram::uint16_gramel m_blue_max;
    rgram::uint8_gramel m_red_shift;
    rgram::uint8_gramel m_green_shift;
    rgram::uint8_gramel m_blue_shift;
    rgram::fixed_array_gramel<rgram::char_gramel, 3> m_padding;
    rgram::sequence_gramel m_sequence;

    pixel_format_gramel () {
      m_sequence.append (&m_bits_per_pixel);
      m_sequence.append (&m_depth);
      m_sequence.append (&m_big_endian_flag);
      m_sequence.append (&m_true_colour_flag);
      m_sequence.append (&m_red_max);
      m_sequence.append (&m_green_max);
      m_sequence.append (&m_blue_max);
      m_sequence.append (&m_red_shift);
      m_sequence.append (&m_green_shift);
      m_sequence.append (&m_blue_shift);
      m_sequence.append (&m_padding);
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

    pixel_format_t get () const {
      return pixel_format_t (m_bits_per_pixel.get (),
			     m_depth.get (),
			     m_big_endian_flag.get (),
			     m_true_colour_flag.get (),
			     m_red_max.get (),
			     m_green_max.get (),
			     m_blue_max.get (),
			     m_red_shift.get (),
			     m_green_shift.get (),
			     m_blue_shift.get ());
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

  struct server_init_gramel :
    public rgram::gramel
  {
    rgram::uint16_gramel m_width;
    rgram::uint16_gramel m_height;
    pixel_format_gramel m_pixel_format;
    rgram::uint32_gramel m_name_length;
    rgram::sequence_gramel m_sequence;
    rgram::dynamic_array_gramel<rgram::char_gramel> m_name_string;

    server_init_gramel () {
      m_sequence.append (&m_width);
      m_sequence.append (&m_height);
      m_sequence.append (&m_pixel_format);
      m_sequence.append (&m_name_length);
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      if (!m_sequence.done ()) {
	m_sequence.put (buf);
      }
      if (m_sequence.done ()) {
	m_name_string.set_size (m_name_length.get ());
	m_name_string.put (buf);
      }
    }

    bool done () const {
      return m_name_string.done ();
    }

    void reset () {
      m_sequence.reset ();
      m_name_string.reset ();
    }

    server_init_t get () const {
      std::string s (m_name_string.get ().begin (), m_name_string.get ().end ());
      return server_init_t (m_width.get (),
			    m_height.get (),
			    m_pixel_format.get (),
			    s);
    }
  };

  const uint8_t SET_PIXEL_FORMAT_TYPE = 0;

  struct set_pixel_format_t {
    uint8_t padding[3];
    pixel_format_t pixel_format;

    set_pixel_format_t () { }

    set_pixel_format_t (const pixel_format_t& format) :
      pixel_format (format)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&SET_PIXEL_FORMAT_TYPE, sizeof (SET_PIXEL_FORMAT_TYPE));
      // Padding.
      buf.resize (buf.size () + 3);
      pixel_format.write_to_buffer (buf);
    }
  };

  struct set_pixel_format_gramel :
    public rgram::gramel
  {
    rgram::fixed_array_gramel<rgram::uint8_gramel, 3> m_padding;
    pixel_format_gramel m_pixel_format;
    rgram::sequence_gramel m_sequence;

    set_pixel_format_gramel () {
      m_sequence.append (&m_padding);
      m_sequence.append (&m_pixel_format);
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

    set_pixel_format_t get () const {
      return set_pixel_format_t (m_pixel_format.get ());
    }
  };

  const uint8_t SET_ENCODINGS_TYPE = 2;

  const int32_t RAW = 0;
  const int32_t COPY_RECT = 1;
  const int32_t RRE = 2;
  const int32_t HEXTILE = 5;
  const int32_t ZRLE = 16;

  struct set_encodings_t {
    uint8_t padding;
    uint16_t number_of_encodings;
    std::vector<int32_t> encodings;

    set_encodings_t () { }

    set_encodings_t (const std::vector<int32_t>& enc) :
      number_of_encodings (enc.size ()),
      encodings (enc)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&SET_ENCODINGS_TYPE, sizeof (SET_ENCODINGS_TYPE));
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

  struct set_encodings_gramel :
    public rgram::gramel
  {
    rgram::uint8_gramel m_padding;
    rgram::uint16_gramel m_number_of_encodings;
    rgram::sequence_gramel m_sequence;
    rgram::dynamic_array_gramel<rgram::int32_gramel> m_encoding_types;

    set_encodings_gramel () {
      m_sequence.append (&m_padding);
      m_sequence.append (&m_number_of_encodings);
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      if (!m_sequence.done ()) {
	m_sequence.put (buf);
      }
      if (m_sequence.done ()) {
	m_encoding_types.set_size (m_number_of_encodings.get ());
	m_encoding_types.put (buf);
      }
    }

    bool done () const {
      return m_encoding_types.done ();
    }

    void reset () {
      m_sequence.reset ();
      m_encoding_types.reset ();
    }

    set_encodings_t get () const {
      return set_encodings_t (m_encoding_types.get ());
    }
    
  };

  const uint8_t FRAMEBUFFER_UPDATE_REQUEST_TYPE = 3;

  struct framebuffer_update_request_t
  {
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
      incremental (incr),
      x_position (xpos),
      y_position (ypos),
      width (w),
      height (h)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&FRAMEBUFFER_UPDATE_REQUEST_TYPE, sizeof (FRAMEBUFFER_UPDATE_REQUEST_TYPE));
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

  struct framebuffer_update_request_gramel :
    public rgram::gramel
  {
    rgram::uint8_gramel m_incremental;
    rgram::uint16_gramel m_x_position;
    rgram::uint16_gramel m_y_position;
    rgram::uint16_gramel m_width;
    rgram::uint16_gramel m_height;
    rgram::sequence_gramel m_sequence;

    framebuffer_update_request_gramel () {
      m_sequence.append (&m_incremental);
      m_sequence.append (&m_x_position);
      m_sequence.append (&m_y_position);
      m_sequence.append (&m_width);
      m_sequence.append (&m_height);
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

    framebuffer_update_request_t get () const {
      return framebuffer_update_request_t (m_incremental.get (),
					   m_x_position.get (),
					   m_y_position.get (),
					   m_width.get (),
					   m_height.get ());
    }
  };

  struct client_message_gramel :
    public rgram::gramel
  {
    set_pixel_format_gramel m_set_pixel_format;
    set_encodings_gramel m_set_encodings;
    framebuffer_update_request_gramel m_framebuffer_update_request;
    rgram::choice_gramel<rgram::uint8_gramel> m_choice;

    client_message_gramel () {
      m_choice.choices.insert (std::make_pair (SET_PIXEL_FORMAT_TYPE, &m_set_pixel_format));
      m_choice.choices.insert (std::make_pair (SET_ENCODINGS_TYPE, &m_set_encodings));
      m_choice.choices.insert (std::make_pair (FRAMEBUFFER_UPDATE_REQUEST_TYPE, &m_framebuffer_update_request));
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_choice.put (buf);
    }

    bool done () const {
      return m_choice.done ();
    }

    void reset () {
      m_choice.reset ();
    }
  };

  const uint8_t FRAMEBUFFER_UPDATE_TYPE = 0;

  struct pixel_data_t {
    virtual ~pixel_data_t () { }
    virtual void write_to_buffer (ioa::buffer& buf) const = 0;
  };

  struct pixel_data_gramel :
    public rgram::gramel
  {
    virtual ~pixel_data_gramel () { }
    virtual void set_dimensions (const uint16_t xpos,
				 const uint16_t ypos,
				 const uint16_t w,
				 const uint16_t h) = 0;
  };

  struct encoding_choice_gramel :
    public rgram::gramel
  {
    typedef rgram::choice_gramel<rgram::int32_gramel, pixel_data_gramel*> choice_type;
    choice_type m_choice;

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_choice.put (buf);
    }

    bool done () const {
      return m_choice.done ();
    }

    void reset () {
      m_choice.reset ();
    }

    void add_encoding (const int32_t type,
		       pixel_data_gramel* gramel) {
      m_choice.choices.insert (std::make_pair (type, gramel));
    }

    void set_dimensions (const uint16_t xpos,
			 const uint16_t ypos,
			 const uint16_t w,
			 const uint16_t h) {
      for (choice_type::map_type::const_iterator pos = m_choice.choices.begin ();
	   pos != m_choice.choices.end ();
	   ++pos) {
	pos->second->set_dimensions (xpos, ypos, w, h);
      }
    }
  };

  struct rectangle_t
  {
    uint16_t x_position;
    uint16_t y_position;
    uint16_t width;
    uint16_t height;
    ioa::shared_ptr<pixel_data_t> data;

    rectangle_t (const uint16_t xpos,
  		 const uint16_t ypos,
  		 const uint16_t w,
  		 const uint16_t h,
		 pixel_data_t* d) :
      x_position (xpos),
      y_position (ypos),
      width (w),
      height (h),
      data (d)
    { }

    void write_to_buffer (ioa::buffer& buf) const {
      uint16_t x;
      x = htons (x_position);
      buf.append (&x, sizeof (x));
      x = htons (y_position);
      buf.append (&x, sizeof (x));
      x = htons (width);
      buf.append (&x, sizeof (x));
      x = htons (height);
      buf.append (&x, sizeof (x));
      data->write_to_buffer (buf);
    }

  };
  
  struct rectangle_gramel :
    public rgram::gramel
  {
    typedef rectangle_t value_type;
    rgram::uint16_gramel m_x_position;
    rgram::uint16_gramel m_y_position;
    rgram::uint16_gramel m_width;
    rgram::uint16_gramel m_height;
    rgram::sequence_gramel m_sequence;
    encoding_choice_gramel m_encoding_choice;

    rectangle_gramel ()
    {
      m_sequence.append (&m_x_position);
      m_sequence.append (&m_y_position);
      m_sequence.append (&m_width);
      m_sequence.append (&m_height);
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      if (!m_sequence.done ()) {
	m_sequence.put (buf);
      }
      if (m_sequence.done ()) {
	m_encoding_choice.set_dimensions (m_x_position.get (),
					  m_y_position.get (),
					  m_width.get (),
					  m_height.get ());
	m_encoding_choice.put (buf);
      }
    }

    bool done () const {
      return m_encoding_choice.done ();
    }

    void reset () {
      m_sequence.reset ();
      m_encoding_choice.reset ();
    }

    void add_encoding (const int32_t type,
		       pixel_data_gramel* gramel) {
      m_encoding_choice.add_encoding (type, gramel);
    }
  };

  struct framebuffer_update_t
  {
    std::vector<rectangle_t> rectangles;

    framebuffer_update_t ()
    { }

    framebuffer_update_t (const std::vector<rectangle_t>& rects) :
      rectangles (rects)
    { }

    void add_rectangle (const rectangle_t& rect) {
      rectangles.push_back (rect);
    }

    void write_to_buffer (ioa::buffer& buf) const {
      buf.append (&FRAMEBUFFER_UPDATE_TYPE, sizeof (FRAMEBUFFER_UPDATE_TYPE));
      buf.resize (buf.size () + 1);
      uint16_t number_of_rectangles = htons (rectangles.size ());
      buf.append (&number_of_rectangles, sizeof (number_of_rectangles));
      for (std::vector<rectangle_t>::const_iterator pos = rectangles.begin ();
      	   pos != rectangles.end ();
      	   ++pos) {
      	(*pos).write_to_buffer (buf);
      }
    }
    
  };
  
  struct rectangles_gramel :
    public rgram::gramel
  {
    rectangle_gramel m_rectangle;
    size_t m_expected_count;
    size_t m_count;
    bool m_count_set;

    rectangles_gramel () :
      m_expected_count (0),
      m_count (0),
      m_count_set (false)
    { }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_rectangle.put (buf);
      if (m_rectangle.done ()) {
	++m_count;
	m_rectangle.reset ();
      }
    }

    bool done () const {
      return m_count_set && m_count == m_expected_count;
    }

    void reset () {
      m_rectangle.reset ();
      m_expected_count = 0;
      m_count = 0;
      m_count_set = false;
    }

    void set_count (const size_t c) {
      m_expected_count = c;
      m_count_set = true;
    }

    void add_encoding (const int32_t type,
		       pixel_data_gramel* gramel) {
      m_rectangle.add_encoding (type, gramel);
    }

  };

  struct framebuffer_update_gramel :
    public rgram::gramel
  {
    rgram::uint8_gramel m_padding;
    rgram::uint16_gramel m_number_of_rectangles;
    rgram::sequence_gramel m_sequence;
    rectangles_gramel m_rectangles;

    framebuffer_update_gramel ()
    {
      m_sequence.append (&m_padding);
      m_sequence.append (&m_number_of_rectangles);
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      if (!m_sequence.done ()) {
	m_sequence.put (buf);
      }
      if (m_sequence.done ()) {
	m_rectangles.set_count (m_number_of_rectangles.get ());
	m_rectangles.put (buf);
      }
    }

    bool done () const {
      return m_rectangles.done ();
    }

    void reset () {
      m_sequence.reset ();
      m_rectangles.reset ();
    }

    void add_encoding (const int32_t type,
		       pixel_data_gramel* gramel) {
      m_rectangles.add_encoding (type, gramel);
    }
  };

  struct server_message_gramel :
    public rgram::gramel
  {
    framebuffer_update_gramel m_framebuffer_update;
    rgram::choice_gramel<rgram::uint8_gramel> m_choice;

    server_message_gramel ()
    {
      m_choice.choices.insert (std::make_pair (FRAMEBUFFER_UPDATE_TYPE, &m_framebuffer_update));
    }

    void put (rgram::buffer& buf) {
      assert (!done ());
      m_choice.put (buf);
    }

    bool done () const {
      return m_choice.done ();
    }

    void reset () {
      m_choice.reset ();
    }

    void add_encoding (const int32_t type,
		       pixel_data_gramel* gramel) {
      m_framebuffer_update.add_encoding (type, gramel);
    }

  };

}

#endif
