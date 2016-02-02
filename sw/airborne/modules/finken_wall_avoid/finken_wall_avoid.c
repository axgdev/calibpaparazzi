#include <modules/finken_wall_avoid/finken_wall_avoid.h>
#include "modules/finken_model/finken_model_sensors.h"
#include "modules/finken_model/finken_model_system.h"

#define T 1.0f/FINKEN_WALL_AVOID_UPDATE_FREQ
#define T1 FINKEN_WALL_AVOID_CONTROL_DELAY_TIME
#define Tv FINKEN_WALL_AVOID_CONTROL_HOLD_TIME
#define Kp FINKEN_WALL_AVOID_CONTROL_GAIN

#define a0 (T-2.0f*T1)/(T+2.0f*T1)
#define b0 Kp*(T-2.0f*Tv)/(T+2.0f*T1)
#define b1 Kp*(T+2.0f*Tv)/(T+2.0f*T1)

#define FINKEN_SONAR_DIFF_FREE_DIST (FINKEN_SONAR_DIFF_GOAL_DIST*FINKEN_SONAR_DIFF_FREE_FACTOR)

static const float maxControlRoll  = FINKEN_WALL_AVOID_MAX_CONTROL;
static const float maxControlPitch = FINKEN_WALL_AVOID_MAX_CONTROL;

float pitchError_k1, pitch_k1, rollError_k1, roll_k1;
float pitchInDamped, rollInDamped;

static float rollControl(float rollError) {
	float roll = -a0*roll_k1 + b1*rollError + b0*rollError_k1;
	roll = (roll < -maxControlPitch) ? -maxControlPitch : roll;
	roll = (roll > maxControlPitch) ? maxControlPitch : roll;
	roll_k1 = roll;
	rollError_k1 = rollError;
	return roll;
}

static float rollWallAvoid(float rollIn, float distY) {
	float newRoll = rollControl(distY);
	float mod = distY/FINKEN_SONAR_DIFF_FREE_DIST;
  	if (mod > 0 && rollIn > 0)
		rollIn*=mod;
	if (mod < 0  && rollIn < 0)
		rollIn*=-mod;
	return newRoll + rollIn;
}

static float pitchControl(float pitchError) {
	float pitch = -a0*pitch_k1 + b1*pitchError + b0*pitchError_k1;
	pitch = (pitch < -maxControlPitch) ? -maxControlPitch : pitch;
	pitch = (pitch > maxControlPitch) ? maxControlPitch : pitch;
	pitch_k1 = pitch;
	pitchError_k1 = pitchError;
	return pitch;
}

static float pitchWallAvoid(float pitchIn, float distX) {
	float newPitch = pitchControl(distX);
	float mod = distX/FINKEN_SONAR_DIFF_FREE_DIST;
	if (mod > 0 && pitchIn > 0)
		pitchIn*=mod;
	if (mod < 0  && pitchIn < 0)
		pitchIn*=-mod;
	return newPitch + pitchIn;
}

void finken_wall_avoid_init() {
	pitchError_k1 = 0.0f;
	pitch_k1      = 0.0f;
	rollError_k1  = 0.0f;
	roll_k1       = 0.0f;
		
	register_periodic_telemetry(DefaultPeriodic, "FINKEN_WALL_AVOID", send_finken_wall_avoid_telemetry);
}

void finken_wall_avoid_periodic() {

	finken_actuators_set_point.pitch = pitchWallAvoid(finken_system_set_point.pitch, finken_sensor_model.distance_diff_x/100.0);
	finken_actuators_set_point.roll = rollWallAvoid(finken_system_set_point.roll, finken_sensor_model.distance_diff_y/100.0);
	finken_actuators_set_point.yaw = finken_system_set_point.yaw;

}


void send_finken_wall_avoid_telemetry(struct transport_tx *trans, struct link_device* link) {
	trans = trans;
	link = link;
	float front=finken_sensor_model.distance_d_front/100.0f;
	float back=finken_sensor_model.distance_d_back/100.0f;
	float left=finken_sensor_model.distance_d_left/100.0f;
	float right=finken_sensor_model.distance_d_right/100.0f;
	float z=POS_FLOAT_OF_BFP(finken_sensor_model.pos.z);
	DOWNLINK_SEND_FINKEN_WALL_AVOID(
	DefaultChannel,
	DefaultDevice,
	&z,
	&pitchInDamped,
	&rollInDamped,
	&pitch_k1,
	&roll_k1,
	&rollError_k1,
	&pitchError_k1,
	&front,
	&back,
	&left,
	&right,
	&finken_actuators_set_point.thrust
	);
}
