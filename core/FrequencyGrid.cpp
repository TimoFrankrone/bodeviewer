#include "FrequencyGrid.h"
#include <cmath>

std::vector<double> FrequencyGrid::logspace(double wMin, double wMax, int n) {
    std::vector<double> w;

    if (n < 2 || wMin <= 0.0 || wMax <= wMin) {
        return w; // leer => UI kann Fehlermeldung zeigen
    }

    w.reserve(n);

    const double a = std::log10(wMin);
    const double b = std::log10(wMax);

    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / (n - 1);
        const double x = a + t * (b - a);
        w.push_back(std::pow(10.0, x));
    }
    return w;
}
