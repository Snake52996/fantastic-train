#include<includes/Logger.hpp>
namespace Logger{
    LogLevel log_level{LogLevel::All};
    void setLogLevel(LogLevel level){
        log_level = level;
    }
}