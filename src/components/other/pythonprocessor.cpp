#include "pythonprocessor.h"

#define PY_SSIZE_T_CLEAN

// Prevent Qt <-> Python macro collision
#ifdef slots
#undef slots
#endif

#include <Python.h>

#include <stdexcept>
#include <iostream>

static void ensurePython()
{
    static bool initialized = false;
    if (!initialized) {
        Py_Initialize();
        initialized = true;
    }
}


struct PythonProcessor::Impl {
    PyObject* module;
    PyObject* instance;
};

PythonProcessor::PythonProcessor()
    : impl(new Impl())
{
    impl->module = NULL;
    impl->instance = NULL;
    ensurePython();
}

void PythonProcessor::PythonProcessorLoadFile(const char* py_file_dir, const char* py_file_name, const char* class_name)
{
    // Ensure current directory is in sys.path
    PyObject* sysPath = PySys_GetObject("path"); // borrowed ref
    if (!sysPath || !PyList_Check(sysPath)) {
        throw std::runtime_error("sys.path is not a list");
    }

    PyObject* cwd = PyUnicode_FromString(
        py_file_dir
        );
    if (!cwd) {
        throw std::runtime_error("Failed to create Python string for path");
    }

    if (PyList_Append(sysPath, cwd) != 0) {
        Py_DECREF(cwd);
        PyErr_Print();
        throw std::runtime_error("Failed to append to sys.path");
    }

    Py_DECREF(cwd);


    std::cout << "The syspath is " << sysPath << std::endl;
    std::cout << "The c file name is " << py_file_name << std::endl;
    PyObject* name = PyUnicode_FromString(py_file_name);


    impl->module = PyImport_Import(name);
    Py_DECREF(name);

    if (!impl->module)
    {std::cout << "Is this printed?" << std::endl;
        throw std::runtime_error("Failed to import Python module");}

    PyObject* cls = PyObject_GetAttrString(impl->module, class_name);
    if (!cls || !PyCallable_Check(cls))
        throw std::runtime_error("Processor class not found");

    impl->instance = PyObject_CallObject(cls, NULL);
    Py_DECREF(cls);

    if (!impl->instance)
        throw std::runtime_error("Failed to create Processor instance");
}

PythonProcessor::~PythonProcessor()
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    Py_XDECREF(impl->instance);
    Py_XDECREF(impl->module);

    PyGILState_Release(gstate);

    delete impl;
}

std::pair<int, int> PythonProcessor::process(int value)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject* arg = PyLong_FromLong(value);
    PyObject* result = PyObject_CallMethod(impl->instance, "process", "O", arg);
    Py_DECREF(arg);

    if (!result) {
        PyGILState_Release(gstate);
        throw std::runtime_error("Python process() call failed");
    }

    int out = (int)PyLong_AsLong(PyTuple_GetItem(result, 0));
    int count = (int)PyLong_AsLong(PyTuple_GetItem(result, 1));

    Py_DECREF(result);
    PyGILState_Release(gstate);

    return std::make_pair(out, count);
}





