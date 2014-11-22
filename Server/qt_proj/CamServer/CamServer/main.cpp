#include "CamControlserver.h"
#include "CntServoControl.h"
#include "ConfigManager.h"
#include <signal.h>


#define MYPORT  1112
#define QUEUE   20
#define BUFFER_SIZE 1024

int main() {
    signal(SIGPIPE,SIG_IGN);
    CamControlServer *server = new CamControlServer(2);
    int startCode;
    if((startCode = server->Listen(INADDR_ANY,ConfigManager::getListenPort())) != NS_ERROR_OK) {
        printf("Server start fail, Errno: %d\n",startCode);
        return 1;
    } else {
        printf("Server running...\n");
        while(getchar() != 'q');
        printf("Server stoping...\n");
        printf("Server stop with %d\n",server->Stop());
        return 0;
    }
}
