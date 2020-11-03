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

#include "k3bFileFilterDialog.h"
#include "ThemeManager.h"

#include <KLocalizedString>
#include <KConfig>
#include <KSharedConfig>
#include <KIO/Global>
#include <KConfigGroup>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QBitmap>

#include <QDebug>

FileFilter::FileFilter(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->setFixedSize(430, 250);

    /*
    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(pal);
    */

    this->setObjectName("MenuFileFilter");
    ThManager()->regTheme(this, "ukui-white", "#MenuFileFilter{background-color: #FFFFFF;"
                                              "border: 1px solid gray;border-radius: 6px;}");
    ThManager()->regTheme(this, "ukui-black", "#MenuFileFilter{background-color: #000000;"
                                              "border: 1px solid gray;border-radius: 6px;}");

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRoundedRect(bmp.rect(), 6, 6);
    setMask(bmp);

    QLabel *icon = new QLabel();
    icon->setFixedSize(30,30);
    icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/logo.png);"
                        "background-repeat: no-repeat;background-color:transparent;}");
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(80,30);
    title->setObjectName("FilterTitile");
    ThManager()->regTheme(title, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(title, "ukui-black", "font: 14px; color: #FFFFFF;");
    QPushButton *close = new QPushButton();
    close->setFixedSize(30,30);
    close->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-关闭-默认.png);"
                         "border:none;background-color:rgb(233, 233, 233);"
                         "border-radius: 4px;background-color:transparent;}"
                          "QPushButton:hover{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                         "border:none;background-color:rgb(248, 100, 87);"
                         "border-radius: 4px;}");
    connect(close, &QPushButton::clicked, this, &FileFilter::filter_exit);

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(11, 4, 4, 0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(5);
    titlebar->addWidget(title);
    titlebar->addStretch(285);
    titlebar->addWidget(close);
    //titlebar->addSpacing(5);
    
    QLabel *filter_label = new QLabel( i18n("filterSet") );
    filter_label->setFixedHeight(25);
    filter_label->setStyleSheet("QLabel{background-color:transparent;\
                                        background-repeat: no-repeat;\
                                        width:96px;\
                                        height:24px;\
                                        font-size:24px;\
                                        font-family:Microsoft YaHei;\
                                        font-weight:400;\
                                        color:rgba(68,68,68,1);\
                                        line-height:32px;}");


    filter_label->setObjectName("FilterLabelTitile");
    ThManager()->regTheme(filter_label, "ukui-white", "font: 24px; color: #444444;");
    ThManager()->regTheme(filter_label, "ukui-black", "font: 24px; color: #FFFFFF;");

    discard_hidden_file = new QCheckBox( i18n("discard hidden file") );
    //discard_hidden_file->setChecked(true);
    discard_hidden_file->setFixedHeight(16);
    QFont label_font;
    label_font.setPixelSize(14);
    discard_hidden_file->setFont( label_font );
    discard_hidden_file->setStyleSheet("color:#444444;");
    discard_hidden_file->setObjectName("HiddenFile");
    ThManager()->regTheme(discard_hidden_file, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(discard_hidden_file, "ukui-black", "font: 14px; color: #FFFFFF;");

    
    discard_broken_link = new QCheckBox( i18n("discard broken link") );
    //discard_broken_link->setChecked(true);
    discard_broken_link->setFixedHeight(18);
    discard_broken_link->setFont( label_font );
    discard_broken_link->setStyleSheet("color:#444444;");
    discard_broken_link->setObjectName("BrokenLink");
    ThManager()->regTheme(discard_broken_link, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(discard_broken_link, "ukui-black", "font: 14px; color: #FFFFFF;");
    
    follow_link = new QCheckBox( i18n("follow link") );
    follow_link->setChecked(false);
    follow_link->setFixedHeight(18);
    follow_link->setFont( label_font );
    follow_link->setStyleSheet("color:#444444;");
    follow_link->setObjectName("FollowLink");
    ThManager()->regTheme(follow_link, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(follow_link, "ukui-black", "font: 14px; color: #FFFFFF;");

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(25, 0, 0, 0);
    vlayout->addWidget(filter_label);
    vlayout->addSpacing(28);
    vlayout->addWidget( discard_hidden_file);
    vlayout->addSpacing(13);
    vlayout->addWidget( discard_broken_link);
    vlayout->addSpacing(13);
    vlayout->addWidget(follow_link);
    vlayout->addStretch(0);


    QVBoxLayout *mainWidgetLayout = new QVBoxLayout(this);
    mainWidgetLayout->setContentsMargins(0, 0, 0, 0);
    mainWidgetLayout->addWidget( label_top );
    mainWidgetLayout->addSpacing(24);
    mainWidgetLayout->addLayout(vlayout);

    this->setModal( true );

    connect(discard_hidden_file, SIGNAL(stateChanged(int)), this, SLOT(hiddenChanged(int)));
    connect(discard_broken_link, SIGNAL(stateChanged(int)), this, SLOT(brokenChanged(int)));
    connect(follow_link, SIGNAL(stateChanged(int)), this, SLOT(replaceChanged(int)));

}

FileFilter::~FileFilter()
{
    if (discard_hidden_file) delete discard_hidden_file;
}

void FileFilter::hiddenChanged(int stat)
{
    qDebug() << "hidden changed : " << stat;
    bool check = false;
    if (Qt::Checked == stat) check = true;
    else check = false;
    emit setHidden(check);
}

void FileFilter::brokenChanged(int stat)
{
    qDebug() << "broken changed : " << stat;
    if (Qt::Checked == stat) emit setBroken(true);
    else emit setBroken(false);
}

void FileFilter::replaceChanged(int stat)
{
    qDebug() << "replace changed : " << stat;
    if (Qt::Checked == stat) emit setReplace(true);
    else emit setReplace(false);
}

void FileFilter::setIsHidden(bool flag)
{
    discard_hidden_file->setChecked(flag);
}

void FileFilter::setIsBroken(bool flag)
{
    discard_broken_link->setChecked(flag);
}

void FileFilter::setIsReplace(bool flag)
{
    follow_link->setChecked(flag);
}

void FileFilter::filter_exit()
{
    KConfigGroup grp( KSharedConfig::openConfig(), "default data settings" );   
    grp.writeEntry( "discard hidden file", discard_hidden_file->isChecked() );
    grp.writeEntry( "discard symlinks", false );
    grp.writeEntry( "discard broken symlinks", discard_broken_link->isChecked() );
    grp.writeEntry( "follow symbolic links", follow_link->isChecked() );
    
    this->close();
}
