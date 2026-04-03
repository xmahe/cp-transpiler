#include "example.h"

static u32 Board___Logger___instance_count;
static u32 Board___DeviceController___instance_count;

void Board___Logger___Destruct(Board___Logger* self) {
    /* empty */
}

i32 Board___Logger___WriteLine(Board___Logger* self, u8* data, u32 len) {
    /* empty */
}

i32 Board___Logger___Flush(Board___Logger* self) {
    /* empty */
}

void Board___Logger___SetPrefix(Board___Logger* self, u8* prefix) {
    /* empty */
}

void Board___Logger___Construct(Board___Logger* self) {
    /* empty */
}

static i32 Board___DeviceController___CheckReady(Board___DeviceController* self) {
    /* empty */
}

static i32 Board___DeviceController___ValidateWindow(Board___DeviceController* self, u32 start, u32 len) {
    /* empty */
}

void Board___DeviceController___Destruct(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Transfer(Board___DeviceController* self, u8* tx, u8* rx, u32 len) {
    /* empty */
}

i32 Board___DeviceController___Lock(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Unlock(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Flush(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Read(Board___DeviceController* self, u8* buffer, u32 len) {
    /* empty */
}

i32 Board___DeviceController___Calibrate(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Reset(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Enable(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___Disable(Board___DeviceController* self) {
    /* empty */
}

i32 Board___DeviceController___IsEnabled(Board___DeviceController* self) {
    /* empty */
}

u32 Board___DeviceController___SampleRate(Board___DeviceController* self) {
    /* empty */
}

void Board___DeviceController___SetMode(Board___DeviceController* self, Board___DeviceMode mode) {
    /* empty */
}

void Board___DeviceController___SetState(Board___DeviceController* self, Board___DiagnosticState state) {
    /* empty */
}

i32 Board___DeviceController___IsReady(Board___DeviceController* self) {
    /* empty */
}

void Board___DeviceController___Construct(Board___DeviceController* self) {
    /* empty */
}

void Board___DeviceController___Construct(Board___DeviceController* self, u32 device_id, u32 sample_rate_hz) {
    /* empty */
}

void Board___DeviceRepository___Destruct(Board___DeviceRepository* self) {
    /* empty */
}

i32 Board___DeviceRepository___InitializeDefaults(Board___DeviceRepository* self) {
    /* empty */
}

i32 Board___DeviceRepository___Reset(Board___DeviceRepository* self) {
    /* empty */
}

i32 Board___DeviceRepository___SelectActive(Board___DeviceRepository* self, u32 id) {
    /* empty */
}

i32 Board___DeviceRepository___PrimeLogger(Board___DeviceRepository* self) {
    /* empty */
}

void Board___DeviceRepository___Construct(Board___DeviceRepository* self, u32 sample_rate_hz) {
    Board___DeviceController___Construct(&self->primary);
    Board___DeviceController___Construct(&self->secondary);
    Board___Logger___Construct(&self->logger);
}

u32 Board___ComputeChecksum(u8* data, u32 len) {
    return 0;
}

i32 Board___Boot() {
    return 0;
}

i32 Board___RunDiagnostics(u32 iterations) {
    return 0;
}

i32 Board___SampleAndReport(u8* buffer, u32 len) {
    return 0;
}

i32 Board___WarmUpLogger() {
    return 0;
}

i32 Board___ResetHardware() {
    return 0;
}

i32 Board___TickDiagnostics(u32 tick) {
    return 0;
}

i32 Board___ReportState(Board___DiagnosticState state) {
    return 0;
}

const char* Board___DeviceMode___ToString(Board___DeviceMode value) {
    switch (value) {
    case Board___DeviceMode___kDeviceModeOff: return "kDeviceModeOff";
    case Board___DeviceMode___kDeviceModeStandby: return "kDeviceModeStandby";
    case Board___DeviceMode___kDeviceModeActive: return "kDeviceModeActive";
    case Board___DeviceMode___kDeviceModeFault: return "kDeviceModeFault";
    case Board___DeviceMode___kDeviceModeSleep: return "kDeviceModeSleep";
    case Board___DeviceMode___kDeviceModeN: return "kDeviceModeN";
    default: return "<invalid>";
    }
}

const char* Board___DiagnosticState___ToString(Board___DiagnosticState value) {
    switch (value) {
    case Board___DiagnosticState___kDiagnosticStateIdle: return "kDiagnosticStateIdle";
    case Board___DiagnosticState___kDiagnosticStateCollecting: return "kDiagnosticStateCollecting";
    case Board___DiagnosticState___kDiagnosticStateReporting: return "kDiagnosticStateReporting";
    case Board___DiagnosticState___kDiagnosticStateFault: return "kDiagnosticStateFault";
    case Board___DiagnosticState___kDiagnosticStateN: return "kDiagnosticStateN";
    default: return "<invalid>";
    }
}

