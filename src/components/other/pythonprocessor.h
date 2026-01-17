#pragma once

//#define PY_SSIZE_T_CLEAN

// //#ifdef slots
// //#undef slots
// //#endif

// //#include <Python.h>

#include <iostream>
#include <utility>


class PythonProcessor {
public:
    PythonProcessor();
    ~PythonProcessor();

    void PythonProcessorLoadFile(const char* dir, const char* name, const char* class_name);

    std::pair<int, int> process(int value);

    /*void setPyFileName(std::string set_py_folder_name, std::string set_py_file_name){
        py_folder_name = set_py_folder_name;
        py_file_name = set_py_file_name;
    }*/

private:
    struct Impl;
    Impl* impl;
    std::string py_folder_name, py_file_name;

};

