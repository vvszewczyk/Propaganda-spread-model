#pragma once

#include <QHash>
#include <QImage>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QtSvg/QSvgRenderer>
#include <array>
#include <cstdint>
#include <optional>
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
                int                  cols = 0;
                int                  rows = 0;
        };

        UsMap(QString svgFilePath, int cols, int rows);
        bool      buildProducts(QString* errorMessage = nullptr);
        Products& getProducts() const;

        void drawBackground(QPainter& painter, const QRect& rect) const;

        [[nodiscard]] uint8_t stateAtViewPos(QPointF position, QSize viewSize) const;

        static std::span<const char* const> abbrevs();

        bool                  loadSvgPatched(QString* errorMessage);
        void                  setDebug(bool on, QString dumpDir = {});
        void                  setDebugColorizeStates(bool on);
        [[nodiscard]] QString getStateName(uint8_t stateId) const;

    private:
        static const std::array<const char* const, 51> m_stateIDs;
        QRectF                                         svgViewBox() const;

        QString              m_svgFilePath;
        mutable QSvgRenderer m_svgRenderer;
        Products             m_outputProducts;
        QByteArray           m_svgRaw;
        QHash<QRgb, uint8_t> m_colorToState;
        std::vector<QString> m_stateNames;

        QImage m_maskImage;
        QImage m_debugColorImage;

        bool m_productsBuilt = false;

        bool             m_debugEnabled  = false;
        bool             m_debugColorize = false;
        QString          m_debugDir;
        std::vector<int> m_statePixelCount;

        void                       debugSave(const QString& name, const QImage& img) const;
        bool                       buildColorMapFromSvg();
        static std::optional<QRgb> parseHexColor(const QString& hex);
};
