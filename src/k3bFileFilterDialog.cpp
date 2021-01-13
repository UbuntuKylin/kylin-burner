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
#include <QEvent>

#include <QStyle>
#include <QDebug>

FileFilter::FileFilter(QWidget *parent) :
    QDialog(parent)
{
    //this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->setFixedSize(430, 250);
    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);


    QLabel *icon = new QLabel();
    icon->setFixedSize(24,24);
    icon->setPixmap(QIcon::fromTheme("burner").pixmap(icon->size()));
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(80,30);
    QFont f;
    f.setPixelSize(14);
    title->setFont(f);
    c = new QPushButton();
    c->setFixedSize(30,30);
    connect(c, &QPushButton::clicked, this, &FileFilter::filter_exit);

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(8, 4, 4, 0);
    titlebar->setSpacing(0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(8);
    titlebar->addWidget(title);
    titlebar->addStretch(285);
    titlebar->addWidget(c);
    c->installEventFilter(this);
    c->setIcon(QIcon::fromTheme("window-close-symbolic"));
    c->setProperty("isWindowButton", 0x2);
    c->setProperty("useIconHighlightEffect", 0x8);
    c->setIconSize(QSize(16, 16));
    c->setFlat(true);
    //titlebar->addSpacing(5);
    
    QLabel *filter_label = new QLabel( i18n("filterSet") );
    filter_label->setFixedHeight(25);
    f = filter_label->font();
    f.setPixelSize(24);
    filter_label->setFont(f);
    discard_hidden_file = new QCheckBox( i18n("discard hidden file") );
    //discard_hidden_file->setChecked(true);
    discard_hidden_file->setFixedHeight(16);
    QFont label_font;
    label_font.setPixelSize(14);
    discard_hidden_file->setFont( label_font );
    
    discard_broken_link = new QCheckBox( i18n("discard broken link") );
    //discard_broken_link->setChecked(true);
    discard_broken_link->setFixedHeight(18);
    discard_broken_link->setFont( label_font );
    
    follow_link = new QCheckBox( i18n("follow link") );
    follow_link->setChecked(false);
    follow_link->setFixedHeight(18);
    follow_link->setFont( label_font );

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

bool FileFilter::eventFilter(QObject *obj, QEvent *e)
{
    return QDialog::eventFilter(obj, e);
}

void FileFilter::paintEvent(QPaintEvent *e)
{
    QPalette pal = style()->standardPalette();
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
