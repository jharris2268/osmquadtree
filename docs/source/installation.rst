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


.. _xmlinspector: https://www.codeproject.com/Articles/587488/Streaming-XML-parser-in-Cplusplus
.. _picojson: https://github.com/kazuho/picojson
.. _included: https://github.com/jharris2268/osmquadtree/tree/master/thirdparty


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
    oqt count example.pbf


    
