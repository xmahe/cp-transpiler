namespace Board {

fn Logger::Destroy() -> void {
}

fn Sensor::Destroy() -> void {
}

fn Boot() -> void {
    Logger logger;
    {
        Sensor sensor;
        return;
    }
}

}
