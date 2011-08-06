#ifndef __rgram_hpp__
#define __rgram_hpp__

#include <stdint.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <ioa/buffer.hpp>

#include <iostream>

// Reactive grammar.
namespace rgram {

  class buffer
  {
  private:
    const ioa::buffer_interface& m_buffer;
    size_t m_idx;
  public:
    buffer (const ioa::buffer_interface& buf) :
      m_buffer (buf),
      m_idx (0)
    { }

    size_t consume (void* ptr,
		    const size_t bytes) {
      const unsigned char* src = static_cast<const unsigned char*> (m_buffer.data ()) + m_idx;
      const unsigned char* const src_limit = static_cast<const unsigned char*> (m_buffer.data ()) + m_buffer.size ();
      unsigned char* dest = static_cast<unsigned char*> (ptr);
      const unsigned char* dest_limit = dest + bytes;

      for (; src != src_limit && dest != dest_limit; *(dest++) = *(src++))
	;;

      const size_t c = bytes - (dest_limit - dest);
      m_idx += c;
      return c;
    }

    bool empty () const {
      return m_idx == m_buffer.size ();
    }

    size_t size () const {
      return m_buffer.size () - m_idx;
    }
  };

  // Grammar element.
  class gramel
  {
  public:
    virtual ~gramel () { }
    virtual void put (buffer& buf) = 0;
    virtual bool done () const = 0;
    virtual void reset () = 0;
  };

  class char_gramel :
    public gramel
  {
  private:
    char m_char;
    uint8_t m_bytes;

  public:
    typedef char value_type;

    char_gramel () :
      m_bytes (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_bytes += buf.consume (&m_char, sizeof (m_char) - m_bytes);
    }

    bool done () const {
      return m_bytes == sizeof (m_char);
    }

    char get () const {
      return m_char;
    }

    void reset () {
      m_bytes = 0;
    }
  };

  class int8_gramel :
    public gramel
  {
  private:
    int8_t m_int;
    uint8_t m_bytes;

  public:
    typedef int8_t value_type;

    int8_gramel () :
      m_bytes (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_bytes += buf.consume (&m_int, sizeof (m_int) - m_bytes);
    }

    bool done () const {
      return m_bytes == sizeof (m_int);
    }

    int8_t get () const {
      return m_int;
    }

    void reset () {
      m_bytes = 0;
    }
  };

  class uint8_gramel :
    public gramel
  {
  private:
    uint8_t m_uint;
    uint8_t m_bytes;

  public:
    typedef uint8_t value_type;

    uint8_gramel () :
      m_bytes (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_bytes += buf.consume (&m_uint, sizeof (m_uint) - m_bytes);
    }

    bool done () const {
      return m_bytes == sizeof (m_uint);
    }

    uint8_t get () const {
      return m_uint;
    }

    void reset () {
      m_bytes = 0;
    }
  };

  class int16_gramel :
    public gramel
  {
  private:
    int16_t m_int;
    uint8_t m_count;

  public:
    typedef int16_t value_type;

    int16_gramel () :
      m_count (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_count += buf.consume (&m_int, sizeof (m_int) - m_count);
      if (done ()) {
	m_int = ntohs (m_int);
      }
    }

    bool done () const {
      return m_count == sizeof (m_int);
    }

    int16_t get () const {
      return m_int;
    }

    void reset () {
      m_count = 0;
    }
  };

  class uint16_gramel :
    public gramel
  {
  private:
    uint16_t m_uint;
    uint8_t m_count;

  public:
    typedef uint16_t value_type;

    uint16_gramel () :
      m_count (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_count += buf.consume (&m_uint, sizeof (m_uint) - m_count);
      if (done ()) {
	m_uint = ntohs (m_uint);
      }
    }

    bool done () const {
      return m_count == sizeof (m_uint);
    }

    uint16_t get () const {
      return m_uint;
    }

    void reset () {
      m_count = 0;
    }
  };

  class int32_gramel :
    public gramel
  {
  private:
    int32_t m_int;
    uint8_t m_count;

  public:
    typedef int32_t value_type;

    int32_gramel () :
      m_count (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_count += buf.consume (&m_int, sizeof (m_int) - m_count);
      if (done ()) {
	m_int = ntohl (m_int);
      }
    }

    bool done () const {
      return m_count == sizeof (m_int);
    }

    int32_t get () const {
      return m_int;
    }

    void reset () {
      m_count = 0;
    }
  };

  class uint32_gramel :
    public gramel
  {
  private:
    uint32_t m_uint;
    uint8_t m_count;

  public:
    typedef uint32_t value_type;

    uint32_gramel () :
      m_count (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      m_count += buf.consume (&m_uint, sizeof (m_uint) - m_count);
      if (done ()) {
	m_uint = ntohl (m_uint);
      }
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

  template <typename T, size_t SIZE>
  class fixed_array_gramel :
    public gramel
  {
  private:
    typename T::value_type m_values[SIZE];
    T m_action;
    size_t m_idx;

  public:
    fixed_array_gramel () :
      m_idx (0)
    { }

    void put (buffer& buf) {
      assert (!done ());

      while (!done () && !buf.empty ()) {
	m_action.put (buf);
	if (m_action.done ()) {
	  m_values[m_idx] = m_action.get ();
	  m_action.reset ();
	  ++m_idx;
	}
      }
    }

    bool done () const {
      return m_idx == SIZE;
    }

    const typename T::value_type * get () const {
      return m_values;
    }

    void reset () {
      m_action.reset ();
      m_idx = 0;
    }

  };

  template <typename T>
  class dynamic_array_gramel :
    public gramel
  {
  private:
    std::vector<typename T::value_type> m_values;
    T m_action;
    bool m_size_set;
    size_t m_expected_size;

  public:
    dynamic_array_gramel () :
      m_size_set (false),
      m_expected_size (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      while (!done () && !buf.empty ()) {
	m_action.put (buf);
	if (m_action.done ()) {
	  m_values.push_back (m_action.get ());
	  m_action.reset ();
	}
      }
    }

    bool done () const {
      return m_size_set && m_values.size () == m_expected_size;
    }

    const std::vector<typename T::value_type>& get () const {
      return m_values;
    }

    void reset () {
      m_values.clear ();
      m_action.reset ();
      m_size_set = false;
      m_expected_size = 0;
    }

    void set_size (const size_t sz) {
      m_expected_size = sz;
      m_size_set = true;
    }
  };

  class sequence_gramel :
    public gramel
  {
  private:
    std::vector<gramel*> m_seq;
    size_t m_idx;

  public:
    sequence_gramel () :
      m_idx (0)
    { }

    void put (buffer& buf) {
      assert (!done ());
      while (!done () && !buf.empty ()) {
	m_seq[m_idx]->put (buf);
	if (m_seq[m_idx]->done ()) {
	  ++m_idx;
	}
      }
    }

    bool done () const {
      return m_idx == m_seq.size ();
    }

    void reset () {
      for (size_t i = 0; i < m_seq.size (); ++i) {
	m_seq[i]->reset ();
      }
      m_idx = 0;
    }

    void append (gramel* ptr) {
      if (ptr != 0) {
	m_seq.push_back (ptr);
      }
    }
  };

  template <typename Key, typename Value = gramel*>
  class choice_gramel :
    public gramel
  {
  private:
    Key m_action;
    typedef std::map<typename Key::value_type, Value> map_type;

    bool m_bad_key;
    
  public:
    map_type choices;

    choice_gramel () :
      m_bad_key (false)
    { }

    void put (buffer& buf) {
      assert (!done ());

      if (!m_action.done ()) {
	m_action.put (buf);
	if (m_action.done ()) {
	  typename map_type::const_iterator pos = choices.find (m_action.get ());
	  if (pos == choices.end ()) {
	    m_bad_key = true;
	  }
	}
      }

      if (!m_bad_key && m_action.done ()) {
	choices[m_action.get ()]->put (buf);
      }
    }

    bool done () const {
      if (m_bad_key) {
	return true;
      }

      if (!m_action.done ()) {
	return false;
      }

      return choices.find (m_action.get ())->second->done ();
    }

    void reset () {
      m_action.reset ();
      for (typename map_type::const_iterator pos = choices.begin ();
	   pos != choices.end ();
	   ++pos) {
	pos->second->reset ();
      }
      m_bad_key = false;
    }

    typename Key::value_type get () const {
      return m_action.get ();
    }
    
  };

}

#endif
