#ifndef LOTUS_NET_LOG_H
#define LOTUS_NET_LOG_H

#include<sstream>
#include<iostream>
#include<chrono>
#include<thread>

namespace lotus
{
namespace net
{
    namespace impl
    {
        constexpr bool is_separator(const char c)
        {
            return (c == '/' || c == '\\');
        }

        constexpr int get_base_index(const char *filename, const int n)
        {
            return (n == -1 || is_separator(filename[n])) ? n + 1 : get_base_index(filename, n - 1);
        }
    }

    class Log
    {    
    public:
        enum LogRank
        {
            ALL=0,TRACE,DEBUG,INFO,WARN,ERROR,FATAL,NONE
        };

        Log() = default;

        template<int N>
        Log &init(const char *file,const int line, const char *func, LogRank level)
        {
            return init_impl(file+N,line,func, level);
        }

        ~Log()noexcept ;

        Log &print_time();

        Log &print_file_info();

        template<typename T>
        Log &operator<<(const T &t)
        {
            out << t;
            return *this;
        }

    public:

        static void set_rank(LogRank i)
        {
            RANK = i;
        }

        static LogRank get_rank()
        {
            return RANK;
        }
        
    private:
        Log & init_impl(const char *file,const int line, const char *func, LogRank level);
        
    private:
        const char *func_;
        const char *file_;
        int line_;
        std::stringstream out;
        static LogRank RANK;
        LogRank level_;
    };
}
}

#define LOG_TRACE if(net::Log::get_rank()<=net::Log::TRACE)\
    net::Log{}.init<net::impl::get_base_index(__FILE__,sizeof(__FILE__)-1)>(__FILE__,__LINE__,__func__, net::Log::TRACE)\
            .print_time()<<"TRACE "
#define LOG_DEBUG if(net::Log::get_rank()<=net::Log::DEBUG)\
    net::Log{}.init<net::impl::get_base_index(__FILE__,sizeof(__FILE__)-1)>(__FILE__,__LINE__,__func__, net::Log::DEBUG)\
            .print_time()<<"DEBUG "
#define LOG_INFO  if(net::Log::get_rank()<=net::Log::INFO)\
    net::Log{}.init<net::impl::get_base_index(__FILE__,sizeof(__FILE__)-1)>(__FILE__,__LINE__,__func__, net::Log::INFO)\
            .print_time()<<" INFO "
#define LOG_WARN  if(net::Log::get_rank()<=net::Log::WARN)\
    net::Log{}.init<net::impl::get_base_index(__FILE__,sizeof(__FILE__)-1)>(__FILE__,__LINE__,__func__, net::Log::WARN)\
            .print_time()<<" WARN "
#define LOG_ERROR if(net::Log::get_rank()<=net::Log::ERROR)\
    net::Log{}.init<net::impl::get_base_index(__FILE__,sizeof(__FILE__)-1)>(__FILE__,__LINE__,__func__, net::Log::ERROR)\
            .print_time()<<"ERROR "
#define LOG_FATAL if(net::Log::get_rank()<=net::Log::FATAL)\
    net::Log{}.init<net::impl::get_base_index(__FILE__,sizeof(__FILE__)-1)>(__FILE__,__LINE__,__func__, net::Log::FATAL)\
            .print_time()<<"FATAL "


#endif //LOTUS_NET_LOG_H