#pragma once

#include <QColor>
#include <QPainterPath>
#include <QString>
#include <QTextObjectInterface>
#include <QVector>
#include <memory>

class QTextDocument;

enum class MathMode { Inline, Display };
struct MathSpan {
    struct Range { int start; int length; } range;
    QString latex;
    MathMode mode;
};
struct MathDrawing {
    struct Ink { QPainterPath path; QColor color; };
    QVector<Ink> drawing;
    qreal width = 0;
    qreal ascent = 0;
    qreal descent = 0;
    QString error;
};

QVector<MathSpan> scanMath(const QString &source);
QString maskMath(QString source);
std::shared_ptr<const MathDrawing> renderMath(const QString &latex, MathMode mode) noexcept;

struct MathMarkdown {
    QString source;
    QStringList tokens;
    QVector<MathSpan> spans;
};
MathMarkdown extractMath(const QString &source);
void insertMath(QTextDocument &document, const MathMarkdown &markdown);
bool isDisplayMath(const QTextBlock &block);
void sizeMath(QTextCharFormat &format, qreal pixels);

class MathObject : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
  public:
    explicit MathObject(QObject *parent) : QObject(parent) {}
    QSizeF intrinsicSize(QTextDocument *, int, const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *, int,
                    const QTextFormat &format) override;
};
