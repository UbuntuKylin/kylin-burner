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
#ifndef TITLE_BAR_H
#define TITLE_BAR_H

#include "option/k3boptiondialog.h"
#include "misc/k3bmediaformattingdialog.h"
#include "k3bFileFilterDialog.h"
#include "k3bMd5Dialog.h"
#include "kylinburnerabout.h"

#include <QToolButton>
#include <QLabel>

namespace K3b{
    
    namespace Device {
        class DeviceManager;
        class Device;
    }   

    class AppDeviceManager;

class TitleBar : public QWidget
{
    Q_OBJECT
signals:
    void setIsHidden(bool);
    void setIsBroken(bool);
    void setIsReplace(bool);

public:
    explicit TitleBar(QWidget *parent = 0);
    ~TitleBar();
    
    void formatMedium( K3b::Device::Device* );
    K3b::Device::Device* testDev;

protected:

    /*
    // 双击标题栏进行界面的最大化/还原
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

    // 进行鼠界面的拖动
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
*/
    // 进行最小化、最大化/还原、关闭操作
public Q_SLOT:    
    void onClicked();
    void popup();
    void clean();
    void md5();
    void filter();
    void help();
    void about();
public slots:
    void isHidden(bool);
    void isBroken(bool);
    void isReplace(bool);
    void callHidden(bool);
    void callBroken(bool);
    void callReplace(bool);

private:
    QPoint mLastMousePosition;
    bool mMoving;
    // 最大化/还原
    void updateMaximize();

private:
    QLabel *m_pIconLabel;
    QLabel *m_pTitleLabel;
    QToolButton *m_pMenubutton;
    QPushButton *m_pMinimizeButton;
    QPushButton *m_pMaximizeButton;
    QPushButton *m_pCloseButton;
    FileFilter  *dlg;
    KylinBurnerAbout *abouta;
    K3b::MediaFormattingDialog *mfDlg;
    K3b::Md5Check *dialog;
};
}
#endif // TITLE_BAR_H

