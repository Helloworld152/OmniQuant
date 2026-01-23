use std::io::Result;

fn main() -> Result<()> {
    // Compile the proto file found in the project root
    prost_build::compile_protos(&["../../protocol/omni.proto"], &["../../protocol/"])?;
    Ok(())
}
