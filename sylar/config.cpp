#include "sylar/config.h"

namespace sylar {

ConfigManager::ConfigVarMap ConfigManager::s_configs;

ConfigVarBase::ptr ConfigManager::LookupBase(const std::string &name) {
    auto it = s_configs.find(name);
    return it == s_configs.end() ? nullptr : it->second;
}

//提取所有的YAML结点
static void
ListAllMembers(const std::string &prefix, const YAML::Node &node,
               std::list<std::pair<std::string, const YAML::Node>> &output) {
    if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") !=
        std::string::npos) {
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
            << "Invalid config name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMembers(prefix.empty() ? it->first.Scalar()
                                          : prefix + "." + it->first.Scalar(),
                           it->second, output);
        }
    }
}

void ConfigManager::LoadFromYaml(const YAML::Node &root) {
    std::list<std::pair<std::string, const YAML::Node>> all_nodes;
    ListAllMembers("", root, all_nodes);

    for (auto &i : all_nodes) {
        std::string key = i.first;
        if (key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);
        if (var) {
            if (i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

} // namespace sylar