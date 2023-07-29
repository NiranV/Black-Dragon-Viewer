# Overview
This repository is intended to centralize string data common to many builds
(such as required compiler switches), both internal to Linden and external, on
TeamCity and on individual dev machines.

# Bash Usage
Intended usage is to clone this repository as a sibling to the source
directory being built, then use the bash "source" or "." commands to read
../build-variables/convenience into your build script. This gives you
shell variables such as

LL_BUILD_RELEASE, LL_BUILD_RELWITHDEBINFO, LL_BUILD_DEBUG

which are aliases for the platform-specific variables (e.g.)

LL_BUILD_LINUX_RELEASE, LL_BUILD_LINUX_RELWITHDEBINFO, LL_BUILD_LINUX_DEBUG

et al.

If you pass a build-type argument (Release, RelWithDebInfo, Debug) when
sourcing the convenience script, e.g.:

    source ../build-variables/convenience Release

then you get the additional alias LL_BUILD, which (in the example above,
assuming you're running on Linux) is the same as LL_BUILD_LINUX_RELEASE.

The convenience script also provides bash functions for removing or replacing
individual switches from any one of those variables. See the comments in the
script for function usage.

# Other Languages
## Variable Suite
The convenience script sources the variables script, which provides the
underlying suite of variables. Variable names are of the form:

LL\_BUILD\_{platform}\_{buildtype}

where:

component | meaning
----------|--------
platform  | WINDOWS, DARWIN, LINUX.
buildtype | RELEASE, RELWITHDEBINFO, DEBUG. BASE for switches common to all.

These variables are in turn composed of variables that separate out
command-line macro definitions from other kinds of compiler switches:

LL\_BUILD\_{platform}\_{buildtype}\_{category}

where:

component | meaning
----------|--------
category  | MACROS, SWITCHES.

These are typically aggregated into the corresponding
LL\_BUILD\_{platform}\_{buildtype} variables:

LL_BUILD_LINUX_RELEASE="$LL_BUILD_LINUX_RELEASE_SWITCHES $LL_BUILD_LINUX_RELEASE_MACROS"

CMake, in particular, distinguishes between macro definitions
([add_definitions()](https://cmake.org/cmake/help/v3.1/command/add_definitions.html))
and other compiler switches
([add_compile_options()](https://cmake.org/cmake/help/v3.1/command/add_compile_options.html)),
so it may be useful to populate those CMake commands from the underlying
_MACROS and _SWITCHES variables.

## Parsing the variables file
The variables file should be kept as simple as possible to parse, so it can be
read from other languages than bash. Refer to the comments in that file itself
for syntax rules.
