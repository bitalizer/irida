// SPDX-License-Identifier: BUSL-1.1
#include "dialogs/welcome_dialog.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace {
constexpr int kMaxRecent = 10;
const char* kRecentKey = "recentFiles";
} // namespace

WelcomeDialog::WelcomeDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Irida");
    setMinimumSize(560, 420);

    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("Irida", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto* subtitle = new QLabel("Open a binary to analyze.", this);
    subtitle->setEnabled(false);
    layout->addWidget(subtitle);

    auto* recentLabel = new QLabel("Recent", this);
    layout->addSpacing(8);
    layout->addWidget(recentLabel);

    recent_ = new QListWidget(this);
    layout->addWidget(recent_, 1);
    connect(recent_, &QListWidget::itemActivated, this, &WelcomeDialog::onRecentActivated);
    populateRecent();

    auto* openButton = new QPushButton("Open File...", this);
    openButton->setDefault(true);
    connect(openButton, &QPushButton::clicked, this, &WelcomeDialog::openFile);
    layout->addWidget(openButton);
}

void WelcomeDialog::populateRecent() {
    recent_->clear();
    const QStringList files = recentFiles();
    for (const QString& path : files) {
        QFileInfo info(path);
        auto* item = new QListWidgetItem(info.fileName() + "  —  " + info.absolutePath(), recent_);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
    }
    if (files.isEmpty()) {
        auto* item = new QListWidgetItem("No recent files", recent_);
        item->setFlags(Qt::NoItemFlags);
    }
}

void WelcomeDialog::openFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Binary", QString(), "Executables (*.exe *.dll *.sys *.bin);;All Files (*)");
    if (path.isEmpty())
        return;
    selected_path_ = path;
    accept();
}

void WelcomeDialog::onRecentActivated(QListWidgetItem* item) {
    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty())
        return;
    selected_path_ = path;
    accept();
}

QStringList WelcomeDialog::recentFiles() {
    QSettings s("Irida", "Irida");
    return s.value(kRecentKey).toStringList();
}

void WelcomeDialog::addRecent(const QString& path) {
    if (path.isEmpty())
        return;
    QSettings s("Irida", "Irida");
    QStringList files = s.value(kRecentKey).toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxRecent)
        files.removeLast();
    s.setValue(kRecentKey, files);
}
