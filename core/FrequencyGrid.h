#pragma once
#include <vector>

/**
 * @brief Hilfsfunktionen zum Erzeugen eines Frequenzgitters.
 */
class FrequencyGrid {
public:
    /**
     * @brief Erzeugt logarithmisch verteilte Kreisfrequenzen [rad/s]
     * @param wMin > 0
     * @param wMax > wMin
     * @param n Anzahl Stützstellen (>=2)
     */
    static std::vector<double> logspace(double wMin, double wMax, int n);
};
