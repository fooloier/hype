#include "math.h"
#include <QAbstractTextDocumentLayout>
#include <QFontMetricsF>
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>

namespace {
constexpr int objectType = QTextFormat::UserObject + 1;
constexpr int latexProperty = QTextFormat::UserProperty + 1;
constexpr int modeProperty = QTextFormat::UserProperty + 2;
constexpr int sizeProperty = QTextFormat::UserProperty + 3;

bool escaped(const QString &source, int index) {
    int slashes = 0;
    while (index > 0 && source[--index] == '\\') ++slashes;
    return slashes % 2;
}
int closingDollar(const QString &source, int index) {
    const bool display = source.mid(index, 2) == "$$";
    const int start = index + (display ? 2 : 1);
    if (start >= source.size() || (!display && source[start].isSpace())) return -1;
    int braces = 0, textGroup = 0;
    for (int end = start; end < source.size(); ++end) {
        if (source[end].isNull() || (!display && (source[end] == '\n' || source[end] == '\r'))) return -1;
        if (display && source[end] == '\n') {
            int next = end + 1;
            while (next < source.size() && (source[next] == ' ' || source[next] == '\t' || source[next] == '\r')) ++next;
            if (next < source.size() && source[next] == '\n') return -1;
        }
        if (escaped(source, end)) continue;
        if (source[end] == '{') {
            ++braces;
            static const QRegularExpression textCommand(R"(\\(?:text[a-zA-Z]*|mbox|hbox)\s*$)");
            if (!textGroup && textCommand.match(source.mid(start, end - start)).hasMatch()) textGroup = braces;
        } else if (source[end] == '}') {
            if (braces == textGroup) textGroup = 0;
            braces = qMax(0, braces - 1);
        }
        if (source[end] != '$' || textGroup) continue;
        if (display) {
            if (end + 1 < source.size() && source[end + 1] == '$' && end > start) return end;
        } else if (end > start && !source[end - 1].isSpace() &&
                   (end + 1 == source.size() || (!source[end + 1].isDigit() && source[end + 1] != '$')))
            return end;
    }
    return -1;
}
QString protectedSource(QString source) {
    auto mask = [&](int start, int length) {
        for (int i = start; i < start + length; ++i)
            if (source[i] != '\n' && source[i] != '\r') source[i] = QChar(0);
    };
    static const QRegularExpression fence("^ {0,3}(`{3,}|~{3,})(.*)$");
    QChar marker;
    int run = 0, listIndent = -1;
    for (int i = 0; i < source.size(); ++i) {
        if (i == 0 || source[i - 1] == '\n') {
            int end = source.indexOf('\n', i);
            if (end < 0) end = source.size();
            const QString line = source.mid(i, end - i);
            static const QRegularExpression list(R"(^( *)(?:[-+*]|\d+[.)]) +)");
            const auto item = list.match(line);
            int indent = 0;
            while (indent < line.size() && line[indent] == ' ') ++indent;
            if (item.hasMatch()) listIndent = item.capturedLength();
            else if (!line.trimmed().isEmpty() && indent < listIndent) listIndent = -1;
            const bool indentedCode = (line.startsWith("    ") || line.startsWith('\t')) &&
                                      (listIndent < 0 || indent >= listIndent + 4);
            const auto match = fence.match(line);
            const bool inside = run > 0;
            if (match.hasMatch()) {
                const QString found = match.captured(1);
                if (!run) { marker = found[0]; run = found.size(); }
                else if (found[0] == marker && found.size() >= run && match.captured(2).trimmed().isEmpty()) run = 0;
            }
            if (inside || match.hasMatch() || indentedCode) {
                mask(i, end - i);
                i = end;
                continue;
            }
        }
        if (source.mid(i, 4) == "<!--") {
            int end = source.indexOf("-->", i + 4);
            if (end < 0) end = source.size() - 3;
            mask(i, end + 3 - i);
            i = end + 2;
        } else if (source[i] == '`' && !escaped(source, i)) {
            int count = 1;
            while (i + count < source.size() && source[i + count] == '`') ++count;
            int end = i + count;
            while ((end = source.indexOf(QString(count, '`'), end)) >= 0) {
                if ((end == 0 || source[end - 1] != '`') &&
                    (end + count == source.size() || source[end + count] != '`')) break;
                end += count;
            }
            if (end >= 0) { mask(i, end + count - i); i = end + count - 1; }
            else i += count - 1;
        } else if (source[i] == ']' && i + 1 < source.size() && source[i + 1] == '(') {
            int depth = 1, end = i + 2;
            for (; end < source.size() && depth; ++end) {
                if (escaped(source, end)) continue;
                if (source[end] == '(') ++depth;
                if (source[end] == ')') --depth;
            }
            mask(i + 1, end - i - 1);
            i = end - 1;
        } else if (source[i] == '$' && !escaped(source, i)) {
            const bool display = source.mid(i, 2) == "$$";
            const int end = closingDollar(source, i);
            if (end >= 0) i = end + (display ? 1 : 0);
            else if (display) ++i;
        } else if (source[i] == '<') {
            static const QRegularExpression htmlOrLink("^<(?:[A-Za-z][A-Za-z0-9+.-]*:|/?[A-Za-z][A-Za-z0-9-]*(?:\\s|/?>)|[^<>\\s]+@)");
            const int end = source.indexOf('>', i + 1);
            if (end >= 0 && htmlOrLink.match(source.mid(i)).hasMatch()) { mask(i, end + 1 - i); i = end; }
        }
    }
    static const QRegularExpression reference("^ {0,3}\\[[^\\]\n]+\\]:[^\n]*", QRegularExpression::MultilineOption);
    const auto matches = reference.globalMatch(source);
    for (const auto &match : matches) mask(match.capturedStart(), match.capturedLength());
    return source;
}
MathMode modeOf(const QTextFormat &format) {
    return MathMode(format.property(modeProperty).toInt());
}
qreal pixelsOf(const QTextFormat &format) { return format.property(sizeProperty).toReal(); }
}

QVector<MathSpan> scanMath(const QString &source) {
    const QString visible = protectedSource(source);
    QVector<MathSpan> spans;
    for (int i = 0; i < visible.size(); ++i) {
        if (visible[i] != '$' || escaped(source, i)) continue;
        const bool display = i + 1 < visible.size() && visible[i + 1] == '$';
        const int delimiter = display ? 2 : 1;
        const int start = i + delimiter;
        if (start >= visible.size() || (!display && visible[start].isSpace())) {
            i += delimiter - 1;
            continue;
        }
        const int end = closingDollar(visible, i);
        if (end >= 0) {
            spans.append({{i, end + delimiter - i}, source.mid(start, end - start),
                          display ? MathMode::Display : MathMode::Inline});
            i = end + delimiter - 1;
        } else if (display) i += 1;
    }
    return spans;
}

QString maskMath(QString source) {
    for (const auto &span : scanMath(source))
        for (int i = span.range.start; i < span.range.start + span.range.length; ++i)
            if (source[i] != '\n' && source[i] != '\r') source[i] = ' ';
    return source;
}

MathMarkdown extractMath(const QString &source) {
    MathMarkdown markdown{source, {}, scanMath(source)};
    QString prefix = "HYPEMATHTOKEN";
    while (source.contains(prefix)) prefix += 'X';
    for (int i = 0; i < markdown.spans.size(); ++i) markdown.tokens << prefix + QString::number(i) + 'Z';
    for (int i = markdown.spans.size() - 1; i >= 0; --i) {
        const auto &span = markdown.spans[i];
        const int lineStart = span.range.start == 0 ? 0 : source.lastIndexOf('\n', span.range.start - 1) + 1;
        int lineEnd = source.indexOf('\n', span.range.start);
        if (lineEnd < 0) lineEnd = source.size();
        const QString line = source.mid(lineStart, lineEnd - lineStart);
        static const QRegularExpression container(R"(^(?:\s|>|[-+*] |\d+[.)] |#{1,6} ))");
        const bool contained = line.contains('|') || container.match(line).hasMatch();
        const QString token = span.mode == MathMode::Display && !contained
            ? "\n\n" + markdown.tokens[i] + "\n\n" : markdown.tokens[i];
        markdown.source.replace(span.range.start, span.range.length, token);
    }
    return markdown;
}

void insertMath(QTextDocument &document, const MathMarkdown &markdown) {
    if (markdown.spans.isEmpty()) return;
    auto handler = new MathObject(document.documentLayout());
    document.documentLayout()->registerHandler(objectType, handler);
    for (int i = 0; i < markdown.spans.size(); ++i) {
        QTextCursor cursor = document.find(markdown.tokens[i]);
        if (cursor.isNull()) continue;
        const auto &span = markdown.spans[i];
        const auto result = renderMath(span.latex, span.mode);
        QTextCharFormat format = cursor.charFormat();
        if (!result->error.isEmpty()) {
            const QString delimiter = span.mode == MathMode::Display ? "$$" : "$";
            cursor.insertText(delimiter + span.latex + delimiter, format);
            continue;
        }
        format.setObjectType(objectType);
        format.setProperty(latexProperty, span.latex);
        format.setProperty(modeProperty, int(span.mode));
        cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
    }
}

bool isDisplayMath(const QTextBlock &block) {
    if (block.text().trimmed() != QString(QChar::ObjectReplacementCharacter)) return false;
    for (auto it = block.begin(); !it.atEnd(); ++it) {
        const auto format = it.fragment().charFormat();
        if (format.objectType() == objectType && modeOf(format) == MathMode::Display) return true;
    }
    return false;
}

void sizeMath(QTextCharFormat &format, qreal pixels) {
    if (format.objectType() != objectType) return;
    format.setProperty(sizeProperty, pixels);
    const auto result = renderMath(format.property(latexProperty).toString(), modeOf(format));
    // AlignBaseline takes object descent from its font; this glyphless font reserves the TeX descent.
    const qreal descent = result->descent * pixels;
    QFont font = format.font();
    int low = 1, high = qMax(2, qCeil(pixels * 8));
    while (low < high) {
        const int middle = (low + high) / 2;
        font.setPixelSize(middle);
        if (QFontMetricsF(font).descent() < descent + 0.5) low = middle + 1;
        else high = middle;
    }
    format.setProperty(QTextFormat::FontPixelSize, low);
    format.setVerticalAlignment(QTextCharFormat::AlignBaseline);
    format.clearProperty(QTextFormat::TextBaselineOffset);
}

QSizeF MathObject::intrinsicSize(QTextDocument *, int, const QTextFormat &format) {
    const auto result = renderMath(format.property(latexProperty).toString(), modeOf(format));
    const qreal pixels = pixelsOf(format);
    return QSizeF(result->width * pixels,
                  result->ascent * pixels + QFontMetricsF(format.toCharFormat().font()).descent());
}
void MathObject::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *, int,
                            const QTextFormat &format) {
    const auto result = renderMath(format.property(latexProperty).toString(), modeOf(format));
    painter->save();
    painter->translate(rect.topLeft());
    painter->scale(pixelsOf(format), pixelsOf(format));
    for (const auto &ink : result->drawing)
        painter->fillPath(ink.path, ink.color == Qt::black ? format.toCharFormat().foreground() : QBrush(ink.color));
    painter->restore();
}
