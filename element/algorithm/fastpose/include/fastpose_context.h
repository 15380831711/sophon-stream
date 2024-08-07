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

#include "algorithmApi/context.h"

namespace sophon_stream {
namespace element {
namespace fastpose {
#define USE_OPENCV 1
// #define USE_ASPECT_RATIO 1

enum class HeatmapLossType { MSELoss };

class FastposeContext : public ::sophon_stream::element::Context {
 public:
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