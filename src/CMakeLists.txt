# @@@LICENSE
#
#      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
#      Copyright (c) 2013 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# LICENSE@@@

webos_machine_dep()
webos_machine_impl_dep()
webos_core_os_dep()

# By default specify all modules to be compiled
set(MODULE_SYSTEM_WEBOS_LINUX     YES CACHE BOOL "Include 'system' modules")
set(MODULE_KEYS_WEBOS_LINUX       YES CACHE BOOL "Include 'keys' module")
set(MODULE_TOUCHPANEL_WEBOS_LINUX YES CACHE BOOL "Include 'touchpanel' module")
set(MODULE_GENERIC_OS_INFO        YES CACHE BOOL "Include generic 'OSInfo' module")
set(MODULE_GENERIC_DEVICE_INFO    YES CACHE BOOL "Include generic 'DeviceInfo' module")
set(MODULE_GENERIC_SECURITY       YES CACHE BOOL "Include generic 'Security' module")
set(MODULE_GENERIC_DISPLAY        YES CACHE BOOL "Include generic 'Display' module")

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(NYXLIB REQUIRED nyx)
include_directories(${NYXLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${NYXLIB_CFLAGS_OTHER})

pkg_check_modules(LIBCRYPTO REQUIRED libcrypto)
include_directories(${LIBCRYPTO_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LIBCRYPTO_CFLAGS_OTHER})

webos_add_linker_options(ALL --no-undefined --as-needed)
webos_add_compiler_flags(ALL -Wall -g -std=c99)
webos_add_compiler_flags(ALL -D_GNU_SOURCE=1)
webos_add_compiler_flags(DEBUG -O0 -DDEBUG -D_DEBUG)
webos_add_compiler_flags(RELEASE -DNDEBUG)

function(nyx_get_pkgconfig_var varname nyx_pkgconfig_var usecache)
	execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=${nyx_pkgconfig_var} nyx
	                OUTPUT_VARIABLE pkgconfig_output
	                RESULT_VARIABLE pkgconfig_failed)

	if(NOT pkgconfig_failed)
		STRING(REGEX REPLACE "(/?\r?\n)+$" "" pkgconfig_output "${pkgconfig_output}")
		if(usecache)
			set(${varname} ${pkgconfig_output} CACHE PATH ${nyx_pkgconfig_var})
		else()
			set(${varname} ${pkgconfig_output} PARENT_SCOPE)
		endif()
	else()
		message(FATAL_ERROR "Error in fetching '${nyx_pkgconfig_var}' from nyx.pc, execute_process() returned ${pkgconfig_failed}")
	endif()
endfunction()

nyx_get_pkgconfig_var(NYX_MODULE_DIR nyx_module_dir TRUE)
nyx_get_pkgconfig_var(NYX_MODULE_PREFIX nyx_module_prefix FALSE)
nyx_get_pkgconfig_var(NYX_MODULE_SUFFIX nyx_module_suffix FALSE)
# Uncomment the following line if mock modules are required
#nyx_get_pkgconfig_var(NYX_MODULE_MOCK_DIR nyx_module_mock_dir TRUE)

if(NOT(NYX_MODULE_PREFIX STREQUAL ""))
	set (CMAKE_SHARED_MODULE_PREFIX ${NYX_MODULE_PREFIX})
endif()

if(NOT(NYX_MODULE_SUFFIX STREQUAL ""))
	set (CMAKE_SHARED_MODULE_SUFFIX ${NYX_MODULE_SUFFIX})
endif()

macro(nyx_create_module module)
	add_library(${module} MODULE ${ARGN})
	target_link_libraries(${module} ${GLIB2_LDFLAGS} ${NYXLIB_LDFLAGS} -lrt -lpthread)
	install(TARGETS ${module} DESTINATION ${NYX_MODULE_DIR})
endmacro()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/machine/${WEBOS_TARGET_MACHINE}.cmake)
	include(${CMAKE_CURRENT_SOURCE_DIR}/machine/${WEBOS_TARGET_MACHINE}.cmake)
endif()

add_subdirectory(generic)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${WEBOS_TARGET_CORE_OS}")
	add_subdirectory(${WEBOS_TARGET_CORE_OS})
endif()
