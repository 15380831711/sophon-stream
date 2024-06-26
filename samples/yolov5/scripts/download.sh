#!/bin/bash
pip3 install dfss

scripts_dir=$(dirname $(readlink -f "$0"))
# echo $scripts_dir

pushd $scripts_dir

mkdir ../data

python3 -m dfss --url=open@sophgo.com:/sophon-stream/yolov5/videos.zip
unzip videos.zip
rm -rf videos.zip
mv ./videos ../data/

mkdir ../data/models
python3 -m dfss --url=open@sophgo.com:/sophon-stream/yolov5/BM1684X.zip
unzip BM1684X.zip
rm -rf BM1684X.zip
mv ./BM1684X ../data/models/BM1684X

python3 -m dfss --url=open@sophgo.com:/sophon-stream/yolov5/BM1684.zip
unzip BM1684.zip
rm -rf BM1684.zip
mv ./BM1684 ../data/models/BM1684

python3 -m dfss --url=open@sophgo.com:/sophon-stream/yolov5/BM1684X_tpukernel.zip
unzip BM1684X_tpukernel.zip
rm -rf BM1684X_tpukernel.zip
mv ./BM1684X ../data/models/BM1684X_tpukernel

python3 -m dfss --url=open@sophgo.com:sophon-demo/YOLOv5/models_a2_0921/BM1688.zip
unzip BM1688.zip
rm -rf BM1688.zip
mv ./BM1688 ../data/models/BM1688

python3 -m dfss --url=open@sophgo.com:sophon-demo/YOLOv5/models_240328/CV186X.zip
unzip CV186X.zip
rm -r CV186X.zip
mv ./CV186X ../data/models/CV186X

python3 -m dfss --url=open@sophgo.com:/sophon-stream/common/coco.names
mv ./coco.names ../data/

popd