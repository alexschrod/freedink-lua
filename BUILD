Instructions to build from scratch (from the repository sources).

If you're using a release (TODO: release a release ;)) you won't need
bootstrap instructions.

These instructions may sound redundant with the packaging specs (.deb,
.rpm, .ebuild, etc.) but they are necessary to maintain for people who
want to compile the latest, not-yet-packaged sources :)

If you get an error about AC_LIB_PROG_LD, that's something that will
be fixed in a bit (today is 3 nov 2007) in Gnulib. As a workaround you
can install gettext which also defines this macro.


On a minimal Debian system
==========================

## Bootstrap
# Source code:
aptitude install git-core
git clone git://git.sv.gnu.org/freedink
cd freedink

# Gnulib
(cd /usr/src && git clone git://git.sv.gnu.org/gnulib)
# or:
#aptitude install gnulib

# autotools
aptitude install autoconf automake

aptitude install libsdl1.2-dev # for sdl.m4
sh bootstrap


## Dependencies
# Base: GCC, make & al.
aptitude install build-essential
# Required: SDL and zziplib
aptitude install libsdl1.2-dev libsdl-gfx1.2-dev libsdl-ttf2.0-dev \
  libsdl-image1.2-dev libsdl-mixer1.2-dev libzzip-dev zip
# Optional:
# - upx compresses binary
# - bzip is for .tar.bz2 release tarballs
aptitude install upx bzip2

## Compilation
./configure
make
make install

## Release tests
make dist
make distcheck

## Optional: software MIDI support, used by SDL_mixer
# Check doc/sound.txt for details
aptitude install timidity freepats

# :)


On a minimal Fedora system
==========================

## Bootstrap
# Source code:
yum install git-core
git clone git://git.sv.gnu.org/freedink
cd freedink

# Gnulib
(cd /usr/src && git clone git://git.sv.gnu.org/gnulib)
# No Fedora package, but there's no need for one.

# autotools
yum install autoconf automake

yum install SDL_devel # for sdl.m4
sh bootstrap


## Dependencies
# Base: GCC, make & al.
yum groupinstall 'Development Tools'
# or just:
#yum install make gcc
yum install SDL_devel SDL_gfx-devel SDL_ttf-devel SDL_image-devel \
  SDL_mixer-devel zziplib-devel zip
# Optional:
# - upx compresses binary
# - bzip is for .tar.bz2 release tarballs
yum install upx bzip2

## Compilation
./configure
make
make install

## Release tests
make dist
make distcheck

## Optional: software MIDI support, used by SDL_mixer
# Check doc/sound.txt for details
# timidity++ and timidity++-patches already installed as dependencies
#   of SDL_mixer

# :)


On a minimal Gentoo system
==========================

## Bootstrap
# Source code:
emerge dev-util/git
git clone git://git.sv.gnu.org/freedink
cd freedink

# Gnulib
(cd /usr/src && git clone git://git.sv.gnu.org/gnulib)
# or:
# Gnulib is currently masked, you need to unmask it:
# http://gentoo-wiki.com/Masked#Masked_by_missing_keyword
# Not that there are issues with the Gentoo wrapper. Use
# /usr/share/gnulib/gnulib-tool instead of /usr/bin/gnulib-tool if
# possible
#echo "dev-libs/gnulib **" >> /etc/portage/package.keywords
#emerge gnulib

# I assume you already have autoconf & al. ;)

emerge libsdl # for sdl.m4
sh bootstrap


## Dependencies
# I also assume you already have GCC ;)
# Required: SDL and zziplib
emerge libsdl sdl-gfx sdl-ttf sdl-image sdl-mixer zziplib zip
# Optional:
# - upx compresses binary
# - bzip is for .tar.bz2 release tarballs (included in base Gentoo)
emerge upx

./configure
make
make install

## Release tests
make dist
make distcheck

## Optional: software MIDI support, used by SDL_mixer
# Check doc/sound.txt for details
emerge media-sound/timidity++
echo "media-sound/timidity-freepats **" >> /etc/portage/package.keywords
emerge media-sound/timidity-freepats # GPLv>=2 + lax exception
emerge media-sound/timidity-eawpatches # non-free

# :)


On a minimal Woe system
=======================

Check doc/cross.txt (for cross compiling from GNU/Linux + MinGW32) or
doc/woe-compile.txt (for compiling from native MinGW32).
