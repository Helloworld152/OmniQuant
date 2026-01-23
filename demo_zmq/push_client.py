import zmq
import time
import random

def run_client():
    context = zmq.Context()
    # Create a PUSH socket (Sender)
    # In our architecture, this is like the 'gateway'
    sender = context.socket(zmq.PUSH)
    sender.connect("tcp://localhost:5555")
    
    print("PUSH Client started, sending messages to tcp://localhost:5555...")
    
    count = 0
    while True:
        count += 1
        msg = f"Tick Update #{count}: Price {random.uniform(3000, 4000):.2f}"
        sender.send_string(msg)
        print(f"Sent: {msg}")
        time.sleep(1)

if __name__ == "__main__":
    run_client()
