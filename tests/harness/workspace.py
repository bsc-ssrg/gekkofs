################################################################################
#  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain           #
#  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany         #
#                                                                              #
#  This software was partially supported by the                                #
#  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).   #
#                                                                              #
#  This software was partially supported by the                                #
#  ADA-FS project under the SPPEXA project funded by the DFG.                  #
#                                                                              #
#  SPDX-License-Identifier: MIT                                                #
################################################################################

from loguru import logger

class Workspace:
    """
    Test workspace, implements a self-contained subdirectory where a test
    can run and generate artifacts in a self-contained manner.
    """

    def __init__(self, twd, bindirs, libdirs):
        """
        Initializes the test workspace by creating the following directories
        under `twd`:

            ./twd/
            ├──logs/
            ├──root/
            └──mnt/

        Parameters
        ----------
        twd: `pathlib.Path`
            Path to the test working directory. Must exist.
        bindirs: `list(pathlib.Path)`
            List of paths where programs required for the test should 
            be searched for.
        libdirs: `list(pathlib.Path)`
            List of paths where libraries required for the test should be
            searched for.
        """
        logger.info(f"workspace created at {twd}")

        self._twd = twd
        self._bindirs = bindirs
        self._libdirs = libdirs
        self._logdir = self._twd / 'logs'
        self._rootdir = self._twd / 'root'
        self._mountdir = self._twd / 'mnt'

        self._logdir.mkdir()
        self._rootdir.mkdir()
        self._mountdir.mkdir()

    @property
    def twd(self):
        return self._twd

    @property
    def bindirs(self):
        return self._bindirs

    @property
    def libdirs(self):
        return self._libdirs

    @property
    def logdir(self):
        return self._logdir

    @property
    def rootdir(self):
        return self._rootdir

    @property
    def mountdir(self):
        return self._mountdir
