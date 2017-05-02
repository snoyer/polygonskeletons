from setuptools import setup
from setuptools.command.install_lib import install_lib
from setuptools.command.build_ext import build_ext
import os,subprocess

PACKAGE_NAME = 'polygonskeletons'

class InstallLibCommand(install_lib):
    """Custom install_lib command. Calls cmake and make to compile CGAL executable"""
    def run(self) :
        install_lib.run(self)

        # tmp directory we're running from, and final path we want the executable at
        dir = os.path.dirname(__file__)
        bin_path = os.path.join(self.install_dir, PACKAGE_NAME, 'skeleton-cgal')

        # possible precompiled executable in `dev-build` dir
        # (shouldn't be there in prod, just saves time when I test)
        builddir = os.path.join(dir, 'cgal-build')
        
        #TODO use finr_executable, check results, raise pretty errors
        cmd_cmake = ['cmake', '../cgal']
        cmd_make = ['make']
        os.makedirs(builddir)
        subprocess.check_call(cmd_cmake, cwd=builddir)
        subprocess.check_call(cmd_cmake, cwd=builddir) # first time around doesn't set c++ flags
        subprocess.check_call(cmd_make,  cwd=builddir)
        subprocess.check_call(['mv', 'skeleton-tgf', bin_path], cwd=builddir)



setup(
    name=PACKAGE_NAME,
    version='0.1',
    description='compute straight skeletons on polygons',
    url='http://github.com/snoyer/polygonskeletons',
    author='snoyer',
    author_email='reach me on github',
    packages=[PACKAGE_NAME],
    install_requires=[
        'networkx',
    ],
    zip_safe=False,
    cmdclass = {
        'install_lib': InstallLibCommand,
    }
)
