_Pragma("once");
#include<iostream>
#include<string>
namespace Logger{
    enum class LogLevel: unsigned short{
        All,
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
        Off
    };
    std::ostream& trace();
    std::ostream& debug();
    std::ostream& info();
    std::ostream& warning();
    std::ostream& error();
    std::ostream& fatal();
    void setLogLevel(LogLevel level);
    std::string threadId();
}