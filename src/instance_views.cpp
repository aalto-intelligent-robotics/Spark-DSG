#include "spark_dsg/instance_views.h"

#include <cstdint>
#include <opencv2/core/mat.hpp>
#include <utility>

namespace spark_dsg {
View::View(const uint64_t& mask_id, const cv::Mat& mask)
    : mask_id_(mask_id), mask_(mask) {}

void InstanceViews::addView(uint16_t map_view_id, const View& mask) {
  id_to_instance_masks_.insert(std::make_pair(map_view_id, mask));
}

void InstanceViews::mergeViews(InstanceViews other) {
  for (const auto& id_view : other.id_to_instance_masks_) {
    uint16_t map_view_id = id_view.first;
    if (id_to_instance_masks_.find(map_view_id) == id_to_instance_masks_.end()) {
      addView(map_view_id, id_view.second);
    }
  }
}
}  // namespace spark_dsg
