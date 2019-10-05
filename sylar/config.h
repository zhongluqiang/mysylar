#ifndef MYSYLAR_CONFIG_H
#define MYSYLAR_CONFIG_H

#include "sylar/log.h"
#include <boost/lexical_cast.hpp>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>

namespace sylar {

//配置项基类，每项配置都包含名称及描述
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    ConfigVarBase(const std::string &name, const std::string &description = "")
        : m_name(name)
        , m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string &getName() const { return m_name; }
    const std::string &getDescription() const { return m_description; }

    virtual std::string toString()                  = 0;
    virtual bool fromString(const std::string &val) = 0;

protected:
    std::string m_name;
    std::string m_description;
};

//具体的配置项，定义成模板类，以适应不同的配置类型
template <class T> class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    ConfigVar(const std::string &name, const T &default_value,
              const std::string &description = "")
        : ConfigVarBase(name, description)
        , m_val(default_value) {}

    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << "ConfigVar::toString exception" << e.what()
                << "convert: " << typeid(m_val).name() << "to string";
        }

        return "";
    }

    bool fromString(const std::string &val) override {
        try {
            m_val = boost::lexical_cast<T>(val);
            return true;
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << "ConfigVar::fromString exception" << e.what()
                << "convert: string to " << typeid(m_val).name();
        }
        return false;
    }

    const T getValue() const { return m_val; }
    void setValue(const T &v) { m_val = v; }

private:
    T m_val;
};

//配置管理类，实现从YAML文件加载配置及查询某个具体的配置
class ConfigManager {
public:
    //配置名称-配置值
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    //根据配置名称查询某个配置的值
    template <class T>
    static typename ConfigVar<T>::ptr LookUp(const std::string &name) {
        auto it = s_configs.find(name);
        if (it == s_configs.end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    //查询某项配置，如果未找到，则使用默认值
    template <class T>
    static typename ConfigVar<T>::ptr
    LookUp(const std::string &name, const T &default_value,
           const std::string &description = "") {
        auto tmp = LookUp<T>(name);
        if (tmp) {
            return tmp;
        }
        //配置不存在，则新增一个配置，使用默认值
        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") !=
            std::string::npos) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "LookUp name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(
            new ConfigVar<T>(name, default_value, description));
        s_configs[name] = v;

        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())
            << "add config: " << name << ", default value: " << v->toString();

        return v;
    }

    static void LoadFromYaml(const YAML::Node &root);
    static ConfigVarBase::ptr LookupBase(const std::string &name);

private:
    static ConfigVarMap s_configs;
};

} // namespace sylar
#endif