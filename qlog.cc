#include "qlog.h"

#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <time.h>
#include <utility>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

using namespace std;

namespace qzg {

// static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
void *log_routine(void *arg) {
    pthread_setname_np(pthread_self(), "wgadpt-qlog");
    // pthread_detach(pthread_self());

    QLogger *h = (QLogger *) arg;
    h->main();
    return nullptr;
}

void QLogger::main() {
    struct timespec ts = {
        .tv_sec = time(NULL) + syncInterval_,
        .tv_nsec = 0
    };

    for (;;) {
        pthread_cond_timedwait(&cond_, &condMtx_, &ts);
        flush();

        ts.tv_sec = time(NULL) + syncInterval_;
    }
}

QLogger::QLogger(): level_(LALL), rotateSize_(64 << 10), fd_(-1), flags_(LTIMESTAMP|LFILELINE|LTHREADTID|LLOGLEVEL),
                    syncInterval_(3), tid_(-1)
{
    pthread_mutex_init(&mtx_, NULL);
    pthread_mutex_init(&condMtx_, NULL);
    pthread_cond_init(&cond_, NULL);
}

QLogger::~QLogger() {
    // printf("debug: ~QLogger()\n");
    flush();
    #if 0
    if (-1 != fd_) {
        close(fd_);
        fd_ = -1;
    }

    if ((int) tid_ != -1)
        pthread_cancel(tid_);

    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mtx_);
    pthread_mutex_destroy(&condMtx_);
    #endif
}

const char *QLogger::levelStrs_[] = {
    "FATAL",
    "ERROR",
    "UERR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
    "ALL"
};

QLogger& QLogger::getLogger() {
    static QLogger logger;
    return logger;
}

void QLogger::setLogLevel(const string& level) {
    LogLevel ilevel = LINFO;
    static const size_t kLen = sizeof(levelStrs_) / sizeof(*levelStrs_);
    for (size_t i = 0; i < kLen; ++i) {
        if (0 == strcasecmp(levelStrs_[i], level.c_str())) {
            ilevel = (LogLevel) i;
            break;
        }
    }
    setLogLevel(ilevel);
}

// rotate file, used by flush
void QLogger::setFileName(string filename, bool locked) {
    if (!locked)
        lock();
    prefix_ = filename;
    int fd = -1;

    // delete old file
    if (!oldFilename_.empty()) {
        int r = unlink(oldFilename_.c_str());
        if (r < 0) {
            fprintf(stderr, "delete log file %s failed. msg: %s (ignored)\n",
                    oldFilename_.c_str(), strerror(errno));
        }
    }
    oldFilename_ = "";

    // rename cur file to .old
    if (!filename_.empty()) {
        oldFilename_ = filename_ + ".old";

        close(fd_);
        fd_ = -1;

        int r = rename(filename_.c_str(), oldFilename_.c_str());
        if (r < 0) {
            fprintf(stderr, "rename log file %s to %s failed. msg: %s (ignored)\n",
                    filename_.c_str(), oldFilename_.c_str(), strerror(errno));
        }
    }

    fd = open(filename.c_str(), O_CREAT|O_APPEND|O_WRONLY|O_CLOEXEC, DEFFILEMODE);
    if (fd < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s (ignored)\n",
                filename.c_str(), strerror(errno));
        return;
    }

    filename_ = filename;
    fd_ = fd;

    if (!locked)
        unlock();
}

// only invoked by log thread
void QLogger::flush() {
    int fd = fd_ == -1 ? 1 : fd_;

    do {
        LockGuard lg(&mtx_);

        off_t fileLen = 0;
        if (-1 != fd_)
            fileLen = lseek(fd_, 0, SEEK_CUR);

        while (!logBuf_.empty()) {
            auto& str = logBuf_.front();
            int r = ::write(fd, str.c_str(), str.size());
            if (r != (int) str.size())
                fprintf(stderr, "[%s:%d] ::write failed. msg = %m (ignored)\n", __FILE__, __LINE__);

            if (r > 0 && fd_ != -1)
                fileLen += r;
            if (fileLen >= rotateSize_) {
                setFileName(prefix_);
                fd = fd_ == -1 ? 1 : fd_;
                fileLen = 0;
            }

            logBuf_.pop();
        }
    } while (0);

    // fsync(fd);
}

int QLogger::getTimestamp(char *buf, int len) {
    time_t now = time(0);
    struct tm tmv;
    int off = strftime(buf, len, "%Y-%m-%d %H:%M:%S ", localtime_r(&now, &tmv));
    return off;
}

static thread_local uint64_t tid;
void QLogger::log(int level, const char *file, int line, const char *func, const char *fmt, ...) {
    if (tid == 0) {
        tid = syscall(SYS_gettid);
    }
    if (level > level_) {
        return;
    }

    static thread_local char buf[4 << 10];
    int off = 0, r = 0;
    va_list ap;

    if (flags_ & LTIMESTAMP) {
        off = getTimestamp(buf, sizeof(buf));
    }
    if (flags_ & LLOGLEVEL) {
        r = snprintf(buf + off, sizeof(buf) - off, "[%s] ", levelStrs_[level]);
        off += r;
    }
    if (flags_ & LFILELINE) {
        r = snprintf(buf + off, sizeof(buf) - off, "[%s:%d(%s)] ", file, line, func);
        off += r;
    }
    if (flags_ & LTHREADTID) {
        r = snprintf(buf + off, sizeof(buf) - off, "[tid=%lu] ", (unsigned long) tid);
        off += r;
    }

    va_start(ap, fmt);
    r = vsnprintf(buf + off, sizeof(buf) - off, fmt, ap);
    va_end(ap);
    off += r;
    if (off >= (int) sizeof(buf))
        buf[sizeof(buf) - 1] = '\0'; // in case of buffer overflow

    do {
        LockGuard lg(&mtx_);
        logBuf_.emplace(buf);
    } while (0);
    inform();
}

} // namespace qzg
