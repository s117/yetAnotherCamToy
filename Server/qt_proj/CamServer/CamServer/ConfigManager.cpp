#include "ConfigManager.h"

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {

}

int ConfigManager::getListenPort() {
    return 1736;
}

const char* ConfigManager::getLoginUser() {
    return "spartan";
}

const char* ConfigManager::getLoginPass() {
    return "nyaruko";
}

const char* ConfigManager::getLoginCipher() {
    return "masterchief";
}

const char* ConfigManager::getCOM() {
    return "/dev/ttyATH0";
}

int ConfigManager::getServoBlanceX() {
    return 89;
}

bool ConfigManager::isDebug(){
    return true;
}

int ConfigManager::getServoBlanceY() {
    return 89;
}

void ConfigManager::setServoBlanceX(int newblance) {
    return;
}

void ConfigManager::setServoBlanceY(int newblance) {
    return;
}
