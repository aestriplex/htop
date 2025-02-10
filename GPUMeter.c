/*
htop - GPUMeter.c
(C) 2023 htop dev team
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "GPUMeter.h"

#include "CRT.h"
#include "RichString.h"


static size_t activeMeters;
static const int GPUMeter_attributes[] = {
   GPU_ENGINE_1,
   GPU_ENGINE_2,
   GPU_ENGINE_3,
   GPU_ENGINE_4,
   GPU_RESIDUE,
};

bool GPUMeter_active(void) {
   return activeMeters > 0;
}

static double totalUsage;

static void GPUMeter_updateValues(Meter* this) {
   char* buffer = this->txtBuffer;
   size_t size = sizeof(this->txtBuffer);
   int written;

	totalUsage = Machine_getGpuUsage(this->host, this->values);
   
   if (totalUsage < 0) {
      this->values[0] = 0;
      written = snprintf(buffer, size, "N/A");
      METER_BUFFER_CHECK(buffer, size, written);
   } else {
      written = snprintf(buffer, size, "%.1f", totalUsage);
      METER_BUFFER_CHECK(buffer, size, written);

      METER_BUFFER_APPEND_CHR(buffer, size, '%');
   }
}

static void GPUMeter_display(const Object* cast, RichString* out) {
   Machine_GPUMeterDisplay(cast, out, totalUsage);
}

static void GPUMeter_init(Meter* this ATTR_UNUSED) {
   activeMeters++;
}

static void GPUMeter_done(Meter* this ATTR_UNUSED) {
   assert(activeMeters > 0);
   activeMeters--;
}

const MeterClass GPUMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete,
      .display = GPUMeter_display,
   },
   .init = GPUMeter_init,
   .done = GPUMeter_done,
   .updateValues = GPUMeter_updateValues,
   .defaultMode = BAR_METERMODE,
   .supportedModes = METERMODE_DEFAULT_SUPPORTED,
   .maxItems = ARRAYSIZE(GPUMeter_attributes),
   .total = 100.0,
   .attributes = GPUMeter_attributes,
   .name = "GPU",
   .uiName = "GPU usage",
   .caption = "GPU"
};
