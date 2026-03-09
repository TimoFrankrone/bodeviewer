#pragma once

#include <QMainWindow>
#include <QMouseEvent>              // für mousePressEvent(QMouseEvent*)
#include <QtCharts/QChartView>      // wichtig: Basisklasse vollständig bekannt

#include <optional>
#include <vector>

// Forward Declarations (alles andere darf forward bleiben)
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QLabel;

class QChart;
class QLineSeries;
class QScatterSeries;
class QValueAxis;
class QLogValueAxis;

#include "core/TransferFunction.h"
#include "core/FrequencyGrid.h"
#include "core/BodeComputation.h"
#include "core/StabilityMargins.h"

/**
 * Die Klasse ClickableChartView erweitert die Qt-Klasse QChartView um eine eigene Interaktionsfunktion für Mausklicks im Diagramm
 */
class ClickableChartView : public QChartView
{
    Q_OBJECT
public:
    explicit ClickableChartView(QWidget* parent = nullptr);

signals:
    void chartClicked(const QPoint& globalPos, const QPointF& value);

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

/**
 * Die Klasse MainWindow bildet das zentrale Fenster der Anwendung und koordiniert sowohl die Benutzeroberfläche
 * als auch die Berechnung und Darstellung des Bode-Diagramms
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onComputeClicked();
    void onExportPngClicked();
    void onExportPdfClicked();
    void onChartClicked(const QPoint& globalPos, const QPointF& value);

private:
    // UI
    QLineEdit* m_numEdit = nullptr;
    QLineEdit* m_denEdit = nullptr;

    QDoubleSpinBox* m_wMin = nullptr;
    QDoubleSpinBox* m_wMax = nullptr;
    QSpinBox* m_points = nullptr;

    QPushButton* m_computeBtn = nullptr;
    QPushButton* m_exportPngBtn = nullptr;
    QPushButton* m_exportPdfBtn = nullptr;

    QLabel* m_resultLabel = nullptr;
    QLabel* m_clickInfoLabel = nullptr;

    // Charts / Series
    ClickableChartView* m_magView = nullptr;
    ClickableChartView* m_phaseView = nullptr;

    QLineSeries* m_magSeries = nullptr;
    QLineSeries* m_phaseSeries = nullptr;

    QLineSeries* m_magZeroLine = nullptr;
    QLineSeries* m_magWgcLine  = nullptr;
    QLineSeries* m_magWpcLine  = nullptr;

    QLineSeries* m_phaseWgcLine = nullptr;
    QLineSeries* m_phaseWpcLine = nullptr;

    QScatterSeries* m_magMarkers = nullptr;
    QScatterSeries* m_phaseMarkers = nullptr;

    // Crosshair
    QLineSeries* m_magCrosshairX = nullptr;
    QLineSeries* m_phaseCrosshairX = nullptr;

    // Axes
    QLogValueAxis* m_magXAxis = nullptr;
    QValueAxis*    m_magYAxis = nullptr;

    QLogValueAxis* m_phaseXAxis = nullptr;
    QValueAxis*    m_phaseYAxis = nullptr;

    // Data cache
    BodeData m_lastBode{};
    Margins  m_lastMargins{};
    bool     m_hasBode = false;

    // Helpers
    void setupUi();
    void setupCharts();
    void updateCharts(const BodeData& d, const Margins& m);

    void setClickInfoText(const QString& text);
    void updateCrosshair(double omega);

    static bool parseCoeffList(const QString& text, std::vector<double>& out);
    static QString formatMargins(const Margins& m);
    static bool interpolateAtOmega(const BodeData& d, double omega, double& magDb, double& phaseDeg);
};
