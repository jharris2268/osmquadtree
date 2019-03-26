.. Copyright (c) 2019 James Harris

   Distributed under the terms of the GPL v3 license.

   The full license is in the file LICENSE, distributed with this software.



Geospatially sorted openstreetmap data

Introduction
------------

`osmquadtree`_ is a C++ library for rearranging `openstreetmap data`_ by
quadtree tile. This allows the user to extract smaller areas, or convert
the original `elements`_ into conventional `geometry`_ objects. These can
also be imported into a `postgis`_ database as an alternative to `osm2pgsql`_.

It is also possible to keep the original imported planet file up to date
using the osmchange replication files.

Python bindings allow the user to analyze reasonably large areas in memory, or
to render `mapnik`_ images without a postgis database.

`osmquadtree` requires a modern C++ compiler supporting C++17, such as gcc 7.0.
The python bindings require `pybind11`_.

Licensing
---------

This software is licensed under the GPL license. See the LICENSE file
for details.


.. toctree::
   :caption: INSTALLATION
   :maxdepth: 1

   installation
   

.. toctree::
   :caption: USAGE
   :maxdepth: 2

   getting_started
   calcqts
   sortblocks
   mergechanges
   update
   python_bindings
   process_geometry


.. toctree::
   :caption: API REFERENCE
   :maxdepth: 2

   api/calcqts
   api/elements
   api/geometry
   api/pbfformat
   api/sorting
   api/update
   api/utils

.. toctree::
   :caption: MISCELLANEOUS

   threaded_callbacks
   protocol_buffer

.. _osmquadtree: https://github.com/jharris2268/osmquadtree
.. _elements: https://wiki.openstreetmap.org/wiki/Elements
.. _openstreetmap data: https://planet.openstreetmap.org/
.. _geometry: https://en.wikipedia.org/wiki/Well-known_text_representation_of_geometry
.. _postgis: https://postgis.net/
.. _osm2pgsql: https://wiki.openstreetmap.org/wiki/Osm2pgsql
.. _pybind11: https://github.com/pybind/pybind11
.. _mapnik: https://github.com/mapnik/mapnik
