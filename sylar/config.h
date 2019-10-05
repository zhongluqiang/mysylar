#ifndef MYSYLAR_CONFIG_H
#define MYSYLAR_CONFIG_H

#include "sylar/log.h"
#include <boost/lexical_cast.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
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
    virtual std::string getTypeName() const         = 0;

protected:
    std::string m_name;
    std::string m_description;
};

//偏特化起始类
template <class F, class T> class LexicalCast {
public:
    T operator()(const F &v) { return boost::lexical_cast<T>(v); }
};

// vector偏特化，实现将YAML格式的string转化为vector
template <class T> class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

// vector偏特化，实现将vector转化为YAML格式的string
template <class T> class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node;
        for (auto &i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//具体的配置项，定义成模板类，以适应不同的配置类型
template <class T, class FromStr = LexicalCast<std::string, T>,
          class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;

    ConfigVar(const std::string &name, const T &default_value,
              const std::string &description = "")
        : ConfigVarBase(name, description)
        , m_val(default_value) {}

    std::string toString() override {
        try {
            // return boost::lexical_cast<std::string>(m_val);
            return ToStr()(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << "ConfigVar::toString exception" << e.what()
                << "convert: " << typeid(m_val).name() << "to string";
        }

        return "";
    }

    bool fromString(const std::string &val) override {
        try {
            // m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
            return true;
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
                << "ConfigVar::fromString exception" << e.what()
                << "convert string: " << val << " to " << typeid(m_val).name();
        }
        return false;
    }

    std::string getTypeName() const override { return typeid(T).name(); }
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