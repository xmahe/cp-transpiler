namespace Board {

fn Thermometer::Construct(u32 initial_reading) -> void {
    reading = initial_reading;
}

fn Thermometer::Reset() -> void {
    reading = 0;
}

fn Thermometer::Calibrate(u32 offset) -> void {
    reading += offset;
}

fn Thermometer::Apply(u32 offset) -> void {
    Reset();
    Calibrate(offset);
}

}
