#!/bin/bash

# Aller dans le dossier de l'app
cd /boot/shared/klax

# Activer l'environnement virtuel
source venv/bin/activate

# Démarrer le patch Pure Data en mode nogui
#pd -nogui looper.pd &
pd looper.pd &

# Démarrer le script Python
python klax_midi.py