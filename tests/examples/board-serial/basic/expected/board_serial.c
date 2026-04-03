#include "board_serial.h"

static u32 Board___SerialPort____instance_count;

void Board___SerialPort___Destruct(Board___SerialPort* self) {
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
    return Board___DriverMode___kDriverModePolling;
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

void Board___ReportSerialStats(Board___SerialStats stats) {
    /* empty */
}

void Board___ResetSerialStats() {
    /* empty */
}

Board___SerialSnapshot Board___SnapshotSerialPort(u32 port_id) {
    Board___SerialSnapshot snapshot;
    return snapshot;
}

void Board___TraceSerialEvent(u32 port_id, u8* message) {
    /* empty */
}

Board___SerialStats Board___CollectSerialStats(u32 port_id) {
    Board___SerialStats stats;
    return stats;
}

u8* Board___FormatSerialConfig(Board___SerialConfig config) {
    return 0;
}

const char* Board___UartState___ToString(Board___UartState value) {
    switch (value) {
    case Board___UartState___kUartStateUninitialized: return "kUartStateUninitialized";
    case Board___UartState___kUartStateReady: return "kUartStateReady";
    case Board___UartState___kUartStateBusy: return "kUartStateBusy";
    case Board___UartState___kUartStateError: return "kUartStateError";
    case Board___UartState___kUartStateN: return "kUartStateN";
    default: return "<invalid>";
    }
}

const char* Board___DriverMode___ToString(Board___DriverMode value) {
    switch (value) {
    case Board___DriverMode___kDriverModePolling: return "kDriverModePolling";
    case Board___DriverMode___kDriverModeInterrupt: return "kDriverModeInterrupt";
    case Board___DriverMode___kDriverModeDma: return "kDriverModeDma";
    case Board___DriverMode___kDriverModeN: return "kDriverModeN";
    default: return "<invalid>";
    }
}

