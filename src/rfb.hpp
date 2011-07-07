#ifndef __rfb_hpp__
#define __rfb_hpp__

#include <stdint.h>

#define RFB_PROTOCOL_VERSION_STRING_LENGTH 12
#define RFB_PROTOCOL_VERSION_3_3 "RFB 003.003\n"
#define RFB_PROTOCOL_VERSION_3_7 "RFB 003.007\n"
#define RFB_PROTOCOL_VERSION_3_8 "RFB 003.008\n"

enum rfb_security_t {
  RFB_SECURITY_INVALID,
  RFB_SECURITY_NONE,
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
  uint8_t blue_shift;
  uint8_t green_shift;
  uint8_t unused0;
  uint8_t unused1;
  uint8_t unused2;
  
  void host_to_net () {
    red_max = htons (red_max);
    green_max = htons (green_max);
    blue_max = htons (blue_max);
  }

  void net_to_host () {
    red_max = ntohs (red_max);
    green_max = ntohs (green_max);
    blue_max = ntohs (blue_max);
  }

};

struct server_init_t {
  uint16_t framebuffer_width;
  uint16_t framebuffer_height;
  pixel_format_t server_pixel_format;
  uint32_t name_length;

  void host_to_net () {
    framebuffer_width = htons (framebuffer_width);
    framebuffer_height = htons (framebuffer_height);
    server_pixel_format.host_to_net ();
    name_length = htonl (name_length);
  }

  void net_to_host () {
    framebuffer_width = ntohs (framebuffer_width);
    framebuffer_height = ntohs (framebuffer_height);
    server_pixel_format.net_to_host ();
    name_length = ntohl (name_length);
  }
};

#endif
