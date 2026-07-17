// SPDX-License-Identifier: BUSL-1.1
#include "theme/asm_tokenizer.hpp"
#include <QChar>
#include <QSet>

namespace theme {
namespace {

// x86-64 register names (common subset). Lower-case compare; the formatter
// emits lower-case register names.
const QSet<QString>& registerNames() {
    static const QSet<QString> regs = {
        "rax",  "rbx",  "rcx",  "rdx", "rsi", "rdi", "rbp", "rsp",  "rip",  "r8",   "r9",   "r10",
        "r11",  "r12",  "r13",  "r14", "r15", "eax", "ebx", "ecx",  "edx",  "esi",  "edi",  "ebp",
        "esp",  "r8d",  "r9d",  "ax",  "bx",  "cx",  "dx",  "al",   "bl",   "cl",   "dl",   "ah",
        "bh",   "ch",   "dh",   "sil", "dil", "bpl", "spl", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4",
        "xmm5", "xmm6", "xmm7", "cs",  "ds",  "es",  "fs",  "gs",   "ss"};
    return regs;
}

bool isWordChar(QChar c) {
    return c.isLetterOrNumber() || c == '_';
}

bool looksLikeNumber(const QString& w) {
    if (w.isEmpty())
        return false;
    if (w.startsWith("0x") || w.startsWith("0X"))
        return true;
    // pure decimal
    for (QChar c : w)
        if (!c.isDigit())
            return false;
    return true;
}

} // namespace

QVector<Token> tokenize(const QString& instruction) {
    QVector<Token> out;
    const int n = instruction.size();
    int i = 0;
    bool seenMnemonic = false;

    while (i < n) {
        const QChar c = instruction.at(i);

        // whitespace run
        if (c.isSpace()) {
            int j = i;
            while (j < n && instruction.at(j).isSpace())
                ++j;
            out.push_back({instruction.mid(i, j - i), TokenKind::Whitespace});
            i = j;
            continue;
        }

        // braces
        if (c == '[' || c == ']') {
            out.push_back({QString(c), TokenKind::Brace});
            ++i;
            continue;
        }

        // punctuation
        if (c == ',' || c == '+' || c == '*' || c == ':' || c == '-') {
            out.push_back({QString(c), TokenKind::Punctuation});
            ++i;
            continue;
        }

        // word: letters/digits/underscore (covers mnemonics, registers, numbers)
        if (isWordChar(c)) {
            int j = i;
            while (j < n && isWordChar(instruction.at(j)))
                ++j;
            const QString w = instruction.mid(i, j - i);
            TokenKind kind;
            if (!seenMnemonic) {
                kind = TokenKind::Mnemonic;
                seenMnemonic = true;
            } else if (registerNames().contains(w.toLower())) {
                kind = TokenKind::Register;
            } else if (looksLikeNumber(w)) {
                kind = TokenKind::Number;
            } else {
                kind = TokenKind::Text;
            }
            out.push_back({w, kind});
            i = j;
            continue;
        }

        // anything else: single char as text
        out.push_back({QString(c), TokenKind::Text});
        ++i;
    }

    return out;
}

} // namespace theme
