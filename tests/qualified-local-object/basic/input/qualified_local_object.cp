namespace Drivers {

fn HelloSayer::Construct() -> void {
}

fn HelloSayer::Destruct() -> void {
}

fn HelloSayer::SayIt() -> void {
}

export_c fn Main() -> void {
    Drivers::HelloSayer hs;
    hs.SayIt();
}

}
