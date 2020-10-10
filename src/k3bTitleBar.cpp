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

#include <QDebug>

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

    /*
    m_pIconLabel->setFixedSize(30,30);
    m_pIconLabel->setStyleSheet("QLabel{background-image: url(:/new/prefix1/pic/logo.png);"
                                "background-color:rgb(255,255,255);"
                                "background-repeat: no-repeat;}");
    */

    m_pTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_pMenubutton->setFixedSize(30, 30);
    m_pMinimizeButton->setFixedSize(30, 30);
    //m_pMaximizeButton->setFixedSize(30, 30);
    m_pCloseButton->setFixedSize(30, 30);

    m_pMenubutton->setObjectName("menuButton");
    m_pMinimizeButton->setObjectName("minimizeButton");
    //m_pMaximizeButton->setObjectName("maximizeButton");
    m_pCloseButton->setObjectName("closeButton");

    m_pMenubutton->setToolTip(i18n("Menu__"));
    m_pMinimizeButton->setToolTip(i18n("Minimize__"));
    //m_pMaximizeButton->setToolTip(i18n("Maximize__"));
    m_pCloseButton->setToolTip(i18n("Close__"));

    //m_pTitleLabel->setContentsMargins(8,0,0,0);
    //m_pTitleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);


    //m_pMenubutton->setIcon( QIcon::fromTheme( "setting-default" ) );
    m_pMenubutton->setStyleSheet("QToolButton{border-image: url(:/icon/icon/icon-设置-默认.png);"
                                 "border:none;background-color:transparent;"
                                 "border-radius: 4px;}"
                                "QToolButton:hover{border-image: url(:/icon/icon/icon-设置-悬停点击.png);"
                                 "border:none;background-color:rgb(61, 107, 229);"
                                 "border-radius: 4px;}"
                                "QToolButton:checked{border-image: url(:/icon/icon/icon-设置-悬停点击.png);"
                                 "border:none;background-color:rgb(50, 87, 202);"
                                 "border-radius: 4px;}"
                                 "QToolButton::menu-indicator{image:none;}");

    m_pMinimizeButton->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-最小化-默认.png);"
                                     "border:none;background-color:transparent;"
                                     "border-radius: 4px;}"
                                "QPushButton:hover{border-image: url(:/icon/icon/icon-最小化-悬停点击.png);"
                                     "border:none;background-color:rgb(61, 107, 229);"
                                     "border-radius: 4px;}"
                                "QPushButton:pressed{border-image: url(:/icon/icon/icon-最小化-悬停点击.png);"
                                     "border:none;background-color:rgb(50, 87, 202);"
                                     "border-radius: 4px;}");

    /*
    m_pMaximizeButton->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-最大化-默认.png);"
                                     "border:none;background-color:transparent;"
                                     "border-radius: 4px;}"
                                "QPushButton:hover{border-image: url(:/icon/icon/icon-最大化-悬停点击.png);"
                                     "border:none;background-color:rgb(61, 107, 229);"
                                     "border-radius: 4px;}"
                                "QPushButton:checked{border-image: url(:/icon/icon/icon-最大化-悬停点击.png);"
                                     "border:none;background-color:rgb(50, 87, 202);"
                                     "border-radius: 4px;}");
    */
    m_pCloseButton->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-关闭-默认.png);"
                                  "border:none;background-color:transparent;"
                                  "border-radius: 4px;}"
                                "QPushButton:hover{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                                  "border:none;background-color:rgb(248, 100, 87);"
                                  "border-radius: 4px;}"
                                 "QPushButton:pressed{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                                    "border:none;background-color:rgba(228, 76, 80, 1);"
                                    "border-radius: 4px;}"
                                "QPushButton:checked{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                                  "border:none;background-color:rgb(228, 236, 80);"
                                  "border-radius: 4px;}");

    QMenu *menu = new QMenu(this);  //新建菜单

    menu->addAction(QIcon(""), i18n("popup"), this,&TitleBar::popup);
    menu->addAction(QIcon(""), i18n("Clean"), this,&TitleBar::clean);
    menu->addSeparator();
    menu->addAction(QIcon(""), i18n("MD5"), this,&TitleBar::md5);
    menu->addAction(QIcon(""), i18n("filter"), this, &TitleBar::filter);
    menu->addSeparator();
    menu->addAction(QIcon(""), i18n("help"), this,&TitleBar::help);
    menu->addAction(QIcon(""), i18n("about"), this,&TitleBar::about);

    menu->setObjectName("menu");
    ThManager()->regTheme(menu, "ukui-white","#menu{background-color: rgba(233, 233, 233, 1);"
                                             "border: none; border-radius: 4px;}"
                                             "#menu::item{background-color: transparent;"
                                             "padding: 8px 5px; margin: 0px 4px;"
                                             "border: none; border-radius: 4px;"
                                             "font: 14px; color: #444444;height: 14px;}"
                                             "#menu::item:selected{background-color: rgba(107, 141, 235, 1);"
                                             "border-bottom: 1px solid gray; color:rgba(61, 107, 229, 1);}"
                                             "#menu::item:pressed{background-color: rgba(65, 95, 195, 1);"
                                             "color: rgba(61, 107, 229, 1);}");
    ThManager()->regTheme(menu, "ukui-black","#menu{background-color: rgba(57, 58, 62, 1);"
                                             "border: none; border-radius: 4px;}"
                                             "#menu::item{background-color: transparent;"
                                             "padding: 8px 5px; margin: 0px 4px;"
                                             "border: none; border-radius: 4px;"
                                             "font: 14px; color: #FFFFFF;height: 14px;}"
                                             "#menu::item:selected{background-color: rgba(107, 141, 235, 1);"
                                             "border-bottom: 1px solid gray; color:rgba(61, 107, 229, 1);}"
                                             "#menu::item:pressed{background-color: rgba(65, 95, 195, 1);"
                                             "color: rgba(61, 107, 229, 1);}");
/*
    menu->setStyleSheet("#menu{background-color: #FFFFFF; "
                        "border: none; border-radius: 4px;"
                        "font: 14px; color: #444444;}");

    menu->setStyleSheet("#menu::item{background-color: transparent;"
                        "border: none; border-radius: 4px;"
                        "font: 14px; color: #444444;height: 14px;"
                        "padding: 8px 32px; margin: 0px 8px;}"
                        "#menu::item:selected{background-color: red;}"
                        "#menu::item:pressed{background-color: blue;}");
*/
    //menu->setStyleSheet("QMenu::item:hover{background-color:#6b8eeb;}");
    //menu->setStyleSheet("QMenu:hover{background-color:#000000;}");

    m_pMenubutton->setPopupMode(QToolButton::InstantPopup); //点击模式
    m_pMenubutton->setMenu(menu);  //下拉菜单


    //QHBoxLayout *mainWidgetLayout = new QHBoxLayout(this);
    //QWidget *mainWidget = new QWidget;
    QLabel* label_top = new QLabel( this );
    ThManager()->regTheme(this, "ukui-white", "QWidget{background-color: rgb(255, 255, 255);}");
    ThManager()->regTheme(this, "ukui-black", "QWidget{background-color: rgb(0, 0, 0);}");
    label_top->setStyleSheet("QWidget{background-color: transparent;}");
    label_top->setFixedHeight( 50 );
    QHBoxLayout *pLayout = new QHBoxLayout( label_top );

    //mainWidgetLayout->addWidget(mainWidget);
    //mainWidget->setLayout(pLayout);
    //mainWidgetLayout->setMargin(0);
    pLayout->setContentsMargins( 0, 0, 4, 16);
    pLayout->setSpacing(0);
    pLayout->addStretch(0);
    pLayout->addWidget(m_pMenubutton);
    pLayout->addSpacing(4);
    pLayout->addWidget(m_pMinimizeButton);
    //pLayout->addWidget(m_pMaximizeButton);
    pLayout->addSpacing(4);
    pLayout->addWidget(m_pCloseButton);
    //pLayout->addSpacing(3);

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


    //m_pMaximizeButton->hide();
    connect(m_pMenubutton, &QPushButton::clicked, this, &TitleBar::onClicked);
    connect(m_pMinimizeButton, &QPushButton::clicked, this, &TitleBar::onClicked);
    //connect(m_pMaximizeButton, &QPushButton::clicked, this, &TitleBar::onClicked);
    connect(m_pCloseButton, &QPushButton::clicked, this, &TitleBar::onClicked);
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
    dlg->show();
}
void K3b::TitleBar::help()
{

    QDBusMessage msg = QDBusMessage::createMethodCall( "com.kylinUserGuide.hotel_1000",
                                                    "/",
                                                    "com.guide.hotel",
                                                    "showGuide");
    msg << "burner";
    qDebug() << msg;
    QDBusMessage response = QDBusConnection::sessionBus().call(msg);

    qDebug() << response;
}
void K3b::TitleBar::about()
{
    abouta->show();
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
