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

#include "ThemeManager.h"

#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QDebug>

#include "k3b.h"
#include "k3bTitleBar.h"
#include "k3bappdevicemanager.h"
#include "k3bdevice.h"
#include "k3bapplication.h"
#include <KMountPoint>
#include <klocalizedstring.h>
#include <QtDBus>

K3b::TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{

    setFixedHeight(50);

    m_pIconLabel = new QLabel(this);
    m_pTitleLabel = new QLabel(this);
    m_pMenubutton = new QToolButton(this);
    m_pMinimizeButton = new QPushButton(this);
    //m_pMaximizeButton = new QPushButton(this);
    m_pCloseButton = new QPushButton(this);

    m_pCloseButton->setProperty("isWindowButton", 0x2);
    m_pMinimizeButton->setProperty("isWindowButton", 0x1);
    m_pMenubutton->setProperty("isWindowButton", 0x1);

    m_pCloseButton->setProperty("useIconHighlightEffect", 0x8);
    m_pMinimizeButton->setProperty("useIconHighlightEffect", 0x2);
    m_pMenubutton->setProperty("useIconHighlightEffect", 0x2);
    m_pMenubutton->setPopupMode(QToolButton::InstantPopup);

    m_pCloseButton->setFlat(true);
    m_pMinimizeButton->setFlat(true);
    m_pMenubutton->setAutoRaise(true);

    m_pCloseButton->installEventFilter(this);
    m_pMinimizeButton->installEventFilter(this);
    m_pMenubutton->installEventFilter(this);

    //m_pTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_pMenubutton->setFixedSize(30, 30);
    m_pMinimizeButton->setFixedSize(30, 30);
    //m_pMaximizeButton->setFixedSize(30, 30);
    m_pCloseButton->setFixedSize(30, 30);

    m_pMenubutton->setToolTip(i18n("Menu__"));
    m_pMinimizeButton->setToolTip(i18n("Minimize__"));
    //m_pMaximizeButton->setToolTip(i18n("Maximize__"));
    m_pCloseButton->setToolTip(i18n("Close__"));

    m_pCloseButton->setIconSize(QSize(16 , 16));
    m_pCloseButton->setIcon(QIcon::fromTheme("window-close-symbolic"));
    m_pMinimizeButton->setIconSize(QSize(16 , 16));
    m_pMinimizeButton->setIcon(QIcon::fromTheme("window-minimize-symbolic"));
    m_pMenubutton->setIconSize(QSize(16 , 16));
    m_pMenubutton->setIcon(QIcon::fromTheme("open-menu-symbolic") );



    menu = new QMenu(this);  //新建菜单
    menu->setMinimumWidth(160);
    menu->setAutoFillBackground(true);

    eject = menu->addAction(i18n("popup"), this,&TitleBar::popup);
    eject->setEnabled(false);
    earse = menu->addAction(i18n("Clean"), this,&TitleBar::clean);
    earse->setEnabled(false);
    menu->addSeparator();
    menu->addAction(i18n("MD5"), this,&TitleBar::md5);
    QAction *f = menu->addAction( i18n("filter"), this, &TitleBar::filter);
    f->setToolTip(i18n("Only invalid at level root."));
    menu->addSeparator();
    menu->addAction(i18n("help"), this,&TitleBar::help);
    menu->addAction(i18n("about"), this,&TitleBar::about);
    menu->addSeparator();
    menu->addAction(i18n("exit"), this,[=](){exit(0);});


    #if 0
    ThManager()->regTheme(menu, "ukui-white","#menu{background-color: rgba(255, 255, 255, 1);"
                                             "border: 1px solid rgba(0, 0, 0, 0.6); border-radius: 4px;}"
                                             "#menu::item{background-color: transparent;"
                                             "padding: 8px 5px; margin: 0px 4px;"
                                             "border: none; border-radius: 4px;"
                                             "font: 14px; color: rgba(68, 68, 68, 1);height: 14px;}"
                                             "#menu::item:selected{background-color: rgba(218, 227, 250, 1);"
                                             "color:rgba(68, 68, 68, 1);}"
                                             "#menu::item:pressed{background-color: rgba(218, 227, 250, 1);"
                                             "color: rgba(68, 68, 68, 1);}");
    ThManager()->regTheme(menu, "ukui-black","#menu{background-color: rgba(57, 58, 62, 1);"
                                             "border: none; border-radius: 4px;}"
                                             "#menu::item{background-color: transparent;"
                                             "padding: 8px 5px; margin: 0px 4px;"
                                             "border: none; border-radius: 4px;"
                                             "font: 14px; color: #FFFFFF;height: 14px;}"
                                             "#menu::item:selected{background-color: rgba(107, 141, 235, 1);"
                                             "color:rgba(61, 107, 229, 1);}"
                                             "#menu::item:pressed{background-color: rgba(65, 95, 195, 1);"
                                             "color: rgba(61, 107, 229, 1);}");

#endif

    m_pMenubutton->setMenu(menu);  //下拉菜单


    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight( 30 );
    QHBoxLayout *pLayout = new QHBoxLayout( label_top );

    pLayout->setContentsMargins( 0, 0, 0, 16);
    pLayout->setSpacing(0);
    pLayout->addStretch(0);
    pLayout->addWidget(m_pMenubutton);
    pLayout->addSpacing(4);
    pLayout->addWidget(m_pMinimizeButton);
    pLayout->addSpacing(4);
    pLayout->addWidget(m_pCloseButton);

    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setContentsMargins( 0, 4, 4, 0);
    mainLayout->addWidget( label_top );
    mainLayout->addStretch( 0 );

    dlg = new FileFilter( this );
    mfDlg = new K3b::MediaFormattingDialog( this );
    dialog = new K3b::Md5Check( this );
    abouta = new KylinBurnerAbout(this);


    connect(dlg, SIGNAL(setHidden(bool)), this, SLOT(callHidden(bool)));
    connect(dlg, SIGNAL(setBroken(bool)), this, SLOT(callBroken(bool)));
    connect(dlg, SIGNAL(setReplace(bool)), this, SLOT(callReplace(bool)));


    connect(m_pMenubutton, &QPushButton::clicked, this, &TitleBar::onClicked);
    connect(m_pMinimizeButton, &QPushButton::clicked, this, &TitleBar::onClicked);
    connect(m_pCloseButton, &QPushButton::clicked, this, &TitleBar::onClicked);
}

bool K3b::TitleBar::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}

void K3b::TitleBar::paintEvent(QPaintEvent *e)
{
    QPalette pal = QApplication::style()->standardPalette();

    QColor c;
    c.setRed(231); c.setBlue(231); c.setGreen(231);
    if (c == pal.background().color())
    {
        pal.setBrush(QPalette::Background, QColor("#FFFFFF"));
        menu->setPalette(pal);
    }
    else
    {
        pal.setBrush(QPalette::Background, QColor("#484848"));
        menu->setPalette(pal);
    }
    QWidget::paintEvent(e);
}

K3b::TitleBar::~TitleBar()
{
   delete m_pIconLabel;
   delete m_pTitleLabel;
   delete m_pMenubutton;
   delete m_pMinimizeButton;
   //delete m_pMaximizeButton;
   delete m_pCloseButton;
   if (dlg) delete dlg;
   if (mfDlg) delete mfDlg;
   if (dialog) delete dialog;
   if (abouta) delete abouta;
}

void K3b::TitleBar::callHidden(bool flag)
{
    qDebug() << "call hidden" << flag;
    emit setIsHidden(flag);
}

void K3b::TitleBar::callBroken(bool flag)
{
    emit setIsBroken(flag);
}

void K3b::TitleBar::callReplace(bool flag)
{
    emit setIsReplace(flag);
}
/*
void K3b::TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    //Q_EMIT m_pMaximizeButton->clicked();
}

void K3b::TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if( event->buttons().testFlag(Qt::LeftButton) && mMoving)
    {
        QWidget *pWindow = this->window();
        pWindow->move(pWindow->pos() + (event->globalPos() - mLastMousePosition));
        mLastMousePosition = event->globalPos();
    }
}

void K3b::TitleBar::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        mMoving = true;
        mLastMousePosition = event->globalPos();
    }
}

void K3b::TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        mMoving = false;
    }
}
*/
void K3b::TitleBar::onClicked()
{
    QPushButton *pButton = qobject_cast<QPushButton *>(sender());
    QWidget *pWindow = this->window();
    if (pWindow->isTopLevel())
    {
        if (pButton == m_pMinimizeButton)
        {
            pWindow->showMinimized();
            updateMaximize();
        }
        /*
        else if (pButton == m_pMaximizeButton)
        {
            pWindow->isMaximized() ? pWindow->showNormal() : pWindow->showMaximized();
            updateMaximize();
        }
        */
        else if (pButton == m_pCloseButton)
        {
            //m_pCloseButton->setIcon(QIcon(":/icon/icon/icon-关闭-悬停点击.png"));
            exit(0);
            pWindow->close();
        }
    }
}

void K3b::TitleBar::updateMaximize()
{
    QWidget *pWindow = this->window();
    if (pWindow->isTopLevel())
    {
        bool bMaximize = pWindow->isMaximized();
        if (bMaximize)
        {
            //m_pMaximizeButton->setToolTip(tr("Restore"));
            //m_pMaximizeButton->setProperty("maximizeProperty", "restore");
        }
        else
        {
            //m_pMaximizeButton->setProperty("maximizeProperty", "maximize");
            //m_pMaximizeButton->setToolTip(tr("Maximize"));
        }

        //m_pMaximizeButton->setStyle(QApplication::style());
    }
}

void K3b::TitleBar::clean()
{
    formatMedium( testDev );
}

void K3b::TitleBar::formatMedium( K3b::Device::Device* dev )
{
    mfDlg->setDevice( dev );
    mfDlg->exec();
}

void K3b::TitleBar::popup()
{
    QList<K3b::Device::Device*> device_list = k3bappcore->appDeviceManager()->allDevices();
    foreach(K3b::Device::Device* dev, device_list){
        if ( dev ){
            KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() );
            if ( dev->diskInfo().diskState() == K3b::Device::STATE_EMPTY ){
                dev->eject();
                break;
            }
            if( mountPoint ){
                dev->eject();
                break;
            }
        }
    }
}

void K3b::TitleBar::md5()
{
    dialog->show();
}

void K3b::TitleBar::filter()
{
    QMessageBox::about(nullptr, i18n("filter"), i18n("Only invalid at level root."));
    dlg->show();
}
void K3b::TitleBar::help()
{
    QDBusMessage msg = QDBusMessage::createMethodCall( "com.kylinUserGuide.hotel_1000",
                                                    "/",
                                                    "com.guide.hotel",
                                                    "showGuide");
    //QFileInfo f("/usr/share/kylin-user-guide/data/guide-ubuntukylin/kylin-burner");
    //if (f.isDir()) msg << "kylin-burner";
    //else msg << "burner";
    //qDebug() << msg;
    msg << "kylin-burner";
    QDBusMessage response = QDBusConnection::sessionBus().call(msg);

    qDebug() << response;
}
void K3b::TitleBar::about()
{
    abouta->show();
    QPoint p(k3bappcore->k3bMainWindow()->pos().x() + (k3bappcore->k3bMainWindow()->width() - abouta->width()) / 2,
             k3bappcore->k3bMainWindow()->pos().y() + (k3bappcore->k3bMainWindow()->height() - abouta->height()) / 2);
    qDebug() << p;
    abouta->move(p);
}

void K3b::TitleBar::isHidden(bool flag)
{
    qDebug() << "Set title bar dialog hidden to " << flag;
    dlg->setIsHidden(flag);
}

void K3b::TitleBar::isBroken(bool flag)
{
    dlg->setIsBroken(flag);
}

void K3b::TitleBar::isReplace(bool flag)
{
    dlg->setIsReplace(flag);
}
