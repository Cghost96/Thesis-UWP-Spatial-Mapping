import re
import math
import locale

# Set to users preferred locale:
locale.setlocale(locale.LC_ALL, '')

D_THRESHOlD = 0.035  # cm
D_CM = 35

#  Right wall, left wall, floor - computed in CloudCompare
plane_points = [(2.069, 0.607, -1.447), (1.271, 0.375, 1.540), (1.706, -1.510, 0.053)]
plane_normals = [(-0.220762, 0.0020059, 0.975326), (-0.226781, 0.00450384, 0.973935), (0.00004, 0.999996, 0.002974)]


# https://www.geeksforgeeks.org/distance-between-a-point-and-a-plane-in-3-d/
def compute_distance(x, y, z, nx, ny, nz) -> float:
    # d = abs((A * x1 + B * y1 + C * z1 + D))
    d = nx * x + ny * y + nz * z + D
    e = math.sqrt(nx * nx + ny * ny + nz * nz)
    return d / e


with open('../Data/Improved/8000Model.obj', 'r') as in_file:
    in_lines = in_file.readlines()

    with open('../Data/Improved/8000Improved_' + str(D_CM) + '.obj', 'w') as out_file:
        p_temp = []

        for line in in_lines:
            if line.find('v ') == 0:
                line_split = re.split(' ', line)
                point = [float(line_split[1]), float(line_split[2]), float(line_split[3])]
                p_temp.append(point)

        for i in range(0, len(plane_points)):
            pp = plane_points[i]
            pn = plane_normals[i]
            D = -(pn[0] * pp[0] + pn[1] * pp[1] + pn[2] * pp[2])

            for j in range(0, len(p_temp)):
                d = compute_distance(p_temp[j][0], p_temp[j][1], p_temp[j][2], pn[0], pn[1], pn[2])
                if -D_THRESHOlD <= d <= D_THRESHOlD:
                    new_point = [p_temp[j][0] - pn[0] * d, p_temp[j][1] - pn[1] * d, p_temp[j][2] - pn[2] * d]
                    p_temp[j] = new_point

        v_index = 0
        for line in in_lines:
            if line.find('v ') == 0:
                line_p = p_temp[v_index]
                v_index = v_index + 1
                out_file.write(
                    'v ' + str(line_p[0]) + ' ' + str(line_p[1]) + ' ' + str(line_p[2]) + '\n')
            else:
                out_file.write(line)
