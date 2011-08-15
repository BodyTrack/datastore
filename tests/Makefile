# Keep intermediate files
.SECONDARY:

CPPFLAGS = -Wall -g -I.. -I../jsoncpp-src-0.5.0/include -I/opt/local/include
LDFLAGS = -L/opt/local/lib -lboost_thread-mt
ALL = TestDataSample.test TestChannel.test TestImport.test TestTileIndex.test TestTile.test TestFilesystemKVS.test

all: $(ALL)

clean:
	rm -rf $(ALL)

VPATH = ..

%.test: %
	./$^

TestDataSample: TestDataSample.cpp DataSample.h utils.cpp utils.h
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestImport: TestImport.cpp ImportBT.cpp Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestTileIndex: TestTileIndex.cpp TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestTile: Log.cpp Log.h TestTile.cpp Tile.cpp Tile.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestChannel: TestChannel.cpp Channel.cpp Channel.h ChannelInfo.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestFilesystemKVS: TestFilesystemKVS.cpp FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h utils.cpp Log.cpp Log.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS)

../jsoncpp-src-0.5.0/libs/libjson_libmt.a:
	make -C .. jsoncpp-src-0.5.0/libs/libjson_libmt.a
