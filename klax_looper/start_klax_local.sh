#!/bin/bash

export DISPLAY=:0

# Aller dans le dossier de l'app
cd /Users/leo/Documents/__PROJETS/Patatas/Klax/klax_looper

# Activer l'environnement virtuel
source venv/bin/activate

# Démarrer le script Python
python klax_midi.py