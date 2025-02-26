/// @file	AP_Camera.h
/// @brief	Photo or video camera manager, with EEPROM-backed storage of constants.
#pragma once

#include "AP_Camera_config.h"

#if AP_CAMERA_ENABLED

#include <AP_Common/Location.h>
#include <AP_Logger/LogStructure.h>
#include <AP_Param/AP_Param.h>
#include <GCS_MAVLink/GCS_MAVLink.h>

#define AP_CAMERA_TRIGGER_DEFAULT_DURATION  10      // default duration servo or relay is held open in 10ths of a second (i.e. 10 = 1 second)

#define AP_CAMERA_SERVO_ON_PWM              1300    // default PWM value to move servo to when shutter is activated
#define AP_CAMERA_SERVO_OFF_PWM             1100    // default PWM value to move servo to when shutter is deactivated

#define AP_CAMERA_FEEDBACK_DEFAULT_FEEDBACK_PIN -1  // default is to not use camera feedback pin

/// @class	Camera
/// @brief	Object managing a Photo or video camera
class AP_Camera {

public:
    AP_Camera(uint32_t _log_camera_bit)
        : log_camera_bit(_log_camera_bit)
    {
        AP_Param::setup_object_defaults(this, var_info);
        _singleton = this;
    }

    /* Do not allow copies */
    CLASS_NO_COPY(AP_Camera);

    // get singleton instance
    static AP_Camera *get_singleton()
    {
        return _singleton;
    }

    // MAVLink methods
    void            handle_message(mavlink_channel_t chan,
                                   const mavlink_message_t &msg);
    void            send_feedback(mavlink_channel_t chan) const;

    // Command processing
    void            configure(float shooting_mode, float shutter_speed, float aperture, float ISO, float exposure_type, float cmd_id, float engine_cutoff_time);
    // handle camera control
    void            control(float session, float zoom_pos, float zoom_step, float focus_lock, float shooting_cmd, float cmd_id);

    // set camera trigger distance in a mission
    void            set_trigger_distance(uint32_t distance_m)
    {
        _trigg_dist.set(distance_m);
    }

    // momentary switch to change camera modes
    void cam_mode_toggle();

    void take_picture();

    // start/stop recording video
    // start_recording should be true to start recording, false to stop recording
    bool record_video(bool start_recording);

    // zoom in, out or hold
    // zoom out = -1, hold = 0, zoom in = 1
    bool set_zoom_step(int8_t zoom_step);

    // focus in, out or hold
    // focus in = -1, focus hold = 0, focus out = 1
    bool set_manual_focus_step(int8_t focus_step);

    // auto focus
    bool set_auto_focus();

    // Update - to be called periodically @at least 50Hz
    void update();

    static const struct AP_Param::GroupInfo        var_info[];

    // set if vehicle is in AUTO mode
    void set_is_auto_mode(bool enable)
    {
        _is_in_auto_mode = enable;
    }

    enum camera_types {
        CAMERA_TYPE_STD,
        CAMERA_TYPE_BMMCC
    };

    enum class CamTrigType {
        servo   = 0,
        relay   = 1,
        gopro   = 2,
        mount   = 3,
    };

    AP_Camera::CamTrigType get_trigger_type(void);

private:

    static AP_Camera *_singleton;

    void            control_msg(const mavlink_message_t &msg);

    AP_Int8         _trigger_type;      // 0:Servo,1:Relay, 2:GoPro in Solo Gimbal
    AP_Int8         _trigger_duration;  // duration in 10ths of a second that the camera shutter is held open
    AP_Int8         _relay_on;          // relay value to trigger camera
    AP_Int16        _servo_on_pwm;      // PWM value to move servo to when shutter is activated
    AP_Int16        _servo_off_pwm;     // PWM value to move servo to when shutter is deactivated
    uint8_t         _trigger_counter;   // count of number of cycles shutter has been held open
    uint8_t         _trigger_counter_cam_function;   // count of number of cycles alternative camera function has been held open
    AP_Int8         _auto_mode_only;    // if 1: trigger by distance only if in AUTO mode.
    AP_Int8         _type;              // Set the type of camera in use, will open additional parameters if set
    bool            _is_in_auto_mode;   // true if in AUTO mode

    void            servo_pic();        // Servo operated camera
    void            relay_pic();        // basic relay activation
    void            feedback_pin_timer();
    void            feedback_pin_isr(uint8_t, bool, uint32_t);
    void            setup_feedback_callback(void);

    AP_Float        _trigg_dist;        // distance between trigger points (meters)
    AP_Int16        _min_interval;      // Minimum time between shots required by camera
    AP_Int16        _max_roll;          // Maximum acceptable roll angle when trigging camera
    uint32_t        _last_photo_time;   // last time a photo was taken
    bool            _trigger_pending;   // true when we have delayed take_picture
    Location        _last_location;
    uint16_t        _image_index;       // number of pictures taken since boot

    // pin number for accurate camera feedback messages
    AP_Int8         _feedback_pin;
    AP_Int8         _feedback_polarity;

    uint32_t        _camera_trigger_count;
    uint32_t        _camera_trigger_logged;
    uint32_t        _feedback_trigger_timestamp_us;
    struct {
        uint64_t        timestamp_us;
        Location        location; // place where most recent image was taken
        int32_t         roll_sensor;
        int32_t         pitch_sensor;
        int32_t         yaw_sensor;
        uint32_t        camera_trigger_logged;  // ID sequence number
    } feedback;
    void prep_mavlink_msg_camera_feedback(uint64_t timestamp_us);

    bool            _timer_installed;
    bool            _isr_installed;
    uint8_t         _last_pin_state;

    void log_picture();

    // Logging Function
    void Write_Camera(uint64_t timestamp_us=0);
    void Write_Trigger(void);
    void Write_CameraInfo(enum LogMessages msg, uint64_t timestamp_us=0);

    uint32_t log_camera_bit;

    // update camera trigger - 50Hz
    void update_trigger();

    // entry point to trip local shutter (e.g. by relay or servo)
    void trigger_pic();

    // de-activate the trigger after some delay, but without using a delay() function
    // should be called at 50hz from main program
    void trigger_pic_cleanup();

    // return true if we are using a feedback pin
    bool using_feedback_pin(void) const
    {
        return _feedback_pin > 0;
    }

};

namespace AP {
AP_Camera *camera();
};

#endif
