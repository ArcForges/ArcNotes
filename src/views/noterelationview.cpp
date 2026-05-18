#include "noterelationview.h"

#include <QGraphicsScene>

NoteRelationView::NoteRelationView(QWidget* parent) : QGraphicsView(parent) {
    setObjectName(QStringLiteral("noteRelationView"));
    setScene(new QGraphicsScene(this));
}
