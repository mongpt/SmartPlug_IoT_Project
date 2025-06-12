import paho.mqtt.client as mqtt
import json
import logging
from datetime import datetime

logging.basicConfig(level=logging.INFO)

class Charger:
    def __init__(self, charger_id=3, state=False, set_point=10, energy_consumption=10):
        self.charger_id = charger_id
        self.state = state
        self.set_point = set_point
        self.energy_consumption = energy_consumption
        self.energy_report = {}  # Format: {start_time: {"end_time": ..., "energy_consumed": ...}}

    def update_state(self, value):
        """Updates the charger's state."""
        self.state = bool(int(value))  # Assuming payload is "0" or "1"
        logging.info(f"Charger {self.charger_id} state updated: {self.state}")

    def update_set_point(self, value):
        """Updates the charger's power setpoint."""
        self.set_point = int(value)
        logging.info(f"Charger {self.charger_id} setpoint updated: {self.set_point}A")

    def get_set_point(self):
        return self.set_point