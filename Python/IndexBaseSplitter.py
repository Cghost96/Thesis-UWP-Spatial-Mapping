import re

with open("meshes (1).obj", "r") as in_file:
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

    with open("meshes_base_split.obj", "w") as out_file:
        current_mesh = -1
        for line in lines:
            if 'o ' in line:
                out_file.write(line)
                current_mesh = current_mesh + 1
            elif line.find('f ') == 0 and current_mesh > 0:
                line_split = re.split(' |//|\n', line)
                i1 = str(int(line_split[1]) - base_offsets[current_mesh - 1])
                i2 = str(int(line_split[3]) - base_offsets[current_mesh - 1])
                i3 = str(int(line_split[5]) - base_offsets[current_mesh - 1])
                out_file.write('f ' + i1 + '//' + i1 + ' ' +
                               i2 + '//' + i2 + ' ' +
                               i3 + '//' + i3 + '\n')
            else:
                out_file.write(line)
