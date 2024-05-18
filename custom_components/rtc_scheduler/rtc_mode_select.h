#pragma once
#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "rtc_text_sensor.h"
#include "rtc_scheduler.h"
#include "esphome/core/preferences.h"
#include <vector>
#include <string>


namespace esphome {
namespace rtc_scheduler {
class RTCSchedulerControllerSwitch;  // Заплановане перемикання 
class RTCSchedulerTextSensor;         // Датчик тексту для відображення статусу в інтерфейсі HA
enum ItemMode : size_t {
  ITEM_MODE_OFF = 0,
  ITEM_MODE_EARLY,
  ITEM_MODE_AUTO,
  ITEM_MODE_ON,
  ITEM_MODE_BOOST
};
enum ItemModeState : size_t {
  ITEM_STATE_INIT = 0,
  ITEM_STATE_OFF,
  ITEM_STATE_EARLY,
  ITEM_STATE_AUTO_OFF,
  ITEM_STATE_AUTO_ON,
  ITEM_STATE_ON,
  ITEM_STATE_BOOST
};
class RTCSchedulerItemMode_Select : public select::Select, public Component {
  public:
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void setup() override;
  void dump_config() override;
  void configure_item(uint8_t item_slot_number,
                      RTCSchedulerControllerSwitch *item_sw,
                      switch_::Switch *item_sw_id,
                      RTCSchedulerTextSensor *item_status,
                      RTCSchedulerTextSensor *item_next_event,
                      binary_sensor::BinarySensor* item_on_indicator
                      );
  void set_item_schedule_valid(bool schedule_valid);
  void set_controller_state(bool schedule_controller_state);
  bool get_controller_state();
  void set_scheduled_item_state(bool scheduled_item_state);
  bool get_scheduled_item_state();
  void set_next_scheduled_event(uint16_t next_schedule_event, bool next_schedule_state);
  uint8_t get_slot_number();
 protected:
  void adjustItemInternalState();
  void prepareItemSensorAndSwitches();
  void adjustItemSensorAndSwitches(bool newState,std::string &newValue);
  void adjustIndicatorState(bool newValue);
  void adjustSwitchState(bool newValue);
  void adjustStatusTextSensor(std::string &newValue);
  void adjustEventTextSensor(std::string &newValue);
  std::string convertEventTimeToStr(uint16_t event_time) const;
  void control(const std::string &value) override; 
  std::string item_mode_state_str_ = "Not Configured Off";
  // Номер слота запланованого елемента, який використовується для отримання розкладу з флеш-пам’яті
  uint8_t item_slot_number_ = 0;
  // Додатковий перемикач, щоб дозволити запланованому елементу використовувати turn_on/turn_off 
  RTCSchedulerControllerSwitch *item_sw_{nullptr};
  // Додатковий сенсор тексту для відображення статусу запланованого елемента в HA
  RTCSchedulerTextSensor *item_status_{nullptr};  
  // Додатковий сенсор тексту для відображення наступної події для запланованого елемента в HA
  RTCSchedulerTextSensor *item_next_event_{nullptr};  
  // Додатковий бінарний датчик для вказівки HA стану запланованого елемента
  binary_sensor::BinarySensor* item_on_indicator_ = nullptr; 
  // Додатковий ідентифікатор перемикача EG GPIO ID, щоб запланований елемент міг ним керувати
  switch_::Switch *item_sw_id_{nullptr};
  ESPPreferenceObject pref_;
  // Використовується запланованим елементом, щоб підтримувати стан контролера вимкнено або увімкнено
  bool schedule_controller_state_ = false;
  // Використовується запланованим елементом для підтримки його стану перемикача, вимкнено або увімкнено 
  bool schedule_controller_item_state_ = false;
  // Використовується для вказівки, чи розклад для елемента дійсний чи ні
  bool schedule_valid_ = false;
  // Текстовий рядок, який містить наступну подію розкладу
  std::string next_schedule_event_str_ = "";
  // Тривалість події в хвилинах (0-6 Days, 0-23 Hours, 0-59 minutes = Max 10080)
  uint16_t next_schedule_event_ = 10081; // Зверніть увагу, що будь-яке значення вище 10080 є недійсним, наприклад, немає події
  // Наступний стан події EG вимкнено або увімкнено
  bool next_schedule_state_ = false;

  ItemMode item_mode_ = ITEM_MODE_OFF;
  ItemModeState item_mode_state_ = ITEM_STATE_INIT;
};

}  // namespace rtc_scheduler
}  // namespace esphome
