from flask import Flask, jsonify, send_from_directory
import requests
import os

app = Flask(__name__, static_folder='.')

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

if __name__ == '__main__':
    app.run(debug=True)
