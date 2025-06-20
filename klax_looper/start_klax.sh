#!/bin/bash

export DISPLAY=:0

# Aller dans le dossier de l'app
cd /home/klax/Documents/GitHub/Klax/klax_looper

# Activer l'environnement virtuel
source venv/bin/activate

# Démarrer le patch Pure Data en mode nogui
#pd -nogui looper.pd &
pd looper.pd &

# Démarrer le script Python
python klax_midi.py