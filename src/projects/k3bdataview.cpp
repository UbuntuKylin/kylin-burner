/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdataview.h"
#include "k3bdataburndialog.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataviewimpl.h"
#include "k3bdirproxymodel.h"
#include "k3bmediaselectioncombobox.h"
#include "k3bdiritem.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KActionCollection>
#include <KToolBar>
#include <KConfig>
#include <KSharedConfig>

#include <QDebug>
#include <QUrl>
#include <QAction>
#include <QHeaderView>
#include <QTreeView>
#include <QSplitter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>

#include "misc/k3bimagewritingdialog.h"
#include "k3bapplication.h"
#include "k3bappdevicemanager.h"
#include "k3bmediacache.h"
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageAccess>
#include <QThread>
#include <KMountPoint>
/*
#include <QStyleFactory>
*/

void K3b::DataView::onDataDelete(bool flag)
{
    button_remove->setEnabled(flag);
    if (flag) enableButtonRemove();
    else disableButtonRemove();
}

void K3b::DataView::onDataChange(QModelIndex parent, QSortFilterProxyModel *model)
{
    qDebug() << "Data Changed : " << model->rowCount(parent);
    if (0 == model->rowCount(parent))
    {
        disableButtonRemove();
        disableButtonClear();
        disableButtonBurn();
        disableBurnSetting();
        btnFileFilter->hide();
        //m_dataViewImpl->view()->setFixedHeight(28);
        //if (tips) tips->show();
    }
    else
    {
        enableButtonRemove();
        enableButtonClear();
        enableButtonBurn();
        enableBurnSetting();
        btnFileFilter->show();
        //m_dataViewImpl->view()->setFixedHeight(375);
        //if (tips) tips->hide();
    }
}
K3b::DataView::DataView( K3b::DataDoc* doc, QWidget* parent )
:
    View( doc, parent ),
    m_doc( doc ),
    m_dataViewImpl( new DataViewImpl( this, m_doc, actionCollection() ) ),
    m_dirView( new QTreeView( this ) ),
    m_dirProxy( new DirProxyModel( this ) )
{
    //dlgFileFilter = new KylinBurnerFileFilter(this);
    dlgFileFilter = new KylinBurnerFileFilter
            (parent->parentWidget()->parentWidget()->parentWidget()
             ->parentWidget()->parentWidget());
    m_dirProxy->setSourceModel( m_dataViewImpl->model() );

    connect(m_dataViewImpl, SIGNAL(dataChange(QModelIndex , QSortFilterProxyModel *)),
            this, SLOT(onDataChange(QModelIndex , QSortFilterProxyModel *)));
    connect(m_dataViewImpl, SIGNAL(dataDelete(bool)), this, SLOT(onDataDelete(bool)));

    toolBox()->show();

    // Dir panel
    m_dirView->setFrameStyle(QFrame::NoFrame); //去掉边框

    m_dirView->setRootIsDecorated( false ); //去虚线边框
    m_dirView->setHeaderHidden( true );
    m_dirView->setAcceptDrops( true );
    m_dirView->setDragEnabled( true );
    m_dirView->setDragDropMode( QTreeView::DragDrop );
    m_dirView->setSelectionMode( QTreeView::SingleSelection );
    m_dirView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_dirView->setModel( m_dirProxy );
    m_dirView->expandToDepth( 1 ); // Show first-level directories directories by default
    m_dirView->setColumnHidden( DataProjectModel::TypeColumn, true );
    m_dirView->setColumnHidden( DataProjectModel::SizeColumn, true );
    //*********************************************************************
    m_dirView->setColumnHidden( DataProjectModel::PathColumn, true );
    m_dirView->hide();
/*    
    QFrame *line = new QFrame(this);
    line->setGeometry(QRect(40, 180, 400, 3));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->raise();
*/
    //刻录设置按钮
    burn_setting = new QPushButton(i18n("open"), this);
    burn_setting->setFixedSize(80, 30);
    burn_setting->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;}"
                                "QPushButton:hover{background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#ffffff}"
                                "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 14px;border-radius: 4px;color:#ffffff}");
    //开始刻录按钮
    burn_button = new QPushButton(i18n("create iso"), this);
    burn_button->setFixedSize(140, 45);
    burn_button->setEnabled( false );
    burn_button->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 18px;border-radius: 4px;}");

    QLabel *label_view = new QLabel(this);
    //QGridLayout *layout = new QGridLayout(label_view);
    QVBoxLayout *layout = new QVBoxLayout( label_view );
    layout->setContentsMargins(0, 0, 5, 25);

    QLabel *label_burner = new QLabel(i18n("current burner"), label_view);
    label_burner->setFixedSize(75, 30);

    combo_burner = new QComboBox( label_view );
    combo_burner->setEnabled( false );
    combo_burner->setMinimumSize(310, 30);
    combo_burner->setStyleSheet("QComboBox{background:rgba(255,255,255,1);  border:1px solid rgba(220,221,222,1);border-radius:4px;}"
                            "QComboBox::drop-down{subcontrol-origin: padding; subcontrol-position: top right; \
                             border-top-right-radius: 3px; \
                             border-bottom-right-radius: 3px;}"
                             "QComboBox::down-arrow{width: 8px; height: 16;  padding: 0px 0px 0px 0px;}");

    
    //QLabel *label_space = new QLabel(label);

    QLabel *label_CD = new QLabel( label_view );
    label_CD->setText(i18n("current CD"));
    label_CD->setMinimumSize(75, 30);

    iso_index = 0;

    combo_CD = new QComboBox( label_view );
    combo_CD->setFixedWidth( 310 );
    combo_CD->setEditable( false );
    combo_CD->setMinimumSize(310, 30);
    image_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/kylin_burner.iso";
    combo_CD->addItem(QIcon(":/icon/icon/icon-镜像.png"), i18n("image file: ") + image_path);

    combo_CD->setStyleSheet("QComboBox{background:rgba(255,255,255,1);  border:1px solid rgba(220,221,222,1);border-radius:4px;}"
                            "QComboBox::drop-down{subcontrol-origin: padding; subcontrol-position: top right; \
                             border-top-right-radius: 3px; \ 
                             border-bottom-right-radius: 3px;}"
                             "QComboBox::down-arrow{width: 8px; height: 16;  padding: 0px 0px 0px 0px;}");

    QHBoxLayout *hlayout_burner = new QHBoxLayout();
    hlayout_burner->setContentsMargins(0, 0, 0, 0);
    hlayout_burner->addWidget( label_burner );
    hlayout_burner->addSpacing( 15 );
    hlayout_burner->addWidget( combo_burner );
    hlayout_burner->addStretch( 0 );
    
    QHBoxLayout *hlayout_CD = new QHBoxLayout();
    hlayout_CD->setContentsMargins(0, 0, 0, 0);
    hlayout_CD->addWidget( label_CD );
    hlayout_CD->addSpacing( 15 );
    hlayout_CD->addWidget( combo_CD );
    hlayout_CD->addSpacing( 10 );
    hlayout_CD->addWidget( burn_setting );
    hlayout_CD->addStretch( 0 );
    
    QVBoxLayout *vlayout_combo = new QVBoxLayout();
    vlayout_combo->setContentsMargins(0, 0, 0, 0);
    vlayout_combo->addLayout( hlayout_burner );
    vlayout_combo->addSpacing( 8 );
    vlayout_combo->addLayout( hlayout_CD );
    
    QVBoxLayout *vlayout_button = new QVBoxLayout();
    vlayout_button->setContentsMargins(0, 0, 20, 0);
    vlayout_button->addStretch( 0 );
    vlayout_button->addWidget( burn_button );
    
    QHBoxLayout *hlayout_bottom = new QHBoxLayout();
    hlayout_bottom->setContentsMargins(0, 0, 0, 0);
    hlayout_bottom->addLayout( vlayout_combo );
    hlayout_bottom->addLayout( vlayout_button );

    /*
    tips = new QLabel(
                i18n("Please click button [Add] or drag you file to current zone for new files."),
                this);

    tips->setMinimumHeight(342);
    */
    //tips->hide();
    QVBoxLayout *vlayout_middle = new QVBoxLayout();
    vlayout_middle->setContentsMargins(0, 0, 20, 0);
    vlayout_middle->addWidget( m_dataViewImpl->view() );
    //vlayout_middle->addWidget( tips );

    //layout->addWidget( m_dataViewImpl->view() );
    layout->addLayout( vlayout_middle );
    layout->addStretch( 26 );
    layout->addLayout( hlayout_bottom );
#if 0
    layout->addWidget( m_dataViewImpl->view(), 0, 0, 1, 5 );
    layout->addWidget( line, 1, 0, 1, 5);
    layout->addWidget( label_burner, 2, 0, 1, 1 );
    layout->addWidget( combo_burner, 2, 1, 1, 1 );
    layout->addWidget( label_CD, 3, 0, 1, 1 );
    layout->addWidget( combo_CD, 3, 1, 1, 1 );
    layout->addWidget( burn_setting, 3, 2, 1, 1 );
    layout->addWidget( label_space, 3, 3, 1, 1 );
    layout->addWidget( burn_button, 3, 4, 1, 1 );
    
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 4);
    layout->setColumnStretch(2, 1);
    layout->setColumnStretch(3, 4);
    layout->setColumnStretch(4, 1);
    layout->setRowStretch(0, 4);
    layout->setRowStretch(1, 1);
    layout->setRowStretch(2, 1);
    layout->setRowStretch(3, 1);
    layout->setHorizontalSpacing(15);
    layout->setVerticalSpacing(10);
#endif
    QSplitter* splitter = new QSplitter( this );
    splitter->addWidget( label_view );
    setMainWidget( splitter );
    m_doc->setVolumeID( "data_burn" );
    
    connect( burn_setting, SIGNAL(clicked()), this, SLOT(slotBurn()) );
    connect( burn_button, SIGNAL(clicked()), this, SLOT(slotStartBurn()) );
    connect( combo_burner, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotComboBurner(int) ) );
    connect( combo_CD, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotComboCD(int) ) );
    connect( m_dataViewImpl, SIGNAL(setCurrentRoot(QModelIndex)), this, SLOT(slotSetCurrentRoot(QModelIndex)) );
    connect( m_dirView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotCurrentDirChanged()) );

    if( m_dirProxy->rowCount() > 0 )
        m_dirView->setCurrentIndex( m_dirProxy->index( 0, 0 ) );
    connect( k3bappcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
              this, SLOT(slotMediaChange(K3b::Device::Device*)) );
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3b::Device::DeviceManager*)),
              this, SLOT(slotDeviceChange(K3b::Device::DeviceManager*)) );
    
    /*add menu button*/
    QLabel* label_action = new QLabel( this );
    label_action->setMinimumSize(370, 30);
    
    //添加按钮
    button_add = new QPushButton(i18n("Add"), label_action);
    button_add->setFixedSize(80, 30);
    button_add->setIcon(QIcon(":/icon/icon/icon-添加-默认.png"));
    button_add->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#444444;}" 
                              "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px; color:#ffffff;}"  
                              "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px; color:#ffffff;}");

    //删除按钮
    button_remove = new QPushButton(i18n("Remove"), label_action );
    button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
    button_remove->setFixedSize(80, 30);
    button_remove->setEnabled( false );
    button_remove->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#d1d1d1;}");

    //清空按钮
    button_clear = new QPushButton(i18n("Clear"), label_action );
    button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png")); 
    button_clear->setFixedSize(80, 30);
    button_clear->setEnabled( false );
    button_clear->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#d1d1d1;}");
    //新建文件夹按钮
    button_newdir = new QPushButton(i18n("New Dir"), label_action );
    button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png")); 
    button_newdir->setFixedSize(112, 30);
    button_newdir->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#444444;}" 
                                 "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px; color:#ffffff;}"  
                                 "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px; color:#ffffff;}");


    QLabel *labelFileFilter = new QLabel( this );
    labelFileFilter->setMinimumSize(132, 30);
    btnFileFilter = new QPushButton(i18n("FileFilter"), labelFileFilter);
    btnFileFilter->setFixedSize(112, 30);
    btnFileFilter->setStyleSheet("QPushButton{ background-color:transparent; border-radius:4px; font: 14px; color:#415fc4;}"
                                 "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px; color:#ffffff;}"
                                 "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px; color:#ffffff;}");


    //安装事件过滤器
    button_add->installEventFilter(this); 
    button_remove->installEventFilter(this); 
    button_clear->installEventFilter(this); 
    button_newdir->installEventFilter(this); 

    //QLabel* label_action = new QLabel( this );
    QHBoxLayout* layout_action = new QHBoxLayout( label_action );
    layout_action->setContentsMargins(0,0,0,0);
    layout_action->addWidget( button_add );
    layout_action->addStretch(8);
    layout_action->addWidget( button_remove );
    layout_action->addStretch(8);
    layout_action->addWidget( button_clear );
    layout_action->addStretch(8);
    layout_action->addWidget( button_newdir );


    QHBoxLayout* layoutFileFilter = new QHBoxLayout( labelFileFilter );
    layoutFileFilter->setContentsMargins(20,0,0,0);
    //layoutFileFilter->addStretch(10);
    layoutFileFilter->addWidget( btnFileFilter );

    
    connect( button_add, SIGNAL( clicked() ), this, SLOT( slotOpenClicked() ) );
    connect( button_remove, SIGNAL( clicked() ), this, SLOT( slotRemoveClicked() ) );
    connect( button_clear, SIGNAL( clicked() ), this, SLOT( slotClearClicked()  ) );
    connect( button_newdir, SIGNAL( clicked() ), this, SLOT( slotNewdirClicked() ) );
    connect( btnFileFilter, SIGNAL( clicked() ), this, SLOT( slotFileFilterClicked() ) );

    toolBox()->addWidget( label_action );
    toolBox()->addWidget( labelFileFilter );
    toolBox()->addAction( actionCollection()->action( "project_volume_name" ) );
    btnFileFilter->hide();

    workerThread = new QThread;
    LoadWorker *worker = new LoadWorker;
    worker->moveToThread(workerThread);
    connect(this,SIGNAL(load(QString, DataDoc*)),worker,SLOT(load(QString, DataDoc*)));
    connect(worker,SIGNAL(loadFinished()),this,SLOT(onLoadFinished()));
    connect(workerThread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    workerThread->start();
   
}

K3b::DataView::~DataView()
{
}

void K3b::DataView::slotFileFilterClicked()
{
    K3b::DirItem *d = m_doc->root();
    K3b::DataItem *c;
    qDebug() << m_doc->URL();
    for (int i = 0; i < d->children().size(); ++i)
    {
        c = d->children().at(i);
        if (c)
        {
            if (c->isDir()) qDebug() << "DIR: " << c->localPath() << "---" << c->k3bPath() << "---" << c->k3bName();
            else qDebug() << "Other: " << c->localPath() << "---" << c->k3bPath() << "---" << c->k3bName();
        }
    }
    dlgFileFilter->slotDoFileFilter(m_doc);
    dlgFileFilter->show();
}

bool K3b::DataView::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
        /*
        if(obj == button_add)
            button_add->setIcon(QIcon(":/icon/icon/icon-添加-悬停点击.png"));
        if(obj == button_remove){
            if ( button_remove->isEnabled() )
                button_remove->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
        }
        if(obj == button_clear){
            if ( button_clear->isEnabled() )
                button_clear->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
        }
        if(obj == button_newdir)
            button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-悬停点击.png"));
        */
        if (obj == button_add) hoverButtonAdd();
        else if (obj == button_remove) hoverButtonRemove();
        else if (obj == button_clear) hoverButtonClear();
        else if (obj == button_newdir) hoverButtonNewDir();
        else break;
        break;
    case QEvent::HoverLeave:
        /*
        if(obj == button_add)
            button_add->setIcon(QIcon(":/icon/icon/icon-添加-默认.png"));
        if(obj == button_remove)
            button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
        if(obj == button_clear)
            button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png"));
        if(obj == button_newdir)
            button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png"));
        */
        if (obj == button_add) enableButtonAdd();
        else if (obj == button_remove) enableButtonRemove();
        else if (obj == button_clear) enableButtonClear();
        else if (obj == button_newdir) enableButtonNewDir();
        else break;
        break;
    case QEvent::MouseButtonPress:
        if(obj == button_add)
            button_add->setIcon(QIcon(":/icon/icon/icon-添加-悬停点击.png"));
        if(obj == button_remove){
            if ( button_remove->isEnabled() )
                button_remove->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
        }
        if(obj == button_clear){
            if ( button_clear->isEnabled() )
                button_clear->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
        }
        if(obj == button_newdir)
            button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-悬停点击.png"));
        break;
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

void K3b::DataView::slotOpenClicked()
{
    int ret = m_dataViewImpl->slotOpenDir();
    if( ret ){
        button_remove->setEnabled( true );
        button_remove->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#444444;}" 
                                     "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px; color:#ffffff;}"  
                                     "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px; color:#ffffff;}");
        
        button_clear->setEnabled( true );
        button_clear->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#444444;}" 
                                    "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px; color:#ffffff;}"  
                                    "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px; color:#ffffff;}");
        
        burn_button->setEnabled( true );
        burn_button->setStyleSheet("QPushButton{background-color:rgb(61, 107, 229);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                   "QPushButton:hover{background-color:rgb(107, 142, 235);font: 18px;border-radius: 4px;color: rgb(255,255,255);}"
                                   "QPushButton:pressed{border:none;background-color:rgb(65, 95, 196);font: 18px;border-radius: 4px;color: rgb(255,255,255);}");

    }
}

void K3b::DataView::slotRemoveClicked()
{
    m_dataViewImpl->slotRemove();
}

void K3b::DataView::slotClearClicked()
{
    const QModelIndex parentDirectory = m_dataViewImpl->view()->rootIndex();
    m_dataViewImpl->slotClear();
    if (0 == m_dataViewImpl->view()->model()->rowCount(parentDirectory))
    {
        button_clear->setEnabled(false);
        button_remove->setEnabled(false);
        burn_button->setEnabled(false);
        burn_setting->setEnabled(false);
    }
}

void K3b::DataView::slotNewdirClicked()
{
    m_dataViewImpl->slotNewDir();
}

void K3b::DataView::slotDeviceChange( K3b::Device::DeviceManager* manager )
{
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    if ( device_list.count() == 0 ){
        combo_burner->setEnabled(false);
        combo_CD->setEditable(true);
    }else 
        slotMediaChange( 0 );
}

void K3b::DataView::slotMediaChange( K3b::Device::Device* dev )
{
    QList<K3b::Device::Device*> device_list = k3bappcore->appDeviceManager()->allDevices();
    burn_button->setVisible(true);
    mount_index.clear();
    device_index.clear();
 
    iso_index = 0;

    combo_burner->blockSignals( true );
    combo_burner->clear();
    combo_burner->blockSignals( false );
   
    combo_CD->blockSignals( true );
    combo_CD->clear();
    combo_CD->blockSignals( false );
    combo_CD->setEditable( false );
    
    foreach(K3b::Device::Device* device, device_list)
    {
        combo_burner->setEnabled( true );
        burn_setting->setText(i18n("setting"));
        burn_button->setText(i18n("start burner"));
         
        device_index.append( device );
        QThread::sleep(2);

        K3b::Medium medium = k3bappcore->mediaCache()->medium( device );
        KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( device->blockDeviceName() );
        
        qDebug()<< "device disk state" << device->diskInfo().diskState() <<endl;

        QString mountInfo;
        QString burnerInfo = device->vendor() + " " + device->description();
        QString CDInfo;
        QString CDSize =  KIO::convertSize(device->diskInfo().remainingSize().mode1Bytes());
        
        
        if( !mountPoint){            
            if ( device->diskInfo().diskState() == K3b::Device::STATE_EMPTY ){     
                mountInfo = i18n("empty medium ");            
                CDInfo = i18n("empty medium ") + CDSize;
            } else {
                mountInfo = "no medium ";
                CDInfo = i18n("please insert a medium or empty CD");
            }        
        } else {
            mountInfo = mountPoint->mountPoint(); 
            CDInfo = medium.shortString() + i18n(", remaining available space") + CDSize;
        }
        mount_index.append(mountInfo);
    
        iso_index++;

        combo_burner->addItem(QIcon(":/icon/icon/icon-刻录机.png"), burnerInfo);
        combo_CD->addItem(QIcon(":/icon/icon/icon-光盘.png"), CDInfo);
    }
    
    image_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/kylin_burner.iso";
    combo_CD->addItem(QIcon(":/icon/icon/icon-镜像.png"), i18n("image file: ") + image_path);

}

void K3b::DataView::slotComboBurner(int index)
{
    qDebug()<< " combo burner index " << index << mount_index << endl;
    if ( index < 0 )
       index = 0;

    // 会调用slotComboCD 槽函数    
    combo_CD->setCurrentIndex( index );
}

void K3b::DataView::slotComboCD(int index)
{
    qDebug()<< " combo Cd index " << index << mount_index << iso_index <<endl;
    if ( index < 0 )
        index = 0;

    if ( index == iso_index ){
        m_doc->clear();
        combo_burner->setEnabled( false );
        //combo_CD->setEditable( true );
        burn_setting->setText(i18n("open"));
        burn_button->setText(i18n("create iso"));
    }else{
        combo_burner->setEnabled( true );
        //combo_CD->setEditable( false );
        burn_setting->setText(i18n("setting"));
        burn_button->setText(i18n("start burner"));

        combo_burner->blockSignals( true ); //不发送信号，不会调用slotComboBurner 槽函数
        combo_burner->setCurrentIndex( index );
        combo_burner->blockSignals( false );

        add_device_urls( mount_index.at( index ) );
    }

}

K3b::ProjectBurnDialog* K3b::DataView::newBurnDialog( QWidget* parent )
{
    return new DataBurnDialog( m_doc, parent );
}

//加载标志位
static bool loadFlag = true;
void K3b::DataView::add_device_urls(QString filepath)
{
    if(loadFlag)
    {
        loadFlag = false;
        m_doc->clear();
        //m_doc->root()->setLocalPath(filepath);
        m_doc->setURL(QUrl::fromLocalFile(filepath));
        emit load(filepath, m_doc);
    }
}

void K3b::DataView::slotStartBurn()
{
    DataBurnDialog *dlg = new DataBurnDialog( m_doc, this);
    if( m_doc->burningSize() == 0 ) { 
         KMessageBox::information( this, i18n("Please add files to your project first."),
                                      i18n("No Data to Burn") );
    }else if ( burn_button->text() == i18n("start burner" )){ 
        int index = combo_burner->currentIndex();
        dlg->setComboMedium( device_index.at( index ) );
        qDebug()<< "index :" <<  index << " device block name: " << device_index.at( index )->blockDeviceName() <<endl;
        dlg->slotStartClicked();
    }else if( burn_button->text() == i18n("create iso" )){ 
        dlg->setOnlyCreateImage( true );
        dlg->setTmpPath( image_path );
        QFileInfo fileinfo( image_path );
        QDir dir( fileinfo.path() );
        if ( !dir.exists() )
            return;
        dlg->slotStartClicked();
    }   
    
    delete dlg;
}

void K3b::DataView::slotBurn()
{
       if( m_doc->burningSize() == 0 ) { 
         KMessageBox::information( this, i18n("Please add files to your project first."),
                                      i18n("No Data to Burn") );
    }else if ( burn_setting->text() == i18n("setting") ){
        ProjectBurnDialog* dlg = newBurnDialog( this );
        dlg->execBurnDialog(true);
        delete dlg;
    }else if ( burn_setting->text() == i18n("open" )){
        QString filepath = QFileDialog::getExistingDirectory(this, "open file dialog", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks/* | QFileDialog::DontUseNativeDialog*/);
        if ( filepath.isEmpty() )
            return;
        combo_CD->setEditable( true );
        image_path = filepath + "/kylin_burner.iso";
        combo_CD->setCurrentText(  i18n("image file: ") + image_path );
    } 
}

void K3b::DataView::addUrls( const QList<QUrl>& urls )
{
    m_dataViewImpl->addUrls( m_dirProxy->mapToSource( m_dirView->currentIndex() ), urls );
}

void K3b::DataView::slotParentDir()
{
    m_dirView->setCurrentIndex( m_dirView->currentIndex().parent() );
}

void K3b::DataView::slotCurrentDirChanged()
{
    QModelIndexList indexes = m_dirView->selectionModel()->selectedRows();
    if( indexes.count() ) {
        m_dataViewImpl->slotCurrentRootChanged( m_dirProxy->mapToSource( indexes.first() ) );
    }
}

void K3b::DataView::slotSetCurrentRoot( const QModelIndex& index )
{
    m_dirView->setCurrentIndex( m_dirProxy->mapFromSource( index ) );
}

void K3b::DataView::onLoadFinished() 
{
    loadFlag = true;
    K3b::DirItem* root = static_cast<K3b::DirItem*>(m_doc->root());
    if (root->children().size()) btnFileFilter->show();
}

void K3b::DataView::enableBurnSetting()
{
    burn_setting->setEnabled(true);
    burn_setting->setStyleSheet("background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;color : #444444");
}

void K3b::DataView::hoverBurnSetting()
{
    burn_setting->setStyleSheet("background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::pressBurnSetting()
{
    burn_setting->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableBurnSetting()
{
    //if (burn_setting->isEnabled()) burn_setting->setEnabled(false);
    if (!burn_setting->isEnabled())
    {
        burn_setting->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonBurn()
{
    if (!burn_button->isEnabled()) return;
    burn_button->setStyleSheet("background-color:rgba(61, 107, 229, 1);font: 18px;border-radius: 4px;color:#ffffff");
}

void K3b::DataView::hoverButtonBurn()
{
    if (!burn_button->isEnabled()) return;
    burn_button->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 18px;border-radius: 4px;color:#ffffff");
}

void K3b::DataView::pressButtonBurn()
{
    if (!burn_button->isEnabled()) return;
    burn_button->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 18px;border-radius: 4px;color:#ffffff");
}

void K3b::DataView::disableButtonBurn()
{
    if (burn_button->isEnabled()) burn_button->setEnabled(false);
    if (!burn_button->isEnabled())
    {
        burn_button->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 18px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonAdd()
{
    if (!button_add->isEnabled()) return;
    button_add->setIcon(QIcon(":/icon/icon/icon-添加-默认.png"));
    button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonAdd()
{
    if (!button_add->isEnabled()) return;
    button_add->setIcon(QIcon(":/icon/icon/icon-添加-悬停点击.png"));
    button_add->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::pressButtonAdd()
{
    if (!button_add->isEnabled()) return;
    button_add->setIcon(QIcon(":/icon/icon/icon-添加-悬停点击.png"));
    button_add->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonAdd()
{
    if (button_add->isEnabled()) button_add->setEnabled(false);
    if (!button_add->isEnabled())
    {
        button_add->setIcon(QIcon(":/icon/icon/icon-添加-默认.png"));
        button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonRemove()
{
    if (!button_remove->isEnabled()) return;
    button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
    button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonRemove()
{
    if (!button_remove->isEnabled()) return;
    button_remove->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
    button_remove->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::pressButtonRemove()
{
    if (!button_remove->isEnabled()) return;
    button_remove->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
    button_remove->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonRemove()
{
    if (button_remove->isEnabled()) button_remove->setEnabled(false);
    if (!button_remove->isEnabled())
    {
        button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
        button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonClear()
{
    if (!button_clear->isEnabled()) return;
    button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png"));
    button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonClear()
{
    if (!button_clear->isEnabled()) return;
    button_clear->setIcon(QIcon(":/icon/icon/icon-清空-悬停点击.png"));
    button_clear->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::pressButtonClear()
{
    if (!button_clear->isEnabled()) return;
    button_clear->setIcon(QIcon(":/icon/icon/icon-清空-悬停点击.png"));
    button_clear->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonClear()
{
    if (button_clear->isEnabled()) button_clear->setEnabled(false);
    if (!button_clear->isEnabled())
    {
        button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png"));
        button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonNewDir()
{
    if (!button_newdir->isEnabled()) return;
    button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png"));
    button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonNewDir()
{
    if (!button_newdir->isEnabled()) return;
    button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-悬停点击.png"));
    button_newdir->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::pressButtonNewDir()
{
    if (!button_newdir->isEnabled()) return;
    button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-悬停点击.png"));
    button_newdir->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonNewDir()
{
    if (button_newdir->isEnabled()) button_newdir->setEnabled(false);
    if (!button_newdir->isEnabled())
    {
        button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png"));
        button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::LoadWorker::load(QString path, DataDoc* m_doc)
{
    if ( path == "empty medium" || path == "no medium" || path == NULL)
        return;

    QDir *dir = new QDir(path);
    QList<QFileInfo> fileinfo(dir->entryInfoList( QDir::AllEntries | QDir::Hidden ) );
    for ( int i = 0; i < fileinfo.count(); i++ )
    {
        qDebug() << fileinfo.at(i).fileName();
        if (fileinfo.at(i).fileName() == "." ||
                fileinfo.at(i).fileName() == "..") continue;
        /*
        if( strstr(fileinfo.at(i).filePath().toLatin1().data(), "/.") != NULL)
            continue;
        */
        m_doc->addUnremovableUrls( QList<QUrl>() <<  QUrl::fromLocalFile(fileinfo.at(i).filePath()) );
        //m_doc->addUrls( QList<QUrl>() <<  QUrl::fromLocalFile(fileinfo.at(i).filePath()) );
    }
    emit loadFinished(); //发送信号，修改标志位
}

