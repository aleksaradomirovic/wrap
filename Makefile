# Copyright 2025 Aleksa Radomirovic
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

VERSION := 0.1

CFLAGS  := -Wall -Wextra -Wno-unused-parameter -O2 -g -MMD
LDFLAGS := $(shell pkg-config --libs libevent)

.PHONY: all clean

all: wrap

clean:
	rm -rf src/*.o src/*.d wrap deb wrap.deb

wrap: src/fork.o src/main.o src/pipe.o
	$(CC) $(CFLAGS) -o $(@) $(^) $(LDFLAGS)

deb/usr/bin/wrap: wrap
	@mkdir -p deb/usr/bin
	cp -f $(<) $(@)

deb/DEBIAN/control:
	@mkdir -p deb/DEBIAN
	@truncate $(@) --size=0
	echo "Package: wrap" >> $(@)
	echo "Version: $(VERSION)" >> $(@)
	echo "Architecture: $(shell dpkg-architecture -qDEB_HOST_ARCH)" >> $(@)
	echo "Section: utilities" >> $(@)
	echo "Maintainer: Aleksa Radomirovic <braleksa2@gmail.com>" >> $(@)
	echo "Depends: libevent-2.1-7 (>= 2.1.12)" >> $(@)
	echo "Description: A terminal wrapper for apps that do it poorly." >> $(@)

wrap.deb: deb/usr/bin/wrap deb/DEBIAN/control
	dpkg-deb --root-owner-group --build deb
	mv -f deb.deb wrap.deb

-include src/*.d
