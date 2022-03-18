import os

path = os.path.join(os.curdir, "Split")
os.mkdir(path)

with open("meshes.obj", "r") as in_file:
    data = in_file.read().split('o ')

    mtllib = data[0]
    for i in range(1, len(data)):
        with open("Split/mesh_" + str(i) + ".obj", "w") as out_file:
            out_file.write(mtllib)
            out_file.write('o ')
            out_file.write(data[i])
