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

import os, sys
import pytest
import logging
from _pytest.logging import caplog as _caplog
from loguru import logger
from pathlib import Path
from harness.cli import add_cli_options
from harness.workspace import Workspace
from harness.gkfs import Daemon, Client

def pytest_addoption(parser):
    """
    Adds extra options from the GKFS harness to the py.test CLI.
    """
    add_cli_options(parser)

@pytest.fixture(autouse=True)
def caplog(_caplog):

    _caplog.set_level(logging.CRITICAL, 'sh.command')
    _caplog.set_level(logging.CRITICAL, 'sh.command.process')
    _caplog.set_level(logging.CRITICAL, 'sh.command.process.streamreader')
    _caplog.set_level(logging.CRITICAL, 'sh.stream_bufferer')
    _caplog.set_level(logging.CRITICAL, 'sh.streamreader')

    class PropagateHandler(logging.Handler):
        def emit(self, record):
            logging.getLogger(record.name).handle(record)

    handler_id = logger.add(PropagateHandler(), format="{message}")
    yield _caplog
    logger.remove(handler_id)

@pytest.fixture
def test_workspace(tmp_path, request):
    """
    Initializes a test workspace by creating a temporary directory for it.
    """

    return Workspace(tmp_path,
            request.config.getoption('--bin-dir'),
            request.config.getoption('--lib-dir'))

@pytest.fixture
def gkfs_daemon(test_workspace, request):
    """
    Initializes a local gekkofs daemon
    """

    interface = request.config.getoption('--interface')

    daemon = Daemon(interface, test_workspace)
    yield daemon.run()
    daemon.shutdown()

@pytest.fixture
def gkfs_client(test_workspace):
    """
    Sets up a gekkofs client environment so that
    operations (system calls, library calls, ...) can 
    be requested from a co-running daemon.
    """

    return Client(test_workspace)
