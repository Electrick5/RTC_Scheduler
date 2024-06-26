#pragma once
#include "rtc_scheduler.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace rtc_scheduler {
class RTCSchedulerTextSensor;         // Датчик тексту для відображення статусу в інтерфейсі HA
class RTCSchedulerTextSensor :  public text_sensor::TextSensor, public Component {
 public:
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void dump_config() override;
};
}
}
