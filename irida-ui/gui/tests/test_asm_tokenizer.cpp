// SPDX-License-Identifier: BUSL-1.1
#include "theme/asm_tokenizer.hpp"
#include <QtTest/QtTest>

using theme::TokenKind;

namespace {
// Concatenating all token texts must reproduce the input exactly.
QString rejoin(const QVector<theme::Token>& toks) {
    QString s;
    for (const auto& t : toks)
        s += t.text;
    return s;
}
// Find the kind of the first token whose text equals `text`.
TokenKind kindOf(const QVector<theme::Token>& toks, const QString& text) {
    for (const auto& t : toks)
        if (t.text == text)
            return t.kind;
    return TokenKind::Text;
}
} // namespace

class TestAsmTokenizer : public QObject {
    Q_OBJECT
  private slots:
    void roundTripsInputExactly() {
        const QString in = "mov rcx, [rax+8]";
        QCOMPARE(rejoin(theme::tokenize(in)), in);
    }

    void classifiesMnemonicRegistersNumbersBraces() {
        auto toks = theme::tokenize("mov rcx, [rax+8]");
        QCOMPARE(kindOf(toks, "mov"), TokenKind::Mnemonic);
        QCOMPARE(kindOf(toks, "rcx"), TokenKind::Register);
        QCOMPARE(kindOf(toks, "rax"), TokenKind::Register);
        QCOMPARE(kindOf(toks, "8"), TokenKind::Number);
        QCOMPARE(kindOf(toks, "["), TokenKind::Brace);
        QCOMPARE(kindOf(toks, "]"), TokenKind::Brace);
        QCOMPARE(kindOf(toks, ","), TokenKind::Punctuation);
        QCOMPARE(kindOf(toks, "+"), TokenKind::Punctuation);
    }

    void firstWordIsAlwaysMnemonic() {
        // Even where the first word looks like something else, it's the mnemonic.
        auto toks = theme::tokenize("ret");
        QCOMPARE(toks.size(), 1);
        QCOMPARE(toks.front().kind, TokenKind::Mnemonic);
    }

    void hexImmediateIsNumber() {
        auto toks = theme::tokenize("lea rdx, [rip+0x2f1c]");
        QCOMPARE(kindOf(toks, "0x2f1c"), TokenKind::Number);
        QCOMPARE(kindOf(toks, "rip"), TokenKind::Register);
    }

    void emptyStringYieldsNoTokens() {
        QVERIFY(theme::tokenize("").isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestAsmTokenizer)
#include "test_asm_tokenizer.moc"
