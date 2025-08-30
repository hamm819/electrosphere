import threading, time
from inference import InferencePipeline
from inference.core.interfaces.stream.sinks import render_boxes

API_KEY = "cKLJvDkcCOKqDCZirSTk"

pipeline = InferencePipeline.init(
    model_id="yolov8n-640",
    video_reference=0,
    on_prediction=render_boxes,
    api_key=API_KEY
)

def run():
    pipeline.start()

t = threading.Thread(target=run, daemon=True)
t.start()

# Untuk berhenti: tekan Ctrl+C di shell
try:
    while t.is_alive():
        time.sleep(1)
except KeyboardInterrupt:
    pipeline.terminate()