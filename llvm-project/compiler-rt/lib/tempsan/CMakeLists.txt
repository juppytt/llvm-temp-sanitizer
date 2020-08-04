include_directories(..)

set (TEMPSAN_SOURCES
    tempsan.cc)
set (TEMPSAN_HEADERS
    tempsan.h)

set (TEMPSAN_CFLAGS ${SANITIZER_COMMON_CFLAGS})
append_rtti_flag(OFF TEMPSAN_CFLAGS)
set (PPCA_DEFINITIONS)

set(TEMPSAN_DYNAMIC_LIBS ${SANITIZER_CXX_ABI_LIBRARIES} ${SANITIZER_COMMON_LINK_LIBS})

append_list_if(COMPILER_RT_HAS_LIBDL dl TEMPSAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBRT rt TEMPSAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBM m TEMPSAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBPTHREAD pthread TEMPSAN_DYNAMIC_LIBS)
append_list_if(COMPILER_RT_HAS_LIBLOG log TEMPSAN_DYNAMIC_LIBS)

if (TARGET cxx-headers OR HAVE_LIBCXX)
  set (TEMPSAN_DEPS cxx-headers)
endif()

# Compile TEMPSAN sources into an object library

add_compiler_rt_object_libraries(RTTempSanitizer
  ARCHS ${X86_64}
  SOURCES ${TEMPSAN_SOURCES}
  ADDITIONAL_HEADERS ${TEMPSAN_HEADERS}
  CFLAGS ${TEMPSAN_CFLAGS}
  DEFS ${TEMPSAN_DEFINITIONS}
  DEPS ${TEMPSAN_DEPS})

# Build TEMPSAN runtimes shipped with Clang.
add_compiler_rt_component(tempsan)
set(TEMPSAN_COMMON_RUNTIME_OBJECT_LIBS
    RTInterception
    RTSanitizerCommon
    RTSanitizerCommonLibc
    RTSanitizerCommonCoverage
    RTSanitizerCommonSymbolizer)

add_compiler_rt_runtime(clang_rt.tempsan
  STATIC
  ARCHS ${X86_64}
  OBJECT_LIBS RTTempSanitizer
              ${TEMPSAN_COMMON_RUNTIME_OBJECT_LIBS}
  CFLAGS ${TEMPSAN_CFLAGS}
  PARENT_TARGET tempsan)

if (SANITIZER_USE_SYMBOLS AND NOT ${arch} STREQUAL "i386")
  add_sanitizer_rt_symbols(clang_rt.tempsan
    ARCHS ${X86_64}
    EXTRA tempsan.syms.extra)
  add_dependencies(tempsan clang_rt.ppac-${X86_64}-symbols)
endif()

add_dependencies(compiler-rt tempsan)
