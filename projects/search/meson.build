meson.add_postconf_script(
    '../../create_symlinks.py',
    meson.current_source_dir(),
    meson.current_build_dir(),
    '--recursive',
    'config.cfg',
    '../shared/models',
    '../shared/shaders',
)

executable(
  'search',
  files('main.cpp'),
  dependencies: [the_chariot_dep]
)