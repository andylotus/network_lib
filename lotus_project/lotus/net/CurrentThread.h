#ifndef LOTUS_NET_CURRENTTHREAD_H
#define LOTUS_NET_CURRENTTHREAD_H


#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace lotus{
  namespace CurrentThread{

    extern thread_local int t_cachedTid;

    inline int tid(){
      if (t_cachedTid == 0){
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
      }
      return t_cachedTid;
    }

  }
}


#endif  // LOTUS_NET_CURRENTTHREAD_H