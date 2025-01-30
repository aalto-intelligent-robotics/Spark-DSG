#pragma once

#include <cstdint>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <map>

namespace spark_dsg {

struct InstanceViews {
  // map from image-id to mask
  std::map<uint16_t, cv::Mat> id_to_instance_masks;

  // Cosntructors.
  InstanceViews() = default;
  virtual ~InstanceViews() = default;

  /**
   * @brief Add a new view
   *
   * @param image_id the id of the frame containing the instance
   * @param mask the semantic mask of the instance
   */
  void addView(uint16_t image_id, cv::Mat& mask);
  void mergeViews(InstanceViews other);
};

}  // namespace spark_dsg
