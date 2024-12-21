fn main() {
    println!("does it get here");
    //println!("cargo:rustc-link-search=native=lib");
    println!("cargo:rustc-link-lib=dynamic=card");
}
