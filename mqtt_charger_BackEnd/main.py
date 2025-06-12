import paho.mqtt.client as paho
from paho import mqtt
import requests
from datetime import datetime, timedelta
import logging
from charger import Charger
import pytz
import json


def get_this_month_energy_consumption_by_id(charger_id: int) -> float:
    url = f"https://smart-plug-api-server.onrender.com/energy_reports/energy_consumption/{charger_id}"
    try:
        res = requests.get(url)
        res.raise_for_status()  # Raise an error for bad status codes
        data = json.loads(res.text)
        # Extract the value and convert it to float
        energy_consumption = float(data[0]['energy_consumption'])
        return energy_consumption
    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")
        return 0.0

def get_current_price ():
    finland_tz = pytz.timezone("Europe/Helsinki")
    current_hour = datetime.now(finland_tz).hour
    current_day = datetime.now(finland_tz).day
    current_month = datetime.now(finland_tz).month
    current_year = datetime.now(finland_tz).year

    # Define the API URL
    url = "https://smart-plug-api-server.onrender.com/electric-price"
    params = {
        "year": current_year,
        "month": current_month,
        "day": current_day
    }

    # Get the current price data from the API
    response = requests.get(url, params=params)
    current_price_per_kwh = 0.0
    if response.status_code == 200:
        prices = response.json()
        current_price_per_kwh = prices[current_hour]  # Get the price for the current hour
        # Calculate the total cost
    else:
        current_price_per_kwh =0.0
    return float(current_price_per_kwh)



def post_energy_report(charger_id:int, start_time:datetime, end_time:datetime, energy_consumed):

    url= "https://smart-plug-api-server.onrender.com/energy_reports"
    data ={
        "charger_id": charger_id,
        "start_time": start_time.strftime("%Y-%m-%dT%H:%M:%S"),
        "end_time": end_time.strftime("%Y-%m-%dT%H:%M:%S"),
        "energy_consume": energy_consumed,
        "price":energy_consumed* 1#get_current_price()
        }
    x = requests.post(url=url, json = data)



# setting callbacks for different events to see if it works, print the message etc.
def on_connect(client, userdata, flags, rc, properties=None):
    print("CONNACK received with code %s." % rc)

# with this callback you can see if your publish was successful
def on_publish(client, userdata, mid, properties=None):
    print("mid: " + str(mid))

# print which topic was subscribed to
def on_subscribe(client, userdata, mid, granted_qos, properties=None):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))

def on_message(client, userdata, msg):
    charger_controller = userdata  # Access Charger instance
    subtopic = msg.topic.split("/")[-1]
    payload = msg.payload.decode("utf-8")
    
    logging.info(f"Received message on {msg.topic}: {payload}")
    try:
        match subtopic:
            case "state_ok":
                charger.update_state(payload)
            case "setpoint_ok":
                charger_controller.update_setpoint(payload)
            case "clock_query":
                now = datetime.now().strftime("%y%m%d%H%M%S")
                client.publish("clock", payload=now, qos=1)
            case "wh_query":
                logging.info("Energy consumption query received.")
                client.publish("wh", payload=get_this_month_energy_consumption_by_id(charger_id=3), qos=1)
            case "post_wh":
                end_time_str, value = payload.split(":")
                # Parse end_time
                end_time = datetime.strptime(end_time_str, "%y%m%d%H%M")
                start_time = end_time - timedelta(minutes=5)
                post_energy_report(charger_id=3,start_time=start_time,end_time=end_time,energy_consumed=value)
            case _:
                logging.warning(f"Unhandled topic: {msg.topic}")
    except Exception as e:
        logging.error(f"Error processing message: {e}")

charger = Charger(charger_id=1)

client = paho.Client(paho.CallbackAPIVersion.VERSION1,client_id="", userdata=charger, protocol=paho.MQTTv5)
client.on_connect = on_connect

# enable TLS for secure connection
client.tls_set(tls_version=mqtt.client.ssl.PROTOCOL_TLS)
# set username and password
client.username_pw_set("smartplug", "SmartPlug2025@")
# connect to HiveMQ Cloud on port 8883 (default for MQTT)
client.connect("4e8b61774358463c87428a6f4f8bf6c3.s1.eu.hivemq.cloud", 8883)

# setting callbacks, use separate functions like above for better visibility
client.on_subscribe = on_subscribe
client.on_message = on_message
client.on_publish = on_publish

# subscribe to all topics of encyclopedia by using the wildcard "#"
client.subscribe("smart_plug/#", qos=1)

# a single publish, this can also be done in loops, etc.
#client.publish("smart_plug/temperature", payload="hot", qos=1)

# loop_forever for simplicity, here you need to stop the loop manually
# you can also use loop_start and loop_stop
client.loop_forever()
