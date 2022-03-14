meshes: dict[str, int] = {}

with open("meshes.obj", "r") as in_file:
    lines = in_file.readlines()
    mesh_id: str
    for line in lines:
        if "o mesh_" in line:
            line_split = line.split()
            mesh_id = line_split[0] + line_split[1]
        if "# Timestamp update" in line:
            line_split = line.split()
            meshes[mesh_id] = int(line_split[3])

sorted_meshes = sorted(meshes.items(), key=lambda x: x[1], reverse=True)

with open("meshes.obj", "a") as out_file:
    for m in sorted_meshes:
        out_file.write("# " + str(m[0]) + ", " + str(m[1]) + "\n")
