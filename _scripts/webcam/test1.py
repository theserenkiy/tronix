import cv2

# cv2.namedWindow("preview")
print("Opening cam...")
vc = cv2.VideoCapture(0)
print("Done")
vc.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
vc.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)


if vc.isOpened(): # try to get the first frame
    rval, frame = vc.read()
else:
    rval = False

nimg = 0
print("reading webcam...")
rval, frame = vc.read()
while rval:
	# cv2.imshow("preview", frame)
	cv2.imwrite("images/"+str(nimg).rjust(4,'0')+".jpg",frame)
	nimg+=1
	rval, frame = vc.read()
	key = cv2.waitKey(500)
	if key == 27: # exit on ESC
		break

vc.release()
# cv2.destroyWindow("preview")