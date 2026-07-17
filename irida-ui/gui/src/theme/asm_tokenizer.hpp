// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include <QString>
#include <QVector>

// Splits a formatted x86-64 instruction string ("mov rcx, [rax+8]") into typed
// tokens so a delegate can paint each in its own theme color. Pure text logic —
// no Qt widgets, no ABI — so it is fully unit-testable.
namespace theme {

enum class TokenKind {
    Mnemonic,    // first word: mov, lea, call, jmp, ...
    Register,    // rax, ecx, r15, xmm0, rip, ...
    Number,      // 0x2f1c, 8, immediates / displacements
    Brace,       // [ ]  (memory operand delimiters)
    Punctuation, // , + * : -
    Whitespace,  // runs of spaces (preserved so layout is exact)
    Text         // anything else
};

struct Token {
    QString text;
    TokenKind kind;
};

// Tokenize one instruction line. Concatenating token.text reproduces the input
// exactly (whitespace preserved).
QVector<Token> tokenize(const QString& instruction);

} // namespace theme
