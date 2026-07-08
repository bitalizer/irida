// SPDX-License-Identifier: BUSL-1.1
#include "irida/mock/mock_backend.h"
#include "main_window.hpp"
#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    IridaSession* session = irida_mock_create();
    MainWindow w(session);
    w.show();
    int rc = app.exec();
    irida_session_destroy(session);
    return rc;
}
