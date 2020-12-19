/*
 * Copyright (C) 2020  KylinSoft Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef K3BFILEFILTER_H
#define K3BFILEFILTER_H

#include <QDialog>
#include <QCheckBox>

class FileFilter : public QDialog
{
    Q_OBJECT
signals:
    void setHidden(bool);
    void setBroken(bool);
    void setReplace(bool);

public:
    explicit FileFilter(QWidget *parent = nullptr);
    ~FileFilter();

    QCheckBox *discard_hidden_file;
    QCheckBox *discard_broken_link;
    QCheckBox *follow_link;

    Q_SLOT void filter_exit();

private slots:
    void hiddenChanged(int);
    void brokenChanged(int);
    void replaceChanged(int);

public:
    void setIsHidden(bool);
    void setIsBroken(bool);
    void setIsReplace(bool);
protected:
    bool eventFilter(QObject *, QEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event);
private:
    QPushButton *c;
    QDialog *filter_dialog;
};

#endif // BURNFILTER_H
