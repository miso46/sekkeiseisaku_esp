#pragma once

#include "car_state.hpp"

// Start the high-priority pulse processing task. The task will call
// car_state->update(delta_count, dt_ms) and enqueue state for ESP-NOW sending.
void startPulseProcessing(CarState *state);
