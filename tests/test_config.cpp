#include "sylar/config.h"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::ConfigManager::LookUp("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::ConfigManager::LookUp("system.value", (float)10.2f, "system value");

int main() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_float_value_config->getValue();

    YAML::Node root =
        YAML::LoadFile("/mnt/d/WSL/mysylar/tests/test_config.yml");
    sylar::ConfigManager::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "after: " << g_float_value_config->getValue();

    return 0;
}