// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QPixmap>
#include <QWidget>
#include <cstdint>
#include <vector>
class DebugController;

// One vertical byte-map strip: the whole image laid out as a 2D grid where
// moving down advances the address quickly (each row spans many bytes) and
// moving right advances it finely (columns divide a row). Each cell is colored
// by one of several schemes so the program's shape reads at a glance. A shared
// crosshair (owned by the parent ByteMapView) marks the hovered address.
class ByteMapStrip : public QWidget {
    Q_OBJECT
  public:
    enum class Mode {
        SectionTexture, // section color, darkened toward black for low bytes
        Content,        // by byte class: zero / code / ascii / other
        Entropy,        // by variety of the bytes in the cell's run
    };

    ByteMapStrip(DebugController* controller, Mode mode, QWidget* parent = nullptr);

    void refresh();
    // Address range and per-row byte span, set by the parent.
    void setRange(uint64_t start, uint64_t end);

    // Maps a point in this strip to an address (0 if outside the image).
    uint64_t addressAt(const QPoint& pos) const;

  signals:
    void picked(uint64_t addr); // mouse pressed on address

  protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

  private:
    int bytesPerRow() const;
    QColor colorForRun(const uint8_t* bytes, int len, uint64_t addr) const;
    // Renders the whole byte-map once into cache_. Rebuilt only when the range or
    // size changes, so hovering (which only moves the crosshair) stays cheap.
    void rebuildCache();

    DebugController* controller_;
    Mode mode_;
    uint64_t start_ = 0;
    uint64_t end_ = 0;
    QPixmap cache_;         // pre-rendered byte-map; blitted each paint
    QPoint pinned_{-1, -1}; // crosshair placed by a click, or (-1,-1)
    bool dragging_ = false; // left button held: the crosshair follows the mouse
    int cols_ = 24;         // fine byte columns across the strip
};
