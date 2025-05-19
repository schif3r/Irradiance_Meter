from flask import Flask, jsonify, send_from_directory, render_template_string
import requests

app = Flask(__name__)

# ThingSpeak configuration
THINGSPEAK_CHANNEL_ID = "2966722"
THINGSPEAK_API_KEY = "YOUR_API_KEY"  # optional if the channel is public
THINGSPEAK_URL = f"https://api.thingspeak.com/channels/{THINGSPEAK_CHANNEL_ID}/feeds.json?results=1"

# Route: Root (Homepage)
@app.route('/')
def home():
    return render_template_string("""
        <h1>Solar Irradiance Dashboard</h1>
        <p>Use <code>/api/data</code> to fetch the latest sensor data.</p>
    """)

# Route: API data endpoint
@app.route('/api/data')
def get_data():
    try:
        response = requests.get(THINGSPEAK_URL)
        data = response.json()

        if 'feeds' in data and len(data['feeds']) > 0:
            feed = data['feeds'][0]
            result = {
                "irradiance": feed.get("field1"),
                "voltage": feed.get("field2"),
                "ldr_value": feed.get("field3"),
                "derivative": feed.get("field4"),
                "buzzer_status": feed.get("field5")
            }
            return jsonify(result)
        else:
            return jsonify({"error": "No data available"}), 404
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
