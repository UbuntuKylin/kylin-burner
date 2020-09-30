/*
 * Copyright (C) 2020  KylinSoft Co., Ltd.
 *
 * Authors:
 *   Derek Wong<Derek_Wang39@163.com>
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

#ifndef KYBTHEMEMANAGER_H
#define KYBTHEMEMANAGER_H

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QList>
#include <QGSettings/QGSettings>

class KYBTheme
{
public:
    KYBTheme();
    KYBTheme(QWidget *obj);
    ~KYBTheme();
    void setObj(QWidget *object) {obj = object;}
    void setStyle(QString content) {style = content;}
    void setStyleHover(QString content) {styleHover = content;}
    void setStyleActive(QString content) {styleActive = content;}
    void setStyleDisable(QString content) {styleDisable = content;}
    void normal() {if (obj && obj->isEnabled()) obj->setStyleSheet(style);}
    void hover() {if (obj && obj->isEnabled()) obj->setStyleSheet(styleHover);}
    void active() {if (obj && obj->isEnabled()) obj->setStyleSheet(styleActive);}
    void disable() {if (obj && !obj->isEnabled()) obj->setStyleSheet(styleDisable);}
    QWidget *getObj() {return obj;}
private:
    QWidget *obj; /* the widget */
    QString  style;
    QString  styleHover;
    QString  styleActive;
    QString  styleDisable;
};

typedef QHash<QString, KYBTheme*> KYBThemes;
typedef QList<QString> KYBThemeTypes;

class KYBThemeManager : public QObject
{
    Q_OBJECT

signals:

public:
    ~KYBThemeManager();

public:
    static KYBThemeManager* instance()
    {
        static KYBThemeManager manager(nullptr);
        return &manager;
    }
public:
    bool regTheme(QWidget *obj, QString theme = QString(),
                  QString normal = QString(),/* default */
                  QString hover= QString(),/* hover */
                  QString active = QString(),/* active */
                  QString disable = QString());/* disable */
    void delTheme(QString theme);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
private:
    KYBThemeManager(QObject *parent = nullptr);
    KYBThemeManager(const KYBThemeManager&) = delete;
    KYBThemeManager &operator=(const KYBThemeManager&) = delete;
private:
    void onChangeTheme(QString);
private:
    KYBThemes     themes; /* key is obj-name + types */
    KYBThemes     themesByType; /* key is only types */
    KYBThemeTypes types;
    QString       currentTheme;
    QGSettings   *setting;
};

#define ThManager() KYBThemeManager::instance()

#endif // KYBTHEMEMANAGER_H
