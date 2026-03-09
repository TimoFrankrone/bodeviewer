#pragma once
#include "BodeComputation.h"
#include <optional>

/**
 * @brief Stabilitätsreserven (Bode-Kriterien) für offene Übertragungsfunktion.
 */
struct Margins {
    std::optional<double> phaseMargin_deg; // PM [°]
    std::optional<double> gainMargin_db;   // GM [dB]
    std::optional<double> w_gc;            // Gain crossover (|G|=1 => 0 dB)
    std::optional<double> w_pc;            // Phase crossover (Phase=-180°)
};

class StabilityMargins {
public:
    /**
     * @brief Bestimmt PM und GM über Durchtrittsfrequenzen per Interpolation.
     */
    static Margins compute(const BodeData& d);
};
