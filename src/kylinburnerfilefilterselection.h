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

#ifndef KYLINBURNERFILEFILTERSELECTION_H
#define KYLINBURNERFILEFILTERSELECTION_H

#include <QWidget>

namespace Ui {
class KylinBurnerFileFilterSelection;
}

class KylinBurnerFileFilterSelection : public QWidget
{
    Q_OBJECT

public:
    explicit KylinBurnerFileFilterSelection(QWidget *parent);
    ~KylinBurnerFileFilterSelection();
public:
    bool eventFilter(QObject *obj, QEvent *event);
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void labelCloseStyle(bool in);
    void setOption(bool hidden, bool broken, bool replace);
signals:
    void changeSetting(int option, bool enable);
private slots:
    void on_checkBoxHidden_stateChanged(int arg1);

    void on_checkBoxWrongSymbol_stateChanged(int arg1);

    void on_checkBoxReplaceSymbol_stateChanged(int arg1);

private:
    Ui::KylinBurnerFileFilterSelection *ui;
};

#endif // KYLINBURNERFILEFILTERSELECTION_H
