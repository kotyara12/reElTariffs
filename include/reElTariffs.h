/* 
   EN: Module for electricity metering and tariff switching
   RU: Модуль для учета электроэнергии и переключения тарифов
   --------------------------
   (с) 2021 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
   --------------------------
   Страница проекта: https://github.com/kotyara12/reElTariffs
*/

#ifndef __RE_ELTARIFFS_H__
#define __RE_ELTARIFFS_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "project_config.h"
#include "def_consts.h"
#include "rTypes.h"
#include "rLog.h"
#include "rStrings.h"
#include "reParams.h"
#include "reEvents.h"

#if defined(CONFIG_ELTARIFFS_ENABLED) && CONFIG_ELTARIFFS_ENABLED

#ifdef __cplusplus
extern "C" {
#endif

void elTariffsInit();
void elTariffsSet(uint8_t index, weekdays_t days, timespan_t timespan, float price);

void elTariffsRegisterParameters();

int8_t   elTariffsGetTariff();
float    elTariffsGetTariffPrice();
uint8_t  elTariffsGetReportDay();
uint8_t* elTariffsGetReportDayAddress();

void elTariffsCheckTime();

bool elTariffsEventHandlerRegister();
void elTariffsEventHandlerUnregister();

void elTariffsRegister();

#ifdef __cplusplus
}
#endif

#endif // CONFIG_ELTARIFFS_ENABLED

#endif // __RE_ELTARIFFS_H__