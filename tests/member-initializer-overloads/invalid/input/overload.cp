namespace Board {

fn Child::Construct(u32 left, u32 right) -> void {
    self->value = left + right;
}

fn Child::Construct(u32 first, u32 second) -> void {
    self->value = first + second;
}

fn Parent::Construct(u32 left, u32 right) -> void : child(left, right) {
}

}
