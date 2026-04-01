/***************************************************************************
 *   Copyright (C) 2026 by Stefan Persson                                  *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "pythonprocessor.h"

#include <Python.h>
#include <QString>
#include <QtDebug>

#include <stdexcept>
#include <string.h>
#include <iostream>

// Ensures Python interpreter is initialized once and properly finalized exactly once
    // at normal program termination — without requiring changes to main.cpp
    namespace {
    struct PythonGlobalLifetime {
        PythonGlobalLifetime() {
            qDebug() << "Python interpreter initialized";
        }
        ~PythonGlobalLifetime() {
            if (Py_IsInitialized()) {
                PyGILState_STATE g = PyGILState_Ensure();
                Py_FinalizeEx();
                PyGILState_Release(g);
                qDebug() << "Python interpreter finalized";
            }
        }
    } python_lifetime_guard;
}

static void ensurePython()
{
    static bool initialized = false;
    if (!initialized) {
        Py_Initialize();
        PyEval_InitThreads();   // still required conceptually
        PyEval_SaveThread();    // release GIL for Qt threads
        initialized = true;
    }
}

static std::vector<double> toDoubleVector(PyObject* obj)
{
    if (!PySequence_Check(obj))
        throw std::runtime_error("Return value is not a sequence");

    PyObject* seq = PySequence_Fast(obj, "Expected sequence");
    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    PyObject** items = PySequence_Fast_ITEMS(seq);

    std::vector<double> out;
    out.reserve(n);

    for (Py_ssize_t i = 0; i < n; ++i) {
        out.push_back(PyFloat_AsDouble(items[i]));
    }

    Py_DECREF(seq);
    return out;
}

static QString fetchPythonError()
{
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

    QString message = "Unknown Python error";

    if (pvalue) {
        PyObject* strObj = PyObject_Str(pvalue);
        if (strObj) {
            message = PyUnicode_AsUTF8(strObj);
            Py_DECREF(strObj);
        }
    }

    QString typeName = "Exception";
    if (ptype) {
        PyObject* typeStr = PyObject_Str(ptype);
        if (typeStr) {
            typeName = PyUnicode_AsUTF8(typeStr);
            Py_DECREF(typeStr);
        }
    }

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);

    return QString("%1: %2").arg(typeName, message);
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

    // The following 7 lines handles reload.
    PyObject* sysModules = PyImport_GetModuleDict(); // borrowed
    PyDict_DelItemString(sysModules, py_file_name);
    // Ignore error if not present
    PyErr_Clear();
    PyObject* name = PyUnicode_FromString(py_file_name);
    impl->module = PyImport_Import(name);
    Py_DECREF(name);

    if (!impl->module) {
        QString err = fetchPythonError();
        throw std::runtime_error(err.toStdString());
    }

    if (!PyModule_Check(impl->module)) {
        throw std::runtime_error("Loaded object is not a Python module");
    }

    PyObject* file = PyObject_GetAttrString(impl->module, "__file__");

    if (!file) { // maybe remove
        QString pyError = fetchPythonError();
        throw std::runtime_error(pyError.toStdString());
    }
    if (file) {  // maybe remove
        qDebug() << "Loaded Python module: " << PyUnicode_AsUTF8(file);
        Py_DECREF(file);
    }

    if (!impl->module) // maybe remove
    {
        throw std::runtime_error("Failed to import Python module");}

    PyObject* cls = PyObject_GetAttrString(impl->module, class_name);
    if (!cls || !PyCallable_Check(cls)){
        PyErr_Print();
        char throw_message[60] = "Processor class not found: ";
        strncat(throw_message, class_name, 30);
        throw std::runtime_error(throw_message);
    }

    impl->instance = PyObject_CallObject(cls, NULL);
    Py_DECREF(cls);

    if (!impl->instance)
        throw std::runtime_error("Failed to create Processor instance");
}

PythonProcessor::~PythonProcessor()
{

    //PyGILState_STATE gil = PyGILState_Ensure();

    PythonGIL gil;

    Py_XDECREF(impl->instance);
    Py_XDECREF(impl->module);

    //PyGILState_Release(gil);

    delete impl;
}

std::vector<double> PythonProcessor::process(
    uint64_t simTime,
    const std::vector<double>& input)
{
    // Convert simTime
    PyObject* pyTime = PyLong_FromUnsignedLongLong(simTime);

    // Convert input vector -> Python list
    PyObject* pyList = PyList_New(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        PyList_SetItem(pyList, i, PyFloat_FromDouble(input[i]));
        // PyList_SetItem steals reference
    }

    PyObject* result = PyObject_CallMethod(
        impl->instance,
        "process",
        "OO",
        pyTime,
        pyList
        );

    Py_DECREF(pyTime);
    Py_DECREF(pyList);

    if (!result) {
        QString pyError = fetchPythonError();
        qCritical() << "[Python]" << pyError;
        throw std::runtime_error("Python process() internal failed. " + pyError.toStdString());
    }

    std::vector<double> output;
    try {
        output = toDoubleVector(result);
    } catch (...) {
        Py_DECREF(result);
        throw std::runtime_error("Python process() must return a sequence of numbers "
                                 "(list, tuple, or NumPy array)");
    }
    Py_DECREF(result);
    return output;
}
