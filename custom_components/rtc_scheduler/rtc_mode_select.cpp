#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "rtc_mode_select.h"
namespace esphome {
namespace rtc_scheduler {
static const char *TAG = "rtc_scheduler_scheduled_item ";
//TODO add some more to dump config
 void RTCSchedulerItemMode_Select::dump_config()
{
  LOG_SELECT("", "Пункт розкладу RTC", this);
 
} 

void RTCSchedulerItemMode_Select::setup()
{
  std::string value;
  size_t index;
  this->pref_ = global_preferences->make_preference<size_t>(this->get_object_id_hash());
  if (!this->pref_.load(&index)) {
    value = "Manual Off";
    ESP_LOGD(TAG, "Початковий стан (не вдалося завантажити): %s", value.c_str());
  } else {
    value = this->traits.get_options().at(index);
    ESP_LOGD(TAG, "Стан після відновлення: %s", value.c_str());
  }
  this->publish_state(value);  
}
// Налаштуйте запланований елемент із його різними компонентами
void RTCSchedulerItemMode_Select::configure_item(uint8_t item_slot_number, RTCSchedulerControllerSwitch *item_sw, switch_::Switch *item_sw_id, RTCSchedulerTextSensor *item_status, RTCSchedulerTextSensor *item_next_event,binary_sensor::BinarySensor *item_on_indicator)
{
  this->item_slot_number_ = item_slot_number;
  this->item_sw_ = item_sw;
  this->item_sw_id_ = item_sw_id;
  if (this->item_sw_id_ != nullptr){
    this->item_sw_id_->set_internal(true);
  }
  if (this->item_sw_ != nullptr){
    this->item_sw_->set_internal(true);
  }
  this->item_status_ = item_status;
  if (this->item_status_ != nullptr){
    this->item_status_->publish_state("Ініціалізація");
  }
  
  this->item_next_event_ = item_next_event;
  if (this->item_next_event_ != nullptr){
    this->item_next_event_->publish_state("Жодного");
  }

  this->item_on_indicator_ = item_on_indicator;
  if (this->item_on_indicator_ != nullptr){
    this->item_on_indicator_->publish_state(false);
  }
//TODO Потрібно налаштувати стани на основі відновленого режиму, терміну дії та розкладу
}
// Повідомте запланованому елементу, що його контролер увімкнено або вимкнено
void RTCSchedulerItemMode_Select::set_controller_state(bool schedule_controller_state)
{
  this->schedule_controller_state_= schedule_controller_state;
}

bool RTCSchedulerItemMode_Select::get_controller_state()
{
  return this->schedule_controller_state_;
}
// Скажіть запланованому елементу, що він має вимкнути або ввімкнути за розкладом
void RTCSchedulerItemMode_Select::set_scheduled_item_state(bool scheduled_item_state)
{
  this->schedule_controller_item_state_ = scheduled_item_state;
   adjustItemInternalState();

}

bool RTCSchedulerItemMode_Select::get_scheduled_item_state()
{
  return this->schedule_controller_item_state_;
}
 void RTCSchedulerItemMode_Select::set_item_schedule_valid(bool schedule_valid){
    schedule_valid_ = schedule_valid;
   
 }
// Залежно від режиму елемента, керованого вибором HA та поточного контролера розкладу EG (Увімк. або Вимк.) Налаштуйте решту внутрішніх станів 
void RTCSchedulerItemMode_Select::adjustItemInternalState()
{
// First lets deal with the corner case of a invalid schedule / start-up
  if (not schedule_valid_){
    item_mode_state_ = ITEM_STATE_INIT;
    item_mode_ = ITEM_MODE_OFF;
    ESP_LOGD(TAG, "Налаштування не дійсні");
  }
  else{
    switch (this->item_mode_)
    {
    case ItemMode::ITEM_MODE_OFF:
      item_mode_state_ = ITEM_STATE_OFF;
      break;
    case ItemMode::ITEM_MODE_ON:
      item_mode_state_ = ITEM_STATE_ON;
      break;
    case ItemMode::ITEM_MODE_EARLY:
      item_mode_state_ = ITEM_STATE_EARLY;
      break;
    case ItemMode::ITEM_MODE_BOOST:
      item_mode_state_ = ITEM_STATE_BOOST;
      break;
    case ItemMode::ITEM_MODE_AUTO:
      if (schedule_controller_item_state_)
        item_mode_state_ = ITEM_STATE_AUTO_ON;
      else
        item_mode_state_ = ITEM_STATE_AUTO_OFF;
      break;
    }
  }

  this->prepareItemSensorAndSwitches();
}
// Керуючись станом внутрішнього режиму, оновіть перемикачі, індикатор і текст стану
void RTCSchedulerItemMode_Select::prepareItemSensorAndSwitches()
{
   ESP_LOGD(TAG, "приготування введення");
  switch (this->item_mode_state_)
  {
  case ItemModeState::ITEM_STATE_INIT:
    item_mode_state_str_ = "Не сконфігуровано";
    adjustItemSensorAndSwitches(false, item_mode_state_str_);
    break;
  case ItemModeState::ITEM_STATE_OFF:
    item_mode_state_str_ = "Manual Off";
    adjustItemSensorAndSwitches(false, item_mode_state_str_);
    break;
  case ItemModeState::ITEM_STATE_EARLY:
    item_mode_state_str_ = "Auto Early Off";
    adjustItemSensorAndSwitches(false, item_mode_state_str_);
    break;
  case ItemModeState::ITEM_STATE_AUTO_OFF:
    item_mode_state_str_ = "Auto Scheduled Off";
    adjustItemSensorAndSwitches(false, item_mode_state_str_);
    break;
  case ItemModeState::ITEM_STATE_AUTO_ON:
    item_mode_state_str_ = "Auto Scheduled On";
    adjustItemSensorAndSwitches(true, item_mode_state_str_);
    break;
  case ItemModeState::ITEM_STATE_ON:
    item_mode_state_str_ = "Manual On";
    adjustItemSensorAndSwitches(true, item_mode_state_str_);
    break;
  case ItemModeState::ITEM_STATE_BOOST:
    item_mode_state_str_ = "Auto Boosted On";
    adjustItemSensorAndSwitches(true, item_mode_state_str_);
    break;
  default:
          // something has gone badley wrong
          item_mode_state_str_ = "Error";
          adjustItemSensorAndSwitches(false, item_mode_state_str_);
    break;
  }

}

void RTCSchedulerItemMode_Select::adjustItemSensorAndSwitches(bool newState,std::string &newValue )
{
  adjustIndicatorState(newState);
  adjustSwitchState(newState);
  adjustStatusTextSensor(newValue);
  adjustEventTextSensor(this->next_schedule_event_str_);
}

void RTCSchedulerItemMode_Select::adjustIndicatorState(bool newValue)
{
  if (this->item_on_indicator_!= nullptr)
      this->item_on_indicator_->publish_state(newValue);
}
void RTCSchedulerItemMode_Select::adjustSwitchState(bool newValue)
{
  if (newValue)
  {
    if (this->item_sw_!= nullptr)
      this->item_sw_->turn_on();
    if (this->item_sw_id_!= nullptr)
      this->item_sw_id_->turn_on();
  }
  else{
    if (this->item_sw_!= nullptr)
      this->item_sw_->turn_off();
    if (this->item_sw_id_!= nullptr)
      this->item_sw_id_->turn_off();
  }
}
uint8_t RTCSchedulerItemMode_Select::get_slot_number()
{
  return this->item_slot_number_;
}
void RTCSchedulerItemMode_Select::adjustStatusTextSensor(std::string &newValue)
{ //TODO додати назву до рядка
  if (this->item_status_ != nullptr){
    this->item_status_->publish_state(newValue);
  }
}

void RTCSchedulerItemMode_Select::adjustEventTextSensor(std::string &newValue)
{
  if (this->item_next_event_ != nullptr){
    this->item_next_event_->publish_state(newValue);
  }
}
// Дозволити елементу розкладу відображати наступну подію, якщо налаштовано текстовий датчик
void RTCSchedulerItemMode_Select::set_next_scheduled_event(uint16_t next_schedule_event, bool next_schedule_state)
{
   
   this->next_schedule_event_ =  next_schedule_event;
   this->next_schedule_state_ = next_schedule_state;
   this->next_schedule_event_str_ = convertEventTimeToStr(next_schedule_event);
   adjustEventTextSensor(this->next_schedule_event_str_);
}
std::string RTCSchedulerItemMode_Select::convertEventTimeToStr(uint16_t next_schedule_event) const
{
  // час у хвилинах, тому потрібно конвертувати в день, години, хвилини
  uint8_t days = next_schedule_event/(24*60);
  uint16_t n = next_schedule_event%(24*60);
  uint8_t hours = n / 60;

  uint8_t mins = n%60;
  ESP_LOGD(TAG, "Days %d Hours %d Mins %d",days,hours,mins);
  std::string result;
  std::string dayStr;
    switch (days){
        case 0 :
            dayStr = "Sun";
        break;
        case 1 :
            dayStr = "Mon";
        break;
        case 2 :
            dayStr = "Tue";
        break;
        case 3 :
            dayStr = "Wed";
        break;
        case 4:
            dayStr = "Thu";
            break;
        case 5:
            dayStr = "Fri";
            break;
        case 6:
            dayStr = "Sat";
            break;
  default:
    break;
  }
  result = str_sprintf( " %02d:%02d",hours,mins);

  result = dayStr +result;
  ESP_LOGD(TAG, "Конвертування %s",result.c_str());  
  return result;
}
void RTCSchedulerItemMode_Select::control(const std::string &value)
{
  convertEventTimeToStr(8639);
  std::string value_Internal = value;
  if (not schedule_valid_){
    ESP_LOGD(TAG, "Слот %d , розклад не дійсний тому Manual Off",this->item_slot_number_);
    value_Internal = "Manual Off";
    this->state = value_Internal;
  }
  else
    ESP_LOGD(TAG, "Налаштування слота %d режим елемента %s",this->item_slot_number_,value_Internal.c_str());
  this->publish_state(value_Internal);
  // save the state
  auto options = this->traits.get_options();
  size_t index = std::find(options.begin(), options.end(), value_Internal) - options.begin();
  this->pref_.save(&index);
  ESP_LOGD(TAG, "Зберегти слот %d index %d",index,index);
  this->item_mode_ = static_cast<ItemMode>(index);
  adjustItemInternalState();
}
} // namespace rtc_scheduler
}  // namespace esphome
