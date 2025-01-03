#include "spark_dsg/instance_views.h"

#include <cstdint>
#include <opencv2/core/mat.hpp>
#include <utility>
#include <iostream>

namespace spark_dsg {

void InstanceViews::add_view(uint16_t image_id, cv::Mat& mask) {
  id_to_instance_masks.insert(std::make_pair(image_id, mask));
}

void InstanceViews::merge_views(InstanceViews other) {
  for (const auto& id_mask : other.id_to_instance_masks) {
    uint16_t id = id_mask.first;
    if (id_to_instance_masks.find(id) == id_to_instance_masks.end()) {
      cv::Mat mask = id_mask.second;
      add_view(id, mask);
    }
  }
}
}  // namespace spark_dsg
