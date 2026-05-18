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

#include "widgetbinder.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QSignalBlocker>
#include <QWidget>

namespace {
QByteArray signalSignature(const char* notifySignal) {
    if (notifySignal == nullptr || *notifySignal == '\0') {
        return {};
    }

    QByteArray signal(notifySignal);
    if (signal.startsWith('2')) {
        return signal;
    }

    QByteArray normalized = QMetaObject::normalizedSignature(notifySignal);
    if (!normalized.contains('(')) {
        normalized += "()";
    }

    return "2" + normalized;
}
}  // namespace

WidgetBinder::WidgetBinder(QObject* parent) : QObject(parent) {}

WidgetBindingCallback::WidgetBindingCallback(std::function<void()> callback, QObject* parent)
    : QObject(parent), _callback(std::move(callback)) {}

void WidgetBindingCallback::trigger() {
    if (_callback) {
        _callback();
    }
}

void WidgetBinder::connectNotify(QObject* viewModel, const char* notifySignal, const std::function<void()>& update) {
    if (viewModel == nullptr || notifySignal == nullptr || *notifySignal == '\0') {
        return;
    }

    const QByteArray signature = signalSignature(notifySignal);
    auto* callback = new WidgetBindingCallback(update, this);
    const QMetaObject::Connection connection =
        QObject::connect(viewModel, signature.constData(), callback, SLOT(trigger()));
    if (!connection) {
        callback->deleteLater();
    }
}

void WidgetBinder::bindLabel(QLabel* label, QObject* viewModel, const char* property, const char* notifySignal) {
    if (label == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, label, viewModel, property]() {
        _updatingFromModel = true;
        label->setText(viewModel->property(property).toString());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
}

void WidgetBinder::bindLineEdit(QLineEdit* edit, QObject* viewModel, const char* property, const char* notifySignal,
                                std::function<void(const QString&)> onUserChanged) {
    if (edit == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, edit, viewModel, property]() {
        _updatingFromModel = true;
        const QSignalBlocker blocker(edit);
        edit->setText(viewModel->property(property).toString());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
    QObject::connect(edit, &QLineEdit::textEdited, this, [this, onUserChanged](const QString& text) {
        if (_updatingFromModel || !onUserChanged) {
            return;
        }
        onUserChanged(text);
    });
}

void WidgetBinder::bindEnabled(QWidget* widget, QObject* viewModel, const char* property, const char* notifySignal) {
    if (widget == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, widget, viewModel, property]() {
        _updatingFromModel = true;
        widget->setEnabled(viewModel->property(property).toBool());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
}

void WidgetBinder::bindVisible(QWidget* widget, QObject* viewModel, const char* property, const char* notifySignal) {
    if (widget == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, widget, viewModel, property]() {
        _updatingFromModel = true;
        widget->setVisible(viewModel->property(property).toBool());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
}

void WidgetBinder::bindCheckBox(QCheckBox* checkBox, QObject* viewModel, const char* property, const char* notifySignal,
                                std::function<void(bool)> onUserChanged) {
    if (checkBox == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, checkBox, viewModel, property]() {
        _updatingFromModel = true;
        const QSignalBlocker blocker(checkBox);
        checkBox->setChecked(viewModel->property(property).toBool());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
    QObject::connect(checkBox, &QCheckBox::toggled, this, [this, onUserChanged](bool checked) {
        if (_updatingFromModel || !onUserChanged) {
            return;
        }
        onUserChanged(checked);
    });
}

void WidgetBinder::bindComboBox(QComboBox* comboBox, QObject* viewModel, const char* property, const char* notifySignal,
                                std::function<void(int)> onUserChanged) {
    if (comboBox == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, comboBox, viewModel, property]() {
        _updatingFromModel = true;
        const QSignalBlocker blocker(comboBox);
        comboBox->setCurrentIndex(viewModel->property(property).toInt());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
    QObject::connect(comboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, onUserChanged](int index) {
        if (_updatingFromModel || !onUserChanged) {
            return;
        }
        onUserChanged(index);
    });
}

void WidgetBinder::bindModel(QAbstractItemView* view, QAbstractItemModel* model) {
    if (view == nullptr) {
        return;
    }

    view->setModel(model);
}

void WidgetBinder::bindActionEnabled(QAction* action, QObject* viewModel, const char* property,
                                     const char* notifySignal) {
    if (action == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, action, viewModel, property]() {
        _updatingFromModel = true;
        action->setEnabled(viewModel->property(property).toBool());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
}

void WidgetBinder::bindActionChecked(QAction* action, QObject* viewModel, const char* property,
                                     const char* notifySignal) {
    if (action == nullptr || viewModel == nullptr || property == nullptr) {
        return;
    }

    const auto update = [this, action, viewModel, property]() {
        _updatingFromModel = true;
        const QSignalBlocker blocker(action);
        action->setChecked(viewModel->property(property).toBool());
        _updatingFromModel = false;
    };

    update();
    connectNotify(viewModel, notifySignal, update);
}
