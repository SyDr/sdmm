#pragma once

#include <boost/preprocessor/stringize.hpp>

#define PROGRAM_NAME  "SD Mod Manager"
#define PROGRAM_ALIAS "Tragedy"

#define PROGRAM_VERSION_MAJOR 0
#define PROGRAM_VERSION_MINOR 98
#define PROGRAM_VERSION_PATCH 45

#define PROGRAM_VERSION_MAJOR_STR BOOST_PP_STRINGIZE(PROGRAM_VERSION_MAJOR)
#define PROGRAM_VERSION_MINOR_STR BOOST_PP_STRINGIZE(PROGRAM_VERSION_MINOR)
#define PROGRAM_VERSION_PATCH_STR BOOST_PP_STRINGIZE(PROGRAM_VERSION_PATCH)

#define PROGRAM_VERSION_BASE \
	PROGRAM_VERSION_MAJOR_STR "." PROGRAM_VERSION_MINOR_STR "." PROGRAM_VERSION_PATCH_STR

#define PROGRAM_VERSION_SUFFIX "-beta"

#define PROGRAM_VERSION PROGRAM_VERSION_BASE PROGRAM_VERSION_SUFFIX

#define PROGRAM_VERSION_TAG "v" PROGRAM_VERSION_BASE
