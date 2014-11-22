#include "CamControlserver.h"
#include "ConfigManager.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include "md5.h"

#define MD5_LEN 16

#define INST_QUERY_TYPE 'Q'
#define INST_SET_TYPE 'S'
#define INST_DROP_TYPE 'D' // Drop Connection
#define INST_KEEP_TYPE 'K' // NOP, just for keep alive

#define INST_BLAC 'B' // Blance Deg
#define INST_APOS 'A' // Absolute Postion
#define INST_SPED 'S' // Moving Speed
#define INST_MOVE 'M' // Moving State
#define INST_REST 'R' // Reset

#define INST_AXIS_X 'X'
#define INST_AXIS_Y 'Y'

#define BUFFER_SIZE 1024
#define ORIGIN_KEY_FORMAT "%s:%04d:%02d:%02d:%01d:%02d:%s" //"USER:YYYY:MM:DD:W:HH:PSWD"

#define NEED_PASSWD

class InternalException : public std::exception {
public:

    InternalException(ServerMsg reason) : reason(reason) {
    }
    ServerMsg reason;
};

static void printTimeStampFirst(){
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    printf("%04d/%02d/%02d %02d:%02d:%02d\t--\t",
           p->tm_year + 1900,
           p->tm_mon + 1,
           p->tm_mday,
           p->tm_hour,
           p->tm_min,
           p->tm_sec);
}

CamControlServer::CamControlServer(int numThread) : NetworkServer(numThread) {
    //serialMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&serialMutex, NULL);
    originKeySize = 0;
    originKeySize += strlen(ConfigManager::getLoginPass());
    originKeySize += strlen(ConfigManager::getLoginUser());
    originKeySize += 18;
    while ((originKeyBuffer = (char*) malloc(sizeof (char)*originKeySize)) == nullptr);
    serialPort = new SerialPort();
    servoctl = new CntServoControl(serialPort);
    servoctl->enable();
};

CamControlServer::~CamControlServer() {
    servoctl->disable();
    pthread_mutex_destroy(&serialMutex);
    delete servoctl;
    delete serialPort;
    delete originKeyBuffer;
}



void CamControlServer::innerAction(int connfd, struct sockaddr *clientAddr) {
    char buffer[32];
    int i;
    int rtnVal;
    try {
        printTimeStampFirst();
        printf("IP:%s Connected.\n",inet_ntoa(((struct sockaddr_in*)clientAddr)->sin_addr));
#ifdef NEED_PASSWD
        sendMsg(connfd,MSG_HELLO_PASSWD_INDICATE,'\0',1);
        for (i = 4; i >= 0; --i) {// authenticate part
            rtnVal = recvFix(connfd, (void*) buffer, MD5_LEN);
            if (rtnVal != MD5_LEN) {
                if (rtnVal == MD5_LEN + EAGAIN) {
                    sendMsg(connfd, MSG_ERROR_TIMEOUT);
                    throw InternalException(MSG_ERROR_TIMEOUT);
                } else {
                    throw InternalException(MSG_INTERNAL_CONNECTION_LOST);
                }
            }
            if (authenticate((unsigned char*) buffer)) {
                sendMsg(connfd,MSG_OK);
                break;
            } else {
                sendMsg(connfd, MSG_ERROR_LOGIN,'\0', i);
            }
        }
        if (i < 0) {
            throw InternalException(MSG_ERROR_TIMEOUT); // authenticate fail
        }
#else
        sendMsg(connfd,MSG_HELLO_IS_NEED_PASSWD,0);
#endif
        // prepare control part
        if (pthread_mutex_trylock(&serialMutex) == EBUSY) {
            sendMsg(connfd, MSG_ERROR_SERIAL_OCCUPY);
            throw InternalException(MSG_ERROR_SERIAL_OCCUPY);
        }else{
            if(((servoctl->getstatus())&(SERVO_LINKDOWN)) != 0){
                sendMsg(connfd,MSG_ERROR_SERIAL_OPEN_FAIL);
                throw InternalException(MSG_ERROR_SERIAL_OPEN_FAIL);
            }
        }
        while (true) {
            int rtnVal;
            int servostat;
            rtnVal = recvFix(connfd, buffer, 5);
            if (rtnVal == 5 + EAGAIN) {//Timeoout
                sendMsg(connfd, MSG_ERROR_TIMEOUT);
                throw InternalException(MSG_ERROR_TIMEOUT);
            } else if (rtnVal == 0) {
                throw InternalException(MSG_INTERNAL_CONNECTION_LOST);
            }

            switch (buffer[1]) {
            case INST_QUERY_TYPE:
                if ((buffer[3] != INST_AXIS_X) && (buffer[3] != INST_AXIS_Y)) {
                    sendMsg(connfd, MSG_ERROR_ILLEGAL_INST,'\0',buffer[3]);
                    break;
                }
                switch (buffer[2]) {
                case INST_BLAC: // Blance Deg
                    sendMsg(connfd, MSG_OK, buffer[3], servoctl->queryblance(buffer[3]));
                    break;
                case INST_APOS: // Absolute Postion
                    sendMsg(connfd, MSG_OK, buffer[3], servoctl->getpos(buffer[3]));
                    break;
                case INST_SPED: // Moving Speed
                    sendMsg(connfd, MSG_OK, buffer[3], servoctl->getspeed(buffer[3]));
                    break;
                case INST_MOVE: // Moving State
                    sendMsg(connfd, MSG_OK, buffer[3], servoctl->getmove(buffer[3]));
                    break;
                default:
                    sendMsg(connfd, MSG_ERROR_ILLEGAL_INST);
                    //TODO: when recv illegal inst 10 times,then throw InternalException(MSG_ERROR_ILLEGAL_INST);
                    break;
                }
                break;
            case INST_SET_TYPE:
                if ((buffer[3] != INST_AXIS_X) && (buffer[3] != INST_AXIS_Y)) {
                    sendMsg(connfd, MSG_ERROR_ILLEGAL_INST,'\0',buffer[3]);
                    break;
                }
                switch (buffer[2]) {
                case INST_BLAC: // Blance Deg
                    servoctl->changeblance(buffer[3], buffer[4]);
                    servostat = servoctl->getstatus();
                    if (servostat == 0) {
                        sendMsg(connfd, MSG_OK, buffer[3], buffer[4]);
                    } else {
                        sendMsg(connfd, MSG_WARNING_SERVO_UNSTABLE, '\0', servostat);
                    }
                    break;
                case INST_APOS: // Absolute Postion
                    servoctl->setpos(buffer[3], buffer[4]);
                    servostat = servoctl->getstatus();
                    if (servostat == 0) {
                        sendMsg(connfd, MSG_OK, buffer[3], buffer[4]);
                    } else {
                        sendMsg(connfd, MSG_WARNING_SERVO_UNSTABLE, '\0', servostat);
                    }
                    break;
                case INST_SPED: // Moving Speed
                    servoctl->setspeed(buffer[3], buffer[4]);
                    servostat = servoctl->getstatus();
                    if (servostat == 0) {
                        sendMsg(connfd, MSG_OK, buffer[3], buffer[4]);
                    } else {
                        sendMsg(connfd, MSG_WARNING_SERVO_UNSTABLE, '\0', servostat);
                    }
                    break;
                case INST_MOVE: // Moving State
                    if ((buffer[4] != SERVO_MOVE_DIRECTION_NEGA) &&
                            (buffer[4] != SERVO_MOVE_DIRECTION_POSE) &&
                            (buffer[4] != SERVO_MOVE_DIRECTION_STOP)) {
                        sendMsg(connfd, MSG_ERROR_ILLEGAL_INST, '\0', buffer[3]);
                    }
                    servoctl->setmove(buffer[3], buffer[4]);
                    ;
                    servostat = servoctl->getstatus();
                    if (servostat == 0) {
                        sendMsg(connfd, MSG_OK, buffer[3], buffer[4]);
                    } else {
                        sendMsg(connfd, MSG_WARNING_SERVO_UNSTABLE, '\0', servostat);
                    }
                    break;
                case INST_REST: // Reset
                    servoctl->reset();
                    sendMsg(connfd, MSG_OK);
                    break;
                default:
                    sendMsg(connfd, MSG_ERROR_ILLEGAL_INST);
                    //TODO: when recv illegal inst 10 times,then throw InternalException(MSG_ERROR_ILLEGAL_INST);
                    break;
                }
                break;
            case INST_KEEP_TYPE:
                servostat = servoctl->getstatus();
                if (servostat == 0) {
                    sendMsg(connfd, MSG_OK, buffer[3], buffer[4]);
                } else {
                    sendMsg(connfd, MSG_WARNING_SERVO_UNSTABLE, '\0', servostat);
                }
                break;
            case INST_DROP_TYPE:
                sendMsg(connfd, MSG_OK);
                throw InternalException(MSG_OK);
                break;
            default:
                sendMsg(connfd, MSG_ERROR_ILLEGAL_INST);
                //TODO: when recv illegal inst 10 times,then throw InternalException(MSG_ERROR_ILLEGAL_INST);
                break;
            }
        }
    } catch (InternalException &ex) {
        const char* quit_reason;
        switch (ex.reason) {
        case MSG_HELLO_PASSWD_INDICATE:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_HELLO_PASSWD_INDICATE;break;
        case MSG_ERROR_LOGIN:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_ERROR_LOGIN;break;
        case MSG_ERROR_SERIAL_OPEN_FAIL:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_ERROR_SERIAL_OPEN_FAIL;break;
        case MSG_ERROR_SERIAL_OCCUPY:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_ERROR_SERIAL_OCCUPY;break;
        case MSG_ERROR_TIMEOUT:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_ERROR_TIMEOUT;break;
        case MSG_ERROR_ILLEGAL_INST:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_ERROR_ILLEGAL_INST;break;
        case MSG_WARNING_SERVO_UNSTABLE:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_ERROR_SERVO_UNSTABLE;break;
        case MSG_INTERNAL_CONNECTION_LOST:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_INTERNAL_CONNECTION_LOST;break;
        case MSG_OK:quit_reason = CAM_CONTROL_DETAIL_STR_MSG_OK;break;
        default:quit_reason = CAM_CONTROL_DETAIL_STR_UNKNOW;break;
        }
        printTimeStampFirst();
        printf("IP:%s Disconnect,reason:%s\n",inet_ntoa(((struct sockaddr_in*)clientAddr)->sin_addr),quit_reason);
    }
    pthread_mutex_unlock(&serialMutex);
    shutdown(connfd, SHUT_RDWR);
}

int CamControlServer::afterSetup() {
    struct timeval timeout = {(CAM_CONTROL_LINK_TIMEOUT_MIN*60)+CAM_CONTROL_LINK_TIMEOUT_SEC, 0};
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*) &timeout, sizeof (timeout)))
        return NS_ERROR_AFTER_SET;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof (timeout)))
        return NS_ERROR_AFTER_SET;
    return NetworkServer::afterSetup();
}

//read fixed bytes from a fd, return nbytes for OK, if timeout return nbytes + EAGAIN, if closed return 0

size_t CamControlServer::recvFix(int fd, void* buf, size_t nbytes) {
    size_t cnt = 0;
    size_t readed;
    while (cnt < nbytes) {
        readed = recv(fd, ((char*)buf)+cnt, 1, 0);
        if (readed == 0) {
            if (errno == EAGAIN)
                return nbytes + EAGAIN;
            else
                return 0;
        }else if (readed == (size_t)-1) {
            if (errno == EAGAIN)
                return nbytes + EAGAIN;
            else
                return 0;
        }else
            cnt += readed;
    }
    if(ConfigManager::isDebug()){
        printTimeStampFirst();
        printf("CamControlServer::recvFix: captured, lenth: %u, data: ",nbytes);
        for(size_t i = 0;i < nbytes;++i){
            printf("%02X(%c), ",((unsigned char*)buf)[i],((unsigned char*)buf)[i]);
        }
        printf("\n");
    }
    return nbytes;
}

//send given event

void CamControlServer::sendMsg(int fd, ServerMsg type, unsigned char XY,unsigned char val) {
    unsigned char sendBuf[5];
    sendBuf[0] = 'S';
    sendBuf[4] = val;
    switch (type) {
    case MSG_HELLO_PASSWD_INDICATE:
        sendBuf[1] = 'H';
        sendBuf[2] = CAM_CONTROL_LINK_TIMEOUT_MIN;
        sendBuf[3] = CAM_CONTROL_LINK_TIMEOUT_SEC;
        break;
    case MSG_ERROR_LOGIN:
        sendBuf[1] = 'E';
        sendBuf[2] = 'L';
        sendBuf[3] = 'F';
        break;
    case MSG_ERROR_SERIAL_OPEN_FAIL:
        sendBuf[1] = 'E';
        sendBuf[2] = 'C';
        sendBuf[3] = 'F';
        break;
    case MSG_ERROR_SERIAL_OCCUPY:
        sendBuf[1] = 'E';
        sendBuf[2] = 'C';
        sendBuf[3] = 'O';
        break;
    case MSG_ERROR_TIMEOUT:
        sendBuf[1] = 'E';
        sendBuf[2] = 'T';
        sendBuf[3] = 'O';
        break;
    case MSG_ERROR_ILLEGAL_INST:
        sendBuf[1] = 'E';
        sendBuf[2] = 'I';
        sendBuf[3] = 'I';
        break;
    case MSG_WARNING_SERVO_UNSTABLE:
        sendBuf[1] = 'E';
        sendBuf[2] = 'S';
        sendBuf[3] = 'E';
        break;
    case MSG_OK:
        sendBuf[1] = 'O';
        sendBuf[2] = 'K';
        sendBuf[3] = XY;
        break;
    default:
        return;
        break;
    }
    send(fd, sendBuf, 5, 0);
    if(ConfigManager::isDebug()){
        printTimeStampFirst();
        printf("CamControlServer::sendMsg: captured, lenth: %d, data: ",5);
        for(int i = 0;i < 5;++i){
            printf("%02X(%c), ",sendBuf[i],sendBuf[i]);
        }
        printf("\n");
    }
}

bool CamControlServer::authenticate(const unsigned char* target) {
    time_t timep;
    int i;
    struct tm *p;
    unsigned char md5[16];

    time(&timep);
    p = gmtime(&timep);
    sprintf(originKeyBuffer, ORIGIN_KEY_FORMAT,
            ConfigManager::getLoginUser(),
            p->tm_year,
            p->tm_mon + 1,
            p->tm_mday,
            p->tm_wday + 1,
            p->tm_hour,
            ConfigManager::getLoginPass());
    MD5((unsigned char*) originKeyBuffer, originKeySize - 1, md5);

    for (i = 0; ((target[i] == md5[i]) && (i < 16)); ++i);
    if (i == 16) {
        printTimeStampFirst();
        printf("Authenticate succ, origin key:%s\n",
               originKeyBuffer);
        return true;
    } else {
        printTimeStampFirst();
        printf("Authenticate fail, origin key:%s\n",
               originKeyBuffer);
        return false;
    }
}
