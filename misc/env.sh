#!/bin/sh
# This script is copied in to the sandbox/ directory during any sandbox make
# target and is used to create an isolated environment for uzbl-core,
# uzbl-browser and uzbl-tabbed to be run and tested.

# It would be better to use something more flexible like $(dirname $0) but I
# couldn't get to work nicely from the Makefile:
#  - Sourcing file gives $0 == /bin/sh
#  - Executing limits scope of variables too much (even with exporting)
# Maybe we should spawn processes from here with an 'exec' at the end?

# Re-define our home location inside the sandbox dir.
HOME=$(pwd)/sandbox/home
export HOME

# Export default XDG_{DATA,CACHE,..}_HOME locations inside the sandbox
# directory according to defaults in the xdg specification.
# <http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html>
XDG_DATA_HOME=$HOME/.local/share
export XDG_DATA_HOME

XDG_CACHE_HOME=$HOME/.cache
export XDG_CACHE_HOME

XDG_CONFIG_HOME=$HOME/.config
export XDG_CONFIG_HOME

# Needed to run uzbl-browser etc from here.
PATH="$(pwd)/sandbox/usr/local/bin:$(pwd)/sandbox/usr/bin:$PATH"
export PATH

# Figure out where setup have installed stuff
PYTHONLIB=$(python3 -c '
from distutils.dist import Distribution
from distutils.command.install import install
dummy = install(Distribution())
dummy.finalize_options()
print(dummy.install_lib)')

UZBL_PLUGIN_PATH="$(pwd)/sandbox/$PYTHONLIB/uzbl/plugins"
export UZBL_PLUGIN_PATH

PYTHONPATH="$(pwd)/sandbox/$PYTHONLIB/"
export PYTHONPATH

if [ ! -d "${PYTHONPATH}" ]; then
    echo "$0: bad python path for sandbox: ${PYTHONPATH}"
    exit 1
fi
