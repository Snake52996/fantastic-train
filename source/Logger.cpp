#include<includes/Logger.hpp>
#include<thread>
namespace Logger{
    namespace{
        std::ostream dummy{nullptr};
        LogLevel log_level{LogLevel::All};
    }
    std::ostream& trace(){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Trace)){
            return std::cerr;
        }else{
            return dummy;
        }
    }
    std::ostream& debug(){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Debug)){
            return std::cerr;
        }else{
            return dummy;
        }
    }
    std::ostream& info(){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Info)){
            return std::cerr;
        }else{
            return dummy;
        }
    }
    std::ostream& warning(){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Warning)){
            return std::cerr;
        }else{
            return dummy;
        }
    }
    std::ostream& error(){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Error)){
            return std::cerr;
        }else{
            return dummy;
        }
    }
    std::ostream& fatal(){
        if(static_cast<unsigned short>(log_level) <= static_cast<unsigned short>(LogLevel::Fatal)){
            return std::cerr;
        }else{
            return dummy;
        }
    }
    void setLogLevel(LogLevel level){
        log_level = level;
    }
    std::string threadId(){
        return std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ": ";
    }
}