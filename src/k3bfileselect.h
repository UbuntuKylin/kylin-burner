#ifndef K3BFILESELECT_H
#define K3BFILESELECT_H

#include <QWidget>
#include <QFileDialog>
#include <QListView>
#include <QTreeView>
#include <QDialogButtonBox>
class myFileSelect :public QFileDialog
{
Q_OBJECT
public:
    explicit myFileSelect(QWidget *parent = 0);

signals:

public slots:
        void go();
};

#endif // K3BFILESELECT_H
