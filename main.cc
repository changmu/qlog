#include "qlog.h"
#include <unistd.h>
#include <time.h>

using namespace qzg;

int main()
{
    // setLog_level(QLogger::LINFO);
    setLog_filename("");
    // setLog_size(1 << 20);
    // setLog_flag(QLogger::LLOGLEVEL);

    int cnt = 0;
    for (int i = 0; i < 100; ++i) {
        // qlog_fatal("this is a fatal msg, [cnt=%d]\n", ++cnt);
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
