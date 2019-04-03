#ifndef GKFS_TRACING_HPP
#define GKFS_TRACING_HPP


#ifdef GKFS_USE_EXTRAE
#include <extrae.h>
#include <dlfcn.h>
#include <stdexcept>
#include <string>
#endif


#ifdef GKFS_USE_EXTRAE
#define TRACE_FUNC_START() \
    Extrae_user_function(1);
#else
#define TRACE_FUNC_START() \
    do { } while(0);
#endif // GKFS_USE_EXTRAE

#ifdef GKFS_USE_EXTRAE
#define TRACE_FUNC_STOP() \
    Extrae_user_function(0);
#else
#define TRACE_FUNC_STOP() \
    do { } while(0);
#endif // GKFS_USE_EXTRAE


#ifdef GKFS_USE_EXTRAE

namespace gkfs { namespace trace {
    
void init();

}}

#endif


#endif // GKFS_EXTRAE_HPP
