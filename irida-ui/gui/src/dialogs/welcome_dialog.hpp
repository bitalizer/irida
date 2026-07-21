// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QDialog>
#include <QString>
class QListWidget;
class QListWidgetItem;

// The landing window shown at launch: a recent-files list plus an Open File
// action. Attaching to a process is not offered here — it lives in the File
// menu once a session is open. selectedPath() is the binary the user chose, or
// empty if they cancelled.
class WelcomeDialog : public QDialog {
    Q_OBJECT
  public:
    explicit WelcomeDialog(QWidget* parent = nullptr);

    QString selectedPath() const {
        return selected_path_;
    }

    // Adds a path to the persisted recent-files list (most-recent first, capped).
    static void addRecent(const QString& path);
    static QStringList recentFiles();

  private slots:
    void openFile();
    void onRecentActivated(QListWidgetItem* item);

  private:
    void populateRecent();

    QListWidget* recent_;
    QString selected_path_;
};
