project(
    'gedf', 'c',
    version : '0.1',
    default_options : [
        'warning_level=3',
        'buildtype=debugoptimized'
    ],
    license : 'LGPL-2.1-or-later'
)

gedf_version = meson.project_version()
version_array = gedf_version.split('.')
version_major = version_array[0]
version_minor = version_array[1]
version_number = version_major + '.' + version_minor
gedf_version = 'gedf-' + version_major + '.' + version_minor
gedf_api_version = version_major + '.0'

#modules 
pymod = import('python')
fs = import('fs')

#python
python = pymod.find_installation('python3')

# include directories of our own
gedf_include = include_directories('include')

# dependencies
libglib_dep = dependency('glib-2.0', fallback : ['glib', 'libglib_dep'] )
libgobject_dep = dependency('gobject-2.0', fallback : ['glib', 'libgobject_dep'])
libgio_dep= dependency('gio-2.0', fallback : ['glib', 'libgio_dep'])
libgmodule_dep = dependency('gmodule-2.0', fallback : ['glib', 'libgmodule_dep'])

gedf_deps = [libglib_dep, libgobject_dep, libgio_dep, libgmodule_dep]

# c compiler
cc = meson.get_compiler('c')


subdir('include')
subdir('src')
#subdir('introspection')
if get_option('build-unit-test')
    subdir('test') 
endif

subdir('python_tests')

pkg = import('pkgconfig')
pkg.generate(
    libgedf,
    name : 'gedf',
    filebase : 'gedf',
    subdirs : 'gedf',
    requires : ['glib-2.0', 'gobject-2.0', 'gmodule-2.0'],
    description : 'A library to read/write european data format files',
    version : gedf_version
)

subdir('docs')

deb_config = configuration_data({
    'NAME'          : '"' + meson.project_name() + '"',
    'VERSION'       :  '"' + meson.project_version() + '"',
    'LICENSE'       : '"' + meson.project_license()[0] + '"',

    'GLIB_NAME'     : '"' + libglib_dep.name() + '"',
    'GLIB_VERSION'  : '"' + libglib_dep.version() + '"',
    
    'GIO_NAME'      : '"' + libgio_dep.name() + '"',
    'GIO_VERSION'   : '"' + libgio_dep.version() + '"',
    
    'GMODULE_NAME'      : '"' + libgmodule_dep.name() + '"',
    'GMODULE_VERSION'   : '"' + libgmodule_dep.version() + '"'
})

configure_file(
    input : 'package.sh.in',
    output : 'package.sh',
    configuration : deb_config
)
configure_file(
    input : 'run-ldconfig.sh.in',
    output : 'run-ldconfig.sh',
    configuration : deb_config
)
