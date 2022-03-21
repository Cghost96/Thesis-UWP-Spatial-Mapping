import re
import numpy as np

with open('meshes_reversed.obj', 'r') as in_file:
    with open('meshes_face_normals.obj', 'w') as out_file:
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

            edge_1 = np.array([v_2[0] - v_1[0], v_2[1] - v_1[1], v_2[2] - v_1[2]])
            edge_2 = np.array([v_3[0] - v_1[0], v_3[1] - v_1[1], v_3[2] - v_1[2]])

            normal = np.cross(edge_1, edge_2)
            normals.append([normal[0], normal[1], normal[2]])

            out_file.write('vn ' + str(normal[0]) + ' ' + str(normal[1]) + ' ' + str(normal[2]) + '\n')

        out_file.write('s off\n')

        face_no = 1
        for face in faces:
            out_file.write('f ' +
                           str(face[0]) + '//' + str(face_no) + ' ' +
                           str(face[1]) + '//' + str(face_no) + ' ' +
                           str(face[2]) + '//' + str(face_no) + ' ' +
                           '\n'
                           )
            face_no = face_no + 1
