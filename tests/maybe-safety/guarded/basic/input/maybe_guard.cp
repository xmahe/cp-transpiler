namespace Board {

fn UnwrapMaybe(maybe<u32> value) -> u32 {
    if (value.has_value()) {
        return value.value();
    }
    return 0;
}

}
