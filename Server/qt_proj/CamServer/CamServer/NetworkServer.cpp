#include "NetworkServer.h"
#include <unistd.h>
#include <stdlib.h>

#define WAIT_QUEUE_LEN 20

struct __thread_control_block {
    int sockdf;
    int stop;
};

WorkingPool::WorkingPool(int maxThread) : PThreadPool(maxThread) {
}

void WorkingPool::innerAction(void* args) {
    ConnectionDesc *conn = (ConnectionDesc*) args;
    conn->server->innerAction(conn->linkfd, &conn->client_addr);
    free(conn);
}

static void* listenRoutine(void* args) {
    NetworkServer *server = (NetworkServer*) args;
    int sockfd = server->getSockFd();
    PThreadPool *pool = server->getThPool();
    ConnectionDesc *conn;
    socklen_t length;

    conn = (ConnectionDesc*) malloc(sizeof (ConnectionDesc));
    conn->server = server;

    while (server->isListening()) {
        length = sizeof (struct sockaddr_in);
        conn->linkfd = accept(sockfd, &conn->client_addr, &length);
        if (!(conn->linkfd < 0)) {
            pool->DispatchJobs(conn);
            conn = (ConnectionDesc*) malloc(sizeof (ConnectionDesc));
            conn->server = server;
        }
    }
    return nullptr;
}




//maxThread mean how many thread in thread pool

NetworkServer::NetworkServer(int maxThread) {
    numThread = maxThread;
    isWorking = false;
    txPool = nullptr;
    sockfd = -1;
    listenThread = 0;
}

NetworkServer::~NetworkServer() {
    Stop();
}

//This method start listening a given port, if server already run, it will invoke Stop() first

int NetworkServer::Listen(in_addr_t addr, int port) {
    int stopRtn;
    if ((stopRtn = Stop()) != NS_ERROR_OK)
        return stopRtn;

    isWorking = true;
    this->sockParam.sin_family = AF_INET;
    this->sockParam.sin_port = htons(port);
    this->sockParam.sin_addr.s_addr = htonl(addr);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return NS_ERROR_REQ_SOCKET;
    if (bind(sockfd, (struct sockaddr *) &sockParam, sizeof (struct sockaddr)) == -1)
        return NS_ERROR_BIND;
    if (listen(sockfd, WAIT_QUEUE_LEN) == -1)
        return NS_ERROR_LISTEN;
    if ((txPool = new WorkingPool(numThread)) == nullptr)
        return NS_ERROR_MEMOUT;
    if (txPool->InitThreadPool() != PTP_RETURN_OK)
        return NS_ERROR_THREAD_CREATE;
    if (pthread_create(&listenThread, NULL, listenRoutine, this))
        return NS_ERROR_THREAD_CREATE;
    return afterSetup();
}

int NetworkServer::afterSetup() {
    return NS_ERROR_OK;
}

//Stop server and release all resource takes

int NetworkServer::Stop() {
    void* rtnVal;
    isWorking = false;
    if (sockfd != -1) {
        shutdown(sockfd, SHUT_RDWR);
        sockfd = -1;
    }
    usleep(200);
    if (txPool != nullptr) {
        if (txPool->ReleaseThreadPool(PTP_WAIT_TIME_INFINITE) != PTP_RETURN_RELEASE_NORMAL)
            return NS_ERROR_THREAD_DESTORY;
        delete txPool;
        txPool = nullptr;
    }
    if(listenThread != 0){
        pthread_cancel(listenThread);
        pthread_join(listenThread, &rtnVal);
        listenThread = 0;
    }

    return NS_ERROR_OK;
}

bool NetworkServer::isListening() {
    return isWorking;
}

struct sockaddr_in NetworkServer::getSockAddrIn() {
    return sockParam;
}

int NetworkServer::getSockFd() {
    return sockfd;
}

PThreadPool* NetworkServer::getThPool() {
    return txPool;
}
