#pragma once

#include "Types.hpp"

#include <QHash>
#include <QImage>
#include <QMap>
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

        int                   stateCount() const { return static_cast<int>(m_states.size()); }
        [[nodiscard]] QString getStateName(uint8_t stateId) const;
        QString               getStateId(uint8_t stateId) const;
        std::optional<int>    indexOfAbbrev(QStringView abbrev) const;

        bool readSvgFile(QString* errorMessage);
        void patchSvgContent();
        bool loadSvgPatched(QString* errorMessage);
        bool loadSvgIntoRenderer(QString* errorMessage);
        void setError(QString* errorMessage, const QString& message) const;
        void setDebug(bool on, QString dumpDir = {});
        void setDebugColorizeStates(bool on);

    private:
        struct State
        {
                QString id;
                QString name;
                QRgb    fill = 0;
        };
        std::vector<State>   m_states;
        QMap<QString, int>   m_stateIdToIndex;
        QHash<QRgb, uint8_t> m_colorToState;

        QRectF svgViewBox() const;

        QString              m_svgFilePath;
        mutable QSvgRenderer m_svgRenderer;
        Products             m_outputProducts;
        QByteArray           m_svgRaw;
        std::vector<QString> m_stateNames;

        QImage m_maskImage;
        QImage m_debugColorImage;

        bool m_productsBuilt = false;

        bool             m_debugEnabled  = false;
        bool             m_debugColorize = false;
        QString          m_debugDir;
        std::vector<int> m_statePixelCount;

        void debugSave(const QString& name, const QImage& img) const;

        void    resetStates();
        bool    isStateElement(const QString& tagName) const;
        void    insertOrUpdateState(State&& state);
        QString extractFill(const QXmlStreamAttributes& attributes) const;
        void    processStateElement(const QXmlStreamAttributes& attributes);
        void    reportXmlError(const QXmlStreamReader& xml) const;
        bool    scanStatesFromSvg();

        static bool                isValidHexColor(const QString& hex);
        static std::optional<QRgb> parseHexColor(const QString& hex);
};
