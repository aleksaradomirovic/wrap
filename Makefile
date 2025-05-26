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

CFLAGS  := -Wall -Wextra -Wno-unused-parameter -O2 -g -MMD
LDFLAGS := $(shell pkg-config --libs libevent)

.PHONY: all clean

all: wrap

clean:
	rm -rf src/*.o src/*.d wrap

wrap: src/fork.o src/main.o src/pipe.o
	$(CC) $(CFLAGS) -o $(@) $(^) $(LDFLAGS)

-include src/*.d
