namespace Board {

fn Child::Construct(u32 value) -> void {
    self->value = value;
}

fn Child::Destruct() -> void {
    value = 0;
}

fn Parent::Construct(u32 primary_value, u32 secondary_value) -> void : primary(primary_value), secondary(secondary_value) {
}

fn Parent::Destruct() -> void {
}

}
