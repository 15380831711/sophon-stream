//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-STREAM is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef SOPHON_STREAM_ELEMENT_CONVERGER_H_
#define SOPHON_STREAM_ELEMENT_CONVERGER_H_

#include <map>
#include <unordered_map>

#include "common/object_metadata.h"
#include "element.h"

namespace sophon_stream {
namespace element {
namespace converger {

class Converger : public ::sophon_stream::framework::Element {
 public:
  Converger();
  ~Converger() override;

  common::ErrorCode initInternal(const std::string& json) override;

  common::ErrorCode doWork(int dataPipeId) override;

  static constexpr const char* CONFIG_INTERNAL_DEFAULT_PORT_FILED =
      "default_port";

 private:
  int mDefaultPort;
  /**
   * @brief key: channel_id, value: map<sub_frame_id, objectMetadata>
   */
  std::unordered_map<int,
                     std::map<std::int64_t, std::shared_ptr<common::ObjectMetadata>>>
      mCandidates;
  /**
   * @brief recored number of branches for every ObjectMetadata in default_port
   */
  std::unordered_map<int, std::unordered_map<int, int>> mBranches;

  std::mutex mtx;
};

}  // namespace converger
}  // namespace element
}  // namespace sophon_stream

#endif