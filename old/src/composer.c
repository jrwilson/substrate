#include "composer.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <ueioa.h>

#include "trigger.h"
#include "counter.h"

#include <automan.h>

typedef struct {
  automan_t* automan;
  aid_t self;
  aid_t parent;

  aid_t trigger;
  aid_t counter1;
  aid_t counter2;

  bool counter1_input_composed;
  bool counter1_output_composed;
  bool counter2_input_composed;
  bool counter2_output_composed;
} composer_t;

static void*
composer_create (const void* arg)
{
  printf ("composer_create\n");
  composer_t* composer = malloc (sizeof (composer_t));

  composer->automan = automan_creat (composer,
				     &composer->self);
  assert (automan_create (composer->automan,
			  &composer->trigger,
			  &trigger_descriptor,
			  NULL,
			  NULL,
			  NULL) == 0);
  assert (automan_create (composer->automan,
			  &composer->counter1,
			  &counter_descriptor,
			  NULL,
			  NULL,
			  NULL) == 0);
  assert (automan_create (composer->automan,
			  &composer->counter2,
			  &counter_descriptor,
			  NULL,
			  NULL,
			  NULL) == 0);
  assert (automan_compose (composer->automan,
			   &composer->counter1_input_composed,
			   &composer->trigger,
			   trigger_output,
			   NULL,
			   &composer->counter1,
			   counter_input,
			   NULL,
			   NULL,
			   NULL) == 0);
  assert (automan_compose (composer->automan,
			   &composer->counter1_output_composed,
			   &composer->counter1,
			   counter_output,
			   NULL,
			   &composer->self,
			   composer_input1,
			   NULL,
			   NULL,
			   NULL) == 0);
  assert (automan_compose (composer->automan,
			   &composer->counter2_input_composed,
			   &composer->trigger,
			   trigger_output,
			   NULL,
			   &composer->counter2,
			   counter_input,
			   NULL,
			   NULL,
			   NULL) == 0);
  assert (automan_compose (composer->automan,
			   &composer->counter2_output_composed,
			   &composer->counter2,
			   counter_output,
			   NULL,
			   &composer->self,
			   composer_input2,
			   NULL,
			   NULL,
			   NULL) == 0);

  return composer;
}

static bid_t composer_system_output (void* state, void* param);

static void
composer_system_input (void* state, void* param, bid_t bid)
{
  printf ("composer_system_input\n");
  assert (state != NULL);
  composer_t* composer = state;

  assert (bid != -1);
  assert (buffer_size (bid) == sizeof (receipt_t));
  const receipt_t* receipt = buffer_read_ptr (bid);

  automan_apply (composer->automan, receipt);
}

static bid_t
composer_system_output (void* state, void* param)
{
  printf ("composer_system_output\n");
  assert (state != NULL);
  composer_t* composer = state;

  return automan_action (composer->automan);
}

void
composer_input1 (void* state, void* param, bid_t bid)
{
  assert (bid != -1);
  assert (buffer_size (bid) == sizeof (counter_output_t));

  const counter_output_t* output = buffer_read_ptr (bid);
  printf ("count(1) = %d\n", output->count);
}

void
composer_input2 (void* state, void* param, bid_t bid)
{
  assert (bid != -1);
  assert (buffer_size (bid) == sizeof (counter_output_t));

  const counter_output_t* output = buffer_read_ptr (bid);
  printf ("count(2) = %d\n", output->count);
}

static input_t composer_free_inputs[] = { NULL };
static input_t composer_inputs[] = { composer_input1, composer_input2, NULL };
static output_t composer_outputs[] = { NULL };
static internal_t composer_internals[] = { NULL };

descriptor_t composer_descriptor = {
  .constructor = composer_create,
  .system_input = composer_system_input,
  .system_output = composer_system_output,
  .free_inputs = composer_free_inputs,
  .inputs = composer_inputs,
  .outputs = composer_outputs,
  .internals = composer_internals,
};
