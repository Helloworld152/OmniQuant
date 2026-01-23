import zmq
import time

def run_server():
    context = zmq.Context()
    # Create a PULL socket (Receiver)
    # In our architecture, this is like the 'core_router'
    receiver = context.socket(zmq.PULL)
    receiver.bind("tcp://*:5555")
    
    print("PULL Server started, waiting for messages on tcp://*:5555...")
    
    while True:
        message = receiver.recv_string()
        print(f"Received: {message}")

if __name__ == "__main__":
    run_server()
