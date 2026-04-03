#ifndef CPLUS_BOARD_SERIAL_H_H
#define CPLUS_BOARD_SERIAL_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool has_value;
    u32 value;
} __cplus_maybe_u32;

typedef enum {
    kUartStateUninitialized,
    kUartStateReady,
    kUartStateBusy,
    kUartStateError,
    kUartStateN,
} Board___UartState;

const char* Board___UartState___ToString(Board___UartState value);

typedef enum {
    kDriverModePolling,
    kDriverModeInterrupt,
    kDriverModeDma,
    kDriverModeN,
} Board___DriverMode;

const char* Board___DriverMode___ToString(Board___DriverMode value);

typedef struct Board___SerialPort {
    u32 _port_id; // private
    u32 _baud_rate; // private
    UartState _state; // private
    DriverMode _mode; // private
    __cplus_maybe_u32 next_port_id;
    u32 tx_count;
    u32 rx_count;
    u32 error_count;
    u32 last_activity_tick;
} Board___SerialPort;

typedef struct Board___SerialConfig {
    u32 port_id;
    u32 baud_rate;
    DriverMode mode;
    __cplus_maybe_u32 retry_limit;
    u8 trace_enabled;
} Board___SerialConfig;

void Board___SerialDriver___Start(Board___SerialDriver* self);
void Board___SerialDriver___Stop(Board___SerialDriver* self);
i32 Board___SerialDriver___Send(Board___SerialDriver* self, u8* data, u32 length);
void Board___SerialDriver___Flush(Board___SerialDriver* self);
void Board___SerialPort___Construct(Board___SerialPort* self, u32 port_id, u32 baud_rate);
void Board___SerialPort___Destroy(Board___SerialPort* self);
void Board___SerialPort___Start(Board___SerialPort* self);
void Board___SerialPort___Stop(Board___SerialPort* self);
i32 Board___SerialPort___Send(Board___SerialPort* self, u8* data, u32 length);
void Board___SerialPort___Flush(Board___SerialPort* self);
void Board___SerialPort___Reconfigure(Board___SerialPort* self, u32 baud_rate);
void Board___SerialPort___ResetCounters(Board___SerialPort* self);
i32 Board___SerialPort___IsReady(Board___SerialPort* self);
DriverMode Board___SerialPort___Mode(Board___SerialPort* self);
void Board___SerialConfig___Construct(Board___SerialConfig* self, u32 port_id, u32 baud_rate);
void Board___SerialConfig___Reset(Board___SerialConfig* self);
void ShutdownSerial();
i32 PollSerial(u8* buffer, u32 length);
u8* SerialStateName(UartState state);
u8* DriverModeName(DriverMode mode);
DriverMode SelectMode(u32 request);
u32 NormalizeBaudRate(u32 baud_rate);
i32 IsPollingMode(DriverMode mode);
__cplus_maybe_u32 AcquireSerialPort(u32 port_id);
void ReportSerialStats(SerialStats stats);
void ResetSerialStats();
SerialSnapshot SnapshotSerialPort(u32 port_id);
void TraceSerialEvent(u32 port_id, u8* message);
SerialStats CollectSerialStats(u32 port_id);
u8* FormatSerialConfig(Board___SerialConfig config);
void Board___BootSerial(u32 port_id, u32 baud_rate);
void Board___ShutdownSerial();
i32 Board___PollSerial(u8* buffer, u32 length);
u8* Board___SerialStateName(UartState state);
u8* Board___DriverModeName(DriverMode mode);
DriverMode Board___SelectMode(u32 request);
u32 Board___NormalizeBaudRate(u32 baud_rate);
i32 Board___IsPollingMode(DriverMode mode);
__cplus_maybe_u32 Board___AcquireSerialPort(u32 port_id);
void Board___ReportSerialStats(SerialStats stats);
void Board___ResetSerialStats();
SerialSnapshot Board___SnapshotSerialPort(u32 port_id);
void Board___TraceSerialEvent(u32 port_id, u8* message);
SerialStats Board___CollectSerialStats(u32 port_id);
u8* Board___FormatSerialConfig(Board___SerialConfig config);

#endif
