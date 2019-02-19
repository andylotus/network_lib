#include<cstdio>
#include<cstdarg>
#include<cstring>
#include<iomanip>
#include <cassert>
#include <syscall.h>
#include <unistd.h>

#include"Log.h"
#include "CurrentThread.h"

namespace lotus
{
namespace net
{
    Log::LogRank Log::RANK = Log::TRACE;

    Log &Log::init_impl(const char *file, const int line, const char *func, LogRank level)
    {
        this->file_ = file;
        this->line_ = line;
        this->func_ = func;
        this->level_ = level;
        return *this;
    }

    Log &Log::print_file_info()
    {
        out << " - " << file_ << ':' << line_ << ' ' << func_;
        return *this;
    }

    Log &Log::print_time()
    {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        time_t sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
        auto micr = std::chrono::duration_cast<std::chrono::microseconds>(now % std::chrono::seconds(1)).count();

        static thread_local time_t last_time{};

        thread_local char t_time[18];
        if (sec != last_time) {
            last_time = sec;

            tm tm_time{};
            localtime_r(&sec, &tm_time);
            int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
            assert(len == 17);
        }

        out << t_time << '.' << std::setfill('0') << std::setw(6) << micr << ' ' <<std::setfill('0') << std::setw(5)<< CurrentThread::tid()<< ' ';
        return *this;
    }

    Log::~Log() noexcept
    {
        print_file_info();
        //redirect the stringstream 'out' to iostream 'cout', so that the cotent will be shown on stdout (screen)
        std::cout << out.rdbuf() << std::endl;
        if(level_ == FATAL)
        {
            abort();
        }
    }
}
}
