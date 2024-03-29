#pragma once

#include <exception>
#include <iostream>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <algorithm>
#include <list>
#include <vector>
#include <yaml-cpp/yaml.h>

#include "config/Lexical_Cast.hpp"
#include "latch/lock.hpp"

namespace zhuyh
{
    class IConfigVar
    {
    public:
        typedef std::shared_ptr<IConfigVar> ptr;
        IConfigVar(const std::string &varName, const std::string &varDesc)
            : s_varName(varName),
              s_varDesc(varDesc)
        {
            std::transform(s_varName.begin(), s_varName.end(), s_varName.begin(), ::tolower);
        }
        virtual ~IConfigVar() {}
        const std::string &getVarName() const { return s_varName; }
        const std::string &getVarDesc() const { return s_varDesc; }

        virtual bool fromStr(const std::string &val) = 0;
        virtual std::string toStr() = 0;
        virtual std::string getVarType() const = 0;

    protected:
        std::string s_varName;
        std::string s_varDesc;
    };

    /*
   *@brief 从std::string --> std::vector<T>
   */
    template <class T>
    class Lexical_Cast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(Lexical_Cast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /*
   *@brief std::vector<F> --> std::string
   */
    template <class F>
    class Lexical_Cast<std::vector<F>, std::string>
    {
    public:
        std::string operator()(const std::vector<F> &v)
        {
            //创建一个序列
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &val : v)
            {
                node.push_back(YAML::Load(Lexical_Cast<F, std::string>()(val)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /*
   *@brief std::string --> std::list<T>
   */
    template <class T>
    class Lexical_Cast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::stringstream ss;
            std::vector<T> vec;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                vec.push_back(Lexical_Cast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /*
   *@brief std::list<F> --> std::string
   */
    template <class F>
    class Lexical_Cast<std::list<F>, std::string>
    {
    public:
        std::string operator()(const std::list<F> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &val : v)
            {
                node.push_back(YAML::Load(Lexical_Cast<F, std::string>()(val)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /*
   *@brief std::string --> std::map<std::string,T>
   */
    template <class T>
    class Lexical_Cast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::stringstream ss;
            std::map<std::string, T> mp;
            for (auto it = node.begin();
                 it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                mp.insert(std::make_pair(it->first.Scalar(),
                                         Lexical_Cast<std::string, T>()(ss.str())));
            }
            return mp;
        }
    };

    /*
   *@brief std::map<std::string,T> --> std::string
   */

    template <class F>
    class Lexical_Cast<std::map<std::string, F>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, F> &v)
        {
            //创建一个类型的Map的节点
            YAML::Node node(YAML::NodeType::Map);
            for (auto &val : v)
            {
                node[val.first] = YAML::Load(Lexical_Cast<F, std::string>()(val.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /*
   *@brief std::string --> std::unordered_map<std::string,T>
   */
    template <class T>
    class Lexical_Cast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::stringstream ss;
            std::unordered_map<std::string, T> mp;
            for (auto it = node.begin();
                 it != node.end(); it++)
            {
                ss.str("");
                ss << it->second;
                mp.insert(std::make_pair(it->first.Scalar(),
                                         Lexical_Cast<std::string, T>()(ss.str())));
            }
            return mp;
        }
    };

    /*
   *@brief std::unordered_map<std::string,T> --> std::string
   */
    template <class F>
    class Lexical_Cast<std::unordered_map<std::string, F>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, F> &v)
        {
            //创建一个类型的Map的节点
            YAML::Node node(YAML::NodeType::Map);
            for (auto &val : v)
            {
                node[val.first] = YAML::Load(Lexical_Cast<F, std::string>()(val.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class Lexical_Cast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::stringstream ss;
            std::set<T> s;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                s.insert(Lexical_Cast<std::string, T>()(ss.str()));
            }
            return s;
        }
    };

    template <class F>
    class Lexical_Cast<std::set<F>, std::string>
    {
    public:
        std::string operator()(const std::set<F> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &val : v)
            {
                node.push_back(YAML::Load(Lexical_Cast<F, std::string>()(val)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T>
    class Lexical_Cast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            std::stringstream ss;
            std::unordered_set<T> s;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str("");
                ss << node[i];
                s.insert(Lexical_Cast<std::string, T>()(ss.str()));
            }
            return s;
        }
    };

    template <class F>
    class Lexical_Cast<std::unordered_set<F>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<F> &v)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (auto &val : v)
            {
                node.push_back(YAML::Load(Lexical_Cast<F, std::string>()(val)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <class T, class FromStr = Lexical_Cast<std::string, T>,
              class ToStr = Lexical_Cast<T, std::string>>
    class ConfigVar : public IConfigVar
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &oldVal, const T &newVal)> on_change_cb;
        ConfigVar(const std::string &varName, const std::string &varDesc,
                  const T &varVal)
            : IConfigVar(varName, varDesc),
              t_varVal(varVal)
        {
        }

        T getVar() const
        {
            return t_varVal;
            ;
        }

        uint64_t addCb(on_change_cb cb)
        {
            LockGuard lg(lk);
            ++u_funcId;
            cbs[u_funcId] = cb;
            return u_funcId;
        }

        void delCb(uint64_t id)
        {
            LockGuard lg(lk);
            auto it = cbs.find(id);
            if (it != cbs.end())
            {
                cbs.erase(it);
            }
        }

        on_change_cb getCb(uint64_t id)
        {
            LockGuard lg(lk);
            auto it = cbs.find(id);
            return it == cbs.end() ? nullptr : it->second;
        }

        void clearCbs()
        {
            LockGuard lg(lk);
            cbs.clear();
        }

        void setVal(const T &v)
        {
            LockGuard lg(lk);
            if (t_varVal == v)
                return;
            for (auto &f : cbs)
            {
                f.second(t_varVal, v);
            }
            t_varVal = v;
        }

        bool fromStr(const std::string &v) override
        {
            try
            {
                setVal(FromStr()(v));
                return true;
            }
            catch (std::exception &e)
            {
                std::cout << "exception:" << e.what() << " Failed to convert string:\" "
                          << v << "\" to type" << getVarType()
                          << " Value_name : " << s_varName << std::endl;
            }
            return false;
        }

        std::string toStr() override
        {
            try
            {
                return ToStr()(t_varVal);
            }
            catch (std::exception &e)
            {
                std::cout << "exception:" << e.what() << " Failed to convert type:\" "
                          << getVarType() << " to string"
                          << "Value_name : " << s_varName << std::endl;
            }
            return "";
        }
        //TODO:demangle
        std::string getVarType() const override
        {
            return typeid(T).name();
        }

    private:
        SpinLock lk;
        //修改回调函数,key --> callback
        std::map<uint64_t, on_change_cb> cbs;
        //值
        T t_varVal;
        //id
        uint64_t u_funcId = 0;
    };

    class Config
    {
    public:
        typedef std::unordered_map<std::string, IConfigVar::ptr> ConfigVarMap;
        /*
     *@brief 查找,查找不到则初始化
     *@exception 出现非法字符抛出非法参数异常
     */
        template <class T>
        static typename ConfigVar<T>::ptr lookUp(const std::string &varName,
                                                 const T &varDftVal,
                                                 const std::string &varDesc)
        {
            LockGuard lg(lk());
            auto &mp = getVarMap();
            auto it = mp.find(varName);
            if (it != mp.end())
            {
                // Android doesn't Not support such cast
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                //可以转换
                if (tmp)
                    return tmp;
                //不可以转换
                std::cout << "LookUp Name: " << varName << " exist but type not "
                          << typeid(T).name() << " real_type :" << it->second->getVarType()
                          << std::endl;
                return nullptr;
            }
            std::string name = varName;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            //unsigned int p;
            //出现非法字符
            if ((name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")) != std::string::npos)
            {
                std::cout << "invalid varName: " << name << " character : " << std::endl;
                throw std::invalid_argument(name);
            }
            typename ConfigVar<T>::ptr v = std::make_shared<ConfigVar<T>>(name, varDesc, varDftVal);
            mp.insert(std::make_pair(name, v));
            return v;
        }

        /*
     *@brief 查找varName,找不到返回空
     */

        template <class T>
        static typename ConfigVar<T>::ptr lookUp(const std::string &varName)
        {
            auto &mp = getVarMap();
            auto it = mp.find(varName);
            if (it == mp.end())
            {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }
        static bool listAllItem(const std::string &prefix,
                                const YAML::Node &node,
                                std::list<std::pair<std::string, YAML::Node>> &out);
        static IConfigVar::ptr lookUpBase(const std::string &varName);
        static bool loadFromYamlNode(YAML::Node &node);
        static bool loadFromYamlFile(const std::string &path, bool force = false);

    private:
        static SpinLock &lk()
        {
            static SpinLock m_lk;
            return m_lk;
        }
        static ConfigVarMap &getVarMap()
        {
            static ConfigVarMap mp;
            return mp;
        }
    };

}
