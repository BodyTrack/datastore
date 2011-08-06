CPPFLAGS = -g -Wall -Ijsoncpp-src-0.5.0/include -I/opt/local/include # -O3 
# LDFLAGS = -Ljsoncpp-src-0.5.0/libs -ljson_linux_libmt -static

ifeq ($(shell uname -s),Linux)
  LDFLAGS = -static
else
  LDFLAGS =
endif

SQL_CPPFLAGS = -I/usr/local/mysql/include 
SQL_LDFLAGS = -L/usr/local/mysql/lib -lmysqlclient

SOURCES=tilegen.cpp mysql_common.cpp MysqlQuery.cpp Channel.cpp Logrec.cpp Tile.cpp utils.cpp
INCLUDES=mysql_common.h MysqlQuery.h Tile.h Channel.h Logrec.h

all: import gettile

test: test-import
	make -C tests

clean:
	rm -f import gettile

ARCH:=$(shell uname -s -m | sed 's/ /_/' | tr 'A-Z' 'a-z')

install-local: gettile import
	mkdir -p ../website/lib/datastore/$(ARCH)
	cp $^ ../website/lib/datastore/$(ARCH)

jsoncpp-src-0.5.0/libs/libjson_libmt.a:
	(cd jsoncpp-src-0.5.0 && python scons.py platform=linux-gcc && cd libs && ln -sf linux*/*.a libjson_libmt.a)

gettile: gettile.cpp Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

test-gettile:
	mkdir -p foo.kvs
	./gettile /home/bodytrack/projects/bodytrack/website/db/dev.kvs 1 A_Home_Desk.Temperature 0 2560569

import:	import.cpp ImportBT.cpp ImportJson.cpp Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

test-import-bt: import
	mkdir -p foo.kvs
	./import foo.kvs 1 ar-basestation bt/ar_basestation/4cfcff7d.bt

test-import-json: import
	mkdir -p foo.kvs
	./import foo.kvs 1 ar-basestation /tmp/json_import20110805-4317-1pzfs94-0.json

docs:
	doxygen KVS.cpp KVS.h

TestFileBlock: TestFileBlock.cpp FileBlock.cpp FileBlock.h MappedFile.cpp MappedFile.h
	g++ -g $(CPPFLAGS) -o $@ $^
	./$@

read_bt: Binrec.cpp crc32.cpp DataStore.cpp utils.cpp jsoncpp-src-0.5.0/libs
	g++ -g $(CPPFLAGS) Binrec.cpp crc32.cpp DataStore.cpp utils.cpp -o $@  -L./jsoncpp-src-0.5.0/libs/linux-gcc -ljson_linux-gcc
	./read_bt

jsoncpp-src-0.5.0/libs:
	cd jsoncpp-src-0.5.0; python scons.py platform=linux-gcc check
	cd jsoncpp-src-0.5.0/libs; ln -sf linux-gcc-* linux-gcc
	cd jsoncpp-src-0.5.0/libs/linux-gcc; ln -sf libjson_linux-gcc-*.a libjson_linux-gcc.a

tilegen: $(SOURCES) $(INCLUDES)
	g++ -g $(CPPFLAGS) $(SOURCES) -o $@ $(LDFLAGS) 

#-./tilegen

tile.8:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/8.9835.json > 8.9835.json

tile.7:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/7.19670.json > 7.19670.json

tile.6:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/6.39340.json > 6.39340.json

tile.0:
	time curl -b btsession http://localhost:3000/tiles/1/Josh_Basestation.Temperature/0.2517760.json > 0.2517760.json

clean-tiles:
	echo "delete from tiles where ch_name='Josh_Basestation.Temperature';" | /usr/local/mysql/bin/mysql -u root bodytrack-dev

login:
	curl -c btsession http://localhost:3000/login.json -d login=anne -d password=bodytrack

