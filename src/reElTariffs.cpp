#include "reElTariffs.h"

static const char* logTAG = "ELTR";

#ifndef CONFIG_ELTARIFFS_COUNT
#define CONFIG_ELTARIFFS_COUNT 1
#endif // CONFIG_ELTARIFFS_COUNT

typedef struct {
  weekdays_t days = WEEK_EMPTY;
  timespan_t timespan = 00000000;
  float price = 0.0;
  char* prm_group_key = nullptr;
  char* prm_group_name = nullptr;
  char* prm_group_friendly = nullptr;
  paramsGroupHandle_t prm_group = nullptr;
} el_tariff_data_t;

typedef struct {
  int8_t tariff = -1;
  el_tariff_data_t tariffs[CONFIG_ELTARIFFS_COUNT];
  uint8_t report_day = 0;
} el_tariffs_t;

static el_tariffs_t _tariffs;

void elTariffsInit()
{
  _tariffs.tariff = -1;
  _tariffs.tariffs[0].days = WEEK_ANY;
  _tariffs.tariffs[0].timespan = 00002400;
  _tariffs.tariffs[0].price = 0.0;
  for (uint8_t i = 1; i < CONFIG_ELTARIFFS_COUNT; i++) {
    _tariffs.tariffs[i].days = WEEK_EMPTY;
    _tariffs.tariffs[i].timespan = 00000000;
    _tariffs.tariffs[i].price = 0.0;
  };
  _tariffs.report_day = 0;
}

void elTariffsSet(uint8_t index, weekdays_t days, timespan_t timespan, float price)
{
  if (index < CONFIG_ELTARIFFS_COUNT) {
    _tariffs.tariffs[index].days = days;
    _tariffs.tariffs[index].timespan = timespan;
    _tariffs.tariffs[index].price = price;
    rlog_d(logTAG, "The parameters of the electricity metering tariff are set: index = %d, days = %d, timespan = %d, price=%.2f",
      index, _tariffs.tariffs[index].days, _tariffs.tariffs[index].timespan, _tariffs.tariffs[index].price);
  };
}

void elTariffsRegisterParameters()
{
  paramsGroupHandle_t pgElTariffs = paramsRegisterGroup(nullptr, 
    CONFIG_ELTARIFFS_PARAMS_GROUP_KEY, CONFIG_ELTARIFFS_PARAMS_GROUP_TOPIC, CONFIG_ELTARIFFS_PARAMS_GROUP_FRIENDLY);

  for (uint8_t i = 0; i < CONFIG_ELTARIFFS_COUNT; i++) {
    _tariffs.tariffs[i].prm_group_key = malloc_stringf(CONFIG_ELTARIFFS_PARAMS_TARIFFS_KEY, i+1);
    _tariffs.tariffs[i].prm_group_name = malloc_stringf(CONFIG_ELTARIFFS_PARAMS_TARIFFS_TOPIC, i+1);
    _tariffs.tariffs[i].prm_group_friendly = malloc_stringf(CONFIG_ELTARIFFS_PARAMS_TARIFFS_FRIENDLY, i+1);
    _tariffs.tariffs[i].prm_group = paramsRegisterGroup(pgElTariffs, 
      (const char*)_tariffs.tariffs[i].prm_group_key, 
      (const char*)_tariffs.tariffs[i].prm_group_name, 
      (const char*)_tariffs.tariffs[i].prm_group_friendly);

    paramsRegisterValue(CONFIG_ELTARIFFS_PARAMS_TYPE, OPT_TYPE_U8, nullptr, _tariffs.tariffs[i].prm_group, 
      CONFIG_ELTARIFFS_PARAM_TARIF_DAYS_TOPIC, CONFIG_ELTARIFFS_PARAM_TARIF_DAYS_FRIENDLY, CONFIG_MQTT_PARAMS_QOS, (void*)&_tariffs.tariffs[i].days);
    paramsRegisterValue(CONFIG_ELTARIFFS_PARAMS_TYPE, OPT_TYPE_U32, nullptr, _tariffs.tariffs[i].prm_group, 
      CONFIG_ELTARIFFS_PARAM_TARIF_TIMESPAN_TOPIC, CONFIG_ELTARIFFS_PARAM_TARIF_TIMESPAN_FRIENDLY, CONFIG_MQTT_PARAMS_QOS, (void*)&_tariffs.tariffs[i].timespan);
    paramsRegisterValue(CONFIG_ELTARIFFS_PARAMS_TYPE, OPT_TYPE_FLOAT, nullptr, _tariffs.tariffs[i].prm_group, 
      CONFIG_ELTARIFFS_PARAM_TARIF_PRICE_TOPIC, CONFIG_ELTARIFFS_PARAM_TARIF_PRICE_FRIENDLY, CONFIG_MQTT_PARAMS_QOS, (void*)&_tariffs.tariffs[i].price);
  };

  paramsSetLimitsU8(
    paramsRegisterValue(CONFIG_ELTARIFFS_PARAMS_TYPE, OPT_TYPE_U8, nullptr, pgElTariffs, 
      CONFIG_ELTARIFFS_PARAM_REPORT_DAY_TOPIC, CONFIG_ELTARIFFS_PARAM_REPORT_DAY_FRIENDLY, CONFIG_MQTT_PARAMS_QOS, (void*)&_tariffs.report_day),
    1, 31);
}

int8_t elTariffsGetTariff()
{
  return _tariffs.tariff;
}

float elTariffsGetTariffPrice()
{
  if ((_tariffs.tariff > -1) && (_tariffs.tariff < CONFIG_ELTARIFFS_COUNT)) {
    return _tariffs.tariffs[_tariffs.tariff].price;
  };
  return 0.0;
}

uint8_t elTariffsGetReportDay()
{
  return _tariffs.report_day;
}

uint8_t* elTariffsGetReportDayAddress()
{
  return &_tariffs.report_day;
}

void elTariffsCheckTime()
{
  time_t nowT = time(nullptr);
  struct tm nowS;
  localtime_r(&nowT, &nowS);

  int8_t newTarif = -1;
  for (uint8_t i = 0; i < CONFIG_ELTARIFFS_COUNT; i++) {
    if (checkWeekday(&nowS, _tariffs.tariffs[i].days) && checkTimespan(&nowS, _tariffs.tariffs[i].timespan)) {
      newTarif = i;
      break;
    };
  };
  if ((newTarif > -1) && (newTarif < CONFIG_ELTARIFFS_COUNT) && (newTarif != _tariffs.tariff)) {
    _tariffs.tariff = newTarif;
    eventLoopPost(RE_TIME_EVENTS, RE_TIME_ELTARIFF_CHANGED, &newTarif, sizeof(newTarif), portMAX_DELAY);
    rlog_i(logTAG, "New electricity tariff was applied: %d", newTarif);
  };
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Event handlers ---------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static void elTariffsTimeEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_id == RE_TIME_EVERY_MINUTE) {
    elTariffsCheckTime();
  };
}

bool elTariffsEventHandlerRegister()
{
  return eventHandlerRegister(RE_TIME_EVENTS, RE_TIME_EVERY_MINUTE, &elTariffsTimeEventHandler, nullptr);
}

void elTariffsEventHandlerUnregister()
{
  eventHandlerUnregister(RE_TIME_EVENTS, RE_TIME_EVERY_MINUTE, &elTariffsTimeEventHandler);
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Initialization ---------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

void elTariffsRegister()
{
  #if defined(CONFIG_ELTARIFFS_TARIF1_TIMESPAN)
    elTariffsSet(0, CONFIG_ELTARIFFS_TARIF1_DAYS, CONFIG_ELTARIFFS_TARIF1_TIMESPAN, CONFIG_ELTARIFFS_TARIF1_PRICE);
  #endif // CONFIG_ELTARIFFS_TARIF1
  #if defined(CONFIG_ELTARIFFS_TARIF2_TIMESPAN)
    elTariffsSet(1, CONFIG_ELTARIFFS_TARIF2_DAYS, CONFIG_ELTARIFFS_TARIF2_TIMESPAN, CONFIG_ELTARIFFS_TARIF2_PRICE);
  #endif // CONFIG_ELTARIFFS_TARIF2
  #if defined(CONFIG_ELTARIFFS_TARIF3_TIMESPAN)
    elTariffsSet(2, CONFIG_ELTARIFFS_TARIF3_DAYS, CONFIG_ELTARIFFS_TARIF3_TIMESPAN, CONFIG_ELTARIFFS_TARIF3_PRICE);
  #endif // CONFIG_ELTARIFFS_TARIF3
  elTariffsRegisterParameters();
  elTariffsEventHandlerRegister();
}