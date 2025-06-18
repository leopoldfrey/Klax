# python -m venv venv
# source venv/bin/activate
# pip install -r requirements.txt

import rtmidi
import json
import sys
import os
from pythonosc.udp_client import SimpleUDPClient

CONFIG_FILE = "config.json"

def load_config(path):
    if not os.path.exists(path):
        print(f"Fichier de configuration '{path}' introuvable.")
        exit(1)
    with open(path, 'r') as f:
        return json.load(f)

def midi_callback(event, data=None):
    message, _ = event
    if message[0] & 0xF0 == 0xB0:  # Control Change
        channel = (message[0] & 0x0F) + 1
        controller_number = message[1]
        controller_value = message[2]
        print(f"CC - Channel: {channel}, Control: {controller_number}, Value: {controller_value}")
        data.send_message("/midi", [channel, controller_number, controller_value])

def choose_midi_input(port_index):
    midiin = rtmidi.MidiIn()
    ports = midiin.get_ports()

    if not ports:
        print("Aucun port MIDI disponible.")
        exit(1)

    if 0 <= port_index < len(ports):
        midiin.open_port(port_index)
        print(f"Port MIDI sélectionné (config): {ports[port_index]}")
        return midiin
    else:
        print(f"Port MIDI invalide dans la config : {port_index}")
        print("Ports disponibles :")
        for i, p in enumerate(ports):
            print(f"{i}: {p}")
        exit(1)

def main():
    config = load_config(CONFIG_FILE)

    midi_port_index = config.get("midi_port_index", 0)
    osc_ip = config.get("osc_ip", "127.0.0.1")
    osc_port = config.get("osc_port", 8000)

    osc_client = SimpleUDPClient(osc_ip, osc_port)
    midiin = choose_midi_input(midi_port_index)
    midiin.set_callback(lambda msg, _: midi_callback(msg, osc_client))

    print(f"Envoi OSC vers {osc_ip}:{osc_port} — Ctrl+C pour quitter.")
    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("Arrêt...")
    finally:
        midiin.close_port()
        del midiin

if __name__ == "__main__":
    main()
