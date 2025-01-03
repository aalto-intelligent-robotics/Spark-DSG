#pragma once

#include <cstdint>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <unordered_map>

namespace spark_dsg {

struct InstanceViews {
  // map from image-id to mask
  std::unordered_map<uint16_t, cv::Mat> id_to_instance_masks;

  // Cosntructors.
  InstanceViews() = default;
  virtual ~InstanceViews() = default;

  /**
   * @brief Add a new view
   *
   * @param image_id the id of the frame containing the instance
   * @param mask the semantic mask of the instance
   */
  void add_view(uint16_t image_id, cv::Mat& mask);
  void merge_views(InstanceViews other);
};

}  // namespace spark_dsg
