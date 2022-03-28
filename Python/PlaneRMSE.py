from skspatial.objects import Plane
from skspatial.objects import Points
import re
import math
import locale

# Set to users preferred locale:
locale.setlocale(locale.LC_ALL, '')


# https://www.geeksforgeeks.org/distance-between-a-point-and-a-plane-in-3-d/
def compute_distance(x1, y1, z1) -> float:
    # d = abs((A * x1 + B * y1 + C * z1 + D))
    d = A * x1 + B * y1 + C * z1 + D
    e = math.sqrt(A * A + B * B + C * C)
    return d / e


points_array: [[float]] = []

with open('../Data/NotImproved/Sections/3500/3500WallSection.obj', 'r') as in_file:
    lines = in_file.readlines()

    for line in lines:
        if line.find('v ') == 0:
            line_split = re.split(' ', line)
            point = [float(line_split[1]), float(line_split[2]), float(line_split[3])]
            points_array.append(point)

plane = Plane.best_fit(Points(points_array))
p = plane.point
px = p[0]
py = p[1]
pz = p[2]
n = plane.normal
A = n[0]
B = n[1]
C = n[2]
D = -(A * px + B * py + C * pz)

with open('../Data/NotImproved/Sections/3500/3500WallSectionRMSE.csv', 'w') as out_file:
    out_file.write(('# Plane point: [' + str(px) + ', ' + str(py) + ', ' + str(pz) + ']').replace('.', ','))
    out_file.write(('\n# Plane normal: [' + str(A) + ', ' + str(B) + ', ' + str(C) + ']').replace('.', ','))
    out_file.write(
        ('\n# Plane equation: ' + str(A) + 'x + ' + str(B) + 'y + ' + str(C) + 'z  + ' + str(D) + ' = 0').replace('.',
                                                                                                                  ','))

    distances_sq: [float] = []
    max_d_neg: float = 0
    max_d_pos: float = 0
    for point in points_array:
        distance = compute_distance(point[0], point[1], point[2])
        if distance > max_d_pos:
            max_d_pos = distance
        if distance < max_d_neg:
            max_d_neg = distance
        out_file.write('\n' + str(distance).replace('.', ','))
        distances_sq.append(math.pow(distance, 2))

    sum_d_sq = sum(distances_sq)
    no_points = len(points_array)
    RMSE = math.sqrt(sum_d_sq / no_points)

    out_file.write('\n\n\nRMSE: ' + str(RMSE).replace('.', ','))
