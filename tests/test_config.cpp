#include "sylar/config.h"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::ConfigManager::LookUp("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::ConfigManager::LookUp("system.value", (float)10.2f, "system value");

int main() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_float_value_config->getValue();

    return 0;
}