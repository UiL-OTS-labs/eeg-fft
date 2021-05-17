
math_dep = cc.find_library('m', required : false)

unit_sources = files(
#    'file-test.c',
#    'header-test.c',
#    'signal-test.c',
#    'unit-test.c',
)

#test_gedf = executable (
#    'unit-test',
#    unit_sources,
#    dependencies : [libgedf_dep, cunit_dep, math_dep]
#)


testdeps = [libgedf_dep, math_dep]
testenv = [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir())
]

# test ('unit-test', test_gedf)
test ('file',
    executable('file', 'file-test.c', dependencies: testdeps),
    env : testenv
)

test ('header',
    executable('header', 'header-test.c', dependencies : testdeps),
    env : testenv
)

test ('signal',
    executable('signal', 'signal-test.c', dependencies : testdeps),
    env : testenv
)
