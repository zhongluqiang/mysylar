#include <iostream>
#include <yaml-cpp/yaml.h>

// YAML结点类型定义
// struct NodeType {
//    enum value { Undefined, Null, Scalar, Sequence, Map };
//};
void print_yaml(const YAML::Node &node, int level) {
    if (node.IsScalar()) {
        std::cout << "isScalar" << std::endl;
        std::cout << std::string(level * 4, ' ') << node.Scalar() << std::endl;
    } else if (node.IsNull()) {
        std::cout << "isNull" << std::endl;
        std::cout << std::string(level * 4, ' ') << "NULL" << std::endl;
    } else if (node.IsMap()) {
        std::cout << "isMap" << std::endl;
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::cout << std::string(level * 4, ' ') << it->first << std::endl;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        std::cout << "isSequence" << std::endl;
        for (size_t i = 0; i < node.size(); ++i) {
            std::cout << std::string(level * 4, ' ') << i << std::endl;
            print_yaml(node[i], level + 1);
        }
    }
}

int main() {
    YAML::Node root = YAML::LoadFile("/mnt/d/WSL/mysylar/tests/test.yml");
    print_yaml(root, 0);
    return 0;
}