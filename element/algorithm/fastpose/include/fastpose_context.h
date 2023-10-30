//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_ELEMENT_FASTPOSE_CONTEXT_H_
#define SOPHON_STREAM_ELEMENT_FASTPOSE_CONTEXT_H_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "bmcv_api.h"
#include "bmcv_api_ext.h"
#include "bmlib_runtime.h"
#include "bmruntime_interface.h"
#include "common/bmnn_utils.h"
#include "common/error_code.h"
#include "common/object_metadata.h"

namespace sophon_stream {
namespace element {
namespace fastpose {
#define USE_OPENCV 1
// #define USE_ASPECT_RATIO 1
#define FFALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

enum class HeatmapLossType {
  MSELoss
};

struct FastposeContext {
  int deviceId;  // 设备ID

  std::shared_ptr<BMNNContext> bmContext;
  std::shared_ptr<BMNNNetwork> bmNetwork;
  bm_handle_t handle;

  int m_net_h, m_net_w;
  int m_net_channel;
  int max_batch;
  HeatmapLossType heatmap_loss;
  float area_thresh;
  bmcv_convert_to_attr converto_attr;
  int output_num;
  int input_num;
};
}  // namespace fastpose
}  // namespace element
}  // namespace sophon_stream

#endif  // SOPHON_STREAM_ELEMENT_FASTPOSE_CONTEXT_H_