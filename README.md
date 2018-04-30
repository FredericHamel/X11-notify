# X11-notify
A battery level popup.
It indicate wheter the battery level is bellow 15 or higher 80.
This are actually hardcoded in source code.

Author
======
- Frédéric Hamel

Build and Installation
======================
This uses the meson build system
which provide a easy way to build
and install.

Installation in system directory.
```
> meson . build
> ninja -C build
> sudo ninja -C build install
```

Installation in a custom directory
```
> DESTDIR=custom_dir ninja -C build install
```

