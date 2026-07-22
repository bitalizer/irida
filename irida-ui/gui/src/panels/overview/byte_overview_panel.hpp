// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QAbstractScrollArea>
#include <cstdint>
class DebugController;

// A dense "shape of the file" view: one glyph per byte across a wide grid, with
// an address gutter down the left. Non-printable bytes render as compact symbols
// so runs of code, padding, and text are visually distinct. Clicking a byte
// navigates the session there; the grid scrolls through the whole image.
class ByteOverviewPanel : public QAbstractScrollArea {
    Q_OBJECT
  public:
    explicit ByteOverviewPanel(DebugController* controller, QWidget* parent = nullptr);

  public slots:
    void refresh();
    void setBase(uint64_t addr);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

  private:
    int columns() const; // bytes per row for the current width
    int visibleRows() const;
    void updateScrollRange();
    uint64_t addressAt(const QPoint& pos) const;

    DebugController* controller_;
    uint64_t start_ = 0; // first mapped address
    uint64_t end_ = 0;   // one past the last mapped address
    uint64_t top_ = 0;   // address of the first visible row
    int cellW_ = 8;
    int cellH_ = 14;
    int gutterW_ = 0;
};
