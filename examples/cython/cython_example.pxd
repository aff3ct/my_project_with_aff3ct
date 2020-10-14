cdef extern from "src/cython_example.hpp":
    void hello(const char *name)
    void world(int size)