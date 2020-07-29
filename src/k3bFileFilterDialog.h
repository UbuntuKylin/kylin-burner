#ifndef K3BFILEFILTER_H
#define K3BFILEFILTER_H

#include <QDialog>
#include <QCheckBox>

class FileFilter : public QDialog
{
    Q_OBJECT

public:
    explicit FileFilter(QWidget *parent = nullptr);
    ~FileFilter();

    QCheckBox *discard_hidden_file;
    QCheckBox *discard_broken_link;
    QCheckBox *follow_link;

    Q_SLOT void filter_exit();

private:

    QDialog *filter_dialog;
};

#endif // BURNFILTER_H
