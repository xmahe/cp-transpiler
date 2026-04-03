namespace Board {

fn Logger::Construct() -> void {
}

fn Logger::Destruct() -> void {
}

fn Logger::Ping() -> void {
}

export_c fn Boot() -> i32 {
    Logger logger;
    logger.Ping();
    return 7;
}

}
