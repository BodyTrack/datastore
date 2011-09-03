# Keep intermediate files
.SECONDARY:

CMPJSON = 2>>log.txt | ./compare-json.rb --ignore-log -
CMPTXT = 2>>log.txt | diff -

CPPFLAGS = -Wall -g -I.. -I../jsoncpp-src-0.5.0-patched/include -I/opt/local/include

LDFLAGS = -L/opt/local/lib -lboost_thread-mt

ALL = \
	test-annebug \
	test-import-bt \
	test-import-json-single \
	test-import-json-multiple \
	test-import-json-format \
	test-import-json-single-entry \
	TestJson.test \
	TestBinaryIO.test \
	TestDataSample.test \
	TestTile.test \
	TestChannel.test \
	TestRange.test \
	TestTileIndex.test \
	TestFilesystemKVS.test

BINARIES = TestRange TestBinaryIO TestDataSample TestTile TestChannel TestTileIndex TestFilesystemKVS 
all: $(ALL)

clean:
	rm -rf $(BINARIES) log.txt

VPATH = ..

%.test: %
	./$^

TestRange: TestRange.cpp utils.cpp utils.h
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) || (rm $@ && exit 1)

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

test-annebug:
	rm -rf anne.kvs
	mkdir anne.kvs
	../import anne.kvs 1 A_Cheststrap testdata/anne-cheststrap-bug/4e386a43.bt $(CMPJSON) output/test-annebug-1
	../gettile anne.kvs 1 A_Cheststrap.Respiration 0 2563125 $(CMPJSON) output/test-annebug-2
	../import anne.kvs 1 A_Cheststrap testdata/anne-cheststrap-bug/345.bt $(CMPJSON) output/test-annebug-3
	../gettile anne.kvs 1 A_Cheststrap.Respiration 0 2563125  $(CMPJSON) output/test-annebug-4

test-import-bt:
	rm -rf foo.kvs
	mkdir -p foo.kvs
	../import foo.kvs 1 ar-basestation testdata/ar_basestation/4cfcff7d.bt $(CMPJSON) output/test-import-bt-1
	../export foo.kvs 1 ar-basestation.Humidity                            $(CMPTXT) output/test-import-bt-2
	../export foo.kvs 1 ar-basestation.Microphone                          $(CMPTXT) output/test-import-bt-3
	../export foo.kvs 1 ar-basestation.Pressure                            $(CMPTXT) output/test-import-bt-4
	../export foo.kvs 1 ar-basestation.Temperature                         $(CMPTXT) output/test-import-bt-5

test-import-json-single:
	rm -rf foo.kvs
	mkdir -p foo.kvs
	../import foo.kvs 1 rphone testdata/json_import20110805-4317-1pzfs94-0.json $(CMPJSON) output/test-import-json-single-1
	../export foo.kvs 1 rphone.accuracy                                         $(CMPTXT) output/test-import-json-single-2
	../export foo.kvs 1 rphone.altitude                                         $(CMPTXT) output/test-import-json-single-3
	../export foo.kvs 1 rphone.bearing                                          $(CMPTXT) output/test-import-json-single-4
	../export foo.kvs 1 rphone.latitude                                         $(CMPTXT) output/test-import-json-single-5
	../export foo.kvs 1 rphone.longitude                                        $(CMPTXT) output/test-import-json-single-6
	../export foo.kvs 1 rphone.speed                                            $(CMPTXT) output/test-import-json-single-7
	../export foo.kvs 1 rphone.provider		                            $(CMPTXT) output/test-import-json-single-8

test-import-json-multiple:
	rm -rf foo.kvs
	mkdir -p foo.kvs
	../import foo.kvs 1 rphone testdata/multiple.json                $(CMPJSON) output/test-import-json-multiple-1
	../export foo.kvs 1 rphone.latitude rphone.altitude rphone.speed $(CMPTXT) output/test-import-json-multiple-2

test-import-json-format:
	rm -rf foo.kvs
	mkdir -p foo.kvs
	../import foo.kvs 1 rphone --format json testdata/json_import20110805-4317-1pzfs94-0 $(CMPJSON) output/test-import-json-format-1
	../export foo.kvs 1 rphone.latitude rphone.altitude rphone.speed                     $(CMPTXT) output/test-import-json-format-2

test-import-json-single-entry:
	rm -rf foo.kvs
	mkdir -p foo.kvs
	../import foo.kvs 1 myiphone testdata/mymee-anne-import-test-110901.json $(CMPJSON) output/test-import-json-single-entry-1
	../export foo.kvs 1 myiphone Dopaboost                                   $(CMPTXT) output/test-import-json-single-entry-2
	../export foo.kvs 1 myiphone Dopaboost._comment                          $(CMPTXT) output/test-import-json-single-entry-3
	../import foo.kvs 1 myiphone testdata/mymee-anne-import-test-110901.json $(CMPJSON) output/test-import-json-single-entry-4
	../export foo.kvs 1 myiphone Dopaboost                                   $(CMPTXT) output/test-import-json-single-entry-2
	../export foo.kvs 1 myiphone Dopaboost._comment                          $(CMPTXT) output/test-import-json-single-entry-3

../jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a:
	make -C .. jsoncpp-src-0.5.0-patched/libs/libjson_libmt.a
