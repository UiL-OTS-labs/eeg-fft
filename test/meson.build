
cunit_dep = dependency('cunit')

math_dep = cc.find_library('m', required : false)

unit_sources = files(
    'file-test.c',
    'header-test.c',
    'signal-test.c',
    'unit-test.c',
)

test_gedf = executable (
    'unit-test',
    unit_sources,
    dependencies : [libgedf_dep, cunit_dep, math_dep]
)

test ('Test edf header', test_gedf)
