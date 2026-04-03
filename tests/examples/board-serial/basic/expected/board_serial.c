#include "board_serial.h"

static u32 Board___SerialPort____instance_count;

void Board___SerialPort___Destroy(Board___SerialPort* self) {
    /* empty */
}


void Board___SerialPort___Start(Board___SerialPort* self) {
    /* empty */
}

void Board___SerialPort___Stop(Board___SerialPort* self) {
    /* empty */
}

i32 Board___SerialPort___Send(Board___SerialPort* self, u8* data, u32 length) {
    /* empty */
}

void Board___SerialPort___Flush(Board___SerialPort* self) {
    /* empty */
}

void Board___SerialPort___Reconfigure(Board___SerialPort* self, u32 baud_rate) {
    /* empty */
}

void Board___SerialPort___ResetCounters(Board___SerialPort* self) {
    /* empty */
}

i32 Board___SerialPort___IsReady(Board___SerialPort* self) {
    /* empty */
}

DriverMode Board___SerialPort___Mode(Board___SerialPort* self) {
    /* empty */
}

static u32 Board___SerialPort___InstanceCount() {
    /* empty */
}

void Board___SerialPort___Construct(Board___SerialPort* self, u32 port_id, u32 baud_rate) {
    /* empty */
}

void Board___SerialConfig___Reset(Board___SerialConfig* self) {
    /* empty */
}

void Board___SerialConfig___Construct(Board___SerialConfig* self, u32 port_id, u32 baud_rate) {
    /* empty */
}

void ShutdownSerial() {
    /* empty */
}

i32 PollSerial(u8* buffer, u32 length) {
    /* empty */
}

u8* SerialStateName(UartState state) {
    /* empty */
}

u8* DriverModeName(DriverMode mode) {
    /* empty */
}

DriverMode SelectMode(u32 request) {
    /* empty */
}

u32 NormalizeBaudRate(u32 baud_rate) {
    /* empty */
}

i32 IsPollingMode(DriverMode mode) {
    /* empty */
}

__cplus_maybe_u32 AcquireSerialPort(u32 port_id) {
    /* empty */
}

void ReportSerialStats(SerialStats stats) {
    /* empty */
}

void ResetSerialStats() {
    /* empty */
}

SerialSnapshot SnapshotSerialPort(u32 port_id) {
    /* empty */
}

void TraceSerialEvent(u32 port_id, u8* message) {
    /* empty */
}

SerialStats CollectSerialStats(u32 port_id) {
    /* empty */
}

u8* FormatSerialConfig(Board___SerialConfig config) {
    /* empty */
}

void Board___BootSerial(u32 port_id, u32 baud_rate) {
    /* empty */
}

void Board___ShutdownSerial() {
    /* empty */
}

i32 Board___PollSerial(u8* buffer, u32 length) {
    return 0;
}

u8* Board___SerialStateName(UartState state) {
    return 0;
}

u8* Board___DriverModeName(DriverMode mode) {
    return 0;
}

DriverMode Board___SelectMode(u32 request) {
    return kDriverModePolling;
}

u32 Board___NormalizeBaudRate(u32 baud_rate) {
    return baud_rate;
}

i32 Board___IsPollingMode(DriverMode mode) {
    return 0;
}

__cplus_maybe_u32 Board___AcquireSerialPort(u32 port_id) {
    return 0;
}

void Board___ReportSerialStats(SerialStats stats) {
    /* empty */
}

void Board___ResetSerialStats() {
    /* empty */
}

SerialSnapshot Board___SnapshotSerialPort(u32 port_id) {
    SerialSnapshot snapshot;
    return snapshot;
}

void Board___TraceSerialEvent(u32 port_id, u8* message) {
    /* empty */
}

SerialStats Board___CollectSerialStats(u32 port_id) {
    SerialStats stats;
    return stats;
}

u8* Board___FormatSerialConfig(Board___SerialConfig config) {
    return 0;
}

const char* Board___UartState___ToString(Board___UartState value) {
    switch (value) {
    case kUartStateUninitialized: return "kUartStateUninitialized";
    case kUartStateReady: return "kUartStateReady";
    case kUartStateBusy: return "kUartStateBusy";
    case kUartStateError: return "kUartStateError";
    case kUartStateN: return "kUartStateN";
    default: return "<invalid>";
    }
}

const char* Board___DriverMode___ToString(Board___DriverMode value) {
    switch (value) {
    case kDriverModePolling: return "kDriverModePolling";
    case kDriverModeInterrupt: return "kDriverModeInterrupt";
    case kDriverModeDma: return "kDriverModeDma";
    case kDriverModeN: return "kDriverModeN";
    default: return "<invalid>";
    }
}
