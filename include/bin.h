// flat binary loader
#pragma once

#include <int/task.h>

void destroy_process(struct process_address_space *s);
struct process_address_space *load(char *path, _Bool has_buffer);

