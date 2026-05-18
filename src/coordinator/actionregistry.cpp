#include "actionregistry.h"

#include <QAction>
#include <QIcon>
#include <QKeySequence>

ActionRegistry::ActionRegistry(QObject* parent) : QObject(parent) {}

QAction* ActionRegistry::registerAction(const QString& id, const QString& text, const QString& category,
                                        const QKeySequence& shortcut, const QIcon& icon) {
    if (id.isEmpty()) {
        return nullptr;
    }

    QAction* registeredAction = _actionsById.value(id, nullptr);
    if (registeredAction == nullptr) {
        registeredAction = new QAction(this);
        registeredAction->setObjectName(id);
        _actionsById.insert(id, registeredAction);
    }

    registeredAction->setText(text);
    registeredAction->setShortcut(shortcut);
    if (!icon.isNull()) {
        registeredAction->setIcon(icon);
    }
    _categoriesById.insert(id, category);
    emit actionRegistered(id, registeredAction);
    return registeredAction;
}

QAction* ActionRegistry::action(const QString& id) const {
    return _actionsById.value(id, nullptr);
}

QList<QAction*> ActionRegistry::actionsForCategory(const QString& category) const {
    QList<QAction*> actions;
    const QStringList ids = actionIds();
    for (const QString& id : ids) {
        if (_categoriesById.value(id) == category) {
            actions.append(_actionsById.value(id));
        }
    }
    return actions;
}

QStringList ActionRegistry::actionIds() const {
    QStringList ids = _actionsById.keys();
    ids.sort();
    return ids;
}

QStringList ActionRegistry::categories() const {
    QStringList values;
    for (const QString& category : _categoriesById) {
        if (!values.contains(category)) {
            values.append(category);
        }
    }
    values.sort();
    return values;
}

QKeySequence ActionRegistry::shortcut(const QString& id) const {
    QAction* registeredAction = action(id);
    return registeredAction == nullptr ? QKeySequence() : registeredAction->shortcut();
}

void ActionRegistry::registerDefaultActions() {
    registerAction(QStringLiteral("file.new-note"), tr("New note"), QStringLiteral("file"),
                   QKeySequence(QKeySequence::New));
    registerAction(QStringLiteral("file.open-note"), tr("Open note"), QStringLiteral("file"),
                   QKeySequence(QKeySequence::Open));
    registerAction(QStringLiteral("file.save-note"), tr("Save note"), QStringLiteral("file"),
                   QKeySequence(QKeySequence::Save));
    registerAction(QStringLiteral("file.export-note"), tr("Export note"), QStringLiteral("file"));
    registerAction(QStringLiteral("file.print-note"), tr("Print note"), QStringLiteral("file"),
                   QKeySequence(QKeySequence::Print));
    registerAction(QStringLiteral("file.quit"), tr("Quit"), QStringLiteral("file"), QKeySequence(QKeySequence::Quit));

    registerAction(QStringLiteral("edit.undo"), tr("Undo"), QStringLiteral("edit"), QKeySequence(QKeySequence::Undo));
    registerAction(QStringLiteral("edit.redo"), tr("Redo"), QStringLiteral("edit"), QKeySequence(QKeySequence::Redo));
    registerAction(QStringLiteral("edit.cut"), tr("Cut"), QStringLiteral("edit"), QKeySequence(QKeySequence::Cut));
    registerAction(QStringLiteral("edit.copy"), tr("Copy"), QStringLiteral("edit"), QKeySequence(QKeySequence::Copy));
    registerAction(QStringLiteral("edit.paste"), tr("Paste"), QStringLiteral("edit"),
                   QKeySequence(QKeySequence::Paste));
    registerAction(QStringLiteral("edit.find"), tr("Find"), QStringLiteral("edit"), QKeySequence(QKeySequence::Find));
    registerAction(QStringLiteral("edit.find-next"), tr("Find next"), QStringLiteral("edit"),
                   QKeySequence(QKeySequence::FindNext));
    registerAction(QStringLiteral("edit.find-previous"), tr("Find previous"), QStringLiteral("edit"),
                   QKeySequence(QKeySequence::FindPrevious));

    registerAction(QStringLiteral("format.bold"), tr("Bold"), QStringLiteral("format"),
                   QKeySequence(Qt::CTRL | Qt::Key_B));
    registerAction(QStringLiteral("format.italic"), tr("Italic"), QStringLiteral("format"),
                   QKeySequence(Qt::CTRL | Qt::Key_I));
    registerAction(QStringLiteral("format.strikethrough"), tr("Strikethrough"), QStringLiteral("format"));
    registerAction(QStringLiteral("format.heading-1"), tr("Heading 1"), QStringLiteral("format"));
    registerAction(QStringLiteral("format.heading-2"), tr("Heading 2"), QStringLiteral("format"));
    registerAction(QStringLiteral("format.heading-3"), tr("Heading 3"), QStringLiteral("format"));
    registerAction(QStringLiteral("format.unordered-list"), tr("Unordered list"), QStringLiteral("format"));
    registerAction(QStringLiteral("format.ordered-list"), tr("Ordered list"), QStringLiteral("format"));
    registerAction(QStringLiteral("format.insert-link"), tr("Insert link"), QStringLiteral("format"),
                   QKeySequence(Qt::CTRL | Qt::Key_L));
    registerAction(QStringLiteral("format.insert-image"), tr("Insert image"), QStringLiteral("format"));

    registerAction(QStringLiteral("note.delete"), tr("Delete note"), QStringLiteral("note"));
    registerAction(QStringLiteral("note.rename"), tr("Rename note"), QStringLiteral("note"));
    registerAction(QStringLiteral("note.duplicate"), tr("Duplicate note"), QStringLiteral("note"));
    registerAction(QStringLiteral("note.move"), tr("Move note"), QStringLiteral("note"));
    registerAction(QStringLiteral("note.restore"), tr("Restore note"), QStringLiteral("note"));
    registerAction(QStringLiteral("note.show-in-folder"), tr("Show in folder"), QStringLiteral("note"));

    registerAction(QStringLiteral("tag.create"), tr("Create tag"), QStringLiteral("tag"));
    registerAction(QStringLiteral("tag.delete"), tr("Delete tag"), QStringLiteral("tag"));
    registerAction(QStringLiteral("tag.rename"), tr("Rename tag"), QStringLiteral("tag"));
    registerAction(QStringLiteral("tag.move"), tr("Move tag"), QStringLiteral("tag"));
    registerAction(QStringLiteral("tag.assign"), tr("Assign tag"), QStringLiteral("tag"));
    registerAction(QStringLiteral("tag.remove"), tr("Remove tag"), QStringLiteral("tag"));

    registerAction(QStringLiteral("view.distraction-free"), tr("Distraction free"), QStringLiteral("view"));
    registerAction(QStringLiteral("view.full-screen"), tr("Full screen"), QStringLiteral("view"),
                   QKeySequence(QKeySequence::FullScreen));
    registerAction(QStringLiteral("view.toggle-preview"), tr("Toggle preview"), QStringLiteral("view"));
    registerAction(QStringLiteral("view.toggle-navigation"), tr("Toggle navigation"), QStringLiteral("view"));
    registerAction(QStringLiteral("view.toggle-tags"), tr("Toggle tags"), QStringLiteral("view"));
    registerAction(QStringLiteral("view.toggle-trash"), tr("Toggle trash"), QStringLiteral("view"));

    registerAction(QStringLiteral("tools.command-bar"), tr("Command bar"), QStringLiteral("tools"),
                   QKeySequence(Qt::CTRL | Qt::Key_P));
    registerAction(QStringLiteral("tools.settings"), tr("Settings"), QStringLiteral("tools"),
                   QKeySequence(QKeySequence::Preferences));
    registerAction(QStringLiteral("tools.reload-index"), tr("Reload index"), QStringLiteral("tools"));
    registerAction(QStringLiteral("tools.clear-trash"), tr("Clear trash"), QStringLiteral("tools"));

    registerAction(QStringLiteral("help.about"), tr("About ArcNotes"), QStringLiteral("help"));
    registerAction(QStringLiteral("help.documentation"), tr("Documentation"), QStringLiteral("help"));
    registerAction(QStringLiteral("help.report-issue"), tr("Report issue"), QStringLiteral("help"));
}

void ActionRegistry::setShortcut(const QString& id, const QKeySequence& shortcut) {
    QAction* registeredAction = action(id);
    if (registeredAction == nullptr || registeredAction->shortcut() == shortcut) {
        return;
    }

    registeredAction->setShortcut(shortcut);
    emit shortcutChanged(id, shortcut);
}
