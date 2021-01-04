/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include "k3bdataprojectmodel.h"

#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bfileitem.h"
#include "k3bisooptions.h"
#include "k3bspecialdataitem.h"

#include <KUrlMimeData>
#include <KLocalizedString>
#include <KIconEngine>

#include <QDataStream>
#include <QMimeData>
#include <QFont>
#include <QBrush>
#include <QBitmap>


class K3b::DataProjectModel::Private
{
public:
    Private( DataProjectModel* parent )
        : project( 0 ),
          q( parent ) {
    }

    K3b::DataDoc* project;

    K3b::DataItem* getChild( K3b::DirItem* dir, int offset );
    int findChildIndex( K3b::DataItem* item );
    void _k_itemsAboutToBeInserted( K3b::DirItem* parent, int start, int end );
    void _k_itemsAboutToBeRemoved( K3b::DirItem* parent, int start, int end );
    void _k_itemsInserted( K3b::DirItem* parent, int start, int end );
    void _k_itemsRemoved( K3b::DirItem* parent, int start, int end );
    void _k_volumeIdChanged();

private:
    DataProjectModel* q;

    bool m_removingItem;
};



K3b::DataItem* K3b::DataProjectModel::Private::getChild( K3b::DirItem* dir, int offset )
{
    QList<K3b::DataItem*> const& children = dir->children();
    if ( offset >= 0 && offset < children.count() ) {
        return children[offset];
    }

    return 0;
}


int K3b::DataProjectModel::Private::findChildIndex( K3b::DataItem* item )
{
    if ( !item )
        return 0;
    else if ( DirItem* dir = item->parent() )
        return dir->children().indexOf( item );
    else
        return 0;
}


void K3b::DataProjectModel::Private::_k_itemsAboutToBeInserted( K3b::DirItem* parent, int start, int end )
{
        qDebug() << q->indexForItem( parent ) << start << end;
    q->beginInsertRows( q->indexForItem( parent ), start, end );
}


void K3b::DataProjectModel::Private::_k_itemsAboutToBeRemoved( K3b::DirItem* parent, int start, int end )
{
    m_removingItem = true;
    qDebug() << q->indexForItem( parent ) << start << end;
    q->beginRemoveRows( q->indexForItem( parent ), start, end );
}


void K3b::DataProjectModel::Private::_k_itemsInserted( K3b::DirItem* /*parent*/, int /*start*/, int /*end*/ )
{
    q->endInsertRows();
}


void K3b::DataProjectModel::Private::_k_itemsRemoved( K3b::DirItem* /*parent*/, int /*start*/, int /*end*/ )
{
    if ( m_removingItem ) {
        q->endRemoveRows();
    }
}


void K3b::DataProjectModel::Private::_k_volumeIdChanged()
{
    QModelIndex index = q->index( 0, 0 );
    emit q->dataChanged( index, index );
}


K3b::DataProjectModel::DataProjectModel( K3b::DataDoc* doc, QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private(this) )
{
    d->project = doc;

    connect( doc, SIGNAL(itemsAboutToBeInserted(K3b::DirItem*,int,int)),
             this, SLOT(_k_itemsAboutToBeInserted(K3b::DirItem*,int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(itemsAboutToBeRemoved(K3b::DirItem*,int,int)),
             this, SLOT(_k_itemsAboutToBeRemoved(K3b::DirItem*,int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(itemsInserted(K3b::DirItem*,int,int)),
             this, SLOT(_k_itemsInserted(K3b::DirItem*,int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(itemsRemoved(K3b::DirItem*,int,int)),
             this, SLOT(_k_itemsRemoved(K3b::DirItem*,int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(volumeIdChanged()),
             this, SLOT(_k_volumeIdChanged()), Qt::DirectConnection );
}


K3b::DataProjectModel::~DataProjectModel()
{
    delete d;
}


K3b::DataDoc* K3b::DataProjectModel::project() const
{
    return d->project;
}

K3b::DataItem* K3b::DataProjectModel::itemForIndex( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Q_ASSERT( index.internalPointer() );
        return static_cast<K3b::DataItem*>( index.internalPointer() );
    }
    else {
        return 0;
    }
}

QModelIndex K3b::DataProjectModel::indexForItem( K3b::DataItem* item ) const
{
    return createIndex( d->findChildIndex( item ), 0, item );
}


int K3b::DataProjectModel::columnCount( const QModelIndex& /*index*/ ) const
{
    return NumColumns;
}


QVariant K3b::DataProjectModel::data( const QModelIndex& index, int role ) const
{
    if ( DataItem* item = itemForIndex( index ) ) {

        if ( role == ItemTypeRole ) {
            if (item->isDir())
                return DirItemType;
            else
                return FileItemType;
        }
        else if ( role == CustomFlagsRole ) {
            if (item->isRemoveable())
                return ItemIsRemovable;
            else
                return 0;
        }
        else if (role == DeleteableRole) {
            if (item->isDeleteable())
                return ItemIsRemovable;
            else
                return 0;
        }
        else if ( role == Qt::StatusTipRole ) {
            if (item->isSymLink())
                return i18nc( "Symlink target shown in status bar", "Link to %1", static_cast<FileItem*>( item )->linkDest() );
            else
                return QVariant();
        }

        switch( index.column() ) {
        case FilenameColumn:
            if( role == Qt::DisplayRole ||
                role == Qt::EditRole ) {
                return item->k3bName();
            }
            else if (role == SortRole)
            {
                return item->inTime();
            }
            else if ( role == Qt::DecorationRole ) {
                QString iconName;
                QIcon iconPic;
                QPixmap pixmap;
                QBitmap bitmap;

                if ( item->isDir() && item->parent() ) {
                    iconName = ( static_cast<K3b::DirItem*>( item )->depth() > 7 ? "folder-root" : "folder" );
                }
                else if ( item->isDir() ) {
                    iconName = "media-optical-data";
                }
                else {
                    iconName = item->mimeType().iconName();
                }

                if( item->isSymLink() )
                    iconPic = QIcon( new KIconEngine( iconName, nullptr, QStringList() << "emblem-symbolic-link" ) );
                else
                    iconPic = QIcon::fromTheme( iconName );
                if (!item->isDeleteable())
                {
                    pixmap = iconPic.pixmap(QSize(24, 24), QIcon::Disabled);
                    return QIcon(pixmap);
                }
                return iconPic;
            }
            else if( role == Qt::FontRole && item->isSymLink() ) {
                QFont font;
                font.setPixelSize( 14 );
                font.setItalic( true );
                return font;
            }
            /*
            if (role == Qt::TextColorRole)
                return QColor(179,179,179,1);
*/
            if( role == Qt::SizeHintRole ){
                return QSize(274, 35);
            }
            break;

        case TypeColumn:
            if( role == Qt::DisplayRole ||
                role == SortRole ) {
                if ( item->isSpecialFile() ) {
                    return static_cast<K3b::SpecialDataItem*>( item )->specialType();
                }
                else {
                    return item->mimeType().comment();
                }
            }
            if( role == Qt::SizeHintRole ){
                return QSize(84, 35);
            }
            break;

        case SizeColumn:
            if( role == Qt::DisplayRole ) {
                return KIO::convertSize( item->size() );
            }
            else if ( role == SortRole ) {
                return item->size();
            }
            if( role == Qt::SizeHintRole ){
                return QSize(77, 35);
            }
            break; 
        //***********************************************************
        case PathColumn:
            if ( role == Qt::DisplayRole )
                return item->localPath();
            if( role == Qt::SizeHintRole ){
                return QSize(198, 35);
            }
            break;

        }
        if (!item->isDeleteable() && role == Qt::ForegroundRole)
            return QBrush(QColor(179,179,179), Qt::SolidPattern);
    }

    return QVariant();
}


QVariant K3b::DataProjectModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole ) {
        switch( section ) {
        case FilenameColumn:
            return i18nc( "file name", "Name" );
        case TypeColumn:
            return i18nc( "file type", "Type" );
        case SizeColumn:
            return i18nc( "file size", "Size" );
        //**************************************************************************
        case PathColumn:
            return i18nc( "file path", "Path" );
        }
    }
    else if (role == Qt::FontRole)
    {
        QFont f;
        f.setPixelSize(14);
        return f;
    }

    return QVariant();
}


Qt::ItemFlags K3b::DataProjectModel::flags( const QModelIndex& index ) const
{
    if ( index.isValid() ) {
        Qt::ItemFlags f = Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDropEnabled;
        if ( index.column() == FilenameColumn ) {
            f |= Qt::ItemIsEditable;
        }
        if ( itemForIndex( index ) != d->project->root() ) {
            f |= Qt::ItemIsDragEnabled;
        }

        return f;
    }
    else {
        return QAbstractItemModel::flags( index )|Qt::ItemIsDropEnabled;
    }
}


QModelIndex K3b::DataProjectModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !hasIndex( row, column, parent ) ) {
        return QModelIndex();
    }
    else if ( parent.isValid() ) {
        K3b::DirItem* dir = itemForIndex( parent )->getDirItem();
        K3b::DataItem* child = d->getChild( dir, row );
        if ( child && parent.column() == 0 ) {
            return createIndex( row, column, child );
        }
        else {
            return QModelIndex();
        }
    }
    else {
        // doc root item
        return createIndex( row, column, d->project->root() );
    }
}


QModelIndex K3b::DataProjectModel::parent( const QModelIndex& index ) const
{
    //qDebug() << index;
    if( K3b::DataItem* item = itemForIndex( index ) ) {
        if( K3b::DirItem* dir = item->parent() ) {
            return createIndex( d->findChildIndex( dir ), 0, dir );
        }
    }
    return QModelIndex();
}


int K3b::DataProjectModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.isValid() ) {
        K3b::DataItem* item = itemForIndex( parent );
        K3b::DirItem* dir = dynamic_cast<K3b::DirItem*>( item );
        if ( dir != 0 && parent.column() == 0 ) {
            return( dir->children().count() );
        }
        else {
            return 0;
        }
    }
    else {
        return 1;
    }
}


bool K3b::DataProjectModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( index.isValid() ) {
        K3b::DataItem* item = itemForIndex( index );
        if ( role == Qt::EditRole ) {
            if ( index.column() == 0 ) {
                item->setK3bName( value.toString() );
                emit dataChanged( index, index );
                return true;
            }
        }
    }

    return false;
}


QMimeData* K3b::DataProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();

    QSet<K3b::DataItem*> items;
    QList<QUrl> urls;
    foreach( const QModelIndex& index, indexes ) {
        K3b::DataItem* item = itemForIndex( index );
        items << item;

        QUrl url = QUrl::fromLocalFile( item->localPath() );
        if ( item->isFile() && !urls.contains(url) ) {
            urls << url;
        }
    }
    mime->setUrls(urls);

    // the easy road: encode the pointers
    QByteArray itemData;
    QDataStream itemDataStream( &itemData, QIODevice::WriteOnly );
    foreach( K3b::DataItem* item, items ) {
        itemDataStream << ( qint64 )item;
    }
    mime->setData( "application/x-k3bdataitem", itemData );

    return mime;
}


Qt::DropActions K3b::DataProjectModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList K3b::DataProjectModel::mimeTypes() const
{
    QStringList s = KUrlMimeData::mimeDataTypes();
    s += QString::fromLatin1( "application/x-k3bdataitem" );
    return s;
}


bool K3b::DataProjectModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    qDebug();

    // no need to handle the row as item order is not important (the model just forces us to use them above)
    Q_UNUSED( row );
    Q_UNUSED( column );

    if (action == Qt::IgnoreAction)
        return true;

    // determine the target dir:
    // - drop ontp an item -> get the item's dir (i.e. itself or its parent)
    // - drop onto the viewport -> the project's root
    // --------------------------------------------------------------
    K3b::DirItem* dir = d->project->root();
    if ( parent.isValid() ) {
        dir = itemForIndex( parent )->getDirItem();
    }

    if ( data->hasFormat( "application/x-k3bdataitem" ) ) {
        if( action == Qt::MoveAction )
            return false;

        qDebug() << "data item drop";

        QByteArray itemData = data->data( "application/x-k3bdataitem" );
        QDataStream itemDataStream( itemData );
        QList<K3b::DataItem*> items;
        while ( !itemDataStream.atEnd() ) {
            qint64 p;
            itemDataStream >> p;
            items << ( K3b::DataItem* )p;
        }
        // always move the items, no copy from within the views
        emit moveItemsRequested( items, dir );
        return true;
    }
    else if ( data->hasUrls() ) {
        qDebug() << "url list drop";
        QList<QUrl> urls = KUrlMimeData::urlsFromMimeData( data );
        emit addUrlsRequested( urls, dir );
        return true;
    }
    else {
        return false;
    }
}


bool K3b::DataProjectModel::removeRows( int row, int count, const QModelIndex& parent)
{
    DirItem* dirItem = dynamic_cast<DirItem*>( itemForIndex( parent ) );
    if( dirItem && row >= 0 && count > 0 ) {
        // remove the indexes from the project
        int i  = row;
        while (count)
        {
            qDebug() << i << count << dirItem->children().size();
            if (dirItem->children().at(i) && dirItem->children().at(i)->isDeleteable())
            {
                d->project->removeItems( dirItem, i, 1 );
                --count; continue;
            }
            --count;
            ++i;
        }
        //d->project->removeItems( dirItem, row, count );
        return true;
    }
    else {
        return false;
    }
}

QModelIndex K3b::DataProjectModel::buddy( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() != FilenameColumn)
        return DataProjectModel::index( index.row(), FilenameColumn, index.parent() );
    else
        return index;
}

#include "moc_k3bdataprojectmodel.cpp"
