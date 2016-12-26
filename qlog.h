#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <queue>
#include <unistd.h>

#define qlog_trace(...) qlog(QLogger::LTRACE, __VA_ARGS__)
#define qlog_debug(...) qlog(QLogger::LDEBUG, __VA_ARGS__)
#define qlog_info(...) qlog(QLogger::LINFO, __VA_ARGS__)
#define qlog_warn(...) qlog(QLogger::LWARN, __VA_ARGS__)
#define qlog_uerr(...) qlog(QLogger::LUERR, __VA_ARGS__)
#define qlog_err(...) qlog(QLogger::LERROR, __VA_ARGS__)
#define qlog_fatal(...) \
    do { \
        qlog(QLogger::LFATAL, __VA_ARGS__); \
        sync(); \
        abort(); \
    } while (0)
#define qlog_fatal_if(x, ...) if (x) qlog_fatal(__VA_ARGS__)

#define setLog_level(l) QLogger::getLogger().setLogLevel(l)
#define setLog_filename(n) QLogger::getLogger().setFileName(n, false)
#define setLog_size(s) QLogger::getLogger().setFileSize(s)
#define setLog_flag(f) QLogger::getLogger().setFileFlag(f)
// #define setLog_syncInterval(sec) QLogger::getLogger().setSyncInterval(sec)

// #define startQLog() QLogger::getLogger().start()
// #define stoplog() QLogger::getLogger().stop()

#define qlog(level, ...) \
    do { \
        if ((level) <= QLogger::getLogger().getLogLevel()) { \
            QLogger::getLogger().log((level), __FILE__, __LINE__, __func__, __VA_ARGS__); \
        } \
    } while (0)


namespace qzg {

struct noncopyable {
    noncopyable() {}
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

struct LockGuard {
    LockGuard(pthread_mutex_t *pmtx): pmtx_(pmtx) {
        pthread_mutex_lock(pmtx_);
    }
    ~LockGuard() {
        pthread_mutex_unlock(pmtx_);
    }
private:
    pthread_mutex_t *pmtx_;
};

// void *log_routine(void *arg);
struct QLogger: private noncopyable {
    enum LogLevel { LFATAL = 0, LERROR, LUERR, LWARN, LINFO, LDEBUG, LTRACE, LALL };
    enum LogFlag {
        LTIMESTAMP  = 1 << 0,
        LTHREADTID  = 1 << 1,
        LFILELINE   = 1 << 2,
        LLOGLEVEL   = 1 << 3
    };
    QLogger();
    ~QLogger();
    void log(int level, const char *file, int line, const char *func, const char *fmt, ...);

    void setFileName(std::string filename, bool locked = true); // rotate
    void setFileSize(off_t rotateSize) { LockGuard lg(&mtx_); rotateSize_ = rotateSize; }
    void setLogLevel(const std::string& level);
    void setLogLevel(LogLevel level) { LockGuard lg(&mtx_); level_ = std::min(LALL, std::max(LFATAL, level)); }
    void setFileFlag(int flags) { LockGuard lg(&mtx_); flags_ = flags; }
    // void setSyncInterval(int sec) { LockGuard lg(&mtx_); syncInterval_ = sec; } // exec QLogger::sync()

    LogLevel getLogLevel() { return level_; }
    const char *getLogLevelStr() { return levelStrs_[level_]; }
    int getFd() { LockGuard lg(&mtx_); return fd_; }

    static QLogger& getLogger();
    // void start() { pthread_create(&tid_, NULL, log_routine, this); }
    // void main();
    // void stop() { pthread_join(tid_, NULL); }
    void flush(const char *str, int size);
private:
    void lock() { pthread_mutex_lock(&mtx_); }
    void unlock() { pthread_mutex_unlock(&mtx_); }
    // void inform() { pthread_cond_signal(&cond_); }

    // std::string getPostfix(); // for extension
    int getTimestamp(char *buf, int len);
    static const char *levelStrs_[LALL + 1];
    LogLevel level_;
    off_t rotateSize_;
    int fd_;
    int flags_;
    // int syncInterval_;
    std::string filename_;
    std::string oldFilename_;
    std::string prefix_; // for auto rotate
    // std::string postfix_;
    pthread_mutex_t mtx_;
    // pthread_mutex_t condMtx_;
    // pthread_cond_t cond_;
    // std::queue<std::string> logBuf_;
    // pthread_t tid_;
};

} // namespace qzg
