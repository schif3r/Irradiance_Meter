from flask import Flask, jsonify, send_from_directory
import requests
import os
import joblib
import numpy as np

app = Flask(__name__, static_folder='.')

# Load the trained ML model and label encoder
model = joblib.load('weather_model.pkl')
label_encoder = joblib.load('label_encoder.pkl')

# Replace with your actual ThingSpeak Channel ID and API Key
THINGSPEAK_CHANNEL_ID = '2966722'
API_KEY = '78DWLBFEZD6V1CXY'  # You can leave this blank if not required for public read
BASE_URL = f'https://api.thingspeak.com/channels/{THINGSPEAK_CHANNEL_ID}/feeds.json?results=1'

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

@app.route('/api/data')
def get_data():
    try:
        response = requests.get(BASE_URL)
        data = response.json()
        feed = data['feeds'][0]

        return jsonify({
            "irradiance": feed['field1'],
            "voltage": feed['field2'],
            "ldr": feed['field3'],
            "derivative": feed['field4'],
            "buzzer": feed['field5']
        })

    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/api/weather')
def get_weather_prediction():
    try:
        response = requests.get(BASE_URL)
        data = response.json()
        feed = data['feeds'][0]

        # Extract and convert required fields
        irradiance = float(feed['field1'])
        voltage = float(feed['field2'])
        ldr = int(feed['field3'])
        derivative = float(feed['field4'])

        # Prepare input for model
        features = np.array([[irradiance, voltage, ldr, derivative]])
        prediction_encoded = model.predict(features)[0]
        prediction_label = label_encoder.inverse_transform([prediction_encoded])[0]

        return jsonify({
            "predicted_weather": prediction_label,
            "irradiance": irradiance,
            "voltage": voltage,
            "ldr": ldr,
            "derivative": derivative
        })

    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
