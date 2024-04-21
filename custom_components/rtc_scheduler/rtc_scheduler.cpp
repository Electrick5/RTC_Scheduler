#include "automation.h"
#include "rtc_scheduler.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include <utility>
#include <string>

namespace esphome {
namespace rtc_scheduler {

static const char *TAG = "rtc_scheduler";
// TODO Add timing code
//RTCScheduler::RTCScheduler() {}
//RTCScheduler::RTCScheduler(const std::string &name) : EntityBase(name) {}

void RTCScheduler::setup() {

}

void RTCScheduler::loop() {
 if (!storage_configured)
 {
  this->configure_storage();
 }
    

// Перевірте розклад подій щодо поточного часу
// Якщо час події минуло, установіть стани перемикача за розкладом, якщо не активовано заміну
// Оновити наступний розклад наступної події для кожного комутатора
}

void RTCScheduler::dump_config(){
    ESP_LOGCONFIG(TAG, "Scheduler component");
    ESP_LOGCONFIG(TAG, "RTC Scheduler Controller -- %s", this->name_.c_str());
     std::string service_sched_name;
     service_sched_name =  this->name_;
 // remove the spaces
 for (size_t i = 0; i < service_sched_name.size(); ++i) {
    if (service_sched_name[i] == ' ') {
        service_sched_name.replace(i, 1, "_");
    }
}
ESP_LOGCONFIG(TAG, "RTC Scheduler Controller name -- %s", service_sched_name.c_str());
    // TODO dump config for EEPROM and Switches

    
}
void RTCScheduler::configure_storage(){
  storage_configured = true;
  storage_valid_ = false;
  // Calc the scheduler slot size
  this->slot_size_ = (this->max_switch_events_ * 2) + 4; // перетворити події чисел у байти та додати 4 для слів конфігурації
  //  lets check eeprom is valid
  if (storage_!=nullptr)
    {
      uint16_t data1;
      uint16_t data2;
      uint16_t storage_address;
      this->storage_->read_object(this->storage_offset_, data1);
      this->storage_->read_object(storage_offset_+2, data2);
      if (( data1 == SCHEDULER_VALID_WORD_1 ) and ( data2 == SCHEDULER_VALID_WORD_2 ))
        {
          storage_valid_ = true;
           ESP_LOGD(TAG, "Scheduler good - looking for valid slots");
          for (uint16_t i = 0; i < this->scheduled_items_.size(); i++)  // проходити між предметами в слоті
            {
               // Check if slot is valid, if not move on
                this->storage_->read_object(this->get_slot_starting_address(i),data1); 
                if (data1 == SLOT_VALID_WORD_1)
                {
                     // Check if checksum is correct
                    if (this->check_the_cksm(i))
                    {
                      // Now checks have passed set item to valid
                      this->set_slot_valid(i,true);
                       ESP_LOGD(TAG, "Scheduler - Slot %d valid", i+1);
                       //TODO Необхідно встановити стан програмного забезпечення елемента залежно від режиму та попереднього стану з розкладу
                       
                    }
                    else
                    {
                        // Помилка контрольної суми, тому встановіть для елемента значення недійсне
                      this->set_slot_valid(i,false);
                      ESP_LOGD(TAG, "Scheduler - Slot %d checksum failed", i+1);
                    }
                }
                else
                {
                   ESP_LOGD(TAG, "Scheduler - Slot %d not valid", i+1);
                }
            }

        }
      else
        {
          // не налаштовано, тому налаштуйте його зараз
          data1=SCHEDULER_VALID_WORD_1;
          data2=SCHEDULER_VALID_WORD_2;
          this->storage_->write_object(this->storage_offset_,data1);
          this->storage_->write_object(this->storage_offset_+2,data2);
          storage_valid_ = true;
          data1 = SLOT_INVALID_WORD_1;
          // за визначенням усі слоти в цьому планувальнику недійсні
          // може статися, якщо кількість слотів попереднього планувальника зміниться
          // Тепер налаштуйте слоти для цього планувальника (це означає, що вони недійсні)
          // спочатку перевірте кількість слотів і максимальну кількість подій для цього планувальника для безпеки
          if ((this->max_switch_events_>=1) and (this->scheduled_items_count_>=1))
            {
              for (uint16_t i = 0; i < this->scheduled_items_.size(); i++)  // перейдіть до sw items
               { 
                this->storage_->write_object(this->get_slot_starting_address(i),data1);
                //  set item to invalid
                this->set_slot_valid(i,false);
               } 
              // TODO Надішліть повідомлення ha, щоб повідомити користувачеві запрограмувати розклади
              this->parent_->send_notification_to_ha("Scheduler Ready", "Need to send schedule to"+this->name_+"for scheduler to opperate","432" );
              
               ESP_LOGD(TAG, "Scheduler storage ready - HA user needs to send schedules");  
            }
          else
            {
              // Ніколи не має потрапляти сюди, оскільки перевірка конфігурації має охоплювати це
              storage_valid_ = false;
              ESP_LOGD(TAG, "Неправильна конфігурація – планувальник не працюватиме");
            }
        }
    }
  else
    {
        storage_valid_ = false;
        ESP_LOGD(TAG, "Сховище не виявлено – планувальник не працюватиме");
        // TODO raise error with HA
        this->parent_->send_notification_to_ha("Scheduler Error", "Storage not detected","433" );
    }
}
void RTCScheduler:: test(){
                ESP_LOGD(TAG, "Sched Mem size in bytes: %d",this->storage_->get_memory_size());
//                int myValue2 = -366;
//                this->storage_->write_object(10, myValue2); //(location, data)
                uint32_t myRead2;
                this->storage_->read_object(10, myRead2); //location to read, thing to put data into
                ESP_LOGD(TAG, "I as sched read: %d",myRead2 );
}


void RTCScheduler::on_text_schedule_recieved(int schedule_slot_id, std::string &events) {
    ESP_LOGD(TAG, "%s Text Schedule Slot %d   recieved %s",this->name_.c_str(), schedule_slot_id, events.c_str());
    this->parent_->send_notification_to_ha("Text Rxed","Have rx a text schedule","103");
    
    //TODO convert to internal format and store it

}

void RTCScheduler::on_schedule_recieved(int schedule_slot_id,  std::vector<int> days ,std::vector<int> hours ,std::vector<int> minutes, std::vector<std::string> &actions) {
    ESP_LOGD(TAG, "%s Schedule Slot %d   recieved",this->name_.c_str(), schedule_slot_id);
    ESP_LOGD(TAG, "Entries Count - Day:%d, Hours: %d Mins:%d, Actions: %d",days.size(),hours.size(), minutes.size(), actions.size() );
    this->parent_->send_log_message_to_ha("error","The test message from Controller","ESPHome: boiler_controller");
    //TODO перетворіть у внутрішній формат і збережіть його
    // Перевірте та запишіть дані в eeprom
   
    // Налаштувати наступний розклад наступної події для комутатора
}

void  RTCScheduler::on_schedule_erase_recieved(int schedule_slot_id){
    ESP_LOGD(TAG, "%s Schedule Slot %d  erase recieved",this->name_.c_str() ,schedule_slot_id);
    // TODO Позначити слот як неактивний і очистити дані
}
void  RTCScheduler::on_erase_all_schedules_recieved(){
        ESP_LOGD(TAG, "%s Erase all schedules recieved",this->name_.c_str());
        // TODO Позначте всі слоти як неактивні та очистіть дані та вимкніть цикл розкладу
}
void RTCScheduler::add_controller(RTCScheduler *other_controller)
{
    this->other_controllers_.push_back(other_controller);
}

void RTCScheduler::set_controller_main_switch(RTCSchedulerControllerSwitch *controller_switch)
{
  this->controller_sw_ = controller_switch;
  this->controller_sw_->set_restore_state(true);
  this->scheduler_turn_off_automation_ = make_unique<Automation<>>(controller_switch->get_turn_off_trigger());
  this->scheduler_shutdown_action_ = make_unique<rtc_scheduler::ShutdownAction<>>(this);
  this->scheduler_turn_off_automation_->add_actions({scheduler_shutdown_action_.get()});

  this->scheduler_turn_on_automation_ = make_unique<Automation<>>(controller_switch->get_turn_on_trigger());
  this->scheduler_start_action_ = make_unique<rtc_scheduler::StartAction<>>(this);
  this->scheduler_turn_on_automation_->add_actions({scheduler_start_action_.get()});

}

float RTCScheduler::get_setup_priority() const { return setup_priority::DATA; }

void RTCScheduler::resume_or_start_schedule_controller()
{
  //TODO запис коду запуску планувальника
    ESP_LOGD(TAG, "Startup called");
    if (this->controllerStatus_ != nullptr) {
      this->controllerStatus_->publish_state("Controller On");
      this->ctl_on_sensor_->publish_state(true);

    }
  
}
void RTCScheduler::shutdown_schedule_controller()
{
  //ЗАВДАННЯ написати код вимкнення планувальника
  ESP_LOGD(TAG, "Shutdown called");
    if (this->controllerStatus_ != nullptr) {
      this->controllerStatus_->publish_state("Controller Off");
      this->ctl_on_sensor_->publish_state(false);

    }
}
void RTCScheduler::set_main_switch_status(RTCSchedulerTextSensor *controller_Status)
{
  controllerStatus_ = controller_Status;
  if (this->controllerStatus_ != nullptr) {
    controllerStatus_->publish_state("Ініціалізація");
  }
}

void RTCScheduler::add_scheduled_item(uint8_t item_slot_number, RTCSchedulerControllerSwitch *item_sw, switch_::Switch *item_sw_id,RTCSchedulerTextSensor *item_status,RTCSchedulerTextSensor *item_next_event, RTCSchedulerItemMode_Select *item_mode_select, binary_sensor::BinarySensor *item_on_indicator)
{
   this->item_mode_select_ = item_mode_select;
   //TODO потрібно оновити дійсність слота перед викликом конфігурації
   this->scheduled_items_.push_back(this->item_mode_select_); // Додати до списку запланованих пунктів
   // Налаштуйте новий запланований елемент
   this->scheduled_items_count_++;  // збільшити кількість пунктів розкладу на 1
   this->item_mode_select_->configure_item(item_slot_number, item_sw,item_sw_id,item_status, item_next_event,  item_on_indicator);
}
RTCSchedulerItemMode_Select* RTCScheduler::get_scheduled_item_from_slot(uint8_t slot)
{
  for (int i = 0; i < this->scheduled_items_.size(); i++) {
    if (this->scheduled_items_[i]->get_slot_number() == slot){
      ESP_LOGD(TAG, "Наявні слоти");
      return  &(*this->scheduled_items_[i]);

    }
  }
  return nullptr;
}
uint16_t RTCScheduler::get_slot_starting_address(uint8_t slot)
{
  // обчислити адресу початку слота (зміщення + 4 байти для планувальника) плюс (розмір слота в байтах * слот)
  return ((this->storage_offset_ + 4) + (this->slot_size_  * slot));
}
bool RTCScheduler::check_the_cksm(uint8_t slot)
{
  uint16_t stored_checksum;
  // читати те, що зберігається
  this->storage_->read_object(get_slot_starting_address(slot)+2,stored_checksum); 
  if (this->calculate_slot_cksm(slot) == stored_checksum)
  {
    return true;
  }
  return false;
}
uint16_t RTCScheduler::calculate_slot_cksm(uint8_t slot)
{
  uint16_t data_addr = get_slot_starting_address(slot)+4;  
  uint16_t len = this->slot_size_ * 2; // 16-бітне слово на подію, тому * 2
  uint16_t crc = 0xFFFF;
  uint8_t data;
  while (len--) {
    // читати дані зі сховища
    this->storage_->read_object(data_addr, data);
    crc ^= data;
    data_addr++;
    for (uint8_t i = 0; i < 8; i++) {
      if ((crc & 0x01) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void RTCScheduler::set_slot_valid(uint8_t item_slot_number, bool valid)
{
  RTCSchedulerItemMode_Select* sched_item = get_scheduled_item_from_slot(item_slot_number);
  if(sched_item != nullptr){
    sched_item->set_item_schedule_valid(valid);
    ESP_LOGD(TAG, "setting slot %d", item_slot_number);
  }
}
void RTCScheduler::set_slot_sw_state(uint8_t item_slot_number, bool sw_state)
{
  RTCSchedulerItemMode_Select* sched_item = get_scheduled_item_from_slot(item_slot_number);
  if(sched_item != nullptr)
    sched_item->set_scheduled_item_state(sw_state);
}

bool RTCScheduler::get_storage_status()
{
  return storage_valid_;
  
}

struct Data {
    int seconds;
    std::string state;
};

/* std::vector<Data> RTCScheduler::splitCsvData(const std::string& csvData) {
    std::vector<Data> result;
    std::string token;
    std::size_t start = 0;
    std::size_t end = 0;
    while ((end = csvData.find(',', start)) != std::string::npos) {
        token = csvData.substr(start, end - start);
        Data data;
        data.seconds = std::stoi(token.substr(0, 5));
        data.state = token.substr(5);
        result.push_back(data);
        start = end + 1;
    }
    token = csvData.substr(start);
    Data data;
    data.seconds = std::stoi(token.substr(0, 5));
    data.state = token.substr(5);
    result.push_back(data);
    return result;
} */

/* int main() {
    std::string csvData = "00000OFF,61140ON,61400OFF";
    std::vector<Data> result = splitCsvData(csvData);
    for (const auto& data : result) {
        printf("Seconds: %d, State: %s\n", data.seconds, data.state.c_str());
    }
    return 0;
} */

/* std::vector<uint16_t> RTCScheduler::split_and_convert(std::string s) {
    std::string delimiter =",";
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<uint16_t> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
} */

//******************************************************************************************

RTCSchedulerControllerSwitch::RTCSchedulerControllerSwitch()
    : turn_on_trigger_(new Trigger<>()), turn_off_trigger_(new Trigger<>())
{
   
}

void RTCSchedulerControllerSwitch::setup()
{
   
  if (!this->restore_state_)
    return;

  auto restored = this->get_initial_state();
  if (!restored.has_value())
    return;

  ESP_LOGD(TAG, "  Відновлений стан %s", ONOFF(*restored));
  if (*restored) {
    this->turn_on();
  } else {
    this->turn_off();
  }
}

void RTCSchedulerControllerSwitch::dump_config()
{
    LOG_SWITCH("", "RTCSchedulerController Switch", this);
  ESP_LOGCONFIG(TAG, "  Відновити стан: %s", YESNO(this->restore_state_));
  ESP_LOGCONFIG(TAG, "  Optimistic: %s", YESNO(this->optimistic_));

}

void RTCSchedulerControllerSwitch::set_state_lambda(std::function<optional<bool>()> &&f) { this->f_ = f;}


void RTCSchedulerControllerSwitch::set_restore_state(bool restore_state)
{
    this->restore_state_ = restore_state;
}

Trigger<> *RTCSchedulerControllerSwitch::get_turn_on_trigger() const { return this->turn_on_trigger_; }

Trigger<> *RTCSchedulerControllerSwitch::get_turn_off_trigger()  const { return this->turn_off_trigger_; }

void RTCSchedulerControllerSwitch::set_optimistic(bool optimistic)
{
    this->optimistic_ = optimistic;
}

void RTCSchedulerControllerSwitch::set_assumed_state(bool assumed_state)
{
    this->assumed_state_ = assumed_state; 
}

void RTCSchedulerControllerSwitch::loop()
{
      if (!this->f_.has_value())
    return;
  auto s = (*this->f_)();
  if (!s.has_value())
    return;

  this->publish_state(*s);
}

float RTCSchedulerControllerSwitch::get_setup_priority() const { return setup_priority::HARDWARE; }

bool RTCSchedulerControllerSwitch::assumed_state()
{
    return this->assumed_state_;
}

void RTCSchedulerControllerSwitch::write_state(bool state)
{
      if (this->prev_trigger_ != nullptr) {
    this->prev_trigger_->stop_action();
  }

  if (state) {
      this->prev_trigger_ = this->turn_on_trigger_;
      this->turn_on_trigger_->trigger();
    } 
  else {
      this->prev_trigger_ = this->turn_off_trigger_;
      this->turn_off_trigger_->trigger();
  }
  if (this->optimistic_)
    this->publish_state(state);
}

// TODO додати код налаштування перемикача
//TODO додати код моніторингу комутатора
//TODO Додайте службу для прийняття розкладу рядків на слот із текстових полів
RTCSchedulerControllerSwitch *controller_sw_{nullptr};

/* void RTCSchedulerTextSensor::dump_config()
{
  LOG_TEXT_SENSOR("  ", "TextSensor ", this);
} */
} // namespace rtc_scheduler
}  // namespace esphome
