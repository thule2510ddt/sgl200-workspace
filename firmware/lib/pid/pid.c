#include "pid.h"

static float clampf(float v, float lo, float hi)
{
	if (v < lo) {
		return lo;
	}
	if (v > hi) {
		return hi;
	}
	return v;
}

void pid_init(pid_t *p, float kp, float ki, float kd,
	      float integrator_limit, float output_limit)
{
	p->kp = kp;
	p->ki = ki;
	p->kd = kd;
	p->integrator = 0.0f;
	p->prev_measurement = 0.0f;
	p->integrator_limit = integrator_limit;
	p->output_limit = output_limit;
}

float pid_compute(pid_t *p, float setpoint, float measurement, float dt)
{
	float error = setpoint - measurement;

	p->integrator += error * dt;
	p->integrator = clampf(p->integrator, -p->integrator_limit, p->integrator_limit);

	/* Derivative on measurement avoids kick on setpoint step change */
	float derivative = -(measurement - p->prev_measurement) / dt;
	p->prev_measurement = measurement;

	float output = p->kp * error + p->ki * p->integrator + p->kd * derivative;
	return clampf(output, -p->output_limit, p->output_limit);
}

void pid_reset(pid_t *p)
{
	p->integrator = 0.0f;
	p->prev_measurement = 0.0f;
}
