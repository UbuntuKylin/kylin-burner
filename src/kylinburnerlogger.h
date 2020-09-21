#ifndef KYLINBURNERLOGGER_H
#define KYLINBURNERLOGGER_H

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <map>
#include <queue>
#include <string>

#include <QString>
#include <QDebug>

#include <fcntl.h>
#include <unistd.h>

using namespace std;

/*
 * if do not appoint the log path and name,
 * these will be default setting for log.
 */
#define DEFAULT_PATH "/tmp/log/burner/"
//#define DEFAULT_PATH "/var/log/burner/"
#define DEFAULT_NAME "kylin-burner.log"

/*
 * level of log, when set LogRecorder level,
 * the information witch level under the setting
 * will not output.
 */
enum LEVEL
{
  INFO = 0,
  WARNING,
    ERROR,
    DEBUG,
    TEST,
    UNKOWN
};

/*
 * the content information.
 *  fd : the file handle to be wrote.
 *  module : the name of current logging module.
 *  format & _list : the real information to logged.
 *  _time : the time when create log content.
 *  level : the name of the log level.
 */
typedef struct _logContent
{
    int      fd;
    char     module[20];
    char    *format;
    char     level[16];
    char     content[1024];
    time_t   _time;
    va_list  _list;
} logContent;

/*
 * the real class to do log.
 * do not suggest new it directly,use the siglton factory LogRecorder
 * to build it, then it could release itself by factory.
 * if new it, must register it to factory, or will not log anything.
 * todo register the object to factory.
 */
class KylinBurnerLogger
{
public:
    KylinBurnerLogger(const char *filePath,
                      const char *fileName,
                      const char *moudleName = 0,
                      int level = INFO)
    {
        _level = level;
        _filePath.clear();
        _filePath.append(filePath);
        _fileName.clear();
        _fileName.append(fileName);
        _moudleName.clear();
        _moudleName = "[";
        if (moudleName) _moudleName.append(moudleName).append("]");
        else _moudleName.append("COMMON]");
    }
    ~KylinBurnerLogger();

public:
    /* level info */
    void info(const char *fmt, ...);
    /* level info */
    void warning(const char *fmt, ...);
    /* level info */
    void error(const char *fmt, ...);
    /* level info */
    void debug(const char *fmt, ...);
    /* level info */
    void test(const char *fmt, ...);
    /* level users' setting, decide by _level */
    void log(const char *fmt, ...);

    void    setLevel(int level) { if (level < UNKOWN) _level = level; }
    char   *getCharFilePath() { return (char *)_filePath.c_str(); }
    char   *getCharFileName() { return (char *)_fileName.c_str(); }
    string  getStringFilePath() { return _filePath; }
    string  getStringFileName() { return _fileName; }
    QString getQStringFilePath() { return QString::fromStdString(_filePath); }
    QString getQStringFileName() { return QString::fromStdString(_fileName); }
private:
    logContent *createContent()
    {
        logContent *ret = NULL;

        ret = (logContent *)calloc(1, sizeof(logContent));
        if (ret) ret->_time = time(NULL);
        return ret;
    }
    void __(int level, const char *fmt, va_list args);

private:
    int     _level;
    string  _filePath;
    string  _fileName;
    string  _moudleName;
};

/* the log content queue to be wrote */
typedef queue<logContent *> _content;
/* register the object to file handle */
typedef map<KylinBurnerLogger *, int*> _LoggerHandle;
/* register the path&name to file handle */
typedef map<string, int> _PathHandle;
/* register the path&name to object, if registered before
   return the stored object directly. */
typedef map<string, KylinBurnerLogger *> _RegHandle;

class LogRecorder
{
private:
    const char *_level[UNKOWN + 1] =
    {
        "[INFO] ",
        "[WARNING] ",
        "[ERROR] ",
        "[DEBUG] ",
        "[TEST] ",
        "[UNKOWN] "
    };
private:
    LogRecorder()
    {
        isReady = false; locker = 0;level = UNKOWN;
    }
    LogRecorder(LogRecorder &) = delete;
    LogRecorder operator=(LogRecorder &) = delete;

public:
    ~LogRecorder()
    {
        if (isReady)
        {
            isReady = false;
            pthread_join(writerProcess, NULL);
        }
        if (locker) while (!__sync_bool_compare_and_swap(&locker, 1, 0));
        _PathHandle::iterator it;
        for (it = pathHandle.begin(); it != pathHandle.end(); ++it) close(it->second);
        _LoggerHandle::iterator i;
        KylinBurnerLogger *logger = NULL;
        for (i = logHandle.begin(); i != logHandle.end(); ++i)
        {
            logger = i->first;
            if (logger) delete logger;
        }
    }
    static LogRecorder &instance()
    {
        static LogRecorder rec;
        return rec;
    }
    KylinBurnerLogger *registration(const char *filePath,
                                    const char *fileName,
                                    const char *moudle = "common");
    KylinBurnerLogger *registration(const char *moudle = "common");
    /* to do */
    KylinBurnerLogger *registration(KylinBurnerLogger *);
    void recorder(KylinBurnerLogger * logger, int level, logContent *content);
    bool ready() { return isReady;}
    void setLevel(int _level) { while (!__sync_bool_compare_and_swap(&level, level, _level)); }
private:
    int                locker;
    int                level;
    bool               isReady;
    KylinBurnerLogger *err;
    unsigned long int  writerProcess;
    _content           contents;
    _LoggerHandle      logHandle;
    _PathHandle        pathHandle;
    _RegHandle         regHandle;
};

#endif // KYLINBURNERLOGGER_H
