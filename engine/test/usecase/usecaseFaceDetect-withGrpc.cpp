/**
@file usecaseFaceDetect-withGrpc.cpp
@brief 主程序
@details
    FaceDetect-withGrpc单元测试

@author lzw
@date 2020-06-12
@version A001
@copyright Lynxi Technologies Co., Ltd
*/

#include "gtest/gtest.h"
#include "common/Logger.h"
#include "framework/Engine.h"
#include "worker/MandatoryLink.h"
#include "grpc/EngineServer.h"
#include "grpc/Engine.grpc.pb.h"
#include "config.h"
#include <opencv2/opencv.hpp>


#define DECODE_ID 5000
#define RETINA_FACE_ID 5005
#define ENCODE_ID 5006
#define REPORT_ID 5555

/**
@brief FaceDetect-withGrpc集成测试函数入口

@param [in] TestMultiAlgorithmGraph 测试用例命名
@param [in] MultiAlgorithmGraph     测试命名
@return void 无返回值


@UnitCase_ID
FaceDetect_IT_0001

@UnitCase_Name
usecaseFaceDetect-withGrpc

@UnitCase_Description
依次经过解码、retinaFace人脸检测、编码、输出worker，检测结果存储在输入objectMetadata的mSubObjectMetadatas字段下的mSpDataInformation中。
实例化EngineServer，启动EngineServer，给各个worker赋值，定义pipeline中各个worker的先后连接顺序，实例化一个客户端存根stub，
调用stub的OperateGraph和ReceiveReport方法，调用OperateChannel方法，通过clientReader的read方法接收处理数据，并实时显示结果

@UnitCase_Version
V0.1

@UnitCase_Precondition
models文件为本地文件，没有随工程一起上传，需要在对应目录放置models文件夹，包括models文件夹中应该按照目录放置对应显卡的模型文件

@UnitCase_Input
TestMultiAlgorithmGraph, MultiAlgorithmGraph

@UnitCase_ExpectedResult
播放视频，在每一帧都会检测出人脸并将对应的box绘制在相应位置，播放结束程序可以正常退出

*/

TEST(TestMultiAlgorithmGraph, MultiAlgorithmGraph) {
    ivs::logInit("debug","","");

    lynxi::ivs::grpc::EngineServer engineServer;
    std::shared_ptr<std::thread> serverThread
    = std::make_shared<std::thread>([&engineServer]() {
        engineServer.run("0.0.0.0:50050",
                         1024 * 1024 * 100,
                         1024 * 1024 * 100);
    });
    sleep(2);
//    auto& engine = lynxi::ivs::framework::SingletonEngine::getInstance();

    nlohmann::json graphConfigure;
    graphConfigure["graph_id"] = 1;
    nlohmann::json workersConfigure;

    workersConfigure.push_back(makeDecoderWorkerConfig(DECODE_ID,"decoder_worker","nvidia",0,1,0,false,1,"../lib/libmultiMediaApi.so" ));
    workersConfigure.push_back(makeWorkerConfig(REPORT_ID,"report_worker","host",0,1,0,false,1, {}));
    nlohmann::json retinafaceJson = makeAlgorithmConfig("../lib/libalgorithmApi.so","retinafaceDetect","face_detect_retina",
    { "../../share/models/retinaFaceVideo.txt" },
    12, { "data" }, { 1 }, {{3, 360, 640}},  {"face_rpn_cls_prob_reshape_stride32",
                                "face_rpn_bbox_pred_stride32",
                                "face_rpn_landmark_pred_stride32",
                                "face_rpn_cls_prob_reshape_stride16",
                                "face_rpn_bbox_pred_stride16",
                                "face_rpn_landmark_pred_stride16",
                                "face_rpn_cls_prob_reshape_stride8",
                                "face_rpn_bbox_pred_stride8",
                                "face_rpn_landmark_pred_stride8"
                               }, { 9}, {{4, 12, 20}, {8, 12, 20}, {20, 12, 20},
        {4, 23, 40}, {8, 23, 40}, {20, 23, 40},
        {4, 45, 80}, {8, 45, 80}, {20, 45, 80}
    },
    { 0.3,0.4 },1, {"faceDetect"});
    workersConfigure.push_back(makeWorkerConfig(RETINA_FACE_ID, "action_worker", "nvidia", 0, 1, 200, false, 8, {retinafaceJson}));

    nlohmann::json encodeJson = makeEncodeConfig("../lib/libalgorithmApi.so","","encode_picture",1);

    workersConfigure.push_back(makeWorkerConfig(ENCODE_ID,"action_worker","host",0,1,200,true,1, {encodeJson}));

    graphConfigure["workers"] = workersConfigure;

    graphConfigure["connections"].push_back(makeConnectConfig(DECODE_ID,0,RETINA_FACE_ID,0));
    graphConfigure["connections"].push_back(makeConnectConfig(RETINA_FACE_ID,0,ENCODE_ID,0));
    graphConfigure["connections"].push_back(makeConnectConfig(ENCODE_ID,0,REPORT_ID,0));

    std::mutex mtx;
    std::condition_variable cv;
    IVS_INFO("~~~~{0}",graphConfigure.dump());

    std::unique_ptr<lynxi::ivs::grpc::Engine::Stub> stub
        = lynxi::ivs::grpc::Engine::NewStub(::grpc::CreateChannel("localhost:50050",
                                            ::grpc::InsecureChannelCredentials()));


    ::grpc::ClientContext operateGraphContext;
    lynxi::ivs::grpc::GraphOperateRequest graphOperateRequest;
    graphOperateRequest.mutable_graph()->set_json_configure(graphConfigure.dump());
    auto operateChannelOutput = graphOperateRequest.mutable_graph()->add_output_worker_ports();
    operateChannelOutput->set_worker_id(DECODE_ID);
    operateChannelOutput->set_worker_port(1);
    auto reportOutput = graphOperateRequest.mutable_graph()->add_output_worker_ports();
    reportOutput->set_worker_id(REPORT_ID);
    reportOutput->set_worker_port(0);
    lynxi::ivs::grpc::GraphOperateResponse graphOperateResponse;
    ::grpc::Status operateGraphStatus = stub->OperateGraph(&operateGraphContext,
                                        graphOperateRequest,
                                        &graphOperateResponse);
//    engine.addGraph(graphConfigure.dump());

    ::grpc::ClientContext receiveReportContext;
    lynxi::ivs::grpc::ReceiveRequest reportReceiveRequest;
    reportReceiveRequest.set_graph_id(1);
    reportReceiveRequest.mutable_report_worker_port()->set_worker_id(REPORT_ID);
    reportReceiveRequest.mutable_report_worker_port()->set_worker_port(0);
    auto clientReader = stub->ReceiveReport(&receiveReportContext,
                                            reportReceiveRequest);

    std::shared_ptr<std::thread> receiveThread
    = std::make_shared<std::thread>([&clientReader, &cv]() {
        lynxi::ivs::grpc::Report report;
        while (clientReader->Read(&report)) {
        IVS_INFO("~~~~~~~~~~~~~~~~~~~~~recieved result!");
            if (report.object_metadata().packet().end_of_stream()) {
                cv.notify_one();
                clientReader->Finish();
            }
            if(report.object_metadata().packet().data().size()!=0){
                std::vector<uchar> inputarray;
                const std::string& strPacket = report.object_metadata().packet().data();
                for(int i=0;i<strPacket.size();i++){
                    inputarray.push_back(strPacket[i]);
                }
                cv::Mat cpuMat = cv::imdecode(inputarray,CV_LOAD_IMAGE_COLOR);
                int subObjLen = report.object_metadata().sub_object_metadatas_size();
                for(int i=0;i<subObjLen;i++){
                    auto subData = report.object_metadata().sub_object_metadatas(i);
                    IVS_INFO("x:{0}, y:{1},  w:{2}, h:{3}", subData.data_information().box().x(), 
                            subData.data_information().box().y(),subData.data_information().box().width(),
                            subData.data_information().box().height());
                    cv::rectangle(cpuMat, cv::Rect(subData.data_information().box().x(),
                                                   subData.data_information().box().y(),
                                                   subData.data_information().box().width(),
                                                   subData.data_information().box().height()),
                                  cv::Scalar(0, 255, 0), 2);
                    for(auto pair:subData.data_information().key_points()){
                        
                        cv::circle(cpuMat, cv::Point(pair.second.point().x(), pair.second.point().y()), 1, cv::Scalar(255, 255, 0), 1, 8, 0);
                    }
                }
                cv::imshow("123", cpuMat);
                cv::waitKey(1);
            }

        }
    });

    nlohmann::json decodeConfigure;
    decodeConfigure["channel_id"] = 1;
    decodeConfigure["url"] = "../test/out-720p-2s.mp4";
    //decodeConfigure["url"] = "../test/18.mp4";
    decodeConfigure["resize_rate"] = 2.0f;
    decodeConfigure["timeout"] = 0;
    decodeConfigure["source_type"] = 0;
    decodeConfigure["multimedia_name"] = "decode_picture";
//    channelTask->request.json = decodeConfigure.dump();
//    std::shared_ptr<void> data = std::static_pointer_cast<void>(channelTask);
//    engine.sendData(1,DECODE_ID,0,data,std::chrono::seconds(4));

    ::grpc::ClientContext operateChannelContext;
    lynxi::ivs::grpc::ChannelOperateRequest channelOperateRequest;
    channelOperateRequest.set_operate(lynxi::ivs::grpc::ChannelOperateRequest::ADD);
    channelOperateRequest.set_graph_id(1);
    channelOperateRequest.mutable_request_worker_port()->set_worker_id(DECODE_ID);
    channelOperateRequest.mutable_request_worker_port()->set_worker_port(0);
    channelOperateRequest.mutable_response_worker_port()->set_worker_id(DECODE_ID);
    channelOperateRequest.mutable_response_worker_port()->set_worker_port(1);
    IVS_INFO("decode config {0}",decodeConfigure.dump());
    channelOperateRequest.mutable_channel()->set_json_configure(decodeConfigure.dump());
    lynxi::ivs::grpc::ChannelOperateResponse channelOperateResponse;
    ::grpc::Status operateChannelStatus = stub->OperateChannel(&operateChannelContext,
                                          channelOperateRequest,
                                          &channelOperateResponse);

    {
        std::unique_lock<std::mutex> uq(mtx);
        cv.wait(uq);
    }
    engineServer.stop();
    receiveThread->join();
    serverThread->join();
}
