#include "mainwindow.h"

// Qt Widgets/Layout
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>

// Widgets
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

// Qt Charts
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>

#include <QRegularExpression>
#include <algorithm>

// -------------------- ClickableChartView --------------------
/**
 * eigene ChartView Klasse, die Maus Klicks im Diagramm erkennt und die geklickte Position in Diagramm Koordinaten umrechnet
 */

ClickableChartView::ClickableChartView(QWidget* parent)
    : QChartView(parent)
{
}

void ClickableChartView::mousePressEvent(QMouseEvent* event)
{
    if (this->chart()) {
        const QPointF value = this->chart()->mapToValue(event->position());
        const QPoint globalPos = this->mapToGlobal(event->position().toPoint());
        emit chartClicked(globalPos, value);
    }
    QChartView::mousePressEvent(event);
}

// -------------------- MainWindow --------------------
/**
 * Konstruktor der Hauptklasse MainWindow, Benutzeroberfläche wird aufgebaut, Diagramme initialisiert,
 * Standard Werte gesetzt und Signal-Slot-Verbindungen hergesetell
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupCharts();

    // Defaults
    m_numEdit->setText("1 0");
    m_denEdit->setText("1 3 2");
    m_wMin->setValue(0.1);
    m_wMax->setValue(100.0);
    m_points->setValue(300);

    connect(m_computeBtn, &QPushButton::clicked, this, &MainWindow::onComputeClicked);
    connect(m_exportPngBtn, &QPushButton::clicked, this, &MainWindow::onExportPngClicked);
    connect(m_exportPdfBtn, &QPushButton::clicked, this, &MainWindow::onExportPdfClicked);

    connect(m_magView, &ClickableChartView::chartClicked, this, &MainWindow::onChartClicked);
    connect(m_phaseView, &ClickableChartView::chartClicked, this, &MainWindow::onChartClicked);

    setClickInfoText("Klick in ein Diagramm, um Messwerte + Crosshair anzuzeigen.");
}

MainWindow::~MainWindow() = default;


/**
 * MainWindow::setupUi: hier wird die komplette grafische Benutzeroberfläche der Anwendung erstellt und strukturiert. Eingabeelemente wie Buttons,
 * Anzeigefelder für Ergebnisse und Curserinformationen erzeugt und über Qt-Layouts angeordnet.
 * Außerdem werden die beiden Diagrammfenster instanziiert und in den rechten Bereich der Oberfläche intergriert.
 */
void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* rootLayout = new QHBoxLayout(central);

    // Left: inputs + info
    auto* inputBox = new QGroupBox("Eingabe & Auswertung", this);
    auto* inputLayout = new QVBoxLayout(inputBox);

    auto* form = new QFormLayout();

    m_numEdit = new QLineEdit(this);
    m_denEdit = new QLineEdit(this);

    form->addRow("Zähler (z.B. 1 0):", m_numEdit);
    form->addRow("Nenner (z.B. 1 3 2):", m_denEdit);

    m_wMin = new QDoubleSpinBox(this);
    m_wMax = new QDoubleSpinBox(this);
    m_points = new QSpinBox(this);

    m_wMin->setRange(1e-6, 1e9);
    m_wMax->setRange(1e-6, 1e9);
    m_points->setRange(50, 5000);

    m_wMin->setDecimals(6);
    m_wMax->setDecimals(6);

    form->addRow("ω_min (rad/s):", m_wMin);
    form->addRow("ω_max (rad/s):", m_wMax);
    form->addRow("Stützstellen N:", m_points);

    inputLayout->addLayout(form);

    m_computeBtn = new QPushButton("Berechnen", this);
    m_exportPngBtn = new QPushButton("Export PNG", this);
    m_exportPdfBtn = new QPushButton("Export PDF", this);

    inputLayout->addWidget(m_computeBtn);
    inputLayout->addWidget(m_exportPngBtn);
    inputLayout->addWidget(m_exportPdfBtn);

    m_resultLabel = new QLabel("Phasenstabilitätsspanne (PM): -\nAmplitudenstabilitätsspanne (GM): -\nω_gc: -\nω_pc: -", this);
    m_resultLabel->setWordWrap(true);
    inputLayout->addWidget(m_resultLabel);

    auto* clickBox = new QGroupBox("Cursor-Info (Klick im Plot)", this);
    auto* clickLayout = new QVBoxLayout(clickBox);

    m_clickInfoLabel = new QLabel(this);
    m_clickInfoLabel->setWordWrap(true);
    m_clickInfoLabel->setMinimumHeight(130);

    clickLayout->addWidget(m_clickInfoLabel);
    inputLayout->addWidget(clickBox);

    inputLayout->addStretch(1);

    // Right: plots
    auto* plotBox = new QGroupBox("Bode-Diagramm", this);
    auto* plotLayout = new QVBoxLayout(plotBox);

    m_magView = new ClickableChartView(this);
    m_phaseView = new ClickableChartView(this);

    plotLayout->addWidget(m_magView, 1);
    plotLayout->addWidget(m_phaseView, 1);

    rootLayout->addWidget(inputBox, 0);
    rootLayout->addWidget(plotBox, 1);

    setWindowTitle("BodeViewer");
    resize(1150, 740);
}

/**
 * MainWindow::setupCharts Initialisiert alle Diagrammobjekte, die für die Darstellung des Bode-Diagramms benötigt werden.
 * Dabei werden zunächst die Linien- und Markerserien für Magnitude, Phase, Referenzlinien, Stabilitätsmarker und Crosshair erzeugt
 * und anschließend zwei separate QChart-Objekte für Betrags- und Phasengang aufgebaut.
 * Danach werden die x- und y-Achsen konfiguriert, mit den Diagrammen verknüpft und schließlich den beiden ChartViews zugewiesen.
 */
void MainWindow::setupCharts()
{
    m_magSeries = new QLineSeries(this);
    m_phaseSeries = new QLineSeries(this);

    m_magZeroLine = new QLineSeries(this);
    m_magWgcLine  = new QLineSeries(this);
    m_magWpcLine  = new QLineSeries(this);

    m_phaseWgcLine = new QLineSeries(this);
    m_phaseWpcLine = new QLineSeries(this);

    m_magMarkers = new QScatterSeries(this);
    m_phaseMarkers = new QScatterSeries(this);
    m_magMarkers->setMarkerSize(9.0);
    m_phaseMarkers->setMarkerSize(9.0);

    // Crosshair
    m_magCrosshairX = new QLineSeries(this);
    m_phaseCrosshairX = new QLineSeries(this);

    QPen refPen;
    refPen.setStyle(Qt::DashLine);
    refPen.setWidth(1);

    m_magZeroLine->setPen(refPen);
    m_magWgcLine->setPen(refPen);
    m_magWpcLine->setPen(refPen);
    m_phaseWgcLine->setPen(refPen);
    m_phaseWpcLine->setPen(refPen);

    QPen crossPen;
    crossPen.setStyle(Qt::DotLine);
    crossPen.setWidth(2);
    m_magCrosshairX->setPen(crossPen);
    m_phaseCrosshairX->setPen(crossPen);

    auto* magChart = new QChart();
    magChart->addSeries(m_magSeries);
    magChart->addSeries(m_magZeroLine);
    magChart->addSeries(m_magWgcLine);
    magChart->addSeries(m_magWpcLine);
    magChart->addSeries(m_magMarkers);
    magChart->addSeries(m_magCrosshairX);
    magChart->legend()->hide();
    magChart->setTitle("Betragsgang |G(jω)| [dB]");

    auto* phaseChart = new QChart();
    phaseChart->addSeries(m_phaseSeries);
    phaseChart->addSeries(m_phaseWgcLine);
    phaseChart->addSeries(m_phaseWpcLine);
    phaseChart->addSeries(m_phaseMarkers);
    phaseChart->addSeries(m_phaseCrosshairX);
    phaseChart->legend()->hide();
    phaseChart->setTitle("Phasengang ∠ G(jω) [°]");

    m_magXAxis = new QLogValueAxis();
    m_magYAxis = new QValueAxis();
    m_phaseXAxis = new QLogValueAxis();
    m_phaseYAxis = new QValueAxis();

    m_magXAxis->setBase(10.0);
    m_phaseXAxis->setBase(10.0);
    m_magXAxis->setMinorTickCount(9);
    m_phaseXAxis->setMinorTickCount(9);

    m_magXAxis->setGridLineVisible(true);
    m_magXAxis->setMinorGridLineVisible(true);
    m_magYAxis->setGridLineVisible(true);

    m_phaseXAxis->setGridLineVisible(true);
    m_phaseXAxis->setMinorGridLineVisible(true);
    m_phaseYAxis->setGridLineVisible(true);

    m_magXAxis->setTitleText("ω [rad/s]");
    m_magYAxis->setTitleText("Magnitude [dB]");
    m_phaseXAxis->setTitleText("ω [rad/s]");
    m_phaseYAxis->setTitleText("Phase [°]");

    magChart->addAxis(m_magXAxis, Qt::AlignBottom);
    magChart->addAxis(m_magYAxis, Qt::AlignLeft);

    phaseChart->addAxis(m_phaseXAxis, Qt::AlignBottom);
    phaseChart->addAxis(m_phaseYAxis, Qt::AlignLeft);

    auto attachAllMag = [this](QAbstractSeries* s) {
        s->attachAxis(m_magXAxis);
        s->attachAxis(m_magYAxis);
    };
    attachAllMag(m_magSeries);
    attachAllMag(m_magZeroLine);
    attachAllMag(m_magWgcLine);
    attachAllMag(m_magWpcLine);
    attachAllMag(m_magMarkers);
    attachAllMag(m_magCrosshairX);

    auto attachAllPhase = [this](QAbstractSeries* s) {
        s->attachAxis(m_phaseXAxis);
        s->attachAxis(m_phaseYAxis);
    };
    attachAllPhase(m_phaseSeries);
    attachAllPhase(m_phaseWgcLine);
    attachAllPhase(m_phaseWpcLine);
    attachAllPhase(m_phaseMarkers);
    attachAllPhase(m_phaseCrosshairX);

    m_magView->setChart(magChart);
    m_phaseView->setChart(phaseChart);

    m_magView->setRenderHint(QPainter::Antialiasing);
    m_phaseView->setRenderHint(QPainter::Antialiasing);

    m_magCrosshairX->clear();
    m_phaseCrosshairX->clear();
}

/**
 * MainWindow::setClickInfoText setzt den Text für die Cursor-Informationen und wird verwendet, um dem Benutzer Hinweise oder Messwerte anzuzeigen
 */
void MainWindow::setClickInfoText(const QString& text)
{
    if (m_clickInfoLabel) m_clickInfoLabel->setText(text);
}

/**
 * MainWindow::parseCoeffList verarbeitet die vom Benutzer eingegebenen Koeffizienten der Übertragungsfunktion.
 * Dabei werden die Eingaben auf gültigkeit überrpüft, bei unggültiger Eingabe wird die Berechnung durch rückgabe von false abgebrochen
 */
bool MainWindow::parseCoeffList(const QString& text, std::vector<double>& out)
{
    out.clear();
    const QStringList parts = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.isEmpty()) return false;

    for (const QString& p : parts) {
        bool ok = false;
        const double v = p.toDouble(&ok);
        if (!ok) return false;
        out.push_back(v);
    }
    return !out.empty();
}

/**
 * MainWindow::formatMargins erzeugt aus den berechneten Stabilitätsreserven einen formatierten Text, der später im Ergebnisbereich der Benutzeroberfläche angezeigt wird.
 * Anschließend werden Phase Margin, Gain Margin sowie die zugehörigen Durchtrittsfrequenzen (w_gc und w_pc) in einen mehrzeiligen String eingesetzt und zurückgegeben.
 */
QString MainWindow::formatMargins(const Margins& m)
{
    auto fmt = [](const std::optional<double>& v, const QString& unit) -> QString {
        if (!v.has_value()) return "-";
        return QString::number(*v, 'f', 3) + unit;
    };

    return QString(
               "Phasenstabilitätsspanne (PM): %1\n"
               "Amplitudenstabilitätsspanne (GM): %2\n"
               "ω_gc (|G|=0 dB): %3\n"
               "ω_pc (Phase=-180°): %4"
               )
        .arg(fmt(m.phaseMargin_deg, "°"))
        .arg(fmt(m.gainMargin_db, " dB"))
        .arg(m.w_gc.has_value() ? QString::number(*m.w_gc, 'f', 3) + " rad/s" : "-")
        .arg(m.w_pc.has_value() ? QString::number(*m.w_pc, 'f', 3) + " rad/s" : "-");
}

/**
 * MainWindow::interpolateAtOmega bestimmt für eine beliebige Frequenz w die zugehörigen Werte für Betrag und Phase,
 * auch wenn diese Frequenz nicht exakt in den berechneten Stützstellen vorhanden ist.
 * Dazu wird im Frequenzvektor nach zwei benachbarten Punkten gesucht, zwischen denen die gewünschte Frequenz liegt,
 * und anschließend eine lineare Interpolation zwischen den entsprechenden Magnitude- und Phasenwerten durchgeführt
 */
bool MainWindow::interpolateAtOmega(const BodeData& d, double omega, double& magDb, double& phaseDeg)
{
    if (d.omega.size() < 2) return false;
    if (omega < d.omega.front() || omega > d.omega.back()) return false;

    for (size_t i = 1; i < d.omega.size(); ++i) {
        const double w0 = d.omega[i - 1];
        const double w1 = d.omega[i];
        if (omega >= w0 && omega <= w1) {
            const double t = (omega - w0) / (w1 - w0);
            magDb = d.mag_db[i - 1] + t * (d.mag_db[i] - d.mag_db[i - 1]);
            phaseDeg = d.phase_deg[i - 1] + t * (d.phase_deg[i] - d.phase_deg[i - 1]);
            return true;
        }
    }
    return false;
}

/**
 * MainWindow::updateCrosshair aktualisiert die vertikale Crosshair-Linie in beiden Diagrammen an der vom Benutzer ausgewählten Frequenz.
 */
void MainWindow::updateCrosshair(double omega)
{
    if (!m_hasBode) return;

    const double magYmin = m_magYAxis->min();
    const double magYmax = m_magYAxis->max();
    const double phYmin  = m_phaseYAxis->min();
    const double phYmax  = m_phaseYAxis->max();

    m_magCrosshairX->clear();
    m_phaseCrosshairX->clear();

    m_magCrosshairX->append(omega, magYmin);
    m_magCrosshairX->append(omega, magYmax);

    m_phaseCrosshairX->append(omega, phYmin);
    m_phaseCrosshairX->append(omega, phYmax);
}

/**
 * MainWindow::updateCharts übernimmt die vollständige Aktualisierung der Diagrammdarstellung nach einer neuen Berechnung.
 * die aktuellen Bode-Daten und Stabilitätsreserven im internen Zustand gespeichert und alle bisherigen Linien-, Marker- und Crosshair-Serien geleert,
 * damit keine veralteten Werte im Diagramm verbleiben.
 * Anschließend werden die neuen Magnitude- und Phasenwerte in die jeweiligen Datenserien eingefügt,
 * die Achsenbereiche automatisch anhand von Minimal- und Maximalwerten angepasst und zusätzliche Referenzelemente wie die 0-dB-Linie sowie Marker- und
 * Vertikallinien für die Durchtrittsfrequenzen w_gc und w_pc eingezeichnet
 */
void MainWindow::updateCharts(const BodeData& d, const Margins& m)
{
    m_lastBode = d;
    m_lastMargins = m;
    m_hasBode = !d.omega.empty();

    m_magSeries->clear();
    m_phaseSeries->clear();
    m_magZeroLine->clear();
    m_magWgcLine->clear();
    m_magWpcLine->clear();
    m_phaseWgcLine->clear();
    m_phaseWpcLine->clear();
    m_magMarkers->clear();
    m_phaseMarkers->clear();
    m_magCrosshairX->clear();
    m_phaseCrosshairX->clear();

    if (d.omega.empty()) return;

    const double wMin = d.omega.front();
    const double wMax = d.omega.back();

    double magMin = d.mag_db.front();
    double magMax = d.mag_db.front();
    double phMin = d.phase_deg.front();
    double phMax = d.phase_deg.front();

    for (size_t i = 0; i < d.omega.size(); ++i) {
        const double w = d.omega[i];
        const double mag = d.mag_db[i];
        const double ph = d.phase_deg[i];

        m_magSeries->append(w, mag);
        m_phaseSeries->append(w, ph);

        magMin = std::min(magMin, mag);
        magMax = std::max(magMax, mag);
        phMin = std::min(phMin, ph);
        phMax = std::max(phMax, ph);
    }

    m_magXAxis->setRange(wMin, wMax);
    m_phaseXAxis->setRange(wMin, wMax);

    const double magYmin = magMin - 3.0;
    const double magYmax = magMax + 3.0;
    const double phYmin = phMin - 10.0;
    const double phYmax = phMax + 10.0;

    m_magYAxis->setRange(magYmin, magYmax);
    m_phaseYAxis->setRange(phYmin, phYmax);

    m_magZeroLine->append(wMin, 0.0);
    m_magZeroLine->append(wMax, 0.0);

    if (m.w_gc.has_value()) {
        const double wgc = *m.w_gc;

        m_magWgcLine->append(wgc, magYmin);
        m_magWgcLine->append(wgc, magYmax);

        m_phaseWgcLine->append(wgc, phYmin);
        m_phaseWgcLine->append(wgc, phYmax);

        m_magMarkers->append(wgc, 0.0);

        double magAt, phAt;
        if (interpolateAtOmega(d, wgc, magAt, phAt)) {
            m_phaseMarkers->append(wgc, phAt);
        }
    }

    if (m.w_pc.has_value()) {
        const double wpc = *m.w_pc;

        m_magWpcLine->append(wpc, magYmin);
        m_magWpcLine->append(wpc, magYmax);

        m_phaseWpcLine->append(wpc, phYmin);
        m_phaseWpcLine->append(wpc, phYmax);

        m_phaseMarkers->append(wpc, -180.0);

        double magAt, phAt;
        if (interpolateAtOmega(d, wpc, magAt, phAt)) {
            m_magMarkers->append(wpc, magAt);
        }
    }
}

/**
 * MainWindow::onComputeClicked wird aufgerufen, wenn der Benutzer den Button „Berechnen“ betätigt, und startet den gesamten Berechnungsablauf des Bode-Diagramms
 * Anschließend wird das logarithmische Frequenzraster erzeugt, der Frequenzgang berechnet,
 * die Stabilitätsreserven bestimmt und schließlich sowohl die Diagramme als auch die Ergebnisanzeige in der Benutzeroberfläche aktualisiert
 */
void MainWindow::onComputeClicked()
{
    std::vector<double> num, den;

    if (!parseCoeffList(m_numEdit->text(), num)) {
        QMessageBox::warning(this, "Eingabefehler",
                             "Zähler: Bitte Zahlen durch Leerzeichen getrennt eingeben (z.B. 1 0).");
        return;
    }
    if (!parseCoeffList(m_denEdit->text(), den)) {
        QMessageBox::warning(this, "Eingabefehler",
                             "Nenner: Bitte Zahlen durch Leerzeichen getrennt eingeben (z.B. 1 3 2).");
        return;
    }

    TransferFunction tf(num, den);
    if (!tf.isValid()) {
        QMessageBox::warning(this, "Eingabefehler", "Nenner ist ungültig (leer oder nur Nullen).");
        return;
    }

    const double wmin = m_wMin->value();
    const double wmax = m_wMax->value();
    const int n = m_points->value();

    const auto omega = FrequencyGrid::logspace(wmin, wmax, n);
    if (omega.empty()) {
        QMessageBox::warning(this, "Eingabefehler", "Frequenzbereich ungültig. Prüfe ω_min > 0 und ω_max > ω_min.");
        return;
    }

    const auto bode = BodeComputation::compute(tf, omega, true);
    const auto margins = StabilityMargins::compute(bode);

    m_resultLabel->setText(formatMargins(margins));
    updateCharts(bode, margins);

    setClickInfoText("Berechnung abgeschlossen.\nKlick in ein Diagramm, um Werte + Crosshair anzuzeigen.");
}

/**
 * MainWindow::onChartClicked wird aufgerufen, wenn der Benutzer in eines der Diagramme klickt, und dient zur interaktiven Analyse des Bode-Diagramms.
 * Zunächst wird geprüft, ob bereits Berechnungsdaten vorhanden sind, anschließend wird aus der Klickposition die zugehörige Frequenz bestimmt,
 * eine Crosshair-Linie im Diagramm aktualisiert und mittels Interpolation der entsprechende Magnitude- und Phasenwert berechnet
 */
void MainWindow::onChartClicked(const QPoint& globalPos, const QPointF& value)
{
    if (!m_hasBode) {
        QToolTip::showText(globalPos, "Noch keine Daten.\nBitte erst „Berechnen“ klicken.");
        setClickInfoText("Noch keine Daten.\nBitte erst „Berechnen“ klicken.");
        return;
    }

    const double omega = value.x();
    if (omega <= 0.0) return;

    updateCrosshair(omega);

    double magDb = 0.0;
    double phaseDeg = 0.0;

    if (!interpolateAtOmega(m_lastBode, omega, magDb, phaseDeg)) {
        const QString msg = QString("ω = %1 rad/s\n(außerhalb des berechneten Bereichs)").arg(omega, 0, 'g', 6);
        QToolTip::showText(globalPos, msg);
        setClickInfoText(msg);
        return;
    }

    const double dist0db = magDb;
    const double dist180 = phaseDeg + 180.0;

    QString panel =
        QString("Klickposition:\n"
                "ω = %1 rad/s\n"
                "|G(jω)| = %2 dB\n"
                "∠G(jω) = %3 °\n\n"
                "Abstand zu 0 dB: %4 dB\n"
                "Abstand zu -180°: %5 °")
            .arg(omega, 0, 'g', 6)
            .arg(magDb, 0, 'f', 2)
            .arg(phaseDeg, 0, 'f', 2)
            .arg(dist0db, 0, 'f', 2)
            .arg(dist180, 0, 'f', 2);

    if (m_lastMargins.w_gc.has_value()) {
        panel += QString("\n\nω_gc = %1 rad/s").arg(*m_lastMargins.w_gc, 0, 'g', 6);
    }
    if (m_lastMargins.w_pc.has_value()) {
        panel += QString("\nω_pc = %1 rad/s").arg(*m_lastMargins.w_pc, 0, 'g', 6);
    }

    QToolTip::showText(globalPos, panel);
    setClickInfoText(panel);
}

/**
 * MainWindow::onExportPngClicked ermöglicht es dem Benutzer, die aktuelle Darstellung der Anwendung als PNG-Datei zu speichern
 */
void MainWindow::onExportPngClicked()
{
    const QString filename = QFileDialog::getSaveFileName(this, "PNG speichern", "", "PNG-Datei (*.png)");
    if (filename.isEmpty()) return;

    const QPixmap pix = this->grab();
    pix.save(filename, "PNG");
}

/**
 * MainWindow::onExportPdfClicked ermöglicht es, die aktuelle Darstellung der Anwendung als PDF-Datei zu exportieren
 */
void MainWindow::onExportPdfClicked()
{
    const QString filename = QFileDialog::getSaveFileName(this, "PDF speichern", "", "PDF-Datei (*.pdf)");
    if (filename.isEmpty()) return;

    QPdfWriter writer(filename);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);

    const QPixmap pix = this->grab();
    const QRect pageRect = painter.viewport();

    QSize scaled = pix.size();
    scaled.scale(pageRect.size(), Qt::KeepAspectRatio);

    painter.drawPixmap(0, 0, pix.scaled(scaled, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    painter.end();
}
