#include "math.h"
#include "core/formula.h"
#include "core/parser.h"
#include "latex.h"
#include "platform/qt/graphic_qt.h"
#include <QCache>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPathStroker>
#include <QRegularExpression>
#include <exception>

namespace {
class VectorEngine : public QPaintEngine {
  public:
    MathDrawing &result;
    explicit VectorEngine(MathDrawing &drawing) : QPaintEngine(AllFeatures), result(drawing) {}
    bool begin(QPaintDevice *) override { return true; }
    bool end() override { return true; }
    Type type() const override { return User; }
    void updateState(const QPaintEngineState &) override {}
    void add(const QPainterPath &path, const QColor &color) {
        if (color.alpha()) result.drawing.append({state->transform().map(path), color});
    }
    void drawPath(const QPainterPath &path) override {
        if (state->brush().style() != Qt::NoBrush) add(path, state->brush().color());
        const QPen pen = state->pen();
        if (pen.style() != Qt::NoPen) {
            QPainterPathStroker stroke;
            stroke.setWidth(pen.widthF());
            stroke.setCapStyle(pen.capStyle());
            stroke.setJoinStyle(pen.joinStyle());
            add(stroke.createStroke(path), pen.color());
        }
    }
    void drawPolygon(const QPointF *points, int count, PolygonDrawMode mode) override {
        QPainterPath path;
        if (!count) return;
        path.moveTo(points[0]);
        for (int i = 1; i < count; ++i) path.lineTo(points[i]);
        if (mode != PolylineMode) path.closeSubpath();
        path.setFillRule(mode == WindingMode ? Qt::WindingFill : Qt::OddEvenFill);
        drawPath(path);
    }
    void drawLines(const QLineF *lines, int count) override {
        QPainterPath path;
        for (int i = 0; i < count; ++i) {
            path.moveTo(lines[i].p1());
            path.lineTo(lines[i].p2());
        }
        drawPath(path);
    }
    void drawLines(const QLine *lines, int count) override {
        for (int i = 0; i < count; ++i) {
            const QLineF line(lines[i]);
            drawLines(&line, 1);
        }
    }
    void drawRects(const QRectF *rects, int count) override {
        for (int i = 0; i < count; ++i) {
            QPainterPath path;
            path.addRect(rects[i]);
            drawPath(path);
        }
    }
    void drawRects(const QRect *rects, int count) override {
        for (int i = 0; i < count; ++i) {
            const QRectF rect(rects[i]);
            drawRects(&rect, 1);
        }
    }
    void drawEllipse(const QRectF &rect) override {
        QPainterPath path;
        path.addEllipse(rect);
        drawPath(path);
    }
    void drawEllipse(const QRect &rect) override { drawEllipse(QRectF(rect)); }
    void drawPixmap(const QRectF &, const QPixmap &, const QRectF &) override {
        result.error = "Images in mathematics are not supported";
    }
    void drawTextItem(const QPointF &point, const QTextItem &item) override {
        QFont font = item.font();
        // TeX coordinates use em units. Outlines avoid point-to-device conversion on replay.
        const qreal pixels = font.pixelSize() > 0 ? font.pixelSize() : font.pointSizeF();
        font.setPixelSize(qMax(1, qRound(pixels * 64)));
        QPainterPath glyphs;
        glyphs.addText(QPointF(), font, item.text());
        QTransform transform;
        transform.translate(point.x(), point.y());
        transform.scale(1.0 / 64, 1.0 / 64);
        add(transform.map(glyphs), state->pen().color());
    }
};
class VectorDevice : public QPaintDevice {
    VectorEngine engine;
  public:
    explicit VectorDevice(MathDrawing &drawing) : engine(drawing) {}
    QPaintEngine *paintEngine() const override { return const_cast<VectorEngine *>(&engine); }
  protected:
    int metric(PaintDeviceMetric metric) const override {
        switch (metric) {
        case PdmDpiX: case PdmDpiY: case PdmPhysicalDpiX: case PdmPhysicalDpiY: return 72;
        case PdmDevicePixelRatio: return 1;
        case PdmDevicePixelRatioScaled: return int(devicePixelRatioFScale());
        case PdmDepth: return 32;
        default: return 4096;
        }
    }
};
QString resources() {
    const QString app = QCoreApplication::applicationDirPath();
    for (const QString &path : {app + "/math-res", app + "/../math-res",
                               app + "/../share/hype/math"})
        if (QFileInfo::exists(path + "/.clatexmath-res_root")) return path;
    return {};
}
}

std::shared_ptr<const MathDrawing> renderMath(const QString &latex, MathMode mode) noexcept {
    try {
        static QMutex mutex;
        static QCache<QString, std::shared_ptr<const MathDrawing>> cache(16 * 1024);
        QMutexLocker lock(&mutex);
        const QString key = QString::number(int(mode)) + ':' + latex;
        if (auto cached = cache.object(key)) return *cached;
        auto result = std::make_shared<MathDrawing>();
        try {
            static bool initialized = false;
            if (!initialized) {
                const auto root = resources();
                if (root.isEmpty()) throw std::runtime_error("Mathematics resources are missing");
                tex::LaTeX::init(root.toStdString());
                initialized = true;
            }
            // Macro/font definitions mutate process-wide engine state and invalidate cached formulas.
            static const QRegularExpression mutating(
                R"(\\(?:newcommand|renewcommand|newenvironment|renewenvironment|def|gdef|let|DeclareMathSizes|DeclareMathOperator|magnification|textfont|definecolor|newfont|externalFont)\b)");
            if (mutating.match(latex).hasMatch())
                throw std::runtime_error("Global TeX definitions are not supported");
            int groups = 0;
            bool comment = false;
            for (int i = 0; i < latex.size(); ++i) {
                const QChar c = latex[i];
                if (c == '\n') comment = false;
                if (comment) continue;
                if (c == '\\') { ++i; continue; }
                if (c == '%') { comment = true; continue; }
                if (c == '{') ++groups;
                if (c == '}' && --groups < 0) break;
            }
            if (groups != 0) throw std::runtime_error("Unbalanced TeX braces");
            tex::Formula formula;
            tex::TeXParser parser(false, latex.toStdWString(), &formula);
            parser.parse();
            tex::TeXRenderBuilder builder;
            std::unique_ptr<tex::TeXRender> render(builder
                .setStyle(mode == MathMode::Display ? tex::TexStyle::display : tex::TexStyle::text)
                .setTextSize(256).setTrueValues(true).build(formula));
            VectorDevice device(*result);
            QPainter painter(&device);
            tex::Graphics2D_qt graphics(&painter);
            render->draw(graphics, 0, 0);
            painter.end();
            const qreal height = render->getHeight();
            result->ascent = height * render->getBaseline();
            result->descent = height - result->ascent;
            result->width = render->getWidth();
            QRectF ink;
            for (const auto &part : result->drawing) ink |= part.path.boundingRect();
            const qreal left = qMin(0.0, ink.left());
            const qreal top = qMin(0.0, ink.top());
            result->width = qMax(result->width, ink.right()) - left;
            result->ascent -= top;
            result->descent += qMax(0.0, ink.bottom() - height);
            QTransform normal;
            normal.scale(1.0 / 256, 1.0 / 256);
            normal.translate(-left, -top);
            for (auto &part : result->drawing) part.path = normal.map(part.path);
            result->width /= 256;
            result->ascent /= 256;
            result->descent /= 256;
        } catch (const std::exception &error) {
            result->drawing.clear();
            result->error = QString::fromUtf8(error.what());
        } catch (...) {
            result->drawing.clear();
            result->error = "Unknown mathematics error";
        }
        qsizetype bytes = sizeof(MathDrawing) + latex.size() * 2;
        for (const auto &ink : result->drawing) bytes += ink.path.elementCount() * 32;
        cache.insert(key, new std::shared_ptr<const MathDrawing>(result), qMax(1, int(bytes / 1024)));
        return result;
    } catch (...) {
        static const auto failure = [] {
            auto result = std::make_shared<MathDrawing>();
            result->error = "Cannot allocate mathematics drawing";
            return result;
        }();
        return failure;
    }
}
