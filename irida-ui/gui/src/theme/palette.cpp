// SPDX-License-Identifier: BUSL-1.1
#include "theme/palette.hpp"

// A single dark palette. Values are chosen for good contrast on a near-black
// background and to separate token kinds at a glance (the way a modern IDE /
// disassembler colors code). One place to retune the whole tool.
namespace theme {

// --- surfaces ---
QColor background() {
    return QColor(0x1e, 0x20, 0x24);
}
QColor gridLine() {
    return QColor(0x2c, 0x2f, 0x35);
}
QColor headerBackground() {
    return QColor(0x26, 0x29, 0x2f);
}
QColor headerText() {
    return QColor(0x9a, 0xa0, 0xac);
}

// --- row states ---
QColor currentIpBackground() {
    return QColor(0x2b, 0x3d, 0x2b); // dim green wash, like an IDE's exec line
}
QColor currentIpMarker() {
    return QColor(0x5c, 0xd6, 0x5c); // bright green arrow
}
QColor breakpointBackground() {
    return QColor(0x3a, 0x22, 0x22); // dim red wash
}
QColor breakpointDot() {
    return QColor(0xe0, 0x3c, 0x3c); // solid red dot (IDE convention)
}
QColor selectionBackground() {
    return QColor(0x2f, 0x3a, 0x4c);
}
QColor rspMarker() {
    return QColor(0x6f, 0xa8, 0xe0);
}

// --- text / tokens ---
QColor defaultText() {
    return QColor(0xd6, 0xd9, 0xdf);
}
QColor address() {
    return QColor(0x7f, 0x88, 0x96); // muted grey-blue; addresses recede
}
QColor mnemonic() {
    return QColor(0x6f, 0xb4, 0xe6); // blue
}
QColor registerName() {
    return QColor(0xe0, 0x9a, 0x5a); // amber
}
QColor number() {
    return QColor(0xb9, 0x8e, 0xe0); // violet
}
QColor punctuation() {
    return QColor(0x8a, 0x90, 0x9c);
}
QColor memoryBrace() {
    return QColor(0x6f, 0xc4, 0x9a); // teal for [ ]
}
QColor jumpTarget() {
    return QColor(0x9a, 0xd0, 0x6f); // green-ish call/jump target
}
QColor liveValue() {
    return QColor(0x9a, 0xd0, 0x6f); // resolved meaning stands out (green)
}
QColor changedValue() {
    return QColor(0xe6, 0x64, 0x64); // changed register value: red
}
QColor bytesText() {
    return QColor(0x76, 0x7c, 0x88); // raw bytes recede
}

// --- CFG graph view ---
QColor graphBackground() {
    return QColor(0x17, 0x18, 0x1c); // slightly darker than panels, canvas feel
}
QColor graphBlockBackground() {
    return QColor(0x24, 0x27, 0x2d);
}
QColor graphBlockBorder() {
    return QColor(0x3a, 0x3e, 0x46);
}
QColor graphEntryBorder() {
    return QColor(0x6f, 0xb4, 0xe6); // blue, matches mnemonic accent
}
QColor graphEdgeTaken() {
    return QColor(0x5c, 0xd6, 0x5c); // green taken branch
}
QColor graphEdgeFail() {
    return QColor(0xe0, 0x3c, 0x3c); // red fall-through
}
QColor graphEdgeUncond() {
    return QColor(0x7f, 0x88, 0x96); // muted grey-blue for a single edge
}

} // namespace theme
