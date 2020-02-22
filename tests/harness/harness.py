import os
import sys
import decorator
import sh
import contextlib
from multiprocessing import Process, set_start_method

bin_path = ['/home/amiranda/var/projects/gekkofs/prefix.memalign/bin/']
libs_path = '/home/amiranda/var/projects/gekkofs/prefix/lib/'


# #@contextlib.contextmanager
# class client():
#     def __init__(self, ns):
#         self._ns = ns
# 
#     def __enter__(self):
#         self._env = os.environ.copy()
#         self._env.update({
#             'LD_LIBRARY_PATH'      : libs_path,
#             'LD_PRELOAD'           : '/home/amiranda/var/projects/gekkofs/prefix/lib/libgkfs_intercept.so',
#             'LIBGKFS_LOG'          : 'all',
#             'LIBGKFS_LOG_OUTPUT'   : '/dev/stderr',
#             #'LIBGKFS_LOG_OUTPUT'   : '/home/amiranda/var/projects/gekkofs/source/tests/logfile.err',
#         })
# 
#         return self
# 
#     def __exit__(self, *exc):
#         pass
# 
#     def exec(self, fun, args_):
# 
#         saved_env = os.environ.copy()
#         os.environ = self._env
# 
#         set_start_method('spawn')
#         p = Process(target=fun, args=args_)
#         p.start()
#         p.join()
# 
#         os.environ = saved_env
# 
# class ClientFactory():
#     """ Dynamically create gekkofs client processes. """
# 
#     def __init__(self):
#         pass
# 
#     def __call__(self, ns):
#         """ Return a client process. """
# 
#         return client(ns)
# 
