#include <cmath>  // std::exp, std::cos
#include <iostream>

#include "plotter.h"

int main() {
    // Create an exponentially decaying sinusoidal signal as an example
    std::vector<double> signal(1024);
    for (size_t i = 0; i < signal.size(); ++i)
        signal[i] = std::exp(i / -256.0) * std::cos(2 * M_PI * 8 * i / 1024.0);

    Plotter* plotter = Plotter::GetInstance();

    plotter->plot(signal);

    std::cout << "Exiting..." << std::endl;
}