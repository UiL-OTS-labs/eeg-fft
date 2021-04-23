
conf_data = configuration_data()
conf_data.set('VERSION', gedf_api_version)
configure_file (
    input : 'importedf.py.in',
    output : 'importedf.py',
    configuration : conf_data
)

test(
    'Python bindings test',
    python,
    env : [
        'PYTHONPATH=' + meson.current_build_dir() ,
        'LD_LIBRARY_PATH=' + fs.parent(libgedf.full_path()),
        'GI_TYPELIB_PATH=' + fs.parent(libgedf.full_path())
    ],
    args : [
        '-m', 'unittest',
        'discover',
        '-s', meson.current_source_dir(),
        '--verbose'
    ],
#    workdir : meson.current_source_dir()
)

message('LD_LIBRARY_PATH=' + fs.parent(libgedf.full_path()))
message('GI_TYPELIB_PATH=' + fs.parent(libgedf.full_path()))
message('workdir : ' + meson.current_source_dir())
