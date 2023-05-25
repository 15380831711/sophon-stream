#include <nlohmann/json.hpp>
#include "Yolov5Algorithm.h"
#include "../../../../framework/ElementFactory.h"
#include "common/Logger.h"

namespace sophon_stream {
namespace algorithm {


constexpr const char* JSON_BATCH_FIELD = "batch";
constexpr const char* JSON_STAGE_NAME = "stage";


/**
 * 构造函数
 */
Yolov5Algorithm::Yolov5Algorithm() {
}

/**
 * 析构函数
 */
Yolov5Algorithm::~Yolov5Algorithm() {
    // std::cout << "Algorithm uninit" << std::endl;


}


void Yolov5Algorithm::doSth() {

}

/**
 * 初始化函数
 * @param[in] side:  设备类型
 * @param[in] deviceId:  设备ID
 * @param[in] json:  初始化字符串
 * @return 错误码
 */
common::ErrorCode Yolov5Algorithm::initInternal(const std::string& json) {
    common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
    do {
        //json是否正确
        auto configure = nlohmann::json::parse(json, nullptr, false);
        if (!configure.is_object()) {
            errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
            break;
        }

        auto batchIt = configure.find(JSON_BATCH_FIELD);
        if (configure.end() == batchIt
                || !batchIt->is_number_integer()) {
            IVS_ERROR("Can not find {0} with integer type in action element json configure, element id: {1:d}, json: {2}",
                      JSON_BATCH_FIELD,
                      getId(),
                      json);
            errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
            break;
        }

        mBatch = batchIt->get<int>();
        mPendingObjectMetadatas.reserve(mBatch);

        auto stageNameIt = configure.find(JSON_STAGE_NAME);
        if(configure.end() == stageNameIt || !stageNameIt->is_array())
        {
            errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
            break;
        }

        std::vector<std::string> stages = stageNameIt->get<std::vector<std::string>>();
        if(std::find(stages.begin(), stages.end(), "pre")!=stages.end())
            use_pre = true;
        if(std::find(stages.begin(), stages.end(), "infer")!=stages.end())
            use_infer = true;
        if(std::find(stages.begin(), stages.end(), "post")!=stages.end())
            use_post = true;


        // 新建context,预处理,推理和后处理对象
        mContext = std::make_shared<yolov5::Yolov5SophgoContext>();
        mPreProcess = std::make_shared<yolov5::Yolov5Pre>();
        mInference = std::make_shared<yolov5::Yolov5Inference>();
        mPostProcess = std::make_shared<yolov5::Yolov5Post>();

        if(!mPreProcess
                || !mInference
                || !mPostProcess || !mContext) {
            // errorCode = common::ErrorCode::ALGORITHM_FACTORY_MAKE_FAIL;
            break;
        }


        //context初始化
        mContext->init(configure.dump());


        // mContext->algorithmName = Yolov5Algorithm::Name;
        mContext->algorithmName = "Yolov5";
        mContext->deviceId = getDeviceId();
        //推理初始化
        errorCode = mInference->init(*mContext);
        //后处理初始化
        mPostProcess->init(*mContext);

    } while(false);
    return errorCode;
}

/**
 * 预测函数
 * @param[in/out] objectMetadatas:  输入数据和预测结果
 */
void Yolov5Algorithm::process(common::ObjectMetadatas& objectMetadatas) {
        common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
        if(use_pre)
        {
            errorCode = mPreProcess->preProcess(*mContext, objectMetadatas);
            if (common::ErrorCode::SUCCESS != errorCode) {
                for (unsigned i = 0; i < objectMetadatas.size(); i++) {
                    objectMetadatas[i]->mErrorCode = errorCode;
                }
                return;
            }
        }
        //推理
        if(use_infer)
        {
            errorCode = mInference->predict(*mContext, objectMetadatas);
            if (common::ErrorCode::SUCCESS != errorCode) {
                for (unsigned i = 0; i < objectMetadatas.size(); i++) {
                    objectMetadatas[i]->mErrorCode = errorCode;
                }
                return;
            }
        }
        //后处理
        if(use_post)
            mPostProcess->postProcess(*mContext, objectMetadatas);
}

/**
 * 资源释放函数
 */
void Yolov5Algorithm::uninitInternal() {
        if (mInference != nullptr) {
            mInference->uninit();
            mInference = nullptr;
        }
}


common::ErrorCode Yolov5Algorithm::doWork() {
    common::ErrorCode errorCode = common::ErrorCode::SUCCESS;

    common::ObjectMetadatas objectMetadatas;
    {
        std::lock_guard<std::mutex> lock(mMutex2);
        
        std::size_t currentDataCount = getDataCount(0);
        bool timeout = mLastDataCount == currentDataCount;
        mLastDataCount = currentDataCount;

        //IVS_INFO("element id:{2}, timeout:{0}, current data count:{1}",timeout, currentDataCount, getId());
        errorCode = sendProcessedData();
        if (common::ErrorCode::SUCCESS != errorCode) {
            return errorCode;
        }

        int pendingSize = mPendingObjectMetadatas.size();
        int iCurrentdataCount = currentDataCount;
        if (mBatch - pendingSize > iCurrentdataCount
                && !timeout) {

            return common::ErrorCode::SUCCESS;
        }

        // ActionElement凑batch
        while(mPendingObjectMetadatas.size() < mBatch && !hasEof)
        {
            // 如果队列为空则等待
            if(getDataCount(0) == 0)
            {
                continue;
            }
            auto data = getData(0);
            if (!data) {
                popData(0);
                continue;
            }
            auto objectMetadata = std::static_pointer_cast<common::ObjectMetadata>(data);
            bool skip = false;

            if (objectMetadata->mFilter) {
                skip = true;
            }

            // auto modelConfigureMap = objectMetadata->mModelConfigureMap;
            // if (modelConfigureMap) {
            //     IVS_WARN("modelConfigure map has data!");
            //     bool skipAllModel = true;
            //     for (auto pair : mAlgorithmApis) {
            //         auto modelConfigureIt = modelConfigureMap->find(pair.first);
            //         if (modelConfigureMap->end() == modelConfigureIt
            //                 || modelConfigureIt->second) {
            //             skipAllModel = false;
            //             break;
            //         }
            //     }
            //     if (skipAllModel) {
            //         skip = true;
            //     }
            // }

            if (skip) {
                mProcessedObjectMetadatas.push_back(objectMetadata);
            } else {
                mPendingObjectMetadatas.push_back(objectMetadata);
            }
            popData(0);
            
            // 如果遇到了eof，将eof帧凑到batch里之后截止
            if(objectMetadata->mFrame->mEndOfStream)
            {    
                hasEof = true;
                break;
            }
        }

        // 遇到了eof的情况凑batch
        if(mPendingObjectMetadatas.size() != 0 && mPendingObjectMetadatas.back()->mFrame!=nullptr&& mPendingObjectMetadatas.back()->mFrame->mEndOfStream)
        {
            while(mPendingObjectMetadatas.size() < mBatch)
            {
                auto ObjectMetadata = std::make_shared<common::ObjectMetadata>();
                ObjectMetadata->mFrame = std::make_shared<common::Frame>();
                ObjectMetadata->mFrame->mEndOfStream = true;
                mPendingObjectMetadatas.push_back(ObjectMetadata);
            }
        }

        mLastDataCount = getDataCount(0);

        errorCode = sendProcessedData();
        if (common::ErrorCode::SUCCESS != errorCode) {
            return errorCode;
        }

        if (mPendingObjectMetadatas.size() < mBatch
                && !timeout) {
            return common::ErrorCode::SUCCESS;
        }

        objectMetadatas.swap(mPendingObjectMetadatas);
    }


    process(objectMetadatas);

    {
        std::lock_guard<std::mutex> lock(mMutex2);

        for (auto objectMetadata : objectMetadatas) {
            mProcessedObjectMetadatas.push_back(objectMetadata);
        }
        errorCode = sendProcessedData();
        if (common::ErrorCode::SUCCESS != errorCode) {
            return errorCode;
        }
    }

    return common::ErrorCode::SUCCESS;
}

common::ErrorCode Yolov5Algorithm::sendProcessedData() {
    while (!mProcessedObjectMetadatas.empty()) {
        auto objectMetadata = mProcessedObjectMetadatas.front();
        common::ErrorCode errorCode = sendData(0,
                                               std::static_pointer_cast<void>(objectMetadata),
                                               std::chrono::milliseconds(200));
        if (common::ErrorCode::SUCCESS != errorCode) {
            IVS_WARN("Send data fail, element id: {0:d}, output port: {1:d}, data: {2:p}, element id:{3:d}",
                     getId(),
                     0,
                     static_cast<void*>(objectMetadata.get()), getId());
            return errorCode;
        }

        mProcessedObjectMetadatas.pop_front();
    }

    return common::ErrorCode::SUCCESS;
}

REGISTER_WORKER("Yolov5", Yolov5Algorithm)

} // namespace algorithm
} // namespace sophon_stream
