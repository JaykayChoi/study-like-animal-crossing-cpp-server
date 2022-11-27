#pragma once

#include "fmt/core.h"
#include "fmt/printf.h"
#include <string>

class Logger
{
public:
    enum class ELogLevel
    {
        Error,
        Warning,
        Info,
        Verbose,
        Debug,
    };

    static Logger& Get();

    void Log(std::string s, ELogLevel level);

private:
    Logger() { }
    Logger(const Logger& ret) { }
    Logger& operator=(const Logger& ref) { }
    ~Logger() { }

    std::string ELogLevelToString(ELogLevel level);
};

/////////////////////////////////////////////////////////////////////////////////////////
// Global functions

template <typename... Args> 
extern void Log(fmt::string_view format, const Args&... args)
{
    Logger::Get().Log(fmt::sprintf(format, args...), Logger::ELogLevel::Verbose);
}

template <typename... Args>
extern void LogWarn(fmt::string_view format, const Args&... args)
{
    Logger::Get().Log(fmt::sprintf(format, args...), Logger::ELogLevel::Warning);
}

template <typename... Args>
extern void LogError(fmt::string_view format, const Args&... args)
{
    Logger::Get().Log(fmt::sprintf(format, args...), Logger::ELogLevel::Error);
}

template <typename... Args>
extern void LogInfo(fmt::string_view format, const Args&... args)
{
    Logger::Get().Log(fmt::sprintf(format, args...), Logger::ELogLevel::Info);
}