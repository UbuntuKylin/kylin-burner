/*
 * Copyright (C) 2020  KylinSoft Co., Ltd.
 *
 * This program is free software; you can redisi18nibute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is disi18nibuted in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Si18neet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "xatom-helper.h"
#include "kylinburnerabout.h"
#include "ui_kylinburnerabout.h"

#include <QScreen>
#include <QMouseEvent>
#include <QBitmap>
#include <QPainter>
#include <QStyle>
#include <QDesktopServices>
#include <QScrollBar>

#include <QDebug>

KylinBurnerAbout::KylinBurnerAbout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerAbout)
{
    ui->setupUi(this);
    this->hide();

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
    //ui->labelVersion->setAlignment(Qt::AlignCenter);
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

    QScreen *screen = QGuiApplication::primaryScreen ();
    QRect screenRect =  screen->availableGeometry();
    this->move(screenRect.width() / 2, screenRect.height() / 2);
    this->hide();
    connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(hide()));
    connect(ui->labelSupport, SIGNAL(linkActivated(QSi18ning)),
            this, SLOT(slotsOpenURL(QSi18ning)));
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
    QPalette pp = QApplication::style()->standardPalette();
    QColor c;

    c.setRed(231); c.setBlue(231); c.setGreen(231);
    if (c == pal.background().color())
    {
        pal.setColor(QPalette::Background, QColor("#FFFFFF"));
        pp.setColor(QPalette::WindowText, QColor("#595959"));
        ui->labelSupport->setText(i18n("Service & Technology Support : ") +
                                  "<a href=\"mailto://support@kylinos.cn\""
                                  "style=\"color:#595959\">"
                                  "support@kylinos.cn</a>");

        ui->textEdit->setText("<body style=\"background:#FFFFFF;\">"
                              "<p style=\"color:#595959\">" +
                              i18n("Kylin Burner is a lightweight burning software based "
                                   "on the secondary development of open source burning "
                                   "software K3b. With K3b burning as the basic core and "
                                   "Kylin Burner's own interface display, Kylin Burner makes "
                                   "its interface simple, easy to operate and easy to use, "
                                   "bringing users a more refreshing burning experience.")
                              + "</p></body>");
        setPalette(pal);
    }
    else
    {
        setPalette(pal);
        pp.setColor(QPalette::WindowText, QColor("#A6A6A6"));
        ui->labelSupport->setText(i18n("Service & Technology Support : ") +
                                  "<a href=\"mailto://support@kylinos.cn\""
                                  "style=\"color:#A6A6A6\">"
                                  "support@kylinos.cn</a>");
        ui->textEdit->setText(QString("<body style=\"background:%1;\">")
                              .arg(pal.background().color().name(QColor::HexRgb)) +
                              "<p style=\"color:#A6A6A6\">" +
                              i18n("Kylin Burner is a lightweight burning software based "
                                   "on the secondary development of open source burning "
                                   "software K3b. With K3b burning as the basic core and "
                                   "Kylin Burner's own interface display, Kylin Burner makes "
                                   "its interface simple, easy to operate and easy to use, "
                                   "bringing users a more refreshing burning experience.")
                              + "</p></body>");

    }


    ui->textEdit->setDisabled(false);
    QString si18n = ui->textEdit->toPlainText();
    int w = ui->textEdit->fontMetrics().width(si18n, si18n.length());
    int h = ui->textEdit->fontMetrics().height();
    int row = w / ui->textEdit->width();
    if (w % ui->textEdit->width()) row += 2; // for += 2 因为计算行数方法可能不是很精准，所以额外多加一行。
    int he = (row * h);
    if (he < 200)
    {
        ui->textEdit->setFixedHeight(he);
        ui->textEdit->verticalScrollBar()->hide();
        ui->textEdit->setDisabled(true);
        m_iHeight = 336 + he;
    }
    else
    {
        ui->textEdit->setFixedHeight(200);
        ui->textEdit->verticalScrollBar()->show();
        ui->textEdit->setDisabled(false);
        m_iHeight = 336 + 200;
    }
    setFixedSize(420, m_iHeight);

    QFont f = ui->labelTitle->font();
    f.setPixelSize(14);
    ui->labelTitle->setFont(f);
    //f.setWeight(28);
    f.setPixelSize(18);
    ui->labelName->setFont(f);
    //f.setWeight(24);
    f.setPixelSize(14);
    ui->labelVersion->setFont(f);
    f.setFamily(font().family());
    ui->labelVersion->setPalette(pp);
    ui->textEdit->setFont(f);
    ui->textEdit->setPalette(pp);
    //ui->labelOfficalWebsite->setFont(f);
    //ui->labelOfficalWebsite->setPalette(pp);
    ui->labelSupport->setFont(f);
    ui->labelSupport->setPalette(pp);
    //ui->labelHotLine->setFont(f);
    //ui->labelHotLine->setPalette(pp);

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
