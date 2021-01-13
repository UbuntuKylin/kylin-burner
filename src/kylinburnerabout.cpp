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
#include <QDesktopServices>

KylinBurnerAbout::KylinBurnerAbout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerAbout)
{
    ui->setupUi(this);
    this->hide();

    setFixedSize(420, 560);

    setWindowFlag(Qt::Dialog);
    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);

    QFont f = ui->labelTitle->font();
    f.setPixelSize(14);
    ui->labelTitle->setText(i18n("kylin-burner"));
    f.setWeight(28);
    f.setPixelSize(18);
    ui->labelName->setFont(f);
    ui->labelName->setText(i18n("kylin-burner"));
    f.setWeight(24);
    f.setPixelSize(14);
    ui->labelVersion->setFont(f);
    ui->labelVersion->setText(i18n("Version: 3.1.0 (based on K3b)"));
    f.setPixelSize(12);
    f.setPixelSize(14);
    setWindowModality(Qt::WindowModal);
    setWindowTitle(i18n("about"));
    ui->labelLogo->setPixmap(QIcon::fromTheme("burner").pixmap(ui->labelLogo->size()));
    ui->labelIcon->setPixmap(QIcon::fromTheme("burner").pixmap(96, 96));

    ui->labelTitle->setText(i18n("kylin-burner"));
    ui->btnClose->setIcon(QIcon::fromTheme("window-close-symbolic"));
    ui->btnClose->setProperty("isWindowButton", 0x2);
    ui->btnClose->setProperty("useIconHighlightEffect", 0x8);
    ui->btnClose->setIconSize(QSize(16, 16));
    ui->btnClose->installEventFilter(this);
    ui->btnClose->setFlat(true);

    ui->textEdit->setText(i18n("Kylin Burner is a lightweight burning software based on the secondary development of open source burning software K3b. With K3b burning as the basic core and Kylin Burner's own interface display, Kylin Burner makes its interface simple, easy to operate and easy to use, bringing users a more refreshing burning experience."));

    ui->labelOfficalWebsite->setText("区域" + i18n("Offical Website : ") +
                                     "<a href=\"http://www.kylinos.cn\">"
                                     "www.kylinos.cn</a>");
    ui->labelSupport->setText(i18n("Service & Technology Support : ") +
                              "<a href=\"mailto://support@kylinos.cn\">"
                              "support@kylinos.cn</a>");
    ui->labelHotLine->setText(i18n("Hot Service : ") +
                              "<a href=#>400-089-1870</a>");

    QScreen *screen = QGuiApplication::primaryScreen ();
    QRect screenRect =  screen->availableVirtualGeometry();
    this->move(screenRect.width() / 2, screenRect.height() / 2);
    this->hide();
    connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(hide()));

    connect(ui->labelOfficalWebsite, SIGNAL(linkActivated(QString)),
            this, SLOT(slotsOpenURL(QString)));
    connect(ui->labelSupport, SIGNAL(linkActivated(QString)),
            this, SLOT(slotsOpenURL(QString)));
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

    QFont f = ui->labelTitle->font();
    f.setPixelSize(14);
    ui->labelTitle->setFont(f);
    //f.setWeight(28);
    f.setPixelSize(18);
    ui->labelName->setFont(f);
    //f.setWeight(24);
    f.setPixelSize(14);
    ui->labelVersion->setFont(f);
    pal.setColor(QPalette::WindowText, QColor("#595959"));
    f.setFamily(font().family());
    ui->labelVersion->setPalette(pal);
    ui->textEdit->setFont(f);
    ui->textEdit->setPalette(pal);
    ui->labelOfficalWebsite->setFont(f);
    ui->labelOfficalWebsite->setPalette(pal);
    ui->labelSupport->setFont(f);
    ui->labelSupport->setPalette(pal);
    ui->labelHotLine->setFont(f);
    ui->labelHotLine->setPalette(pal);

    QWidget::paintEvent(e);
}

void KylinBurnerAbout::slotsOpenURL(QString u)
{
    QUrl url(u);
    QDesktopServices::openUrl(url);
}

#if 0
void KylinBurnerAbout::on_btnClose_clicked()
{
    hide();
}
#endif
