#include "FixServoControl.h"

#define SERVO_X_MAX 200
#define SERVO_X_MIN 0
#define SERVO_Y_MAX 200
#define SERVO_Y_MIN 0

/*------------------------------------------------------------------*/

static const unsigned char BUFFER_INST_RECOVERY[] = {'I', 'I', 'I'};

static void* workroutine(void* args) {
    FixServoControlWorkThxCtrlBlk *tcb = (FixServoControlWorkThxCtrlBlk*) args;
    int cur_flag;
    int badlink_cnt = 0;
    while (tcb->workingflag) {
        thread_enabler_wait(&tcb->enabler);
        switch (tcb->movX) {
        case SERVO_MOVE_DIRECTION_POSE:
            if (tcb->posX < SERVO_X_MAX)
                tcb->posX += tcb->spdX;
            else
                tcb->movX = SERVO_MOVE_DIRECTION_STOP;
            break;
        case SERVO_MOVE_DIRECTION_NEGA:
            if (tcb->posX > SERVO_X_MIN)
                tcb->posX -= tcb->spdX;
            else
                tcb->movX = SERVO_MOVE_DIRECTION_STOP;
            break;
        default:
            break;
        }

        cur_flag = tcb->sendPos('X', tcb->posX);
        if (cur_flag == SERVO_OK) {
            tcb->errorflag = 0;
            if (badlink_cnt > 0) {
                --badlink_cnt;
                tcb->errorflag |= SERVO_BADLINK;
            }
        } else {
            tcb->commPort->writeport(BUFFER_INST_RECOVERY, sizeof (BUFFER_INST_RECOVERY));
            tcb->commPort->flush();
            if (cur_flag == SERVO_BADLINK) {
                tcb->errorflag |= SERVO_BADLINK;
                badlink_cnt = COMM_BADCOMM_THRESHOLD;
            } else {
                if (cur_flag == SERVO_LINKDOWN)
                    tcb->reset();
                tcb->errorflag |= cur_flag;
            }
        }

        switch (tcb->movY) {
        case SERVO_MOVE_DIRECTION_POSE:
            if (tcb->posY < SERVO_Y_MAX)
                tcb->posY += tcb->spdY;
            else
                tcb->movY = SERVO_MOVE_DIRECTION_STOP;
            break;
        case SERVO_MOVE_DIRECTION_NEGA:
            if (tcb->posY > SERVO_Y_MIN)
                tcb->posY -= tcb->spdY;
            else
                tcb->movY = SERVO_MOVE_DIRECTION_STOP;
            break;
        default:
            break;
        }

        cur_flag |= tcb->sendPos('Y', tcb->posY);
        if (cur_flag == SERVO_OK) {
            tcb->errorflag = 0;
            if (badlink_cnt > 0) {
                --badlink_cnt;
                tcb->errorflag |= SERVO_BADLINK;
            }
        } else {
            tcb->commPort->writeport(BUFFER_INST_RECOVERY, sizeof (BUFFER_INST_RECOVERY));
            tcb->commPort->flush();
            if (cur_flag == SERVO_BADLINK) {
                tcb->errorflag |= SERVO_BADLINK;
                badlink_cnt = COMM_BADCOMM_THRESHOLD;
            } else {
                if (cur_flag == SERVO_LINKDOWN)
                    tcb->reset();
                tcb->errorflag |= cur_flag;
            }
        }

        usleep(COMM_INTERVAL);
    }
    return 0;
}

FixServoControlWorkThxCtrlBlk::FixServoControlWorkThxCtrlBlk(SerialPort *controlPort, FixServoControl *controller) {
    commPort = controlPort;
    commPort->openport(ConfigManager::getCOM(), SERVO_COMM_BUAD, true);
    commPort->set_timeout(10);
    //commPort->setblock(true);

    tv.tv_sec = INST_RESEND_TIMEOUT_S_PART;
    tv.tv_usec = INST_RESEND_TIMEOUT_US_PART;

    sfd = commPort->getfd();
    FD_ZERO(&rfds);
    FD_SET(sfd, &rfds);

    thread_enabler_init(&enabler);
    thread_enabler_disable(&enabler);
    this->controller = controller;
    midX = ConfigManager::getServoBlanceX();
    posX = midX;
    spdX = 0;
    movX = SERVO_MOVE_DIRECTION_STOP;

    midY = ConfigManager::getServoBlanceY();
    posY = midY;
    spdY = 0;
    movY = SERVO_MOVE_DIRECTION_STOP;
    errorflag = SERVO_OK;
    workingflag = true;
    pthread_create(&workthd, nullptr, workroutine, this);
}

FixServoControlWorkThxCtrlBlk::~FixServoControlWorkThxCtrlBlk() {
    workingflag = false;
    pthread_cancel(workthd);
    pthread_join(workthd, nullptr); //wait thread exit and release resource full
    thread_enabler_destory(&enabler);
    commPort->closeport();
}

void FixServoControlWorkThxCtrlBlk::reset() {
    commPort->closeport();
    usleep(100);
    if (commPort->openport(ConfigManager::getCOM(), SERVO_COMM_BUAD, true) == SERIAL_OK)
        errorflag = SERVO_OK;
    else
        errorflag = SERVO_LINKDOWN;
    commPort->set_timeout(10);
    //commPort->setblock(true);
    sfd = commPort->getfd();
    FD_ZERO(&rfds);
    FD_SET(sfd, &rfds);

    tv.tv_sec = INST_RESEND_TIMEOUT_S_PART;
    tv.tv_usec = INST_RESEND_TIMEOUT_US_PART;

    midX = ConfigManager::getServoBlanceX();
    posX = midX;
    spdX = 0;
    movX = SERVO_MOVE_DIRECTION_STOP;
    midY = ConfigManager::getServoBlanceY();
    posY = midY;
    spdY = 0;
    movY = SERVO_MOVE_DIRECTION_STOP;
}

int FixServoControlWorkThxCtrlBlk::sendPos(unsigned char axis, unsigned char deg) {
    unsigned char txbuf[2];
    unsigned char rxbuf[3];
    txbuf[0] = axis;
    txbuf[1] = deg;

    for (int i = 0; i < 3; ++i) {
        int rtnval;
        if (commPort->writeport(txbuf, 2) != 2)
            return SERVO_LINKDOWN;
        usleep(100000);
        tv.tv_sec = INST_RESEND_TIMEOUT_S_PART;
        tv.tv_usec = INST_RESEND_TIMEOUT_US_PART;
        //rtnval = select(sfd+1,&rfds,NULL,NULL,&tv);
        rtnval = commPort->readfix(rxbuf, 3);
        if (rtnval == 3) {
            if ((rxbuf[0] == 'S')&&(rxbuf[1] == axis)&&(rxbuf[2] == deg))
                return SERVO_OK;
            else if (rxbuf[0] == 'F')
                return SERVO_FAIL;
            else {
                printf("SERVO SEND ERROR DAT:%02X,%02X,%02X", rxbuf[0], rxbuf[1], rxbuf[2]);
                return SERVO_BADLINK;
            }
        } else if (rtnval < 0) {
            return SERVO_LINKDOWN;
        }
    }
    return SERVO_OFFLINE;
}

/*------------------------------------------------------------------*/

FixServoControl::FixServoControl(SerialPort *controlPort) : tcb(FixServoControlWorkThxCtrlBlk(controlPort, this)) {
}

FixServoControl::~FixServoControl() {

}

int FixServoControl::enable() {
    thread_enabler_enable(&tcb.enabler);
    return tcb.errorflag;
}

int FixServoControl::disable() {
    thread_enabler_disable(&tcb.enabler);
    return tcb.errorflag;
}

int FixServoControl::reset() {
    tcb.reset();
    return tcb.errorflag;
}

int FixServoControl::setmove(int axis, int direction) {
    if (axis == 'X') {
        tcb.movX = direction;
    } else if (axis == 'Y') {
        tcb.movY = direction;
    }
    return tcb.errorflag;
}

int FixServoControl::setspeed(int axis, int speed) {
    if (axis == 'X') {
        tcb.spdX = speed;
    } else if (axis == 'Y') {
        tcb.spdY = speed;
    }
    return tcb.errorflag;
}

int FixServoControl::setpos(int axis, int val) {
    if (axis == 'X') {
        tcb.posX = val;
    } else if (axis == 'Y') {
        tcb.posY = val;
    }
    return tcb.errorflag;
}

int FixServoControl::getmove(int axis) {
    if (axis == 'X') {
        return tcb.movX;
    } else if (axis == 'Y') {
        return tcb.movY;
    }
    return tcb.errorflag;
}

int FixServoControl::getspeed(int axis) {
    if (axis == 'X') {
        return tcb.spdX;
    } else if (axis == 'Y') {
        return tcb.spdY;
    }
    return tcb.errorflag;
}

int FixServoControl::getpos(int axis) {
    if (axis == 'X') {
        return tcb.posX;
    } else if (axis == 'Y') {
        return tcb.posY;
    }
    return tcb.errorflag;
}

int FixServoControl::changeblance(int axis, int newblance) {
    if (axis == 'X') {
        tcb.midX = newblance;
        ConfigManager::setServoBlanceX(newblance);
    } else if (axis == 'Y') {
        tcb.midY = newblance;
        ConfigManager::setServoBlanceY(newblance);
    }
    return SERVO_OK;
}

int FixServoControl::queryblance(int axis) {
    if (axis == 'X') {
        return tcb.midX;
        //return ConfigManager::getServoBlanceX();
    } else if (axis == 'Y') {
        return tcb.midY;
        //return ConfigManager::getServoBlanceY();
    }
    return tcb.errorflag;
}

int FixServoControl::getstatus() {
    return tcb.errorflag;
}


