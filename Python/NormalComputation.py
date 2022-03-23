import math
import re

with open('../TestData/emulator_sample.obj', 'r') as in_file:
    with open('../TestData/emulator_sample_face_normals.obj', 'w') as out_file:
        lines = in_file.readlines()
        out_file.write(lines[0])

        vertices: list[list[float]] = []
        faces: list[tuple[int, int, int]] = []
        normals: list[list[float]] = []

        for line in lines:
            if line.find('v ') == 0:
                line_split = re.split(' ', line)
                vertices.append([float(line_split[1]), float(line_split[2]), float(line_split[3])])
                out_file.write(line)
            if line.find('f ') == 0:
                line_split = re.split(' |//|\n', line)
                faces.append((int(line_split[1]), int(line_split[3]), int(line_split[5])))

        for face in faces:
            v_1 = vertices[face[0] - 1]
            v_2 = vertices[face[1] - 1]
            v_3 = vertices[face[2] - 1]

            edge_1 = [v_2[0] - v_1[0], v_2[1] - v_1[1], v_2[2] - v_1[2]]
            edge_2 = [v_3[0] - v_1[0], v_3[1] - v_1[1], v_3[2] - v_1[2]]
            normal = [
                edge_1[1] * edge_2[2] - edge_1[2] * edge_2[1],
                edge_1[0] * edge_2[2] - edge_1[2] * edge_2[0],
                edge_1[0] * edge_2[1] - edge_1[1] * edge_2[0]
            ]
            l = math.sqrt(math.pow(normal[0], 2) + math.pow(normal[1], 2) + math.pow(normal[2], 2))
            normalized = [normal[0] / l, -normal[1] / l, normal[2] / l] if l != 0 else [normal[0], -normal[1], normal[2]]
            normals.append(normalized)

            out_file.write('vn ' + str(normalized[0]) + ' ' + str(normalized[1]) + ' ' + str(normalized[2]) + '\n')

        out_file.write('s off\n')

        face_no = 1
        for face in faces:
            out_file.write('f ' +
                           str(face[0]) + '//' + str(face_no) + ' ' +
                           str(face[1]) + '//' + str(face_no) + ' ' +
                           str(face[2]) + '//' + str(face_no) + '\n'
                           )
            face_no = face_no + 1
