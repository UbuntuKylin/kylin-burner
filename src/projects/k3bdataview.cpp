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
#include "k3b.h"
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
    if (flag)
    {
        enableButtonRemove();
        enableButtonClear();
    }
    else
    {
        disableButtonRemove();
        disableButtonClear();
    }
}

void K3b::DataView::onDataChange(QModelIndex parent, QSortFilterProxyModel *model)
{
    if (0 == model->rowCount(parent))
    {
        disableButtonRemove();
        disableButtonClear();
        disableButtonBurn();
        disableBurnSetting();
        btnFileFilter->hide();
        m_dataViewImpl->view()->setFixedHeight(28);
        if (tips)
        {
            tips->show();
            tips->installEventFilter(this);
        }
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
    if (logger) logger->debug("Draw data burner begin...");
    mainWindow = parent->parentWidget()->parentWidget()->parentWidget()
            ->parentWidget();
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
    //刻录设置按钮
    burn_setting = new QPushButton(i18n("open"), this);
    burn_setting->setFixedSize(80, 30);
    burn_setting->setObjectName("BurnDataSetting");
    //开始刻录按钮
    burn_button = new QPushButton(i18n("create iso"), this);
    burn_button->setFixedSize(140, 45);
    burn_button->setObjectName("BurnDataStart");

    burn_button->setEnabled( false );
    //burn_button->setStyleSheet("QPushButton{background-color:rgb(233, 233, 233);font: 18px;border-radius: 4px;}");

    QLabel *label_view = new QLabel(this);
    //QGridLayout *layout = new QGridLayout(label_view);
    QVBoxLayout *layout = new QVBoxLayout( label_view );
    layout->setContentsMargins(0, 0, 5, 25);

    QLabel *label_burner = new QLabel(i18n("C-Burner"), label_view);
    label_burner->setToolTip(i18n("current burner"));
    label_burner->setStyleSheet("font: 14px;");
    label_burner->setFixedSize(80, 30);

    combo_burner = new QComboBox( label_view );
    combo_burner->setEnabled( false );
    combo_burner->setFixedSize(310, 30);
    combo_burner->setObjectName("ComboBurner");

    QLabel *label_CD = new QLabel( label_view );
    label_CD->setText(i18n("C-CD"));
    label_CD->setToolTip(i18n("current CD"));
    label_CD->setStyleSheet("font: 14px;");
    label_CD->setMinimumSize(80, 30);

    iso_index = 0;

    combo_CD = new QComboBox( label_view );
    combo_CD->setFixedSize(310, 30);
    image_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/kylin_burner.iso";
    combo_burner->insertItem(comboIndex,
                             QIcon(":/icon/icon/icon-burner.png"),
                             i18n(" "));
    combo_CD->insertItem(comboIndex++, QIcon(":/icon/icon/icon-image.png"), i18n("image file: ") + image_path);
    /* docs[0] is local image */
    K3b::DataDoc *tmpDoc = new K3b::DataDoc(this);
    tmpDoc->setURL(m_doc->URL());
    tmpDoc->newDocument();
    docs << tmpDoc;
    if (logger) logger->debug("Add new data at index : %d", docs.indexOf(tmpDoc));
    dlgFileFilter->addData();

    combo_CD->setObjectName("ComboCD");

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
    tips->setMinimumHeight(342);
    m_dataViewImpl->view()->setObjectName("DataView");
    m_dataViewImpl->view()->header()->setObjectName("DataViewHeader");

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
    label_action->setMinimumSize(370, 40);
    
    //添加按钮
    button_add = new QPushButton(i18n("Add"), label_action);
    button_add->setFixedSize(80, 30);
    button_add->setIcon(QIcon::fromTheme("edit-add", QIcon(":/icon/icon/icon-add.png")));
    button_add->setObjectName("btnAdd");
    button_add->setProperty("useIconHighlightEffect", 0x8);
    //删除按钮
    button_remove = new QPushButton(i18n("Remove"), label_action );
    button_remove->setIcon(QIcon::fromTheme("edit-clear", QIcon(":/icon/icon/icon-delete.png")));
    button_remove->setFixedSize(80, 30);
    button_remove->setEnabled( false );
    button_remove->setObjectName("btnRemove");
    button_remove->setProperty("useIconHighlightEffect", 0x8);

    //清空按钮
    button_clear = new QPushButton(i18n("Clear"), label_action );
    button_clear->setIcon(QIcon::fromTheme("edit-delete", QIcon(":/icon/icon/icon-clean.png")));
    button_clear->setFixedSize(80, 30);
    button_clear->setEnabled( false );
    button_clear->setObjectName("btnClear");
    button_clear->setProperty("useIconHighlightEffect", 0x8);

    //新建文件夹按钮
    button_newdir = new QPushButton(i18n("New Dir"), label_action );
    button_newdir->setIcon(QIcon::fromTheme("folder-add" , QIcon(":/icon/icon/icon-newfolder.png")));
    button_newdir->setFixedSize(112, 30);
    button_newdir->setObjectName("btnNewDir");
    button_newdir->setProperty("useIconHighlightEffect", 0x8);


    QLabel *labelFileFilter = new QLabel( this );
    labelFileFilter->setMinimumSize(132, 30);
    btnFileFilter = new QPushButton(i18n("FileFilter"), labelFileFilter);
    btnFileFilter->setFixedSize(112, 30);
    btnFileFilter->setProperty("useIconHighlightEffect", 0x2);
    btnFileFilter->setFlat(true);
    btnFileFilter->setToolTip(i18n("Only invalid at level root."));
#if 0
    btnFileFilter->setStyleSheet("QPushButton{ background-color:transparent; border-radius:4px; font: 14px;}"
                                 "QPushButton::hover{background-color:#6b8eeb; border-radius:4px; font: 14px;}"
                                 "QPushButton::pressed{background-color:#415fc4; border-radius:4px; font: 14px;}");
#endif


    //安装事件过滤器
    //button_add->installEventFilter(this);
    //button_remove->installEventFilter(this);
    //button_clear->installEventFilter(this);
    //button_newdir->installEventFilter(this);

    //QLabel* label_action = new QLabel( this );
    QHBoxLayout* layout_action = new QHBoxLayout( label_action );
    layout_action->setContentsMargins(0,0,0, 10);
    layout_action->addWidget( button_add );
    layout_action->addStretch(8);
    layout_action->addWidget( button_remove );
    layout_action->addStretch(8);
    layout_action->addWidget( button_clear );
    layout_action->addStretch(8);
    layout_action->addWidget( button_newdir );


    QHBoxLayout* layoutFileFilter = new QHBoxLayout( labelFileFilter );
    layoutFileFilter->setContentsMargins(20,0,0,0);
    layoutFileFilter->addWidget( btnFileFilter );

    
    connect( button_add, SIGNAL( clicked() ), this, SLOT( slotOpenClicked() ) );
    connect( button_remove, SIGNAL( clicked() ), this, SLOT( slotRemoveClicked() ) );
    connect( button_clear, SIGNAL( clicked() ), this, SLOT( slotClearClicked()  ) );
    connect( button_newdir, SIGNAL( clicked() ), this, SLOT( slotNewdirClicked() ) );
    connect( btnFileFilter, SIGNAL( clicked() ), this, SLOT( slotFileFilterClicked() ) );

    toolBox()->addWidget( label_action );
    toolBox()->addWidget( labelFileFilter );
    //toolBox()->addAction( actionCollection()->action( "project_volume_name" ) );
    m_oProjectSize = new QLabel(i18n("Project Size:"), this);
    m_oSize = new QLabel(KIO::convertSize(m_doc->size()), this);
    QFont fl = m_oProjectSize->font();
    fl.setPixelSize(14);
    m_oProjectSize->setFont(fl);
    m_oSize->setFont(fl);
    toolBox()->addWidget(m_oProjectSize);
    toolBox()->addWidget(m_oSize);

    QLabel *tmp = new QLabel(this);
    tmp->setFixedWidth(25);
    toolBox()->addWidget(tmp);
    btnFileFilter->hide();


#if 0
    workerThread = new QThread;
    LoadWorker *worker = new LoadWorker;
    worker->moveToThread(workerThread);
    connect(this,SIGNAL(load(QString, DataDoc*)),worker,SLOT(load(QString, DataDoc*)));
    connect(worker,SIGNAL(loadFinished()),this,SLOT(onLoadFinished()));
    connect(workerThread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    workerThread->start();
#endif

    QFont f = burn_setting->font();
    f.setPixelSize(14);
    combo_burner->setFont(f);
    combo_CD->setFont(f);
    burn_setting->setFont(f);
    button_add->setFont(f);
    button_remove->setFont(f);
    button_clear->setFont(f);
    button_newdir->setFont(f);
    btnFileFilter->setFont(f);
    f.setPixelSize(18);
    burn_button->setFont(f);
    QPalette p = palette();
    p.setColor(QPalette::Background, QColor("#FFFFFF"));
    setPalette(p);


    f = m_oProjectSize->font();
    f.setPixelSize(14);
    m_oProjectSize->setFont(f);
    m_oSize->setFont(f);


    pdlg = newBurnDialog( this );
    disableBurnSetting();
    if (logger) logger->debug("Draw data burner end");
}

K3b::DataView::~DataView()
{
    delete dlgFileFilter;
    if (pdlg) delete pdlg;
}

void K3b::DataView::slotFileFilterClicked()
{
    //K3b::DirItem *d = m_doc->root();
    //K3b::DataItem *c;
    dlgFileFilter->setAttribute(Qt::WA_ShowModal);
    dlgFileFilter->setDoFileFilter(combo_CD->currentIndex());
    //dlgFileFilter->slotDoFileFilter(docs[combo_CD->currentIndex()]);

    dlgFileFilter->show();
    QPoint p(k3bappcore->k3bMainWindow()->pos().x() + (k3bappcore->k3bMainWindow()->width() - dlgFileFilter->width()) / 2,
             k3bappcore->k3bMainWindow()->pos().y() + (k3bappcore->k3bMainWindow()->height() - dlgFileFilter->height()) / 2);
    dlgFileFilter->move(p);
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
        if (obj == button_add) hoverButtonAdd();
        else if (obj == button_remove) hoverButtonRemove();
        else if (obj == button_clear) hoverButtonClear();
        else if (obj == button_newdir) hoverButtonNewDir();
        else if (obj == burn_button) hoverButtonBurn();
        else break;
        break;
    case QEvent::HoverLeave:
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
            button_add->setIcon(QIcon(":/icon/icon/icon-add-click.png"));
        }
        if(obj == button_remove){
            if ( button_remove->isEnabled() )
            {
                pressButtonRemove();
                button_remove->setIcon(QIcon(":/icon/icon/icon-delete-click.png"));
            }
        }
        if(obj == button_clear){
            if ( button_clear->isEnabled() )
            {
                pressButtonClear();
                button_clear->setIcon(QIcon(":/icon/icon/icon-clean-click.png"));
            }
        }
        if(obj == button_newdir)
        {
            pressButtonNewDir();
            button_newdir->setIcon(QIcon(":/icon/icon/icon-newfolder-click.png"));
        }
        else if (obj == burn_button) pressButtonBurn();
        break;
    case QEvent::DragEnter:
        if (obj == tips)
        {
            if (logger) logger->debug("Tips drag in...");
            event->accept();
        }
        break;
    case QEvent::DragLeave:
        if (obj == tips) event->ignore();
        break;
   case QEvent::Drop:
        if (obj == tips /*|| obj == m_dataViewImpl->view()*/)
        {
            dropEvent = static_cast<QDropEvent *>(event);
            if (lastDrop.isEmpty() || lastDrop != dropEvent->mimeData()->urls())
            {
                lastDrop = dropEvent->mimeData()->urls();
            }
            else return QWidget::eventFilter(obj, event);
            if (logger) logger->debug("Tips drag in...");
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
                for (int i = 0; i < docs.size(); ++i)
                {
                    docs[i]->addUrls(lastDrop);
                    m_oSize->setText(KIO::convertSize(m_doc->size()));
                    QCoreApplication::processEvents();
                }
                copyData(m_doc, docs.at(combo_CD->currentIndex()));
                if (dlgFileFilter->getStatus(combo_CD->currentIndex()).isHidden)
                    slotOption(0, dlgFileFilter->getStatus(combo_CD->currentIndex()).isHidden);
                if (dlgFileFilter->getStatus(combo_CD->currentIndex()).isBroken)
                    slotOption(1, dlgFileFilter->getStatus(combo_CD->currentIndex()).isBroken);
                if (dlgFileFilter->getStatus(combo_CD->currentIndex()).isReplace)
                    slotOption(2, dlgFileFilter->getStatus(combo_CD->currentIndex()).isReplace);
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

void K3b::DataView::paintEvent(QPaintEvent *e)
{
    QPalette pal = QApplication::style()->standardPalette();

    QColor c;
    c.setRed(231); c.setBlue(231); c.setGreen(231);
    if (c == pal.background().color())
    {
        m_dataViewImpl->whiteHeader();
    }
    else
    {
        m_dataViewImpl->blackHeader();
    }
    QWidget::paintEvent(e);
}

void K3b::DataView::slotAddFile(QList<QUrl> urls)
{
    unsigned long long dataSize, mediumSize;
    int idx = combo_CD->currentIndex();
    m_oSize->setStyleSheet("color : green;");
    if (idx > 1)
    {
        mediumSize = device_index.at(idx -1)->diskInfo().remainingSize().mode1Bytes();
        dataSize = m_doc->size();
        if (dataSize > mediumSize)
        {
#if 0
            QMessageBox::critical(nullptr,
                                  i18n("Overload"),
                                  i18n("The data size over the medium size."));
#endif
            m_oSize->setStyleSheet("color : red;");
            return;
        }
    }
#if 1
    for (int i = 0; i < docs.size(); ++i)
    {
        docs[i]->addUrls(urls);
        m_oSize->setText(KIO::convertSize(m_doc->size()));
        QCoreApplication::processEvents();
    }
#endif
}

void K3b::DataView::slotOpenClicked()
{
    int ret = m_dataViewImpl->slotOpenDir();
    if (ret)
    {
        copyData(docs.at(combo_CD->currentIndex()), m_doc);
        m_oSize->setText(KIO::convertSize(m_doc->size()));
        if (dlgFileFilter->getStatus(combo_CD->currentIndex()).isHidden)
            slotOption(0, dlgFileFilter->getStatus(combo_CD->currentIndex()).isHidden);
        if (dlgFileFilter->getStatus(combo_CD->currentIndex()).isBroken)
            slotOption(1, dlgFileFilter->getStatus(combo_CD->currentIndex()).isBroken);
        if (dlgFileFilter->getStatus(combo_CD->currentIndex()).isReplace)
            slotOption(2, dlgFileFilter->getStatus(combo_CD->currentIndex()).isReplace);
    }
}

void K3b::DataView::slotRemoveClicked()
{
    m_dataViewImpl->slotRemove();
    lastDrop.clear();
    copyData(docs.at(combo_CD->currentIndex()), m_doc);

    m_oSize->setText(KIO::convertSize(m_doc->size()));
}

void K3b::DataView::slotClearClicked()
{
    //const QModelIndex parentDirectory = m_dataViewImpl->view()->rootIndex();
    QMessageBox box(QMessageBox::Warning,
                    i18n("Clean Data"),
                    i18n("Are you sure to clean all data?"),
                    QMessageBox::Ok | QMessageBox::Cancel,
                    this);
    QPoint p(k3bappcore->k3bMainWindow()->pos().x() + (k3bappcore->k3bMainWindow()->width() - box.width()) / 2,
             k3bappcore->k3bMainWindow()->pos().y() + (k3bappcore->k3bMainWindow()->height() - box.height()) / 2);
    box.move(p);
    if (box.exec() != QMessageBox::Ok) return;
    m_dataViewImpl->slotClear();
    lastDrop.clear();
    copyData(docs.at(combo_CD->currentIndex()), m_doc);
}

void K3b::DataView::slotNewdirClicked()
{
    m_dataViewImpl->slotNewDir();
    copyData(docs.at(combo_CD->currentIndex()), m_doc);
}

void K3b::DataView::slotDeviceChange( K3b::Device::DeviceManager* manager )
{
    if (logger) logger->debug("Device changed...");
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
            if (logger) logger->debug("remove device [%s] at index %d on selection item index %d",
                          (device_index[j]->blockDeviceName()).toStdString().c_str(), j, j + 1);
        }
        QCoreApplication::processEvents();
    }

    for (int i = 0; i < device_list.size(); ++i)
    {
        if (-1 == device_index.indexOf(device_list[i])
                && !(device_list[i]->vendor().startsWith("Linux")))
        {
            if (logger) logger->debug("add device [%s] at index %d on selection item index",
                          (device_list[i]->blockDeviceName()).toStdString().c_str(), i);
            slotMediaChange(device_list[i]);
        }
        QCoreApplication::processEvents();
    }
}

void K3b::DataView::slotMediaChange( K3b::Device::Device* dev )
{
    if (logger) logger->debug("Media changed..., device : %s", dev->blockDeviceName().toStdString().c_str());

    int     idx = -1;
    QString cdSize;
    QString cdInfo;
    QString mountInfo;
    K3b::DataDoc *tmpDoc = NULL;
    K3b::Medium medium = k3bappcore->mediaCache()->medium( dev );
    KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByDevice( dev->blockDeviceName() );

    QCoreApplication::processEvents();

    // start with Linux, not a CDROM
    if (dev->vendor().startsWith("Linux")) return;

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
                if (logger) logger->debug("Device [%s] have no medium, do not show in selection item."
                              "delete it at idx %d, data idx %d",
                              dev->blockDeviceName().toStdString().c_str(), idx, idx + 1);
                device_index.removeAt(idx++);
                tmpDoc = docs[idx];
                if (tmpDoc) delete tmpDoc;
                docs.removeAt(idx);
                if (logger) logger->debug("Remove data at index : %d", docs.indexOf(tmpDoc));
                dlgFileFilter->removeData(idx);
                combo_burner->removeItem(idx);
                combo_CD->removeItem(idx);
                --comboIndex;
                if (idx - 1 < 0 ) combo_CD->setCurrentIndex(0);
                else combo_CD->setCurrentIndex(idx - 1);
            }
            else
            {
                if (logger) logger->debug("Device [%s] have no medium, do not show in selection item.",
                              dev->blockDeviceName().toStdString().c_str());
            }
            return;
        }
    }
    else
    {
        mountInfo = mountPoint->mountPoint();
        cdInfo = medium.shortString(K3b::Medium::WithContents) + i18n(", remaining available space") + cdSize;
        if (i18n("No medium present") == medium.shortString())
        {
            idx = device_index.indexOf(dev);
            device_index.removeAt(idx++);
            tmpDoc = docs[idx];
            if (tmpDoc) delete tmpDoc;
            docs.removeAt(idx);
            if (logger) logger->debug("Remove data at index : %d", docs.indexOf(tmpDoc));
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
                                 QIcon(":/icon/icon/icon-burner.png"),
                                 dev->vendor() + " - " + dev->description());
        combo_CD->insertItem(comboIndex++, QIcon(":/icon/icon/icon-disk.png"), i18n("cdrom file: ") + cdInfo);
        tmpDoc = new K3b::DataDoc(this);
        tmpDoc->setURL(m_doc->URL());
        tmpDoc->newDocument();
        if (tmpDoc) docs << tmpDoc;
        if (logger) logger->debug("Add new data at index : %d", docs.indexOf(tmpDoc));
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
#if 1
        setDisabled(true);
        for ( int i = 0; i < fileinfo.count(); i++ )
        {
            qDebug() << fileinfo.at(i).fileName();
            QCoreApplication::processEvents();
            if (fileinfo.at(i).fileName() == "." ||
                    fileinfo.at(i).fileName() == "..") continue;
            tmpDoc->addUnremovableUrls( QList<QUrl>() <<  QUrl::fromLocalFile(fileinfo.at(i).filePath()) );
        }
        setDisabled(false);
#endif
        copyData(m_doc, tmpDoc);
        combo_CD->setCurrentIndex(idx);
        lastIndex = combo_CD->currentIndex();
    }
    if (0 == m_dataViewImpl->view()->model()->rowCount() ||
            0 == m_doc->root()->children().size())
    {
        m_dataViewImpl->view()->setFixedHeight(28);
        tips->show();
        tips->installEventFilter(this);
    }
    else
    {
        tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
    }
    m_oSize->setText(KIO::convertSize(m_doc->size()));
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

    if (device_index.size()) emit cdrom(true);
    else cdrom(false);

    tmpDoc = docs[index];
    m_doc->clear();
    copyData(m_doc, tmpDoc);
    combo_burner->setCurrentIndex( index );
    if (index)
    {
        K3b::Medium medium = k3bappcore->mediaCache()->medium( device_index.at(index - 1) );
        qDebug() << (medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE)
                 << medium.diskInfo().diskState();
        if (medium.diskInfo().diskState() == K3b::Device::STATE_INCOMPLETE)
        {
            // can appendale medium,default set all busness button to false;
            burn_button->setEnabled(false);
        }
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
#if 0
    slotOption(0, dlgFileFilter->getStatus(index).isHidden);
    slotOption(1, dlgFileFilter->getStatus(index).isBroken);
    slotOption(2, dlgFileFilter->getStatus(index).isReplace);
#endif
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


    if (logger) logger->debug("Start Burn button cliked. now project size is %llu", m_doc->burningSize());


    if( m_doc->root()->children().size() == 0 ) {
         KMessageBox::information( this, i18n("Please add files to your project first."),
                                      i18n("No Data to Burn") );
    }else if ( burn_button->text() == i18n("start burner" )){ 
        if (logger) logger->debug("Button model is start burner");
        int index = combo_burner->currentIndex();
        --index;
        dlg->setComboMedium( device_index.at( index ) );
        m_doc->setBurner( device_index.at( index ) );
        dlg->setOnlyCreateImage(pdlg->onlyCreateImage());
        dlg->setCacheImage(pdlg->cacheImage());
        dlg->setTmpPath(image_path);
        dlg->setSpeed(pdlg->speed());
        qDebug()<< "index :" <<  index << " device block name: " << device_index.at( index )->blockDeviceName() <<endl;
        dlg->slotStartClicked();
    }else if( burn_button->text() == i18n("create iso" )){ 
        if (logger) logger->debug("Button model is create iso");
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
       if( m_doc->root()->children().size() == 0 ) {
         QMessageBox::information( nullptr, i18n("open"), i18n("No Data to Burn"));
    }else if ( burn_setting->text() == i18n("setting") ){
        pdlg->execBurnDialog(true);
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
    //burn_setting->setStyleSheet("background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;color : #444444");
}

void K3b::DataView::hoverBurnSetting(bool in)
{
    if (!burn_setting->isEnabled()) return;
    /*
    if (in)
        burn_setting->setStyleSheet("background-color:rgb(107, 142, 235);font: 14px;border-radius: 4px;color:#444444");
    else
        burn_setting->setStyleSheet("background-color:rgb(233, 233, 233);font: 14px;border-radius: 4px;color : #444444");
    */
}

void K3b::DataView::pressBurnSetting()
{
    //burn_setting->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableBurnSetting()
{
    if (burn_setting->isEnabled()) burn_setting->setEnabled(false);
    /*
    if (!burn_setting->isEnabled())
    {
        burn_setting->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
    */
}

void K3b::DataView::enableButtonBurn()
{
    burn_button->setEnabled(true);
    //burn_button->setStyleSheet("background-color:rgba(61, 107, 229, 1);font: 18px;border-radius: 4px;color:#ffffff");
}

void K3b::DataView::hoverButtonBurn(bool in)
{
    if (!burn_button->isEnabled()) return;
    /*
    if (in)
        burn_button->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 18px;border-radius: 4px;color:#ffffff");
    else
        burn_button->setStyleSheet("background-color:rgba(61, 107, 229, 1);font: 18px;border-radius: 4px;color:#ffffff");
    */
}

void K3b::DataView::pressButtonBurn()
{
    if (!burn_button->isEnabled()) return;
    //burn_button->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 18px;border-radius: 4px;color:#ffffff");
}

void K3b::DataView::disableButtonBurn()
{
    if (burn_button->isEnabled()) burn_button->setEnabled(false);
    /*
    if (!burn_button->isEnabled())
    {
        burn_button->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 18px;border-radius: 4px;color:#C1C1C1");
    }
    */
}

void K3b::DataView::enableButtonAdd()
{
    button_add->setEnabled(true);
    button_add->setIcon(QIcon(":/icon/icon/icon-add.png"));
    //button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonAdd(bool in)
{
    if (!button_add->isEnabled()) return;
    if (in)
    {
        button_add->setIcon(QIcon(":/icon/icon/icon-add-click.png"));
        //button_add->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_add->setIcon(QIcon(":/icon/icon/icon-add.png"));
        //button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
}

void K3b::DataView::pressButtonAdd()
{
    if (!button_add->isEnabled()) return;
    button_add->setIcon(QIcon(":/icon/icon/icon-add-click.png"));
    //button_add->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonAdd()
{
    if (button_add->isEnabled()) button_add->setEnabled(false);
    if (!button_add->isEnabled())
    {
        button_add->setIcon(QIcon(":/icon/icon/icon-add.png"));
        //button_add->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonRemove()
{
    button_remove->setEnabled(true);
    button_remove->setIcon(QIcon(":/icon/icon/icon-delete.png"));
    //button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonRemove(bool in)
{
    if (!button_remove->isEnabled()) return;
    if (in)
    {
        button_remove->setIcon(QIcon(":/icon/icon/icon-delete-click.png"));
        //button_remove->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_remove->setIcon(QIcon(":/icon/icon/icon-delete.png"));
        //button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
}

void K3b::DataView::pressButtonRemove()
{
    if (!button_remove->isEnabled()) return;
    button_remove->setIcon(QIcon(":/icon/icon/icon-delete-click.png"));
    //button_remove->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonRemove()
{
    if (button_remove->isEnabled()) button_remove->setEnabled(false);
    if (!button_remove->isEnabled())
    {
        button_remove->setIcon(QIcon(":/icon/icon/icon-delete.png"));
        //button_remove->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonClear()
{
    button_clear->setEnabled(true);
    button_clear->setIcon(QIcon(":/icon/icon/icon-clean.png"));
    //button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonClear(bool in)
{
    if (!button_clear->isEnabled()) return;
    if (in)
    {
        button_clear->setIcon(QIcon(":/icon/icon/icon-clean-click.png"));
        //button_clear->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_clear->setIcon(QIcon(":/icon/icon/icon-clean.png"));
        //button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
}

void K3b::DataView::pressButtonClear()
{
    if (!button_clear->isEnabled()) return;
    button_clear->setIcon(QIcon(":/icon/icon/icon-clean-click.png"));
    //button_clear->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonClear()
{
    if (button_clear->isEnabled()) button_clear->setEnabled(false);
    if (!button_clear->isEnabled())
    {
        button_clear->setIcon(QIcon(":/icon/icon/icon-clean.png"));
        //button_clear->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

void K3b::DataView::enableButtonNewDir()
{
    button_newdir->setEnabled(true);
    button_newdir->setIcon(QIcon(":/icon/icon/icon-newfolder.png"));
   // button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::hoverButtonNewDir(bool in)
{
    if (!button_newdir->isEnabled()) return;
    if (in)
    {
        button_newdir->setIcon(QIcon(":/icon/icon/icon-newfolder-click.png"));
        //button_newdir->setStyleSheet("background-color:rgba(107, 142, 235, 1);font: 14px;border-radius: 4px;color:#444444");
    }
    else
    {
        button_newdir->setIcon(QIcon(":/icon/icon/icon-newfolder.png"));
        //button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#444444");
    }
}

void K3b::DataView::pressButtonNewDir()
{
    if (!button_newdir->isEnabled()) return;
    button_newdir->setIcon(QIcon(":/icon/icon/icon-newfolder-click.png"));
    //button_newdir->setStyleSheet("background-color:rgba(65, 95, 196, 1);font: 14px;border-radius: 4px;color:#444444");
}

void K3b::DataView::disableButtonNewDir()
{
    if (button_newdir->isEnabled()) button_newdir->setEnabled(false);
    if (!button_newdir->isEnabled())
    {
        button_newdir->setIcon(QIcon(":/icon/icon/icon-newfolder.png"));
        //button_newdir->setStyleSheet("background-color:rgba(233, 233, 233, 1);font: 14px;border-radius: 4px;color:#C1C1C1");
    }
}

bool K3b::DataView::checkIsDeleteable(K3b::DataDoc *doc)
{
    for (int i = 0; i < doc->root()->children().size(); ++i)
    {
        if (doc->root()->children().at(i)->isDeleteable()) return true;
    }
    return false;
}

void K3b::DataView::copyData(K3b::DataDoc *target, K3b::DataDoc *source)
{
    K3b::DataItem *child = NULL;
    target->clear();

    setDisabled(true);

    for (int i = 0; i < source->root()->children().size(); ++i)
    {
        QCoreApplication::processEvents();
        child = source->root()->children().at(i);
        if (child->isDeleteable())
            target->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
        else
            target->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
    }

    bool cls = checkIsDeleteable(m_doc);

    if (m_doc->root()->children().size() == 0)
    {
        disableButtonRemove();
        disableButtonClear();
        disableButtonBurn();
        disableBurnSetting();
        btnFileFilter->hide();
        m_dataViewImpl->view()->setFixedHeight(28);
        if (tips)
        {
            tips->show();
            tips->installEventFilter(this);
        }
    }
    else if (!cls)
    {
        disableButtonRemove();
        disableButtonClear();
        //enableButtonBurn();
        burn_button->setEnabled(false);
        enableBurnSetting();
        btnFileFilter->show();
        if (tips) tips->hide();
        m_dataViewImpl->view()->setFixedHeight(370);
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

    unsigned long long dataSize, mediumSize;
    m_oSize->setText(KIO::convertSize(m_doc->size()));
    int idx = combo_CD->currentIndex();
    m_oSize->setStyleSheet("color : green;");
    if (idx > 1)
    {
        mediumSize = device_index.at(idx -1)->diskInfo().remainingSize().mode1Bytes();
        dataSize = m_doc->size();
        if (dataSize > mediumSize)
        {
#if 0
            QMessageBox::critical(nullptr,
                                  i18n("Overload"),
                                  i18n("The data size over the medium size."));
#endif
            m_oSize->setStyleSheet("color : red;");
        }
    }
    setDisabled(false);
}

void K3b::DataView::isHidden(bool flag)
{
    if (logger) logger->debug("call to set file filter hidden is %s",
                  flag ? "true" : "false");
    K3b::DataDoc *tmp = dlgFileFilter->getData(combo_CD->currentIndex());
    K3b::DataItem *child = NULL;
    if (!tmp)
    {
        if (logger) logger->error("Cannot get the file filter data at index : %d",
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
                if (logger) logger->info("Filter hidden item <%s>", child->localPath().toStdString().c_str());
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
                if (logger) logger->info("Recovery hidden item <%s>", child->localPath().toStdString().c_str());
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
    if (logger) logger->debug("call to set file filter broken link is %s",
                  flag ? "true" : "false");
    K3b::DataDoc *tmp = dlgFileFilter->getData(combo_CD->currentIndex());
    K3b::DataItem *child = NULL;
    if (!tmp)
    {
        if (logger) logger->error("Cannot get the file filter data at index : %d",
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
                if (logger) logger->info("Filter broken link item <%s>", child->localPath().toStdString().c_str());
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
                if (logger) logger->info("Recovery broken link item <%s>", child->localPath().toStdString().c_str());
                tmp->removeItem(child);
            }
        }
        copyData(docs[combo_CD->currentIndex()], m_doc);
    }
    dlgFileFilter->setBroken(combo_CD->currentIndex(), flag);
    //dlgFileFilter->reload(combo_CD->currentIndex());
    dlgFileFilter->slotDoFileFilter(docs[combo_CD->currentIndex()]);
}


void K3b::DataView::isReplace(bool flag)
{
    if (logger) logger->debug("call to set file filter replace link is %s",
                  flag ? "true" : "false");
    K3b::DataDoc *tmp = dlgFileFilter->getData(combo_CD->currentIndex());
    K3b::DataItem *child = NULL;
    if (!tmp)
    {
        if (logger) logger->error("Cannot get the file filter data at index : %d",
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
                QFileInfo f(child->localPath());
                if (child->isDeleteable())
                {
                    tmp->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    tmp->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                if (logger) logger->info("Filter replace link item <%s>", child->localPath().toStdString().c_str());
                m_doc->removeItem(child);
                --i;
                m_doc->addUrl(QUrl::fromLocalFile(f.symLinkTarget()));
                //copyData(m_doc, docs[combo_CD->currentIndex()]);
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
                QFileInfo f(child->localPath());
                QFileInfo fi(f.symLinkTarget());
                for (int j = 0; j < m_doc->root()->children().size(); ++j)
                {
                    if (m_doc->root()->children().at(j)->k3bName() ==
                            fi.fileName())
                        m_doc->removeItem(m_doc->root()->children().at(j));
                }
                if (child->isDeleteable())
                {
                    m_doc->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                else
                {
                    m_doc->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                }
                if (logger) logger->info("Recovery replace link item <%s>", child->localPath().toStdString().c_str());
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
    if (logger) logger->debug("Do option %d to %s.", option, flag ? "true" : "false");
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

