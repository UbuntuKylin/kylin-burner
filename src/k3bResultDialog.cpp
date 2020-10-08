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

BurnResult::BurnResult( int ret ,QString str, QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setFixedSize(430, 260);

    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(pal);

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRoundedRect(bmp.rect(), 6, 6);
    setMask(bmp);

    setObjectName("ResultDialog");
    ThManager()->regTheme(this, "ukui-white", "#ResultDialog{background-color: #FFFFFF;}");
    ThManager()->regTheme(this, "ukui-black", "#ResultDialog{background-color: #000000;}");

    QLabel *icon = new QLabel();
    icon->setFixedSize(30,30);
    icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/logo.png);"
                        "background-repeat: no-repeat;background-color:transparent;}");
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(80,30);
    title->setObjectName("ResultTitle");
    ThManager()->regTheme(title, "ukui-white", "background-color: transparent; font: 14px; color: #444444;");
    ThManager()->regTheme(title, "ukui-black", "background-color: transparent; font: 14px; color: #FFFFFF;");
    QPushButton *close = new QPushButton();
    close->setFixedSize(30,30);
    close->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-关闭-默认.png);"
                         "border:none;background-color:rgb(233, 233, 233);"
                         "border-radius: 4px;background-color:transparent;}"
                          "QPushButton:hover{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                         "border:none;background-color:rgb(248, 100, 87);"
                         "border-radius: 4px;}");
    connect(close, SIGNAL(clicked()), this, SLOT(exit() ) );

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    QHBoxLayout *titlebar = new QHBoxLayout(label_top);
    titlebar->setContentsMargins(11, 4, 4, 0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(0);
    titlebar->addWidget(title);
    titlebar->addStretch(290);
    titlebar->addWidget(close);

    QLabel* label_icon = new QLabel();
    label_icon->setFixedSize(50,50);
    label_icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/icon-right.png);"
                              "background-repeat: no-repeat;background-color:transparent;}");
   
    QString string = str + " success!";
    QLabel* label_info = new QLabel( i18n( string.toLatin1().data() ), this );
    label_info->setObjectName("ResultInfo");
    ThManager()->regTheme(title, "ukui-white", "background-color: transparent; font: 30px; color: #444444;");
    ThManager()->regTheme(title, "ukui-black", "background-color: transparent; font: 30px; color: #FFFFFF;");
    label_info->setStyleSheet("font: 30px");
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

