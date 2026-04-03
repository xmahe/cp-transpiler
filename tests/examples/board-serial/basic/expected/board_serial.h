#ifndef CPLUS_BOARD_SERIAL_H_H
#define CPLUS_BOARD_SERIAL_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool has_value;
    u32 value;
} __cplus_maybe_u32;

typedef enum {
    Board___UartState___kUartStateUninitialized,
    Board___UartState___kUartStateReady,
    Board___UartState___kUartStateBusy,
    Board___UartState___kUartStateError,
    Board___UartState___kUartStateN,
} Board___UartState;

const char* Board___UartState___ToString(Board___UartState value);

typedef enum {
    Board___DriverMode___kDriverModePolling,
    Board___DriverMode___kDriverModeInterrupt,
    Board___DriverMode___kDriverModeDma,
    Board___DriverMode___kDriverModeN,
} Board___DriverMode;

const char* Board___DriverMode___ToString(Board___DriverMode value);

typedef struct Board___SerialPort {
    u32 _port_id; // private
    u32 _baud_rate; // private
    Board___UartState _state; // private
    Board___DriverMode _mode; // private
    __cplus_maybe_u32 next_port_id;
    u32 tx_count;
    u32 rx_count;
    u32 error_count;
    u32 last_activity_tick;
} Board___SerialPort;

typedef struct Board___SerialConfig {
    u32 port_id;
    u32 baud_rate;
    Board___DriverMode mode;
    __cplus_maybe_u32 retry_limit;
    u8 trace_enabled;
} Board___SerialConfig;

typedef struct Board___SerialStats {
    u32 tx_bytes;
    u32 rx_bytes;
    u32 framing_errors;
    u32 parity_errors;
    u32 overrun_errors;
    u32 dropped_bytes;
    u32 retried_frames;
} Board___SerialStats;

typedef struct Board___SerialSnapshot {
    u32 port_id;
    Board___UartState state;
    Board___DriverMode mode;
    u32 tx_bytes;
    u32 rx_bytes;
    u32 error_count;
} Board___SerialSnapshot;

void Board___SerialPort___Destruct(Board___SerialPort* self);
void Board___SerialPort___Start(Board___SerialPort* self);
void Board___SerialPort___Stop(Board___SerialPort* self);
i32 Board___SerialPort___Send(Board___SerialPort* self, u8* data, u32 length);
void Board___SerialPort___Flush(Board___SerialPort* self);
void Board___SerialPort___Reconfigure(Board___SerialPort* self, u32 baud_rate);
void Board___SerialPort___ResetCounters(Board___SerialPort* self);
i32 Board___SerialPort___IsReady(Board___SerialPort* self);
Board___DriverMode Board___SerialPort___Mode(Board___SerialPort* self);
void Board___SerialPort___Construct(Board___SerialPort* self, u32 port_id, u32 baud_rate);
void Board___SerialConfig___Reset(Board___SerialConfig* self);
void Board___SerialConfig___Construct(Board___SerialConfig* self, u32 port_id, u32 baud_rate);
void Board___BootSerial(u32 port_id, u32 baud_rate);
void Board___ShutdownSerial();
i32 Board___PollSerial(u8* buffer, u32 length);
u8* Board___SerialStateName(Board___UartState state);
u8* Board___DriverModeName(Board___DriverMode mode);
Board___DriverMode Board___SelectMode(u32 request);
u32 Board___NormalizeBaudRate(u32 baud_rate);
i32 Board___IsPollingMode(Board___DriverMode mode);
__cplus_maybe_u32 Board___AcquireSerialPort(u32 port_id);
void Board___ReportSerialStats(Board___SerialStats stats);
void Board___ResetSerialStats();
Board___SerialSnapshot Board___SnapshotSerialPort(u32 port_id);
void Board___TraceSerialEvent(u32 port_id, u8* message);
Board___SerialStats Board___CollectSerialStats(u32 port_id);
u8* Board___FormatSerialConfig(Board___SerialConfig config);

#endif
