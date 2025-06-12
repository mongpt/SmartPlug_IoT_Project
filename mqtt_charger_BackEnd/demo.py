import requests
import pytz
from datetime import datetime

# Define Finland timezone
finland_tz = pytz.timezone("Europe/Helsinki")

# Get current local time in Finland
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

if response.status_code == 200:
    prices = response.json()
    current_price_per_kwh = prices[current_hour]  # Get the price for the current hour
    # Assuming you have the actual electricity consumption in kWh
    consumption_kwh = 0.2  # Example of a very small consumption value (in kWh)

    # Calculate the total cost
    total_cost = consumption_kwh * current_price_per_kwh
    total_cost_float = float(total_cost)
    print(f"Total cost for {consumption_kwh} kWh at the current price: {total_cost_float}")

else:
    print("Failed to retrieve data:", response.status_code)


