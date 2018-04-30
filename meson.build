project('X11-notify', 'c')
cc = meson.get_compiler('c')
Xlib = cc.find_library('X11')

src = ['src/X11-notify.c']
executable('X11-notify', src,
    dependencies: Xlib,
    install: true)
