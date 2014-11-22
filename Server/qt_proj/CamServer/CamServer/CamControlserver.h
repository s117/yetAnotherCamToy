#ifndef CAMCONTROLSERVER_H
#define CAMCONTROLSERVER_H
#include <time.h>
#include "NetworkServer.h"
#include "SerialPort.h"
#include "CntServoControl.h"

#define BUFFER_SIZE 1024
#define CAM_CONTROL_LINK_TIMEOUT_MIN 0
#define CAM_CONTROL_LINK_TIMEOUT_SEC 60

#define CAM_CONTROL_DETAIL_STR_MSG_HELLO_PASSWD_INDICATE    "MSG_HELLO_PASSWD_INDICATE"
#define CAM_CONTROL_DETAIL_STR_MSG_ERROR_LOGIN              "Authenticate Fail"
#define CAM_CONTROL_DETAIL_STR_MSG_ERROR_SERIAL_OPEN_FAIL   "Cannot open serial port"
#define CAM_CONTROL_DETAIL_STR_MSG_ERROR_SERIAL_OCCUPY      "Serial port is locked by other user"
#define CAM_CONTROL_DETAIL_STR_MSG_ERROR_TIMEOUT            "Link timeout"
#define CAM_CONTROL_DETAIL_STR_MSG_ERROR_ILLEGAL_INST       "Illegal instruction recieved"
#define CAM_CONTROL_DETAIL_STR_MSG_ERROR_SERVO_UNSTABLE     "Servo's data link unstable"
#define CAM_CONTROL_DETAIL_STR_MSG_INTERNAL_CONNECTION_LOST "Client disconnected"
#define CAM_CONTROL_DETAIL_STR_MSG_OK                       "Client drop the connection"
#define CAM_CONTROL_DETAIL_STR_UNKNOW                       "Unknow"

enum ServerMsg {
    MSG_HELLO_PASSWD_INDICATE,
    MSG_ERROR_LOGIN,
    MSG_ERROR_SERIAL_OPEN_FAIL,
    MSG_ERROR_SERIAL_OCCUPY,
    MSG_ERROR_TIMEOUT,
    MSG_ERROR_ILLEGAL_INST,
    MSG_WARNING_SERVO_UNSTABLE,
    MSG_INTERNAL_CONNECTION_LOST,

    MSG_OK
};

class CamControlServer : public NetworkServer {
public:
    CamControlServer(int numThread);
    ~CamControlServer();
    virtual int afterSetup();
    virtual void innerAction(int connfd, struct sockaddr *clientAddr);
    size_t recvFix(int fd, void* buf, size_t nbytes);
    void sendMsg(int fd, ServerMsg type, unsigned char val = '\0', unsigned char XY = '\0');
    bool authenticate(const unsigned char* target);
private:
    int originKeySize;
    char *originKeyBuffer;
    pthread_mutex_t serialMutex;
    ServoControl *servoctl;
    SerialPort *serialPort;
};

#endif // CAMCONTROLSERVER_H
