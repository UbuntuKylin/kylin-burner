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

#include "ThemeManager.h"

#include <QListView>
#include <QComboBox>
#include <QEvent>
#include <QDebug>

KYBThemeManager::KYBThemeManager(QObject *parent) : QObject(parent)
{
    setting = NULL;
    if (QGSettings::isSchemaInstalled("org.ukui.style"))
    {
        setting = new QGSettings("org.ukui.style");
    }
#if 1
    if (setting)
    {
        currentTheme = setting->get("styleName").toString();
        if (-1 == types.indexOf(currentTheme)) types << currentTheme;
        connect(setting, &QGSettings::changed, this, [=](const QString &key){
        if (key == "styleName")
            onChangeTheme(setting->get(key).toString());
        });
    }
#endif
    themes.clear();
}

KYBThemeManager::~KYBThemeManager()
{
    qDebug() << "Release theme manager.";
    KYBThemes::const_iterator it;
    KYBTheme *val = NULL;
    for (it = themes.constBegin(); it != themes.constEnd(); ++it)
    {
        val = it.value();
        if (val) delete(val);
    }
    themes.clear();
    themesByType.clear();
    if (setting) delete setting;
}

bool KYBThemeManager::eventFilter(QObject *obj, QEvent *event)
{
    KYBTheme *theme = NULL;

    if (!obj->isWidgetType()) return QObject::eventFilter(obj, event);

    QWidget *widget  = static_cast<QWidget *>(obj);
    theme = themes.value(obj->objectName() + currentTheme);
    if (NULL == theme) return QObject::eventFilter(obj, event);

    switch (event->type())
    {
    case QEvent::HoverEnter:
        theme->hover();
        if (obj->inherits("QComboBox")) (static_cast<QComboBox *>(obj))->setView(new QListView());
        break;
    case QEvent::HoverLeave:
        theme->normal();
        if (obj->inherits("QComboBox")) (static_cast<QComboBox *>(obj))->setView(new QListView());
        break;
    case QEvent::MouseButtonPress:
        theme->active();
        widget->setFocus();
        if (obj->inherits("QComboBox")) (static_cast<QComboBox *>(obj))->setView(new QListView());
        break;
    case QEvent::EnabledChange:
        theme->disable();
        if (obj->inherits("QComboBox")) (static_cast<QComboBox *>(obj))->setView(new QListView());
        break;
    }

    return QObject::eventFilter(obj, event);
}

bool KYBThemeManager::regTheme(QWidget *obj, QString themeName, QString normal, QString hover, QString active, QString disable)
{
    //return false;
    KYBTheme *theme;

    if (themeName.isEmpty()) themeName = currentTheme;
    if (!obj || themeName.isEmpty())
    {
        qDebug() << "TTHEME ERROR:" << "obj or theme name is invalid."
                 << "obj : " << obj << "theme : " << themeName;
        return false;
    }

    theme = themes[obj->objectName() + themeName];
    if (theme)
    {
        qDebug() << "object : " << obj->objectName() << "has registered in theme : " << currentTheme;
        return false;
    }

    obj->installEventFilter(this);

    theme = new KYBTheme(obj);
    theme->setStyle(normal);
    if (hover.isEmpty()) theme->setStyleHover(normal);
    else theme->setStyleHover(hover);
    if (active.isEmpty()) theme->setStyleActive(normal);
    else theme->setStyleActive(active);
    theme->setStyleDisable(disable);

    if (currentTheme == themeName)
    {
        if (obj->isEnabled()) theme->normal();
        else theme->disable();
    }

    themes.insert(obj->objectName() + themeName, theme);
    themesByType.insertMulti(themeName, theme);
    if (-1 == types.indexOf(themeName)) types << themeName;

    if ("ukui-white" == themeName)
    {
        regTheme(obj, "ukui-default", normal, hover,  active, disable);
        return regTheme(obj, "ukui-light", normal, hover,  active, disable);
    }
    else if ("ukui-black" == themeName)
    {
        return regTheme(obj, "ukui-dark", normal, hover,  active, disable);
    }
    else {}

    return true;
}

void KYBThemeManager::delTheme(QString theme)
{
    if (theme.isEmpty()) return;

    QList<KYBTheme *> ths;

    ths = themesByType.values(theme);
    if (ths.size())
    {
        themesByType.remove(theme);
        types.removeAt(types.indexOf(theme));
    }
    for (int i = 0; i < ths.size(); ++i)
    {
        themes.remove(ths[i]->getObj()->objectName() + theme);
        delete ths[i];
    }
}

void KYBThemeManager::onChangeTheme(QString styleName)
{
    //return;
    qDebug() << "current theme name : " << styleName;
    if (styleName == currentTheme) return;
    if (types.indexOf(styleName) == -1) return;

    QList<KYBTheme *> theme;

    theme = themesByType.values(styleName);
    for (int i = 0; i < theme.size(); ++i)
    {
        if (theme[i]->getObj()->isEnabled()) theme[i]->normal();
        else theme[i]->disable();
        theme[i]->getObj()->update();
    }
    currentTheme = styleName;
}

KYBTheme::KYBTheme()
{}

KYBTheme::KYBTheme(QWidget *obj) : obj(obj)
{}

KYBTheme::~KYBTheme()
{}
