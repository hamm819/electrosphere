import cv2
import numpy as np
from ultralytics import YOLO

def main():
    # 1. Load YOLO-World model
    print("🔄 Loading YOLO-World model...")
    model = YOLO("yolov8x-world.pt")
    
    # 2. Target gym objects (lowercase untuk matching)
    gym_objects = [
        "person", "treadmill", "dumbbell", "barbell", "bench press", 
        "kettlebell", "rowing machine", "elliptical trainer", 
        "leg press", "cable machine", "yoga mat", "exercise bike",
        "weight", "weights", "gym equipment", "fitness equipment"
    ]
    
    # 3. Setup webcam
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("❌ Error: Cannot open webcam")
        return
        
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    cap.set(cv2.CAP_PROP_FPS, 30)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1) 
    
    print("✅ YOLO-World Gym Detection Started")
    print("🎯 Hanya objek gym yang akan ditampilkan")
    print("📷 Press 'q' to quit...")
    
    frame_count = 0
    
    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("❌ Cannot read frame")
                break
            
            frame_count += 1
            
            # Run inference
            results = model.predict(frame, conf=0.25, verbose=False)
            
            # Create clean frame for drawing filtered results
            display_frame = frame.copy()
            detected_gym_objects = []
            
            # Filter and draw only gym objects
            if results[0].boxes is not None:
                for box in results[0].boxes:
                    class_id = int(box.cls[0])
                    class_name = results[0].names[class_id].lower()
                    confidence = float(box.conf[0])
                    
                    # Check if it's a gym object
                    is_gym_object = False
                    for gym_obj in gym_objects:
                        if (gym_obj.lower() in class_name or 
                            class_name in gym_obj.lower() or 
                            class_name == gym_obj.lower()):
                            is_gym_object = True
                            break
                    
                    # Only draw gym objects
                    if is_gym_object:
                        detected_gym_objects.append(f"{class_name} ({confidence:.2f})")
                        
                        # Get bounding box coordinates
                        x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                        x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)
                        
                        # Draw bounding box
                        cv2.rectangle(display_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                        
                        # Draw label
                        label = f"{class_name}: {confidence:.2f}"
                        label_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0]
                        cv2.rectangle(display_frame, (x1, y1 - label_size[1] - 10), 
                                    (x1 + label_size[0], y1), (0, 255, 0), -1)
                        cv2.putText(display_frame, label, (x1, y1 - 5), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
            
            # Add info overlay
            info_text = f"Frame: {frame_count} | Gym Objects: {len(detected_gym_objects)}"
            cv2.putText(display_frame, info_text, (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            cv2.putText(display_frame, "Press 'q' to quit", (10, 60), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            # Show detected gym objects in terminal
            if detected_gym_objects and frame_count % 30 == 0:  # Print every 30 frames
                print(f"🏋️ Detected: {', '.join(detected_gym_objects[:5])}")
            
            cv2.imshow("YOLO-World - GYM OBJECTS ONLY", display_frame)
            
            # Exit on 'q'
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
    except KeyboardInterrupt:
        print("\n🛑 Stopped by user")
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        cap.release()
        cv2.destroyAllWindows()
        print("✅ Program selesai")

if __name__ == "__main__":
    main()