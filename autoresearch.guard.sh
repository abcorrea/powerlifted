#!/usr/bin/env bash
# Dependency-hygiene guard for the autoresearch loop.
#
# Powerlifted depends ONLY on the C++ standard library and the deps already
# vendored in-tree (src/search/parallel_hashmap/). It is NOT linked against
# boost or any other third-party/system C++ library. boost headers happen to
# live in /usr/include, so `#include <boost/...>` compiles with no CMake or
# link change and a build will NOT catch it -- this guard does.
#
# An experiment that needs a container the stdlib lacks (e.g. a small-buffer-
# optimized vector) must VENDOR a minimal single-header implementation into the
# repo, the way phmap is vendored -- never pull in an external dependency.
#
# Detection: any angle-bracket include with a slash (`#include <lib/...>`) is
# the shape of an external library. Real exceptions are POSIX/platform/sanitizer
# headers (sys/, mach/, ...) and the vendored phmap dir, which are allow-listed.
# Anything left over is a forbidden dependency. Project headers use quotes
# ("...") and are never matched.
#
# Exit 0 = clean, exit 1 = a forbidden include was found (revert the experiment).
set -euo pipefail
cd "$(dirname "$0")"

# First path component of an angle-bracket include that is allowed to contain a
# slash: standard POSIX / platform / toolchain header roots, none of which are
# third-party dependencies.
ALLOW='sys|mach|libkern|android|sanitizer|asm|linux|bits|net|netinet|arpa|c\+\+'

forbidden=$(grep -rnE '#[[:space:]]*include[[:space:]]*<[^>]+/[^>]+>' src \
                 --include='*.h' --include='*.hpp' \
                 --include='*.cc' --include='*.cpp' 2>/dev/null \
              | grep -v '/parallel_hashmap/' \
              | grep -vE "<($ALLOW)/" \
              || true)

if [ -n "$forbidden" ]; then
    {
        echo "DEPENDENCY GUARD FAILED: external/third-party library include(s) found."
        echo "Powerlifted uses only the C++ standard library and its vendored deps"
        echo "(src/search/parallel_hashmap/). boost and other system libraries are"
        echo "NOT project dependencies. Vendor a minimal single-header version"
        echo "instead of including one. Offending include(s):"
        echo "$forbidden"
    } >&2
    exit 1
fi

echo "dependency guard: OK (no external includes)"
