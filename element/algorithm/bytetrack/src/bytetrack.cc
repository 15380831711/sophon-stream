//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "bytetrack.h"

#include <nlohmann/json.hpp>

#include "common/logger.h"
#include "element_factory.h"

namespace sophon_stream {
namespace element {
namespace bytetrack {

Bytetrack::Bytetrack() { IVS_DEBUG("Bytetrack Element construct!!!"); }

Bytetrack::~Bytetrack() {}

common::ErrorCode Bytetrack::initContext(const std::string& json) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;
  do {
    IVS_DEBUG("Bytetrack::initContext");
    mContext = std::make_shared<BytetrackContext>();

    auto configure = nlohmann::json::parse(json, nullptr, false);
    if (!configure.is_object()) {
      IVS_WARN("Bytetrack::initContext: PARSE_CONFIGURE_FAIL");
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }

    auto frameRateIt = configure.find(CONFIG_INTERNAL_FRAME_RATE_FIELD);
    mContext->frameRate =
        frameRateIt != configure.end() ? frameRateIt->get<int>() : 30;

    auto trackBufferIt = configure.find(CONFIG_INTERNAL_TRACK_BUFFER_FIELD);
    mContext->trackBuffer =
        trackBufferIt != configure.end() ? trackBufferIt->get<int>() : 30;

    auto minBoxAreaIt = configure.find(CONFIG_INTERNAL_MIN_BOX_AREA_FIELD);
    mContext->minBoxArea =
        minBoxAreaIt != configure.end() ? minBoxAreaIt->get<int>() : 10;

    auto trackThreshIt = configure.find(CONFIG_INTERNAL_TRACK_THRESH_FIELD);
    mContext->trackThresh =
        trackThreshIt != configure.end() ? trackThreshIt->get<float>() : 0.5;

    auto highThreshIt = configure.find(CONFIG_INTERNAL_HIGH_THRESH_FIELD);
    mContext->highThresh =
        highThreshIt != configure.end() ? highThreshIt->get<float>() : 0.6;

    auto matchThreshIt = configure.find(CONFIG_INTERNAL_MATCH_THRESH_FIELD);
    mContext->matchThresh =
        matchThreshIt != configure.end() ? matchThreshIt->get<float>() : 0.7;

    auto correctBoxIt = configure.find(CONFIG_INTERNAL_CORRECT_BOX_FIELD);
    mContext->correctBox =
        correctBoxIt != configure.end() ? correctBoxIt->get<bool>() : true;

    auto agnosticIt = configure.find(CONFIG_INTERNAL_AGNOSTIC_FIELD);
    mContext->agnostic =
        agnosticIt != configure.end() ? agnosticIt->get<bool>() : true;

    IVS_DEBUG(
        "Bytetrack::initContext: frameRate: {0}, trackBuffer: {1}, "
        "trackThresh: {2}, "
        "highThresh: {3}, matchThresh: {4}, correctBox: {5}, agnostic: {6}",
        mContext->frameRate, mContext->trackBuffer, mContext->trackThresh,
        mContext->highThresh, mContext->matchThresh, mContext->correctBox, mContext->agnostic);

  } while (false);

  return common::ErrorCode::SUCCESS;
}

common::ErrorCode Bytetrack::initInternal(const std::string& json) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;

  do {
    IVS_DEBUG("Bytetrack::initInternal");
    // json是否正确
    auto configure = nlohmann::json::parse(json, nullptr, false);
    if (!configure.is_object()) {
      IVS_WARN("Bytetrack::initInternal: PARSE_CONFIGURE_FAIL");
      errorCode = common::ErrorCode::PARSE_CONFIGURE_FAIL;
      break;
    }

    // 获取参数
    initContext(configure.dump());

    int threadNumber = getThreadNumber();

    IVS_DEBUG("Bytetrack threadNumber: {0}", threadNumber);

    // 初始化 tracker
    for (int t = 0; t < threadNumber; ++t) {
      mByteTrackerMap[t] = std::make_shared<BYTETracker>(mContext);
    }

  } while (false);

  return errorCode;
}

/**
 * update tracker
 * @param[in/out] objectMetadatas:  更新 tracker
 */
void Bytetrack::process(
    int dataPipeId, std::shared_ptr<common::ObjectMetadata>& objectMetadata) {
  auto byteTrackerIt = mByteTrackerMap.find(dataPipeId);
  if (mByteTrackerMap.end() != byteTrackerIt) {
    auto byteTracker = byteTrackerIt->second;
    if (byteTracker) {
      byteTracker->update(objectMetadata);
    } else {
      IVS_WARN("empty byteTrackerMap for dataPipeId : {0}", dataPipeId);
    }
  }
}

/**
  运行
*/
common::ErrorCode Bytetrack::doWork(int dataPipeId) {
  common::ErrorCode errorCode = common::ErrorCode::SUCCESS;

  std::vector<int> inputPorts = getInputPorts();
  int inputPort = inputPorts[0];
  int outputPort = 0;
  if (!getSinkElementFlag()) {
    std::vector<int> outputPorts = getOutputPorts();
    int outputPort = outputPorts[0];
  }

  if (getThreadStatus() != ThreadStatus::RUN) {
    IVS_DEBUG("Bytetrack doWork thread stop");
    return errorCode;
  }

  common::ObjectMetadatas pendingObjectMetadatas;

  std::shared_ptr<common::ObjectMetadata> objectMetadata = nullptr;

  while (getThreadStatus() == ThreadStatus::RUN) {
    auto data = popInputData(inputPort, dataPipeId);
    if (!data) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    objectMetadata = std::static_pointer_cast<common::ObjectMetadata>(data);
    pendingObjectMetadatas.push_back(objectMetadata);
    if (!objectMetadata->mFilter) {
      break;
    }
    if (objectMetadata->mFrame->mEndOfStream) {
      break;
    }
  }
  if (objectMetadata != nullptr) process(dataPipeId, objectMetadata);

  for (auto& obj : pendingObjectMetadatas) {
    int channel_id_internal = obj->mFrame->mChannelIdInternal;
    int pipeId =
        getSinkElementFlag()
            ? 0
            : (channel_id_internal % getOutputConnectorCapacity(outputPort));

    errorCode =
        pushOutputData(outputPort, pipeId, std::static_pointer_cast<void>(obj));
    if (common::ErrorCode::SUCCESS != errorCode) {
      IVS_WARN(
          "Send data fail, element id: {0:d}, output port: {1:d}, data: "
          "{2:p}",
          getId(), outputPort, static_cast<void*>(obj.get()));
    }
  }

  return common::ErrorCode::SUCCESS;
}

REGISTER_WORKER("bytetrack", Bytetrack)

}  // namespace bytetrack
}  // namespace element
}  // namespace sophon_stream
