CPPFLAGS = -Wall -g -I.. -I../jsoncpp-src-0.5.0/include -I/opt/local/include
LDFLAGS = -L/opt/local/lib -lboost_thread-mt
ALL = TestChannel TestImport TestTileIndex TestTile TestFilesystemKVS

all: $(ALL)

clean:
	rm -rf $(ALL)

VPATH = ..

TestImport: TestImport.cpp ImportBT.cpp Binrec.cpp Binrec.h Channel.cpp Channel.h ChannelInfo.h crc32.cpp crc32.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestTileIndex: TestTileIndex.cpp TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestTile: TestTile.cpp Tile.cpp Tile.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestChannel: TestChannel.cpp Channel.cpp Channel.h ChannelInfo.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)
	./$@ || mv $@ $@.broken

TestFilesystemKVS: TestFilesystemKVS.cpp FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h utils.cpp ../jsoncpp-src-0.5.0/libs/libjson_libmt.a
	g++ -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS)
	./$@ || mv $@ $@.broken

../jsoncpp-src-0.5.0/libs/libjson_libmt.a:
	make -C .. jsoncpp-src-0.5.0/libs/libjson_libmt.a
