#pragma once

#include <sampler/sampler_context.h>

#define SAMPLER_MAX_ITERATIONS_PER_SECOND 10

void sampler_worker(void* argument);
void sampler_worker_iteration(struct sampler_context *context);