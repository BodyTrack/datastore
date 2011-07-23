CPPFLAGS = -Wall -g -I.. -I../jsoncpp-src-0.5.0/include -I/opt/local/include
LDFLAGS = -L/opt/local/lib -lboost_thread-mt -L../jsoncpp-src-0.5.0/libs/linux-gcc -ljson_linux-gcc

all: TestImport TestTileIndex TestTile TestChannel TestFilesystemKVS
VPATH = ..

TestImport: TestImport.cpp ImportBT.cpp Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestTileIndex: TestTileIndex.cpp TileIndex.h utils.cpp utils.h
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ -L /opt/local/lib -lboost_thread-mt || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestTile: TestTile.cpp Tile.cpp Tile.h
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ -L /opt/local/lib -lboost_thread-mt || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestChannel: TestChannel.cpp Channel.cpp Channel.h ChannelInfo.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ -L /opt/local/lib -lboost_thread-mt || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestFilesystemKVS: TestFilesystemKVS.cpp FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ -L /opt/local/lib -lboost_thread-mt
	./$@ || mv $@ $@.broken
