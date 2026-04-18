#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/devices/at90can128.hpp"
#include "vioavr/core/devices/at90can32.hpp"
#include "vioavr/core/devices/at90can64.hpp"
#include "vioavr/core/devices/at90pwm161.hpp"
#include "vioavr/core/devices/at90pwm1.hpp"
#include "vioavr/core/devices/at90pwm216.hpp"
#include "vioavr/core/devices/at90pwm2b.hpp"
#include "vioavr/core/devices/at90pwm316.hpp"
#include "vioavr/core/devices/at90pwm3b.hpp"
#include "vioavr/core/devices/at90pwm3.hpp"
#include "vioavr/core/devices/at90pwm81.hpp"
#include "vioavr/core/devices/at90usb1286.hpp"
#include "vioavr/core/devices/at90usb1287.hpp"
#include "vioavr/core/devices/at90usb162.hpp"
#include "vioavr/core/devices/at90usb646.hpp"
#include "vioavr/core/devices/at90usb647.hpp"
#include "vioavr/core/devices/at90usb82.hpp"
#include "vioavr/core/devices/atmega1280.hpp"
#include "vioavr/core/devices/atmega1281.hpp"
#include "vioavr/core/devices/atmega1284.hpp"
#include "vioavr/core/devices/atmega1284p.hpp"
#include "vioavr/core/devices/atmega1284rfr2.hpp"
#include "vioavr/core/devices/atmega128a.hpp"
#include "vioavr/core/devices/atmega128.hpp"
#include "vioavr/core/devices/atmega128rfa1.hpp"
#include "vioavr/core/devices/atmega128rfr2.hpp"
#include "vioavr/core/devices/atmega1608.hpp"
#include "vioavr/core/devices/atmega1609.hpp"
#include "vioavr/core/devices/atmega162.hpp"
#include "vioavr/core/devices/atmega164a.hpp"
#include "vioavr/core/devices/atmega164pa.hpp"
#include "vioavr/core/devices/atmega164p.hpp"
#include "vioavr/core/devices/atmega165a.hpp"
#include "vioavr/core/devices/atmega165pa.hpp"
#include "vioavr/core/devices/atmega165p.hpp"
#include "vioavr/core/devices/atmega168a.hpp"
#include "vioavr/core/devices/atmega168.hpp"
#include "vioavr/core/devices/atmega168pa.hpp"
#include "vioavr/core/devices/atmega168pb.hpp"
#include "vioavr/core/devices/atmega168p.hpp"
#include "vioavr/core/devices/atmega169a.hpp"
#include "vioavr/core/devices/atmega169pa.hpp"
#include "vioavr/core/devices/atmega169p.hpp"
#include "vioavr/core/devices/atmega16a.hpp"
#include "vioavr/core/devices/atmega16.hpp"
#include "vioavr/core/devices/atmega16hva.hpp"
#include "vioavr/core/devices/atmega16hvb.hpp"
#include "vioavr/core/devices/atmega16hvbrevb.hpp"
#include "vioavr/core/devices/atmega16m1.hpp"
#include "vioavr/core/devices/atmega16u2.hpp"
#include "vioavr/core/devices/atmega16u4.hpp"
#include "vioavr/core/devices/atmega2560.hpp"
#include "vioavr/core/devices/atmega2561.hpp"
#include "vioavr/core/devices/atmega2564rfr2.hpp"
#include "vioavr/core/devices/atmega256rfr2.hpp"
#include "vioavr/core/devices/atmega3208.hpp"
#include "vioavr/core/devices/atmega3209.hpp"
#include "vioavr/core/devices/atmega324a.hpp"
#include "vioavr/core/devices/atmega324pa.hpp"
#include "vioavr/core/devices/atmega324pb.hpp"
#include "vioavr/core/devices/atmega324p.hpp"
#include "vioavr/core/devices/atmega3250a.hpp"
#include "vioavr/core/devices/atmega3250.hpp"
#include "vioavr/core/devices/atmega3250pa.hpp"
#include "vioavr/core/devices/atmega3250p.hpp"
#include "vioavr/core/devices/atmega325a.hpp"
#include "vioavr/core/devices/atmega325.hpp"
#include "vioavr/core/devices/atmega325pa.hpp"
#include "vioavr/core/devices/atmega325p.hpp"
#include "vioavr/core/devices/atmega328.hpp"
#include "vioavr/core/devices/atmega328pb.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/devices/atmega3290a.hpp"
#include "vioavr/core/devices/atmega3290.hpp"
#include "vioavr/core/devices/atmega3290pa.hpp"
#include "vioavr/core/devices/atmega3290p.hpp"
#include "vioavr/core/devices/atmega329a.hpp"
#include "vioavr/core/devices/atmega329.hpp"
#include "vioavr/core/devices/atmega329pa.hpp"
#include "vioavr/core/devices/atmega329p.hpp"
#include "vioavr/core/devices/atmega32a.hpp"
#include "vioavr/core/devices/atmega32c1.hpp"
#include "vioavr/core/devices/atmega32.hpp"
#include "vioavr/core/devices/atmega32hvb.hpp"
#include "vioavr/core/devices/atmega32hvbrevb.hpp"
#include "vioavr/core/devices/atmega32m1.hpp"
#include "vioavr/core/devices/atmega32u2.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"
#include "vioavr/core/devices/atmega406.hpp"
#include "vioavr/core/devices/atmega4808.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/devices/atmega48a.hpp"
#include "vioavr/core/devices/atmega48.hpp"
#include "vioavr/core/devices/atmega48pa.hpp"
#include "vioavr/core/devices/atmega48pb.hpp"
#include "vioavr/core/devices/atmega48p.hpp"
#include "vioavr/core/devices/atmega640.hpp"
#include "vioavr/core/devices/atmega644a.hpp"
#include "vioavr/core/devices/atmega644.hpp"
#include "vioavr/core/devices/atmega644pa.hpp"
#include "vioavr/core/devices/atmega644p.hpp"
#include "vioavr/core/devices/atmega644rfr2.hpp"
#include "vioavr/core/devices/atmega6450a.hpp"
#include "vioavr/core/devices/atmega6450.hpp"
#include "vioavr/core/devices/atmega6450p.hpp"
#include "vioavr/core/devices/atmega645a.hpp"
#include "vioavr/core/devices/atmega645.hpp"
#include "vioavr/core/devices/atmega645p.hpp"
#include "vioavr/core/devices/atmega6490a.hpp"
#include "vioavr/core/devices/atmega6490.hpp"
#include "vioavr/core/devices/atmega6490p.hpp"
#include "vioavr/core/devices/atmega649a.hpp"
#include "vioavr/core/devices/atmega649.hpp"
#include "vioavr/core/devices/atmega649p.hpp"
#include "vioavr/core/devices/atmega64a.hpp"
#include "vioavr/core/devices/atmega64c1.hpp"
#include "vioavr/core/devices/atmega64.hpp"
#include "vioavr/core/devices/atmega64hve2.hpp"
#include "vioavr/core/devices/atmega64m1.hpp"
#include "vioavr/core/devices/atmega64rfr2.hpp"
#include "vioavr/core/devices/atmega808.hpp"
#include "vioavr/core/devices/atmega809.hpp"
#include "vioavr/core/devices/atmega8515.hpp"
#include "vioavr/core/devices/atmega8535.hpp"
#include "vioavr/core/devices/atmega88a.hpp"
#include "vioavr/core/devices/atmega88.hpp"
#include "vioavr/core/devices/atmega88pa.hpp"
#include "vioavr/core/devices/atmega88pb.hpp"
#include "vioavr/core/devices/atmega88p.hpp"
#include "vioavr/core/devices/atmega8a.hpp"
#include "vioavr/core/devices/atmega8.hpp"
#include "vioavr/core/devices/atmega8hva.hpp"
#include "vioavr/core/devices/atmega8u2.hpp"
#include "vioavr/core/devices/atmegas128.hpp"
#include "vioavr/core/devices/atmegas64m1.hpp"

namespace vioavr::core {

const DeviceDescriptor* DeviceCatalog::find(std::string_view name) noexcept {
    // Note: This relies on DeviceDescriptor being updated in headers
    static const DeviceDescriptor* catalog[] = {
        &devices::at90can128,
        &devices::at90can32,
        &devices::at90can64,
        &devices::at90pwm161,
        &devices::at90pwm1,
        &devices::at90pwm216,
        &devices::at90pwm2b,
        &devices::at90pwm316,
        &devices::at90pwm3b,
        &devices::at90pwm3,
        &devices::at90pwm81,
        &devices::at90usb1286,
        &devices::at90usb1287,
        &devices::at90usb162,
        &devices::at90usb646,
        &devices::at90usb647,
        &devices::at90usb82,
        &devices::atmega1280,
        &devices::atmega1281,
        &devices::atmega1284,
        &devices::atmega1284p,
        &devices::atmega1284rfr2,
        &devices::atmega128a,
        &devices::atmega128,
        &devices::atmega128rfa1,
        &devices::atmega128rfr2,
        &devices::atmega1608,
        &devices::atmega1609,
        &devices::atmega162,
        &devices::atmega164a,
        &devices::atmega164pa,
        &devices::atmega164p,
        &devices::atmega165a,
        &devices::atmega165pa,
        &devices::atmega165p,
        &devices::atmega168a,
        &devices::atmega168,
        &devices::atmega168pa,
        &devices::atmega168pb,
        &devices::atmega168p,
        &devices::atmega169a,
        &devices::atmega169pa,
        &devices::atmega169p,
        &devices::atmega16a,
        &devices::atmega16,
        &devices::atmega16hva,
        &devices::atmega16hvb,
        &devices::atmega16hvbrevb,
        &devices::atmega16m1,
        &devices::atmega16u2,
        &devices::atmega16u4,
        &devices::atmega2560,
        &devices::atmega2561,
        &devices::atmega2564rfr2,
        &devices::atmega256rfr2,
        &devices::atmega3208,
        &devices::atmega3209,
        &devices::atmega324a,
        &devices::atmega324pa,
        &devices::atmega324pb,
        &devices::atmega324p,
        &devices::atmega3250a,
        &devices::atmega3250,
        &devices::atmega3250pa,
        &devices::atmega3250p,
        &devices::atmega325a,
        &devices::atmega325,
        &devices::atmega325pa,
        &devices::atmega325p,
        &devices::atmega328,
        &devices::atmega328pb,
        &devices::atmega328p,
        &devices::atmega3290a,
        &devices::atmega3290,
        &devices::atmega3290pa,
        &devices::atmega3290p,
        &devices::atmega329a,
        &devices::atmega329,
        &devices::atmega329pa,
        &devices::atmega329p,
        &devices::atmega32a,
        &devices::atmega32c1,
        &devices::atmega32,
        &devices::atmega32hvb,
        &devices::atmega32hvbrevb,
        &devices::atmega32m1,
        &devices::atmega32u2,
        &devices::atmega32u4,
        &devices::atmega406,
        &devices::atmega4808,
        &devices::atmega4809,
        &devices::atmega48a,
        &devices::atmega48,
        &devices::atmega48pa,
        &devices::atmega48pb,
        &devices::atmega48p,
        &devices::atmega640,
        &devices::atmega644a,
        &devices::atmega644,
        &devices::atmega644pa,
        &devices::atmega644p,
        &devices::atmega644rfr2,
        &devices::atmega6450a,
        &devices::atmega6450,
        &devices::atmega6450p,
        &devices::atmega645a,
        &devices::atmega645,
        &devices::atmega645p,
        &devices::atmega6490a,
        &devices::atmega6490,
        &devices::atmega6490p,
        &devices::atmega649a,
        &devices::atmega649,
        &devices::atmega649p,
        &devices::atmega64a,
        &devices::atmega64c1,
        &devices::atmega64,
        &devices::atmega64hve2,
        &devices::atmega64m1,
        &devices::atmega64rfr2,
        &devices::atmega808,
        &devices::atmega809,
        &devices::atmega8515,
        &devices::atmega8535,
        &devices::atmega88a,
        &devices::atmega88,
        &devices::atmega88pa,
        &devices::atmega88pb,
        &devices::atmega88p,
        &devices::atmega8a,
        &devices::atmega8,
        &devices::atmega8hva,
        &devices::atmega8u2,
        &devices::atmegas128,
        &devices::atmegas64m1,
    };

    for (const auto* device : catalog) {
        if (device->name == name) {
            Logger::debug("DeviceCatalog::find: found " + std::string(name) + " TCA count: " + std::to_string(device->tca_count));
            return device;
        }
    }
    return nullptr;
}

std::vector<std::string_view> DeviceCatalog::list_devices() noexcept {
    return {
        devices::at90can128.name,
        devices::at90can32.name,
        devices::at90can64.name,
        devices::at90pwm161.name,
        devices::at90pwm1.name,
        devices::at90pwm216.name,
        devices::at90pwm2b.name,
        devices::at90pwm316.name,
        devices::at90pwm3b.name,
        devices::at90pwm3.name,
        devices::at90pwm81.name,
        devices::at90usb1286.name,
        devices::at90usb1287.name,
        devices::at90usb162.name,
        devices::at90usb646.name,
        devices::at90usb647.name,
        devices::at90usb82.name,
        devices::atmega1280.name,
        devices::atmega1281.name,
        devices::atmega1284.name,
        devices::atmega1284p.name,
        devices::atmega1284rfr2.name,
        devices::atmega128a.name,
        devices::atmega128.name,
        devices::atmega128rfa1.name,
        devices::atmega128rfr2.name,
        devices::atmega1608.name,
        devices::atmega1609.name,
        devices::atmega162.name,
        devices::atmega164a.name,
        devices::atmega164pa.name,
        devices::atmega164p.name,
        devices::atmega165a.name,
        devices::atmega165pa.name,
        devices::atmega165p.name,
        devices::atmega168a.name,
        devices::atmega168.name,
        devices::atmega168pa.name,
        devices::atmega168pb.name,
        devices::atmega168p.name,
        devices::atmega169a.name,
        devices::atmega169pa.name,
        devices::atmega169p.name,
        devices::atmega16a.name,
        devices::atmega16.name,
        devices::atmega16hva.name,
        devices::atmega16hvb.name,
        devices::atmega16hvbrevb.name,
        devices::atmega16m1.name,
        devices::atmega16u2.name,
        devices::atmega16u4.name,
        devices::atmega2560.name,
        devices::atmega2561.name,
        devices::atmega2564rfr2.name,
        devices::atmega256rfr2.name,
        devices::atmega3208.name,
        devices::atmega3209.name,
        devices::atmega324a.name,
        devices::atmega324pa.name,
        devices::atmega324pb.name,
        devices::atmega324p.name,
        devices::atmega3250a.name,
        devices::atmega3250.name,
        devices::atmega3250pa.name,
        devices::atmega3250p.name,
        devices::atmega325a.name,
        devices::atmega325.name,
        devices::atmega325pa.name,
        devices::atmega325p.name,
        devices::atmega328.name,
        devices::atmega328pb.name,
        devices::atmega328p.name,
        devices::atmega3290a.name,
        devices::atmega3290.name,
        devices::atmega3290pa.name,
        devices::atmega3290p.name,
        devices::atmega329a.name,
        devices::atmega329.name,
        devices::atmega329pa.name,
        devices::atmega329p.name,
        devices::atmega32a.name,
        devices::atmega32c1.name,
        devices::atmega32.name,
        devices::atmega32hvb.name,
        devices::atmega32hvbrevb.name,
        devices::atmega32m1.name,
        devices::atmega32u2.name,
        devices::atmega32u4.name,
        devices::atmega406.name,
        devices::atmega4808.name,
        devices::atmega4809.name,
        devices::atmega48a.name,
        devices::atmega48.name,
        devices::atmega48pa.name,
        devices::atmega48pb.name,
        devices::atmega48p.name,
        devices::atmega640.name,
        devices::atmega644a.name,
        devices::atmega644.name,
        devices::atmega644pa.name,
        devices::atmega644p.name,
        devices::atmega644rfr2.name,
        devices::atmega6450a.name,
        devices::atmega6450.name,
        devices::atmega6450p.name,
        devices::atmega645a.name,
        devices::atmega645.name,
        devices::atmega645p.name,
        devices::atmega6490a.name,
        devices::atmega6490.name,
        devices::atmega6490p.name,
        devices::atmega649a.name,
        devices::atmega649.name,
        devices::atmega649p.name,
        devices::atmega64a.name,
        devices::atmega64c1.name,
        devices::atmega64.name,
        devices::atmega64hve2.name,
        devices::atmega64m1.name,
        devices::atmega64rfr2.name,
        devices::atmega808.name,
        devices::atmega809.name,
        devices::atmega8515.name,
        devices::atmega8535.name,
        devices::atmega88a.name,
        devices::atmega88.name,
        devices::atmega88pa.name,
        devices::atmega88pb.name,
        devices::atmega88p.name,
        devices::atmega8a.name,
        devices::atmega8.name,
        devices::atmega8hva.name,
        devices::atmega8u2.name,
        devices::atmegas128.name,
        devices::atmegas64m1.name,
    };
}

} // namespace vioavr::core
