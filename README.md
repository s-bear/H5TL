H5TL: HDF5 C++ Template Library
===============================

This library is a C++ wrapper of the excellent HDF5 Library. It is a single-header library that does not depend on the official HDF5 C++ bindings. The purpose of this library is to provide an easy-to-use, basic level of functionality for reading and writing HDF5 files.

The library includes compatibility for
- OpenCV Mat
- Blitz++ Array
- std::vector and std::array

TODO:
- [ ] Get a first version working
- [ ] Work out APIs for just about everything


Example
-------

```C++

#include "H5TL.hpp"
#include <vector>
#include <algorithm>
using namespace std;

int main(int argc, char *argv[]) {
    H5TL::File mFile("test.h5");
    mFile.write("note","This file was created using the H5TL wrapper library");

    //writing
    vector<int> a(10,1);
    partial_sum(a.begin(),a.end(),a.begin());
    mFile.write("data\a",a);

    //reading
    vector<float> b;
    mFile.read("data\a",b); //doesn't work yet :(
}

```
