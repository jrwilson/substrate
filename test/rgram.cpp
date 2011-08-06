#include <substrate/rgram.hpp>

#include "minunit.h"

#include <iostream>
#include <ioa/buffer.hpp>

static const char* char_test () {
  std::cout << __func__ << std::endl;
  char c = 'A';
  ioa::buffer ibuf;
  ibuf.append (&c, sizeof (c));
  rgram::buffer rbuf (ibuf);
  rgram::char_gramel receiver;

  receiver.put (rbuf);
  mu_assert (receiver.done ());
  mu_assert (receiver.get () == c);

  return 0;
}

static const char* int8_test () {
  std::cout << __func__ << std::endl;
  int8_t i = -111;
  ioa::buffer ibuf;
  ibuf.append (&i, sizeof (i));
  rgram::buffer rbuf (ibuf);
  rgram::int8_gramel receiver;

  receiver.put (rbuf);
  mu_assert (receiver.done ());
  mu_assert (receiver.get () == i);

  return 0;
}

static const char* uint8_test () {
  std::cout << __func__ << std::endl;
  uint8_t i = 250;
  ioa::buffer ibuf;
  ibuf.append (&i, sizeof (i));
  rgram::buffer rbuf (ibuf);
  rgram::uint8_gramel receiver;

  receiver.put (rbuf);
  mu_assert (receiver.done ());
  mu_assert (receiver.get () == i);

  return 0;
}

static const char* int16_test () {
  std::cout << __func__ << std::endl;
  int16_t i = -111;
  ioa::buffer ibuf;
  i = htons (i);
  ibuf.append (&i, sizeof (i));
  i = ntohs (i);
  rgram::buffer rbuf (ibuf);
  rgram::int16_gramel receiver;

  receiver.put (rbuf);
  mu_assert (receiver.done ());
  mu_assert (receiver.get () == i);

  return 0;
}

static const char* uint16_test () {
  std::cout << __func__ << std::endl;
  uint16_t i = 550;
  ioa::buffer ibuf;
  i = htons (i);
  ibuf.append (&i, sizeof (i));
  i = ntohs (i);
  rgram::buffer rbuf (ibuf);
  rgram::uint16_gramel receiver;

  receiver.put (rbuf);
  mu_assert (receiver.done ());
  mu_assert (receiver.get () == i);

  return 0;
}

static const char* int32_test () {
  std::cout << __func__ << std::endl;
  int32_t i = -100000;
  ioa::buffer ibuf;
  i = htonl (i);
  ibuf.append (&i, sizeof (i));
  i = ntohl (i);
  rgram::buffer rbuf (ibuf);
  rgram::int32_gramel receiver;

  receiver.put (rbuf);
  mu_assert (receiver.done ());
  mu_assert (receiver.get () == i);

  return 0;
}

static const char* uint32_test () {
  std::cout << __func__ << std::endl;
  uint32_t i = 100000;
  ioa::buffer ibuf;
  i = htonl (i);
  ibuf.append (&i, sizeof (i));
  i = ntohl (i);
  rgram::buffer rbuf (ibuf);
  rgram::uint32_gramel receiver;

  receiver.put (rbuf);

  mu_assert (receiver.done ());
  mu_assert (receiver.get () == i);

  return 0;
}

static const char* fixed_array_test () {
  std::cout << __func__ << std::endl;
  const size_t SIZE = 5;
  uint32_t r[SIZE] = { 0, 1, 2, 3, 4};
  ioa::buffer ibuf;
  for (size_t i = 0; i < SIZE; ++i) {
    r[i] = htonl (r[i]);
    ibuf.append (&r[i], sizeof (r[i]));
    r[i] = ntohl (r[i]);
  }
  rgram::buffer rbuf (ibuf);
  rgram::fixed_array_gramel<rgram::uint32_gramel, SIZE> receiver;

  receiver.put (rbuf);

  mu_assert (receiver.done ());
  const uint32_t* q = receiver.get ();
  mu_assert (std::equal (r, r + SIZE, q));

  return 0;
}

static const char* dynamic_array_test () {
  std::cout << __func__ << std::endl;
  const size_t SIZE = 5;
  uint32_t r[SIZE] = { 0, 1, 2, 3, 4};
  ioa::buffer ibuf;
  for (size_t i = 0; i < SIZE; ++i) {
    r[i] = htonl (r[i]);
    ibuf.append (&r[i], sizeof (r[i]));
    r[i] = ntohl (r[i]);
  }
  rgram::buffer rbuf (ibuf);
  rgram::dynamic_array_gramel<rgram::uint32_gramel> receiver;
  receiver.set_size (SIZE);

  receiver.put (rbuf);

  mu_assert (receiver.done ());
  const std::vector<uint32_t>& q = receiver.get ();
  mu_assert (std::equal (r, r + SIZE, q.begin ()));

  return 0;
}

static const char* sequence_test () {
  std::cout << __func__ << std::endl;

  rgram::char_gramel r1;
  rgram::int8_gramel r2;
  rgram::uint8_gramel r3;

  rgram::sequence_gramel receiver;
  receiver.append (&r1);
  receiver.append (&r2);
  receiver.append (&r3);

  ioa::buffer ibuf;

  char c1 = 'A';
  ibuf.append (&c1, sizeof (c1));

  int8_t c2 = -111;
  ibuf.append (&c2, sizeof (c2));

  uint8_t c3 = 111;
  ibuf.append (&c3, sizeof (c3));

  rgram::buffer rbuf (ibuf);

  receiver.put (rbuf);

  mu_assert (receiver.done ());
  mu_assert (r1.get () == c1);
  mu_assert (r2.get () == c2);
  mu_assert (r3.get () == c3);

  return 0;
}

static const char* choice_test () {
  std::cout << __func__ << std::endl;

  rgram::int8_gramel r1;
  rgram::int8_gramel r2;
  rgram::int8_gramel r3;

  rgram::choice_gramel<rgram::char_gramel> receiver;
  receiver.choices.insert (std::make_pair ('A', &r1));
  receiver.choices.insert (std::make_pair ('B', &r2));
  receiver.choices.insert (std::make_pair ('C', &r3));

  ioa::buffer ibuf;

  char c1 = 'B';
  ibuf.append (&c1, sizeof (c1));

  int8_t c2 = -111;
  ibuf.append (&c2, sizeof (c2));

  rgram::buffer rbuf (ibuf);

  receiver.put (rbuf);

  mu_assert (receiver.done ());
  mu_assert (receiver.get () == c1);
  mu_assert (r2.get () == c2);

  return 0;
}

const char*
all_tests ()
{
  mu_run_test (char_test);
  mu_run_test (int8_test);
  mu_run_test (uint8_test);
  mu_run_test (int16_test);
  mu_run_test (uint16_test);
  mu_run_test (int32_test);
  mu_run_test (uint32_test);
  mu_run_test (fixed_array_test);
  mu_run_test (dynamic_array_test);
  mu_run_test (sequence_test);
  mu_run_test (choice_test);

  return 0;
}
