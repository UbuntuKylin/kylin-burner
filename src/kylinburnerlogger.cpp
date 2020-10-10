/*
 * Copyright (C) 2020  KylinSoft Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kylinburnerlogger.h"

#include <ctime>
#include <utility>

#include <pthread.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include<QDebug>

#define THREAD_SAFE 1
#if THREAD_SAFE
static int gLocker = 0;
static inline void lock() {while (!__sync_bool_compare_and_swap(&gLocker, 0, 1));}
static inline void unlock() {while (!__sync_bool_compare_and_swap(&gLocker, 1, 0));}
#else
static inline void lock() {}
static inline void unlock(){}
#endif

KylinBurnerLogger::~KylinBurnerLogger()
{
}

void KylinBurnerLogger::log(const char *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    __(this->_level, fmt, args);
    va_end(args);

}

void KylinBurnerLogger::test(const char *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    __(TEST, fmt, args);
    va_end(args);
}

void KylinBurnerLogger::debug(const char *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    __(DEBUG, fmt, args);
    va_end(args);

}

void KylinBurnerLogger::error(const char *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    __(ERROR, fmt, args);
    va_end(args);

}

void KylinBurnerLogger::warning(const char *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    __(WARNING, fmt, args);
    va_end(args);

}

void KylinBurnerLogger::info(const char *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    __(INFO, fmt, args);
    va_end(args);

}

void KylinBurnerLogger::__(int level, const char *fmt, va_list args)
{
    logContent *content;

    content = createContent();
    if (NULL == content)
    {
        qDebug() << "Error to create content.";
        return;
    }
    content->format = strdup(fmt);
    if (NULL == content->format)
    {
        free(content);
        qDebug() << "Error to copy format information.";
        return;
    }
    if (_moudleName.length() > 20) { strncpy(content->module, _moudleName.c_str(), 18); strcat(content->module, "]"); }
    else strcpy(content->module, _moudleName.c_str());
    va_copy(content->_list, args);
    vsprintf(content->content, fmt, args);
    LogRecorder::instance().recorder(this, level, content);
}

void *write_log_content(void *arg)
{
    _content   *c = (_content *)arg;
    logContent *w = NULL;
    char       *p,
                content[2048];

    prctl(PR_SET_NAME, "logger");

    while (LogRecorder::instance().ready())
    {
        while (c->size())
        {
            lock();
            w = c->front();
            c->pop();
            unlock();
            memset(content, 0, 1024);
            p = content;
            strcpy(p, ctime(&(w->_time)));
            p += strlen(p);
            --p;
            *p++ = ' ';
            strcpy(p, w->module);
            p += strlen(p);
            *p++ = ' ';
            strcpy(p, w->level);
            p += strlen(p);
            strcat(content, w->content);
            if ('\n' != content[strlen(content)]) strcat(content, "\n");
            write(w->fd, content, strlen(content));
            va_end(w->_list);
            free(w->format);
            free(w);
        }
    }
    return 0;
}

static inline std::string &trim(std::string &str)
{
    if (str.empty()) return str;

    if (str.find_first_not_of(" ")) str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1, str.length());
    return str;
}

KylinBurnerLogger *LogRecorder::registration(const char *filePath,
                                             const char *fileName,
                                             const char *moudle)
{
    KylinBurnerLogger *ret = NULL;
    string             path,
                       name,
                       whole,
                       regWhole;
    int                suffixPos,
                       fd;
    bool               needOpen;
    char              *p,
                      *q,
                       tmp[128];

    /*
    if (!isReady)
    {
        isReady = true;
        if (pthread_create(&writerProcess, NULL, write_log_content, &contents))
        { isReady = false; return NULL;}
    }
    */

    path.clear();
    if (filePath) path.append(filePath);
    else path.append(DEFAULT_PATH);
    if (trim(path).empty()) path.append(DEFAULT_PATH);
    if (path[path.length() - 1] != '/') path.append("/");

    /*
     * Adjust the directory is exist or not.
     */
    p = (char *)path.c_str();
    memset(tmp, 0, 32);
    q = tmp;
    while ('\0' != *p)
    {
        *q++ = *p++;
        if (*(q - 1) == '/')
        {
            if (access(tmp, 0))
            {
                if (mkdir(tmp, 0755) < 0)
                {
                    return NULL;
                }
            }
        }
    }

    name.clear();
    if (fileName) name.append(fileName);
    else name.append(DEFAULT_NAME);
    if (trim(name).empty()) name.append(DEFAULT_NAME);
    suffixPos = name.find_last_of(".");
    if (-1 == suffixPos) name.append(".log");
    else if (-1 == name.find("log", suffixPos)) name.append(".log");
    else {}

    whole.clear();
    whole = path + name;

    regWhole.clear();
    regWhole = whole;
    regWhole.append(moudle);

    while (!__sync_bool_compare_and_swap(&locker, 0, 1));
    _RegHandle::iterator rit;
    rit = regHandle.find(regWhole);
    if (rit != regHandle.end()) return rit->second;

    _PathHandle::iterator it;
    it = pathHandle.find(whole);
    if (it != pathHandle.end()) needOpen = false;
    else needOpen = true;
    if (needOpen)
    {
        fd = -1;
        fd = open(whole.c_str(), O_CREAT | O_APPEND | O_RDWR, 0644);
        if (fd < 0)
        {
            while (!__sync_bool_compare_and_swap(&locker, 1, 0));
            return NULL;
        }
        pathHandle.insert(pair<string, int>(whole, fd));
    }

    ret = new KylinBurnerLogger(path.c_str(), name.c_str(), moudle);
    if (!ret) return NULL;
    logHandle.insert(pair<KylinBurnerLogger *, int*>(ret, &(pathHandle[whole])));
    regHandle.insert(pair<string, KylinBurnerLogger *>(regWhole, ret));
    while (!__sync_bool_compare_and_swap(&locker, 1, 0));

    return ret;
}

KylinBurnerLogger *LogRecorder::registration(const char *moudle)
{
    return registration(DEFAULT_PATH, DEFAULT_NAME, moudle);
}

void LogRecorder::recorder(KylinBurnerLogger * logger, int level, logContent *contentd)
{

    char       *p,
                content[2048];
    _LoggerHandle::iterator it;

    if (!logger || !contentd) return;
    if (level > this->level)
    {
        qDebug() << "out of current level : " << this->level << " recorder level : " << level;
        if (contentd->format) free(contentd->format);
        free(contentd);
        return;
    }

    strcpy(contentd->level, _level[level]);

    it = logHandle.find(logger);
    if (it == logHandle.end())
    {
        qDebug() << "cannot find logger recorder, not registration logger.";
        if (contentd->format) free(contentd->format);
        free(contentd);
        return;
    }
    contentd->fd = *(it->second);

    memset(content, 0, 1024);
    p = content;
    strcpy(p, ctime(&(contentd->_time)));
    p += strlen(p);
    --p;
    *p++ = ' ';
    strcpy(p, contentd->module);
    p += strlen(p);
    *p++ = ' ';
    strcpy(p, contentd->level);
    p += strlen(p);
    strcat(content, contentd->content);
    if ('\n' != content[strlen(content)]) strcat(content, "\n");
    write(contentd->fd, content, strlen(content));
    free(contentd->format);
    free(contentd);

    /*
    lock();
    contents.push(content);
    unlock();
    */
}
