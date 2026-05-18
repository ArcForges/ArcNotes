#include "aboutdialog.h"

#include <utils/misc.h>

#include <QApplication>
#include <QDate>
#include <QDialogButtonBox>
#include <QFile>
#include <QIcon>
#include <QTextBrowser>
#include <QTextStream>
#include <QVBoxLayout>

#include "build_number.h"
#include "release.h"
#include "version.h"

AboutDialog::AboutDialog(QWidget* parent) : MasterDialog(parent) {
    setWindowTitle(tr("About ArcNotes"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("dialog-information"),
                                   QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/dialog-information.svg"))));
    resize(684, 553);

    auto* layout = new QVBoxLayout(this);
    auto* textBrowser = new QTextBrowser(this);
    textBrowser->setEnabled(true);
    textBrowser->setStyleSheet(
        QStringLiteral("* {\n"
                       "    background-color: transparent;\n"
                       "    border: none;\n"
                       "}"));
    textBrowser->setOpenExternalLinks(true);
    layout->addWidget(textBrowser);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    afterSetupUI();

    QFile f(QStringLiteral(":/html/about.html"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream istream(&f);
        QString html = istream.readAll();
        const QDate date = QDate::currentDate();

        QString release = qApp->property("release").toString();
        if (release.isEmpty()) {
            release = QLatin1String("generic");
        }

        html.replace(QLatin1String("QT_VERSION_STR"), QStringLiteral(QT_VERSION_STR));
        html.replace(QLatin1String("BUILD_NUMBER"), QString::number(BUILD));
        html.replace(QLatin1String("BUILD_DATE"), __DATE__);
        html.replace(QLatin1String("VERSION"), QStringLiteral(VERSION));
        html.replace(QLatin1String("RELEASE"), release);
        html.replace(QLatin1String("CURRENT_YEAR"), QString::number(date.year()));

        textBrowser->document()->setDefaultStyleSheet(Utils::Misc::genericCSS());
        textBrowser->setHtml(html);
        f.close();
    }
}

AboutDialog::~AboutDialog() = default;
