project('mtop-2', 'cpp',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17'
  ]
)

# Минимальные зависимости - только стандартные системные библиотеки
thread_dep = dependency('threads')

# Include directories
inc_dirs = include_directories('src/Core', 'src/Config')

executable('mtop',
  sources : [
    'src/Core/main.cpp',
    'src/Core/system_info.cpp',
    'src/Config/parser.cpp'
  ],
  include_directories : inc_dirs,
  dependencies : [thread_dep],
  install : true
)