import cv2
from ultralytics import YOLO

# 1. Muat model YOLO-World (otomatis download pertama kali)
model = YOLO("yolov8x-world.pt")

# 2. Daftar objek yang ingin dideteksi (bebas tambah/hapus)
desired = ["person"]

# 3. Buka webcam
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

# 4. Loop real-time
while True:
    ret, frame = cap.read()
    if not ret:
        break

    # 5. Inference YOLO-World
    results = model.predict(frame, classes=desired, conf=0.25)

    # 6. Gambar hasil
    annotated = results[0].plot()
    cv2.imshow("YOLO-World Webcam", annotated)

    # 7. Keluar dengan 'q'
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# 8. Tutup
cap.release()
cv2.destroyAllWindows()