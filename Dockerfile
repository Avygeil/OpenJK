FROM avygeil/jkbuild:latest

# Copy sources
COPY . /usr/src/openjk

# Build i386 arch
CMD \
	mkdir /usr/src/openjk/build.i386 &&\
	cd /usr/src/openjk/build.i386 &&\
	cmake -DCMAKE_TOOLCHAIN_FILE=CMakeModules/Toolchains/linux-i686.cmake \
		-DCMAKE_INSTALL_PREFIX=/root/out \
		-DBuildMPDed=ON -DBuildMPGame=ON \
		-DBuildPortableVersion=OFF \
		.. &&\
	make -j4 &&\
	make install
