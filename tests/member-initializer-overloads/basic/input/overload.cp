namespace Board {

fn Child::Construct(u32 value) -> void {
    self->value = value;
}

fn Child::Construct(f32 value) -> void {
    self->value = 0;
}

fn Child::Destruct() -> void {
    self->value = 0;
}

fn Parent::Construct(u32 value) -> void : child(value) {
}

fn Parent::Destruct() -> void {
}

}
