// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "panels/overview/byte_map_strip.hpp"
#include <QWidget>
#include <cstdint>
class DebugController;

// The vertical byte-map beside the code view: a single strip coloring the whole
// image by section, textured by byte content so padding and data read as speckle
// over each section's hue. A crosshair follows the mouse while hovering; clicking
// navigates the session to that address.
class ByteMapView : public QWidget {
    Q_OBJECT
  public:
    explicit ByteMapView(DebugController* controller, QWidget* parent = nullptr);

  public slots:
    void refresh();

  protected:
    void resizeEvent(QResizeEvent* event) override;

  private:
    void computeRange();

    DebugController* controller_;
    ByteMapStrip* strip_;
    uint64_t start_ = 0;
    uint64_t end_ = 0;
};
