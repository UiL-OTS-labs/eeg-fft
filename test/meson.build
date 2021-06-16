
math_dep = cc.find_library('m', required : false)

unit_sources = files(
    'bdf-file-test.c',
    'file-test.c',
    'header-test.c',
    'signal-test.c',
    'unit-test.c',
)

testdeps = [libgedf_dep, math_dep]
testenv = [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir())
]

unittest = executable('unittest', unit_sources, dependencies : testdeps)

test (
    'unittest',
    unittest,
    env : testenv
)

#test ('file',
#    executable('file', 'file-test.c', dependencies: testdeps),
#    env : testenv
#)
#
#test ('header',
#    executable('header', 'header-test.c', dependencies : testdeps),
#    env : testenv
#)
#
#test ('signal',
#    executable('signal', 'signal-test.c', dependencies : testdeps),
#    env : testenv
#)
