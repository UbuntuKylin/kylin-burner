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

#include "k3bResultDialog.h"

K3b::Md5Check::Md5Check(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->setFixedSize(430, 380);

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

    QLabel *icon = new QLabel();
    icon->setFixedSize(30,30);
    icon->setStyleSheet("QLabel{background-image: url(:/icon/icon/logo.png);"
                        "background-repeat: no-repeat;background-color:transparent;}");
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(80,30);
    title->setStyleSheet("QLabel{background-color:transparent;"
                         "background-repeat: no-repeat;color:#444444;"
                         "font: 14px;}");
    title->setObjectName("MD5Titile");
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
    connect(close, &QPushButton::clicked, this, &Md5Check::exit);

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(34);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(11, 4, 4, 0);
    titlebar->addWidget(icon);
    //titlebar->addSpacing(5);
    titlebar->addWidget(title);
    titlebar->addStretch(290);
    titlebar->addWidget(close);
    //titlebar->addSpacing(5);

    QLabel* label_title = new QLabel( i18n("md5 check"),this );
//    label_title->setFixedHeight(24);
    QFont label_font;
    label_font.setPixelSize(24);
    label_title->setFont( label_font );
    label_title->setStyleSheet("color:#444444");
    label_title->setObjectName("MD5LabelTitile");
    ThManager()->regTheme(label_title, "ukui-white", "font: 24px; color: #444444;");
    ThManager()->regTheme(label_title, "ukui-black", "font: 24px; color: #FFFFFF;");
    
    QLabel* label_CD = new QLabel( i18n("choice CD"),this );
//    label_CD->setFixedHeight(12);
    label_font.setPixelSize(14);
    label_CD->setFont( label_font );
    label_CD->setStyleSheet("color:#444444");
    label_CD->setObjectName("MD5LabelCD");
    ThManager()->regTheme(label_CD, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(label_CD, "ukui-black", "font: 14px; color: #FFFFFF;");

    combo = new QComboBox( this );
    combo->setEnabled( false );
    combo->setFixedSize( 368, 30);
    combo->setFont( label_font );
    combo->setStyleSheet("color:#444444;");
    combo->setObjectName("MD5Combo");
    ThManager()->regTheme(combo, "ukui-white","#MD5Combo{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#MD5Combo::hover{border:1px solid #6B8EEB;}"
                                                 "#MD5Combo::selected{border:1px solid #6B8EEB;}"
                                                 "#MD5Combo QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #FFFFFF;border:1px solid #DCDDDE;}"
                                                 //"#MD5Combo QAbstractItemView::hover{"
                                                 //"padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 //"background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#MD5Combo QAbstractItemView::item{"
                                                 "background-color: #DAE3FA;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#MD5Combo QAbstractItemView::item::hover{border: none;"
                                                 "background-color: #3D6BE5;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#MD5Combo::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#MD5Combo::down-arrow{image: url(:/icon/icon/draw-down.jpg); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#MD5Combo QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#MD5Combo QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#MD5Combo QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#MD5Combo QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#MD5Combo{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#MD5Combo::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");
    ThManager()->regTheme(combo, "ukui-black","#comboISO{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;"
                                                 "background-color: #242424;}"
                                                 "#comboISO::hover{border:1px solid #6B8EEB;"
                                                 "background-color: rgba(0, 0, 0, 0.15);}"
                                                 "#comboISO::selected{border:1px solid #6B8EEB;"
                                                 "background-color: #242424}"
                                                 "#comboISO QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #242424;border:1px solid #DCDDDE;}"
                                                 "#comboISO QAbstractItemView::hover{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#comboISO QAbstractItemView::item{"
                                                 "background-color: rgba(0, 0, 0, 0.15);border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO QAbstractItemView::item::hover{"
                                                 "background-color: #3D6BE5;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#comboISO::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#comboISO::down-arrow{image: url(:/lb/icon_xl.png); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#comboISO QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#comboISO QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#comboISO QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#comboISO QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#comboISO{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#comboISO::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");

    check = new QCheckBox( i18n("choice md5 file"), this );
    //check->setFixedSize( 125, 11);
    check->setChecked( true );
    check->setFont( label_font );
    check->setStyleSheet("color:#444444;");
    check->setObjectName("MD5Check");
    ThManager()->regTheme(check, "ukui-white", "font: 14px; color: #444444;");
    ThManager()->regTheme(check, "ukui-black", "font: 14px; color: #FFFFFF;");


    lineedit = new QLineEdit( this );
    lineedit->setFixedSize( 278, 30);
    lineedit->setFont( label_font );
    lineedit->setStyleSheet("color:#444444;");
    lineedit->setEnabled(true);  
    lineedit->setObjectName("MD5LineEdit");
    ThManager()->regTheme(lineedit, "ukui-white", "#MD5LineEdit{background-color: transparent;"
                                                      "border:1px solid rgba(220,221,222,1);"
                                                      "border-radius:4px;}");
    ThManager()->regTheme(lineedit, "ukui-black", "#MD5LineEdit{background-color: transparent;"
                                                      "border:1px solid rgba(255, 255, 255);"
                                                      "border-radius:4px;}");
 
    button_open = new QPushButton( this );
    button_open->setText( i18n("Browse") );
    button_open->setFixedSize( 80, 30);

    button_open->setStyleSheet("QPushButton{background-color:#e9e9e9;font: 14px;border-radius: 4px;color: #444444;}"
                          "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color: rgb(255,255,255);}"
                          "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color: rgb(255,255,255);}");
    button_open->setObjectName("MD5Browse");
    ThManager()->regTheme(button_open, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(67, 67, 67, 1);",
                                                         "background-color: rgba(107, 141, 235, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(65, 95, 195, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(button_open, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");

    button_open->setEnabled(true);    

    button_ok = new QPushButton( this );
    button_ok->setText( i18n("ok") );
    button_ok->setFixedSize( 80, 31);
    button_ok->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 14px;border-radius: 4px;color:rgb(255,255,255);}"
                          "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color: rgb(255,255,255);}"
                          "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color: rgb(255,255,255);}");
    button_ok->setObjectName("MD5IDOK");
    ThManager()->regTheme(button_ok, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(67, 67, 67, 1);",
                                                         "background-color: rgba(107, 141, 235, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(65, 95, 195, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(button_ok, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
    QPushButton* button_cancel = new QPushButton( this );
    button_cancel->setText( i18n("cancel") );
    button_cancel->setFixedSize( 80, 31);
    button_cancel->setStyleSheet("QPushButton{background-color:#e9e9e9;font: 14px;border-radius: 4px;color: #444444;}"
                          "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color: rgb(255,255,255);}"
                          "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color: rgb(255,255,255);}");
    button_cancel->setObjectName("MD5IDCancel");
    ThManager()->regTheme(button_cancel, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(67, 67, 67, 1);",
                                                         "background-color: rgba(107, 141, 235, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(65, 95, 195, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(61, 107, 229, 1);",
                                                         "background-color: rgba(233, 233, 233, 1);"
                                                         "border: none; border-radius: 4px;"
                                                         "font: 14px \"MicrosoftYaHei\";"
                                                         "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(button_cancel, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
     
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

    result = checkMd5(cmd.toLatin1().data());
    
    qDebug() << __FUNCTION__ << __LINE__ << "result :" << result;
    
    BurnResult* dialog = new BurnResult( result , "md5");
    dialog->show();
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
