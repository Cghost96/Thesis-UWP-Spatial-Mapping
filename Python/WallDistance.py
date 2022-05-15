import math

#  HoloLens - Right wall, left wall - computed in CloudCompare
# plane_points = [(2.069, 0.607, -1.447), (1.271, 0.375, 1.540)]
# plane_normals = [(-0.220762, 0.0020059, 0.975326), (-0.226781, 0.00450384, 0.973935)]

#  Lidar walk - Left wall, right wall
# plane_points = [(.873, .428, .368), (-2.01, .523, -.755)]
# plane_normals = [(.939918, -.0331853, .339784), (.94163, -.04742, .333294)]

# Lidar standstill whole - Left wall, right wall
# plane_points = [(-1.836, .376, .224), (1.251, .292, -.067)]
# plane_normals = [(.992703, -.005986, -.12044), (.992951, .00348, -.1185)]

#  Lidar standstill piece - Left wall, right wall
# plane_points = [(-1.814, .195, .396), (1.276, .066, .138)]
# plane_normals = [(.994724, -.0068258, -.102365), (.993377, .006591, -.114712)]

#  Lidar standstill 1 frame whole - Left wall, right wall
# plane_points = [(-1.83, .369, .305), (1.266, .26, .087)]
# plane_normals = [(.992758, -.00472116, -.120037), (.993053, .0041928, -.117594)]

#  Lidar standstill 1 frame piece - Left wall, right wall
plane_points = [(-1.816, .156, .412), (1.272, .053, .123)]
plane_normals = [(.994649, -.00890175, -.102927), (.993412, .00694525, -.114383)]


# https://www.geeksforgeeks.org/distance-between-a-point-and-a-plane-in-3-d/
def compute_distance(x, y, z, nx, ny, nz, D) -> float:
    # d = abs((A * x1 + B * y1 + C * z1 + D))
    d = nx * x + ny * y + nz * z + D
    e = math.sqrt(nx * nx + ny * ny + nz * nz)
    return d / e


pp0 = plane_points[0]
pp1 = plane_points[1]

pn0 = plane_normals[0]
pn1 = plane_normals[1]

p0 = plane_points[1]
p1 = plane_points[0]

D0 = -(pn0[0] * pp0[0] + pn0[1] * pp0[1] + pn0[2] * pp0[2])
D1 = -(pn1[0] * pp1[0] + pn1[1] * pp1[1] + pn1[2] * pp1[2])

print(str(compute_distance(p0[0], p0[1], p0[2], pn0[0], pn0[1], pn0[2], D0)))
print(str(compute_distance(p1[0], p1[1], p1[2], pn1[0], pn1[1], pn1[2], D1)))
