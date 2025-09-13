import cv2
from ultralytics import YOLO
import time
import requests
from threading import Thread
import json
import os

class TelegramNotifier:
    def __init__(self, bot_token, chat_id):
        self.bot_token = bot_token
        self.chat_id = chat_id
        self.api_url = f"https://api.telegram.org/bot{bot_token}/sendMessage"
        self.last_notification_time = 0
        self.notification_cooldown = 30  # detik antara notifikasi
        
    def send_message(self, message):
        """Mengirim pesan ke Telegram"""
        try:
            payload = {
                'chat_id': self.chat_id,
                'text': message,
                'parse_mode': 'HTML'
            }
            response = requests.post(self.api_url, data=payload, timeout=10)
            return response.status_code == 200
        except Exception as e:
            print(f"Error sending Telegram message: {e}")
            return False
            
    def should_send_notification(self):
        """Memeriksa apakah sudah waktunya untuk mengirim notifikasi"""
        current_time = time.time()
        if current_time - self.last_notification_time > self.notification_cooldown:
            self.last_notification_time = current_time
            return True
        return False

def load_config():
    """Memuat konfigurasi dari file atau environment variables"""
    config = {
        'bot_token': os.getenv('TELEGRAM_BOT_TOKEN', 'YOUR_BOT_TOKEN_HERE'),
        'chat_id': os.getenv('TELEGRAM_CHAT_ID', 'YOUR_CHAT_ID_HERE')
    }
    
    # Coba load dari file config.json jika ada
    try:
        with open('config.json', 'r') as f:
            file_config = json.load(f)
            config.update(file_config)
    except FileNotFoundError:
        pass
        
    return config

def ultra_fast_detection():
    # 1. Gunakan model terkecil
    model = YOLO("yolov8s-world.pt")  # NANO version - paling cepat
    
    # 2. Setup webcam dengan optimasi maksimal
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)    # Resolusi kecil
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
    cap.set(cv2.CAP_PROP_FPS, 30)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)       # Minimal buffer
    
    # 3. Gym objects keywords
    gym_keywords = ["person", "weight", "dumbbell", "barbell", "treadmill"]
    
    # 4. Inisialisasi notifier Telegram
    config = load_config()
    notifier = TelegramNotifier(config['bot_token'], config['chat_id'])
    
    # 5. FPS tracking
    fps_counter = 0
    start_time = time.time()
    last_inference_time = 0
    cached_detections = []
    object_count_history = []
    
    print("⚡ ULTRA FAST MODE - Target 30+ FPS")
    print("📷 Press 'q' to quit")
    
    while True:
        loop_start = time.time()
        
        ret, frame = cap.read()
        if not ret:
            break
        
        fps_counter += 1
        current_time = time.time()
        
        # Run inference hanya setiap 0.2 detik (5 FPS inference, 30 FPS display)
        if current_time - last_inference_time > 0.2:
            try:
                # Resize ke ukuran mini untuk inference super cepat
                tiny_frame = cv2.resize(frame, (320, 240))
                
                results = model.predict(
                    tiny_frame, 
                    conf=0.5,      # High confidence
                    verbose=False,
                    imgsz=320      # Explicit image size
                )
                
                # Filter dan scale hasil
                new_detections = []
                if results[0].boxes is not None:
                    scale_x = frame.shape[1] / 320
                    scale_y = frame.shape[0] / 240
                    
                    for box in results[0].boxes:
                        class_name = results[0].names[int(box.cls[0])].lower()
                        
                        # Quick gym object check
                        if any(keyword in class_name for keyword in gym_keywords):
                            x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                            new_detections.append({
                                'bbox': (int(x1*scale_x), int(y1*scale_y), 
                                        int(x2*scale_x), int(y2*scale_y)),
                                'class': class_name,
                                'conf': float(box.conf[0])
                            })
                
                cached_detections = new_detections
                last_inference_time = current_time
                
                # Simpan jumlah objek untuk riwayat
                object_count_history.append(len(cached_detections))
                if len(object_count_history) > 10:  # Simpan 10 pengukuran terakhir
                    object_count_history.pop(0)
                
                # Kirim notifikasi jika jumlah objek berubah signifikan
                if len(object_count_history) >= 5 and notifier.should_send_notification():
                    avg_count = sum(object_count_history) / len(object_count_history)
                    if avg_count > 0:
                        # Buat pesan notifikasi
                        class_counts = {}
                        for det in cached_detections:
                            cls = det['class']
                            class_counts[cls] = class_counts.get(cls, 0) + 1
                        
                        message = "🏋️ <b>Deteksi Peralatan Gym</b>\n"
                        message += f"📊 Total objek terdeteksi: {int(avg_count)}\n\n"
                        
                        for obj_type, count in class_counts.items():
                            message += f"• {obj_type.capitalize()}: {count}\n"
                        
                        message += f"\n⏰ {time.strftime('%H:%M:%S')}"
                        
                        # Kirim notifikasi di thread terpisah agar tidak mengganggu deteksi
                        Thread(target=notifier.send_message, args=(message,)).start()
                        print("📤 Notifikasi Telegram terkirim!")
                
            except Exception as e:
                print(f"⚠️ Inference error: {e}")
        
        # Draw cached detections (super fast)
        display_frame = frame.copy()
        for det in cached_detections:
            x1, y1, x2, y2 = det['bbox']
            
            # Simple drawing - minimal operations
            cv2.rectangle(display_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(display_frame, f"{det['class']}", (x1, y1-10), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
        
        # Calculate FPS
        if fps_counter % 30 == 0:
            elapsed = current_time - start_time
            current_fps = 30 / elapsed if elapsed > 0 else 0
            start_time = current_time
            print(f"🎯 FPS: {current_fps:.1f} | Objects: {len(cached_detections)}")
        
        # Minimal overlay
        cv2.putText(display_frame, f"Objects: {len(cached_detections)}", 
                   (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
        
        cv2.imshow("Ultra Fast Gym Detection", display_frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
        
        # Frame time management
        loop_time = time.time() - loop_start
        target_time = 1.0 / 30  # 30 FPS target
        if loop_time < target_time:
            time.sleep(target_time - loop_time)
    
    cap.release()
    cv2.destroyAllWindows()
    print("✅ Ultra fast detection stopped")

# Jalankan versi yang diinginkan
if __name__ == "__main__":
    # Option 2: Ultra fast version (simple dan cepat)
    ultra_fast_detection()