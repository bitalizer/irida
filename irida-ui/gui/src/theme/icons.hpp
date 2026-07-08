// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QIcon>
#include <QString>

namespace icons {
// Loads :/icons/<name>.svg, tinted to the application's text color. Returns a
// null QIcon (no crash) if the resource is missing.
QIcon load(const QString& name);
} // namespace icons
