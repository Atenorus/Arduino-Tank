# Conqueror Tank - Arduino RC Tank Controller

## Description
Ce projet est un système de contrôle pour un tank télécommandé Arduino, intégrant à la fois des commandes manuelles via télécommande IR et un mode automatique avec capteurs ultrasons. Le tank dispose de fonctionnalités avancées telles que la gestion des vitesses, des modes de sécurité, un tableau de bord LCD, un buzzer/klaxon configurable et la sauvegarde des paramètres dans l’EEPROM.

## Fonctionnalités principales

- **Contrôle via télécommande IR** : avancer, reculer, tourner à gauche/droite, freiner, klaxon.
- **Modes automatiques** :
  - **AutoSpeed** : ajuste la vitesse en fonction de la distance aux obstacles.
  - **Full Auto** : pilotage automatique basé sur le capteur ultrason pour éviter les collisions.
- **Sécurité** : arrêt automatique ou bip sonore si un obstacle est trop proche.
- **Gestion des vitesses** : trois vitesses ajustables et mémorisables dans l’EEPROM.
- **LCD I2C 16x2** :
  - Affiche l’état du radar, klaxon, vitesse et modes (manuel/auto).
  - Veille automatique avec extinction du rétroéclairage après période d’inactivité.
- **Buzzer/Klaxon configurable** : plusieurs styles de sons et alertes de proximité.
- **EEPROM** : sauvegarde et restauration des paramètres (vitesse, modes, klaxon, code PIN).
- **Code PIN et verrouillage** : contrôle d’accès par télécommande avec gestion des tentatives.
- **Fonction reboot via watchdog** pour réinitialiser le microcontrôleur si nécessaire.

## Matériel requis

- Carte Arduino compatible (ex. Arduino Uno, Nano, MKR WiFi 1010)
- Module télécommande IR et récepteur
- Capteur ultrason (HC-SR04 ou équivalent)
- LCD I2C 16x2
- Buzzer
- Pilotes moteurs (L298N ou équivalent)
- Moteurs et châssis tank

## Librairies utilisées

- **IRremote** – pour la télécommande IR
- **LiquidCrystal_I2C** – pour l’affichage LCD
- **Wire** – pour I2C (inclus par défaut dans Arduino IDE)
- **EEPROM** – pour la sauvegarde des paramètres
- **avr/wdt** – pour le reboot via watchdog

## Installation

1. Installer les librairies nécessaires dans l’IDE Arduino.
2. Connecter le matériel selon le schéma des pins définies dans le code.
3. Téléverser le code `.ino` sur la carte Arduino.
4. Initialiser le code PIN via la télécommande IR si nécessaire.
5. Profiter du contrôle manuel ou automatique du tank.

## Organisation du code

- **`setup()`** : initialise le matériel, LCD, télécommande IR, et charge les paramètres depuis l’EEPROM.
- **`loop()`** : gère la lecture de la télécommande, le mode automatique, la sécurité, l’affichage LCD et l’ajustement automatique de la vitesse.
- **Fonctions modulaires** : gestion des moteurs, capteur ultrason, buzzer/klaxon, menu de paramètres, EEPROM, et code PIN.
