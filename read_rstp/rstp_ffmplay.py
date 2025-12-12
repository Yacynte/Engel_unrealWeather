import ffmpeg
import numpy as np
import cv2

URL = "rtsp://10.116.88.38:8554/mystream"

# Force TCP like ffplay does
process = (
    ffmpeg
    .input(URL, rtsp_transport='tcp', timeout='5000000')  # 5s
    .output('pipe:', format='rawvideo', pix_fmt='bgr24')
    .run_async(pipe_stdout=True, pipe_stderr=True)
)

WIDTH = 640   # put your real width here
HEIGHT = 320   # put your real height here

while True:
    raw = process.stdout.read(WIDTH * HEIGHT * 3)
    if not raw:
        break
    frame = np.frombuffer(raw, dtype=np.uint8).reshape((HEIGHT, WIDTH, 3))
    cv2.imshow("stream", frame)
    if cv2.waitKey(1) == ord("q"):
        break
