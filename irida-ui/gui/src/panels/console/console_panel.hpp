// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QWidget>

class QPlainTextEdit;
class DebugController;

class ConsolePanel : public QWidget {
    Q_OBJECT
  public:
    explicit ConsolePanel(DebugController* controller, QWidget* parent = nullptr);

  public slots:
    void appendLine(const QString& text);

  private slots:
    void onStateChanged();

  private:
    DebugController* controller_;
    QPlainTextEdit* output_;
};
