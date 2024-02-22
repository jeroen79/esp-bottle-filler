#/bin/bash

read -p "Target [esp32c3]: " target
target=${name:-esp32c3}

version=$(head -n 1 version)
version_file=${version//./_}

echo "Building Version: $version"

# Create our release dir if it doens't exist

if [ ! -d release ]; then
  mkdir -p release;
fi

# Clean old config, we always want to start from sdkconfig.defaults for our builds
if [ ! -d sdkconfig ]; then
  rm sdkconfig;
fi

if [ ! -d loader/sdkconfig ]; then
  rm loader/sdkconfig;
fi

# Build Engine

idf.py fullclean

idf.py set-target $target

idf.py build

# Build Loader
cd loader

idf.py fullclean

idf.py set-target $target

idf.py build

cd ..

# Copy to release
cp build/esp-bottle-filler.bin release/esp-bottle-filler_${target}_${version_file}.bin

# Todo check if esp32 bootloader also works at 0x0, it used to be 0x1000 but this didn't work for C3

# Combine Loader Release to make it flashable with gui and web
esptool.py --chip $target merge_bin \
  -o release/esp-bottle-filler-loader_${target}_${version_file}.bin \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 4MB \
  0x0 loader/build/bootloader/bootloader.bin \
  0x8000 loader/build/partition_table/partition-table.bin \
  0x40000 loader/build/esp-bottle-filler-loader.bin \
  0x35000 loader/build/ota_data_initial.bin

# Combine LoaderFull Release to make it flashable with gui and web, with custom ota boot data to boot ota0
esptool.py --chip $target merge_bin \
  -o release/esp-bottle-filler-full_${target}_${version_file}.bin \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 4MB \
  0x0 loader/build/bootloader/bootloader.bin \
  0x8000 loader/build/partition_table/partition-table.bin \
  0x40000 loader/build/esp-bottle-filler-loader.bin \
  0x35000 misc/ota_boot_ota0.bin \
  0x110000 build/esp-bottle-filler.bin
