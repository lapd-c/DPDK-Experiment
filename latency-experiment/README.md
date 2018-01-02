### F-SHARK

- **installing F-SHARK**
```
# Install from github
git clone https://github.com/F-Stack/f-stack.git /data/f-stack

# Compile F-Stack
export FF_PATH=/data/f-stack
export FF_DPDK=/data/f-stack/dpdk/x86_64-native-linuxapp-gcc

export RTE_SDK=/data/f-stack/dpdk
export DESTDIR=/usr/local
export RTE_TARGET=x86_64-native-linuxapp-gcc

# install gwak
sudo apt-get install gawk

/freebsd/netinet/libalias/alias_sctp.c:2130:32:

# Running the F-Stack
./start.sh -b example/helloworld -c config.ini
./start.sh -b example/helloworld_epoll -c config.ini


# the ip address needs to be within the broadcast []
wget -qO- http://10.218.111.254 | sed -e 's/<[^>]*>//g'
```

<br>

- **[implementing f-stack-tcp](f-stack-tcp)**
```
cd f-stack-tcp
./server --conf config_server.ini --proc-type=primary --proc-id=0
./client
```

<br>

- **testing with IMB-benchmark**
    - [test results](results/traditional_test.md)
```
mpirun -np 2 --hostfile hosts_example ./IMB-MPI1 pingpong
```