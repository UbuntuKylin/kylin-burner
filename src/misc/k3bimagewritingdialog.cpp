/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bimagewritingdialog.h"
#include "k3biso9660imagewritingjob.h"
#include "k3bbinimagewritingjob.h"
#include "k3bcuefileparser.h"
#include "k3bclonetocreader.h"
#include "k3baudiocuefilewritingjob.h"
#include "k3bclonejob.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"

#include <config-kylinburner.h>

#include "k3btempdirselectionwidget.h"
#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bwriterselectionwidget.h"
#include "k3bburnprogressdialog.h"
#include "k3bstdguiitems.h"
#include "k3bmd5job.h"
#include "k3bdatamodewidget.h"
#include "k3bglobals.h"
#include "k3bwritingmodewidget.h"
#include "k3bcore.h"
#include "k3biso9660.h"
#include "k3btoc.h"
#include "k3btrack.h"
#include "k3bcdtext.h"

#include <KComboBox>
#include <KConfig>
#include <KSharedConfig>
#include <KColorScheme>
#include <KIconLoader>
#include <KIO/Global>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGuiItem>
#include <KUrlRequester>

#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QMimeData>
#include <QUrl>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QCheckBox>
#include <QInputDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolTip>
#include <QTreeWidget>
#include "k3bResultDialog.h"
#include <QBitmap>
#include <QPainter>

namespace {

    enum ImageType {
        IMAGE_UNKNOWN,
        IMAGE_ISO,
        IMAGE_CUE_BIN,
        IMAGE_AUDIO_CUE,
        IMAGE_CDRDAO_TOC,
        IMAGE_CDRECORD_CLONE,
        IMAGE_RAW
    };

} // namespace

class K3b::ImageWritingDialog::Private
{
public:
    Private()
        : md5SumItem(0),
          haveMd5Sum( false ),
          foundImageType( IMAGE_UNKNOWN ),
          imageForced( false ) {
    }

    WriterSelectionWidget* writerSelectionWidget;
    QCheckBox* checkDummy;
    QCheckBox* checkNoFix;
    QCheckBox* checkCacheImage;
    QCheckBox* checkVerify;
    DataModeWidget* dataModeWidget;
    WritingModeWidget* writingModeWidget;
    QSpinBox* spinCopies;

    KUrlRequester* editImagePath;
    KComboBox* comboRecentImages;
    QComboBox* comboImageType;

    QTreeWidget* infoView;
    TempDirSelectionWidget* tempDirSelectionWidget;

    QTreeWidgetItem* md5SumItem;
    QString lastCheckedFile;

    K3b::Md5Job* md5Job;
    bool haveMd5Sum;

    ImageType foundImageType;

    QMap<int,int> imageTypeSelectionMap;
    QMap<int,int> imageTypeSelectionMapRev;
    QString imageFile;
    QString tocFile;

    QTabWidget* optionTabbed;

    QWidget* advancedTab;
    QWidget* tempPathTab;
    bool advancedTabVisible;
    bool tempPathTabVisible;

    int advancedTabIndex;
    int tempPathTabIndex;

    bool imageForced;

    QColor infoTextColor;
    QColor negativeTextColor;
    QColor normalTextColor;

    static KIO::filesize_t volumeSpaceSize( const Iso9660& iso );
    void createIso9660InfoItems( Iso9660* );
    void createCdrecordCloneItems( const QString&, const QString& );
    void createCueBinItems( const QString&, const QString& );
    void createAudioCueItems( const CueFileParser& cp );
    int currentImageType();
    QString imagePath() const;
    bool flag;
};


KIO::filesize_t K3b::ImageWritingDialog::Private::volumeSpaceSize( const Iso9660& isoFs )
{
    return static_cast<KIO::filesize_t>( isoFs.primaryDescriptor().volumeSpaceSize*2048 );
}


void K3b::ImageWritingDialog::Private::createIso9660InfoItems( K3b::Iso9660* isoF )
{
    QTreeWidgetItem* isoRootItem = new QTreeWidgetItem( infoView );
    isoRootItem->setText( 0, i18n("Detected:") );
    isoRootItem->setText( 1, i18n("ISO 9660 image") );
    isoRootItem->setForeground( 0, infoTextColor );
    isoRootItem->setIcon( 1, QIcon::fromTheme( "application-x-cd-image") );
    isoRootItem->setTextAlignment( 0, Qt::AlignRight );

    const KIO::filesize_t size = K3b::filesize( QUrl::fromLocalFile(isoF->fileName()) );
    const KIO::filesize_t volumeSpaceSize = Private::volumeSpaceSize( *isoF );

    QTreeWidgetItem* item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Filesize:") );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );
    if( size < volumeSpaceSize ) {
        item->setText( 1, i18n("%1 (different than declared volume size)", KIO::convertSize( size )) );
        item->setForeground( 1, negativeTextColor );
        item->setIcon( 1, QIcon::fromTheme( "dialog-error") );

        item = new QTreeWidgetItem( infoView );
        item->setText( 0, i18n("Volume Size:") );
        item->setText( 1, KIO::convertSize( volumeSpaceSize ) );
        item->setForeground( 0, infoTextColor );
        item->setTextAlignment( 0, Qt::AlignRight );
    }
    else {
        item->setText( 1, KIO::convertSize( size ) );
    }

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("System Id:") );
    item->setText( 1, isoF->primaryDescriptor().systemId.isEmpty()
                      ? QString("-")
                      : isoF->primaryDescriptor().systemId );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Volume Id:") );
    item->setText( 1, isoF->primaryDescriptor().volumeId.isEmpty()
                      ? QString("-")
                      : isoF->primaryDescriptor().volumeId );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Volume Set Id:") );
    item->setText( 1, isoF->primaryDescriptor().volumeSetId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().volumeSetId );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Publisher Id:") );
    item->setText( 1, isoF->primaryDescriptor().publisherId.isEmpty()
                                ? QString("-")
                                : isoF->primaryDescriptor().publisherId );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Preparer Id:") );
    item->setText( 1, isoF->primaryDescriptor().preparerId.isEmpty()
                                ? QString("-") : isoF->primaryDescriptor().preparerId );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Application Id:") );
    item->setText( 1, isoF->primaryDescriptor().applicationId.isEmpty()
                      ? QString("-")
                      : isoF->primaryDescriptor().applicationId );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );
}


void K3b::ImageWritingDialog::Private::createCdrecordCloneItems( const QString& tocFile, const QString& imageFile )
{
    QTreeWidgetItem* isoRootItem = new QTreeWidgetItem( infoView );
    isoRootItem->setText( 0, i18n("Detected:") );
    isoRootItem->setText( 1, i18n("Cdrecord clone image") );
    isoRootItem->setForeground( 0, infoTextColor );
    isoRootItem->setIcon( 1, QIcon::fromTheme( "application-x-cd-image") );
    isoRootItem->setTextAlignment( 0, Qt::AlignRight );

    QTreeWidgetItem* item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Filesize:") );
    item->setText( 1, KIO::convertSize( K3b::filesize(QUrl::fromLocalFile(imageFile)) ) );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Image file:") );
    item->setText( 1, imageFile );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("TOC file:") );
    item->setText( 1, tocFile );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );
}


void K3b::ImageWritingDialog::Private::createCueBinItems( const QString& cueFile, const QString& imageFile )
{
    QTreeWidgetItem* isoRootItem = new QTreeWidgetItem( infoView );
    isoRootItem->setText( 0, i18n("Detected:") );
    isoRootItem->setText( 1, i18n("Cue/bin image") );
    isoRootItem->setForeground( 0, infoTextColor );
    isoRootItem->setIcon( 1, QIcon::fromTheme( "application-x-cd-image") );
    isoRootItem->setTextAlignment( 0, Qt::AlignRight );

    QTreeWidgetItem* item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Filesize:") );
    item->setText( 1, KIO::convertSize( K3b::filesize(QUrl::fromLocalFile(imageFile)) ) );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Image file:") );
    item->setText( 1, imageFile );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );

    item = new QTreeWidgetItem( infoView );
    item->setText( 0, i18n("Cue file:") );
    item->setText( 1, cueFile );
    item->setForeground( 0, infoTextColor );
    item->setTextAlignment( 0, Qt::AlignRight );
}


void K3b::ImageWritingDialog::Private::createAudioCueItems( const K3b::CueFileParser& cp )
{
    QTreeWidgetItem* rootItem = new QTreeWidgetItem( infoView );
    rootItem->setText( 0, i18n("Detected:") );
    rootItem->setText( 1, i18n("Audio Cue Image") );
    rootItem->setForeground( 0, infoTextColor );
    rootItem->setIcon( 1, QIcon::fromTheme( "audio-x-generic") );
    rootItem->setTextAlignment( 0, Qt::AlignRight );

    QTreeWidgetItem* trackParent = new QTreeWidgetItem( infoView );
    trackParent->setText( 0, i18np("One track", "%1 tracks", cp.toc().count() ) );
    trackParent->setText( 1, cp.toc().length().toString() );
    if( !cp.cdText().isEmpty() ) {
        trackParent->setText( 1,
                              QString("%1 (%2 - %3)")
                              .arg(trackParent->text(1))
                              .arg(cp.cdText().performer())
                              .arg(cp.cdText().title()) );
    }

    int i = 1;
    foreach( const K3b::Device::Track& track, cp.toc() ) {

        QTreeWidgetItem* trackItem = new QTreeWidgetItem( trackParent );
        trackItem->setText( 0, i18n("Track") + ' ' + QString::number(i).rightJustified( 2, '0' ) );
        trackItem->setText( 1, "    " + ( i < cp.toc().count()
                                        ? track.length().toString()
                                        : QString("??:??:??") ) );

        if( !cp.cdText().isEmpty() && (cp.cdText().count() > 0) &&!cp.cdText()[i-1].isEmpty() )
            trackItem->setText( 1,
                                QString("%1 (%2 - %3)")
                                .arg(trackItem->text(1))
                                .arg(cp.cdText()[i-1].performer())
                                .arg(cp.cdText()[i-1].title()) );

        ++i;
    }

    trackParent->setExpanded( true );
}


int K3b::ImageWritingDialog::Private::currentImageType()
{
    if( comboImageType->currentIndex() == 0 )
        return foundImageType;
    else
        return imageTypeSelectionMap[comboImageType->currentIndex()];
}


QString K3b::ImageWritingDialog::Private::imagePath() const
{
    return K3b::convertToLocalUrl( editImagePath->url() ).toLocalFile();
}


K3b::ImageWritingDialog::ImageWritingDialog( QWidget* parent )
    : K3b::InteractionDialog( parent,
                            i18n("Burn Image"),
                            "iso cue toc",
                            START_BUTTON|CANCEL_BUTTON,
                            START_BUTTON,
                            "image writing" ) // config group
{
    d = new Private();
    const KColorScheme colorScheme( QPalette::Normal, KColorScheme::View );
    d->infoTextColor = palette().color( QPalette::Disabled, QPalette::Text );
    d->negativeTextColor = colorScheme.foreground( KColorScheme::NegativeText ).color();
    d->normalTextColor = colorScheme.foreground( KColorScheme::NormalText ).color();

    d->flag = 1;
    /*
    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(pal);
    */
    setAcceptDrops( true );

    setupGui();

    disconnect( button_ok, SIGNAL( clicked() ), this, SLOT( slotStartClicked() ) );
    connect( button_ok, SIGNAL( clicked() ), this, SLOT( accept() ) );

    d->md5Job = new K3b::Md5Job( 0, this );
    connect( d->md5Job, SIGNAL(finished(bool)),
             this, SLOT(slotMd5JobFinished(bool)) );
    connect( d->md5Job, SIGNAL(percent(int)),
             this, SLOT(slotMd5JobPercent(int)) );

    connect( d->writerSelectionWidget, SIGNAL(writerChanged()),
             this, SLOT(slotToggleAll()) );
    connect( d->writerSelectionWidget, SIGNAL(writingAppChanged(K3b::WritingApp)),
             this, SLOT(slotToggleAll()) );
    connect( d->writerSelectionWidget, SIGNAL(writerChanged(K3b::Device::Device*)),
             d->writingModeWidget, SLOT(setDevice(K3b::Device::Device*)) );
    connect( d->comboImageType, SIGNAL(activated(int)),
             this, SLOT(slotToggleAll()) );
    connect( d->writingModeWidget, SIGNAL(writingModeChanged(WritingMode)),
             this, SLOT(slotToggleAll()) );
    connect( d->editImagePath, SIGNAL(textChanged(QString)),
             this, SLOT(slotUpdateImage(QString)) );
    connect( d->checkDummy, SIGNAL(toggled(bool)),
             this, SLOT(slotToggleAll()) );
    connect( d->checkCacheImage, SIGNAL(toggled(bool)),
             this, SLOT(slotToggleAll()) );
}


K3b::ImageWritingDialog::~ImageWritingDialog()
{
    d->md5Job->cancel();

    KConfigGroup c( KSharedConfig::openConfig(), configGroup() );
    QStringList recentImages;
    // do not store more than 10 recent images
    for ( int i = 0; i < d->comboRecentImages->count() && recentImages.count() < 10; ++i ) {
        QString image = d->comboRecentImages->itemText( i );
        if ( !recentImages.contains( image ) )
            recentImages += image;
    }
    c.writePathEntry( "recent images", recentImages );

    delete d;
}


void K3b::ImageWritingDialog::init()
{
    KConfigGroup c( KSharedConfig::openConfig(), configGroup() );

    if( !d->imageForced ) {
        // when opening the dialog first the default settings are loaded and afterwards we set the
        // last written image because that's what most users want
        QString image = c.readPathEntry( "last written image", QString() );
        if( QFile::exists( image ) )
            d->editImagePath->setUrl( QUrl::fromLocalFile( image ) );

        d->comboRecentImages->clear();
    }

    d->comboRecentImages->addItems( c.readPathEntry( "recent images", QStringList() ) );
}


void K3b::ImageWritingDialog::setupGui()
{
    QWidget* frame = K3b::InteractionDialog::mainWidget();

    // image
    // -----------------------------------------------------------------------
    //QGroupBox* groupImageUrl = new QGroupBox( i18n("Image to Burn"), frame );
    QWidget* w = new QWidget();
    QGroupBox* groupImageUrl = new QGroupBox( i18n(" "), w);
    //d->comboRecentImages = new KComboBox( true, this );
    d->comboRecentImages = new KComboBox( true );
    d->comboRecentImages->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLength );
    d->editImagePath = new KUrlRequester( d->comboRecentImages, groupImageUrl );
    d->editImagePath->setMode( KFile::File|KFile::ExistingOnly );
    d->editImagePath->setWindowTitle( i18n("Choose Image File") );
    d->editImagePath->setFilter( i18n("*.iso *.toc *.ISO *.TOC *.cue *.CUE|Image Files")
                                + '\n'
                                + i18n("*.iso *.ISO|ISO 9660 Image Files")
                                + '\n'
                                + i18n("*.cue *.CUE|Cue Files")
                                + '\n'
                                + i18n("*.toc *.TOC|Cdrdao TOC Files and Cdrecord Clone Images")
                                + '\n'
                                + i18n("*|All Files") );
    QHBoxLayout* groupImageUrlLayout = new QHBoxLayout( groupImageUrl );
    groupImageUrlLayout->addWidget( d->editImagePath );

    //QGroupBox* groupImageType = new QGroupBox( i18n("Image Type"), frame );
    QGroupBox* groupImageType = new QGroupBox( i18n(" "), w );
    QHBoxLayout* groupImageTypeLayout = new QHBoxLayout( groupImageType );
    d->comboImageType = new QComboBox( groupImageType );
    groupImageTypeLayout->addWidget( d->comboImageType );
    groupImageTypeLayout->addStretch( 1 );
    d->comboImageType->addItem( i18n("Auto Detection") );
    d->comboImageType->addItem(i18n("ISO 9660 filesystem image"));
    d->comboImageType->addItem( i18n("Cue/bin image") );
    d->comboImageType->addItem( i18n("Audio cue file") );
    d->comboImageType->addItem( i18n("Cdrdao TOC file") );
    d->comboImageType->addItem( i18n("Cdrecord clone image") );
    d->comboImageType->addItem(i18n("Plain data image"));
    d->imageTypeSelectionMap[1] = IMAGE_ISO;
    d->imageTypeSelectionMap[2] = IMAGE_CUE_BIN;
    d->imageTypeSelectionMap[3] = IMAGE_AUDIO_CUE;
    d->imageTypeSelectionMap[4] = IMAGE_CDRDAO_TOC;
    d->imageTypeSelectionMap[5] = IMAGE_CDRECORD_CLONE;
    d->imageTypeSelectionMap[6] = IMAGE_RAW;
    d->imageTypeSelectionMapRev[IMAGE_ISO] = 1;
    d->imageTypeSelectionMapRev[IMAGE_CUE_BIN] = 2;
    d->imageTypeSelectionMapRev[IMAGE_AUDIO_CUE] = 3;
    d->imageTypeSelectionMapRev[IMAGE_CDRDAO_TOC] = 4;
    d->imageTypeSelectionMapRev[IMAGE_CDRECORD_CLONE] = 5;
    d->imageTypeSelectionMapRev[IMAGE_RAW] = 6;


    // image info
    // -----------------------------------------------------------------------
    //d->infoView = new QTreeWidget( frame );
    d->infoView = new QTreeWidget( );
    d->infoView->setColumnCount( 2 );
    d->infoView->headerItem()->setText( 0, "key" );
    d->infoView->headerItem()->setText( 1, "value" );
    d->infoView->setHeaderHidden( true );
    d->infoView->setSelectionMode( QAbstractItemView::NoSelection );
    d->infoView->setItemsExpandable( false );
    d->infoView->setRootIsDecorated( false );
    d->infoView->header()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    d->infoView->setFocusPolicy( Qt::NoFocus );
    d->infoView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    d->infoView->setContextMenuPolicy( Qt::CustomContextMenu );
    //d->infoView->setNoItemText( i18n("No image file selected") );
    //d->infoView->setSorting( -1 );
    //d->infoView->setAlternateBackground( QColor() );
    //d->infoView->setFullWidth(true);
    //d->infoView->setSelectionMode( Q3ListView::NoSelection );
    //d->infoView->setHScrollBarMode( Q3ScrollView::AlwaysOff );

    connect( d->infoView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotContextMenuRequested(QPoint)) );

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    resize(430, 485);

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
    icon->setFixedSize(20,20);
    icon->setStyleSheet("QLabel{border-image: url(:/icon/icon/logo-小.png);"
                        "background-repeat: no-repeat;background-color:transparent;}");
    QLabel *title = new QLabel(i18n("kylin-burner"));
    title->setFixedSize(80,20);
    title->setStyleSheet("QLabel{background-color:transparent;"
                         "background-repeat: no-repeat;color:#444444;"
                         "font: 14px;}");
    QPushButton *close = new QPushButton( this );
    close->setFixedSize(20,20);
    close->setStyleSheet("QPushButton{border-image: url(:/icon/icon/icon-关闭-默认.png);"
                         "border:none;background-color:rgb(233, 233, 233);"
                         "border-radius: 4px;background-color:transparent;}"
                          "QPushButton:hover{border-image: url(:/icon/icon/icon-关闭-悬停点击.png);"
                         "border:none;background-color:rgb(248, 100, 87);"
                         "border-radius: 4px;}");
    connect(close, SIGNAL( clicked() ), this , SLOT( close() ) );

    QLabel* label_top = new QLabel( this );
    label_top->setFixedHeight(20);
    QHBoxLayout *titlebar = new QHBoxLayout( label_top );
    titlebar->setContentsMargins(11, 0, 0, 0);
    titlebar->addWidget(icon);
    titlebar->addSpacing(5);
    titlebar->addWidget(title);
    titlebar->addStretch();
    titlebar->addWidget(close);
    titlebar->addSpacing(5);

    QLabel* label_title = new QLabel( i18n("burn setting"),this );
    label_title->setFixedHeight(24);
    QFont label_font;
    label_font.setPixelSize(24);
    label_title->setFont( label_font );

    d->writerSelectionWidget = new K3b::WriterSelectionWidget( frame );
    d->writerSelectionWidget->hideComboMedium();
    d->writerSelectionWidget->setWindowFlags(Qt::FramelessWindowHint);
    d->writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE );
    d->writerSelectionWidget->setWantedMediumState( K3b::Device::STATE_EMPTY );

    // options
    // -----------------------------------------------------------------------
    //d->optionTabbed = new QTabWidget( frame );
    d->optionTabbed = new QTabWidget();
    /* hide frame*/
    d->optionTabbed->setWindowFlags(Qt::FramelessWindowHint);

    QWidget* optionTab = new QWidget( d->optionTabbed );
    /* hide frame*/
    optionTab->setWindowFlags(Qt::FramelessWindowHint);
    
    QGridLayout* optionTabLayout = new QGridLayout( optionTab );
    optionTabLayout->setAlignment( Qt::AlignTop );

    //QGroupBox* writingModeGroup = new QGroupBox( i18n("Writing Mode"), optionTab );
    QGroupBox* writingModeGroup = new QGroupBox( i18n("Writing Mode") );
    d->writingModeWidget = new K3b::WritingModeWidget( writingModeGroup );
    QHBoxLayout* writingModeGroupLayout = new QHBoxLayout( writingModeGroup );
    writingModeGroupLayout->addWidget( d->writingModeWidget );

    // copies --------
    //QGroupBox* groupCopies = new QGroupBox( i18n("Copies"), optionTab );
    QGroupBox* groupCopies = new QGroupBox( i18n("Copies") );
    QLabel* pixLabel = new QLabel( groupCopies );
    pixLabel->setPixmap( SmallIcon( "tools-media-optical-copy", KIconLoader::SizeMedium ) );
    pixLabel->setScaledContents( false );
    d->spinCopies = new QSpinBox( groupCopies );
    d->spinCopies->setMinimum( 1 );
    d->spinCopies->setMaximum( 999 );
    QHBoxLayout* groupCopiesLayout = new QHBoxLayout( groupCopies );
    groupCopiesLayout->addWidget( pixLabel );
    groupCopiesLayout->addWidget( d->spinCopies );
    // -------- copies

    //QGroupBox* optionGroup = new QGroupBox( i18n("Settings"), optionTab );
    //QGroupBox* optionGroup = new QGroupBox();
    d->checkDummy = K3b::StdGuiItems::simulateCheckbox();
    d->checkDummy->setFixedHeight(16);
    label_font.setPixelSize(14);
    d->checkDummy->setFont( label_font );
    d->checkDummy->setStyleSheet("color:#444444;");

    d->checkCacheImage = K3b::StdGuiItems::createCacheImageCheckbox();
    d->checkCacheImage->setFixedHeight(16);
    d->checkCacheImage->setFont( label_font );
    d->checkCacheImage->setStyleSheet("color:#444444;");
    
    d->checkVerify = K3b::StdGuiItems::verifyCheckBox();
    d->checkVerify->setFixedHeight(16);
    d->checkVerify->setFont( label_font );
    d->checkVerify->setStyleSheet("color:#444444;");

//tmp
    d->tempDirSelectionWidget = new K3b::TempDirSelectionWidget( );
    QLabel *m_labeltmpPath = new QLabel( optionTab );
    QLineEdit *m_tmpPath = new QLineEdit( optionTab );
    QString tmp_path =i18n("temp file path: ");
    KIO::filesize_t tempFreeSpace = d->tempDirSelectionWidget->freeTempSpace();
    QString tmp_size = d->tempDirSelectionWidget->tempPath() + "     "  +  KIO::convertSize(tempFreeSpace);
    m_labeltmpPath->setText( tmp_path );
    m_labeltmpPath->setFixedSize( 80, 30);
    m_labeltmpPath->setFont( label_font );
    m_labeltmpPath->setStyleSheet("color:#444444;");

    m_tmpPath->setText( tmp_size);
    m_tmpPath->setFixedSize( 368, 30);
    m_tmpPath->setFont( label_font );
    m_tmpPath->setStyleSheet("color:#444444;");
    m_tmpPath->setEnabled(false);
  //***************

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setContentsMargins(31, 0, 0, 0);
    vlayout->addWidget( label_title );
    vlayout->addSpacing( 25 );
    vlayout->addWidget( d->writerSelectionWidget );
    vlayout->addSpacing( 25 );
    vlayout->addWidget( d->checkDummy );
    vlayout->addSpacing( 11 );
    vlayout->addWidget( d->checkCacheImage );
    vlayout->addSpacing( 11 );
    vlayout->addWidget( d->checkVerify );
    vlayout->addSpacing( 25 );
    vlayout->addWidget( m_labeltmpPath );
    vlayout->addSpacing( 10 );
    vlayout->addWidget( m_tmpPath );
    vlayout->addStretch( 0 );
 

#if 0
    QVBoxLayout* optionGroupLayout = new QVBoxLayout( optionGroup );
    optionGroupLayout->addWidget( d->checkDummy );
    optionGroupLayout->addWidget( d->checkCacheImage );
    optionGroupLayout->addWidget( d->checkVerify );
    //tmp
    optionGroupLayout->addWidget( m_labeltmpPath );
    optionGroupLayout->addWidget( m_tmpPath );

    optionGroupLayout->addStretch( 1 );
#endif
/*
    optionTabLayout->addWidget( writingModeGroup, 0, 0 );
    optionTabLayout->addWidget( groupCopies, 1, 0 );
    optionTabLayout->addWidget( optionGroup, 0, 1, 2, 1 );
    optionTabLayout->setRowStretch( 1, 1 );
    optionTabLayout->setColumnStretch( 1, 1 );
*/
    //optionTabLayout->addWidget( optionGroup , 0, 0);
    d->optionTabbed->addTab( optionTab, i18n("Settings") );
    //hide tabBar
    d->optionTabbed->tabBar()->hide();
/*
    // image tab ------------------------------------
    d->tempPathTab = new QWidget( d->optionTabbed );
    QGridLayout* imageTabGrid = new QGridLayout( d->tempPathTab );

    d->tempDirSelectionWidget = new K3b::TempDirSelectionWidget( d->tempPathTab );

    imageTabGrid->addWidget( d->tempDirSelectionWidget, 0, 0 );

    d->tempPathTabIndex = d->optionTabbed->addTab( d->tempPathTab, i18n("&Image") );
    d->tempPathTabVisible = true;
    // -------------------------------------------------------------
*/

    // advanced ---------------------------------
    d->advancedTab = new QWidget( d->optionTabbed );
    QGridLayout* advancedTabLayout = new QGridLayout( d->advancedTab );
    advancedTabLayout->setAlignment( Qt::AlignTop );

    d->dataModeWidget = new K3b::DataModeWidget( d->advancedTab );
    d->checkNoFix = K3b::StdGuiItems::startMultisessionCheckBox( d->advancedTab );

    advancedTabLayout->addWidget( new QLabel( i18n("Data mode:"), d->advancedTab ), 0, 0 );
    advancedTabLayout->addWidget( d->dataModeWidget, 0, 1 );
    advancedTabLayout->addWidget( d->checkNoFix, 1, 0, 1, 3 );
    advancedTabLayout->setRowStretch( 2, 1 );
    advancedTabLayout->setColumnStretch( 2, 1 );

    d->advancedTabIndex = d->optionTabbed->addTab( d->advancedTab, i18n("Advanced") );
    d->advancedTabVisible = true;
    // -----------------------------------------------------------------------




    //QGridLayout* grid = new QGridLayout( frame );
    QVBoxLayout* grid = new QVBoxLayout( frame );
    grid->setContentsMargins( 0, 0, 0, 0 );
    grid->addWidget( label_top );
    grid->addSpacing( 30 );
    grid->addLayout( vlayout );


    //grid->addWidget( groupImageUrl, 0, 0 );
    //grid->addWidget( groupImageType, 0, 1 );
    //grid->setColumnStretch( 0, 1 );
    //grid->addWidget( d->infoView, 1, 0, 1, 2 );
    //grid->addWidget( d->writerSelectionWidget, 2, 0, 1, 2 );
    //grid->addWidget( d->optionTabbed, 3, 0, 1, 2 );
    //grid->addWidget( d->writerSelectionWidget, 0, 0, 1, 2 );
    //grid->addWidget( d->optionTabbed, 1, 0, 1, 2 );
    //grid->setRowStretch( 1, 1 );


    d->comboImageType->setWhatsThis( i18n("<p><b>Image types supported by K3b:</p>"
                                         "<p><b>Plain image</b><br/>"
                                         "Plain images are written as is to the medium using "
                                         "a single data track. Typical plain images are iso "
                                         "images as created by K3b's data project."
                                         "<p><b>Cue/bin images</b><br/>"
                                         "Cue/bin images consist of a cue file describing the "
                                         "table of contents of the medium and an image file "
                                         "which contains the actual data. The data will be "
                                         "written to the medium according to the cue file."
                                         "<p><b>Audio Cue image</b><br/>"
                                         "Audio cue images are a special kind of cue/bin image "
                                         "containing an image of an audio CD. The actual audio "
                                         "data can be encoded using any audio format supported "
                                         "by K3b. Audio cue files can also be imported into "
                                         "K3b audio projects which allows one to change the order "
                                         "and add or remove tracks."
                                         "<p><b>Cdrecord clone images</b><br/>"
                                         "K3b creates a cdrecord clone image of a single-session "
                                         "CD when copying a CD in clone mode. These images can "
                                         "be reused here."
                                         "<p><b>Cdrdao TOC files</b><br/>"
                                         "K3b supports writing cdrdao's own image format, the toc "
                                         "files.") );
}


void K3b::ImageWritingDialog::close()
{
    K3b::InteractionDialog::slotCancelClicked();
}

void K3b::ImageWritingDialog::slotCancelClicked()
{
    saveConfig();
    this->close();
}

void K3b::ImageWritingDialog::loadConfig()
{
    KConfigGroup grp( KSharedConfig::openConfig(), "image writing" );
    loadSettings( grp );
}

void K3b::ImageWritingDialog::saveConfig()
{
    KConfigGroup grp( KSharedConfig::openConfig(), "image writing" );
    saveSettings( grp );
}

void K3b::ImageWritingDialog::setComboMedium( K3b::Device::Device* dev )
{
    d->writerSelectionWidget->setWantedMedium( dev );
}

void K3b::ImageWritingDialog::slotStartClicked()
{
    KConfigGroup g( KSharedConfig::openConfig(), "image writing" );
    loadSettings( g );
    
    d->md5Job->cancel();

    // save the path
    KConfigGroup grp( KSharedConfig::openConfig(), configGroup() );
    grp.writePathEntry( "last written image", d->imagePath() );

    if( d->imageFile.isEmpty() )
        d->imageFile = d->imagePath();
    if( d->tocFile.isEmpty() )
        d->tocFile = d->imagePath();

    // create a progresswidget
    K3b::BurnProgressDialog dlg( parentWidget() );

    // create the job
    K3b::BurnJob* job = 0;
    switch( d->currentImageType() ) {
    case IMAGE_CDRECORD_CLONE:
    {
        K3b::CloneJob* _job = new K3b::CloneJob( &dlg, this );
        _job->setWriterDevice( d->writerSelectionWidget->writerDevice() );
        _job->setImagePath( d->imageFile );
        _job->setSimulate( d->checkDummy->isChecked() );
        _job->setWriteSpeed( d->writerSelectionWidget->writerSpeed() );
        _job->setCopies( d->checkDummy->isChecked() ? 1 : d->spinCopies->value() );
        _job->setOnlyBurnExistingImage( true );

        job = _job;
    }
    break;

    case IMAGE_AUDIO_CUE:
    {
        K3b::AudioCueFileWritingJob* job_ = new K3b::AudioCueFileWritingJob( &dlg, this );

        job_->setBurnDevice( d->writerSelectionWidget->writerDevice() );
        job_->setSpeed( d->writerSelectionWidget->writerSpeed() );
        job_->setSimulate( d->checkDummy->isChecked() );
        job_->setWritingMode( d->writingModeWidget->writingMode() );
        job_->setCueFile( d->tocFile );
        job_->setCopies( d->checkDummy->isChecked() ? 1 : d->spinCopies->value() );
        job_->setOnTheFly( !d->checkCacheImage->isChecked() );
        job_->setTempDir( d->tempDirSelectionWidget->tempPath() );

        job = job_;
    }
    break;

    case IMAGE_CUE_BIN:
        // for now the K3b::BinImageWritingJob decides if it's a toc or a cue file
    case IMAGE_CDRDAO_TOC:
    {
        K3b::BinImageWritingJob* job_ = new K3b::BinImageWritingJob( &dlg, this );

        job_->setWriter( d->writerSelectionWidget->writerDevice() );
        job_->setSpeed( d->writerSelectionWidget->writerSpeed() );
        job_->setTocFile( d->tocFile );
        job_->setSimulate(d->checkDummy->isChecked());
        job_->setMulti( false /*d->checkNoFix->isChecked()*/ );
        job_->setCopies( d->checkDummy->isChecked() ? 1 : d->spinCopies->value() );

        job = job_;
    }
    break;

    case IMAGE_RAW:
    case IMAGE_ISO:
    {
        if (d->currentImageType() == IMAGE_ISO) {
            K3b::Iso9660 isoFs(d->imageFile);
            if (isoFs.open()) {
                if (K3b::filesize(QUrl::fromLocalFile(d->imageFile)) < Private::volumeSpaceSize(isoFs)) {
                    if (KMessageBox::questionYesNo(this,
                                                   i18n("<p>The actual file size does not match the size declared in the file header. "
                                                     "If it has been downloaded make sure the download is complete.</p>"
                                                     "<p>Only continue if you know what you are doing.</p>"),
                                                   i18n("Warning"),
                                                   KStandardGuiItem::cont(),
                                                   KStandardGuiItem::cancel() ) == KMessageBox::No )
                        return;
                }
            }
        }

        // TODO: it needs a NON ISO 9660 images testcase
        K3b::Iso9660ImageWritingJob* job_ = new K3b::Iso9660ImageWritingJob( &dlg );

        job_->setBurnDevice( d->writerSelectionWidget->writerDevice() );
        job_->setSpeed( d->writerSelectionWidget->writerSpeed() );
        job_->setSimulate( d->checkDummy->isChecked() );
        job_->setWritingMode( d->writingModeWidget->writingMode() );
        job_->setVerifyData( d->checkVerify->isChecked() );
        job_->setNoFix( d->checkNoFix->isChecked() );
        job_->setDataMode( d->dataModeWidget->dataMode() );
        job_->setImagePath( d->imageFile );
        job_->setCopies( d->checkDummy->isChecked() ? 1 : d->spinCopies->value() );

        job = job_;
    }
    break;

    default:
        qDebug() << "(K3b::ImageWritingDialog) this should really not happen!";
        break;
    }

    if( job ) {
        job->setWritingApp( d->writerSelectionWidget->writingApp() );
        
        hide();
        
        connect( job, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );

        dlg.startJob(job);

        delete job;

        BurnResult* dialog = new BurnResult( d->flag, "image");
        dialog->show();

        if( KConfigGroup( KSharedConfig::openConfig(), "General Options" ).readEntry( "keep action dialogs open", false ) )
            show();
        else
            close();
    }
}


void K3b::ImageWritingDialog::slotFinished( bool ret )
{
    d->flag = ret;
}

void K3b::ImageWritingDialog::slotUpdateImage( const QString& )
{
    QString path = d->imagePath();

    // check the image types

    d->haveMd5Sum = false;
    d->md5Job->cancel();
    d->infoView->clear();
    //d->infoView->header()->resizeSection( 0, 20 );
    d->md5SumItem = 0;
    d->foundImageType = IMAGE_UNKNOWN;
    d->tocFile.truncate(0);
    d->imageFile.truncate(0);

    QFileInfo info( path );
    if( info.isFile() ) {

        // ------------------------------------------------
        // Test for iso9660 image
        // ------------------------------------------------
        K3b::Iso9660 isoF( path );
        if( isoF.open() ) {
#ifdef K3B_DEBUG
            isoF.debug();
#endif

            d->createIso9660InfoItems( &isoF );
            isoF.close();
            calculateMd5Sum( path );

            d->foundImageType = IMAGE_ISO;
            d->imageFile = path;
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {

            // check for cdrecord clone image
            // try both path and path.toc as tocfiles
            K3b::CloneTocReader cr;

            if( path.right(4) == ".toc" ) {
                cr.openFile( path );
                if( cr.isValid() ) {
                    d->tocFile = path;
                    d->imageFile = cr.imageFilename();
                }
            }
            if( d->imageFile.isEmpty() ) {
                cr.openFile( path + ".toc" );
                if( cr.isValid() ) {
                    d->tocFile = cr.filename();
                    d->imageFile = cr.imageFilename();
                }
            }

            if( !d->imageFile.isEmpty() ) {
                // we have a cdrecord clone image
                d->createCdrecordCloneItems( d->tocFile, d->imageFile );
                calculateMd5Sum( d->imageFile );

                d->foundImageType = IMAGE_CDRECORD_CLONE;
            }
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {

            // check for cue/bin stuff
            // once again we try both path and path.cue
            K3b::CueFileParser cp;

            if( path.right(4).toLower() == ".cue" )
                cp.openFile( path );
            else if( path.right(4).toLower() == ".bin" )
                cp.openFile( path.left( path.length()-3) + "cue" );

            if( cp.isValid() ) {
                d->tocFile = cp.filename();
                d->imageFile = cp.imageFilename();
            }

            if( d->imageFile.isEmpty() ) {
                cp.openFile( path + ".cue" );
                if( cp.isValid() ) {
                    d->tocFile = cp.filename();
                    d->imageFile = cp.imageFilename();
                }
            }

            if( !d->imageFile.isEmpty() ) {
                // we have a cue file
                if( cp.toc().contentType() == K3b::Device::AUDIO ) {
                    d->foundImageType = IMAGE_AUDIO_CUE;
                    d->createAudioCueItems( cp );
                }
                else {
                    d->foundImageType = IMAGE_CUE_BIN;  // we cannot be sure if writing will work... :(
                    d->createCueBinItems( d->tocFile, d->imageFile );
                    calculateMd5Sum( d->imageFile );
                }
            }
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {
            // TODO: check for cdrdao tocfile
        }

        // TODO: treat unusable image to opaque
        if (d->foundImageType == IMAGE_UNKNOWN) {
            if (KMessageBox::questionYesNo(this,
                                           i18n("Type of image file is not recognizable. Do you want to burn it anyway?"),
                                           i18n("Unknown image type")) == KMessageBox::Yes) {
                d->foundImageType = IMAGE_RAW;
                d->imageFile = path;
            }
        }

        if( d->foundImageType == IMAGE_UNKNOWN ) {
            QTreeWidgetItem* item = new QTreeWidgetItem( d->infoView );
            if ( !info.isReadable() ) {
                item->setText( 0, i18n("Unable to read image file") );
            } else {
                item->setText( 0, i18n("Seems not to be a usable image") );
            }
            item->setForeground( 0, d->negativeTextColor );
            item->setIcon( 0, QIcon::fromTheme( "dialog-error") );
        }
        else {
            // remember as recent image
            int i = 0;
            while ( i < d->comboRecentImages->count() && d->comboRecentImages->itemText(i) != path )
                ++i;
            if ( i == d->comboRecentImages->count() )
                d->comboRecentImages->insertItem( 0, path );
        }
    }
    else {
        QTreeWidgetItem* item = new QTreeWidgetItem( d->infoView );
        item->setText( 0, i18n("File not found") );
        item->setForeground( 0, d->negativeTextColor );
        item->setIcon( 0, QIcon::fromTheme( "dialog-error") );
    }

    slotToggleAll();
}


void K3b::ImageWritingDialog::toggleAll()
{

    K3b::Medium medium = k3bappcore->mediaCache()->medium( d->writerSelectionWidget->writerDevice() );

    // set usable writing apps
    // for now we only restrict ourselves in case of CD images
    switch( d->currentImageType() ) {
    case IMAGE_CDRDAO_TOC:
        d->writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppCdrdao );
        break;
    case IMAGE_CDRECORD_CLONE:
        d->writerSelectionWidget->setSupportedWritingApps( K3b::WritingAppCdrecord );
        break;
    default: {
        K3b::WritingApps apps = K3b::WritingAppCdrecord;
        if (d->currentImageType() == IMAGE_ISO || d->currentImageType() == IMAGE_RAW) {
            // DVD/BD is always ISO here
            apps |= K3b::WritingAppGrowisofs;
        }
        if ( K3b::Device::isCdMedia( medium.diskInfo().mediaType() ) )
            apps |= K3b::WritingAppCdrdao;

        d->writerSelectionWidget->setSupportedWritingApps( apps );
        break;
    }
    }

    // set a wanted media type
    // some image types can be put on any medium, some only on CD
    if (d->currentImageType() == IMAGE_ISO ||
        d->currentImageType() == IMAGE_RAW ||
        d->currentImageType() == IMAGE_UNKNOWN) {
        d->writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE );
    }
    else {
        d->writerSelectionWidget->setWantedMediumType( K3b::Device::MEDIA_WRITABLE_CD );
    }

    // set wanted image size
    if (d->currentImageType() == IMAGE_ISO || d->currentImageType() == IMAGE_RAW)
        d->writerSelectionWidget->setWantedMediumSize( K3b::filesize( QUrl::fromLocalFile(d->imagePath()) )/2048 );
    else
        d->writerSelectionWidget->setWantedMediumSize( Msf() );

    // cdrecord clone and cue both need DAO
    if( d->writerSelectionWidget->writingApp() != K3b::WritingAppCdrdao
        && ( d->currentImageType() == IMAGE_ISO ||
             d->currentImageType() == IMAGE_AUDIO_CUE ) )
        d->writingModeWidget->determineSupportedModesFromMedium( medium );
    else
        d->writingModeWidget->setSupportedModes( K3b::WritingModeSao );

    // enable the Write-Button if we found a valid image or the user forced an image type
    setButtonEnabled( START_BUTTON, d->writerSelectionWidget->writerDevice()
                      && d->currentImageType() != IMAGE_UNKNOWN
                      && QFile::exists( d->imagePath() ) );

    // some stuff is only available for iso and opaque images
    if (d->currentImageType() == IMAGE_ISO || d->currentImageType() == IMAGE_RAW) {
        d->checkVerify->show();
        if( !d->advancedTabVisible ) {
            d->advancedTabIndex = d->optionTabbed->addTab( d->advancedTab, i18n("Advanced") );
        }
        d->advancedTabVisible = true;
        if( d->checkDummy->isChecked() ) {
            d->checkVerify->setEnabled( false );
            d->checkVerify->setChecked( false );
        }
        else
            d->checkVerify->setEnabled( true );
    }
    else {
        if( d->advancedTabVisible ) {
            d->optionTabbed->removeTab( d->advancedTabIndex );
        }
        d->advancedTabVisible = false;
        d->checkVerify->hide();
    }


    // and some other stuff only makes sense for audio cues
    if( d->currentImageType() == IMAGE_AUDIO_CUE ) {
        if( !d->tempPathTabVisible )
            d->tempPathTabIndex = d->optionTabbed->addTab( d->tempPathTab, i18n("&Image") );
        d->tempPathTabVisible = true;
        d->tempDirSelectionWidget->setDisabled( !d->checkCacheImage->isChecked() );
    }
    else {
        if( d->tempPathTabVisible ) {
            d->optionTabbed->removeTab( d->tempPathTabIndex );
        }
        d->tempPathTabVisible = false;
    }
    d->checkCacheImage->setVisible( d->currentImageType() == IMAGE_AUDIO_CUE );
    //d->checkCacheImage->setVisible( true );

    d->spinCopies->setEnabled( !d->checkDummy->isChecked() );


    if( QTreeWidgetItem* item = d->infoView->topLevelItem( 0 ) ) {
        item->setForeground( 1,
                             d->currentImageType() != d->foundImageType
                             ? d->negativeTextColor : d->normalTextColor );
    }
}


void K3b::ImageWritingDialog::setImage( const QUrl& url )
{
    d->imageForced = true;
    d->editImagePath->setUrl( url );
}


void K3b::ImageWritingDialog::calculateMd5Sum( const QString& file )
{
    d->haveMd5Sum = false;

    if( !d->md5SumItem ) {
        d->md5SumItem = new QTreeWidgetItem( d->infoView );
    }

    d->md5SumItem->setText( 0, i18n("MD5 Sum:") );
    d->md5SumItem->setForeground( 0, d->infoTextColor );
    d->md5SumItem->setTextAlignment( 0, Qt::AlignRight );

    if( file != d->lastCheckedFile ) {

        QProgressBar* progress = new QProgressBar( d->infoView );
        progress->setMaximumHeight( fontMetrics().height() );
        progress->setRange( 0, 100 );
        progress->setValue( 0 );
        d->infoView->setItemWidget( d->md5SumItem, 1, progress );
        d->lastCheckedFile = file;
        d->md5Job->setFile( file );
        d->md5Job->start();
    }
    else
        slotMd5JobFinished( true );
}


void K3b::ImageWritingDialog::slotMd5JobPercent( int p )
{
    QWidget* widget = d->infoView->itemWidget( d->md5SumItem, 1 );
    if( QProgressBar* progress = qobject_cast<QProgressBar*>( widget ) )
    {
        progress->setValue( p );
    }
}


void K3b::ImageWritingDialog::slotMd5JobFinished( bool success )
{
    if( success ) {
        d->md5SumItem->setText( 1, d->md5Job->hexDigest() );
        d->md5SumItem->setIcon( 1, QIcon::fromTheme("dialog-information") );
        d->haveMd5Sum = true;
    }
    else {
        d->md5SumItem->setForeground( 1, d->negativeTextColor );
        if( d->md5Job->hasBeenCanceled() )
            d->md5SumItem->setText( 1, i18n("Calculation canceled") );
        else
            d->md5SumItem->setText( 1, i18n("Calculation failed") );
        d->md5SumItem->setIcon( 1, QIcon::fromTheme("dialog-error") );
        d->lastCheckedFile.truncate(0);
    }

    // Hide progress bar
    d->infoView->setItemWidget( d->md5SumItem, 1, 0 );
}


void K3b::ImageWritingDialog::slotContextMenuRequested( const QPoint& pos )
{
    if( !d->haveMd5Sum )
        return;

    QMenu popup;
    QAction *copyItem = popup.addAction( i18n("Copy checksum to clipboard") );
    QAction *compareItem = popup.addAction( i18n("Compare checksum...") );

    QAction *act = popup.exec( d->infoView->mapToGlobal( pos ) );

    if( act == compareItem ) {
        bool ok;
        QString md5sumToCompare = QInputDialog::getText( this,
                                                         i18n("MD5 Sum Check"),
                                                         i18n("Please insert the MD5 Sum to compare:"),
                                                         QLineEdit::Normal,
                                                         QString(),
                                                         &ok );
        if( ok ) {
            if( md5sumToCompare.toLower().toUtf8() == d->md5Job->hexDigest().toLower() )
                KMessageBox::information( this, i18n("The MD5 Sum of %1 equals that specified.",d->imagePath()),
                                          i18n("MD5 Sums Equal") );
            else
                KMessageBox::sorry( this, i18n("The MD5 Sum of %1 differs from that specified.",d->imagePath()),
                                    i18n("MD5 Sums Differ") );
        }
    }
    else if( act == copyItem ) {
        QApplication::clipboard()->setText( d->md5Job->hexDigest().toLower(), QClipboard::Clipboard );
    }
}


void K3b::ImageWritingDialog::loadSettings( const KConfigGroup& c )
{
    d->writingModeWidget->loadConfig( c );
    d->checkDummy->setChecked( c.readEntry("simulate", false ) );
    d->checkNoFix->setChecked( c.readEntry("multisession", false ) );
    d->checkCacheImage->setChecked( !c.readEntry("on_the_fly", true ) );

    d->dataModeWidget->loadConfig(c);

    d->spinCopies->setValue( c.readEntry( "copies", 1 ) );

    d->checkVerify->setChecked( c.readEntry( "verify_data", false ) );

    d->writerSelectionWidget->loadConfig( c );

    if( !d->imageForced ) {
        QString image = c.readPathEntry( "image path", c.readPathEntry( "last written image", QString() ) );
        if( QFile::exists( image ) )
            d->editImagePath->setUrl( QUrl::fromLocalFile( image ) );
    }

    QString imageType = c.readEntry( "image type", "auto" );
    int x = 0;
    if( imageType == "iso9660" )
        x = d->imageTypeSelectionMapRev[IMAGE_ISO];
    else if( imageType == "cue-bin" )
        x = d->imageTypeSelectionMapRev[IMAGE_CUE_BIN];
    else if( imageType == "audio-cue" )
        x = d->imageTypeSelectionMapRev[IMAGE_AUDIO_CUE];
    else if( imageType == "cdrecord-clone" )
        x = d->imageTypeSelectionMapRev[IMAGE_CDRECORD_CLONE];
    else if( imageType == "cdrdao-toc" )
        x = d->imageTypeSelectionMapRev[IMAGE_CDRDAO_TOC];
    else if (imageType == "opaque")
        x = d->imageTypeSelectionMapRev[IMAGE_RAW];

    d->comboImageType->setCurrentIndex( x );

    d->tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );

    slotToggleAll();
}


void K3b::ImageWritingDialog::saveSettings( KConfigGroup c )
{
    d->writingModeWidget->saveConfig( c ),
    c.writeEntry( "simulate", d->checkDummy->isChecked() );
    c.writeEntry( "multisession", d->checkNoFix->isChecked() );
    c.writeEntry( "on_the_fly", !d->checkCacheImage->isChecked() );
    d->dataModeWidget->saveConfig(c);

    c.writeEntry( "verify_data", d->checkVerify->isChecked() );

    d->writerSelectionWidget->saveConfig( c );

    c.writePathEntry( "image path", d->imagePath() );

    c.writeEntry( "copies", d->spinCopies->value() );

    QString imageType;
    if( d->comboImageType->currentIndex() == 0 )
        imageType = "auto";
    else {
        switch( d->imageTypeSelectionMap[d->comboImageType->currentIndex()] ) {
        case IMAGE_ISO:
            imageType = "iso9660";
            break;
        case IMAGE_CUE_BIN:
            imageType = "cue-bin";
            break;
        case IMAGE_AUDIO_CUE:
            imageType = "audio-cue";
            break;
        case IMAGE_CDRECORD_CLONE:
            imageType = "cdrecord-clone";
            break;
        case IMAGE_CDRDAO_TOC:
            imageType = "cdrdao-toc";
            break;
        case IMAGE_RAW:
            imageType = "opaque";
            break;
        }
    }
    c.writeEntry( "image type", imageType );

    if( d->tempDirSelectionWidget->isEnabled() )
        d->tempDirSelectionWidget->saveConfig();
}


void K3b::ImageWritingDialog::dragEnterEvent( QDragEnterEvent* e )
{
    e->setAccepted( e->mimeData()->hasUrls() );
}


void K3b::ImageWritingDialog::dropEvent( QDropEvent* e )
{
    QList<QUrl> urls = e->mimeData()->urls();
    if( !urls.isEmpty() ) {
        d->editImagePath->setUrl( urls.first() );
    }
}


