#
#	Boilerplate CMake configuration for a cross-platform library, 
#	including configuration for a static framework on Xcode/macOS/iOS
#
#	Designed by Fletcher T. Penney
#	See the LICENSE file for copyright and licensing information.
#

cmake_minimum_required(VERSION 3.28)


# ===================
# Customize This Part
# ===================

# Project Description

set (My_Project_Title "libMultiMarkdown7")
set (My_Project_Description "C parser for Markdown with additional features and multiple output formats.")
set (My_Project_Author "Fletcher T. Penney")
set (My_Project_Revised_Date "2026-03-13")
set (My_Project_Version_Major 7)
set (My_Project_Version_Minor 0)
set (My_Project_Version_Patch 0)
# set (My_Project_Version_Prerelease -beta.1)

set (My_Project_Copyright_Date "2024-2026")

set (My_Project_Identifier "net.fletcherpenney.multimarkdown")

string(TIMESTAMP My_Build_Version "%Y%m%d.%H.%M" UTC)
string(TIMESTAMP My_Build_Date "%Y-%m-%d" UTC)


# Source files and headers

set(src_files
	src/mmd_core.c
	src/mmd_scanner.c
	src/mmd_line_scanner.c
	src/mmd_node.c
	src/mmd_node_pool.c
	src/criticmarkup.c
	src/transclude.c

	src/base64.c
	src/char.c
	src/stack.c
	src/text_buffer.c
	src/mmd_tokenizer.c
	src/mmd_token_scanner.c
	src/mmd_span_parser.c
	src/mmd_utilities.c

	src/mmd_parser_hand_2.c

	src/read_ctx.c

	src/export/assets.c
	src/export/dc.c
	src/export/export_core.c
	src/export/write_ctx.c

	src/export/ast.c
	src/export/docx.c
	src/export/epub.c
	src/export/html.c
	src/export/latex.c
	src/export/outline.c
	src/export/textbundle.c

	src/import/outline.c
	src/import/html.c

	src/aho-corasick.c

	src/vector_line_node.c

	src/miniz.c
	src/yxml.c
	src/zip.c
)

set(public_headers
	src/libMultiMarkdown7.h
)

set(private_headers
	src/mmd_core.h
	src/mmd_scanner.h
	src/mmd_line_scanner.h
	src/mmd_node.h
	src/mmd_node_pool.h
	src/criticmarkup.h
	src/transclude.h

	src/base64.h
	src/char.h
	src/stack.h
	src/text_buffer.h
	src/mmd_tokenizer.h
	src/mmd_token_scanner.h
	src/mmd_span_parser.h
	src/mmd_utilities.h

	src/mmd_parser_hand_2.h

	src/read_ctx.h

	src/export/assets.h
	src/export/dc.h
	src/export/export_core.h
	src/export/write_ctx.h

	src/export/ast.h
	src/export/docx.h
	src/export/epub.h
	src/export/html.h
	src/export/latex.h
	src/export/outline.h
	src/export/textbundle.h

	src/import/outline.h
	src/import/html.h

	src/aho-corasick.h

	src/vector_line_node.h

	src/miniz.h
	src/yxml.h
	src/zip.h
	
	version.h
)

set(resources
)

source_group("Resources" FILES ${resources})

set(main_storyboard
)

# Include other CMake projects?
# add_subdirectory(submodules/foo)

# Libraries for linking
# on macOS can link to frameworks using: "-framework QuartzCore", for example
set(libraries_to_link
)

# This is an ugly hack, but it seems to work.
# When linking to a framework in Xcode, we need to be able to
# search for public headers within that framework
# "$SRCROOT/build-xcode/submodules/libFoo/$<CONFIG>/libFoo.framework/.."
# 
set(framework_search_paths
)

# Link to Cocoa Framework?
# set (link_to_cocoa 1)

# Use Assets for app icon set for iOS or macOS application?
# (Assets stored in resources/Assets.xcassets)
# set (use_assets 1)

# Enable ARC for Obj-C?
# set (enable_arc 1)


# =======================
# Leave the rest alone...
# =======================

# Configuration

set (My_Project_Version "${My_Project_Version_Major}.${My_Project_Version_Minor}.${My_Project_Version_Patch}")
set (My_Project_Copyright "Copyright © ${My_Project_Copyright_Date} ${My_Project_Author}.")

string(TOUPPER ${My_Project_Title} My_Project_Title_Caps  )
string(REGEX REPLACE " " "_" My_Project_Title_Caps ${My_Project_Title_Caps} )

string(REGEX REPLACE " " "" My_Project_Title_Clean ${My_Project_Title} )

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project (${My_Project_Title} VERSION "${My_Project_Version}")

if (CMAKE_GENERATOR MATCHES "Xcode")
	set (CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
else()
  if (APPLE)
		set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
  endif()
endif()

# from http://stackoverflow.com/questions/25199677/how-to-detect-if-current-scope-has-a-parent-in-cmake
get_directory_property(hasParent PARENT_DIRECTORY)

if (hasParent)
else()
	# If building the framework independently, we can set a deployment target
	set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13")
endif()


# Search source directory
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/export)
include_directories(${PROJECT_SOURCE_DIR}/src/import)
include_directories(${PROJECT_SOURCE_DIR}/test)


# Search build directory (for version.h)
include_directories(${PROJECT_BINARY_DIR})


# Configure library/framework

add_library("${My_Project_Title}"
	STATIC
	${src_files}
	${private_headers}
	${public_headers}
	${resources}
)

string (REPLACE ";" " " framework_search_paths_string "${framework_search_paths}")

set_target_properties("${My_Project_Title_Clean}"
	PROPERTIES
	PREFIX ""
	FRAMEWORK_VERSION A
	MACOSX_FRAMEWORK_IDENTIFIER "${My_Project_Identifier}"
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${My_Project_Identifier}"
	XCODE_ATTRIBUTE_MARKETING_VERSION ${My_Project_Version}
	RESOURCE "${resources}"
	XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS} ${framework_search_paths_string}"
	PUBLIC_HEADER "${public_headers}"
	ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}

	XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] YES
	XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=MinSizeRel] NO
	XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=RelWithDebInfo] NO
	XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Release] NO

	XCODE_ATTRIBUTE_SDKROOT auto
	XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator macosx"
	XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
)

# Warn for everything and treat as errors
if(MSVC)
  # target_compile_options("${My_Project_Title_Clean}" PRIVATE /W4 /WX)
else()
  target_compile_options("${My_Project_Title_Clean}" PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-implicit-fallthrough -Wno-unknown-warning-option)
endif()


# Link to other libraries
target_link_libraries("${My_Project_Title}"
	${libraries_to_link}
)

# Link to Apple Cocoa Framework?
if (link_to_cocoa)
	target_link_libraries("${My_Project_Title}" "-framework Cocoa")
endif (link_to_cocoa)

if (hasParent)
else()
	# Use assets for application icons?
	if (use_assets)
		target_sources("${My_Project_Title}"
			PRIVATE
			resources/Assets.xcassets
		)

		set_source_files_properties(resources/Assets.xcassets
			PROPERTIES
			MACOSX_PACKAGE_LOCATION Resources
		)

		set_target_properties("${My_Project_Title}"
			PROPERTIES
			XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
		)
	endif (use_assets)

	# Enable ARC
	if (enable_arc)
		set_property (TARGET "${My_Project_Title}"
			APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
		)
	endif (enable_arc)
endif()


# Is libcurl available?
# Note -- linking to libcurl seems to impart a small performance hit, even if it is not used?
if (DEFINED TEST)
else()
	find_package(CURL)
	if (CURL_FOUND)
		add_definitions(-DUSE_CURL)
		target_link_libraries(libMultiMarkdown7 CURL::libcurl)
	endif(CURL_FOUND)
endif()


# ======================
# Process Template Files
# ======================

file(READ ${PROJECT_SOURCE_DIR}/LICENSE My_Project_License)

string(REGEX REPLACE "\n" "\n\t" My_Project_License_Indented ${My_Project_License})

string(REGEX REPLACE "\"" "\\\\\"" My_Project_License_Escaped ${My_Project_License_Indented})

string(REGEX REPLACE "\n" "\\\\n\"\\\\\n\"" My_Project_License_Literal ${My_Project_License_Escaped})


configure_file (
	"${PROJECT_SOURCE_DIR}/templates/template.c.in"
	"${PROJECT_BINARY_DIR}/template.c"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/export.h.in"
	"${PROJECT_BINARY_DIR}/export.h"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/export.c.in"
	"${PROJECT_BINARY_DIR}/export.c"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/template.h.in"
	"${PROJECT_BINARY_DIR}/template.h"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/main.c.in"
	"${PROJECT_BINARY_DIR}/main.c"
)

# Update the project README, to pull in new version #, etc.
configure_file (
	"${PROJECT_SOURCE_DIR}/templates/README.md.in"
	"${CMAKE_CURRENT_LIST_DIR}/README.md"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/version.h.in"
	"${PROJECT_BINARY_DIR}/version.h"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/archive.sh.in"
	"${PROJECT_BINARY_DIR}/archive.sh"
)


# ===========================================
# Build Test Suite with CuTest (unit testing)
# ===========================================

# from http://stackoverflow.com/questions/25199677/how-to-detect-if-current-scope-has-a-parent-in-cmake
get_directory_property(hasParent PARENT_DIRECTORY)


set(test_files
	test/CuTest.c
	test/CuTest.h
	${PROJECT_BINARY_DIR}/AllTests.c
)


# Process source files to look for tests to run
add_custom_command (
	OUTPUT ${PROJECT_BINARY_DIR}/AllTests.c
	COMMAND bash ${PROJECT_SOURCE_DIR}/test/make-tests.sh ${PROJECT_SOURCE_DIR}/src/*.c > ${PROJECT_BINARY_DIR}/AllTests.c
)


if(hasParent)
	# Don't create "run_tests" when we're a sub-project for something else
else()
	# Only create framework if we are the parent project
	set_target_properties("${My_Project_Title}"
		PROPERTIES
		FRAMEWORK TRUE
	)

	if (DEFINED TEST)
		add_definitions(-DTEST)

		add_executable(run_tests
			${src_files}
			${private_headers}
			${public_headers}
			${test_files}
		)

		# Link to other libraries
		target_link_libraries(run_tests
			${libraries_to_link}
		)

		# Link to Apple Cocoa Framework?
		if (link_to_cocoa)
			target_link_libraries(run_tests "-framework Cocoa")
		endif (link_to_cocoa)

		# Enable ARC
		if (enable_arc)
			set_property (TARGET run_tests
				APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
			)
		endif (enable_arc)

		enable_testing()

		add_test( test ${PROJECT_BINARY_DIR}/run_tests)

		# valgrind memory testing
	#	find_program (MEMORYCHECK_COMMAND valgrind)
	#	SET (MEMORYCHECK_COMMAND_OPTIONS --leak-check=full --error-exitcode=1)
	#
	#	add_test( memory_test ${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS} ${PROJECT_BINARY_DIR}/run_tests)

	else()
		if (WIN32)
			add_executable(multimarkdown
				src/main.c
				src/getopt.c # from https://github.com/alex85k/wingetopt
			)
		else()
			add_executable(multimarkdown
				src/main.c
			)
		endif()

		target_link_libraries(multimarkdown PRIVATE "${My_Project_Title}")

		set_target_properties(multimarkdown
			PROPERTIES
			PREFIX ""
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${My_Project_Identifier}"
			XCODE_ATTRIBUTE_MARKETING_VERSION ${My_Project_Version}
			RESOURCE "${resources}"

			XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS} ${framework_search_paths_string}"

			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] YES
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=MinSizeRel] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=RelWithDebInfo] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Release] NO
		)

		add_executable(generate_test_emph
			src/_generate_test_emph.c
		)

		set_target_properties(generate_test_emph
			PROPERTIES
			PREFIX ""
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${My_Project_Identifier}"
			XCODE_ATTRIBUTE_MARKETING_VERSION ${My_Project_Version}

			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] YES
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=MinSizeRel] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=RelWithDebInfo] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Release] NO
		)

		add_executable(generate_test_reference
			src/_generate_test_reference.c
		)

		set_target_properties(generate_test_reference
			PROPERTIES
			PREFIX ""
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${My_Project_Identifier}"
			XCODE_ATTRIBUTE_MARKETING_VERSION ${My_Project_Version}

			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] YES
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=MinSizeRel] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=RelWithDebInfo] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Release] NO
		)

		add_executable(generate_test_structure
			src/_generate_test_structure.c
		)

		set_target_properties(generate_test_structure
			PROPERTIES
			PREFIX ""
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${My_Project_Identifier}"
			XCODE_ATTRIBUTE_MARKETING_VERSION ${My_Project_Version}

			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] YES
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=MinSizeRel] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=RelWithDebInfo] NO
			XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Release] NO
		)
	endif()
endif()


# ================================
# Build Documentation with Doxygen
# ================================


# Generate doxygen configuration file
# string(REPLACE ";" " " doxygen_src_files "${src_files}" )
string(REPLACE ";" " " doxygen_header_files "${public_headers}" )

configure_file (
	"${PROJECT_SOURCE_DIR}/templates/doxygen.conf.in"
	"${PROJECT_BINARY_DIR}/doxygen.conf"
)


# ==========================
# Build Installer with CPack
# ==========================

if(TARGET multimarkdown)

	# Build documentation

	if (CMAKE_GENERATOR MATCHES "Xcode")
	else()
		add_custom_command(TARGET multimarkdown
			POST_BUILD
			COMMAND $<TARGET_FILE:multimarkdown> -E -r "${CMAKE_SOURCE_DIR}/docs/dev.md" > "${PROJECT_BINARY_DIR}/Developer\'s\ Guide.html"
			COMMAND $<TARGET_FILE:multimarkdown> -E -r "${CMAKE_SOURCE_DIR}/docs/user.md" > "${PROJECT_BINARY_DIR}/User\'s\ Guide.html"
			COMMAND $<TARGET_FILE:multimarkdown> -E -r "${CMAKE_SOURCE_DIR}/docs/history.md" > "${PROJECT_BINARY_DIR}/MMD\ History.html"
			COMMAND $<TARGET_FILE:multimarkdown> -E -r "${CMAKE_SOURCE_DIR}/README.md" > "${PROJECT_BINARY_DIR}/ReadMe.html"
			VERBATIM
			COMMENT "Generating Documentation..."
		)
	endif()

	set (CPACK_PACKAGE_NAME "MultiMarkdown")
	set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "${My_Project_Description}")
	set (CPACK_PACKAGE_VERSION "${My_Project_Version_Major}.${My_Project_Version_Minor}.${My_Project_Version_Patch}")
	set (CPACK_PACKAGE_VERSION_MAJOR "${My_Project_Version_Major}")
	set (CPACK_PACKAGE_VERSION_MINOR "${My_Project_Version_Minor}")
	set (CPACK_PACKAGE_VERSION_PATCH "My_Project_Version_Patch")

	configure_file (
		"${PROJECT_SOURCE_DIR}/LICENSE"
		"${PROJECT_BINARY_DIR}/LICENSE.txt"
	)
	configure_file (
		"${PROJECT_SOURCE_DIR}/README.md"
		"${PROJECT_BINARY_DIR}/README.txt"
	)

	set (CPACK_RESOURCE_FILE_LICENSE "${PROJECT_BINARY_DIR}/LICENSE.txt")
	set (CPACK_RESOURCE_FILE_README "${PROJECT_BINARY_DIR}/README.txt")

	if (DEFINED TEST)
		# Don't install the test suite
	else()
		if (APPLE)
			# Apple Builds

			# Prevent installer from attempting to install Rosetta
			#    https://gitlab.kitware.com/cmake/cmake/-/issues/21734
			list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/tools)

			if (DEFINED ZIP)
				set (CPACK_GENERATOR ZIP)
			else (DEFINED ZIP)
				if (DEFINED DND)
					set (CPACK_GENERATOR DragNDrop)
					set (CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK ON)
					set (CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE ON)
				else()
					set (CPACK_GENERATOR productbuild)
					# set (CPACK_GENERATOR TGZ)
				endif (DEFINED DND)
			endif (DEFINED ZIP)
		else (APPLE)
			if (WIN32)
				# Windows Builds

				if (DEFINED ZIP)
					set (CPACK_GENERATOR ZIP)
				else (DEFINED ZIP)
					set (CPACK_GENERATOR NSIS)
				endif (DEFINED ZIP)
			elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
				# Linux
				set (CPACK_GENERATOR ZIP)
			endif (WIN32)
		endif (APPLE)

		# Install Target
		install (TARGETS multimarkdown
			DESTINATION bin
			COMPONENT Main
		)
		set (CPACK_COMPONENT_MAIN_DISPLAY_NAME "MultiMarkdown ${My_Project_Version}")
		set (CPACK_COMPONENT_MAIN_DESCRIPTION "Install MultiMarkdown ${My_Project_Version}.")

		install (FILES
			latex/mmd7-core.sty
			latex/mmd7-article.sty
			latex/mmd7-beamer.sty
			latex/mmd7-letterhead.sty
			COMPONENT LaTeX
			DESTINATION /Latex\ Packages
		)

		install (FILES
			"${PROJECT_BINARY_DIR}/README.html"
			"${PROJECT_BINARY_DIR}/LICENSE.txt"
			"${PROJECT_BINARY_DIR}/Developer\'s\ Guide.html"
			"${PROJECT_BINARY_DIR}/User\'s\ Guide.html"
			"${PROJECT_BINARY_DIR}/MMD\ History.html"
			COMPONENT Readme
			DESTINATION /
		)

		set (CPACK_COMPONENT_LATEX_DISPLAY_NAME "LaTeX Packages")
		set (CPACK_COMPONENT_LATEX_DESCRIPTION "Install LaTeX packages for MultiMarkdown support")

		# Rename package
		if (APPLE)
			set (CPACK_PACKAGE_FILE_NAME "MultiMarkdown-Mac-${CPACK_PACKAGE_VERSION}")
		else (APPLE)
			set (CPACK_PACKAGE_FILE_NAME "MultiMarkdown-${CMAKE_SYSTEM_NAME}-${CPACK_PACKAGE_VERSION}")
		endif (APPLE)

		set (CPACK_COMPONENTS_ALL Main LaTeX Readme)

		if (WIN32)
			set (CPACK_INSTALL_PREFIX "")
			set (CPACK_NSIS_MODIFY_PATH ON)
			if (ZIP)
				set(CPACK_SET_DESTDIR true)
			endif(ZIP)
		else(WIN32)
			set(CPACK_SET_DESTDIR true)
			set (CPACK_INSTALL_PREFIX /usr/local)
		endif(WIN32)

		set (CPACK_PACKAGE_CONTACT "fletcher@fletcherpenney.net")
	endif (DEFINED TEST)

	include(CPack)

endif(TARGET multimarkdown)


# =====================================
# MultiMarkdown Integration Test Suite
# =====================================

if (hasParent)
else()
	if (DEFINED TEST)
	else()
		enable_testing()

		# MMD 7

		function(ADD_MMD_TEST_2 NAME ARGS FOLDER EXTI EXTO)
			if (WIN32)
				add_test(NAME ${NAME}
					COMMAND "C:/Program Files/Git/bin/bash.exe" -c "../tests/MMDTest.sh $<TARGET_FILE:multimarkdown> \"${ARGS}\" ../tests/${FOLDER} ${EXTI} ${EXTO}"
				)
			else()
				add_test(NAME ${NAME}
					COMMAND ${PROJECT_SOURCE_DIR}/tests/MMDTest.sh
					$<TARGET_FILE:multimarkdown>
					${ARGS}
					${PROJECT_SOURCE_DIR}/tests/${FOLDER}
					${EXTI}
					${EXTO}
				)
			endif()
		endfunction(ADD_MMD_TEST_2)


		ADD_MMD_TEST_2(mmd-7 "parse -r" MMD7Tests text html)
		ADD_MMD_TEST_2(mmd-7-compatible "parse -c" MMD7Tests text htmlc)
		ADD_MMD_TEST_2(mmd-7-critic-accept "parse -A" CriticMarkup text htmla)
		ADD_MMD_TEST_2(mmd-7-critic-reject "parse -R" CriticMarkup text htmlr)
		ADD_MMD_TEST_2(mmd-7-latex "parse -r -t latex" MMD7Tests text tex)
		ADD_MMD_TEST_2(mmd-7-opml "parse -r -t opml" MMD7Tests text opml)

		# This requires that LaTeX is installed
		# ADD_MMD_TEST_2(mmd-7-beamer "parse -r -E -t beamer" Beamer text tex)

		ADD_MMD_TEST_2(mmd-7-beamer "parse -r -t beamer" Beamer text tex2)

		if (WIN32)
			# Too much trouble to make this work on Windows
		else ()
			ADD_MMD_TEST_2(mmd-7-ast "parse -r -t ast" MMD7Tests text ast)
		endif()

		# These won't all pass, but it is useful to generally verify that OPML round-tripping works
		# ADD_MMD_TEST_2(mmd-7-opml-2 "parse -t mmd" MMD7Tests opml text)

		# Round trip tests with simple documents
		ADD_MMD_TEST_2(mmd-7-opml2html "parse -O -t html" Import opml html)
		ADD_MMD_TEST_2(mmd-7-opml2mmd "parse -O -t mmd" Import opml mmd)
		ADD_MMD_TEST_2(mmd-7-itmz2html "parse -I -t html" Import itmz html)
		ADD_MMD_TEST_2(mmd-7-itmz2mmd "parse -I -t mmd" Import itmz mmd)

		# There are (generally) slight differences between the source
		# HTML file (generated by MMD) and the round-tripped HTML file,
		# so we compare against a modified version of the HTML
		ADD_MMD_TEST_2(mmd-7-html2html "parse -H -t html" Import html html2)

		# System to exhaustively test combinations of lines for consistent parsing of overall structure
		# ADD_MMD_TEST_2(mmd-7-struct-2 "parse -C -c" Structure text html)
		# ADD_MMD_TEST_2(mmd-7-struct-2-H "parse -H -C -c" Structure html html)
	endif()
endif()


# ADD_MMD_TEST(mmd-6-memoir "-t memoir" Memoir tex)
# 
# ADD_MMD_TEST(mmd-6-fodt "-t fodt" MMD7Tests fodt)
