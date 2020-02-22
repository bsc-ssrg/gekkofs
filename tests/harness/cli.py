from pathlib import Path

### This code is meant to be included automatically by CMake in the build 
### directory's top-level conftest.py as well as the source directory's 
### conftest.py, so that tests can be correctly run from both directories
def add_cli_options(parser):
    """
    Adds extra options to the py.test CLI so that we can provide
    search directories for libraries and helper programs.
    """

    try:
        parser.addoption(
            '--interface',
            action='store',
            type=str,
            default='lo',
            help="network interface used for communications (default: 'lo')."
        )

        parser.addoption(
            "--bin-dir",
            action='append',
            default=[Path.cwd()],
            help="directory that should be considered when searching "
                "for programs (multi-allowed)."
        )

        parser.addoption(
            "--lib-dir",
            action='append',
            default=[Path.cwd()],
            help="directory that should be considered when searching "
                "for libraries (multi-allowed)."
        )
    except ValueError:
        # if the CLI args have already been added, we have been called both 
        # from the build directory's conftest.py and from the source 
        # directory's conftest.py through automatic finding, ignore the error
        pass
