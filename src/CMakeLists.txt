# @@@LICENSE
#
#      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(NYXLIB REQUIRED nyx)
include_directories(${NYXLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${NYXLIB_CFLAGS_OTHER})

webos_add_linker_options(ALL --no-undefined --as-needed)
webos_add_compiler_flags(ALL -Wall -g -std=c99)
webos_add_compiler_flags(ALL -D_GNU_SOURCE=1)
webos_add_compiler_flags(DEBUG -O0 -DDEBUG -D_DEBUG)
webos_add_compiler_flags(RELEASE -DNDEBUG)

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=nyx_module_dir nyx
		OUTPUT_VARIABLE nyx_module_dirpath
		RESULT_VARIABLE pkgconfig_failed)

if(NOT pkgconfig_failed)
	STRING(REGEX REPLACE "(/?\r?\n)+$" "" nyx_module_dirpath "${nyx_module_dirpath}")
	set(NYX_MODULE_DIR ${nyx_module_dirpath} CACHE PATH "Nyx modules directory path")
else()
	message(FATAL_ERROR "Error in fetching 'nyx_module_dir' from nyx.pc, execute_process() returned ${pkgconfig_failed}")
endif()

# Uncomment the following lines if mock modules are required

#execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=nyx_module_mock_dir nyx
#		OUTPUT_VARIABLE nyx_module_mock_dirpath
#		RESULT_VARIABLE pkgconfig_failed)
#
#if(NOT pkgconfig_failed)
#	STRING(REGEX REPLACE "(/?\r?\n)+$" "" nyx_module_mock_dirpath "${nyx_module_mock_dirpath}")
#	set(NYX_MODULE_MOCK_DIR ${nyx_module_mock_dirpath} CACHE PATH "Nyx mock modules directory path")
#else()
#	message(FATAL_ERROR "Error in fetching 'nyx_module_mock_dir' from nyx.pc, execute_process() returned ${pkgconfig_failed}")
#endif()

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
