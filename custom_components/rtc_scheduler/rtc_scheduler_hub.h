#pragma once
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/hal.h"
#include "rtc_scheduler.h"
#include "../external_eeprom/external_eeprom.h"

namespace esphome {
namespace rtc_scheduler {
class RTCScheduler;                       // контролер планувальника
class RTCSchedulerHub;                    // цей компонент

class RTCSchedulerHub : public Component, public api::CustomAPIDevice {
 public:
//   RTCSchedulerHub();
//   RTCSchedulerHub(const std::string &name);  
   void setup() override;
   void loop() override;
   void dump_config() override;
   float get_setup_priority() const override { return setup_priority::DATA; }
   void add_controller(RTCScheduler *schedule_controller);
   void on_text_schedule_recieved(std::string scheduler_id,int schedule_slot_id, std::string events);
   void on_schedule_recieved(std::string scheduler_id,int schedule_slot_id, std::vector<int> days ,std::vector<int> hours ,std::vector<int> minutes, std::vector<std::string> actions);
   void on_schedule_erase_recieved(std::string scheduler_id,int schedule_slot_id);
   void on_erase_all_schedules_recieved(std::string scheduler_id);
   void send_log_message_to_ha(std::string level, std::string logMessage, std::string sender);
   void send_event_to_ha(std::string event_str);
   void send_notification_to_ha(std::string title, std::string message,std::string id );
   void set_storage(external_eeprom::ExtEepromComponent *storage) ;
   void set_name(const std::string &name) { this->name_ = name; }
   void display_storage_status();
  

 protected:
   RTCScheduler* get_scheduler(std::string &scheduler_id);
   std::vector<RTCScheduler *> schedule_controllers_;
   external_eeprom::ExtEepromComponent *storage_;
   std::string name_;
};



} // namespace rtc_scheduler
}  // namespace esphome
