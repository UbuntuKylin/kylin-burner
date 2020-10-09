/*
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd. <Derek_Wang39@163.com>
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
#include "ThemeManager.h"

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
#include <QListView>
#include <QLineEdit>
#include <QDropEvent>
#include <QMimeData>
/*
#include <QStyleFactory>
*/

void K3b::DataView::onDataDelete(bool flag)
{
    //button_remove->setEnabled(flag);
    if (flag) enableButtonRemove();
    else disableButtonRemove();
}

void K3b::DataView::onDataChange(QModelIndex parent, QSortFilterProxyModel *model)
{
    disableButtonRemove();
    disableButtonClear();
    if (0 == model->rowCount(parent))
    {
        disableButtonRemove();
        disableButtonClear();
        disableButtonBurn();
        disableBurnSetting();
        btnFileFilter->hide();
        m_dataViewImpl->view()->setFixedHeight(28);
        if (tips) tips->show();
    }
    else
    {
        //enableButtonRemove();
        //enableButtonClear();
        enableButtonBurn();
        enableBurnSetting();
        btnFileFilter->show();        
        if (tips) tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
    }
}

void K3b::DataView::addDragFiles(QList<QUrl> urls, K3b::DirItem* targetDir)
{
    for (int i = 0; i < docs.size(); ++i)
        docs[i]->addUrls(urls);
    copyData(m_doc, docs[combo_CD->currentIndex()]);
}

K3b::DataView::DataView( K3b::DataDoc* doc, QWidget* parent )
:
    View( doc, parent ),
    m_doc( doc ),
    m_dataViewImpl( new DataViewImpl( this, m_doc, actionCollection() ) ),
    m_dirView( new QTreeView( this ) ),
    m_dirProxy( new DirProxyModel( this ) )
{
    lastDrop.clear();
    logger = LogRecorder::instance().registration(i18n("Data Burner").toStdString().c_str());
    logger->debug("Draw data burner begin...");
    //dlgFileFilter = new KylinBurnerFileFilter(this);
    mainWindow = parent->parentWidget()->parentWidget()->parentWidget()
            ->parentWidget()->parentWidget();
    dlgFileFilter = new KylinBurnerFileFilter(mainWindow);

    //connect(dlgFileFilter, SIGNAL(finished(K3b::DataDoc *)), this, SLOT(slotFinish(K3b::DataDoc *)));
    connect(dlgFileFilter, SIGNAL(setOption(int, bool)), this, SLOT(slotOption(int, bool)));

    m_dirProxy->setSourceModel( m_dataViewImpl->model() );

    connect(m_dataViewImpl, SIGNAL(dataChange(QModelIndex , QSortFilterProxyModel *)),
            this, SLOT(onDataChange(QModelIndex , QSortFilterProxyModel *)));
    connect(m_dataViewImpl, SIGNAL(dataDelete(bool)), this, SLOT(onDataDelete(bool)));

    toolBox()->show();

    /* init the docs */
    docs.clear();
    device_index.clear();
    comboIndex = 0;/* to make the list is null, and the index 0 is local image */

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
    burn_setting->setObjectName("BurnDataSetting");
    ThManager()->regTheme(burn_setting, "ukui-white", "background-color: rgba(233, 233, 233, 1);"
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
    ThManager()->regTheme(burn_setting, "ukui-black",
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
    //开始刻录按钮
    burn_button = new QPushButton(i18n("create iso"), this);
    burn_button->setFixedSize(140, 45);
    burn_button->setObjectName("BurnDataStart");
    ThManager()->regTheme(burn_button, "ukui-white",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #6B8DEB;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #3D6BE5;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #415FC3;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #E9E9E9;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: rgba(193, 193, 193, 1);");
    ThManager()->regTheme(burn_button, "ukui-black",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #6B8DEB;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #3D6BE5;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #415FC3;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: #FFFFFF;",
                                   "border:none; border-radius: 4px;"
                                   "background-color: #E9E9E9;"
                                   "font: 18px \"MicrosoftYaHei\";"
                                   "color: rgba(193, 193, 193, 1);");
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
    combo_burner->setObjectName("ComboBurner");
    ThManager()->regTheme(combo_burner, "ukui-white","#ComboBurner{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboBurner::hover{border:1px solid #6B8EEB;}"
                                                 "#ComboBurner::selected{border:1px solid #6B8EEB;}"
                                                 "#ComboBurner QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #FFFFFF;border:1px solid #DCDDDE;}"
                                                 //"#ComboBurner QAbstractItemView::hover{"
                                                 //"padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 //"background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#ComboBurner QAbstractItemView::item{"
                                                 "background-color: #DAE3FA;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboBurner QAbstractItemView::item::hover{border: none;"
                                                 "background-color: #3D6BE5;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#ComboBurner::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#ComboBurner::down-arrow{image: url(:/icon/icon/draw-down.jpg); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#ComboBurner QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#ComboBurner QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#ComboBurner QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#ComboBurner QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#ComboBurner{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#ComboBurner::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");
    ThManager()->regTheme(combo_burner, "ukui-black","#comboISO{border:1px solid #DCDDDE;"
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
    /*
    combo_burner->setStyleSheet("QComboBox{background:rgba(255,255,255,1);  border:1px solid rgba(220,221,222,1);border-radius:4px;}"
                            "QComboBox::drop-down{subcontrol-origin: padding; subcontrol-position: top right; \
                             border-top-right-radius: 3px; \
                             border-bottom-right-radius: 3px;}"
                             "QComboBox::down-arrow{width: 8px; height: 16;  padding: 0px 0px 0px 0px;}");
    */

    
    //QLabel *label_space = new QLabel(label);

    QLabel *label_CD = new QLabel( label_view );
    label_CD->setText(i18n("current CD"));
    label_CD->setMinimumSize(75, 30);

    iso_index = 0;

    combo_CD = new QComboBox( label_view );
    combo_CD->setFixedWidth( 310 );
    //combo_CD->setEditable( false );
    combo_CD->setMinimumSize(310, 30);
    image_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/kylin_burner.iso";
    //combo_CD->addItem(QIcon(":/icon/icon/icon-镜像.png"), i18n("image file: ") + image_path);
    combo_burner->insertItem(comboIndex,
                             QIcon(":/icon/icon/icon-刻录机.png"),
                             i18n(" "));
    combo_CD->insertItem(comboIndex++, QIcon(":/icon/icon/icon-镜像.png"), i18n("image file: ") + image_path);
    /* docs[0] is local image */
    K3b::DataDoc *tmpDoc = new K3b::DataDoc(this);
    tmpDoc->setURL(m_doc->URL());
    tmpDoc->newDocument();
    docs << tmpDoc;
    logger->debug("Add new data at index : %d", docs.indexOf(tmpDoc));
    dlgFileFilter->addData();

    combo_CD->setObjectName("ComboCD");
    ThManager()->regTheme(combo_CD, "ukui-white","#ComboCD{border:1px solid #DCDDDE;"
                                                 "border-radius: 4px; combobox-popup: 0;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboCD::hover{border:1px solid #6B8EEB;}"
                                                 "#ComboCD::selected{border:1px solid #6B8EEB;}"
                                                 "#ComboCD QAbstractItemView{"
                                                 "padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 "background-color: #FFFFFF;border:1px solid #DCDDDE;}"
                                                 //"#ComboCD QAbstractItemView::hover{"
                                                 //"padding: 5px 5px 5px 5px; border-radius: 4px;"
                                                 //"background-color: #242424;border:1px solid #6B8EEB;}"
                                                 "#ComboCD QAbstractItemView::item{"
                                                 "background-color: #DAE3FA;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #444444;}"
                                                 "#ComboCD QAbstractItemView::item::hover{border: none;"
                                                 "background-color: #3D6BE5;border-bottom: 1px solid #DCDDDE;"
                                                 "border-radius: 4px;height: 30px;"
                                                 "font: 14px \"MicrosoftYaHei\"; color: #FFFFFF;}"
                                                 "#ComboCD::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}"
                                                 "#ComboCD::down-arrow{image: url(:/icon/icon/draw-down.jpg); "
                                                 "height: 20px; width: 12px; padding: 5px 5px 5px 5px;}"
                                                 "#ComboCD QScrollBar::vertical{background-color: transparent;"
                                                 "width: 5px; border: none;}"
                                                 "#ComboCD QScrollBar::handle::vertical{"
                                                 "background-color: #3D6BE5;border-radius: 2px;}"
                                                 "#ComboCD QScrollBar::add-line{border: none; height: 0px;}"
                                                 "#ComboCD QScrollBar::sub-line{border: none; height: 0px;}",
                                                 QString(), QString(),
                                                 "#ComboCD{background-color: #EEEEEE;border: none; "
                                                 "font: 14px \"MicrosoftYaHei\";color: rgba(193, 193, 193, 1); "
                                                 "border-radius: 4px;}"
                                                 "#ComboCD::drop-down{subcontrol-origin: padding;"
                                                 "subcontrol-position: top right; border: none;}");
    ThManager()->regTheme(combo_CD, "ukui-black","#comboISO{border:1px solid #DCDDDE;"
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
    /*
    combo_CD->setStyleSheet("QComboBox{combobox-popup: 0;background:rgba(255,255,255,1);"
                            "border:1px solid rgba(220,221,222,1);border-radius:4px;}"
                            "QComboBox::drop-down{subcontrol-origin: padding; "
                            "subcontrol-position: top right; "
                            "border-top-right-radius: 3px;"
                            "border-bottom-right-radius: 3px;}"
                            "QComboBox::down-arrow{width: 8px; height: 16;"
                            "image: url(:/icon/icon/draw-down.jpg);"
                            "padding: 0px 0px 0px 0px;}");
    */

    /*
    combo_CD->setStyleSheet("QComboBox{background:rgba(255,255,255,1);}"
                            "QComboBox::drop-down{background:rgba(255,255,255,1); "
                            "subcontrol-origin: padding;"
                            "width : 10px; height : 20 px;subcontrol-position: top right;}"
                            "::down-arrow{background:transparent;"
                            "width : 10px; height : 20 px;}"
                            "image: url(:/icon/icon/icon-right.png);");

    */
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

    tips = new QLabel(
                i18n("Please click button [Add] or drag you file to current zone for new files."),
                this);
    tips->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    tips->setAcceptDrops(true);
    tips->installEventFilter(this);
    tips->setObjectName("tip");
    ThManager()->regTheme(tips, "ukui-white", "background-color:rgba(255, 255, 255, 1);"
                                              "font: 14px; color: #444444;");
    ThManager()->regTheme(tips, "ukui-black", "background-color:rgba(36, 36, 36, 1);"
                                              "font: 14px; color: #FFFFFF;");

    tips->setMinimumHeight(342);
    m_dataViewImpl->view()->setObjectName("DataView");
    ThManager()->regTheme(m_dataViewImpl->view(), "ukui-white", "background-color:rgba(255, 255, 255, 1);"
                                              "font: 14px; color: #444444;");
    ThManager()->regTheme(m_dataViewImpl->view(), "ukui-black", "background-color:rgba(36, 36, 36, 1);"
                                              "font: 14px; color: #FFFFFF;");
    m_dataViewImpl->view()->header()->setObjectName("DataViewHeader");
    ThManager()->regTheme(m_dataViewImpl->view()->header(), "ukui-white", "#DataViewHeader:section{"
                                                                          "background-color:rgba(242, 242, 242, 1);"
                                                                          "border: 0px;"
                                                                          //"border-color: rgba(242, 242, 242, 1);"
                                              "font: 12px; color: rgba(68, 68, 68, 1);}");
    ThManager()->regTheme(m_dataViewImpl->view()->header(), "ukui-black", "#DataViewHeader:section{"
                                                                          "background-color:rgba(36, 36, 36, 1);"
                                                                          "border: 0px;"
                                                                          //"border-color: rgba(242, 242, 242, 1);"
                                              "font: 12px; color: rgba(255, 255, 255, 1);}");
    /*
    m_dataViewImpl->view()->setStyleSheet("background-color: rgba(36, 36, 36, 1);font: 14px; color: #FFFFFF;");
    m_dataViewImpl->view()->header()->setStyleSheet("background-color: rgba(36, 36, 36, 1);font: 14px; color: #FFFFFF;");
    */
    m_dataViewImpl->view()->setFixedHeight(28);
    m_dataViewImpl->view()->setAcceptDrops(true);
    m_dataViewImpl->view()->installEventFilter(this);

    //tips->hide();
    QVBoxLayout *vlayout_middle = new QVBoxLayout();
    vlayout_middle->setContentsMargins(0, 0, 20, 0);
    vlayout_middle->addWidget( m_dataViewImpl->view() );
    vlayout_middle->addWidget( tips );

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
    burn_button->installEventFilter(this);
    connect( combo_burner, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotComboBurner(int) ) );
    connect( combo_CD, SIGNAL( currentIndexChanged(int) ), this, SLOT( slotComboCD(int) ) );
    connect( m_dataViewImpl, SIGNAL(setCurrentRoot(QModelIndex)), this, SLOT(slotSetCurrentRoot(QModelIndex)) );
    connect( m_dirView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotCurrentDirChanged()) );
    connect( m_dataViewImpl, SIGNAL(addDragFiles(QList<QUrl>, K3b::DirItem*)), this, SLOT(addDragFiles(QList<QUrl>, K3b::DirItem*)) );

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
    button_add->setObjectName("btnAdd");
    ThManager()->regTheme(button_add, "ukui-white",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(67, 67, 67, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
       ThManager()->regTheme(button_add, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
    /*
    button_add->setStyleSheet("QPushButton{ background-color:#e9e9e9; border-radius:4px; font: 14px; color:#444444;}" 
                              "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px; color:#ffffff;}"  
                              "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px; color:#ffffff;}");
    */
    //删除按钮
    button_remove = new QPushButton(i18n("Remove"), label_action );
    button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
    button_remove->setFixedSize(80, 30);
    button_remove->setEnabled( false );
    button_remove->setObjectName("btnRemove");
    ThManager()->regTheme(button_remove, "ukui-white",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(67, 67, 67, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
       ThManager()->regTheme(button_remove, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");

    //清空按钮
    button_clear = new QPushButton(i18n("Clear"), label_action );
    button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png")); 
    button_clear->setFixedSize(80, 30);
    button_clear->setEnabled( false );
    button_clear->setObjectName("btnClear");
    ThManager()->regTheme(button_clear, "ukui-white",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(67, 67, 67, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
       ThManager()->regTheme(button_clear, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");

    //新建文件夹按钮
    button_newdir = new QPushButton(i18n("New Dir"), label_action );
    button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png")); 
    button_newdir->setFixedSize(112, 30);
    button_newdir->setObjectName("btnNewDir");
    ThManager()->regTheme(button_newdir, "ukui-white",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(67, 67, 67, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");
       ThManager()->regTheme(button_newdir, "ukui-black",
                                       "background-color: rgba(57, 58, 62, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(255, 255, 255, 1);",
                                       "background-color: rgba(107, 141, 235, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(65, 95, 195, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-action.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(61, 107, 229, 1);",
                                       "background-color: rgba(233, 233, 233, 1);"
                                       "border: none; border-radius: 4px;"
                                       //"qproperty-icon: url(:/func/add-default.png);"
                                       "qproperty-iconSize: 15px 15px;"
                                       "font: 14px \"MicrosoftYaHei\";"
                                       "color: rgba(193, 193, 193, 1);");


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

    /*
    workerThread = new QThread;
    LoadWorker *worker = new LoadWorker;
    worker->moveToThread(workerThread);
    connect(this,SIGNAL(load(QString, DataDoc*)),worker,SLOT(load(QString, DataDoc*)));
    connect(worker,SIGNAL(loadFinished()),this,SLOT(onLoadFinished()));
    connect(workerThread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    workerThread->start();
    */

    dlg = newBurnDialog( this );

    logger->debug("Draw data burner end");
}

K3b::DataView::~DataView()
{
    delete dlgFileFilter;
    if (dlg) delete dlg;
}

void K3b::DataView::slotFileFilterClicked()
{
    //K3b::DirItem *d = m_doc->root();
    //K3b::DataItem *c;
    dlgFileFilter->setAttribute(Qt::WA_ShowModal);
    dlgFileFilter->setDoFileFilter(combo_CD->currentIndex());
    //dlgFileFilter->slotDoFileFilter(docs[combo_CD->currentIndex()]);

    QPoint p = mapToGlobal(pos());
    dlgFileFilter->move(p.x() + width() / 2 - dlgFileFilter->width() / 2,
                        p.y() + height() / 2 - dlgFileFilter->height() / 2);

    dlgFileFilter->show();
}

void K3b::DataView::slotFinish(K3b::DataDoc *doc)
{
    copyData(m_doc, doc);
}

bool K3b::DataView::eventFilter(QObject *obj, QEvent *event)
{
    QDropEvent *dropEvent;
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
        else if (obj == burn_button) hoverButtonBurn();
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
        if (obj == button_add) hoverButtonAdd(false);
        else if (obj == button_remove) hoverButtonRemove(false);
        else if (obj == button_clear) hoverButtonClear(false);
        else if (obj == button_newdir) hoverButtonNewDir(false);
        else if (obj == burn_button) hoverButtonBurn(false);
        else break;
        break;
    case QEvent::MouseButtonPress:
        if(obj == button_add)
        {
            pressButtonAdd();
            button_add->setIcon(QIcon(":/icon/icon/icon-添加-悬停点击.png"));
        }
        if(obj == button_remove){
            if ( button_remove->isEnabled() )
            {
                pressButtonRemove();
                button_remove->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
            }
        }
        if(obj == button_clear){
            if ( button_clear->isEnabled() )
            {
                pressButtonClear();
                button_clear->setIcon(QIcon(":/icon/icon/icon-清空-悬停点击.png"));
            }
        }
        if(obj == button_newdir)
        {
            pressButtonNewDir();
            button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-悬停点击.png"));
        }
        else if (obj == burn_button) pressButtonBurn();
        break;
    case QEvent::DragEnter:
        if (obj == tips)
        {
            logger->debug("Tips drag in...");
            event->accept();
        }
        else if (obj == m_dataViewImpl->view())
        {
            logger->debug("Data view drag in ....");
            event->accept();
        }
        else return QWidget::eventFilter(obj, event);
   case QEvent::Drop:
        if (obj == tips /*|| obj == m_dataViewImpl->view()*/)
        {
            dropEvent = static_cast<QDropEvent *>(event);
            if (lastDrop.isEmpty() || lastDrop != dropEvent->mimeData()->urls())
            {
                lastDrop = dropEvent->mimeData()->urls();
            }
            else return QWidget::eventFilter(obj, event);
            logger->debug("Tips drag in...");
            qDebug() << "Tips drop..." << dropEvent->mimeData()->urls();
            if (lastDrop.size() > 0)
            {
                if (tips) tips->hide();
                m_dataViewImpl->view()->setFixedHeight(370);
                enableButtonBurn();
                enableBurnSetting();
                btnFileFilter->show();
                enableButtonRemove();
                enableButtonClear();
                m_doc->addUrls(lastDrop);
                //slotAddFile(lastDrop);
                qDebug() << combo_CD->currentIndex();
                //copyData(m_doc, docs[combo_CD->currentIndex()]);
            }
            /*
            if (0 == m_dataViewImpl->model()->rowCount() ||
                    m_doc->root()->children().size() == 0)
            {
                disableButtonRemove();
                disableButtonClear();
                disableButtonBurn();
                disableBurnSetting();
                btnFileFilter->hide();
                m_dataViewImpl->view()->setFixedHeight(28);
                if (tips) tips->show();
            }
            else {
                enableButtonBurn();
                enableBurnSetting();
                btnFileFilter->show();
                enableButtonRemove();
                enableButtonClear();
                if (tips) tips->hide();
                m_dataViewImpl->view()->setFixedHeight(370);
            }
            */
        }
        else return QWidget::eventFilter(obj, event);
        return true;
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}

void K3b::DataView::slotAddFile(QList<QUrl> urls)
{
    for (int i = 0; i < docs.size(); ++i)
        docs[i]->addUrls(urls);
}

void K3b::DataView::slotOpenClicked()
{
    int ret = m_dataViewImpl->slotOpenDir();
    if (ret)
    {
        copyData(docs.at(combo_CD->currentIndex()), m_doc);


        slotOption(0, dlgFileFilter->getStatus(combo_CD->currentIndex()).isHidden);
        slotOption(1, dlgFileFilter->getStatus(combo_CD->currentIndex()).isBroken);
        slotOption(2, dlgFileFilter->getStatus(combo_CD->currentIndex()).isReplace);

    }
    /*
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
    */
}

void K3b::DataView::slotRemoveClicked()
{
    m_dataViewImpl->slotRemove();
    copyData(docs.at(combo_CD->currentIndex()), m_doc);
}

void K3b::DataView::slotClearClicked()
{
    //const QModelIndex parentDirectory = m_dataViewImpl->view()->rootIndex();
    m_dataViewImpl->slotClear();
    copyData(docs.at(combo_CD->currentIndex()), m_doc);
}

void K3b::DataView::slotNewdirClicked()
{
    m_dataViewImpl->slotNewDir();
    copyData(docs.at(combo_CD->currentIndex()), m_doc);
}

void K3b::DataView::slotDeviceChange( K3b::Device::DeviceManager* manager )
{
    /*
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    if ( device_list.count() == 0 ){
        //combo_burner->setEnabled(false);
        //combo_CD->setEditable(true);
    }else 
        slotMediaChange( 0 );
    */

    /*
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();
    logger->debug("Device changed, now have %d device(s)", device_list.size());
    for (int i = 0; i < device_list.size(); ++i)
    {
        if (-1 == device_index.indexOf(device_list[i]))
        {
            logger->debug("Device %s need to add.", (device_list[i]->blockDeviceName()).toStdString().c_str());
            slotMediaChange(device_list[i]);
        }
    }
    */


    logger->debug("Device changed...");
    QList<K3b::Device::Device*> device_list = k3bcore->deviceManager()->allDevices();

    for (int j = 0; j < device_index.size(); ++j)
    {
        if (-1 == device_list.indexOf(device_index[j]))
        {
            device_index.removeAt(j);
            docs.removeAt(j + 1);
            combo_burner->removeItem(j + 1);
            combo_CD->removeItem(j + 1);
            dlgFileFilter->removeData(j + 1);
            docs.removeAt(j + 1);
            logger->debug("remove device [%s] at index %d on selection item index %d",
                          (device_index[j]->blockDeviceName()).toStdString().c_str(), j, j + 1);
        }
    }

    for (int i = 0; i < device_list.size(); ++i)
    {
        if (-1 == device_index.indexOf(device_list[i]))
        {
            logger->debug("add device [%s] at index %d on selection item index",
                          (device_list[i]->blockDeviceName()).toStdString().c_str(), i);
            slotMediaChange(device_list[i]);
        }
    }
}

void K3b::DataView::slotMediaChange( K3b::Device::Device* dev )
{
    logger->debug("Media changed..., device : %s", dev->blockDeviceName().toStdString().c_str());

    int     idx = -1;
    QString cdSize;
    QString cdInfo;
    QString mountInfo;
    K3b::DataDoc *tmpDoc = NULL;
    K3b::Medium medium = k3bappcore->mediaCache()->medium( dev );
    KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() );

    cdSize =  KIO::convertSize(dev->diskInfo().remainingSize().mode1Bytes());
    if( !mountPoint)
    {
        if (dev->diskInfo().diskState() == K3b::Device::STATE_EMPTY )
        {
            cdInfo = i18n("empty medium ") + cdSize;
        }
        else
        {
            cdInfo = i18n("please insert a medium or empty CD");            
            idx = device_index.indexOf(dev);
            if (-1 != idx)
            {
                logger->debug("Device [%s] have no medium, do not show in selection item."
                              "delete it at idx %d, data idx %d",
                              dev->blockDeviceName().toStdString().c_str(), idx, idx + 1);
                device_index.removeAt(idx++);
                tmpDoc = docs[idx];
                if (tmpDoc) delete tmpDoc;
                docs.removeAt(idx);
                logger->debug("Remove data at index : %d", docs.indexOf(tmpDoc));
                dlgFileFilter->removeData(idx);
                combo_burner->removeItem(idx);
                combo_CD->removeItem(idx);
                --comboIndex;
                if (idx - 1 < 0 ) combo_CD->setCurrentIndex(0);
                else combo_CD->setCurrentIndex(idx - 1);
            }
            else
            {
                logger->debug("Device [%s] have no medium, do not show in selection item.",
                              dev->blockDeviceName().toStdString().c_str());
            }
            return;
        }
    }
    else
    {
        mountInfo = mountPoint->mountPoint();
        cdInfo = medium.shortString() + i18n(", remaining available space") + cdSize;
        if (i18n("No medium present") == medium.shortString())
        {
            idx = device_index.indexOf(dev);
            device_index.removeAt(idx++);
            tmpDoc = docs[idx];
            if (tmpDoc) delete tmpDoc;
            docs.removeAt(idx);
            logger->debug("Remove data at index : %d", docs.indexOf(tmpDoc));
            dlgFileFilter->removeData(idx);
            combo_burner->removeItem(idx);
            combo_CD->removeItem(idx);
            --comboIndex;
            if (idx - 1 < 0 ) combo_CD->setCurrentIndex(0);
            else combo_CD->setCurrentIndex(idx - 1);
            return;
        }
    }

    idx = device_index.indexOf(dev);
    if (-1 == idx)
    {
        device_index << dev;
        idx = comboIndex;
    }
    else ++idx;
    if (idx == comboIndex)
    {
        combo_burner->insertItem(comboIndex,
                                 QIcon(":/icon/icon/icon-刻录机.png"),
                                 dev->vendor() + " - " + dev->description());
        combo_CD->insertItem(comboIndex++, QIcon(":/icon/icon/icon-光盘.png"), i18n("cdrom file: ") + cdInfo);
        tmpDoc = new K3b::DataDoc(this);
        tmpDoc->setURL(m_doc->URL());
        tmpDoc->newDocument();
        if (tmpDoc) docs << tmpDoc;
        logger->debug("Add new data at index : %d", docs.indexOf(tmpDoc));
        dlgFileFilter->addData();
        combo_CD->setCurrentIndex(idx);
        lastIndex = combo_CD->currentIndex();
    }
    else
    {
        combo_CD->setItemText(idx, i18n("cdrom file: ") + cdInfo);
        tmpDoc = docs[idx];
        tmpDoc->clear();
        combo_CD->setCurrentIndex(idx);
        lastIndex = combo_CD->currentIndex();
    }
    if (!mountInfo.isEmpty())
    {
        QDir *dir = new QDir(mountInfo);
        QList<QFileInfo> fileinfo(dir->entryInfoList( QDir::AllEntries | QDir::Hidden ) );
        for ( int i = 0; i < fileinfo.count(); i++ )
        {
            qDebug() << fileinfo.at(i).fileName();
            if (fileinfo.at(i).fileName() == "." ||
                    fileinfo.at(i).fileName() == "..") continue;
            tmpDoc->addUnremovableUrls( QList<QUrl>() <<  QUrl::fromLocalFile(fileinfo.at(i).filePath()) );
        }
        copyData(m_doc, tmpDoc);
        combo_CD->setCurrentIndex(idx);
        lastIndex = combo_CD->currentIndex();
    }
    if (0 == m_dataViewImpl->view()->model()->rowCount() ||
            0 == m_doc->root()->children().size())
    {
        m_dataViewImpl->view()->setFixedHeight(28);
        tips->show();
    }
    else
    {
        tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
    }


    /*
    QList<K3b::Device::Device*> device_list = k3bappcore->appDeviceManager()->allDevices();
    QString cdSize;
    QString cdInfo;
    QString mountInfo;
    K3b::DataDoc *tmpDoc = NULL;
    int     idx = -1;

    foreach(K3b::Device::Device* device, device_list)
    {
        K3b::Medium medium = k3bappcore->mediaCache()->medium( device );
        KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( device->blockDeviceName() );
        idx = device_index.indexOf(device);
        if (-1 == idx) idx = comboIndex;
        else ++idx;
        if (idx == comboIndex) device_index << device;
        cdInfo.clear();
        cdSize.clear();
        mountInfo.clear();
        cdSize =  KIO::convertSize(device->diskInfo().remainingSize().mode1Bytes());
        if( !mountPoint)
        {
            if (device->diskInfo().diskState() == K3b::Device::STATE_EMPTY )
            {
                cdInfo = i18n("empty medium ") + cdSize;
            }
            else
            {
                cdInfo = i18n("please insert a medium or empty CD");
            }
        }
        else
        {
            mountInfo = mountPoint->mountPoint();
            cdInfo = medium.shortString() + i18n(", remaining available space") + cdSize;
        }
        if (idx == comboIndex)
        {
            combo_burner->insertItem(comboIndex,
                                     QIcon(":/icon/icon/icon-刻录机.png"),
                                     device->vendor() + " - " + device->description());
            combo_CD->insertItem(comboIndex++, QIcon(":/icon/icon/icon-光盘.png"), i18n("cdrom file: ") + cdInfo);
            tmpDoc = new K3b::DataDoc(this);
            tmpDoc->setURL(m_doc->URL());
            tmpDoc->newDocument();
            if (tmpDoc) docs << tmpDoc;
        }
        else
        {
            combo_CD->setItemText(idx, i18n("cdrom file: ") + cdInfo);
            tmpDoc = docs[idx];
            tmpDoc->clear();
        }
        if (cdInfo == i18n("please insert a medium or empty CD"))
        {
        }
        if (!mountInfo.isEmpty())
        {
            QDir *dir = new QDir(mountInfo);
            QList<QFileInfo> fileinfo(dir->entryInfoList( QDir::AllEntries | QDir::Hidden ) );
            for ( int i = 0; i < fileinfo.count(); i++ )
            {
                qDebug() << fileinfo.at(i).fileName();
                if (fileinfo.at(i).fileName() == "." ||
                        fileinfo.at(i).fileName() == "..") continue;
                tmpDoc->addUnremovableUrls( QList<QUrl>() <<  QUrl::fromLocalFile(fileinfo.at(i).filePath()) );
            }
            copyData(m_doc, tmpDoc);
            combo_CD->setCurrentIndex(idx);
        }
    }
    lastIndex = combo_CD->currentIndex();
    if (0 == m_dataViewImpl->view()->model()->rowCount())
    {
        m_dataViewImpl->view()->setFixedHeight(28);
        tips->show();
    }
    else
    {
        tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
    }
    */

    /*
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
        combo_CD->addItem(QIcon(":/icon/icon/icon-光盘.png"), i18n("cdrom file: ") + CDInfo);
    }
    */
    //image_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/kylin_burner.iso";
    //combo_CD->addItem(QIcon(":/icon/icon/icon-镜像.png"), i18n("image file: ") + image_path);
}

void K3b::DataView::slotComboBurner(int index)
{
    if ( index < 0 )
       index = 0;
    // 会调用slotComboCD 槽函数    
    combo_CD->setCurrentIndex( index );
}

void K3b::DataView::slotComboCD(int index)
{
    K3b::DataDoc *tmpDoc = NULL;

    if (-1 == index) return;

    tmpDoc = docs[index];
    m_doc->clear();
    copyData(m_doc, tmpDoc);
    combo_burner->setCurrentIndex( index );
    if (index)
    {
        combo_burner->setEnabled( true );
        burn_setting->setText(i18n("setting"));
        burn_button->setText(i18n("start burner"));
    }
    else
    {
        combo_burner->setEnabled( false );
        burn_setting->setText(i18n("open"));
        burn_button->setText(i18n("create iso"));
    }

    slotOption(0, dlgFileFilter->getStatus(index).isHidden);
    slotOption(1, dlgFileFilter->getStatus(index).isBroken);
    slotOption(2, dlgFileFilter->getStatus(index).isReplace);

    /*
    if (0 == m_dataViewImpl->view()->model()->rowCount())
    {
        qDebug() << ".....................................";
        m_dataViewImpl->view()->setFixedHeight(28);
        tips->show();
    }
    else
    {
        tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
    }
    */
    /*
    qDebug()<< " combo Cd index " << index << mount_index << iso_index <<endl;
    if ( index < 0 )
        index = 0;

    combo_CD->setEditable(false);

    if ( index == iso_index ){
        m_doc->clear();
        combo_burner->setEnabled( false );
        //combo_CD->setEditable( true );
        burn_setting->setText(i18n("open"));
        burn_button->setText(i18n("create iso"));
    }else{
        combo_burner->setEnabled( true );
        combo_CD->setEditable( false );
        burn_setting->setText(i18n("setting"));
        burn_button->setText(i18n("start burner"));


        combo_burner->blockSignals( true ); //不发送信号，不会调用slotComboBurner 槽函数
        combo_burner->setCurrentIndex( index );
        combo_burner->blockSignals( false );

        add_device_urls( mount_index.at( index ) );
    }
    */

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
    DataBurnDialog *dlg = new DataBurnDialog( m_doc, mainWindow);


    logger->debug("Start Burn button cliked. now project size is %llu", m_doc->burningSize());


    if( m_doc->burningSize() == 0 ) { 
         KMessageBox::information( this, i18n("Please add files to your project first."),
                                      i18n("No Data to Burn") );
    }else if ( burn_button->text() == i18n("start burner" )){ 
        logger->debug("Button model is start burner");
        int index = combo_burner->currentIndex();
        --index;
        dlg->setComboMedium( device_index.at( index ) );
        qDebug()<< "index :" <<  index << " device block name: " << device_index.at( index )->blockDeviceName() <<endl;
        dlg->slotStartClicked();
    }else if( burn_button->text() == i18n("create iso" )){ 
        logger->debug("Button model is create iso");
        dlg->setOnlyCreateImage( true );
        dlg->setTmpPath( image_path );
        QFileInfo fileinfo( image_path );
        QDir dir( fileinfo.path() );
        if ( !dir.exists() )
            return;
        dlg->slotStartClicked();
        qDebug() << dlg->flag;
        if (0 == dlg->flag) m_doc->clear();
        combo_CD->setCurrentIndex(0);
    }   
    delete dlg;
}

void K3b::DataView::slotBurn()
{
       if( m_doc->burningSize() == 0 ) { 
         KMessageBox::information( this, i18n("Please add files to your project first."),
                                      i18n("No Data to Burn") );
    }else if ( burn_setting->text() == i18n("setting") ){
        dlg->execBurnDialog(true);
    }else if ( burn_setting->text() == i18n("open" )){
           connect(this, SIGNAL(disableCD(bool)), this, SLOT(slotDisableCD(bool)));
        QString filepath = QFileDialog::getExistingDirectory(this, i18n("open" ), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks/* | QFileDialog::DontUseNativeDialog*/);
        if ( filepath.isEmpty() )
            return;
        image_path = filepath + "/kylin_burner.iso";
        //combo_CD->setCurrentText( image_path );
        qDebug() << "image path : " << image_path;
        combo_CD->setItemText(0, i18n("image file: ") + image_path);
        //combo_CD->lineEdit()->setCursorPosition(0);
        //emit disableCD(false);
    } 
    //combo_CD->setEditable( false );
}

void K3b::DataView::slotDisableCD(bool enb)
{
    combo_CD->setEditable( enb );
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

void K3b::DataView::hoverBurnSetting(bool in)
{
    if (!burn_setting->isEnabled()) return;
    if (in)
        burn_setting->setStyleSheet("background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#444444");
    else
        burn_setting->setStyleSheet("background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;color : #444444");
}

void K3b::DataView::pressBurnSetting()
{
    burn_setting->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableBurnSetting()
{
    if (burn_setting->isEnabled()) burn_setting->setEnabled(false);
    if (!burn_setting->isEnabled())
    {
        burn_setting->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonBurn()
{
    burn_button->setEnabled(true);
    burn_button->setStyleSheet("background-color:rgba(61, 107, 229, 1);font: 18px;border-radius: 4px;color:#ffffff");
}

void K3b::DataView::hoverButtonBurn(bool in)
{
    if (!burn_button->isEnabled()) return;
    if (in)
        burn_button->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 18px;border-radius: 4px;color:#ffffff");
    else
        burn_button->setStyleSheet("background-color:rgba(61, 107, 229, 1);font: 18px;border-radius: 4px;color:#ffffff");
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
    button_add->setEnabled(true);
    button_add->setIcon(QIcon(":/icon/icon/icon-添加-默认.png"));
    button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonAdd(bool in)
{
    if (!button_add->isEnabled()) return;
    if (in)
    {
        button_add->setIcon(QIcon(":/icon/icon/icon-添加-悬停点击.png"));
        button_add->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_add->setIcon(QIcon(":/icon/icon/icon-添加-默认.png"));
        button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
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
    button_remove->setEnabled(true);
    button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
    button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonRemove(bool in)
{
    if (!button_remove->isEnabled()) return;
    if (in)
    {
        button_remove->setIcon(QIcon(":/icon/icon/icon-删除-悬停点击.png"));
        button_remove->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_remove->setIcon(QIcon(":/icon/icon/icon-删除-默认.png"));
        button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
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
    button_clear->setEnabled(true);
    button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png"));
    button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonClear(bool in)
{
    if (!button_clear->isEnabled()) return;
    if (in)
    {
        button_clear->setIcon(QIcon(":/icon/icon/icon-清空-悬停点击.png"));
        button_clear->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_clear->setIcon(QIcon(":/icon/icon/icon-清空-默认.png"));
        button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
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
    button_newdir->setEnabled(true);
    button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png"));
    button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonNewDir(bool in)
{
    if (!button_newdir->isEnabled()) return;
    if (in)
    {
        button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-悬停点击.png"));
        button_newdir->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_newdir->setIcon(QIcon(":/icon/icon/icon-新建文件-默认.png"));
        button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
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

void K3b::DataView::copyData(K3b::DataDoc *target, K3b::DataDoc *source)
{
    K3b::DataItem *child = NULL;
    target->clear();

    for (int i = 0; i < source->root()->children().size(); ++i)
    {
        child = source->root()->children().at(i);
        if (child->isDeleteable())
            target->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
        else
            target->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
    }

    if (m_doc->root()->children().size() == 0)
    {
        disableButtonRemove();
        disableButtonClear();
        disableButtonBurn();
        disableBurnSetting();
        btnFileFilter->hide();
        m_dataViewImpl->view()->setFixedHeight(28);
        if (tips) tips->show();
    }
    else
    {
        enableButtonRemove();
        enableButtonClear();
        enableButtonBurn();
        enableBurnSetting();
        btnFileFilter->show();
        if (tips) tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
    }
}

void K3b::DataView::isHidden(bool flag)
{
    logger->debug("call to set file filter hidden is %s",
                  flag ? "true" : "false");

    K3b::DataDoc *tmp = dlgFileFilter->getData(combo_CD->currentIndex());
    K3b::DataItem *child = NULL;
    if (!tmp)
    {
        logger->error("Cannot get the file filter data at index : %d",
                      combo_CD->currentIndex());
        return;
    }
    if (flag)
    {
        for (int i = 0; i < m_doc->root()->children().size(); ++i)
        {
            child = m_doc->root()->children().at(i);
            if (child->k3bName().size() > 1 && child->k3bName().at(0) == '.')
            {
                if (child->isDeleteable())
                {
                    tmp->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    tmp->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                logger->info("Filter hidden item <%s>", child->localPath().toStdString().c_str());
                m_doc->removeItem(child);
                --i;
            }
        }
        copyData(docs[combo_CD->currentIndex()], m_doc);
    }
    else
    {
        for (int i = 0; i < tmp->root()->children().size(); ++i)
        {
            child = tmp->root()->children().at(i);
            if (child->k3bName().size() > 1 && child->k3bName().at(0) == '.')
            {
                if (child->isDeleteable())
                {
                    m_doc->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    m_doc->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                logger->info("Recovery hidden item <%s>", child->localPath().toStdString().c_str());
                tmp->removeItem(child);
                --i;
            }
        }
        copyData(docs[combo_CD->currentIndex()], m_doc);
    }
    dlgFileFilter->setHidden(combo_CD->currentIndex(), flag);
    dlgFileFilter->reload(combo_CD->currentIndex());
}

void K3b::DataView::isBroken(bool flag)
{
    logger->debug("call to set file filter broken link is %s",
                  flag ? "true" : "false");
    K3b::DataDoc *tmp = dlgFileFilter->getData(combo_CD->currentIndex());
    K3b::DataItem *child = NULL;
    if (!tmp)
    {
        logger->error("Cannot get the file filter data at index : %d",
                      combo_CD->currentIndex());
        return;
    }
    if (flag)
    {
        for (int i = 0; i < m_doc->root()->children().size(); ++i)
        {
            child = m_doc->root()->children().at(i);
            if (child->isSymLink())
            {
                if (child->isDeleteable())
                {
                    tmp->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    tmp->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                logger->info("Filter broken link item <%s>", child->localPath().toStdString().c_str());
                m_doc->removeItem(child);
                --i;
            }
        }
        copyData(docs[combo_CD->currentIndex()], m_doc);
    }
    else
    {
        for (int i = 0; i < tmp->root()->children().size(); ++i)
        {
            child = tmp->root()->children().at(i);
            if (child->isSymLink())
            {
                if (child->isDeleteable())
                {
                    m_doc->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    m_doc->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                --i;
                logger->info("Recovery broken link item <%s>", child->localPath().toStdString().c_str());
                tmp->removeItem(child);
            }
        }
        copyData(docs[combo_CD->currentIndex()], m_doc);
    }
    dlgFileFilter->setBroken(combo_CD->currentIndex(), flag);
    dlgFileFilter->reload(combo_CD->currentIndex());
}


void K3b::DataView::isReplace(bool flag)
{
    logger->debug("call to set file filter replace link is %s",
                  flag ? "true" : "false");
    K3b::DataDoc *tmp = dlgFileFilter->getData(combo_CD->currentIndex());
    K3b::DataItem *child = NULL;
    if (!tmp)
    {
        logger->error("Cannot get the file filter data at index : %d",
                      combo_CD->currentIndex());
        return;
    }
    if (flag)
    {
        for (int i = 0; i < m_doc->root()->children().size(); ++i)
        {
            child = m_doc->root()->children().at(i);
            if (child->isSymLink())
            {
                if (child->isDeleteable())
                {
                    tmp->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    tmp->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                logger->info("Filter replace link item <%s>", child->localPath().toStdString().c_str());
                m_doc->removeItem(child);
                --i;
            }
            copyData(docs[combo_CD->currentIndex()], m_doc);
        }
    }
    else
    {
        for (int i = 0; i < tmp->root()->children().size(); ++i)
        {
            child = tmp->root()->children().at(i);
            if (child->isSymLink())
            {
                if (child->isDeleteable())
                {
                    m_doc->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    m_doc->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                logger->info("Recovery replace link item <%s>", child->localPath().toStdString().c_str());
                tmp->removeItem(child);
                --i;
            }
        }
        copyData(docs[combo_CD->currentIndex()], m_doc);
    }
    dlgFileFilter->setReplace(combo_CD->currentIndex(), flag);
    dlgFileFilter->reload(combo_CD->currentIndex());
}

void K3b::DataView::slotOption(int option, bool flag)
{
    qDebug() << "Do set option" << option << flag;
    logger->debug("Do option %d to %s.", option, flag ? "true" : "false");
    switch (option)
    {
    case 0:/* hidden */
        isHidden(flag);
        emit setIsHidden(flag);
        break;
    case 1:/* broken */
        isBroken(flag);
        emit setIsBroken(flag);
        break;
    default:/* replace */
        isReplace(flag);
        emit setIsReplace(flag);
        break;
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

