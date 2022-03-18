project('novelest', 'cpp', version: '0.1.0',
        default_options: ['cpp_std=c++17'])

sources = [
    'src/window.cpp',
    'src/main.cpp',
    'src/db.cpp'
]

deps = [
    dependency('gtkmm-3.0'),
    dependency('gtksourceviewmm-3.0'),
    dependency('gtk+-3.0'),
    dependency('glibmm-2.68'),
    dependency('sqlite3')
]

executable('novelest', sources, dependencies: deps)