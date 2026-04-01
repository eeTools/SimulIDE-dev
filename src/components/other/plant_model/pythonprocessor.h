/***************************************************************************
 *   Copyright (C) 2026 by Stefan Persson                                  *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

//#define PY_SSIZE_T_CLEAN

// //#ifdef slots
// //#undef slots
// //#endif

// //#include <Python.h>

#include <vector>
#include <cstdint>
#include <QString>
#include <QtDebug>

// ────────────────────────────────────────────────
// Protect Python.h from Qt's slots macro
#ifdef slots
#  undef slots
#endif

#include <Python.h>

// Restore Qt's macro right away (important!)
#ifdef QT_NO_KEYWORDS
// nothing — already using no-keywords mode
#else
#  ifndef slots
#    define slots
#  endif
#endif
// ────────────────────────────────────────────────

class PythonGIL {
public:
    PythonGIL()  { m_state = PyGILState_Ensure(); }
    ~PythonGIL() { PyGILState_Release(m_state);   }
private:
    PyGILState_STATE m_state;
};

class PythonProcessor {
public:
    PythonProcessor();
    ~PythonProcessor();

    // This function **require** that GIL is already held by the caller
    void PythonProcessorLoadFile(const char* dir, const char* name, const char* class_name);

    // This function **require** that GIL is already held by the caller
    std::vector<double> process(uint64_t simTime,
                                const std::vector<double>& input);

private:
    struct Impl;
    Impl* impl;
    std::string py_folder_name, py_file_name;

};



