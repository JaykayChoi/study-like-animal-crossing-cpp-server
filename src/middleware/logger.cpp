#include "logger.h"
#include <time.h>

Logger& Logger::Get()
{
    static Logger instance;
    return instance;
}

void Logger::Log(std::string s, ELogLevel level)
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    std::string msg = fmt::sprintf("[%02d:%02d:%02d] %s", timeinfo->tm_hour,
        timeinfo->tm_min, timeinfo->tm_sec, s.c_str());

    msg = fmt::sprintf("%s: %s\n", ELogLevelToString(level).c_str(), msg.c_str());

    printf("%s", msg.c_str());

    // TODO jaykay write on file.
}

std::string Logger::ELogLevelToString(ELogLevel level)
{
    switch (level)
    {
    case ELogLevel::Error:
        return "Error";
    case ELogLevel::Warning:
        return "Warning";
    case ELogLevel::Info:
        return "Info";
    case ELogLevel::Verbose:
        return "Verbose";
    case ELogLevel::Debug:
        return "Debug";
    default:
        return "";
    }
}
