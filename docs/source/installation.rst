Installation
============

Dependencies
------------

Osmquadtree uses CMake to build and install the library. A c++17 compiler
and standard library is required, e.g. gcc version 7. If another compiler
is used then the CMakeLists.txt file in the root directory will need to be
edited.

The only requirements for the osmquadtree c++ library are zlib (debian
package zlib1g-dev), and the header-only libraries:

* gzstream.hpp 
* xmlinspector_
* picojson_

All three are included_ in the osmquadtree repository.





C++ library
-----------

To clone the github repository, build and install, type:

.. code::
    
    git clone https://github.com/jharris2268/osmquadtree.git
    cd osmquadtree
    
    mkdir build
    cd build
    cmake ../
    
    make
    sudo make install


This will install the liboqt.so library under /usr/local/lib, the headers
under /usr/local/include and the demo executable under /usr/local/bin.

To use the library you must have /usr/local/lib in your LD_LIBRARY_PATH.
If it isn't, add
.. code::
    export LD_LIBRARY_PATH=$LD_LIBARY_PATH:/usr/local/lib

to your ${HOME}/.bashrc file.

To use the demo executable you must have /usr/local/bin in your PATH.
If it isn't, add
.. code::
    export PATH=$PATH:/usr/local/lib

to your ${HOME}/.bashrc file.

To see if the installation has worked, try running
.. code::
    cd ..
    oqt count example.pbf

Which should produce:
.. code::

    [      0.0s]: numchan=4
    [      0.0s]: count fn=example.pbf, numchan=4, countgeom=0, countflags=0
    [      0.0s]: 
    [0]: 3 (  81.7,      0.3)
    nodes: 6979 objects: 107715 => 6372573324 [2007-01-28T20:56:22 => 2019-03-30T16:25:20] {-1396598, 514858926, -734935, 515353624}
    ways:  1084 objects: 2198814 => 676780197 [2008-09-18T23:47:13 => 2019-03-30T15:58:17] {9216 refs, 107715 to 6337577706. Longest: 446}
    rels:  278 objects: 12176 => 9194319 [2012-09-17T09:44:19 => 2019-03-31T14:19:42] {397 N, 1388 W, 172 R. Longest: 43, 0 empties}
    pid = 20108 VmRSS:	    6440 kB
    [      0.0s]: count:     0.0s. 
    TOTAL:     0.0s. 



Python Bindings
---------------

The python bindings require pybind11_. To install:

.. code::
    git clone https://github.com/pybind/pybind11.git
    sudo cp -R pybind11/include/pybind11 /usr/local/include/
    
You may need to install the python dev package. On ubuntu:

.. code::
    sudo apt install python-dev  # for python version 2
    sudo apt install python3-dev # for python version 3


To build and install:

.. code::
    cd python
    python setup.py install --user  # for python version 2
    python3 setup.py install --user # for python version 3

To see if the installation has worked, try running

.. code:
    cd ..
    python
    
    >>> import oqt
    >>> print oqt.run_count('example.pbf')


Which should produce the same output as the previous example.


.. _xmlinspector: https://www.codeproject.com/Articles/587488/Streaming-XML-parser-in-Cplusplus
.. _picojson: https://github.com/kazuho/picojson
.. _included: https://github.com/jharris2268/osmquadtree/tree/master/thirdparty
.. _pybind11: https://github.com/pybind/pybind11/
