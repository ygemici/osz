#!/bin/bash

DIR="/usr/local/gcc"
CFG=" --enable-shared --enable-threads=posix --enable-libmpx --with-system-zlib --with-isl --enable-__cxa_atexit \
--disable-libunwind-exceptions --enable-clocale=gnu --disable-libstdcxx-pch --disable-libssp --enable-gnu-unique-object \
--disable-linker-build-id --enable-lto --enable-plugin --enable-install-libiberty --with-linker-hash-style=gnu \
--enable-gnu-indirect-function --disable-multilib --disable-werror --enable-checking=release --enable-default-pie \
--enable-default-ssp --with-gnu-ld"

#sudo mkdir -p $DIR
#sudo chown `whoami` $DIR
#rm -rf $DIR/*

[ ! -d tarballs ] && mkdir tarballs
cd tarballs

echo "Downloading tarballs..."
[ ! -f binutils-*tar* ] && wget http://ftpmirror.gnu.org/binutils/binutils-2.29.tar.gz
[ ! -f gcc-*tar* ] && wget http://ftpmirror.gnu.org/gcc/gcc-7.2.0/gcc-7.2.0.tar.gz
[ ! -f mpfr-*tar* ] && wget http://ftpmirror.gnu.org/mpfr/mpfr-3.1.6.tar.gz
[ ! -f gmp-*tar* ] && wget http://ftpmirror.gnu.org/gmp/gmp-6.1.2.tar.bz2
[ ! -f mpc-*tar* ] && wget http://ftpmirror.gnu.org/mpc/mpc-1.0.3.tar.gz
[ ! -f isl-*tar* ] && wget ftp://gcc.gnu.org/pub/gcc/infrastructure/isl-0.18.tar.bz2
[ ! -f cloog-*tar* ] && wget ftp://gcc.gnu.org/pub/gcc/infrastructure/cloog-0.18.1.tar.gz
cd ..

echo -n "Unpacking tarballs... "
for i in tarballs/*.tar.gz; do d=${i%%.tar*};d=${d#*/}; echo -n "$d "; [ ! -d $d ] && tar -xzf $i; done
for i in tarballs/*.tar.bz2; do d=${i%%.tar*};d=${d#*/}; echo -n "$d "; [ ! -d $d ] && tar -xjf $i; done
echo "OK"

cd binutils-* && for i in isl; do ln -s ../$i-* $i 2>/dev/null || true; done && cd ..
cd gcc-* && for i in mpfr gmp mpc cloog; do ln -s ../$i-* $i 2>/dev/null || true; done && cd ..

#path buggy gcc configure script
cd gcc-*/gcc
cat configure | sed 's/gcc_cv_as_hidden=no/gcc_cv_as_hidden=yes/' | sed 's/gcc_cv_ld_hidden=no/gcc_cv_ld_hidden=yes/' >..c
mv ..c configure
chmod +x configure
cd ../..

for arch in aarch64 x86_64; do
    echo "  -------------------------------------------------- $arch -----------------------------------------------------------"
    mkdir $arch-binutils 2>/dev/null
    cd $arch-binutils
    ../binutils-*/configure --prefix=$DIR --target=${arch}-elf $CFG
    make -j4
    make install
    cd ..
    mkdir $arch-gcc 2>/dev/null
    cd $arch-gcc
    ../gcc-*/configure --prefix=$DIR --target=${arch}-elf --enable-languages=c $CFG
    make -j4 all-gcc
    make install-gcc
    cd ..
done
