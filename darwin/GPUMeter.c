/*
htop - GPUMeter.c
(C) 2023 htop dev team
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h" // IWYU pragma: keep

#include "darwin/GPUMeter.h"

#include "CRT.h"
#include "RichString.h"
#include "darwin/DarwinMachine.h"
#include "XUtils.h"

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>


static size_t activeMeters;
static double totalUsage = 0;
static io_service_t service;

struct EngineData {
   const char* key;
   unsigned long long int timeDiff;
};

static const int GPUMeter_attributes[] = {
   GPU_ENGINE_1,
};

bool GPUMeter_active(void) {
   return activeMeters > 0;
}


static void GPUMeter_updateValues(Meter* this) {
   char* buffer = this->txtBuffer;
   size_t size = sizeof(this->txtBuffer);
   CFMutableDictionaryRef properties = NULL;
   CFDictionaryRef perfStats;
   CFNumberRef deviceUtil;
   int written, device = 0;

   if (!service) {
      /* GPU not found, early exit */
      this->values[0] = device;
      written = snprintf(buffer, size, "N/A");
      METER_BUFFER_CHECK(buffer, size, written);
      return;
   }

   if (IORegistryEntryCreateCFProperties(service, &properties, kCFAllocatorDefault, kNilOptions) != KERN_SUCCESS) {
      return;
   }

   perfStats = CFDictionaryGetValue(properties, CFSTR("PerformanceStatistics"));

   assert(perfStats && CFGetTypeID(perfStats) == CFDictionaryGetTypeID());
   
   deviceUtil = CFDictionaryGetValue(perfStats, CFSTR("Device Utilization %"));
   if (deviceUtil) CFNumberGetValue(deviceUtil, kCFNumberIntType, &device);

   CFRelease(properties);

   this->values[0] = (double) device;
   totalUsage = (double) device;
   written = snprintf(buffer, size, "%.1f", totalUsage);
   METER_BUFFER_CHECK(buffer, size, written);
   METER_BUFFER_APPEND_CHR(buffer, size, '%');
}

static void GPUMeter_display(const Object* cast ATTR_UNUSED, RichString* out) {
   char buffer[50];
   int written;

   RichString_writeAscii(out, CRT_colors[METER_TEXT], ":");
   written = xSnprintf(buffer, sizeof(buffer), "%4.1f", totalUsage);
   RichString_appendnAscii(out, CRT_colors[METER_VALUE], buffer, written);
}

static void GPUMeter_init(Meter* this ATTR_UNUSED) {
   service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOGPU"));
   activeMeters++;
}

static void GPUMeter_done(Meter* this ATTR_UNUSED) {
   IOObjectRelease(service);
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
   .maxItems = 1,
   .total = 100.0,
   .attributes = GPUMeter_attributes,
   .name = "GPU",
   .uiName = "GPU usage",
   .caption = "GPU"
};
