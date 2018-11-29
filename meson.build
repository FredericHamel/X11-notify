project('X11-notify', 'c')

libpthread = dependency('threads')
libSDL2 = dependency('sdl2')
libSDL2_ttf = dependency('sdl2_ttf', required : false)
libX11 = dependency('x11')

cc = meson.get_compiler('c')
Xlib = cc.find_library('X11')

if not libSDL2.found()
  libSDL2 = cc.find_library('SDL2')
endif

if not libSDL2_ttf.found()
  libSDL2_ttf = cc.find_library('SDL2_ttf')
endif

X11_src = ['src/X11-notify.c']
executable('X11-notify', X11_src,
    dependencies: [Xlib, libpthread],
    install_dir: '/bin',
    install: true)

SDL_src = ['src/SDL-notify.c']
executable('SDL-notify', SDL_src,
    dependencies: [libSDL2, libSDL2_ttf],
    install_dir: '/bin',
    install: true)
