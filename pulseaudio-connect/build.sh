set -e
# # build libsound
# cd libsoundio
# # rm -rf build
# mkdir -p build
# cd build
# cmake ..
# make
# sudo make install
# cd ../../

# build main code
rm -rf build main
mkdir build
cd build
cmake ..
make
mv main ../
cd ..
rm -rf build