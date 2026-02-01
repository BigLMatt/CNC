import json
import os
import time

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
POSITION_FILE = os.path.join(BASE_DIR, "position.json")

def load_position():
    if not os.path.exists(POSITION_FILE):
        print("File not found")
        return 0
    try:
        with open(POSITION_FILE) as file:
            return json.load(file)["position_um"]
    except (json.JSONDecodeError, KeyError):
        print("json decoding error")
        return 0

def save_position(position_um):
    os.makedirs(os.path.dirname(POSITION_FILE), exist_ok=True)
    with open(POSITION_FILE, "w") as file:
        json.dump({"position_um":int(position_um),"timestamp": int(time.time())}, file)
