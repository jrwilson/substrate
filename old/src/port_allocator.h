#ifndef __port_allocator_h__
#define __port_allocator_h__

#include "component_manager.h"

typedef struct port_allocator_struct port_allocator_t;

port_allocator_t* port_allocator_create (port_type_descriptor_t* port_descriptors);
uint32_t port_allocator_port_type_count (port_allocator_t* port_allocator);
bool port_allocator_contains_free_port (port_allocator_t* port_allocator, uint32_t idx);
uint32_t port_allocator_get_free_port (port_allocator_t* port_allocator, uint32_t idx);
uint32_t port_allocator_cardinality (port_allocator_t* port_allocator, uint32_t idx);
uint32_t port_allocator_free_count (port_allocator_t* port_allocator, uint32_t idx);
uint32_t port_allocator_input_message_count (port_allocator_t* port_allocator, uint32_t idx);
uint32_t port_allocator_output_message_count (port_allocator_t* port_allocator, uint32_t idx);

#endif
