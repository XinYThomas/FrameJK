//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_CONFLOAD_H
#define FRAME_CONFLOAD_H

#include <sstream>
#include <string>
#include <vector>
#include <Tool.h>
#include "fstream"
#include <cjson/cJSON.h>

using namespace std;

class ConfLoad {
protected:
    cJSON* conf_cjson_;
    static ConfLoad* conf_load_;
    static std::string conf_dir_;
    // 获取对应的节点
    cJSON* get_node(string &key) const {
        vector<string> keys;
        spilt_string(key, ".", keys);
        if (keys.empty()) {
            return nullptr;
        }
        cJSON* section_obj = conf_cjson_;
        for (const auto & key_unit : keys) {
            section_obj= cJSON_GetObjectItem(section_obj, key_unit.c_str());
            if (!section_obj) {
                return nullptr;
            }
        }
        return section_obj;
    }

    // 加载配置
    explicit ConfLoad() {
        const ifstream file(conf_dir_);
        if (!file.is_open()) {
            // 日志
            throw runtime_error("加载失败");
        }
        stringstream buffer;
        buffer << file.rdbuf();
        string content = buffer.str();
        conf_cjson_ = cJSON_Parse(content.c_str());
        if (conf_cjson_ == nullptr) {
            throw runtime_error("加载失败");
        }
    }

    // 释放内存
    void freeMemory() {
        if (conf_cjson_ != nullptr) {
            cJSON_Delete(conf_cjson_);
            conf_cjson_ = nullptr;
        }
    }

public:


    ConfLoad(ConfLoad &other) = delete;
    void operator=(const ConfLoad &) = delete;

    template <typename T>
    T get_value(string key) {
        // 需要特化实现
        static_assert(sizeof(T) == 0, "Unsupported type");
        return T();
    }

    static ConfLoad *GetInstance() {
        if (conf_load_ == nullptr) {
            conf_load_ = new ConfLoad();
        }
        return conf_load_;
    }

    ~ConfLoad() {
        freeMemory();
    }
};

// 特化实现 string 类型
template<>
inline std::string ConfLoad::get_value(string key) {
    const cJSON* node = get_node(key);
    if (!node || !cJSON_IsString(node)) return "";
    return node->valuestring;
}

// 特化实现 int 类型
template<>
inline int ConfLoad::get_value(std::string key) {
    const cJSON* node = get_node(key);
    if (!node || !cJSON_IsNumber(node)) return 0;
    return node->valueint;
}

// 特化实现 bool 类型
template<>
inline bool ConfLoad::get_value(std::string key) {
    const cJSON* node = get_node(key);
    if (!node || !cJSON_IsBool(node)) return false;
    return cJSON_IsTrue(node);
}

// 特化实现 double 类型
template<>
inline double ConfLoad::get_value(std::string key) {
    const cJSON* node = get_node(key);
    if (!node || !cJSON_IsNumber(node)) return 0.0;
    return node->valuedouble;
}

#endif //FRAME_CONFLOAD_H