# Check for sudo
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi
apt-get install -y automake build-essential cmake git libboost-dev libboost-thread-dev libntl-dev libsodium-dev libssl-dev libtool m4 python3 texinfo yasm
make boost
make -j 8 tldr
./compile.py dark_pool_inputs