#ifndef CNTSERVOCONTROL_H
#define CNTSERVOCONTROL_H

#include "ServoControl.h"
#include "SerialPort.h"
#include "PThreadPool.h"
#include "nullptr.h"
#include "ConfigManager.h"
#include "thread_enabler.h"

#define COMM_INTERVAL 1000 //Serial instruction sending interval, in us
#define SERVO_COMM_BUAD B115200
#define SERVO_MAX_DEG 90  //Servo type, in degree

#define COMM_BADCOMM_THRESHOLD 100

#define INST_RESEND_TIMEOUT_S_PART 0 //Data send fail interval, in sec
#define INST_RESEND_TIMEOUT_US_PART 500 //Data send fail interval, in us



class CntServoControl;

class CntServoControlWorkThxCtrlBlk {
public:
    CntServoControlWorkThxCtrlBlk(SerialPort *controlPort, CntServoControl *controller);
    ~CntServoControlWorkThxCtrlBlk();
    void reset();

    CntServoControl *controller;

    SerialPort *commPort;
    fd_set rfds;
    struct timeval tv;
    int sfd;

    thread_enabler_t enabler;
    pthread_t workthd;
    volatile int midX;
    volatile int posX;
    volatile int spdX;
    volatile int movX;

    volatile int midY;
    volatile int posY;
    volatile int spdY;
    volatile int movY;

    volatile bool workingflag;
    volatile int errorflag;

    int sendPos(unsigned char axis, unsigned char deg);
};

class CntServoControl : public ServoControl {
public:
    CntServoControl(SerialPort *controlPort);
    ~CntServoControl();
    virtual int enable();
    virtual int disable();
    virtual int reset();

    virtual int setmove(int axis, int direction);
    virtual int setspeed(int axis, int speed);
    virtual int setpos(int axis, int val);

    virtual int getmove(int axis);
    virtual int getspeed(int axis);
    virtual int getpos(int axis);

    virtual int changeblance(int axis, int newblance);
    virtual int queryblance(int axis);

    virtual int getstatus();
private:
    class CntServoControlWorkThxCtrlBlk tcb;

};

#endif // CNTSERVOCONTROL_H
