// SPDX-License-Identifier: BUSL-1.1
#include "app/main_window.hpp"
#include "dialogs/load_options_dialog.hpp"
#include "dialogs/welcome_dialog.hpp"
#include "irida/irida.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // A path on the command line opens that binary directly (file association,
    // terminal use); otherwise the welcome window chooses the target.
    QString path;
    if (argc > 1) {
        path = QString::fromLocal8Bit(argv[1]);
    } else {
        WelcomeDialog welcome;
        if (welcome.exec() != QDialog::Accepted)
            return 0; // no target chosen
        path = welcome.selectedPath();
    }

    LoadOptionsDialog loadOptions(path);
    if (loadOptions.exec() != QDialog::Accepted)
        return 0; // cancelled at the options step
    LoadOptionsDialog::Options opts = loadOptions.options();

    IridaSession* session = irida_session_create_file(path.toLocal8Bit().constData());
    if (!session) {
        QMessageBox::critical(nullptr, "Open failed", "Could not open or parse:\n" + path);
        return 1;
    }
    WelcomeDialog::addRecent(path);

    MainWindow w(session, SessionKind::Static, opts.analyze);
    w.setWindowTitle("Irida — " + path);
    w.show();
    return app.exec();
}
