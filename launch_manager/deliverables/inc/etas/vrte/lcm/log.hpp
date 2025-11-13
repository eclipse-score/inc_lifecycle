/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
// (c) 2025 ETAS GmbH. All rights reserved.

#ifndef LCM_LOG_HPP_INCLUDED
#define LCM_LOG_HPP_INCLUDED

// Compile time switch to use different logging subsystems.
// Parts of LM code will be compiled into different binaries, think IPC between LifecycleClient and LM daemon.
// In this situation, this code will need to inherit logging mechanism of the binary file.

#ifdef LC_LOG_SCORE_MW_LOG

#include "score/mw/log/logger.h"

namespace etas {

namespace vrte {

namespace lcm {

/// @brief Function to access global logging context, for Launch Manager.
/// Launch Manager (LM) daemon, uses a single global logging context.
/// This context is stored as a static variable inside this function and used all over LM daemon implementation.
/// Please note that code should not call this function directly, but should use a set of wrapper macros.
/// More information can be found in docs/architecture/concepts/logging/logging.rst file.
inline score::mw::log::Logger& _getLmLogger() noexcept {
    // RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "This is safe because the static is a function local.", true);
    static score::mw::log::Logger& log{score::mw::log::CreateLogger("LM", "Launch Manager logging context")};
    return log;
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#else  // LC_LOG_SCORE_MW_LOG

// The only other solution supported is console logging.
#include <iostream>
#include <atomic>
#include <ctime>
#include <cstdlib>
#include <string>

namespace etas {

namespace vrte {

namespace lcm {

enum class LogLevel
{
    kFatal = 0,
    kError = 1,
    kWarn = 2,
    kInfo = 3,
    kDebug = 4,
    kVerbose = 5,
};

inline LogLevel GetLevelFromEnv() {
    if (const char* levelStr = std::getenv("LC_STDOUT_LOG_LEVEL")) {
        std::string_view levelSv{levelStr};
        int logLevelTmp;
        try {
            logLevelTmp = std::stoi(levelSv.data());
         }catch(...) {
            return LogLevel::kInfo;
         }

         if(logLevelTmp >= static_cast<int>(LogLevel::kFatal) && logLevelTmp  <= static_cast<int>(LogLevel::kVerbose)) {
            return LogLevel(logLevelTmp);
         } else {
            return LogLevel::kInfo;
         }

    } else {
        return LogLevel::kInfo;
    }
}

static LogLevel GetLevel() {
    const static LogLevel logLevel = GetLevelFromEnv();
    return logLevel;
}

inline std::ostream& operator<<(std::ostream& os, const std::tm* now ) {
    std::cout << (now->tm_year + 1900) << '/'
         << (now->tm_mon + 1) << '/'
         <<  now->tm_mday << " "
         << now->tm_hour << ":"
         << now->tm_min << ":"
         << now->tm_sec; 
    return os;
} 

class Stream
{
public:
    Stream() noexcept = default;
    Stream(const Stream &) = delete;
    Stream(Stream && other) noexcept {
        print_ = other.print_;
        moved_ = true;
    };

    void SetPrint() {
        print_ = true;
    }

    template <typename T>
    Stream& operator<<(const T* value) noexcept
    {

        if(print_)
            std::cout << " " << value;
        return *this;
    }

    template <typename T>
    Stream& operator<<(const T& value) noexcept
    {
        if(print_)
            std::cout << " " << value;
        return *this;
    }

    ~Stream()
    {
        if(print_ && moved_)
            std::cout << " ]" << reset_color_ << std::endl;
    }
private:
    bool print_{false};
    bool moved_{false};
    std::string_view reset_color_{"\033[0m"};
};

class Logger
{
public:
    Logger(std::string_view f_context, std::string_view f_description) : 
        ctxId_(f_context), ctxDescription_{f_description} {

    }

    Stream LogFatal() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kFatal) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm now;
            localtime_r(&t, &now);
            stream << check_it_ << text_color_ << &now  << appId_  << ctxId_ << "FATAL:   [";
        }
        return std::move(stream);
    }

    Stream LogError() noexcept
    {
        Stream stream;
        if(GetLevel()  >= LogLevel::kError) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm now;
            localtime_r(&t, &now);
            stream << check_it_ << text_color_ << &now << appId_ << ctxId_ <<  "ERROR:   [";
        }

        return std::move(stream);
    }

    Stream LogWarn() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kWarn) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm now;
            localtime_r(&t, &now);
            stream << check_it_ << text_color_ << &now << appId_ << ctxId_ <<  "WARNING: [";
        }
        return std::move(stream);
    }

    Stream LogInfo() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kInfo) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm now;
            localtime_r(&t, &now);
            stream << text_color_ << &now << appId_ << ctxId_ <<  "INFO:    [";
        }
        return std::move(stream);
    }

    Stream LogDebug() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kDebug) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm now;
            localtime_r(&t, &now);
            stream << text_color_ << &now << appId_ << ctxId_ <<  "DEBUG:  [";
        }
        return std::move(stream);
    }

    Stream LogVerbose() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kVerbose) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm now;
            localtime_r(&t, &now);
            stream << text_color_ << &now << appId_ << ctxId_ <<  "VERBOSE: [";
        }
        return std::move(stream);
    }
private:
    const std::string_view appId_{"LCLM"};
    const std::string_view ctxId_{"####"};
    const std::string_view ctxDescription_{"####"};
    const std::string_view text_color_{"\033[0;34m"};
    const std::string_view check_it_{"\033[101;30m !!! -> \033[0m"};
};

inline Logger& _getLmLogger() noexcept {
    // RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "This is safe because the static is a function local.", true);
    static Logger log{"LCLM", "Launch Manager logging context"};
    return log;
}

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  // LC_LOG_SCORE_MW_LOG

// wrapper macros for Launch Manager
#define LM_LOG_FATAL() (etas::vrte::lcm::_getLmLogger().LogFatal())
#define LM_LOG_ERROR() (etas::vrte::lcm::_getLmLogger().LogError())
#define LM_LOG_WARN() (etas::vrte::lcm::_getLmLogger().LogWarn())
#define LM_LOG_INFO() (etas::vrte::lcm::_getLmLogger().LogInfo())
#define LM_LOG_DEBUG() (etas::vrte::lcm::_getLmLogger().LogDebug())

#endif  // LCM_LOG_HPP_INCLUDED
