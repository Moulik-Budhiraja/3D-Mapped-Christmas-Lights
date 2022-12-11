import cv2 as cv
import requests
import time
import json
import requests
import json
from ldam import *

with open('sample-positions.json', 'r') as f:
    positions = json.load(f)

Position.from_json(positions)

reset_tasks: Task = []


for p in Position.all().values():
    reset_tasks.append(Task(p, HSV(0, 0, 0), 0))


manager = ldamManager()


url = 'http://192.168.50.28'

h1 = 0
s1 = 0
v1 = 70

minx = 720
miny = 720
maxx = 0
maxy = 0

# Open camera

cap = cv.VideoCapture(0)

positions1 = {}

manager.upload(url, manager.format(reset_tasks))

for i in range(50):
    if i > 0:
        manager.upload(url, manager.format(
            [Task(Position.get(i - 1), HSV(0, 0, 0), 0), Task(Position.get(i), HSV(h1, s1, v1), 0)]))
        # requests.post(url, data={'position': f'{i - 1}',
        #                          "color": f'0,0,0'})
    else:
        manager.upload(url, manager.format([Task(Position.get(i), HSV(h1, s1, v1), 0)]))

    # r = requests.post(url, data={'position': f'{i}',
    #                              "color": f'{h1/360 * 255},{s1/100 * 255},{v1/100 * 255}'})

    time.sleep(1)

    ret, frame = cap.read()

    if not ret:
        print("Can't receive frame (stream end?). Exiting ...")
        break

    grey = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)
    blur = cv.GaussianBlur(grey, (5, 5), 0)

    # Brightest pixel
    minVal, maxVal, minLoc, maxLoc = cv.minMaxLoc(blur)

    # Draw circle
    cv.circle(frame, maxLoc, 10, (0, 0, 255), 2)

    for x, y in positions1.values():
        cv.circle(frame, (x, y), 2, (0, 255, 0))

    if maxLoc[0] < minx:
        minx = maxLoc[0]
    if maxLoc[1] < miny:
        miny = maxLoc[1]
    if maxLoc[0] > maxx:
        maxx = maxLoc[0]
    if maxLoc[1] > maxy:
        maxy = maxLoc[1]

    cv.line(frame, (minx, 0), (minx, 480), (255, 0, 0))
    cv.line(frame, (maxx, 0), (maxx, 480), (255, 0, 0))
    cv.line(frame, (0, miny), (640, miny), (255, 0, 0))
    cv.line(frame, (0, maxy), (640, maxy), (255, 0, 0))

    # Lines from left and bottom
    cv.line(frame, (0, maxLoc[1]), maxLoc, (0, 0, 255), 2)
    cv.line(frame, (maxLoc[0], 480), maxLoc, (0, 0, 255), 2)

    # Label pixel distance
    cv.putText(frame, f'{maxLoc[0]}', (maxLoc[0] + 10, maxLoc[1] + 10),
               cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)
    cv.putText(frame, f'{480 - maxLoc[1]}', (maxLoc[0] + 10,
               maxLoc[1] - 10), cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

    positions1[i] = maxLoc

    cv.imshow('frame', frame)

    if cv.waitKey(1) == ord('q'):
        break

input('Turn tree 90 degrees.\nPress enter to continue')

positions2 = {}

for i in range(50):

    r = requests.post(url, data={'position': f'{i}',
                                 "color": f'0,0,0'})

for i in range(50):
    if i > 0:
        requests.post(url, data={'position': f'{i - 1}',
                                 "color": f'0,0,0'})

    r = requests.post(url, data={'position': f'{i}',
                                 "color": f'{h1/360 * 255},{s1/100 * 255},{v1/100 * 255}'})

    time.sleep(1)

    ret, frame = cap.read()

    if not ret:
        print("Can't receive frame (stream end?). Exiting ...")
        break

    grey = cv.cvtColor(frame, cv.COLOR_BGR2GRAY)
    blur = cv.GaussianBlur(grey, (5, 5), 0)

    # Brightest pixel
    minVal, maxVal, minLoc, maxLoc = cv.minMaxLoc(blur)

    # Draw circle
    cv.circle(frame, maxLoc, 10, (0, 0, 255), 2)

    for x, y in positions2.values():
        cv.circle(frame, (x, y), 2, (0, 255, 0))

    if maxLoc[0] < minx:
        minx = maxLoc[0]
    if maxLoc[1] < miny:
        miny = maxLoc[1]
    if maxLoc[0] > maxx:
        maxx = maxLoc[0]
    if maxLoc[1] > maxy:
        maxy = maxLoc[1]

    cv.line(frame, (minx, 0), (minx, 480), (255, 0, 0))
    cv.line(frame, (maxx, 0), (maxx, 480), (255, 0, 0))
    cv.line(frame, (0, miny), (640, miny), (255, 0, 0))
    cv.line(frame, (0, maxy), (640, maxy), (255, 0, 0))

    # Lines from left and bottom
    cv.line(frame, (0, maxLoc[1]), maxLoc, (0, 0, 255), 2)
    cv.line(frame, (maxLoc[0], 480), maxLoc, (0, 0, 255), 2)

    # Label pixel distance
    cv.putText(frame, f'{maxLoc[0]}', (maxLoc[0] + 10, maxLoc[1] + 10),
               cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)
    cv.putText(frame, f'{480 - maxLoc[1]}', (maxLoc[0] + 10,
               maxLoc[1] - 10), cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

    positions2[i] = maxLoc

    cv.imshow('frame', frame)

    if cv.waitKey(1) == ord('q'):
        break


cap.release()

for key, value in positions2.items():
    positions2[key] = (value[0] - minx, value[1] - miny)

positions3d = {}

for key, value in positions1.items():
    positions3d[key] = (value[0], value[1], positions2[key][0])

# move every point down so that the lowest point is at 0

miny = min(positions3d.values(), key=lambda x: x[1])[1]

for key, value in positions3d.items():
    positions3d[key] = (value[0], value[1] - miny, value[2])

with open('positions3d4235.json', 'w') as f:
    json.dump(positions3d, f)
