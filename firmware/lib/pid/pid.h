#pragma once

typedef struct {
	float kp;
	float ki;
	float kd;
	float integrator;
	float prev_measurement;
	float integrator_limit;
	float output_limit;
} pid_t;

void pid_init(pid_t *p, float kp, float ki, float kd,
	      float integrator_limit, float output_limit);
float pid_compute(pid_t *p, float setpoint, float measurement, float dt);
void pid_reset(pid_t *p);
