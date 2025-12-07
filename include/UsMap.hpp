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
#include <QStringview>
#include <QSvgrenderer>
#include <cstdint>
#include <optional>
#include <vector>

class QXmlStreamReader;
class QXmlStreamAttributes;

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

        [[nodiscard]] bool buildStateProducts(QString* errorMessage = nullptr);

        [[nodiscard]] const Products& getProducts() const;
        [[nodiscard]] int     stateCount() const { return static_cast<int>(m_states.size()); }
        [[nodiscard]] bool    isValidStateId(uint8_t stateId) const;
        [[nodiscard]] QString getStateName(uint8_t stateId) const;
        [[nodiscard]] QString getStateId(uint8_t stateId) const;

        void drawStateOutlines(QPainter& painter, const QRect& rect) const;

        void setDebug(bool on, QString dumpDir = {});

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

        QString              m_svgFilePath;
        mutable QSvgRenderer m_svgRenderer;
        mutable QSvgRenderer m_svgOutlineRenderer;

        Products   m_outputProducts;
        QByteArray m_svgRawOriginal;
        QByteArray m_svgRaw;
        QImage     m_maskImage;

        bool             m_productsBuilt = false;
        bool             m_debugEnabled  = false;
        QString          m_debugDir;
        std::vector<int> m_statePixelCount;

        [[nodiscard]] bool loadAndParseSvg(QString* errorMessage);
        [[nodiscard]] bool readSvgFile(QString* errorMessage);
        void               removeStatesOutlines();
        [[nodiscard]] bool loadSvgPatched(QString* errorMessage);
        [[nodiscard]] bool loadSvgIntoRenderer(QString* errorMessage);
        [[nodiscard]] bool loadSvgOutlineWithoutFill(QString* errorMessage);
        void               setError(QString* errorMessage, const QString& message) const;

        void                  resetStates();
        [[nodiscard]] bool    isStateElement(const QString& tagName) const;
        void                  insertOrUpdateState(State&& state);
        [[nodiscard]] QString extractFill(const QXmlStreamAttributes& attributes) const;
        void                  processStateElement(const QXmlStreamAttributes& attributes);
        void                  reportXmlError(const QXmlStreamReader& xml) const;
        [[nodiscard]] bool    scanStatesFromSvg();

        void                 buildMaskAndActiveStates(int cols, int rows);
        [[nodiscard]] QImage renderColorImage(int cols, int rows) const;
        void                 buildColorToStateMap();
        void                 initProductsBuffers();
        void    assignStatesFromColorImage(const QImage& colorImage, int cols, int rows);
        QString rgbToHex(QRgb c) const;

        void debugSave(const QString& name, const QImage& img) const;

        [[nodiscard]] static bool                isValidHexColor(const QString& hex);
        [[nodiscard]] static std::optional<QRgb> parseHexColor(const QString& hex);
};
