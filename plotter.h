#ifndef PYBIND_PLOT_PLOTTER_H
#define PYBIND_PLOT_PLOTTER_H

#include <cmath>  // std::exp, std::cos
#include <iostream>
#include <vector>

#include <pybind11/embed.h>  // py::scoped_interpreter
#include <pybind11/stl.h>    // bindings from C++ STL containers to Python types

namespace py = pybind11;

class Plotter{
public:
    Plotter(Plotter & other) = delete;

    void operator=(const Plotter& ) = delete;

    static Plotter * GetInstance();

    template<class DataType>
    void plot(const std::vector<DataType>& signal){
        // Start the Python interpreter
        py::scoped_interpreter guard{};
        using namespace py::literals;

        // Save the necessary local variables in a Python dict
        py::dict locals = py::dict{
                "signal"_a = signal,
        };

        // Execute Python code, using the variables saved in `locals`
        py::exec(R"(

    import matplotlib.pyplot as plt

    plt.plot(signal)
    plt.show()

    )",
                 py::globals(), locals);

        std::cout << "Exiting..." << std::endl;
    };

protected:
    Plotter(){

    }


private:
    static Plotter* _instance;
};

Plotter* Plotter::_instance = nullptr;;

/**
 * Static methods should be defined outside the class.
 */
Plotter *Plotter::GetInstance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if(_instance==nullptr){
        _instance = new Plotter();
    }
    return _instance;
}

#endif //PYBIND_PLOT_PLOTTER_H
