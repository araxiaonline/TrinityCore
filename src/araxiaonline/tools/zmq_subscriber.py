#!/usr/bin/env python3
"""
ZeroMQ Event Bus Subscriber - Test Script

Connects to the worldserver's ZMQ publisher and prints all received events.
Use this to verify spawn/encounter/player events are being published.

Usage:
    python3 zmq_subscriber.py [endpoint] [topic_filter]
    
Examples:
    python3 zmq_subscriber.py                    # All events on default endpoint
    python3 zmq_subscriber.py tcp://localhost:5555 world.spawn
    python3 zmq_subscriber.py tcp://localhost:5555 dungeon.
"""

import sys
import json
import zmq
from datetime import datetime

def main():
    endpoint = sys.argv[1] if len(sys.argv) > 1 else "tcp://localhost:5555"
    topic_filter = sys.argv[2] if len(sys.argv) > 2 else ""
    
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    print(f"Connecting to {endpoint}...")
    socket.connect(endpoint)
    
    # Subscribe to topic filter (empty string = all topics)
    socket.setsockopt_string(zmq.SUBSCRIBE, topic_filter)
    print(f"Subscribed to topics: '{topic_filter}*'")
    print("Waiting for events... (Ctrl+C to quit)\n")
    
    try:
        while True:
            message = socket.recv_string()
            
            # Parse topic and payload (format: "topic {json}")
            space_idx = message.find(' ')
            if space_idx > 0:
                topic = message[:space_idx]
                payload = message[space_idx + 1:]
                
                try:
                    data = json.loads(payload)
                    ts = datetime.fromtimestamp(data.get('ts', 0) / 1000)
                    
                    print(f"[{ts.strftime('%H:%M:%S.%f')[:-3]}] {topic}")
                    print(f"  Context: map={data.get('context', {}).get('map_id')}, "
                          f"instance={data.get('context', {}).get('instance_id')}, "
                          f"type={data.get('context', {}).get('type')}")
                    print(f"  Payload: {json.dumps(data.get('payload', {}), indent=None)}")
                    print()
                except json.JSONDecodeError:
                    print(f"[RAW] {message}\n")
            else:
                print(f"[RAW] {message}\n")
                
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        socket.close()
        context.term()

if __name__ == "__main__":
    main()
