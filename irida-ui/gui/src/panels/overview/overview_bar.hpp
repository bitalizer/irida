// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "irida/irida.h"
#include <QWidget>
#include <cstdint>
#include <vector>
class DebugController;

// A horizontal byte-map of the whole program: each section is a colored band
// sized in proportion to its address range, colored by permissions. A marker
// tracks the current address; clicking or dragging navigates to the address
// under the cursor.
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
        uint64_t packedStart; // offset of this band along the packed bar
        QString name;
    };

    uint64_t addressAt(int x) const;
    int xForAddress(uint64_t addr) const;
    const Band* bandForAddress(uint64_t addr) const;
    void navigateToX(int x);

    DebugController* controller_;
    std::vector<Band> bands_;
    uint64_t total_ = 0; // summed size of all bands
    uint64_t current_ = 0;
};
