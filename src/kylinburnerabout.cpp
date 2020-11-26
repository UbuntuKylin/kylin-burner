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

#include "xatom-helper.h"
#include "kylinburnerabout.h"
#include "ui_kylinburnerabout.h"
#include "ThemeManager.h"

#include <QScreen>
#include <QMouseEvent>
#include <QBitmap>
#include <QPainter>

KylinBurnerAbout::KylinBurnerAbout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerAbout)
{
    ui->setupUi(this);
    this->hide();

    setFixedSize(450, 300);

    setWindowFlag(Qt::Dialog);
    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);

    setObjectName("AboutDialog");
#if 1
    ThManager()->regTheme(ui->aboutBackground, "ukui-white", "#aboutBackground{background-color: #FFFFFF;"
                                              "}");
    ThManager()->regTheme(ui->aboutBackground, "ukui-black", "#aboutBackground{background-color: #000000;"
                                              "");
#endif
    ThManager()->regTheme(ui->labelTitle, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(ui->labelTitle, "ukui-black", "font: 14px; color: #FFFFFF;");

    ui->burnerName->setText(i18n("kylin-burner"));
    ThManager()->regTheme(ui->burnerName, "ukui-white", "color: #444444; "
                                                        "font: 1000 20pt \"Noto Sans CJK SC\";");
    ThManager()->regTheme(ui->burnerName, "ukui-black", "color: #FFFFFF;"
                                                        "font: 1000 20pt \"Noto Sans CJK SC\";");

    //ui->burnerBase->setText(i18n("Version 3.1.0 (based on K3b, Thanks.)"));
    ui->burnerBase->setText(i18n("Version: 3.1.0 (based on K3b)"));
    ThManager()->regTheme(ui->burnerBase, "ukui-white", "color: #444444; font: 14px;");
    ThManager()->regTheme(ui->burnerBase, "ukui-black", "color: #FFFFFF; font: 14px;");

    //ui->copyright->setText(i18n("Copyright (C) 2020  KylinSoft Co., Ltd."));
    ui->copyright->setText(i18n("Package: kylin-burner"));
    ThManager()->regTheme(ui->copyright, "ukui-white", "color: #444444; font: 14px;");
    ThManager()->regTheme(ui->copyright, "ukui-black", "color: #FFFFFF; font: 14px;" );

    /*ui->labelContent->setText(i18n("Modify & develop by \n    Team Desktop.Beijing KylinSoft Co., Ltd."
                                   "\n Thanks for using and making a suggestion."));*/
    ui->labelContent->setText(i18n("Contributors\nwangye@kylinos.cn、biwenjie@kylinos.cn"));
    ThManager()->regTheme(ui->labelContent, "ukui-white", "color: #444444; font: 14px;");
    ThManager()->regTheme(ui->labelContent, "ukui-black", "color: #FFFFFF; font: 14px;" );

#if 0
    ui->btnClose->setText(i18n("Close__"));
    ThManager()->regTheme(ui->btnClose, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(67, 67, 67, 1);",
                                                         "background-color: rgba(107, 141, 235, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(65, 95, 195, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(ui->btnClose, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
#endif
    setWindowModality(Qt::WindowModal);
    setWindowTitle(i18n("about"));

    ui->labelTitle->setText(i18n("kylin-burner"));
    ui->labelClose->setIcon(QIcon(":/icon/icon/icon-关闭-默认.png"));
    ui->labelClose->setProperty("isWindowButton", 0x2);
    ui->labelClose->setProperty("useIconHighlightEffect", 0x8);
    ui->labelClose->setIconSize(QSize(26, 26));
    ui->labelClose->installEventFilter(this);

    QScreen *screen = QGuiApplication::primaryScreen ();
    QRect screenRect =  screen->availableVirtualGeometry();
    this->move(screenRect.width() / 2, screenRect.height() / 2);
    this->hide();
}

KylinBurnerAbout::~KylinBurnerAbout()
{
    delete ui;
}

bool KylinBurnerAbout::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent;

    switch (event->type())
    {
    case QEvent::MouseButtonRelease:
        mouseEvent = static_cast<QMouseEvent *>(event);
        if (ui->labelClose == obj && (Qt::LeftButton == mouseEvent->button()))
            this->hide();
        break;
    case QEvent::HoverEnter:
        if (ui->labelClose == obj)
            ui->labelClose->setIcon(QIcon(":/icon/icon/icon-关闭-悬停点击.png"));
        break;
    case QEvent::HoverLeave:
        if (ui->labelClose == obj)
            ui->labelClose->setIcon(QIcon(":/icon/icon/icon-关闭-默认.png"));
        break;
    default:
        return QWidget::eventFilter(obj, event);
    }

    return QWidget::eventFilter(obj, event);
}

void KylinBurnerAbout::labelCloseStyle(bool in)
{
    if (in)
    {
        ui->labelClose->setStyleSheet("background-color:rgba(247,99,87,1);"
                                      "image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                                      "border-radius: 4px;");
    }
    else
    {
        ui->labelClose->setStyleSheet("background-color:transparent;"
                                      "image: url(:/icon/icon/icon-关闭-默认.png); "
                                      "border-radius: 4px;");
    }
}
#if 0
void KylinBurnerAbout::on_btnClose_clicked()
{
    hide();
}
#endif
