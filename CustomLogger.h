#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

/// <summary>
/// 日志模块,使用spdlog 1.x版本，
/// 支持QT
/// </summary>
class CustomLogger
{

public:
    double verion = 0.90;
#if CONSOLE_MODE == false
    GUIUpdater* updater;
    const char* strbuf = {0}; //如果需要缓存可使用
#endif

private:
    template<typename... Args>
    using format_string_t = fmt::format_string<Args...>;
    spdlog::logger* logger;
    std::string formattedString;
    bool verbose;

public:
    CustomLogger(CustomLogger const&) = delete;
    void operator=(CustomLogger const&) = delete;
    //如果需要同步更新图形界面需要传入更新器(GUIUpdater)指针
    static CustomLogger& get(void* updater = NULL) {
        static CustomLogger instance(updater);
        return instance;
    }

    //注意实际上调用Debug
    template<typename... Args> void trace(format_string_t<Args...> fmt, Args &&... args) {
        logger->trace(fmt, std::forward<Args>(args)...);
        #if UPDATE_QT_MESSAGE == true
        if (updater == NULL) { return; }
        if (!verbose) { return; }
        std::string message = "[追踪] "+fmt::format(fmt, std::forward<Args>(args)...);
        updater->updateMessage(message.c_str());
        #endif
    }

    template<typename... Args> void debug(format_string_t<Args...> fmt, Args &&... args) {
        logger->debug(fmt, std::forward<Args>(args)...);
#if UPDATE_QT_MESSAGE == true
        if (updater == NULL) { return; }
        if (!verbose) { return; }
        std::string message = "[DEBUG] " + fmt::format(fmt, std::forward<Args>(args)...);
        updater->updateMessage(message.c_str());
#endif
    }

    template<typename... Args> void info(format_string_t<Args...> fmt, Args &&... args){
        logger->info(fmt, std::forward<Args>(args)...);
        #if UPDATE_QT_MESSAGE == true 
        if (updater == NULL) { return; }
        std::string message = "[消息] " + fmt::format(fmt, std::forward<Args>(args)...);
        updater->updateMessage(message.c_str());
        #endif
    }

    template<typename... Args> void warn(format_string_t<Args...> fmt, Args &&... args){
        logger->warn(fmt, std::forward<Args>(args)...);
        #if UPDATE_QT_MESSAGE == true 
        if (updater == NULL) { return; }
        std::string message = "[警告] " + fmt::format(fmt, std::forward<Args>(args)...);
        updater->updateMessage(message.c_str());
        #endif
    }

    template<typename... Args> void error(format_string_t<Args...> fmt, Args &&... args){
        logger->error(fmt, std::forward<Args>(args)...);
        #if UPDATE_QT_MESSAGE == true 
        if (updater == NULL) { return; }
        std::string message = "[错误] " + fmt::format(fmt, std::forward<Args>(args)...);
        updater->updateMessage(message.c_str());
        #endif
    }

private:
    CustomLogger(void* updater = NULL, bool verbose = false) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/traceback.txt", true);
#if CONSOLE_MODE == true
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        logger = new spdlog::logger("log", { file_sink, console_sink });
#else   
        logger = new spdlog::logger("log", { file_sink });
        this->updater = (GUIUpdater*)updater;
#endif
        if (verbose) {
            //TODO 未设置文件和控制台分别使用不同等级
            logger->set_level(spdlog::level::trace);
        }
    }

};

