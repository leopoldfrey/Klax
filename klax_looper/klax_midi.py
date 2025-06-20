# python -m venv venv
# source venv/bin/activate
# pip install -r requirements.txt

import rtmidi
import json
import sys
import time
import os
from pythonosc.udp_client import SimpleUDPClient
from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer

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
        if debug:
            print(f"CC - Channel: {channel}, Control: {controller_number}, Value: {controller_value}")
        data.send_message("/midi", [channel, controller_number, controller_value])

def choose_midi_input(port_index):
    midiin = rtmidi.MidiIn()
    ports = midiin.get_ports()
    #print(ports)

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

def choose_midi_input_by_name(port_name):
    def normalize(s):
        return s.lower().replace(" ", "").replace("-", "")
    midiin = rtmidi.MidiIn()
    ports = midiin.get_ports()

    if not ports:
        print("Aucun port MIDI disponible.")
        exit(1)

    for i, name in enumerate(ports):
        if normalize(port_name) in normalize(name):
            midiin.open_port(i)
            print(f"Port MIDI sélectionné (nom): {name}")
            return midiin

    print(f"Port MIDI avec le nom '{port_name}' introuvable.")
    print("Ports disponibles :")
    for i, p in enumerate(ports):
        print(f"{i}: {p}")
    exit(1)

def choose_midi_output_by_name(port_name):
    def normalize(s):
        return s.lower().replace(" ", "").replace("-", "")
    midiout = rtmidi.MidiOut()
    ports = midiout.get_ports()
    if not ports:
        print("Aucun port MIDI OUT disponible.")
        exit(1)
    for i, name in enumerate(ports):
        if normalize(port_name) in normalize(name):
            midiout.open_port(i)
            print(f"Port MIDI OUT sélectionné (nom): {name}")
            return midiout
    print(f"Port MIDI OUT avec le nom '{port_name}' introuvable.")
    print("Ports disponibles :")
    for i, p in enumerate(ports):
        print(f"{i}: {p}")
    exit(1)

def osc_to_midi_handler(address, *args):
    # args = liste d'octets (ex: [240, 67, 16, 4, 1, 247])
    if debug:
        print(f"OSC reçu {address} {args}")
    if midiout:
        # Envoie les octets tels quels (brut, y compris SysEx)
        midiout.send_message(list(args))

def main():
    global debug, midiout
    config = load_config(CONFIG_FILE)

    midi_in_port_name = config.get("midi_in_port_name")
    midi_out_port_name = config.get("midi_out_port_name")
    osc_ip = config.get("osc_ip", "127.0.0.1")
    osc_in_port = config.get("osc_in_port", 9001)
    osc_out_port = config.get("osc_out_port", 9000)
    debug = config.get("debug", True)

    osc_client = SimpleUDPClient(osc_ip, osc_out_port)
    midiin = choose_midi_input_by_name(midi_in_port_name)
    midiin.set_callback(lambda msg, _: midi_callback(msg, osc_client))

    # MIDI OUT
    midiout = None
    if midi_out_port_name:
        midiout = choose_midi_output_by_name(midi_out_port_name)

    # OSC IN
    dispatcher = Dispatcher()
    dispatcher.map("/midi_raw", osc_to_midi_handler)
    server = BlockingOSCUDPServer(("0.0.0.0", osc_in_port), dispatcher)

    print(f"Envoi OSC vers {osc_ip}:{osc_out_port} - Réception OSC sur port {osc_in_port} - Ctrl+C pour quitter.")
    try:
        import threading
        osc_thread = threading.Thread(target=server.serve_forever, daemon=True)
        osc_thread.start()
        while True:
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("Arret...")
    finally:
        midiin.close_port()
        del midiin
        if midiout:
            midiout.close_port()
            del midiout

if __name__ == "__main__":
    main()
