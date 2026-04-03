namespace Board {

fn Logger::Flush() -> void {
}

fn Thermometer::Apply() -> void {
    logger.Flush();
}

fn Thermometer::UseLocalLogger() -> void {
    Logger logger;
    logger.Flush();
}

}
