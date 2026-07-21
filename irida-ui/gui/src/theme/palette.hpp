// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QColor>

// Central Irida color palette. Every panel and delegate pulls its colors from
// here so the whole UI reads as one system and can be re-themed in one place.
// A dark, modern theme with its own identity.
namespace theme {

// --- surfaces ---
QColor background(); // panel base background
QColor gridLine();   // subtle row/column separators
QColor headerBackground();
QColor headerText();

// --- row states ---
QColor currentIpBackground();  // the instruction-pointer row
QColor currentIpMarker();      // the filled IP arrow
QColor breakpointBackground(); // a row carrying a breakpoint
QColor breakpointDot();        // the filled red breakpoint circle
QColor selectionBackground();
QColor rspMarker(); // stack RSP-slot marker

// --- text / tokens (disassembly + values) ---
QColor defaultText();
QColor address();      // code/data addresses
QColor mnemonic();     // mov, lea, call, jmp...
QColor registerName(); // rax, rcx, xmm0...
QColor number();       // immediates / displacements
QColor punctuation();  // , [ ] + * :
QColor memoryBrace();  // the [ ] of a memory operand
QColor jumpTarget();   // a branch/call target address
QColor liveValue();    // the resolved live-value / hint (our differentiator)
QColor changedValue(); // a register value that changed since the last stop
QColor bytesText();    // raw instruction bytes column

// --- CFG graph view ---
QColor graphBackground(); // graph canvas behind the blocks
QColor graphBlockBackground();
QColor graphBlockBorder();
QColor graphEntryBorder(); // highlight border on the entry block
QColor graphEdgeTaken();   // green: conditional taken branch
QColor graphEdgeFail();    // red: conditional fall-through
QColor graphEdgeUncond();  // neutral: single successor

QColor overviewCode();     // overview bar: executable section band
QColor overviewData();     // overview bar: writable data band
QColor overviewReadonly(); // overview bar: read-only band
QColor overviewMarker();   // overview bar: current-address marker

} // namespace theme
