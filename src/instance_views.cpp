#include "spark_dsg/instance_views.h"

#include <opencv2/core/mat.hpp>
#include <utility>

namespace spark_dsg {

void InstanceViews::add_view(uint16_t image_id, cv::Mat& mask) {
  id_to_instance_masks.insert(std::pair<uint16_t, cv::Mat>(image_id, mask));
}

}  // namespace spark_dsg
