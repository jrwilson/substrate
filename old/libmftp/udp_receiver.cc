#include "udp_receiver.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>

typedef struct {
  int fd;
  bidq_t* bidq;
} udp_receiver_t;

static void*
udp_receiver_create (const void* a)
{
  const udp_receiver_create_arg_t* arg = (const udp_receiver_create_arg_t*)a;
  assert (arg != NULL);

  udp_receiver_t* udp_receiver = (udp_receiver_t*)malloc (sizeof (udp_receiver_t));
  /* Create the socket. */
  if ((udp_receiver->fd = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror ("socket");
    exit (EXIT_FAILURE);
  }

#ifdef SO_REUSEPORT
  /* Reuse the port. */
  {
    int v = 1;
    if (setsockopt (udp_receiver->fd, SOL_SOCKET, SO_REUSEPORT, &v, sizeof (v)) == -1) {
      perror ("setsockopt");
      exit (EXIT_FAILURE);
    }
  }
#endif
  /* Reuse the address. */
  {
    int v = 1;
    if (setsockopt (udp_receiver->fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof (v)) == -1) {
      perror ("setsockopt");
      exit (EXIT_FAILURE);
    }
  }

  /* Set non-blocking. */
  if (fcntl (udp_receiver->fd, F_SETFL, O_NONBLOCK) == -1) {
    perror ("fcntl");
    exit (EXIT_FAILURE);
  }

  /* Bind. */
  struct sockaddr_in dest;
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = htonl (INADDR_ANY);
  dest.sin_port = htons (arg->port);
  if (bind (udp_receiver->fd, (struct sockaddr*)&dest, sizeof (dest)) == -1) {
    perror ("bind");
    exit (EXIT_FAILURE);
  }

  /* Initialize the queue. */
  udp_receiver->bidq = bidq_create ();

  return udp_receiver;
}

static void
udp_receiver_system_input (void* state, void* param, bid_t bid)
{
  udp_receiver_t* udp_receiver = (udp_receiver_t*)state;
  assert (udp_receiver != NULL);
  assert (bid != -1);
  assert (buffer_size (bid) == sizeof (receipt_t));

  const receipt_t* receipt = (const receipt_t*)buffer_read_ptr (bid);
  if (receipt->type == SELF_CREATED) {
    assert (schedule_read_input (udp_receiver->fd) == 0);
  }
}

bid_t
udp_receiver_packet_out (void* state, void* param)
{
  udp_receiver_t* udp_receiver = (udp_receiver_t*)state;
  assert (udp_receiver != NULL);

  if (!bidq_empty (udp_receiver->bidq)) {
    bid_t bid = bidq_front (udp_receiver->bidq);
    bidq_pop_front (udp_receiver->bidq);

    /* Go again. */
    assert (schedule_output (udp_receiver_packet_out, NULL) == 0);

    return bid;
  }
  else {
    return -1;
  }
}

static void
udp_receiver_read_input (void* state, void* param, bid_t b)
{
  udp_receiver_t* udp_receiver = (udp_receiver_t*)state;
  assert (udp_receiver != NULL);

  /* Determine the number of bytes we can read. */
  int bytes_to_read;
  ioctl (udp_receiver->fd, FIONREAD, &bytes_to_read);
  /* Create a new buffer. */
  bid_t bid = buffer_alloc (bytes_to_read);
  /* Fill the buffer. */
  ssize_t bytes_read = recv (udp_receiver->fd, buffer_write_ptr (bid), bytes_to_read, 0);
  if (bytes_read == -1) {
    perror ("recv");
    exit (EXIT_FAILURE);
  }
  if (bytes_read < bytes_to_read) {
    /* Shrink the buffer by duplicating it. */
    buffer_incref (bid);
    bid = buffer_dup (bid, bytes_read);
  }
  /* Put it into the queue. */
  bidq_push_back (udp_receiver->bidq, bid);

  /* Schedule the output function. */
  assert (schedule_output (udp_receiver_packet_out, NULL) == 0);
  /* Go again. */
  assert (schedule_read_input (udp_receiver->fd) == 0);
}

static output_t udp_receiver_outputs[] = { udp_receiver_packet_out, NULL };

descriptor_t udp_receiver_descriptor = {
  udp_receiver_create,
  udp_receiver_system_input,
  NULL,
  NULL,
  udp_receiver_read_input,
  NULL,
  NULL,
  NULL,
  udp_receiver_outputs,
  NULL
};
