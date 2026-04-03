#ifndef CPLUS_EXAMPLE_H_H
#define CPLUS_EXAMPLE_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool has_value;
    u32 value;
} __cplus_maybe_u32;

typedef enum {
    kDeviceModeOff,
    kDeviceModeStandby,
    kDeviceModeActive,
    kDeviceModeFault,
    kDeviceModeSleep,
    kDeviceModeN,
} Board___DeviceMode;

const char* Board___DeviceMode___ToString(Board___DeviceMode value);

typedef enum {
    kDiagnosticStateIdle,
    kDiagnosticStateCollecting,
    kDiagnosticStateReporting,
    kDiagnosticStateFault,
    kDiagnosticStateN,
} Board___DiagnosticState;

const char* Board___DiagnosticState___ToString(Board___DiagnosticState value);

typedef struct Board___Logger {
} Board___Logger;

typedef struct Board___DeviceController {
    u32 device_id;
    __cplus_maybe_u32 maybe_last_error;
    u32 sample_rate_hz;
    u8* _scratch; // private
    DeviceMode current_mode;
    DiagnosticState current_state;
    u32 transfer_count;
    u32 read_count;
    u32 reset_count;
} Board___DeviceController;

typedef struct Board___DeviceRepository {
    u32 next_id;
    DeviceController primary;
    DeviceController secondary;
    Logger logger;
} Board___DeviceRepository;

i32 Board___SpiBus___Transfer(Board___SpiBus* self, u8* tx, u8* rx, u32 len);
i32 Board___SpiBus___Lock(Board___SpiBus* self);
i32 Board___SpiBus___Unlock(Board___SpiBus* self);
i32 Board___SpiBus___Flush(Board___SpiBus* self);
i32 Board___SensorDriver___Read(Board___SensorDriver* self, u8* buffer, u32 len);
i32 Board___SensorDriver___Calibrate(Board___SensorDriver* self);
i32 Board___SensorDriver___Reset(Board___SensorDriver* self);
i32 Board___PowerControl___Enable(Board___PowerControl* self);
i32 Board___PowerControl___Disable(Board___PowerControl* self);
i32 Board___PowerControl___IsEnabled(Board___PowerControl* self);
void Board___Logger___destroy(Board___Logger* self);
i32 Board___Logger___WriteLine(Board___Logger* self, u8* data, u32 len);
i32 Board___Logger___Flush(Board___Logger* self);
void Board___Logger___SetPrefix(Board___Logger* self, u8* prefix);
void Board___Logger___construct(Board___Logger* self);
void Board___DeviceController___destroy(Board___DeviceController* self);
i32 Board___DeviceController___Transfer(Board___DeviceController* self, u8* tx, u8* rx, u32 len);
i32 Board___DeviceController___Lock(Board___DeviceController* self);
i32 Board___DeviceController___Unlock(Board___DeviceController* self);
i32 Board___DeviceController___Flush(Board___DeviceController* self);
i32 Board___DeviceController___Read(Board___DeviceController* self, u8* buffer, u32 len);
i32 Board___DeviceController___Calibrate(Board___DeviceController* self);
i32 Board___DeviceController___Reset(Board___DeviceController* self);
i32 Board___DeviceController___Enable(Board___DeviceController* self);
i32 Board___DeviceController___Disable(Board___DeviceController* self);
i32 Board___DeviceController___IsEnabled(Board___DeviceController* self);
u32 Board___DeviceController___SampleRate(Board___DeviceController* self);
void Board___DeviceController___SetMode(Board___DeviceController* self, DeviceMode mode);
void Board___DeviceController___SetState(Board___DeviceController* self, DiagnosticState state);
i32 Board___DeviceController___IsReady(Board___DeviceController* self);
void Board___DeviceController___construct(Board___DeviceController* self, u32 device_id, u32 sample_rate_hz);
void Board___DeviceRepository___destroy(Board___DeviceRepository* self);
i32 Board___DeviceRepository___InitializeDefaults(Board___DeviceRepository* self);
i32 Board___DeviceRepository___Reset(Board___DeviceRepository* self);
i32 Board___DeviceRepository___SelectActive(Board___DeviceRepository* self, u32 id);
i32 Board___DeviceRepository___PrimeLogger(Board___DeviceRepository* self);
void Board___DeviceRepository___construct(Board___DeviceRepository* self, u32 sample_rate_hz);
u32 Board___ComputeChecksum(u8* data, u32 len);
i32 Board___Boot();
i32 Board___RunDiagnostics(u32 iterations);
i32 Board___SampleAndReport(u8* buffer, u32 len);
i32 Board___WarmUpLogger();
i32 Board___ResetHardware();
i32 Board___TickDiagnostics(u32 tick);
i32 Board___ReportState(DiagnosticState state);

#endif
