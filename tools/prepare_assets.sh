set -x

mkdir -p build/cards

# tiles
#sh ./tools/adjust_exposure.sh assets/cards/cards3.png output.png 0.05 0.25 1.0
#magick output.png -channel RGB -ordered-dither o4x4,3 +channel Source/cards/cards3.png
#magick assets/cards/cards.png -channel RGB -ordered-dither o8x8 +channel Source/cards/cards.png
#magick assets/cards/cards3.png -channel RGB -ordered-dither o8x8 +channel Source/cards/cards3.png

./tools/qoiconv ./assets/cards/cards.png ./build/cards/cards.qoi
./tools/qoiconv ./assets/cards/cards3.png ./build/cards/cards3.qoi
./tools/qoiconv ./assets/cards/cards4.png ./build/cards/cards4.qoi

cp ./assets/cards/cards.png ./Source/cards/
cp ./assets/cards/cards3.png ./Source/cards/
cp ./assets/cards/cards4.png ./Source/cards/
magick assets/cards/cards4.png -channel RGB -ordered-dither o8x8 +channel Source/cards/cards4.png