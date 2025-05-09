project('loder', 'c')

pwd = meson.current_source_dir()
cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required : true, disabler: true)
gl_dep = dependency('opengl', required: true, disabler: true)
sdl2_dep = dependency('SDL2', required: true, disabler: true)
sdl2_mixer_dep = dependency('SDL2_Mixer', required: true, disabler: true)
sdl2_image_dep = dependency('SDL2_image', required: true, disabler: true)

engine_files = [
        'engine/platform.c',
        'engine/render.c',
        'engine/camera.c',
        'engine/utils.c',
        'engine/animation.c',
        'engine/image.c',
        'engine/input.c',
        'engine/trace.c',
        'engine/entity.c',
        'engine/engine.c',
        'engine/noise.c',
        'engine/font.c',
        'engine/json.c',
        'engine/sound.c',
        'engine/map.c',
]

game_files = [
    'src/game_main.c',
    'src/state.c',
    'src/pit.c',
    'src/gold.c',
    'src/trap.c',
    'src/player.c',
    'src/guard.c',
    'src/map_astar.c',
    'src/map_hint.c',
    'src/draw_text.c',
    'src/options.c',
]

includes = include_directories(
    'engine',
    'src'
)

# executable('test_data', 
# 	'test.c',
# 	game_files,
# 	include_directories: includes,
# 	dependencies: [ m_dep ]
# )

# if get_option('b_sanitize') == 'address'
    add_project_arguments('-fsanitize=address', language: 'c')
    add_project_link_arguments('-fsanitize=address', language: 'c')
# endif

executable('game',
    engine_files,
    game_files,
    include_directories: [ includes ],
    c_args: [
    	'-DRENDER_SOFTWARE', '-DPLATFORM_SDL', '-Wno-unused-parameter', '-Wno-unused-variable',
    ],
    dependencies: [ m_dep, sdl2_dep, sdl2_mixer_dep, sdl2_image_dep, gl_dep ]
)
