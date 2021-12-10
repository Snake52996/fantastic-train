_Pragma("once");
#include<iostream>
#include<string>
#include<thread>
#include<sstream>
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
    extern LogLevel log_level;
    template<typename...Args>
    void trace(const Args&...args){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Trace)){
            std::ostringstream out_buffer;
            (out_buffer << '[' << std::this_thread::get_id() << "][TRACE] " << ... << args);
            std::cerr << out_buffer.view();
        }
    }
    template<typename...Args>
    void debug(const Args&...args){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Debug)){
            std::ostringstream out_buffer;
            (out_buffer << '[' << std::this_thread::get_id() << "][DEBUG] " << ... << args);
            std::cerr << out_buffer.view();
        }
    }
    template<typename...Args>
    void info(const Args&...args){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Info)){
            std::ostringstream out_buffer;
            (out_buffer << '[' << std::this_thread::get_id() << "][INFO] " << ... << args);
            std::cerr << out_buffer.view();
        }
    }
    template<typename...Args>
    void warning(const Args&...args){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Warning)){
            std::ostringstream out_buffer;
            (out_buffer << '[' << std::this_thread::get_id() << "][WARN] " << ... << args);
            std::cerr << out_buffer.view();
        }
    }
    template<typename...Args>
    void error(const Args&...args){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Error)){
            std::ostringstream out_buffer;
            (out_buffer << '[' << std::this_thread::get_id() << "][ERROR] " << ... << args);
            std::cerr << out_buffer.view();
        }
    }
    template<typename...Args>
    void fatal(const Args&...args){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Fatal)){
            std::ostringstream out_buffer;
            (out_buffer << '[' << std::this_thread::get_id() << "][FATAL] " << ... << args);
            std::cerr << out_buffer.view();
        }
    }
    void setLogLevel(LogLevel level);
}