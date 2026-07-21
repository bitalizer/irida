// SPDX-License-Identifier: BUSL-1.1
#include "app/main_window.hpp"
#include "dialogs/welcome_dialog.hpp"
#include "irida/irida.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    WelcomeDialog welcome;
    if (welcome.exec() != QDialog::Accepted)
        return 0; // no target chosen

    QString path = welcome.selectedPath();
    IridaSession* session = irida_session_create_file(path.toLocal8Bit().constData());
    if (!session) {
        QMessageBox::critical(nullptr, "Open failed", "Could not open or parse:\n" + path);
        return 1;
    }
    WelcomeDialog::addRecent(path);

    MainWindow w(session);
    w.setWindowTitle("Irida — " + path);
    w.show();
    return app.exec();
}
