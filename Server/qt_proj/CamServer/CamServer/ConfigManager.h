#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    static int getListenPort();
    static const char* getLoginUser();
    static const char* getLoginPass();
    static const char* getLoginCipher();
    static const char* getCOM();
    static int getServoBlanceX();
    static int getServoBlanceY();
    static void setServoBlanceX(int newblance);
    static void setServoBlanceY(int newblance);
    static bool isDebug();
};

#endif // CONFIGMANAGER_H
