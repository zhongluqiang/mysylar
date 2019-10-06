#include "sylar/config.h"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::ConfigManager::LookUp("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::ConfigManager::LookUp("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    sylar::ConfigManager::LookUp("system.int_vec", std::vector<int>{1, 2},
                                 "system int vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    sylar::ConfigManager::LookUp("system.int_list", std::list<int>{1, 2},
                                 "system int list");

int main() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "before: " << g_float_value_config->getValue();

    for (auto &i : g_int_vec_value_config->getValue()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << i;
    }

    for (auto &i : g_int_list_value_config->getValue()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << i;
    }

    YAML::Node root =
        YAML::LoadFile("/mnt/d/WSL/mysylar/tests/test_config.yml");
    sylar::ConfigManager::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
        << "after: " << g_float_value_config->getValue();

    for (auto &i : g_int_vec_value_config->getValue()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << i;
    }

    for (auto &i : g_int_list_value_config->getValue()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << i;
    }

    return 0;
}