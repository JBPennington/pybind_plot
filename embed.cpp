#include <iostream>
#include <fstream>
#include <Python.h>
#include <filesystem>
#include "tools/cpp/runfiles/runfiles.h"

using bazel::tools::cpp::runfiles::Runfiles;

void InitializePythonEnvironment(const std::string& pythonHome, const std::vector<std::string>& additionalPaths) {

    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    // Set PYTHONHOME
    wchar_t* pythonHomeW = Py_DecodeLocale(pythonHome.c_str(), nullptr);
    status = PyConfig_SetString(&config, &config.home, pythonHomeW);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }

    config.isolated = 1;

    // Initialize the interpreter with the given configuration
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    // The PyConfig structure should be released after initialization
    PyConfig_Clear(&config);

    // Check if initialization was successful
    if (!Py_IsInitialized()) {
        std::cout << "Failed to initialize Python interpreter." << std::endl;
    }

    // Import the sys module.
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        std::cout << "Failed to import 'sys' module." << std::endl;
    }

    // Get the sys.path list.
    PyObject* sysPath = PyObject_GetAttrString(sysModule, "path");
    if (!sysPath) {
        std::cout << "Failed to get 'sys.path'." << std::endl;
    }

    // Add each path in additionalPaths to sys.path.
    for (const auto& path : additionalPaths) {
        PyObject* pyPath = PyUnicode_FromString(path.c_str());
        if (!pyPath) {
            std::cout << "Failed to create Python string from path." << std::endl;
            continue;
        }
        if (PyList_Append(sysPath, pyPath) != 0) {
            std::cout << "Failed to append path to 'sys.path'." << std::endl;
        }
        Py_DECREF(pyPath);
    }

    // Clean up references.
    Py_DECREF(sysPath);
    Py_DECREF(sysModule);
    Py_DECREF(pythonHomeW); // Clean up the allocated string.

    // At this point, the Python interpreter is initialized, PYTHONHOME is set,
    // and additional paths have been added to sys.path. You can now proceed to
    // execute Python scripts or finalize the interpreter as needed.
}

void PrintSysPath() {
    // Import the sys module.
    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        PyErr_Print(); // Print any error if occurred
        std::cout << "Failed to import 'sys' module." << std::endl;
        return;
    }

    // Get the sys.path list.
    PyObject* sysPath = PyObject_GetAttrString(sysModule, "path");
    if (!sysPath || !PyList_Check(sysPath)) {
        PyErr_Print(); // Print any error if occurred
        std::cout << "Failed to access 'sys.path'." << std::endl;
        Py_XDECREF(sysModule); // Py_XDECREF safely decrements the ref count if the object is not NULL
        return;
    }

    // Get the size of sys.path list to iterate over it
    Py_ssize_t size = PyList_Size(sysPath);
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject* path = PyList_GetItem(sysPath, i); // Borrowed reference, no need to DECREF
        if (path) {
            const char* pathStr = PyUnicode_AsUTF8(path);
            if (pathStr) {
                std::cout << "\t" << pathStr << std::endl;
            } else {
                PyErr_Print(); // Print any error if occurred
            }
        }
    }

    // Clean up: DECREF objects created via PyImport_ImportModule and PyObject_GetAttrString
    Py_DECREF(sysPath);
    Py_DECREF(sysModule);
}


void ImportAndPrintVersion(const std::string& python_module_name) {
    // Import the scipy module.
    PyObject* pyModule = PyImport_ImportModule(python_module_name.c_str());
    if (!pyModule) {
        PyErr_Print(); // Print the error to stderr.
        std::cerr << "Failed to import module" << python_module_name << std::endl;
        return;
    }

    // Access the __version__ attribute of the module.
    PyObject* version = PyObject_GetAttrString(pyModule, "__version__");
    if (!version) {
        PyErr_Print(); // Print the error to stderr.
        std::cerr << "Failed to get '__version__'." << std::endl;
        Py_DECREF(pyModule);
        return;
    }

    // Convert the version PyObject to a C string.
    const char* versionStr = PyUnicode_AsUTF8(version);
    if (!versionStr) {
        PyErr_Print(); // Print the error to stderr.
        std::cerr << "Failed to convert '__version__' to C string." << std::endl;
    } else {
        // Print the version string.
        std::cout << "\t" << python_module_name << " version: " << versionStr << std::endl;
    }

    // Clean up references.
    Py_DECREF(version);
    Py_DECREF(pyModule);
}

std::vector<std::string> read_lines(const std::string& path) {
    std::ifstream file(path);

//    CHECK(file.is_open()) << "Could not open file " << path;

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}

int main(int argc, char* argv[]) {

    std::cout << "Starting" << std::endl;
    std::string error;
    std::unique_ptr<Runfiles> runfiles(
            Runfiles::Create(argv[0], BAZEL_CURRENT_REPOSITORY, &error));
//    CHECK(runfiles) << "Could not create runfiles";

    std::string dot_python_home_path = runfiles->Rlocation("_main/python/experimental/embed_paths.python_home");
    std::string python_home_path = read_lines(dot_python_home_path).front();
    std::string python_home_path_absolute = runfiles->Rlocation("_main/" + python_home_path);
    auto external_dir = std::filesystem::path(python_home_path_absolute).parent_path();

    std::string dot_imports_path = runfiles->Rlocation("_main/python/experimental/embed_paths.imports");
    std::vector<std::string> imports = read_lines(dot_imports_path);
    std::vector<std::string> absolute_imports;
    for (const std::string& relative_import : imports) {
        const auto absolute_import = runfiles->Rlocation(relative_import);
        absolute_imports.push_back(absolute_import);
    }

    InitializePythonEnvironment(python_home_path_absolute, absolute_imports);
    std::cout << "Initialized. Dumping sys path." << "\n";

    PrintSysPath();

    std::cout << "Testing module import" << "\n";
    ImportAndPrintVersion("numpy");
    ImportAndPrintVersion("scipy");

    Py_Finalize();

    return 0;
}
