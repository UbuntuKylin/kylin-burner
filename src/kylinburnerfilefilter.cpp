#include "kylinburnerfilefilter.h"
#include "ui_kylinburnerfilefilter.h"
#include "k3bapplication.h"
#include "k3bprojectmanager.h"
#include "k3bdiritem.h"

#include <QDebug>
#include <QMouseEvent>

KylinBurnerFileFilter::KylinBurnerFileFilter(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KylinBurnerFileFilter)
{
    //setAttribute(Qt::WA_ShowModal);
    ui->setupUi(this);
    selection = new KylinBurnerFileFilterSelection(this);
    setWindowFlags (Qt::Window);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    //qDebug() << "-----------------------------" << pos().x() << pos().y();
    //qDebug() << "-----------------------------" << mapFromGlobal(parent->pos()).x() << mapFromGlobal(parent->pos()).y();
    this->move(parent->width() / 2 - width() / 2, parent->height() / 2 - height() / 2);
    this->hide();

    ui->labelTitle->setText(i18n("Kylin-Burner"));
    ui->labelName->setText(i18n("FilterFile"));
    ui->btnSetting->setText(i18n("FileFilterSetting"));
    ui->btnRecovery->setText(i18n("Recovery"));
    ui->labelClose->setAttribute(Qt::WA_Hover, true);
    ui->labelClose->installEventFilter(this);
    currentData = static_cast<K3b::DataDoc *>(k3bappcore->projectManager()->createProject( K3b::Doc::DataProject ));
    model = new K3b::DataProjectModel(currentData, this);
    /*
    QList<QUrl> urls;

    urls << QUrl("file:///media/derek/ubuntukylin");
    currentData->addUrls(urls);
    */
    ui->treeView->setModel(model);
    ui->treeView->header()->setStyleSheet("QHeaderView::section{"
                                        "border: 0px solid white;"
                                        "background-color : rgba(242, 242, 242, 1);}");
    ui->treeView->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->treeView->setColumnWidth(0, ui->treeView->width() / 5 * 4);
    ui->treeView->setColumnWidth(1, ui->treeView->width());
    ui->treeView->setColumnWidth(2, 0);
    ui->treeView->setColumnWidth(3, 0);
    ui->treeView->header()->setSectionHidden(2, true);
    ui->treeView->header()->setSectionHidden(3, true);
    ui->treeView->setItemsExpandable( false );
    ui->treeView->setRootIsDecorated( false );
    ui->treeView->setSelectionMode( QTreeView::ExtendedSelection );
    ui->treeView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    ui->treeView->setContextMenuPolicy( Qt::ActionsContextMenu );
    ui->treeView->setSortingEnabled( false );
    ui->treeView->sortByColumn( K3b::DataProjectModel::FilenameColumn, Qt::AscendingOrder );
    ui->treeView->setMouseTracking( false );
    ui->treeView->setAllColumnsShowFocus( true );
    ui->treeView->setRootIndex(model->indexForItem(currentData->root()));
    /*
    K3b::DirItem *root = static_cast<K3b::DirItem *>(currentData->root());
    while (root->children().size() == 1) root = static_cast<K3b::DirItem *>(root->children().at(0));
    ui->treeView->setRootIndex(model->indexForItem(root));
    */
    isChange = false; isHidden = false; isBroken = false; isReplace = false;
    connect( ui->treeView, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(slotDoubleClicked(QModelIndex)) );
}

KylinBurnerFileFilter::~KylinBurnerFileFilter()
{
    delete selection;
    delete ui;
}

void KylinBurnerFileFilter::slotDoubleClicked(QModelIndex idx)
{
    K3b::DataItem *d = NULL;

    d = model->itemForIndex(idx);
    if (d && d->isDir() && static_cast<K3b::DirItem *>(d)->children().size()) ui->treeView->setRootIndex(idx);
}

void KylinBurnerFileFilter::slotDoFileFilter(K3b::DataDoc *doc)
{
    K3b::DataItem *child;
    oldData = doc;
    currentData->clear();
    for (int i = 0; i < doc->root()->children().size(); ++i)
    {
        child = doc->root()->children().at(i);
        if (child->isDeleteable())
            currentData->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
        else
            currentData->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
    }
}

void KylinBurnerFileFilter::slotDoChangeSetting(int option, bool enable)
{
    //currentData->clearDisk();
    currentData->clear();
    switch (option)
    {
    case 0:/* hidden */
        if (isHidden != enable)
        {
            isChange = true;
            isHidden = enable;
        }
        break;
    case 1:/* broken */
        if (isBroken != enable)
        {
            isChange = true;
            isBroken = enable;
        }
        break;
    default:/* replace */
        if (isReplace != enable)
        {
            isChange = true;
            isReplace = enable;
        }
        break;
    }
    /* this means default setting is all disable, so means no change.
     * can add an function to set these default and remember the default
     * to consider the data is changed or not.
     */
    if (!isHidden && !isBroken && !isReplace) isChange = false;
    onChanged(oldData->root());
}

bool KylinBurnerFileFilter::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent *mouseEvent;
    K3b::DataItem *child;

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
        mouseEvent = static_cast<QMouseEvent *>(event);
        if (ui->labelClose == obj && (Qt::LeftButton == mouseEvent->button()))
        {
            this->close();
            if (isChange)
            {
                oldData->clear();
                for (int i = 0; i < currentData->root()->children().size(); ++i)
                {
                    child = currentData->root()->children().at(i);
                    if (child->isDeleteable())
                        oldData->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
                    else
                        oldData->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));

                }
                emit finished(oldData);
            }
        }
        break;
    case QEvent::HoverEnter:
        if (ui->labelClose == obj) labelCloseStyle(true);
        break;
    case QEvent::HoverLeave:
        if (ui->labelClose == obj) labelCloseStyle(false);
        break;
    default:
        return QWidget::eventFilter(obj, event);
    }

    return QWidget::eventFilter(obj, event);
}

void KylinBurnerFileFilter::labelCloseStyle(bool in)
{
    if (in)
    {
        ui->labelClose->setStyleSheet("background-color:rgba(247,99,87,1);"
                                      "image: url(:/icon/icon/icon-关闭-悬停点击.png); ");
    }
    else
    {
        ui->labelClose->setStyleSheet("background-color:transparent;"
                                      "image: url(:/icon/icon/icon-关闭-默认.png); ");
    }
}

void KylinBurnerFileFilter::onChanged(K3b::DirItem *parent)
{
    if (!parent) return;
    K3b::DataItem *child;
    for (int i = 0; i < parent->children().size(); ++i)
    {
        child = parent->children().at(i);
        if (isHidden && child->k3bName().size() > 1 && child->k3bName().at(0) == '.') continue;
        /*
         * Todo : if hide all files under dir or not?
         */
        if (child->isDeleteable())
            currentData->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
        else
            currentData->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));

        //if (child->isDir()) onChanged(static_cast<K3b::DirItem *>(child));
    }
}

void KylinBurnerFileFilter::on_btnSetting_clicked()
{
    //selection->setAttribute(Qt::WA_ShowModal);
    selection->setOption(isHidden, isBroken, isReplace);
    selection->show();
}

void KylinBurnerFileFilter::on_btnRecovery_clicked()
{
    if (!isChange) return;
    K3b::DataItem *child;
    isChange = false; isHidden = false; isBroken = false; isReplace = false;
    selection->setOption(isHidden, isBroken, isReplace);
    currentData->clear();
    for (int i = 0; i < oldData->root()->children().size(); ++i)
    {
        child = oldData->root()->children().at(i);
        if (child->isDeleteable())
            currentData->addUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));
        else
            currentData->addUnremovableUrls(QList<QUrl>() << QUrl::fromLocalFile(child->localPath()));

        //if (child->isDir()) onChanged(static_cast<K3b::DirItem *>(child));
    }
}
