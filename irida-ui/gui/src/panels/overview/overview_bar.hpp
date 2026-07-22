// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QWidget>
#include <cstdint>
#include <vector>
class DebugController;

// A horizontal byte-map of the whole program: each section is a colored band,
// widths compressed so tiny sections stay visible next to large ones, each a
// distinct cycling color. A marker tracks the current address; clicking or
// dragging navigates to the address under the cursor.
class OverviewBar : public QWidget {
    Q_OBJECT
  public:
    explicit OverviewBar(DebugController* controller, QWidget* parent = nullptr);

    QSize sizeHint() const override;

  public slots:
    void refresh();
    void setCurrent(uint64_t addr);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

  private:
    struct Band {
        uint64_t vaddr;
        uint64_t vsize;
        // Visual span along the bar. Widths are compressed (not linear in byte
        // size) so a tiny section stays visible and clickable next to a huge
        // one; address math still uses vaddr/vsize, so navigation is exact.
        double displayStart;
        double displaySpan;
        int colorIndex;
        QString name;
    };

    uint64_t addressAt(int x) const;
    int xForAddress(uint64_t addr) const;
    const Band* bandForAddress(uint64_t addr) const;
    void navigateToX(int x);

    DebugController* controller_;
    std::vector<Band> bands_;
    double displayTotal_ = 0; // summed visual span of all bands
    uint64_t current_ = 0;
};
