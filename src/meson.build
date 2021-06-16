
gedf_sources = files (
    'edf-bdf-file.c',
    'edf-bdf-header.c',
    'edf-bdf-signal.c',
    'edf-file.c',
    'edf-header.c',
    'edf-record-priv.c',
    'edf-signal.c'
)

extra_c_args = []
build_type = get_option('buildtype')
if build_type != 'debug' and build_type != 'plain'
    extra_c_args += ['-DG_DISABLE_ASSERT']
else
    extra_c_args += []
endif


libgedf = shared_library (
    'gedf',
    gedf_sources,
    include_directories : gedf_include,
    dependencies : gedf_deps,
    gnu_symbol_visibility : 'hidden',
    install : true,
    c_args : extra_c_args
)

libgedf_dep = declare_dependency(
    link_with : libgedf,
    include_directories : gedf_include,
    dependencies : gedf_deps
)
    
gnome = import('gnome')
gnome.generate_gir(
    libgedf,
    sources : gedf_public_headers + gedf_sources,
    namespace : 'Edf',
    nsversion : gedf_api_version,
    identifier_prefix : 'Edf',
    export_packages : 'edf',
    symbol_prefix : 'edf',
    includes : ['GObject-2.0', 'GLib-2.0', 'Gio-2.0'],
    dependencies : [libgedf_dep],
    header : gedf_public_header,
    install : true,
) 

