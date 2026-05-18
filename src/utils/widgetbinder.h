/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 */

#pragma once

#include <QObject>
#include <functional>

class QAction;
class QAbstractItemModel;
class QAbstractItemView;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QWidget;

class WidgetBindingCallback : public QObject {
    Q_OBJECT

public:
    explicit WidgetBindingCallback(std::function<void()> callback, QObject* parent = nullptr);

public slots:
    void trigger();

private:
    std::function<void()> _callback;
};

class WidgetBinder : public QObject {
    Q_OBJECT

public:
    explicit WidgetBinder(QObject* parent = nullptr);

    void bindLabel(QLabel* label, QObject* viewModel, const char* property, const char* notifySignal);
    void bindLineEdit(QLineEdit* edit, QObject* viewModel, const char* property, const char* notifySignal,
                      std::function<void(const QString&)> onUserChanged);
    void bindEnabled(QWidget* widget, QObject* viewModel, const char* property, const char* notifySignal);
    void bindVisible(QWidget* widget, QObject* viewModel, const char* property, const char* notifySignal);
    void bindCheckBox(QCheckBox* checkBox, QObject* viewModel, const char* property, const char* notifySignal,
                      std::function<void(bool)> onUserChanged);
    void bindComboBox(QComboBox* comboBox, QObject* viewModel, const char* property, const char* notifySignal,
                      std::function<void(int)> onUserChanged);
    void bindModel(QAbstractItemView* view, QAbstractItemModel* model);
    void bindActionEnabled(QAction* action, QObject* viewModel, const char* property, const char* notifySignal);
    void bindActionChecked(QAction* action, QObject* viewModel, const char* property, const char* notifySignal);

private:
    void connectNotify(QObject* viewModel, const char* notifySignal, const std::function<void()>& update);
    bool _updatingFromModel = false;
};
