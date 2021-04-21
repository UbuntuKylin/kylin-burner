/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2009-2011 Michal Malek <michalm@jabster.pl>
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

#include "k3bdataviewimpl.h"
#include "k3bbootimagedialog.h"
#include "k3bdatadoc.h"
#include "k3bdatamultisessionimportdialog.h"
#include "k3bdataprojectdelegate.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataprojectsortproxymodel.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdataurladdingdialog.h"
#include "k3bdiritem.h"
#include "k3bview.h"
#include "k3bviewcolumnadjuster.h"
#include "k3bvolumenamewidget.h"

#include <KLocalizedString>
#include <KFileItemDelegate>
#include <KRun>
#include <KActionCollection>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KConfig>

#include <QSortFilterProxyModel>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QShortcut>
#include <QWidgetAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "k3bfileselect.h"

namespace {

class VolumeNameWidgetAction : public QWidgetAction
{
public:
    VolumeNameWidgetAction( K3b::DataDoc* doc, QObject* parent )
    :
        QWidgetAction( parent ),
        m_doc( doc )
    {
    }

protected:
    QWidget* createWidget( QWidget* parent ) override
    {
        return new K3b::VolumeNameWidget( m_doc, parent );
    }

private:
    K3b::DataDoc* m_doc;
};

} // namespace


K3b::DataViewImpl::DataViewImpl( View* view, DataDoc* doc, KActionCollection* actionCollection )
:
    QObject( view ),
    m_view( view ),
    m_doc( doc ),
    m_model( new DataProjectModel( doc, view ) ),
    m_sortModel( new DataProjectSortProxyModel( this ) ),
    m_fileView( new QTreeView( view ) )
{
    connect( m_doc, SIGNAL(importedSessionChanged(int)), this, SLOT(slotImportedSessionChanged(int)) );
    connect( m_model, SIGNAL(addUrlsRequested(QList<QUrl>,K3b::DirItem*)), SLOT(slotAddUrlsRequested(QList<QUrl>,K3b::DirItem*)) );
    connect( m_model, SIGNAL(moveItemsRequested(QList<K3b::DataItem*>,K3b::DirItem*)), SLOT(slotMoveItemsRequested(QList<K3b::DataItem*>,K3b::DirItem*)) );

    m_sortModel->setSourceModel( m_model );

    //m_fileView->setItemDelegate( new DataProjectDelegate( this ) );

    m_fileView->setModel( m_sortModel );
    m_fileView->setAcceptDrops( true );
    m_fileView->setDragEnabled( true );
    m_fileView->setDragDropMode( QTreeView::DragDrop );
    m_fileView->setItemsExpandable( false );
    m_fileView->setRootIsDecorated( false );
    m_fileView->setSelectionMode( QTreeView::ExtendedSelection );
    m_fileView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_fileView->setContextMenuPolicy( Qt::ActionsContextMenu );
    m_fileView->setSortingEnabled( false );
    m_fileView->sortByColumn( DataProjectModel::FilenameColumn, Qt::AscendingOrder );
    m_fileView->setMouseTracking( false );
    m_fileView->setAllColumnsShowFocus( true );

    //*********************
    m_fileView->setIconSize( QSize(24,24) );
    m_fileView->setFixedHeight( 370 );
    //m_fileView->setMaximumHeight( 370 );
    //m_fileView->setMinimumHeight(28);
    m_fileView->setFrameStyle(QFrame::NoFrame);

    connect( m_fileView, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(slotItemActivated(QModelIndex)) );
 
    connect( m_fileView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotSelectionChanged()) );

    m_columnAdjuster = new ViewColumnAdjuster( this );
    m_columnAdjuster->setView( m_fileView );
    m_columnAdjuster->addFixedColumn( DataProjectModel::TypeColumn );
    m_columnAdjuster->setColumnMargin( DataProjectModel::TypeColumn, 10 );
    m_columnAdjuster->addFixedColumn( DataProjectModel::SizeColumn );
    m_columnAdjuster->setColumnMargin( DataProjectModel::SizeColumn, 10 );
    //*********************************************************************
    m_columnAdjuster->addFixedColumn( DataProjectModel::PathColumn );
    m_columnAdjuster->setColumnMargin( DataProjectModel::PathColumn, 10 );

    m_actionNewDir = new QAction( QIcon::fromTheme( "folder-new" ), i18n("New Folder..."), m_fileView );
    m_actionNewDir->setShortcut( Qt::CTRL + Qt::Key_N );
    m_actionNewDir->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "new_dir", m_actionNewDir );
    connect( m_actionNewDir, SIGNAL(triggered(bool)), this, SLOT(slotNewDir()) );
    //************************************************************************************************ 
    m_actionOpenDir = new QAction( QIcon::fromTheme( "folder-new" ), i18n("Open"), m_fileView );
    //m_actionRemove->setShortcut( Qt::Key_Delete );
    //m_actionRemove->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "open_dir", m_actionOpenDir );
    connect( m_actionOpenDir, SIGNAL(triggered(bool)), this, SLOT(slotOpenDir()) );
    
    m_actionClear = new QAction( QIcon::fromTheme( "edit-delete" ), i18n("Clear"), m_fileView );
    //m_actionRemove->setShortcut( Qt::Key_Delete );
    //m_actionRemove->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "clear", m_actionClear );
    connect( m_actionClear, SIGNAL(triggered(bool)), view, SLOT(slotClearClicked()) );
    

    m_actionRemove = new QAction( QIcon::fromTheme( "edit-delete" ), i18n("Remove"), m_fileView );
    m_actionRemove->setShortcut( Qt::Key_Delete );
    //m_actionRemove->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "remove", m_actionRemove );
    connect( m_actionRemove, SIGNAL(triggered(bool)), view, SLOT(slotRemoveClicked()) );

    m_actionRename = new QAction( QIcon::fromTheme( "edit-rename" ), i18n("Rename"), m_fileView );
    m_actionRename->setShortcut( Qt::Key_F2 );
    //m_actionRename->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "rename", m_actionRename );
    connect( m_actionRename, SIGNAL(triggered(bool)), this, SLOT(slotRename()) );

    m_actionParentDir = new QAction( QIcon::fromTheme( "go-up" ), i18n("Parent Folder"), m_fileView );
    //m_actionParentDir->setShortcut( Qt::Key_Backspace );
    m_actionParentDir->setShortcut( Qt::ControlModifier + Qt::Key_Up );
    //m_actionParentDir->setShortcutContext( Qt::ApplicationShortcut );
    actionCollection->addAction( "parent_dir", m_actionParentDir );
    connect( m_actionParentDir, SIGNAL(triggered(bool)), this, SLOT(slotParent()) );

    m_actionProperties = new QAction( QIcon::fromTheme( "document-properties" ), i18n("Properties"), m_fileView );
    m_actionProperties->setShortcut( Qt::ALT + Qt::Key_Return );
    //m_actionProperties->setShortcutContext( Qt::WidgetShortcut );
    actionCollection->addAction( "properties", m_actionProperties );
    connect( m_actionProperties, SIGNAL(triggered(bool)), this, SLOT(slotProperties()) );

    m_actionOpen = new QAction( QIcon::fromTheme( "document-open" ), i18n("Open"), m_view );
    actionCollection->addAction( "open", m_actionOpen );
    connect( m_actionOpen, SIGNAL(triggered(bool)), this, SLOT(slotOpen()) );

    m_actionImportSession = new QAction( QIcon::fromTheme( "document-import" ), i18n("&Import Session..."), m_view );
    m_actionImportSession->setToolTip( i18n("Import a previously burned session into the current project") );
    actionCollection->addAction( "project_data_import_session", m_actionImportSession );
    connect( m_actionImportSession, SIGNAL(triggered(bool)), this, SLOT(slotImportSession()) );

    m_actionClearSession = new QAction( QIcon::fromTheme( "edit-clear" ), i18n("&Clear Imported Session"), m_view );
    m_actionClearSession->setToolTip( i18n("Remove the imported items from a previous session") );
    m_actionClearSession->setEnabled( m_doc->importedSession() > -1 );
    actionCollection->addAction( "project_data_clear_imported_session", m_actionClearSession );
    connect( m_actionClearSession, SIGNAL(triggered(bool)), this, SLOT(slotClearImportedSession()) );

    m_actionEditBootImages = new QAction( QIcon::fromTheme( "document-properties" ), i18n("&Edit Boot Images..."), m_view );
    m_actionEditBootImages->setToolTip( i18n("Modify the bootable settings of the current project") );
    actionCollection->addAction( "project_data_edit_boot_images", m_actionEditBootImages );
    connect( m_actionEditBootImages, SIGNAL(triggered(bool)), this, SLOT(slotEditBootImages()) );

    QWidgetAction* volumeNameWidgetAction = new VolumeNameWidgetAction( m_doc, this );
    actionCollection->addAction( "project_volume_name", volumeNameWidgetAction );

    QShortcut* enterShortcut = new QShortcut( QKeySequence( Qt::Key_Return ), m_fileView );
    enterShortcut->setContext( Qt::WidgetShortcut );
    connect( enterShortcut, SIGNAL(activated()), this, SLOT(slotEnterPressed()) );
    connect(this, SIGNAL(addFiles(QList<QUrl>)), m_view, SLOT(slotAddFile(QList<QUrl>)));

    // Create data context menu

    QAction* separator = new QAction( this );
    separator->setSeparator( true );
    m_fileView->addAction( m_actionParentDir );
    m_actionParentDir->setEnabled(false);
    m_fileView->addAction( separator );
    m_fileView->addAction( m_actionRename );
    m_fileView->addAction( m_actionRemove );
    m_fileView->addAction( m_actionNewDir );
    m_fileView->addAction( separator );
    m_fileView->addAction( m_actionOpen );
    //m_fileView->addAction( separator );
    //m_fileView->addAction( m_actionProperties );
    //m_fileView->addAction( separator );
    //m_fileView->addAction( actionCollection->action("project_burn") );

}


void K3b::DataViewImpl::addUrls( const QModelIndex& parent, const QList<QUrl>& urls )
{
    DirItem *item = dynamic_cast<DirItem*>( m_model->itemForIndex( parent ) );
    if (!item)
        item = m_doc->root();

    DataUrlAddingDialog::addUrls( urls, item, m_view );
}


void K3b::DataViewImpl::slotCurrentRootChanged( const QModelIndex& newRoot )
{
    // make the file view show only the child nodes of the currently selected
    // directory from dir view
    m_fileView->setRootIndex( m_sortModel->mapFromSource( newRoot ) );
    m_columnAdjuster->adjustColumns();
#if 0
    m_actionParentDir->setEnabled
            ( newRoot.isValid() && m_model->indexForItem( m_doc->root() ) != newRoot );
#endif
    if (m_fileView->rootIndex().parent().parent().isValid())
        m_actionParentDir->setEnabled(true);
    else
        m_actionParentDir->setEnabled(false);
}


void K3b::DataViewImpl::slotNewDir()
{
    const QModelIndex parent = m_sortModel->mapToSource( m_fileView->rootIndex() );
    DirItem* parentDir = 0;

    if (parent.isValid())
        parentDir = dynamic_cast<DirItem*>( m_model->itemForIndex( parent ) );

    if (!parentDir)
        parentDir = m_doc->root();

    QString name;
    bool ok;

    QInputDialog newDir(m_view);
    newDir.setWindowTitle(i18n("New Folder"));
    newDir.setWindowIcon(QIcon::fromTheme("brasero"));
    newDir.setLabelText(i18n("Please insert the name for the new folder:"));
    newDir.setTextValue(i18n("New Folder"));
    newDir.setFixedSize(300, 200);
    newDir.setWindowFlag(Qt::FramelessWindowHint);
    ok = newDir.exec();
    name = newDir.textValue();
    name = name.trimmed();

    qDebug() << newDir.findChildren<QWidget *>();

    /*
    name = QInputDialog::getText( m_view,
                                  i18n("New Folder"),
                                  i18n("Please insert the name for the new folder:"),
                                  QLineEdit::Normal,
                                  i18n("New Folder"),
                                  &ok );
    */

    while( ok && DataDoc::nameAlreadyInDir( name, parentDir ) ) {
        QCoreApplication::processEvents();
        /*
        name = QInputDialog::getText( m_view,
                                      i18n("New Folder"),
                                      i18n("A folder with that name already exists. Please enter a new name:"),
                                      QLineEdit::Normal,
                                      i18n("New Folder"),
                                      &ok );
                                      */
        if (name.isEmpty())
            newDir.setLabelText(i18n("Cannot add a folder with empty name. Please enter a valid name:"));
        else
            newDir.setLabelText(i18n("A folder with that name already exists. Please enter a new name:"));
        ok = newDir.exec();
        name = newDir.textValue();
        name = name.trimmed();
    }

    if( !ok )
        return;

    m_doc->addEmptyDir( name, parentDir );
}
//*******************************************************************************************
int K3b::DataViewImpl::slotOpenDir()
//void K3b::DataViewImpl::slotOpenDir()
{
    QStringList filepath;
    QUrl url;
    QList<QUrl> urls;
    myFileSelect *a =new myFileSelect( m_view );
    a->setOption(QFileDialog::DontUseNativeDialog, true);
    a->setViewMode(QFileDialog::List);
    a->setWindowTitle(i18n("Add"));

    bool addtoDir = false;
    K3b::DirItem *dir = NULL;

    QListView *listView = a->findChild<QListView*>("listView");
    if (listView)
        listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QTreeView *treeView = a->findChild<QTreeView*>();
    if (treeView)
        treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QDialogButtonBox *button = a->findChild<QDialogButtonBox *>("buttonBox");

    disconnect(button,SIGNAL(accepted()),a,SLOT(accept()));

    connect(button,SIGNAL(accepted()),a,SLOT(go()));

    if(a->exec()==QDialog::Accepted)
    {
        urls = a->selectedUrls();
    }
    if( urls.count() == 0 )
    {
        m_view->setCursor(Qt::ArrowCursor);
        return 0;
    }

    if (m_fileView->selectionModel()->selectedRows().size() != 1)
    {
        addtoDir = false;
        K3b::DataItem *d = m_model->itemForIndex(
                    m_sortModel->mapToSource(m_fileView->rootIndex()));
        if (d && d->isDir())
        {
            if (d->isDeleteable())
            {
                addtoDir = true;
                dir = d->getDirItem();
            }
        }
    }
    else
    {
        K3b::DataItem *d = m_model->itemForIndex(
                    m_sortModel->mapToSource(m_fileView->currentIndex()));
        if (d && d->isDir())
        {
            addtoDir = true;
            dir = d->getDirItem();
            qDebug() << "Add to dir name" << d->k3bName() << dir->k3bName();
        }
    }

    if (addtoDir) m_doc->addUrlsToDir(urls, dir);
    else m_doc->addUrls(urls);
    emit addFiles(urls);
    return 1;
}

void K3b::DataViewImpl::slotClear()
{
    //m_doc->clear();
    m_doc->clearDisk();
}

void K3b::DataViewImpl::whiteHeader()
{
#if 1
    m_fileView->header()->setStyleSheet("QHeaderView::section{"
                                        "border: 0px solid white;"
                                        "background-color : rgba(242, 242, 242, 1);}");
#endif
}

void K3b::DataViewImpl::blackHeader()
{
#if 1
    m_fileView->header()->setStyleSheet("QHeaderView::section{"
                                        "border: 0px solid white;"
                                        "background-color : #242424;}");
#endif
}

void K3b::DataViewImpl::slotRemove()
{
    int start = 0, count = 0;
    // Remove items directly from sort model to avoid unnecessary mapping of indexes
    const QItemSelection selection = m_fileView->selectionModel()->selection();
    const QModelIndex parentDirectory = m_fileView->rootIndex();
    //m_model->removeDatas(parentDirectory, m_fileView->selectionModel()->selectedRows());
    count = m_fileView->selectionModel()->selectedRows().size();
    QList<QModelIndex> idxs = m_fileView->selectionModel()->selectedRows();
    for (int i = 0; i < count; ++i)
    {
        m_sortModel->removeRows(idxs[i].row() - i, 1, parentDirectory);
    }
  //  qDebug() << selection.at(0).top() << m_fileView->selectionModel()->selectedRows().size() << parentDirectory.data().toString();
    //m_sortModel->removeRows( selection.at(0).top(), m_fileView->selectionModel()->selectedRows().size(), parentDirectory );
#if 0
    for( int i = selection.size() - 1; i >= 0; --i ) {
    }
#endif
#if 0
    if (!parentDirectory.child(0, 0).isValid())
        m_fileView->setRootIndex(parentDirectory.parent());
#endif
    if (parentDirectory.parent().isValid() &&
            !parentDirectory.child(0, 0).isValid())
        m_fileView->setRootIndex(parentDirectory.parent());
    emit dataChange(parentDirectory, m_sortModel);
}


void K3b::DataViewImpl::slotRename()
{
    const QModelIndex current = m_fileView->currentIndex();
    if( current.isValid() ) {
        m_fileView->edit( current );
    }
}


void K3b::DataViewImpl::slotProperties()
{
    const QModelIndexList indices = m_fileView->selectionModel()->selectedRows();
    if ( indices.isEmpty() )
    {
        // show project properties
        m_view->slotProperties();
    }
    else
    {
        QList<DataItem*> items;

        foreach(const QModelIndex& index, indices) {
            items.append( m_model->itemForIndex( m_sortModel->mapToSource( index ) ) );
        }

        DataPropertiesDialog dlg( items, m_view );
        dlg.exec();
    }
}


void K3b::DataViewImpl::slotOpen()
{
    const QModelIndex current = m_sortModel->mapToSource( m_fileView->currentIndex() );
    if( !current.isValid() )
        return;

    DataItem* item = m_model->itemForIndex( current );

    if (item->isDir() && current.child(0, 0).isValid())
        m_fileView->setRootIndex(m_fileView->currentIndex());
    else
    {
       QUrl url = QUrl::fromLocalFile( item->localPath() );
       QFileInfo f(item->localPath());
       if (!f.exists())
       {
           QMessageBox::critical(m_view, i18n("Error"), i18n("Path not exists"));
           return;
       }
       //KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
       bool a = KRun::runUrl( url,
                   item->mimeType().name(),
                   m_view,
                   KRun::RunFlags());
       if (!a) KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
    }
#if 0
    if( !item->isFile() ) {
        QUrl url = QUrl::fromLocalFile( item->localPath() );
        if( !KRun::isExecutableFile( url,
                                    item->mimeType().name() ) ) {
            bool a = KRun::runUrl( url,
                        item->mimeType().name(),
                        m_view,
                        KRun::RunFlags());
            if (!a) KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
        }
        else {
            KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
        }
    }
#endif
}

void K3b::DataViewImpl::slotParent()
{
    if (m_fileView->rootIndex().parent().isValid())
        m_fileView->setRootIndex(m_fileView->rootIndex().parent());
    m_actionParentDir->setEnabled(false);
#if 0
    m_actionParentDir->setEnabled(false);
    qDebug() << "to parent";
    const QModelIndexList indexes = m_fileView->selectionModel()->selectedRows();
    if (!(indexes[0].parent().parent().isValid()))
        qDebug() << "invalid parent index" << indexes[0].parent().isValid();
    else
        m_fileView->setRootIndex(indexes[0].parent().parent());
#endif
}

void K3b::DataViewImpl::slotSelectionChanged()
{
#if 1
    const QModelIndexList indexes = m_fileView->selectionModel()->selectedRows();

    bool open = true, rename = true, remove = true, flag = true, parent = true;

    qDebug() << indexes.count();

    // we can only rename one item at a time
    // also, we can only create a new dir over a single directory
    if (indexes.count() > 1)
    {
        rename = false;
        open = false;
    }
    else if (indexes.count() == 1)
    {
        QModelIndex index = indexes.first();
        rename = (index.flags() & Qt::ItemIsEditable);
        //open = (index.data(DataProjectModel::ItemTypeRole).toInt() == DataProjectModel::FileItemType);
    }
    else // selectedIndex.count() == 0
    {
        remove = false;
        rename = false;
        open = false;
        parent = false;
    }

    if (indexes.count())
    {
        const QModelIndex current = m_sortModel->mapToSource( indexes[0].parent() );
        if( !current.isValid() ) parent = false;
        else
        {
            DataItem *d = m_model->itemForIndex( current );
            if (d == m_doc->root()) parent = false;
        }

        // check if all selected items can be removed
        foreach(const QModelIndex &index, indexes)
        {
            const QModelIndex current = m_sortModel->mapToSource( index );
            if( !current.isValid() )
            {
                qDebug() << "invalid index." << current;
                break;
            }
            DataItem *d = m_model->itemForIndex( current );
            if (!d->isDeleteable()) { flag = false; remove = false;}
            else { flag = true; remove = true; }
        }
    }

    if (m_fileView->rootIndex().parent().isValid()) parent = true;
    else parent = false;
    m_actionParentDir->setEnabled( parent );
    m_actionRename->setEnabled( rename );
    m_actionRemove->setEnabled( remove );
    m_actionOpen->setEnabled( open );
    emit dataDelete(flag);
#endif
}


void K3b::DataViewImpl::slotItemActivated( const QModelIndex& index )
{
#if 0
    if( index.isValid() ) {
        const int type = index.data( DataProjectModel::ItemTypeRole ).toInt();
        if( type == DataProjectModel::DirItemType ) {
            emit setCurrentRoot( m_sortModel->mapToSource( index ) );
        }
        else if( type == DataProjectModel::FileItemType ) {
            m_fileView->edit( index );
        }
    }
#endif
    K3b::DataItem *d = NULL;
    K3b::DataItem *child = NULL;
    QList<K3b::DataItem *> children;

    d = m_model->itemForIndex(m_sortModel->mapToSource(index));
    if (d && d->isDir() && static_cast<K3b::DirItem *>(d)->children().size())
    {
        m_fileView->setRootIndex(index.siblingAtColumn(0));
        children = static_cast<K3b::DirItem *>(d)->children();
        if (children.count()) child = children.at(0);
        if (child)
            m_fileView->setCurrentIndex(m_model->index(0, 0, m_model->indexForItem(child)));
    }
    else
    {
       QUrl url = QUrl::fromLocalFile( d->localPath() );
       QFileInfo f(d->localPath());
       if (!f.exists())
       {
           QMessageBox::critical(m_view, i18n("Error"), i18n("Path not exists"));
           return;
       }
       //KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
       bool a = KRun::runUrl( url,
                   d->mimeType().name(),
                   m_view,
                   KRun::RunFlags());
       if (!a) KRun::displayOpenWithDialog( QList<QUrl>() << url, m_view );
    }
}


void K3b::DataViewImpl::slotEnterPressed()
{
    slotItemActivated( m_fileView->currentIndex() );
}


void K3b::DataViewImpl::slotImportSession()
{
    K3b::DataMultisessionImportDialog::importSession( m_doc, m_view );
}


void K3b::DataViewImpl::slotClearImportedSession()
{
    m_doc->clearImportedSession();
}


void K3b::DataViewImpl::slotEditBootImages()
{
    BootImageDialog dlg( m_doc );
    dlg.setWindowTitle( i18n("Edit Boot Images") );
    dlg.exec();
}


void K3b::DataViewImpl::slotImportedSessionChanged( int importedSession )
{
    const QModelIndex parent = m_fileView->rootIndex();
    //m_actionClearSession->setEnabled( importedSession > -1 );
    emit dataChange(parent, m_sortModel);
}


void K3b::DataViewImpl::slotAddUrlsRequested( QList<QUrl> urls, K3b::DirItem* targetDir )
{
    //DataUrlAddingDialog::addUrls( urls, targetDir, m_view );
    m_doc->addUrlsToDir(urls, targetDir);
    emit addFiles(urls);
    //emit addDragFiles(urls, targetDir);
}

void K3b::DataViewImpl::slotMoveItemsRequested( QList<K3b::DataItem*> items, K3b::DirItem* targetDir )
{
    DataUrlAddingDialog::moveItems( items, targetDir, m_view );
}
