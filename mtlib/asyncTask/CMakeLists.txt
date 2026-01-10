cmake_minimum_required (VERSION 3.10)

project(asyncTask)

include_directories(..)

set(SOURCES
    "AsyncTask.cpp"
    "AsyncTaskQueue.cpp")

add_library(asyncTask
            ${SOURCES})
