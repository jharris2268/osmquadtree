Getting Started
===============

Osmquadtree works on both the full planet file, available from
planet.openstreetmap.org_ or mirrors_, or on extracts, such as those
from _geofabrik. Processing a full planet file requires 12gb of RAM.
Smaller extracts (up to 2gb in file size) require 4gb RAM.

To process a planet file:

1. Download
-----------

Planet files are prepared weekly, appearing three days after the specified
date.

.. code::

    $ wget https://planet.openstreetmap.org/pbf/planet-190429.osm.pbf -O planet-20190429.pbf
    $ wget https://planet.openstreetmap.org/pbf/planet-190429.osm.pbf.md5 -O planet-20190429.pbf.md5
    $ md5sum -c planet-20190429.pbf.md5

This can take several hours to complete.

2. Calculate object quadtrees
-----------------------------

.. code::

    $ oqt calcqts planet-20190429.pbf
    
This will produce a file called planet-20190429-qts.pbf. This is pbf
format file, with the osmquadtree extensions, but with only the id and
quadtree fields present. This takes approximately 90mins to run on
my pc (i5 2300, with 12gb RAM).

3. Sort blocks
--------------

.. code::

    $ mkdir planet-apr2019
    $ oqt sortblocks planet-20190429.pbf -outfn=planet-apr2019/20190429.pbf
    
This will rearrange the planet file into approximately 150000 blocks of
40000 objects. This takes about 3.5 hours to run, and needs 60gb of disk
space to write temporary files.

4. Initial update
-----------------

.. code::

    $ mkdir repl_diffs
    $ oqt_initial.py planet-apr2019/ 20190429.pbf 2019-04-29T00:00:00 /home/james/map_data/repl_diffs/ 2430

This will create an index file for planet-apr2019/20190429.pbf and two
settings files (settings.json and filelist.json). This takes about 25mins.

5. Update to current
--------------------

.. code::

    $ oqt_update.py planet-apr2019/

This will produce an update .pbfc file for each day since the initial update
timestamp. This takes approximately 10min per day.


    
    














.. _planet.openstreetmap.org: https://planet.openstreetmap.org/
.. _mirrors: https://wiki.openstreetmap.org/wiki/Planet.osm#Planet.osm_mirrors
.. _geofabrik: https://download.geofabrik.de/
