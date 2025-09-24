# Conqueror Tank - Arduino RC Tank Controller

![Arduino](https://img.shields.io/badge/Arduino-Compatible-brightgreen)
![C++](https://img.shields.io/badge/Language-C++-blue)


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
- **Code PIN et verrouillage** : contrôle d’accès par télécommande avec gestion des tentatives. (le code pin de base est : **1111**)
- **Fonction reboot via watchdog** pour réinitialiser le microcontrôleur si nécessaire.

## Touches de la télécommande IR

<table>
<tr>
<td>

| Touche       | Fonction                  | Code         |
|--------------|--------------------------|-------------|
| Flèches ↑↓←→ | Déplacement              | Voir ci-dessous |
| Avant        | Déplacement avant        | `0xB946FF00` |
| Arrière      | Déplacement arrière      | `0xEA15FF00` |
| Gauche       | Déplacement gauche       | `0xBB44FF00` |
| Droite       | Déplacement droite       | `0xBC43FF00` |
| OK           | Klaxon                   | `0xBF40FF00` |
| 1            | Vitesse 1                | `0xE916FF00` |
| 2            | Vitesse 2                | `0xE619FF00` |
| 3            | Vitesse 3                | `0xF20DFF00` |
| 4            | Détecteur                | `0xF30CFF00` |
| 5            | Frein                    | `0xE718FF00` |
| 6            | Vitesse automatique      | `0xA15EFF00` |
| 7            | Pilote automatique       | `0xF708FF00` |
| 0            | Menu paramètres          | `0xAD52FF00` |

</td>
<td>

<p align="center">
  <img src="https://github.com/user-attachments/assets/bb0b26a3-3b62-4267-b84b-5004fbe5095e" alt="Front View" width="300">
</p>

</td>
</tr>
</table>


## Matériel requis

- Carte Arduino compatible (devloper pour Mega2560)
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

## Images
<p align="center">
<img src="https://github.com/user-attachments/assets/91ceb904-e7c3-40d3-9e58-bea9f4743c09" alt="Front View" width="500" height="auto">
<img src="https://github.com/user-attachments/assets/ed5b23ee-d5ec-4145-93c7-c00f6ccf086f" alt="LCD Dashboard" width="500" height="auto">
<img src="https://github.com/user-attachments/assets/bb0b26a3-3b62-4267-b84b-5004fbe5095e" alt="Télécomande IR" width="300" height="auto">
<p>
