CFLAGS = -Wall -I /usr/local/mysql/include
LDFLAGS = -L/usr/local/mysql/lib -lmysqlclient

SOURCES=tilegen.cpp mysql_common.cpp MysqlQuery.cpp Channel.cpp Logrec.cpp Tile.cpp utils.cpp
INCLUDES=mysql_common.h MysqlQuery.h Tile.h Channel.h Logrec.h

tilegen: $(SOURCES) $(INCLUDES)
	g++ -g $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS) 
	-./tilegen

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

