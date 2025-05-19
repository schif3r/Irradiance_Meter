from flask import Flask, jsonify, send_from_directory
import requests

app = Flask(__name__)

# === Replace with your actual ThingSpeak credentials ===
CHANNEL_ID = "2966722"
API_KEY = "78DWLBFEZD6V1CXY"  # <- Get this from ThingSpeak's API Keys tab

THINGSPEAK_URL = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json?results=1&api_key={API_KEY}"

@app.route('/api/data')
def get_data():
    try:
        response = requests.get(THINGSPEAK_URL)
        response.raise_for_status()
        feeds = response.json()['feeds'][0]

        data = {
            "irradiance": feeds.get("field1"),
            "voltage": feeds.get("field2"),
            "ldr": feeds.get("field3"),
            "derivative": feeds.get("field4"),
            "buzzer_status": feeds.get("field5")
        }
        return jsonify(data)
    except Exception as e:
        return jsonify({"error": str(e)})

@app.route('/')
def serve_dashboard():
    return send_from_directory('.', 'index.html')

if __name__ == '__main__':
    app.run(debug=True)