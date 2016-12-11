qlog
====

## 简洁易用的C++11多线程共享异步日志模块

### 代码示例

```c++
#include "qlog.h"

using namespace qzg;

int main()
{
    // setLog_level(QLogger::LINFO);
    // setLog_filename("out.log");
    // setLog_size(1 << 20);
    // setLog_flag(QLogger::LLOGLEVEL);
    // setLog_syncInterval(1000);
    startQLog();

    int cnt = 0;
    for (int i = 0; i < 100; ++i) {
        qlog_fatal("this is a fatal msg, [cnt=%d]\n", ++cnt);
        qlog_err("this is a err msg, [cnt=%d]\n", ++cnt);
        qlog_uerr("this is a uerr msg, [cnt=%d]\n", ++cnt);
        qlog_warn("this is a warn msg, [cnt=%d]\n", ++cnt);
        qlog_info("this is a info msg, [cnt=%d]\n", ++cnt);
        qlog_debug("this is a debug msg, [cnt=%d]\n", ++cnt);
        qlog_trace("this is a trace msg, [cnt=%d]\n", ++cnt);
    }
    printf("Program exit succeed.\n");
    return 0;
}
```

### NOTE:
#### startQLog()应在daemon()之后再调用，不然daemon的fork会清除掉之前生成的log线程, log线程的作用主要是刷新缓冲区内容到文件，即便没有此线程，在程序正常结束时内容会自动刷新到文件。
