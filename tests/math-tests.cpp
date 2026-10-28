#include "deck.h"
#include "math.h"
#include "renderer.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPdfDocument>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextLayout>
#include <QTextTable>
#include <QtConcurrentRun>
#include <QtTest>

class MathTests : public QObject {
    Q_OBJECT
    static QVariantMap palette() {
        return {{"foreground", "#161616"}, {"background", "#ffffff"},
                {"accent", "#dd3311"}, {"font", "DejaVu Sans"}};
    }
    static QImage documentImage(QTextDocument &doc) {
        QImage image(QSize(qCeil(doc.size().width()), qCeil(doc.size().height())), QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        QAbstractTextDocumentLayout::PaintContext context;
        doc.documentLayout()->draw(&painter, context);
        return image;
    }
    static QRect ink(const QImage &image, int left, int right) {
        QRect bounds;
        for (int y = 0; y < image.height(); ++y)
            for (int x = qMax(0, left); x < qMin(right, image.width()); ++x)
                if (image.pixelColor(x, y).lightness() < 180)
                    bounds |= QRect(x, y, 1, 1);
        return bounds;
    }
    static int objects(const QTextDocument &doc) {
        return doc.toPlainText().count(QChar::ObjectReplacementCharacter);
    }
  private slots:
    void inlineAndDisplay() {
        QTextDocument doc;
        layoutSlideText(doc, "English $x_i^2$ 中文\n\n$$\\frac{1}{2}$$\n\nAfter", palette(), 48, 900, false, false);
        QCOMPARE(objects(doc), 2);
        QVERIFY(!doc.toPlainText().contains("HYPEMATH"));
        int displays = 0;
        for (auto block = doc.begin(); block.isValid(); block = block.next())
            if (isDisplayMath(block)) {
                ++displays;
                QCOMPARE(block.blockFormat().alignment(), Qt::AlignHCenter);
                QCOMPARE(block.text(), QString(QChar::ObjectReplacementCharacter));
            }
        QCOMPARE(displays, 1);
        const auto text = renderMath("\\sum_{i=1}^{n} \\frac{1}{i}", MathMode::Inline);
        const auto display = renderMath("\\sum_{i=1}^{n} \\frac{1}{i}", MathMode::Display);
        QVERIFY2(text->error.isEmpty(), qPrintable(text->error));
        QVERIFY2(display->error.isEmpty(), qPrintable(display->error));
        QVERIFY(!text->drawing.isEmpty());
        QVERIFY(display->ascent + display->descent > text->ascent + text->descent);
        const auto fraction = renderMath("\\frac{x}{y}", MathMode::Inline);
        bool fractionBar = false;
        for (const auto &part : fraction->drawing) {
            const auto bounds = part.path.boundingRect();
            fractionBar |= bounds.width() > 0.3 && bounds.height() > 0 && bounds.height() < 0.08;
        }
        QVERIFY(fractionBar);
        layoutSlideText(doc, "before $$x+\ny$$ after $$z$$", palette(), 48, 900, false, false);
        QCOMPARE(objects(doc), 2);
        QCOMPARE(doc.blockCount(), 4);
    }
    void codeEscapesCurrency_data() {
        QTest::addColumn<QString>("source");
        QTest::addColumn<int>("count");
        QTest::newRow("ordinary") << "Use $x$ and $y_i$" << 2;
        QTest::newRow("escaped") << R"(Use \$x\$ and $y$)" << 1;
        QTest::newRow("currency") << "| Basic | $9 |\n| --- | --- |\n| Pro | $29 |" << 0;
        QTest::newRow("currency-inline") << "Costs $9 or $29 today" << 0;
        QTest::newRow("unclosed") << "unclosed $x\nnext y$" << 0;
        QTest::newRow("whitespace") << "$ x$ $x $ $x$2" << 0;
        QTest::newRow("fence") << "```tex\n$x$\n```\n~~~\n$$y$$\n~~~" << 0;
        QTest::newRow("inline-code") << "`$x$` ``a`$y$`` $z$" << 1;
        QTest::newRow("comment") << "<!-- $$x$$ --> $y$" << 1;
        QTest::newRow("target") << "[link](https://host/$x$) [ref][a]\n[a]: https://host/$z$\n$y$" << 1;
        QTest::newRow("nested-target") << "[link](https://host/a($x$)) $y$" << 1;
        QTest::newRow("autolink") << "<https://host/$x$> $y$" << 1;
        QTest::newRow("comparisons") << "Inline $x < y$ and $z > 0$ then $$x < y < z$$" << 3;
        QTest::newRow("formula-markup") << R"($$\text{![alt](fake.png)}$$ $$\text{<!-- not a note -->}$$)" << 2;
        QTest::newRow("token-collision") << "HYPEMATHTOKEN0Z $x$" << 1;
        QTest::newRow("display-indented-content") << "$$\n    x + y = z\n$$" << 1;
        QTest::newRow("display-tab-content") << "$$\n\tx + y = z\n$$" << 1;
        QTest::newRow("indented-code") << "    $x$\n\t$$y$$" << 0;
        QTest::newRow("unclosed-display") << "$$unclosed\n\n# Heading\n\nInline $x+y$ here.\n\n$$" << 1;
        QTest::newRow("nested-text-math") << R"(Value $\text{for $x>0$}$ works.)" << 1;
    }
    void codeEscapesCurrency() {
        QFETCH(QString, source);
        QFETCH(int, count);
        QTextDocument doc;
        layoutSlideText(doc, source, palette(), 32, 900, false, false);
        QCOMPARE(objects(doc), count);
        QVERIFY(slideProblems(source, ".").isEmpty());
    }
    void tableAndTheme() {
        QTextDocument doc;
        layoutSlideText(doc, "| Formula | Price |\n| --- | ---: |\n| $\\lvert x_i * y \\rvert$ | $9 |\n| $a|b$ | $29 |", palette(), 48, 900, false, false);
        QCOMPARE(objects(doc), 2);
        QTextCursor cursor = doc.find("Formula");
        QVERIFY(cursor.currentTable());
        QCOMPARE(cursor.currentTable()->columns(), 2);
        QCOMPARE(cursor.currentTable()->rows(), 3);
        QVERIFY(doc.toPlainText().contains("$9"));
        auto dark = palette();
        dark["foreground"] = "#ffffff";
        dark["background"] = "#161616";
        QImage image(1920, 1080, QImage::Format_RGB32);
        QPainter painter(&image);
        paintSlide(&painter, image.rect(), "$$x_i$$", ".", dark);
        painter.end();
        int bright = 0;
        for (int y = 300; y < 780; ++y)
            for (int x = 300; x < 1620; ++x)
                if (image.pixelColor(x, y).lightness() > 230) ++bright;
        QVERIFY(bright > 100);
    }
    void displayContainers() {
        QTextDocument doc;
        layoutSlideText(doc, "| A | B |\n| --- | --- |\n| $$x$$ | $y$ |", palette(), 48, 900, false, false);
        QCOMPARE(objects(doc), 2);
        const auto table = doc.find("A").currentTable();
        QVERIFY(table);
        QCOMPARE(table->rows(), 2);
        QCOMPARE(table->columns(), 2);
        QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString(QChar::ObjectReplacementCharacter));
        layoutSlideText(doc, "- item $$x$$\n- next", palette(), 48, 900, false, false);
        const auto item = doc.find("item").block();
        QVERIFY(item.textList());
        QVERIFY(item.text().contains(QChar::ObjectReplacementCharacter));
        layoutSlideText(doc, "- item\n    $$a+b$$\n- next", palette(), 48, 900, false, false);
        QCOMPARE(objects(doc), 1);
        const auto nested = doc.find(QString(QChar::ObjectReplacementCharacter)).block();
        QVERIFY(nested.textList() || nested.blockFormat().indent() > 0);
        layoutSlideText(doc, "> $$x$$", palette(), 48, 900, false, false);
        QCOMPARE(objects(doc), 1);
        const auto formula = doc.find(QString(QChar::ObjectReplacementCharacter)).block();
        QVERIFY(formula.blockFormat().intProperty(QTextFormat::BlockQuoteLevel) > 0);
    }
    void baselineAndLineHeight() {
        QTextDocument doc;
        layoutSlideText(doc, "H $H$ H\nNext line", palette(), 64, 900, false, false);
        const auto image = documentImage(doc).copy(0, 0, 900, qCeil(doc.begin().layout()->lineAt(0).height()));
        const auto line = doc.begin().layout()->lineAt(0);
        const int position = doc.toPlainText().indexOf(QChar::ObjectReplacementCharacter);
        const qreal start = line.cursorToX(position), end = line.cursorToX(position + 1);
        const QRect prose = ink(image, 0, int(start));
        const QRect math = ink(image, int(start), qCeil(end));
        QVERIFY(!math.isEmpty());
        qInfo() << "baseline prose/math" << prose << math << "line" << line.ascent() << line.descent();
        QVERIFY2(qAbs(math.bottom() - prose.bottom()) <= 2,
                 qPrintable(QString("prose bottom %1, math bottom %2").arg(prose.bottom()).arg(math.bottom())));
        layoutSlideText(doc, "H $Hg$ H", palette(), 64, 900, false, false);
        const auto descenderImage = documentImage(doc);
        const auto descenderLine = doc.begin().layout()->lineAt(0);
        const int descenderPosition = doc.toPlainText().indexOf(QChar::ObjectReplacementCharacter);
        const qreal descenderStart = descenderLine.cursorToX(descenderPosition);
        const qreal descenderEnd = descenderLine.cursorToX(descenderPosition + 1);
        const QRect proseH = ink(descenderImage, 0, int(descenderStart));
        const QRect mathHg = ink(descenderImage, int(descenderStart), qCeil(descenderEnd));
        const auto hg = renderMath("Hg", MathMode::Inline);
        const qreal actualBaseline = mathHg.top() + hg->ascent * 64;
        const qreal proseBaseline = descenderLine.y() + descenderLine.ascent();
        qInfo() << "descender baseline" << actualBaseline << proseBaseline << mathHg << proseH;
        QVERIFY(qAbs(actualBaseline - proseBaseline) <= 3);
        QVERIFY(mathHg.bottom() > proseH.bottom() + 4);
        QVERIFY(mathHg.bottom() < descenderImage.height());
        layoutSlideText(doc, "H $\\frac{1}{\\frac{x}{y}}$ H\nSecond line\nThird line", palette(), 64, 900, false, false);
        const auto tall = documentImage(doc);
        for (auto block = doc.begin(); block.isValid(); block = block.next())
            QVERIFY(block.layout()->lineCount() >= 1);
        const auto first = doc.begin().layout()->lineAt(0);
        QVERIFY(doc.documentLayout()->blockBoundingRect(doc.begin().next()).top() >= first.height() - 1);
        QVERIFY(!ink(tall, 0, tall.width()).isEmpty());
        const auto drawing = renderMath("\\frac{1}{\\frac{x}{y}}", MathMode::Inline);
        QVERIFY(first.height() >= (drawing->ascent + drawing->descent) * 64 - 2);
    }
    void scalingAndConcurrentRendering() {
        const QString source = "H $x_i^2$\n\n$$\\int_0^1 x^2\\,dx=\\frac{1}{3}$$";
        QTextDocument small, large;
        layoutSlideText(small, source, palette(), 32, 900, false, false);
        layoutSlideText(large, source, palette(), 64, 1800, false, false);
        const QRect a = ink(documentImage(small), 0, 900);
        const QRect b = ink(documentImage(large), 0, 1800);
        QVERIFY(qAbs(b.height() / qreal(a.height()) - 2) < 0.12);
        QList<QFuture<QImage>> jobs;
        auto render = [=](int index) {
            QImage image(960, 540, QImage::Format_RGB32);
            image.setDotsPerMeterX(index % 2 ? 7560 : 3780);
            image.setDotsPerMeterY(index % 2 ? 7560 : 3780);
            QPainter painter(&image);
            paintSlide(&painter, image.rect(), source + QString(" $%1+x$").arg(index % 4), ".", palette());
            return image;
        };
        for (int i = 0; i < 16; ++i) jobs << QtConcurrent::run(render, i);
        for (int i = 0; i < jobs.size(); ++i) {
            const auto actual = jobs[i].result();
            QCOMPARE(actual, render(i));
        }
        QCOMPARE(render(0), render(4));
        QImage normal(960, 540, QImage::Format_RGB32), highDpi(normal.size(), normal.format());
        highDpi.setDotsPerMeterX(11339); highDpi.setDotsPerMeterY(11339);
        for (auto *image : {&normal, &highDpi}) {
            QPainter painter(image);
            paintSlide(&painter, image->rect(), "$$\\frac{x}{y}$$", ".", palette());
        }
        QCOMPARE(normal, highDpi);
    }
    void invalidFormulaAndMedia() {
        const QString source = R"(Bad $\unknownHypeMacro{x}$ and $\frac{1}{$)";
        QTextDocument doc;
        layoutSlideText(doc, source, palette(), 48, 900, false, false);
        QCOMPARE(doc.toPlainText(), source);
        QCOMPARE(slideProblems(source, ".").size(), 2);
        QVERIFY(slideProblems("Unclosed $x", ".").isEmpty());
        QVERIFY(slideProblems("<!-- $\\unknownMacro$ -->\n`$\\unknownMacro$`", ".").isEmpty());
        QCOMPARE(speakerNotes("$x$ <!-- note $y$ -->"), QString("note $y$"));
        const QString protectedMarkup = R"($$\text{![alt](fake.png)}$$ $$\text{<!-- not a note -->}$$)";
        QVERIFY(parseMedia(protectedMarkup, ".").file.isEmpty());
        QCOMPARE(parseMedia(protectedMarkup, ".").text, protectedMarkup);
        QVERIFY(speakerNotes(protectedMarkup).isEmpty());
        const auto media = parseMedia("$x$\n![](image.png)", ".");
        QCOMPARE(media.file, QString("image.png"));
        QVERIFY(media.text.contains("$x$"));
        Deck deck;
        deck.editSource(source);
        QTemporaryDir tmp;
        QVERIFY(!deck.exportPdf(tmp.filePath("bad.pdf")));
        QVERIFY(!QFile::exists(tmp.filePath("bad.pdf")));
    }
    void pngPdfPptx() {
        Deck deck;
        deck.editSource("English $E=mc^2$ 中文\n\n---\n\n$$\\frac{1}{2}+\\int_0^1 x\\,dx$$\n\n---\n\n$$\\begin{pmatrix}a&b\\\\c&d\\end{pmatrix}$$");
        QTemporaryDir tmp;
        QVERIFY(deck.renderImages(tmp.filePath("png")));
        const QImage png(tmp.filePath("png/slide-002.png"));
        QCOMPARE(png.size(), QSize(1920, 1080));
        QVERIFY(deck.exportPdf(tmp.filePath("math.pdf")));
        QFile pdfFile(tmp.filePath("math.pdf"));
        QVERIFY(pdfFile.open(QIODevice::ReadOnly));
        const auto bytes = pdfFile.readAll();
        QVERIFY(!bytes.contains("/Subtype /Image"));
        QPdfDocument pdf;
        QCOMPARE(pdf.load(tmp.filePath("math.pdf")), QPdfDocument::Error::None);
        QCOMPARE(pdf.pageCount(), 3);
        QVERIFY(!pdf.render(1, QSize(1920, 1080)).isNull());
        QVERIFY(deck.exportPptx(tmp.filePath("math.pptx")));
        QProcess unzip;
        unzip.start("unzip", {"-p", tmp.filePath("math.pptx"), "ppt/media/slide2.jpg"});
        QVERIFY(unzip.waitForFinished());
        const QImage slide = QImage::fromData(unzip.readAllStandardOutput());
        QVERIFY(!slide.isNull());
    }
};
QTEST_MAIN(MathTests)
#include "math-tests.moc"
