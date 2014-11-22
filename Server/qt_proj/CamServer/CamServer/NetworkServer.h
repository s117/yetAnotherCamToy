#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <stack>
#include <list>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nullptr.h"
#include "PThreadPool.h"

#define NS_ERROR_OK             0x00000000
#define NS_ERROR_REQ_SOCKET     0x00000001
#define NS_ERROR_BIND           0x00000002
#define NS_ERROR_LISTEN         0x00000003
#define NS_ERROR_ACCEPT         0x00000004
#define NS_ERROR_MEMOUT         0x00000005
#define NS_ERROR_ILLEGAL_STATE  0x00000006
#define NS_ERROR_AFTER_SET      0x00000007
#define NS_ERROR_THREAD_CREATE  0x00000101
#define NS_ERROR_THREAD_DESTORY 0x00000102

class WorkingPool : public PThreadPool {
public:
    WorkingPool(int maxThread);
    virtual void innerAction(void* args);
};

class NetworkServer;

typedef struct _ConnectionDesc {
    NetworkServer *server;
    int linkfd;
    struct sockaddr client_addr;
} ConnectionDesc;

class NetworkServer {
public:
    NetworkServer(int maxThread);
    ~NetworkServer();
    virtual int Listen(in_addr_t addr, int port);
    virtual int afterSetup();

    virtual int Stop();
    virtual bool isListening();
    virtual struct sockaddr_in getSockAddrIn();
    virtual int getSockFd();
    virtual PThreadPool* getThPool();
    virtual void innerAction(int connfd, struct sockaddr *clientAddr) = 0;
protected:
    int numThread;
    PThreadPool *txPool;
    struct sockaddr_in sockParam;
    int sockfd;
    volatile bool isWorking;
    pthread_t listenThread;
    friend class WorkingPool;
};

#endif // NETWORKSERVER_H
