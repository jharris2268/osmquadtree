from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext
from distutils.sysconfig import customize_compiler

import shutil, os.path
import pybind11

xmlinspector_path = os.path.abspath("../thirdparty/xmlinspector")

if not os.path.exists(os.path.join(xmlinspector_path, "XmlInspector.hpp")):
    raise Exception("specify path to xml-inspector headers")

#postgresql_path = '/usr/include/postgresql'
#if not os.path.exists(os.path.join(postgresql_path, "libpq-fe.h")):
#    raise Exception("specify path to postgresql (libpq) headers")


#srcs=['src/'+f for f in ("oqt_python.cpp","block_python.cpp", "core_python.cpp","change_python.cpp", "geometry_python.cpp", "postgis_python.cpp")]




#libs=['z','pq','stdc++fs', 'oqt']
libs=['z','stdc++fs', 'oqt']

class my_build_ext(build_ext):
    def build_extensions(self):
        customize_compiler(self.compiler)
        try:
            self.compiler.compiler_so.remove("-Wstrict-prototypes")
        except (AttributeError, ValueError):
            pass
        build_ext.build_extensions(self)




ext_modules = []

mods = {
    'calcqts._calcqts': ['calcqts_python.cpp'],
    'count._count': ['count_python.cpp'],
    'elements._elements': ['elements_python.cpp'],
    'geometry._geometry': ['geometry_python.cpp'],
    #'geometry.postgis._postgis': ['postgis_python.cpp'],
    'pbfformat._pbfformat': ['pbfformat_python.cpp'],
    'sorting._sorting': ['sorting_python.cpp'],
    'update._update': ['update_python.cpp'],
    'utils._utils': ['utils_python.cpp'],
}


for modname,srcs in mods.items():
    ext_modules.append(
    
        Extension(
                'oqt.%s' % (modname,),
                ['src/%s' % s for s in srcs],
                include_dirs=[
                    '/usr/local/include',
                    os.path.abspath('../include/'),
                    os.path.abspath('../thirdparty/'),
                    pybind11.get_include(),
                    xmlinspector_path,
                    #postgresql_path,
                ],
                libraries=libs,
                extra_compile_args=['-std=c++17','-DINDIVIDUAL_MODULES'],
                
        )
    )
    


setup(
    name='oqt',
    #packages=['oqt','oqt.geometry','oqt.geometry.postgis','oqt.utils','oqt.elements','oqt.pbfformat','oqt.update','oqt.sorting','oqt.calcqts','oqt.count'],
    packages=['oqt','oqt.geometry','oqt.utils','oqt.elements','oqt.pbfformat','oqt.update','oqt.sorting','oqt.calcqts','oqt.count'],
    scripts=['scripts/oqt_initial.py', 'scripts/oqt_update.py'],
    version='0.0.1',
    long_description='',
    ext_modules=ext_modules,
    zip_safe=False,
    cmdclass = {'build_ext': my_build_ext},    
)
