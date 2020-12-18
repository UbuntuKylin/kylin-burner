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
#include <QStyle>

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

    QFont f = ui->burnerName->font();
    f.setPixelSize(14);
    ui->burnerName->setText(i18n("kylin-burner"));

    ui->burnerBase->setText(i18n("Version: 3.1.0 (based on K3b)"));
    f.setPixelSize(12);
    f.setPixelSize(14);
    ui->copyright->setText(i18n("Package: kylin-burner"));

    ui->labelContent->setText(i18n("Contributors\nwangye@kylinos.cn、biwenjie@kylinos.cn"));

    setWindowModality(Qt::WindowModal);
    setWindowTitle(i18n("about"));
    ui->label->setPixmap(QIcon::fromTheme("burner").pixmap(ui->label->size()));
    ui->label_2->setPixmap(QIcon::fromTheme("burner").pixmap(128, 128));

    ui->labelTitle->setText(i18n("kylin-burner"));
    ui->labelClose->setIcon(QIcon::fromTheme("window-close-symbolic"));
    ui->labelClose->setProperty("isWindowButton", 0x2);
    ui->labelClose->setProperty("useIconHighlightEffect", 0x8);
    ui->labelClose->setIconSize(QSize(26, 26));
    ui->labelClose->installEventFilter(this);

    QScreen *screen = QGuiApplication::primaryScreen ();
    QRect screenRect =  screen->availableVirtualGeometry();
    this->move(screenRect.width() / 2, screenRect.height() / 2);
    this->hide();
    connect(ui->labelClose, SIGNAL(clicked()), this, SLOT(hide()));
}

KylinBurnerAbout::~KylinBurnerAbout()
{
    delete ui;
}

bool KylinBurnerAbout::eventFilter(QObject *obj, QEvent *event)
{
    return QWidget::eventFilter(obj, event);
}

void KylinBurnerAbout::paintEvent(QPaintEvent *e)
{
    QPalette pal = QApplication::style()->standardPalette();
    QColor c;

    c.setRed(231); c.setBlue(231); c.setGreen(231);
    if (c == pal.background().color())
    {
        pal.setColor(QPalette::Background, QColor("#FFFFFF"));
        setPalette(pal);
    }
    else
    {
        setPalette(pal);
    }

    QFont f = ui->burnerName->font();
    f.setPixelSize(14);
    ui->burnerName->setFont(f);
    ui->copyright->setFont(f);
    ui->labelContent->setFont(f);
    f.setPixelSize(12);
    ui->burnerBase->setFont(f);

    QWidget::paintEvent(e);
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
