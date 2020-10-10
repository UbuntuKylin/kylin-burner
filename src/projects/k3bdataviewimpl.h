/*
 *
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

#ifndef K3B_DATA_VIEW_IMPL_H
#define K3B_DATA_VIEW_IMPL_H

#include <QAbstractItemModel>
#include <QObject>
#include <QUrl>

class KActionCollection;
class QAction;
class QSortFilterProxyModel;
class QTreeView;

namespace K3b {
    class ViewColumnAdjuster;
    class DataDoc;
    class DataItem;
    class DataProjectModel;
    class DirItem;
    class View;

    /**
     * This class was created to share code and behaviour between \ref K3b::DataView and \ref K3b::MixedView.
     */
    class DataViewImpl : public QObject
    {
        Q_OBJECT

    public:
        DataViewImpl( View* view, DataDoc* doc, KActionCollection* actionCollection );

        void addUrls( const QModelIndex& parent, const QList<QUrl>& urls );

        DataProjectModel* model() const { return m_model; }
        QTreeView* view() const { return m_fileView; }
        void addDragFile( QList<QUrl> urls, K3b::DirItem* targetDir )
        {
            slotAddUrlsRequested(urls, targetDir);
        }

    signals:
        void setCurrentRoot( const QModelIndex& index );
        void dataChange(QModelIndex parent, QSortFilterProxyModel *model);
        void dataDelete(bool flag);
        void addFiles(QList<QUrl>);
        void addDragFiles(QList<QUrl>, K3b::DirItem*);

    public Q_SLOTS:
        void slotCurrentRootChanged( const QModelIndex& newRoot );
        void slotItemActivated( const QModelIndex& index );
       
        void slotNewDir();
        //void slotOpenDir();
        int slotOpenDir();
        void slotClear();
        void slotRemove();

    private Q_SLOTS:
        /*
        void slotNewDir();
        *************************************
        void slotOpenDir();
        void slotClear();

        void slotRemove();
        */
        void slotRename();
        void slotProperties();
        void slotOpen();
        void slotSelectionChanged();
        void slotEnterPressed();
        void slotImportSession();
        void slotClearImportedSession();
        void slotEditBootImages();
        void slotImportedSessionChanged( int importedSession );
        void slotAddUrlsRequested( QList<QUrl> urls, K3b::DirItem* targetDir );
        void slotMoveItemsRequested( QList<K3b::DataItem*> items, K3b::DirItem* targetDir );

    private:
        View* m_view;
        DataDoc* m_doc;
        DataProjectModel* m_model;
        QSortFilterProxyModel* m_sortModel;
        QTreeView* m_fileView;
        ViewColumnAdjuster* m_columnAdjuster;

        QAction* m_actionParentDir;
        QAction* m_actionRemove;
        QAction* m_actionRename;
        QAction* m_actionNewDir;
        //******************************************
        QAction* m_actionOpenDir;
        QAction* m_actionClear;

        QAction* m_actionProperties;
        QAction* m_actionOpen;
        QAction* m_actionImportSession;
        QAction* m_actionClearSession;
        QAction* m_actionEditBootImages;
    };

} // namespace K3b

#endif
