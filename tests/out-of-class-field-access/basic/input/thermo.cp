namespace Board {

fn Thermometer::construct(u32 initial_reading) -> void {
    reading = initial_reading;
}

fn Thermometer::Calibrate(u32 offset) -> void {
    reading += offset;
}

fn Thermometer::Read() -> u32 {
    return reading;
}

fn ResetThermometer(u32 value) -> void {
    u32 adjusted;
    adjusted = value + 1;
}

}
