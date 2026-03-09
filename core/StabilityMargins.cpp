#include "StabilityMargins.h"

/**
 * @brief Lineare Interpolation: y(x) zwischen (x0,y0) und (x1,y1)
 */
static double lerp(double x0, double y0, double x1, double y1, double x) {
    const double t = (x - x0) / (x1 - x0);
    return y0 + t * (y1 - y0);
}

Margins StabilityMargins::compute(const BodeData& d) {
    Margins m;

    // Phase Margin (PM):
    // 1) finde ω_gc, wo Betrag 0 dB schneidet
    // 2) PM = 180° + Phase(ω_gc)
    for (size_t i = 1; i < d.omega.size(); ++i) {
        const double y0 = d.mag_db[i - 1];
        const double y1 = d.mag_db[i];

        // Prüfe, ob 0 dB zwischen y0 und y1 liegt
        if ((y0 <= 0.0 && y1 >= 0.0) || (y0 >= 0.0 && y1 <= 0.0)) {
            const double w0 = d.omega[i - 1];
            const double w1 = d.omega[i];

            // Interpolierte Frequenz ω_gc
            const double wgc = lerp(y0, w0, y1, w1, 0.0);

            // Phase an ω_gc interpolieren
            const double ph = lerp(w0, d.phase_deg[i - 1], w1, d.phase_deg[i], wgc);

            m.w_gc = wgc;
            m.phaseMargin_deg = 180.0 + ph;
            break;
        }
    }

    // Gain Margin (GM):
    // 1) finde ω_pc, wo Phase -180° schneidet
    // 2) GM(dB) = -Mag_dB(ω_pc)
    for (size_t i = 1; i < d.omega.size(); ++i) {
        // Wir suchen Phase = -180 => (Phase + 180) = 0
        const double p0 = d.phase_deg[i - 1] + 180.0;
        const double p1 = d.phase_deg[i] + 180.0;

        if ((p0 <= 0.0 && p1 >= 0.0) || (p0 >= 0.0 && p1 <= 0.0)) {
            const double w0 = d.omega[i - 1];
            const double w1 = d.omega[i];

            const double wpc = lerp(p0, w0, p1, w1, 0.0);
            const double magAtWpc = lerp(w0, d.mag_db[i - 1], w1, d.mag_db[i], wpc);

            m.w_pc = wpc;
            m.gainMargin_db = -magAtWpc;
            break;
        }
    }

    return m;
}
