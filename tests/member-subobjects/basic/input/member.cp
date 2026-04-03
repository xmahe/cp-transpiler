namespace Board {

fn Inner::Construct() -> void {
    value = 1;
}

fn Inner::Destroy() -> void {
    value = 0;
}

fn Outer::Construct() -> void {
}

fn Outer::Destroy() -> void {
}

}
