/**
 * MadgwickFilter.h
 *
 * Madgwick AHRS (Attitude and Heading Reference System) algorithm
 * for quaternion-based orientation estimation from IMU data.
 *
 * Based on: Sebastian O.H. Madgwick (2010)
 * "An efficient orientation filter for inertial and inertial/magnetic sensor
 * arrays" https://x-io.co.uk/res/doc/madgwick_internal_report.pdf
 *
 * Adapted for ESP32/M5Stack by GaitOS Team (2026)
 * Optimized for ankle-mounted gait analysis
 */

#ifndef MADGWICK_FILTER_H
#define MADGWICK_FILTER_H

#include <Arduino.h>
#include <math.h>

class MadgwickFilter {
private:
  // Quaternion representation of orientation (q0 = w, q1 = x, q2 = y, q3 = z)
  float q0, q1, q2, q3;

  // Filter gain (beta)
  // Higher = trust accel/mag more (less smooth, more responsive)
  // Lower = trust gyro more (smoother, less responsive)
  // Typical range: 0.01 - 0.5
  float beta;

  // Fast inverse square root (Quake III algorithm)
  // More efficient than 1.0f / sqrtf(x)
  float invSqrt(float x) {
    float halfx = 0.5f * x;
    float y = x;
    long i = *(long *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
    y = y * (1.5f - (halfx * y * y));
    return y;
  }

public:
  /**
   * Constructor
   */
  MadgwickFilter() : q0(1.0f), q1(0.0f), q2(0.0f), q3(0.0f), beta(0.1f) {}

  /**
   * Initialize filter with custom gain
   * @param beta Filter gain (default 0.1 works well for ankle-mounted IMU)
   */
  void begin(float beta = 0.1f) {
    this->beta = beta;
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
  }

  /**
   * Update filter with new IMU measurements (6DOF - no magnetometer)
   *
   * @param gx Gyroscope X (rad/s)
   * @param gy Gyroscope Y (rad/s)
   * @param gz Gyroscope Z (rad/s)
   * @param ax Accelerometer X (g)
   * @param ay Accelerometer Y (g)
   * @param az Accelerometer Z (g)
   * @param dt Sample period (seconds)
   */
  void update(float gx, float gy, float gz, float ax, float ay, float az,
              float dt) {
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1,
        q2q2, q3q3;

    // Rate of change of quaternion from gyroscope
    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

    // Compute feedback only if accelerometer measurement valid (avoids NaN in
    // accelerometer normalisation)
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

      // Normalise accelerometer measurement
      recipNorm = invSqrt(ax * ax + ay * ay + az * az);
      ax *= recipNorm;
      ay *= recipNorm;
      az *= recipNorm;

      // Auxiliary variables to avoid repeated arithmetic
      _2q0 = 2.0f * q0;
      _2q1 = 2.0f * q1;
      _2q2 = 2.0f * q2;
      _2q3 = 2.0f * q3;
      _4q0 = 4.0f * q0;
      _4q1 = 4.0f * q1;
      _4q2 = 4.0f * q2;
      _8q1 = 8.0f * q1;
      _8q2 = 8.0f * q2;
      q0q0 = q0 * q0;
      q1q1 = q1 * q1;
      q2q2 = q2 * q2;
      q3q3 = q3 * q3;

      // Gradient decent algorithm corrective step
      s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
      s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 +
           _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
      s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 +
           _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
      s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
      recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 +
                          s3 * s3); // normalise step magnitude
      s0 *= recipNorm;
      s1 *= recipNorm;
      s2 *= recipNorm;
      s3 *= recipNorm;

      // Apply feedback step
      qDot1 -= beta * s0;
      qDot2 -= beta * s1;
      qDot3 -= beta * s2;
      qDot4 -= beta * s3;
    }

    // Integrate rate of change of quaternion to yield quaternion
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;

    // Normalise quaternion
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
  }

  /**
   * Get current quaternion
   */
  void getQuaternion(float *q0_out, float *q1_out, float *q2_out,
                     float *q3_out) {
    *q0_out = q0;
    *q1_out = q1;
    *q2_out = q2;
    *q3_out = q3;
  }

  /**
   * Get Euler angles from quaternion
   * @param roll Output roll angle (degrees)
   * @param pitch Output pitch angle (degrees)
   * @param yaw Output yaw angle (degrees)
   */
  void getEuler(float *roll, float *pitch, float *yaw) {
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
    float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
    *roll = atan2(sinr_cosp, cosr_cosp) * 180.0f / PI;

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (q0 * q2 - q3 * q1);
    if (fabs(sinp) >= 1)
      *pitch = copysign(90.0f, sinp); // Use 90 degrees if out of range
    else
      *pitch = asin(sinp) * 180.0f / PI;

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
    float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    *yaw = atan2(siny_cosp, cosy_cosp) * 180.0f / PI;
  }

  /**
   * Rotate a vector from body frame to navigation frame using current
   * quaternion
   * @param vx, vy, vz Input vector (body frame)
   * @param result Output vector (navigation frame) - must be float[3]
   */
  void rotateVector(float vx, float vy, float vz, float *result) {
    // Quaternion rotation: v' = q * v * q_conj
    // Implemented as DCM multiplication for efficiency

    float q0q0 = q0 * q0;
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;

    result[0] = vx * (q0q0 + q1q1 - q2q2 - q3q3) +
                vy * 2.0f * (q1 * q2 - q0 * q3) +
                vz * 2.0f * (q1 * q3 + q0 * q2);

    result[1] = vx * 2.0f * (q1 * q2 + q0 * q3) +
                vy * (q0q0 - q1q1 + q2q2 - q3q3) +
                vz * 2.0f * (q2 * q3 - q0 * q1);

    result[2] = vx * 2.0f * (q1 * q3 - q0 * q2) +
                vy * 2.0f * (q2 * q3 + q0 * q1) +
                vz * (q0q0 - q1q1 - q2q2 + q3q3);
  }

  /**
   * Reset quaternion to identity (no rotation)
   */
  void reset() {
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
  }
};

#endif // MADGWICK_FILTER_H
