import cv2
from ultralytics import YOLO

# 1. Muat model YOLO-World
model = YOLO("yolov8x-world.pt")

# 2. Daftar objek yang ingin dideteksi
gym_objects = [
    "person", "treadmill", "dumbbell", "barbell", "bench press", "kettlebell",
    "rowing machine", "elliptical trainer", "leg press", "cable machine", 
    "yoga mat", "exercise bike"
]

# 3. Buka webcam
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

print("🎯 YOLO-World Gym Detection Started")
print("📷 Tekan 'q' untuk keluar...")

# 4. Loop real-time
try:
    while True:
        ret, frame = cap.read()
        if not ret:
            print("❌ Tidak bisa membaca frame dari kamera")
            break
        
        # 5. Prediksi tanpa filter classes (deteksi semua, filter nanti)
        results = model.predict(frame, conf=0.25, verbose=False)
        
        # 6. Filter hasil berdasarkan gym objects
        if results[0].boxes is not None:
            # Ambil nama kelas yang terdeteksi
            detected_classes = []
            filtered_boxes = []
            
            for i, box in enumerate(results[0].boxes):
                class_id = int(box.cls[0])
                class_name = results[0].names[class_id].lower()
                
                # Cek apakah termasuk objek gym yang diinginkan
                for gym_obj in gym_objects:
                    if gym_obj.lower() in class_name or class_name in gym_obj.lower():
                        detected_classes.append(class_name)
                        filtered_boxes.append(box)
                        break
            
            if detected_classes:
                print(f"🔍 Terdeteksi: {', '.join(set(detected_classes))}")
        
        # 7. Gambar hasil (semua deteksi untuk sementara)
        annotated = results[0].plot()
        
        # Tambahkan info di frame
        cv2.putText(annotated, f"Target: {len(gym_objects)} gym objects", 
                   (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.putText(annotated, "Press 'q' to quit", 
                   (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
        cv2.imshow("YOLO-World Gym Equipment Detection", annotated)
        
        # 8. Keluar dengan 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

except Exception as e:
    print(f"❌ Error: {str(e)}")

finally:
    # 9. Tutup
    cap.release()
    cv2.destroyAllWindows()
    print("✅ Program selesai.")