namespace Board {

fn SpiA::Construct() -> void {
}

fn SpiA::Transfer(u8* tx, u8* rx, u32 len) -> i32 {
    return 1;
}

fn SpiB::Construct() -> void {
}

fn SpiB::Transfer(u8* tx, u8* rx, u32 len) -> i32 {
    return 2;
}

fn Device::Construct() -> void {
}

fn Device::Send(u8* tx, u8* rx, u32 len) -> i32 {
    return control_spi.Transfer(tx, rx, len) + data_spi.Transfer(tx, rx, len);
}

}
