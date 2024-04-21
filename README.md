# RTC_Scheduler
> :warning:  **Note This is a work in progress and is not fully working yet.**

This is a realtime weekly scheduler component for ESPHome that allows the scheduling of switches independantly of Home Assistant. This ensures that scheduled switched equipment will always opperate regardless of the availability of Home Assistant and the network. The schedule is supplied by Home Assistant via services this is either text string or complex data structure.
The scheduler has the following structure
-Scheduler Hub 
  -- Schedule Controller (1.. n)
      --- Switches (1..n)
Each scheduled switch can have multiple timing events that define on and off times upto ``` max_events_per_switch ```.
Day 0 - 7, Hour 0 - 23, Minutes 0 - 59, State (On / Off)
Currently there are 2 ways to get the schedule from Home Assistant
* Text string supplied by service call. This is limited 256 characters so given it takes 9 characters to describe an event this equates to 28 events in a week.
* A service call that takes a complex structure that can be used with the supplied python script and a HACS scheduler. See https://github.com/nielsfaber/scheduler-component
* In the future a version a  built in home assistant scheduler helper will be produced and will call the data service. Fingers crossed 

## Addtional Hardware required 
To acheive this the software requires a couple of hardware components (DS3231 & 24LCxx E2). The easiest way to add these devices is via a ZS-042 (https://www.google.com/search?q=zs-04). These devices are I2C so the device configuration needs setup I2C with the appropiate pin configurations for your device.

> :warning:  **Note there is a modification needed to the module. Remove the 1N4148 and/or the 200R resistor and everything is fine.**

## Example configuration
```
# See further details below
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true    
  time:
  - platform: ds1307
    id: rtc_time
    # repeated synchronization is not necessary unless the external RTC
    # is much more accurate than the internal clock
    update_interval: never
ext_eeprom_component:
  id: ext_eeprom_component_1
  address: 0x57 
  pollWriteComplete: true
  writeTime: 5
  pageSize: 32
  memorySize: 4096
rtc_scheduler:
  - id: scheduler_hub
    schedulers:
      - id: scheduler_1
        storage: ext_eeprom_component_1
        storage_offset: 1000
        storage_size: 32768
        max_events_per_switch: 56
        schedule_controller_status_id: "Heater Sheduler Status"
        scheduler_mode: "Heater Controller Mode"
        scheduler_ind: "Heater Indicator"
        main_switch: 
          name: "Heater Scheduler"
          on_turn_on:
            then:
              - logger.log: "Heater Turned On by action!"
        switches:
          - scheduled_mode: "Element 1 Mode"
            scheduled_switch: 
              name: "Element 1"
              on_turn_on:
                then:
                - logger.log: "element 1 Turned On by action!"
            scheduled_switch_id: relay3
            scheduler_slot: 1
            scheduled_status: "Element 1 Status"
            scheduled_next_event_text: "Element 1 Next Event"
            scheduled_indicator: "Element 1 Indicator"
          - scheduled_switch: "Element 2"
            scheduled_switch_id: relay4 
            scheduler_slot: 2  
            scheduled_status: "Element 2 Status"
            scheduled_mode: "Element 2 Mode"
            scheduled_indicator: "Element 2 Indicator"    
```
## Configuration variables:
**id** *(Required)* Вручну вкажіть ідентифікатор, який використовується для створення коду.

**storage** *(**Required**, string)* : це ідентифікатор компонента сховища.

**storage_offset** *(**Required**, int)* : Це зсув у байтах від початку пристрою E2, корисний, якщо у вас налаштовано кілька планувальників.

**storage_size** *(**Required**, int)* : це розмір пристрою E2 у байтах.

**max_events_per_switch** *(**Required**, int)* : це кількість подій, які може містити кожен слот.

**main_switch** *(**Required**, string)*:  ім’я головного перемикача контролера планувальника, яке відображатиметься у інтерфейсі. Якщо цей перемикач увімкнено, він дозволяє планувальнику працювати за розкладом або за допомогою елементів ручного керування (вибір) із Home Assistant.

**switches** *(**Required**, list)* : список запланованих елементів, якими може керувати планувальник. Кожен предмет складається з:

- **scheduled_switch** *(Optiopnal, string)*: ім’я віртуального комутатора, яким може керувати планувальник. Для цього можна використовувати стандартні функції перемикача ESPHome on_turn_on і on_turn_off.
- **scheduled_id** *(Optional, ID)*: це switch <config-switch>компонент :ref:. Зазвичай це :doc: GPIO switch <switch/gpio>підключений для керування реле чи іншим комутаційним пристроєм. Не рекомендується виставляти цей перемикач на передній кінець.
- **scheduled_slot** *(**Required**, int)*: це номер слота для запланованого перемикання. Це використовується для зберігання подій для елемента розкладу в пристрої E2. Діапазон від 1 до 255. Слід зазначити, що вони мають бути унікальними для контролера розкладу
- **scheduled_mode** *(**Required, string**)*: ім’я вибору, яке додається до інтерфейсу домашнього помічника та використовується для керування запланованим елементом. Варіанти:-

* ```Manual Off```, Пункт розкладу постійно вимкнено. Поки не буде обрано інший режим.

* ```Early Off```,  Елемент розкладу ввімкнено Auto, і його можна вимкнути до запланованого часу. Після настання запланованого часу режим повернеться Auto

* ```Auto```,  Запланований елемент вмикається та вимикається відповідно до наданого розкладу.

* ```Manual On```,  Елемент розкладу постійно ввімкнено. Поки не буде обрано інший режим.



* ```Boost On```,  Елемент розкладу доступний Autoі може бути продовжений після закінчення запланованого часу вимкнення. Після настання наступного запланованого часу режим повернеться до Auto. Посилення можна скасувати в будь-який час, вибравши Autoще раз. У цей момент запланований елемент прийме правильний стан для цього періоду часу з наданого розкладу.

- **scheduled_status** *(Optiopnal, string)*: назва текстового датчика, який відображає поточний стан запланованого елемента в Home Assistant. Корисно для налагодження розкладів.
- **scheduled_indicator** *(Optiopnal, string)*:  ім’я бінарного датчика, який відображає поточний стан запланованого елемента в Home Assistant.

## Services presented to Home Assistant
Планувальник надає набір послуг Home Assistant. Кожна служба має префікс «esphome». і device_name і scheduler_name з конфігурації в yaml. Як приклад "ESPHome: schedule_test_pump_scheduler_send_schedule"

Послуги такі:-
### send_schedule "надсилати розклад"
Надішліть розклад для слота.
#### parameters
- ```schedule_slot_id int``` - Слот для оновлення.
- ```std::vector<int> days``` - Масив ints, що представляє дні (0-6)
- ```std::vector<int> hours``` - Масив ints, що представляє години (0-23)
- ```std::vector<int> minutes``` - Масив ints, що представляє хвилини (0-59)
- ```std::vector<std::string> action``` - Наразі "ON" або "OfFF" буде розширено для керування станом. 

### send_schedule_text "надсилати_текст_розкладу"
Надішліть розклад для слота.
#### parameters
- ```scheduler_id string``` - ID контролера розкладу 
- ```schedule_slot_id int``` - Слот для оновлення.
- ```std::string ``` - Має форму ідентифікатора слота, події, події, .....
- Подія кодується як цей стан DHHMMState, де стан ON або OFF
- Приклад "00000OFF,61140ON,61400OFF" - увімкнути в суботу об 11:40, вимкнути в суботу о 14:00
> :info:  **Примітка Кожен розклад має починатися зі стану неділі 00:00.**
### erase_schedule
Стерти розклад одного слота
#### parameters
- ```schedule_slot_id int``` - The slot to be deleted
### erase_all_schedules 
No parameters - Erase all slots that belong that scheduler

## Storage Allocation
Each schedule controller defines the starting address of its event storage using the ```storage_offset``` configuration entry E.G. Offset plus location. 

### Schedule Controller Usage
| Bytes   | Description                | Notes                         |
| ---------- | -------------------------- | ----------------------------- |
| 0:3       | Controller Storage Setup | Used to initialise E2 for schedule storage. Once configured these are set to known value |
| 4:--      | Slots with events | See the slot definition below |

The each scheduled item has it own slot with header information as per below. 
### Slot Usage
| Bytes   | Description                | Notes                         |
| ---------- | -------------------------- | ----------------------------- |
| 0:1       | Slot Valid | Used to initialise the schedule storage. Once configured these are set to known value. If the slot is erased it is setback to non valid value |
| 2:3      | Slots CRC Checksum | This is calculated on writing the slot data and is checked  before a new schedule is written to the slot (Protects Flash Life) |
| 4:--      | Event Storage | See definition of a event below |

### Event Format
Each event is consists 16bit word (2 bytes)
| Bits   | Description                | Notes                         |
| ---------- | -------------------------- | ----------------------------- |
| 0:13       | Event Time | In seconds, the maximum value is  10080 *EG 7 Days x 24 Hours x 60 Mins*|
| 14     |  Event state | 1 for on, 0 for off   |
| 15     | Valid Event | This has to be set to true for the event to be valid. It should be noted that on a new schedule being recieved un-used events are set to invalid EG upto value supplied in *max_events_per_switch*  |
