from skspatial.objects import Plane
from skspatial.objects import Points
import re
import math

with open('../Data/NotImproved/Sections/1000/1000WallSection.obj', 'r') as in_file:
    lines = in_file.readlines()
    points_array: [[float]] = []

    for line in lines:
        if line.find('v ') == 0:
            line_split = re.split(' ', line)
            point = [float(line_split[1]), float(line_split[2]), float(line_split[3])]
            points_array.append(point)

    points = Points(points_array)
    plane = Plane.best_fit(points)

    print('Plane point: ' + plane.point)
    print('Plane normal: ' + plane.normal)


