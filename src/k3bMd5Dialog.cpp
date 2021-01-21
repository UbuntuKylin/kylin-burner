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
#include "k3bMd5Dialog.h"

#include "k3bappdevicemanager.h"
#include "k3bdevice.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "ThemeManager.h"
#include <KMountPoint>
#include <KLocalizedString>

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPainter>
#include <QBitmap>
#include <QGraphicsDropShadowEffect>
#include <QEvent>

#include "k3bResultDialog.h"

K3b::Md5Check::Md5Check(QWidget *parent) :
    QDialog(parent)
{
    this->setFixedSize(430, 380);

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
    c->setFlat(true);
    c->setIcon(QIcon::fromTheme("window-close-symbolic"));
    c->setProperty("isWindowButton", 0x2);
    c->setProperty("useIconHighlightEffect", 0x8);
    c->setIconSize(QSize(16, 16));
    connect(c, &QPushButton::clicked, this, &Md5Check::exit);

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(8, 4, 4, 0);
    titlebar->addWidget(icon);
    //titlebar->addSpacing(5);
    titlebar->addWidget(title);
    titlebar->addStretch(290);
    titlebar->addWidget(c);
    //titlebar->addSpacing(5);

    QLabel* label_title = new QLabel( i18n("md5 check"),this );
//    label_title->setFixedHeight(24);
    QFont label_font;
    label_font.setPixelSize(24);
    label_title->setFont( label_font );
    
    QLabel* label_CD = new QLabel( i18n("choice CD"),this );
//    label_CD->setFixedHeight(12);
    label_font.setPixelSize(14);
    label_CD->setFont( label_font );

    combo = new QComboBox( this );
    combo->setEnabled( false );
    combo->setFixedSize( 368, 30);
    combo->setFont( label_font );

    check = new QCheckBox( i18n("choice md5 file"), this );
    //check->setFixedSize( 125, 11);
    check->setChecked( true );
    check->setFont( label_font );


    lineedit = new QLineEdit( this );
    lineedit->setFixedSize( 278, 30);
    lineedit->setFont( label_font );
    lineedit->setEnabled(true);  
    button_open = new QPushButton( this );
    button_open->setText( i18n("Browse") );
    button_open->setFixedSize( 80, 30);


    button_open->setEnabled(true);    

    button_ok = new QPushButton( this );
    button_ok->setText( i18n("ok") );
    button_ok->setFixedSize( 80, 31);
    QPushButton* button_cancel = new QPushButton( this );
    button_cancel->setText( i18n("cancel") );
    button_cancel->setFixedSize( 80, 31);
     
    QHBoxLayout* hlayout_lineedit = new QHBoxLayout();
    hlayout_lineedit->setContentsMargins(0, 0, 0, 0);
    hlayout_lineedit->addWidget( lineedit );
    hlayout_lineedit->addSpacing( 10 );
    hlayout_lineedit->addWidget( button_open );
    
    QHBoxLayout* hlayout_button = new QHBoxLayout();
    hlayout_button->setContentsMargins(0, 0, 0, 0);
    hlayout_button->addStretch( 0 );
    hlayout_button->addWidget( button_cancel );
    hlayout_button->addSpacing( 8 );
    hlayout_button->addWidget( button_ok );

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(31, 0, 30, 30);
    vlayout->addWidget( label_title );
    vlayout->addSpacing( 30 );
    vlayout->addWidget( label_CD );
    vlayout->addSpacing( 10 );
    vlayout->addWidget( combo );
    vlayout->addSpacing( 30 );
    vlayout->addWidget( check );
    vlayout->addSpacing( 10 );
    vlayout->addLayout( hlayout_lineedit );
    vlayout->addSpacing( 70 );
    vlayout->addLayout( hlayout_button );

    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget( label_top );
    mainLayout->addSpacing( 30 );
    mainLayout->addLayout( vlayout );
    mainLayout->addStretch( 0 );

    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
              this, SLOT(slotMediaChange(K3b::Device::Device*)) );
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
              this, SLOT(slotDeviceChange(K3b::Device::DeviceManager*)) );
    connect(button_open, &QPushButton::clicked, this, &Md5Check::openfile);
    connect(button_ok, &QPushButton::clicked, this, &Md5Check::md5_start);
    connect(button_cancel, &QPushButton::clicked, this, &Md5Check::exit);

    connect(check, SIGNAL(stateChanged(int)), this, SLOT(checkChange(int)));


    slotMediaChange( 0 );
    setModal( true );
}

K3b::Md5Check::~Md5Check()
{

}

bool K3b::Md5Check::checkMd5(const char* cmd)
{
    array<char, 1024>buffer;

    bool result = false;
    unique_ptr<FILE, decltype (&pclose)> pipe(popen(cmd,"r"), pclose);
    if(!pipe){
        qDebug() << __FUNCTION__ << __LINE__ << " Popen() failed!!! ";
        return false;
    }

    bool errFlag = false;
    bool runFlag = false;
    while(fgets(buffer.data(),buffer.size(),pipe.get()) != nullptr){
        runFlag = true;
        QString str = QString(buffer.data());
        result = str.contains("成功");
        if(!result)
        {
           errFlag = true; 
           break;
        }
    }
    if(errFlag && runFlag)
        result = true;

    return result;
}


void K3b::Md5Check::md5_start()
{
     int index = combo->currentIndex();
     if (mount_index.size() == 0) return;
     QString mountPoint = mount_index.at( index );

    qDebug() << __FUNCTION__ << __LINE__ << "mountPoint :" << mountPoint;
    if(mountPoint.isEmpty()){
        return ;
    }

    //切换当前工作目录
    bool result = QDir::setCurrent(mountPoint);
    if(!result)
    {
        BurnResult* dialog = new BurnResult( result , "md5");
        dialog->show();
        return ;
    }

    array<char, 1024>buffer;

    QString cmd = "md5sum -c ";
    QString fileName = "md5sum.txt";
    
    //选中复选框
    if(check->isChecked()){
        fileName = lineedit->text();
        if(fileName.isEmpty())
        {
            BurnResult* dialog = new BurnResult( false , "md5");
            dialog->show();
            return ;
        }
    }
    cmd += fileName;

    qDebug() << __FUNCTION__ << __LINE__ << "cmd :" << cmd;

    result = checkMd5(cmd.toUtf8().data());
    
    qDebug() << __FUNCTION__ << __LINE__ << "result :" << result;
    
    BurnResult* dialog = new BurnResult( result , "md5");
    dialog->show();
}

bool K3b::Md5Check::eventFilter(QObject *obj, QEvent *e)
{
    return QDialog::eventFilter(obj, e);
}

void K3b::Md5Check::paintEvent(QPaintEvent *e)
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

void K3b::Md5Check::openfile()
{
    QString str;
    str = QFileDialog::getOpenFileName(this, i18n("choice md5 file"),
                                       "/home","All files(*.*)", 0/*,
                                   QFileDialog::DontUseNativeDialog*/);
    
    if(str == NULL)
        return;
    lineedit->setText(str);

}

void K3b::Md5Check::exit()
{
    close();
}

void K3b::Md5Check::slotDeviceChange( K3b::Device::DeviceManager* manager )
{
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    if ( device_list.count() == 0 ){
        combo->setEnabled( false );
        lineedit->setEnabled( false );
    }else 
        slotMediaChange( 0 );
}

void K3b::Md5Check::slotMediaChange( K3b::Device::Device* dev )
{
    QList<K3b::Device::Device*> device_list = k3bappcore->appDeviceManager()->allDevices();
    combo->setEnabled( true );

    combo->clear();
    mount_index.clear();
    //device_index.clear();

    foreach(K3b::Device::Device* device, device_list)
    {
        //device_index.append( device );

        K3b::Medium medium = k3bappcore->mediaCache()->medium( device );
        KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( device->blockDeviceName() );

        qDebug()<< "device disk state" << device->diskInfo().diskState() <<endl;
        
        QString mountInfo;
        QString burnerInfo = device->vendor() + " " + device->description();
        QString CDInfo;
        QString CDSize =  KIO::convertSize(device->diskInfo().remainingSize().mode1Bytes());

        if ( device->diskInfo().diskState() == K3b::Device::STATE_EMPTY ){
            mountInfo = "empty medium ";
            CDInfo = i18n("empty medium ") + CDSize;
        }

        if( !mountPoint ){
            mountInfo = "no medium ";
            CDInfo = i18n("please insert a medium or empty CD");
        } else {
            mountInfo = mountPoint->mountPoint();
            CDInfo = medium.shortString() + i18n(" remaining available space  ") + CDSize;
        }
        mount_index.append(mountInfo);

        combo->addItem(QIcon(":/icon/icon/icon-光盘.png"), CDInfo);
    }
}

void K3b::Md5Check::checkChange(int state)
{
    if(state){
        lineedit->setEnabled(true);  
        button_open->setEnabled(true);
        button_ok->setEnabled(true);
        button_open->setStyleSheet("QPushButton{background-color:#e9e9e9;font: 14px;border-radius: 4px;color: #444444;}"
                              "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color: rgb(255,255,255);}"
                              "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color: rgb(255,255,255);}");
        button_ok->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 14px;border-radius: 4px;color:rgb(255,255,255);}"
                              "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color: rgb(255,255,255);}"
                              "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color: rgb(255,255,255);}");
    }else{
        lineedit->setEnabled(false);
        button_open->setEnabled(false);
        button_ok->setEnabled(false);
        button_open->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
        button_ok->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}
