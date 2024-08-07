//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_ELEMENT_OPENPOSE_CONTEXT_H_
#define SOPHON_STREAM_ELEMENT_OPENPOSE_CONTEXT_H_

#include "algorithmApi/context.h"

namespace sophon_stream {
namespace element {
namespace openpose {
#define USE_OPENCV 1
// #define USE_ASPECT_RATIO 1

class OpenposeContext : public ::sophon_stream::element::Context {
 public:
  int deviceId;  // 设备ID

  std::shared_ptr<BMNNContext> bmContext;
  std::shared_ptr<BMNNNetwork> bmNetwork;
  bm_handle_t handle;

  bool use_tpu_kernel = false;
  tpu_kernel_function_t func_id;
  int net_h, net_w;
  int m_net_channel;
  int max_batch;
  common::PosedObjectMetadata::EModelType m_model_type;
  bmcv_convert_to_attr converto_attr;
  int output_num;
  int input_num;
  float nms_threshold;
  int thread_number;
};
}  // namespace openpose
}  // namespace element
}  // namespace sophon_stream

#endif  // SOPHON_STREAM_ELEMENT_OPENPOSE_CONTEXT_H_