#ifndef SERVOCONTROL_H
#define SERVOCONTROL_H

#define SERVO_OK (0)
#define SERVO_FAIL (1<<1)           // 1 Servo board report error
#define SERVO_OFFLINE (1<<2)        // 2 Servo offline
#define SERVO_BADLINK (1<<3)        // 4 Serial transmission unstable
#define SERVO_LINKDOWN (1<<4)       // 8 Serial port operate fail

#define SERVO_MOVE_DIRECTION_POSE 1
#define SERVO_MOVE_DIRECTION_STOP 0
#define SERVO_MOVE_DIRECTION_NEGA 2

#define SERVO_INVALID_VAL -1

class ServoControl {
public:
    virtual int enable() = 0;
    virtual int disable() = 0;
    virtual int reset() = 0;
    virtual int setmove(int axis, int direction) = 0;
    virtual int setspeed(int axis, int speed) = 0;
    virtual int setpos(int axis, int val) = 0;
    virtual int getmove(int axis) = 0;
    virtual int getspeed(int axis) = 0;
    virtual int getpos(int axis) = 0;
    virtual int changeblance(int axis, int newblance) = 0;
    virtual int queryblance(int axis) = 0;
    virtual int getstatus() = 0;
};

#endif // SERVOCONTROL_H
