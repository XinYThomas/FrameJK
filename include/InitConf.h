//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_INITCONF_H
#define FRAME_INITCONF_H

#include <ConfLoad.h>
#include <Logger.h>
#include <string>

using namespace std;
// 声明
void InitConf();
void InitLogger();


inline void InitConf() {
    InitLogger();
}

inline void InitLogger() {
    auto* conf_ = new LoggerConfig;
    ConfLoad* conf_load_ = ConfLoad::GetInstance();
    conf_->logPath = conf_load_->get_value<string>("logger.logdir");
    conf_->minLogLevel = stringToLogLevel(
        conf_load_->get_value<string>("logger.minLevel")
    );
    conf_->asyncMode = conf_load_->get_value<bool>("logger.asyncMode");
    conf_->consoleOutput = conf_load_->get_value<bool>("logger.consoleOutput");
    conf_->maxFiles = conf_load_->get_value<int>("logger.maxFiles");
    conf_->maxFileSize = conf_load_->get_value<int>("logger.maxFileSize") * 1024;
    SpdLogger::set_config(conf_);
}

#endif //FRAME_INITCONF_H