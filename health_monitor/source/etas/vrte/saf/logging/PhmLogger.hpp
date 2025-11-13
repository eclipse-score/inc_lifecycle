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

#ifndef PHMLOGGER_HPP_INCLUDED
#define PHMLOGGER_HPP_INCLUDED

#include <string_view>

#ifdef LC_LOG_SCORE_MW_LOG
#include "score/mw/log/logger.h"
#include "score/mw/log/log_stream.h"
#else
#include <iostream>
#include <atomic>
#include <ctime>
#include <cstdlib>
#include <string>
#endif


namespace etas
{
namespace vrte
{
namespace saf
{
namespace logging
{

#ifdef LC_LOG_SCORE_MW_LOG

using Stream = score::mw::log::LogStream;
using Logger = score::mw::log::Logger;

#else // LC_LOG_SCORE_MW_LOG

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
            std::tm* now = std::localtime(&t);
            stream << check_it_ << text_color_ << now  << appId_  << ctxId_ << "FATAL:   [";
        }
        return std::move(stream);
    }

    Stream LogError() noexcept
    {
        Stream stream;
        if(GetLevel()  >= LogLevel::kError) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm* now = std::localtime(&t);
            stream << check_it_ << text_color_ << now << appId_ << ctxId_ <<  "ERROR:   [";
        }

        return std::move(stream);
    }

    Stream LogWarn() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kWarn) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm* now = std::localtime(&t);
            stream << check_it_ << text_color_ << now << appId_ << ctxId_ <<  "WARNING: [";
        }
        return std::move(stream);
    }

    Stream LogInfo() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kInfo) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm* now = std::localtime(&t);
            stream << text_color_ << now << appId_ << ctxId_ <<  "INFO:    [";
        }
        return std::move(stream);
    }

    Stream LogDebug() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kDebug) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm* now = std::localtime(&t);
            stream << text_color_ << now << appId_ << ctxId_ <<  "DEBUG:  [";
        }
        return std::move(stream);
    }

    Stream LogVerbose() noexcept
    {
        Stream stream;
        if(GetLevel() >= LogLevel::kVerbose) {
            stream.SetPrint();
            std::time_t t = std::time(0);
            std::tm* now = std::localtime(&t);
            stream << text_color_ << now << appId_ << ctxId_ <<  "VERBOSE: [";
        }
        return std::move(stream);
    }
private:
    const std::string_view appId_{"LCHM"};
    const std::string_view ctxId_{"####"};
    const std::string_view ctxDescription_{"####"};
    const std::string_view text_color_{"\033[0;32m"};
    const std::string_view check_it_{"\033[101;30m !!! -> \033[0m"};
};

#endif // LC_LOG_SCORE_MW_LOG

/// @brief Phm Logging Adapter
/// @details This class is a thin adaptor to score::mw::log::Logger to centrally adapt potential API changes of the Logger
/// class.
class PhmLogger
{
public:
    /// @brief Context Enum
    /// @details Defines a list of PHM logging context to choose from
    enum class EContext : uint8_t
    {
        factory = 0,
        supervision = 1,
        recovery = 2,
        watchdog = 3

    };

    /// @brief Get Logger (Singleton)
    /// @brief Calls the score::mw::log::Logger Singleton for the given logging context
    /// @param f_context    Logging context
    /// @return             PhmLogger reference for the given logging context
    static PhmLogger& getLogger(const EContext f_context) noexcept(true);

    /// @brief Invoke score::mw::log::Logger::LogFatal()
    Stream LogFatal() noexcept;

    /// @brief Invoke score::mw::log::Logger::LogError()
    Stream LogError() noexcept;

    /// @brief Invoke score::mw::log::Logger::LogWarn()
    Stream LogWarn() noexcept;

    /// @brief Invoke score::mw::log::Logger::LogInfo()
    Stream LogInfo() noexcept;

    /// @brief Invoke score::mw::log::Logger::LogDebug()
    Stream LogDebug() noexcept;

    /// @brief Invoke score::mw::log::Logger::LogVerbose()
    Stream LogVerbose() noexcept;

protected:
    /// @brief Constructor
    /// @param f_context        Logging context ID (must match to logging configurations)
    /// @param f_description    Logging context description
    PhmLogger(std::string_view f_context, std::string_view f_description) noexcept(true);

    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /// @brief Default Move Constructor
    PhmLogger(PhmLogger&&) = default;

    /// @brief No Move Assignment
    PhmLogger& operator=(PhmLogger&&) = delete;
    /// @brief No Copy Constructor
    PhmLogger(const PhmLogger&) = delete;
    /// @brief No Copy Assignment
    PhmLogger& operator=(const PhmLogger&) = delete;

    /// @brief Default Destructor
    virtual ~PhmLogger() = default;

private:
    /// @brief Logger reference for specific context
    // Logger& logger_r;
    Logger logger_r;
};

}  // namespace logging
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
