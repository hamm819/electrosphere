import cv2
from ultralytics import YOLO
import time

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
    
    # 4. FPS tracking
    fps_counter = 0
    start_time = time.time()
    last_inference_time = 0
    cached_detections = []
    
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
    # Uncomment salah satu:
    
    # Option 1: Threading version (smooth tapi complex)
    # detector = OptimizedGymDetector()
    # detector.run()
    
    # Option 2: Ultra fast version (simple dan cepat)
    ultra_fast_detection()