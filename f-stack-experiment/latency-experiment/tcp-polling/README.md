```
wget -qO- http://10.107.30.33:8888 | sed -e 's/<[^>]*>//g'

./wrk -t2 -c2 -d10s http://10.107.30.33:8888
./wrk -t4 -c4 -d10s http://10.107.30.33:8888
./wrk -t8 -c8 -d10s http://10.107.30.33:8888
./wrk -t16 -c16 -d10s http://10.107.30.33:8888
./wrk -t32 -c32 -d10s http://10.107.30.33:8888
./wrk -t64 -c64 -d10s http://10.107.30.33:8888

```
