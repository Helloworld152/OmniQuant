use zeromq::{Socket, SocketRecv};
use prost::Message; // Trait for .decode()
use std::error::Error;

// Include generated proto code
pub mod omni {
    include!(concat!(env!("OUT_DIR"), "/omni.rs"));
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    println!("Starting Rust PULL Server (Protobuf)...");

    let mut socket = zeromq::PullSocket::new();
    socket.bind("tcp://127.0.0.1:5556").await?;

    println!("Listening on tcp://127.0.0.1:5556");

    loop {
        let msg = socket.recv().await?;
        
        if let Some(payload) = msg.get(0) {
            // Decode Protobuf
            match omni::EventFrame::decode(payload.as_ref()) {
                Ok(frame) => {
                    print!("Received Frame from [{}]: ", frame.source_id);
                    
                    match frame.payload {
                        Some(omni::event_frame::Payload::Tick(tick)) => {
                            println!("Tick {} {} Price={}", tick.exchange, tick.symbol, tick.last_price);
                        },
                        Some(_) => println!("Other event type"),
                        None => println!("Empty payload"),
                    }
                },
                Err(e) => eprintln!("Failed to decode Protobuf: {}", e),
            }
        }
    }
}