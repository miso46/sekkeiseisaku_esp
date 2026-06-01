#pragma once

class PID {
public:
  float kp, ki, kd;

  PID(float kp, float ki, float kd) : kp(kp), ki(ki), kd(kd) {}

  float update(float setpoint, float measured, float dt_ms) {
    if (dt_ms <= 0.f)
      return 0.f;
    float dt_s = dt_ms / 1000.f;

    float error = setpoint - measured;
    integral += error * dt_s;
    float derivative = (error - prev_error) / dt_s;
    prev_error = error;

    float output = kp * error + ki * integral + kd * derivative;
    if (output < 0.f)
      output = 0.f;
    if (output > 255.f)
      output = 255.f;
    return output;
  }

  void reset() {
    integral = 0.f;
    prev_error = 0.f;
  }

private:
  float integral = 0.f;
  float prev_error = 0.f;
};
