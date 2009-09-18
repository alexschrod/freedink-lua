#!/bin/bash -ex
# MS Woe release

# Copyright (C) 2008, 2009  Sylvain Beucler

# This file is part of GNU FreeDink

# GNU FreeDink is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# GNU FreeDink is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

PACKAGE=freedink
TARBALL=$(cd /mnt/snapshots && ls $PACKAGE-*.tar.gz | grep ^$PACKAGE-[0-9] | sort -n | tail -1)
VERSION=${TARBALL#$PACKAGE-}
VERSION=${VERSION%.tar.gz}
cp -a /mnt/snapshots/$TARBALL .
rm -rf t/
mkdir t
pushd t
tar xzf ../freedink-$VERSION.tar.gz
cd freedink-$VERSION/

# full-static
mkdir cross
pushd cross/
SDL_CONFIG=/usr/local/i586-mingw32msvc/bin/sdl-config \
  ../configure --host=i586-mingw32msvc --build=i686-pc-linux-gnu \
  --enable-static --enable-upx
make
make install-strip DESTDIR=`pwd`/destdir
# move .exe but avoid symlinks
find destdir/usr/local/bin/ -type f -name "*.exe" | while read file; do
  mv $file /mnt/snapshots/woe/$(basename ${file%.exe}-$VERSION.exe)
done

# .dll
rm -rf destdir/
SDL_CONFIG=/usr/local/i586-mingw32msvc/bin/sdl-config \
  ../configure --host=i586-mingw32msvc --build=i686-pc-linux-gnu \
  --enable-upx
make
make install-strip DESTDIR=`pwd`/destdir
mkdir zip
find destdir/usr/local/bin/ -type f -name "*.exe" | while read file; do
  cp -a $file /mnt/snapshots/woe/$(basename ${file%.exe}-$VERSION-dll.exe)
  mv $file zip/$(basename ${file%.exe}-$VERSION-dll.exe)
done
for i in SDL.dll SDL_image.dll SDL_mixer.dll SDL_ttf.dll \
    libSDL_gfx-13.dll libfreetype-6.dll libjpeg-7.dll libogg-0.dll \
    libpng12-0.dll libtiff-3.dll libvorbis-0.dll libvorbisfile-3.dll \
    libz-1.dll libzip-1.dll; do
   cp -a /usr/local/i586-mingw32msvc/bin/$i zip/
done
cat <<EOF > zip/freedink-DLL.txt
The .dll files are compiled versions of several free software
projects.

See
  http://www.freedink.org/releases/woe/depsources/
  http://www.freedink.org/snapshots/woe/depsources/
for their source code, and
  doc/cross.txt
in the FreeDink source code to see how they were compiled.

See also licenses/ for your rights on these projects.
EOF
mkdir -m 755 zip/licenses
cp /usr/src/SDL-1.2.13/COPYING zip/licenses/SDL-1.2.13_COPYING
cp /usr/src/libogg-1.1.4/COPYING zip/licenses/libogg-1.1.4_COPYING
cp /usr/src/libvorbis-1.2.3/COPYING zip/licenses/libvorbis-1.2.3_COPYING
cp /usr/src/SDL_ttf-2.0.9/COPYING zip/licenses/SDL_ttf-2.0.9_COPYING
cp /usr/src/freetype-2.3.9/docs/GPL.TXT zip/licenses/freetype-2.3.9_GPL.TXT
cp /usr/src/SDL_gfx-2.0.19/LICENSE zip/licenses/SDL_ttf-2.0.19_LICENSE
cp /usr/src/SDL_mixer-1.2.8/COPYING zip/licenses/SDL_mixer-1.2.8_COPYING
cp /usr/src/jpeg-7/README zip/licenses/jpeg-7_README
cp /usr/src/zlib-1.2.3.3.dfsg/README zip/licenses/zlib-1.2.3.3.dfsg_README
cp /usr/src/libpng-1.2.40/LICENSE zip/licenses/libpng-1.2.40_LICENSE
cp /usr/src/tiff-3.9.1/COPYRIGHT zip/licenses/tiff-3.9.1_COPYRIGHT
cp /usr/src/SDL_image-1.2.7/COPYING zip/licenses/SDL_image-1.2.7_COPYING
cp /usr/src/libzip-0.9/lib/zip.h zip/licenses/libzip-0.9_zip.h

rm -f /mnt/snapshots/woe/$PACKAGE-$VERSION.zip
(cd zip/ && zip -r /mnt/snapshots/woe/$PACKAGE-$VERSION.zip *)
popd

popd
#rm -rf t
