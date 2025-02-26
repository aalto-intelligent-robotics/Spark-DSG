#pragma once

#include <cstdint>
#include <map>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>

namespace spark_dsg {

struct View {
  uint64_t mask_id_;  // for bookkeeping the embeddings
  cv::Mat mask_;

  // Cosntructors.
  View() = default;
  View(const uint64_t& mask_id, const cv::Mat& mask);
  virtual ~View() = default;
};

struct InstanceViews {
  /**
   * @brief mapped as {map_view_id -> View(mask_id, mask)}
   */
  std::map<uint64_t, View> id_to_instance_masks_;

  InstanceViews() = default;
  virtual ~InstanceViews() = default;

  /**
   * @brief Add a new view
   *
   * @param map_view_id the id of the frame containing the instance
   * @param mask the semantic mask of the instance
   */
  void addView(uint16_t map_view_id, const View& mask);
  /**
   * @brief Merge with another InstanceView
   *
   * @param other InstanceView to merge with
   */
  void mergeViews(InstanceViews other);
};

}  // namespace spark_dsg
