namespace Board {

fn BootSerial(u32 port_id, u32 baud_rate) -> void {
}

fn ShutdownSerial() -> void {
}

fn PollSerial(u8* buffer, u32 length) -> i32 {
    return 0;
}

fn SerialStateName(UartState state) -> u8* {
    return 0;
}

fn DriverModeName(DriverMode mode) -> u8* {
    return 0;
}

fn SelectMode(u32 request) -> DriverMode {
    return kDriverModePolling;
}

fn NormalizeBaudRate(u32 baud_rate) -> u32 {
    return baud_rate;
}

fn IsPollingMode(DriverMode mode) -> i32 {
    return 0;
}

fn AcquireSerialPort(u32 port_id) -> maybe<u32> {
    return 0;
}

fn ReportSerialStats(SerialStats stats) -> void {
}

fn ResetSerialStats() -> void {
}

fn SnapshotSerialPort(u32 port_id) -> SerialSnapshot {
    SerialSnapshot snapshot;
    return snapshot;
}

fn TraceSerialEvent(u32 port_id, u8* message) -> void {
}

fn CollectSerialStats(u32 port_id) -> SerialStats {
    SerialStats stats;
    return stats;
}

fn FormatSerialConfig(SerialConfig config) -> u8* {
    return 0;
}

}
