# Keep intermediate files
.SECONDARY:

CPPFLAGS = -Wall -g -I.. -I../jsoncpp-src-0.5.0-patched/include -I/opt/local/include
LDFLAGS = -L/opt/local/lib -lboost_thread-mt
ALL = TestJson.test TestBinaryIO.test TestDataSample.test TestTile.test TestChannel.test TestTileIndex.test TestFilesystemKVS.test
BINARIES = TestBinaryIO TestDataSample TestTile TestChannel TestTileIndex TestFilesystemKVS
all: $(ALL)

clean:
	rm -rf $(BINARIES)

VPATH = ..

%.test: %
	./$^

TestJson: TestJson.cpp utils.cpp utils.h ../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestBinaryIO: TestBinaryIO.cpp BinaryIO.cpp BinaryIO.h utils.cpp utils.h
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestDataSample: TestDataSample.cpp DataSample.h utils.cpp utils.h
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestTileIndex: TestTileIndex.cpp TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestTile: TestTile.cpp BinaryIO.cpp BinaryIO.h Log.cpp Log.h Tile.cpp Tile.h utils.cpp utils.h ../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestChannel: TestChannel.cpp BinaryIO.cpp BinaryIO.h Channel.cpp Channel.h ChannelInfo.h FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h Log.cpp Log.h Tile.cpp Tile.h TileIndex.h utils.cpp utils.h ../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

TestFilesystemKVS: TestFilesystemKVS.cpp FilesystemKVS.cpp FilesystemKVS.h KVS.cpp KVS.h utils.cpp Log.cpp Log.h ../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
	g++ $(CPPFLAGS) -Wall -g -I .. -I /opt/local/include -o $@ $^ $(LDFLAGS)

../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a:
	make -C .. jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
