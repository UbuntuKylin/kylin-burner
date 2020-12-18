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
#include "k3bResultDialog.h"
#include "ThemeManager.h"

#include <KLocalizedString>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDebug>
#include <QBitmap>
#include <QPainter>
#include <QEvent>
#include <QStyle>
#include <QApplication>

bool BurnResult::eventFilter(QObject *obj, QEvent *e)
{
    return QDialog::eventFilter(obj, e);
}

void BurnResult::paintEvent(QPaintEvent *e)
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

    QWidget::paintEvent(e);
}

BurnResult::BurnResult( int ret ,QString str, QWidget *parent) :
    QDialog(parent)
{
    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);
    setFixedSize(430, 260);


    QLabel *icon = new QLabel();
    icon->setFixedSize(30,30);
    icon->setPixmap(QIcon::fromTheme("burner").pixmap(icon->size()));
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(80,30);
    QFont f;
    f.setPixelSize(14);
    title->setFont(f);
    c = new QPushButton();
    c->setFlat(true);
    c->setIcon(QIcon::fromTheme("window-close-symbolic"));
    c->setProperty("isWindowButton", 0x2);
    c->setProperty("useIconHighlightEffect", 0x8);
    c->setIconSize(QSize(26, 26));
    c->installEventFilter(this);
    c->setFixedSize(30,30);
    connect(c, SIGNAL(clicked()), this, SLOT(exit() ) );

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    QHBoxLayout *titlebar = new QHBoxLayout(label_top);
    titlebar->setContentsMargins(11, 4, 4, 0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(0);
    titlebar->addWidget(title);
    titlebar->addStretch(290);
    titlebar->addWidget(c);

    QLabel* label_icon = new QLabel();
    label_icon->setFixedSize(50,50);
    label_icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/icon-right.png);"
                              "background-repeat: no-repeat;background-color:transparent;}");
   
    QString string = str + " success!";
    QLabel* label_info = new QLabel( i18n( string.toLatin1().data() ), this );
    f.setPixelSize(30);
    label_info->setFont(f);
    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addSpacing( 100 );
    hlayout->addWidget( label_icon );
    hlayout->addSpacing( 24 );
    hlayout->addWidget( label_info );
    hlayout->addStretch( 0 );
    
    if( !ret ){
        label_icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/icon-error.png);"
                                  "background-repeat: no-repeat;background-color:transparent;}");
        string = str + " failed!";
        label_info->setText( i18n( string.toLatin1().data() ) );
    }
    
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget( label_top );
    mainLayout->addSpacing( 78 );
    mainLayout->addLayout( hlayout );
    mainLayout->addStretch( 0 );

    setModal( true );
}

BurnResult::~BurnResult()
{
   
}

void BurnResult::exit()
{
    this->close();
}

