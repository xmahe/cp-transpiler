namespace Board {

fn Inner::Construct() -> void {
    value = 1;
}

fn Inner::Destruct() -> void {
    value = 0;
}

fn Outer::Construct() -> void {
}

fn Outer::Destruct() -> void {
}

}
