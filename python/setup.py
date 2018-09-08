from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext
from distutils.sysconfig import customize_compiler

import shutil, os.path

xmlinspector_path = os.path.abspath("../thirdparty/xmlinspector")

if not os.path.exists(os.path.join(xmlinspector_path, "XmlInspector.hpp")):
    raise Exception("specify path to xml-inspector headers")

postgresql_path = '/usr/include/postgresql'
if not os.path.exists(os.path.join(postgresql_path, "libpq-fe.h")):
    raise Exception("specify path to postgresql (libpq) headers")


srcs=['src/'+f for f in ("oqt_python.cpp","block_python.cpp", "core_python.cpp","change_python.cpp", "geometry_python.cpp")]


src_path = os.path.abspath('../src')
oqt_src = [os.path.join(src_path,t) for t in
     """calcqts.cpp combineblocks.cpp count.cpp mergechanges.cpp
        minimalblock.cpp packedblock.cpp primitivegroup.cpp quadtree.cpp
        quadtreegroups.cpp readblock.cpp readfile.cpp simplepbf.cpp sortblocks.cpp
        sortfile.cpp store.cpp tempobjs.cpp utils.cpp waynodes.cpp writeblock.cpp
        writefile.cpp writepbffile.cpp xmlchange.cpp update.cpp
        geometry/addwaynodes.cpp geometry/geometrytypes.cpp geometry/makegeometries.cpp
        geometry/addparenttags.cpp geometry/handlerelations.cpp geometry/multipolygons.cpp
        geometry/postgiswriter.cpp""".split()]


ela = []
libs=['z','pq','stdc++fs']

if False:
    srcs+=oqt_src
else:
    ela = ['-L/home/james/.local/lib']
    libs.append('oqt')


class my_build_ext(build_ext):
    def build_extensions(self):
        customize_compiler(self.compiler)
        try:
            self.compiler.compiler_so.remove("-Wstrict-prototypes")
        except (AttributeError, ValueError):
            pass
        build_ext.build_extensions(self)



ext_modules = [
    Extension(
        'oqt._oqt',
        srcs,
        include_dirs=[
            '/usr/local/include',
            os.path.abspath('../include/'),
            os.path.abspath('../thirdparty/'),
            xmlinspector_path,
            postgresql_path,
        ],
        libraries=libs,
        extra_compile_args=['-std=c++14',],
        extra_link_args=ela,
        
    ),
    
]



setup(
    name='oqt',
    packages=['oqt'],
    version='0.0.1',
    long_description='',
    ext_modules=ext_modules,
    zip_safe=False,
    cmdclass = {'build_ext': my_build_ext},    
)
