#pragma once

#include <QHash>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QtSvg/QSvgRenderer>
#include <array>
#include <cstdint>
#include <optional>
#include <qstringview.h>
#include <qsvgrenderer.h>
#include <qtypes.h>
#include <qwindowdefs.h>
#include <span>
#include <vector>

class UsMap
{
    public:
        static constexpr uint8_t kNoState = 255;

        struct Products
        {
                std::vector<uint8_t> activeStates;
                std::vector<uint8_t> stateIds;
                int                  cols = 0, rows = 0;
        };

        UsMap(QString svgFilePath, int cols, int rows);
        bool      buildProducts(QString* errorMessage = nullptr);
        Products& getProducts() const;

        void drawBackground(QPainter& painter, const QRect& rect) const;

        uint8_t stateAtViewPos(QPointF position, QSize viewSize) const;

        static std::span<const char* const> abbrevs();

        bool    loadSvgPatched(QString* errorMessage);
        void    setDebug(bool on, QString dumpDir = {});
        void    setDebugColorizeStates(bool on);
        QString getStateName(uint8_t stateId) const;

    private:
        static const std::array<const char* const, 51> m_stateIDs;
        QRectF                                         svgViewBox() const;

        QString              m_svgFilePath;
        mutable QSvgRenderer m_svgRenderer;
        mutable QSvgRenderer m_svgRendererMask;
        Products             m_outputProducts;
        QPixmap              m_borderPixmap;
        QByteArray           m_svgRaw;
        QHash<QRgb, uint8_t> m_colorToState;
        std::vector<QString> m_stateNames;

        QImage m_maskImage;
        QImage m_layerImage;
        QImage m_debugColorImage;

        bool m_productsBuilt = false;

        bool                       m_debugEnabled  = false;
        bool                       m_debugColorize = false;
        QString                    m_debugDir;
        std::vector<int>           m_statePixelCount;
        void                       debugSave(const QString& name, const QImage& img) const;
        static inline QRgb         rgbNoAlpha(QRgb c) { return qRgb(qRed(c), qGreen(c), qBlue(c)); }
        bool                       buildColorMapFromSvg();
        static std::optional<QRgb> parseHexColor(const QString& hex);
};
