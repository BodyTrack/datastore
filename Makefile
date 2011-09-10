CPPFLAGS = -g -Wall -Ijsoncpp-src-0.5.0-patched/include -I/opt/local/include # -O3 
# LDFLAGS = -Ljsoncpp-src-0.5.0-patched/libs -ljson_linux_libmt -static

ifeq ($(shell uname -s),Linux)
  LDFLAGS = -static
else
  LDFLAGS =
endif

SQL_CPPFLAGS = -I/usr/local/mysql/include 
SQL_LDFLAGS = -L/usr/local/mysql/lib -lmysqlclient

SOURCES=tilegen.cpp mysql_common.cpp MysqlQuery.cpp Channel.cpp Logrec.cpp Tile.cpp utils.cpp Log.cpp
INCLUDES=mysql_common.h MysqlQuery.h Tile.h Channel.h Logrec.h
INSTALL_BINS=export import gettile info

all: $(INSTALL_BINS)

test: all
	make -C test

clean:
	rm -f $(INSTALL_BINS)
	make -C test clean

ARCH:=$(shell uname -s -m | sed 's/ /_/' | tr 'A-Z' 'a-z')

install-local: $(INSTALL_BINS)
	mkdir -p ../website/lib/datastore/$(ARCH)
	cp $^ ../website/lib/datastore/$(ARCH)

install-dev-local: $(INSTALL_BINS)
	mkdir -p ../website-dev/lib/datastore/$(ARCH)
	cp $^ ../website-dev/lib/datastore/$(ARCH)

install-prod-local: $(INSTALL_BINS)
	mkdir -p ../website-prod/lib/datastore/$(ARCH)
	cp $^ ../website-prod/lib/datastore/$(ARCH)

install-static: $(INSTALL_BINS)
	mkdir -p /u/apps/bodytrack/static/$(ARCH)
	cp $^ /u/apps/bodytrack/static/$(ARCH)

install-remote: $(INSTALL_BINS)
	scp $^ bodytrack2:/u/apps/bodytrack/static/$(ARCH)

install-deploy: $(INSTALL_BINS)
	mkdir -p /u/apps/bodytrack/current/lib/datastore/$(ARCH)
	rsync -a $^ /u/apps/bodytrack/current/lib/datastore/$(ARCH)

install-test-deploy: $(INSTALL_BINS)
	mkdir -p /u/apps/bodytrack-test/current/lib/datastore/$(ARCH)
	rsync -a $^ /u/apps/bodytrack-test/current/lib/datastore/$(ARCH)


jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a:
	(cd jsoncpp-src-0.5.0-patched && python scons.py platform=linux-gcc && cd libs && ln -sf linux*/*.a libjson_libmt.a)

copy: copy.cpp BinaryIO.cpp BinaryIO.h Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

export: export.cpp BinaryIO.cpp BinaryIO.h Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

gettile: gettile.cpp BinaryIO.cpp BinaryIO.h Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

import:	import.cpp BinaryIO.cpp BinaryIO.h ImportBT.cpp ImportJson.cpp Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

info: info.cpp BinaryIO.cpp BinaryIO.h Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h DataSample.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

docs:
	doxygen KVS.cpp KVS.h

read_bt: Binrec.cpp crc32.cpp DataStore.cpp utils.cpp Log.cpp jsoncpp-src-0.5.0-patched/libs
	g++ -g $(CPPFLAGS) Binrec.cpp crc32.cpp DataStore.cpp utils.cpp -o $@  -L./jsoncpp-src-0.5.0-patched/libs/linux-gcc -ljson_linux-gcc
	./read_bt

jsoncpp-src-0.5.0-patched/libs:
	cd jsoncpp-src-0.5.0-patched; python scons.py platform=linux-gcc check
	cd jsoncpp-src-0.5.0-patched/libs; ln -sf linux-gcc-* linux-gcc
	cd jsoncpp-src-0.5.0-patched/libs/linux-gcc; ln -sf libjson_linux-gcc-*.a libjson_linux-gcc.a

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

