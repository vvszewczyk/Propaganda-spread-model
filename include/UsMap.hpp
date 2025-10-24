#pragma once

#include <QImage>
#include <QPixmap>
#include <QtSvg/QSvgRenderer>
#include <cstdint>
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
        void      drawBackground(QPainter& painter, const QRect& rect) const;
        uint8_t   stateAtViewPos(QPointF position, QSize viewSize) const;
        static std::span<const char* const> abbrevs();
        static std::optional<int>           indexOfAbbrev(QString twoLetterAbbrev);
        bool                                loadSvgPatched(QString* errorMessage);

    private:
        static const std::array<const char* const, 51> m_stateIDs;
        QRectF                                         svgViewBox() const;

        QString              m_svgFilePath;
        mutable QSvgRenderer m_svgRenderer;
        Products             m_outputProducts;
        QPixmap              m_borderPixmap;

        QImage m_maskImage;
        QImage m_layerImage;
        bool   m_productsBuilt = false;
};
