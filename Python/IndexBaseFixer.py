with open("meshes.obj", "r") as in_file:
    lines = in_file.readlines()
    current_mesh = -1
    no_meshes = 0

    for line in lines:
        if 'o ' in line:
            no_meshes = no_meshes + 1

    base_offsets: [int] = [0] * no_meshes

    for line in lines:
        if 'o ' in line:
            current_mesh = current_mesh + 1
        if current_mesh > 0 and base_offsets[current_mesh] == 0:
            base_offsets[current_mesh] = base_offsets[current_mesh - 1]
        if 'v ' in line:
            base_offsets[current_mesh] = base_offsets[current_mesh] + 1

    with open("meshes_base_fixed.obj", "w") as out_file:
        current_mesh = -1
        for line in lines:
            if 'o ' in line:
                out_file.write(line)
                current_mesh = current_mesh + 1
            elif 'f ' in line and current_mesh > 0:
                line_split = line.split()
                line_split[1] = str(int(line_split[1]) + base_offsets[current_mesh - 1])
                line_split[2] = str(int(line_split[2]) + base_offsets[current_mesh - 1])
                line_split[3] = str(int(line_split[3]) + base_offsets[current_mesh - 1])
                out_file.write('f ' + line_split[1] + ' ' + line_split[2] + ' ' + line_split[3] + '\n')
            else:
                out_file.write(line)
