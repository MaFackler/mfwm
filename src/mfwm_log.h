#pragma once
#define SPDLOG_COMPILED_LIB
#include "spdlog/spdlog.h"
//#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"


#define INFO spdlog::info
#define ERROR spdlog::error

void mfwm_log_init() {
    //auto logger = spdlog::basic_logger_mt("file_logger", "mfwm.log", true);
    auto logger = spdlog::rotating_logger_mt("file_logger", "mfwm.log", 1024 * 1024, 1);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(1));
}


