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
#include "kylinburnerfilefilterselection.h"
#include "ui_kylinburnerfilefilterselection.h"
#include "ThemeManager.h"

#include <QMouseEvent>
#include <QScreen>
#include <QBitmap>
#include <QPainter>
#include <QStyle>

KylinBurnerFileFilterSelection::KylinBurnerFileFilterSelection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerFileFilterSelection)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);
    setWindowFlag (Qt::Dialog);
    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);

    this->setFixedSize(430, 250);

    setWindowTitle(i18n("FilterFile"));

    QScreen *screen = QGuiApplication::primaryScreen ();
    QRect screenRect =  screen->availableVirtualGeometry();
    this->move(screenRect.width() / 2, screenRect.height() / 2);
    this->hide();
    this->hide();
    ui->labelTitle->setText(i18n("Kylin-Burner"));
    ui->labelName->setText(i18n("FilterSetting"));
    ui->label->setPixmap(QIcon::fromTheme("burner").pixmap(ui->label->size()));
    ui->labelClose->setIcon(QIcon::fromTheme("window-close-symbolic"));
    ui->labelClose->setProperty("isWindowButton", 0x2);
    ui->labelClose->setProperty("useIconHighlightEffect", 0x8);
    ui->labelClose->setIconSize(QSize(16, 16));
    ui->labelClose->installEventFilter(this);
}

KylinBurnerFileFilterSelection::~KylinBurnerFileFilterSelection()
{
    delete ui;
}

bool KylinBurnerFileFilterSelection::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent;

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
        mouseEvent = static_cast<QMouseEvent *>(event);
        if (ui->labelClose == obj && (Qt::LeftButton == mouseEvent->button())) this->hide();
        break;
    case QEvent::HoverEnter:
        break;
    case QEvent::HoverLeave:
        break;
    default:
        return QWidget::eventFilter(obj, event);
    }

    return QWidget::eventFilter(obj, event);
}

void KylinBurnerFileFilterSelection::paintEvent(QPaintEvent *event)
{
    QPalette pal = QApplication::style()->standardPalette();
    QPalette spal = palette();
    QColor c;
    //c.setRed(231); c.setBlue(231); c.setGreen(231);
    c.setRed(240); c.setBlue(240); c.setGreen(240);
    if (c == pal.background().color())
    {
        spal.setColor(QPalette::Background, QColor("#FFFFFF"));
        setPalette(spal);
    }
    else
    {
        spal.setColor(QPalette::Background, QColor("#242424"));
        setPalette(spal);
    }
    QWidget::paintEvent(event);
}

void KylinBurnerFileFilterSelection::labelCloseStyle(bool in)
{
    if (in)
    {
        ui->labelClose->setStyleSheet("background-color:rgba(247,99,87,1);"
                                      "border-radius: 4px;");
    }
    else
    {
        ui->labelClose->setStyleSheet("background-color:transparent;"
                                      "border-radius: 4px;");
    }
}

void KylinBurnerFileFilterSelection::on_checkBoxHidden_stateChanged(int arg1)
{
    bool check = false;
    if (arg1 == Qt::Checked)  check = true;
    emit changeSetting(0, check);
}

void KylinBurnerFileFilterSelection::on_checkBoxWrongSymbol_stateChanged(int arg1)
{
    bool check = false;
    if (arg1 == Qt::Checked)  check = true;
    emit changeSetting(1, check);
}

void KylinBurnerFileFilterSelection::on_checkBoxReplaceSymbol_stateChanged(int arg1)
{
    bool check = false;
    if (arg1 == Qt::Checked)  check = true;
    emit changeSetting(2, check);
}

void KylinBurnerFileFilterSelection::setOption(bool hidden, bool broken, bool replace)
{
    if (hidden) ui->checkBoxHidden->setCheckState(Qt::Checked);
    else ui->checkBoxHidden->setCheckState(Qt::Unchecked);
    if (broken) ui->checkBoxWrongSymbol->setCheckState(Qt::Checked);
    else ui->checkBoxWrongSymbol->setCheckState(Qt::Unchecked);
    if (replace) ui->checkBoxReplaceSymbol->setCheckState(Qt::Checked);
    else ui->checkBoxReplaceSymbol->setCheckState(Qt::Unchecked);
}
