import cv2

RTSP_URL = "rtsp://10.116.88.38:8554/mystream"  # replace with your RTSP URL

def main():
    cap = cv2.VideoCapture(RTSP_URL, cv2.CAP_FFMPEG)
    # cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    cap.set(cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, 60000)  # 60 seconds
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # minimal buffering

    if not cap.isOpened():
        print("Failed to connect to RTSP stream.")
        return

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Frame grab failed.")
            break

        cv2.imshow("RTSP Stream", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
